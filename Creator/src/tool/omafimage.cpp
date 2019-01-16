
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "Heif.h"

#include "GridImageItem.h"
#include "HEVCCodedImageItem.h"
#include "HEVCDecoderConfiguration.h"

// property boxes for images
#include "DescriptiveProperty.h"

#include "omaf/parser/h265parser.hpp"

#include "localcommandline.h"
#include "localoptional.h"
#include "buildinfo.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>

class FileNotFoundError : public std::runtime_error
{
public:
    FileNotFoundError() : std::runtime_error("FileNotFoundError") {}
};

class H265StdIoStream : public H265InputStream
{
public:
    H265StdIoStream(std::istream& aStream);

    uint8_t getNextByte() override;
    bool eof() override;

    // This implementation is not able to rewind more than 4 consecutive bytes (unless 4 other bytes are read in between)
    void rewind(size_t) override;

private:
    std::istream& mStream;
    std::vector<uint8_t> mBuffer; // always hold at least four bytes as required by H265Parser
    std::size_t mBufferSize = 0;
    std::size_t mReadOffset = 0;
    bool mEof = false;

    void fillBuffer();
};

H265StdIoStream::H265StdIoStream(std::istream& aStream)
    : mStream(aStream)
    , mBuffer(1024)
{
    fillBuffer();
}

void H265StdIoStream::fillBuffer()
{
    size_t writeOffset = 0;
    if (mBufferSize)
    {
        // preserve at least 4 last bytes
        writeOffset = std::min(mBufferSize, size_t(4));
        std::copy(&mBuffer[0] + mBufferSize - writeOffset,
                  &mBuffer[0] + mBufferSize,
                  &mBuffer[0]);
    }

    // readsome doesn't work on Windows as we'd hope, so let's just keep track of how much we got
    std::streamsize bytesRead = mStream.tellg();
    mStream.read(reinterpret_cast<char*>(&mBuffer[0] + writeOffset),
                 mBuffer.size() - writeOffset);
    if (mStream.eof())
    {
        // eof and other errors are handled by dealing with bytesRead
        mStream.clear();
    }
    bytesRead = mStream.tellg() - bytesRead;
    mReadOffset = writeOffset;
    mBufferSize = size_t(bytesRead) + writeOffset;

    mEof = bytesRead == 0;
}

uint8_t H265StdIoStream::getNextByte()
{
    if (mReadOffset == mBufferSize)
    {
        fillBuffer();
    }
    if (!mEof)
    {
        auto value = mBuffer[mReadOffset];
        ++mReadOffset;
        return value;
    }
    else
    {
        return 0;
    }
}

void H265StdIoStream::rewind(size_t aOffset)
{
    assert(mReadOffset > aOffset);
    mReadOffset -= aOffset;
}

bool H265StdIoStream::eof()
{
    if (mReadOffset == mBufferSize)
    {
        fillBuffer();
    }
    return mEof;
}

void setImageDataFromHevc(HEIFPP::Heif* heif, std::string hevcFilePath, HEIFPP::HEVCCodedImageItem& image)
{
    std::vector<uint8_t> hevcData;
    std::vector<uint8_t> configs;

    H265Parser parser;
    std::ifstream input(hevcFilePath, std::ios::binary);
    H265StdIoStream h265Input(input);
    ParserInterface::AccessUnit au;

    ParserInterface::AccessUnit accessUnit = {};  // Make it empty
    std::vector<uint8_t> currNalUnitData;
    bool firstImageFound = false; // support reading just the first frame from video hevc files
    bool nextImageEncountered = false;
    while (!h265Input.eof() && !nextImageEncountered)
    {
        currNalUnitData.clear();
        auto type = parser.readNextNalUnit(h265Input, currNalUnitData);
        if (currNalUnitData.size() == 0)
        {
            // skip the nal header to scan for the next one
            while (h265Input.getNextByte() == 0 && !h265Input.eof())
            {
                // skip
            }
        }
        else
        {
            if (type == H265::H265NalUnitType::SPS ||
                type == H265::H265NalUnitType::PPS ||
                type == H265::H265NalUnitType::VPS)
            {
                std::copy(currNalUnitData.begin(), currNalUnitData.end(),
                          std::back_inserter(configs));
            }
            else if ((type >= H265::H265NalUnitType::CODED_SLICE_TRAIL_N &&
                      type <= H265::H265NalUnitType::CODED_SLICE_RASL_R) ||
                     (type >= H265::H265NalUnitType::CODED_SLICE_BLA_W_LP &&
                      type <= H265::H265NalUnitType::CODED_SLICE_CRA))
            {
                std::vector<uint8_t> nalUnitRBSP;
                parser.convertToRBSP(currNalUnitData, nalUnitRBSP);
                Parser::BitStream bitstr(nalUnitRBSP);
                H265::NalUnitHeader naluHeader;
                H265::SliceHeader sliceHeader;
                parser.parseNalUnitHeader(bitstr, naluHeader);

                // we just read the first bit here instead of using parser.parseSliceHeader, as that function requires
                // storing data about SPS and PPS and that's not really required for other work here.
                unsigned int firstSliceSegmentInPicFlag = bitstr.readBits(1);

                if (firstSliceSegmentInPicFlag)
                {
                    if (firstImageFound)
                    {
                        nextImageEncountered = true;
                    }
                    firstImageFound = true;
                }

                if (!nextImageEncountered)
                {
                    // apparently we want to stick a 0 before the data, it was in the original code
                    if (hevcData.size() == 0)
                    {
                        hevcData.push_back(0u);
                    }
                    std::copy(currNalUnitData.begin(), currNalUnitData.end(),
                              std::back_inserter(hevcData));
                }
            }
        }
    }

    auto decoderConfig = new HEIFPP::HEVCDecoderConfiguration(heif);

    decoderConfig->setConfig(&configs[0], configs.size());
    image.setDecoderConfiguration(decoderConfig);
    image.setItemData(&hevcData[0], hevcData.size());
}

HEIFPP::GridImageItem* write360GridHeif(HEIFPP::Heif* heif,
                                        std::vector<std::string>& hevcTiles,
                                        uint32_t origWidth,
                                        uint32_t origHeight,
                                        uint32_t tileWidth,
                                        uint32_t tileHeight,
                                        HEIF::FramePackingProperty framepacking)
{
    uint32_t colCount = std::ceil(float(origWidth) / tileWidth);
    uint32_t rowCount = std::ceil(float(origHeight) / tileHeight);

    // create image grid
    auto grid = new HEIFPP::GridImageItem(heif, colCount, rowCount);
    grid->setSize(origWidth, origHeight);

    for (auto row = 0u; row < rowCount; row++)
    {
        for (auto col = 0u; col < colCount; col++)
        {
            auto image = new HEIFPP::HEVCCodedImageItem(heif);
            image->setHidden(true);

            auto& tilePath = hevcTiles[row * colCount + col];

            setImageDataFromHevc(heif, tilePath, *image);

            grid->setImage(col, row, image);
        }
    }

    // create projection format and framepacking omaf properties
    auto projectionFormatProperty     = new HEIFPP::ProjectionFormatProperty(heif);
    projectionFormatProperty->mFormat = HEIF::OmafProjectionType::EQUIRECTANGULAR;
    grid->addProperty(projectionFormatProperty, true);

    if (framepacking != HEIF::FramePackingProperty::MONOSCOPIC)
    {
        auto framePackingProperty                       = new HEIFPP::FramePackingProperty(heif);
        framePackingProperty->mHeifFramePackingProperty = framepacking;
        grid->addProperty(framePackingProperty, true);
    }

    return grid;
}

HEIFPP::HEVCCodedImageItem* write360Heif(HEIFPP::Heif* heif, std::string hevcImage, HEIF::FramePackingProperty framepacking)
{
    auto image = new HEIFPP::HEVCCodedImageItem(heif);
    setImageDataFromHevc(heif, hevcImage, *image);

    // create projection format and framepacking omaf properties
    auto projectionFormatProperty     = new HEIFPP::ProjectionFormatProperty(heif);
    projectionFormatProperty->mFormat = HEIF::OmafProjectionType::EQUIRECTANGULAR;
    image->addProperty(projectionFormatProperty, true);

    if (framepacking != HEIF::FramePackingProperty::MONOSCOPIC)
    {
        auto framePackingProperty                       = new HEIFPP::FramePackingProperty(heif);
        framePackingProperty->mHeifFramePackingProperty = framepacking;
        image->addProperty(framePackingProperty, true);
    }

    return image;
}

struct Dimensions {
    uint32_t width;
    uint32_t height;
};

bool operator==(const Dimensions& a, const Dimensions& b)
{
    return a.width == b.width && a.height == b.height;
}

LocalCopy::Optional<Dimensions> readHevcDimensions(std::string aFilename)
{
    H265Parser parser;
    std::ifstream input(aFilename, std::ios::binary);

    if (input)
    {
        H265StdIoStream h265Input(input);
        ParserInterface::AccessUnit au;

        ParserInterface::AccessUnit accessUnit = {};  // Make it empty
        std::vector<uint8_t> currNalUnitData;
        while (!h265Input.eof())
        {
            currNalUnitData.clear();
            auto type = parser.readNextNalUnit(h265Input, currNalUnitData);
            if (currNalUnitData.size() == 0)
            {
                // skip the nal header to scan for the next one
                while (h265Input.getNextByte() == 0 && !h265Input.eof())
                {
                    // skip
                }
            }
            else
            {
                if (type == H265::H265NalUnitType::SPS)
                {
                    std::vector<uint8_t> rbsp;
                    H265Parser::convertToRBSP(currNalUnitData, rbsp);
                    Parser::BitStream bitstr(rbsp);
                    H265::SequenceParameterSet sps;
                    H265::NalUnitHeader naluHeader;
                    H265Parser::parseNalUnitHeader(bitstr, naluHeader);
                    H265Parser::parseSPS(bitstr, sps);
                    return { { sps.mPicWidthInLumaSamples, sps.mPicHeightInLumaSamples } };
                }
            }
        }
    }
    return {};
}

uint32_t digitOfChar(char ch)
{
    if (isdigit(ch))
    {
        return uint32_t(ch - '0');
    }
    else
    {
        throw LocalCopy::CommandLine::ParseError(std::string(1, ch) + " is not a digit");
    }
}

Dimensions parseDimensions(std::string aString)
{
    enum {
        WIDTH,
        HEIGHT
    } state = WIDTH;

    Dimensions dimensions {};

    for (size_t i = 0; i < aString.size(); ++i)
    {
        char ch = aString[i];
        switch (state)
        {
        case WIDTH:
            {
                if (ch == 'x' || ch == ',')
                {
                    state = HEIGHT;
                }
                else
                {
                    dimensions.width = dimensions.width * 10 + digitOfChar(ch);
                }
            } break;
        case HEIGHT:
            {
                dimensions.height = dimensions.height * 10 + digitOfChar(ch);
            } break;
        }
    }

    return dimensions;
}

struct SourceArguments
{
    HEIF::FramePackingProperty framepacking;
    LocalCopy::Optional<Dimensions> dimensions; // dimensions, if any
    std::string remaining; // the remaining part after :, or the complete thing if there was no :
};

SourceArguments parseSourceArguments(std::string aStr)
{
    SourceArguments arguments;
    std::string value;
    std::set<std::string> keywords;

    enum {
        SUFFIX_OR_ARGUMENTS,
        SUFFIX
    } state = SUFFIX_OR_ARGUMENTS;

    // "<=" as we handle the last character specially
    for (size_t i = 0; i <= aStr.size(); ++i)
    {
        bool end = i == aStr.size();
        char ch = end ? 0 : aStr[i];

        switch (state)
        {
        case SUFFIX_OR_ARGUMENTS:
            {
                if (end)
                {
                    arguments.remaining = std::move(value);
                }
                else if (ch == ',' || ch == ':')
                {
                    if (value.size() && isdigit(value[0]))
                    {
                        if (arguments.dimensions)
                        {
                            throw LocalCopy::CommandLine::ParseError("Dimensions can be provided at most once");
                        }
                        arguments.dimensions = parseDimensions(value);
                    }
                    else
                    {
                        keywords.insert(std::move(value));
                    }
                    value.clear();
                    if (ch == ':')
                    {
                        state = SUFFIX;
                    }
                }
                else
                {
                    value += ch;
                }
            } break;
        case SUFFIX:
            {
                if (end)
                {
                    arguments.remaining = std::move(value);
                }
                else
                {
                    value += ch;
                }
            } break;
        }
    }

    arguments.framepacking = HEIF::FramePackingProperty::MONOSCOPIC;
    for (auto kwd: keywords)
    {
        if (kwd == "mono")
        {
            arguments.framepacking = HEIF::FramePackingProperty::MONOSCOPIC;
        }
        else if (kwd == "lr" || kwd == "left-right" || kwd == "sbs")
        {
            arguments.framepacking = HEIF::FramePackingProperty::SIDE_BY_SIDE_PACKING;
        }
        else if (kwd == "tb" || kwd == "top-bottom")
        {
            arguments.framepacking = HEIF::FramePackingProperty::TOP_BOTTOM_PACKING;
        }
        else
        {
            throw LocalCopy::CommandLine::ParseError("Unknown keyword " + kwd);
        }
    }

    return arguments;
}


class SourceBase {
public:
    SourceBase() = default;
    SourceBase(const SourceBase&) = delete;
    void operator=(const SourceBase&) = delete;
    virtual ~SourceBase() = default;

    virtual HEIFPP::ImageItem* process(HEIFPP::Heif& aHeif) = 0;
};

using SourceList = std::list<std::shared_ptr<SourceBase>>;

class SingleSource: public SourceBase {
public:
    struct Config {
        HEIF::FramePackingProperty framepacking;
        std::string filename;

        Config(std::string aStr);
    };

    SingleSource(Config aConfig);

    HEIFPP::ImageItem* process(HEIFPP::Heif& aHeif) override;

private:
    const Config mConfig;
};

SingleSource::Config::Config(std::string aStr)
{
    auto args = parseSourceArguments(aStr);

    framepacking = args.framepacking;
    filename = args.remaining;

    if (args.dimensions)
    {
        throw LocalCopy::CommandLine::ParseError("Dimensions cannot be provided for single images");
    }
}

SingleSource::SingleSource(SingleSource::Config aConfig)
    : mConfig(aConfig)
{
    // nothing
}

HEIFPP::ImageItem* SingleSource::process(HEIFPP::Heif& aHeif)
{
    // racy, but the API doesn't do errors for this case
    if (auto dimension = readHevcDimensions(mConfig.filename))
    {
        std::cout << "Adding single image from " << mConfig.filename << std::endl;
        return write360Heif(&aHeif, mConfig.filename, mConfig.framepacking);
    }
    else
    {
        std::cerr << "Failed to open or parse input file " << mConfig.filename << std::endl;
        return nullptr;
    }
}

class TiledSource: public SourceBase {
public:
    struct Config
    {
        Config(std::string aConfig);

        std::string prefix;
        Dimensions resultDims;
        HEIF::FramePackingProperty framepacking;
    };

    TiledSource(Config aConfig);

    HEIFPP::ImageItem* process(HEIFPP::Heif& aHeif) override;

private:
    const Config mConfig;
};

TiledSource::Config::Config(std::string aConfig)
{
    auto args = parseSourceArguments(aConfig);

    if (!args.dimensions)
    {
        throw LocalCopy::CommandLine::ParseError("Dimensions not provided");
    }
    resultDims = *args.dimensions;

    framepacking = args.framepacking;

    prefix = args.remaining;
}

TiledSource::TiledSource(TiledSource::Config aConfig)
    : mConfig(aConfig)
{
    // nothing
}

template <typename T>
T ceilDiv(T a, T b)
{
    return (a + b - 1) / b;
}

HEIFPP::ImageItem* TiledSource::process(HEIFPP::Heif& aHeif)
{
    auto tileFileName = [this](uint32_t i)
        {
            return mConfig.prefix + std::to_string(i) + ".hevc";
        };

    auto tileDimensions = readHevcDimensions(tileFileName(0u));

    if (!tileDimensions)
    {
        std::cerr << "Failed to read image dimensions from " << tileFileName(0) << std::endl;
        return nullptr;
    }
    else
    {
        const uint32_t numHorTiles = ceilDiv(mConfig.resultDims.width, tileDimensions->width);
        const uint32_t numVerTiles = ceilDiv(mConfig.resultDims.height, tileDimensions->height);
        const uint32_t numTiles = numHorTiles * numVerTiles;

        std::cout << "Tile size " << tileDimensions->width << "x" << tileDimensions->height
                  << " results in " << numTiles << " tiles (" << numHorTiles << "x" << numVerTiles << ")"
                  << std::endl;

        std::vector<std::string> tiles;
        bool ok = true;

        for (uint32_t i = 0; ok && i < numTiles; ++i)
        {
            std::string filename = tileFileName(i);
            auto curTileDimensions = readHevcDimensions(filename);
            if (curTileDimensions)
            {
                using namespace std::rel_ops;
                if (*curTileDimensions != *tileDimensions)
                {
                    std::cerr << "Tile " << filename << " is not the same size as tile " << tileFileName(0u) << std::endl;;
                    ok = false;
                }
                else
                {
                    std::cout << "Adding tile from file " << filename << std::endl;
                    tiles.push_back(filename);
                }
            }
            else
            {
                std::cerr << "Cannot find or parse file " << filename << std::endl;
                ok = false;
            }
        }

        if (std::ifstream(tileFileName(numTiles), std::ios::binary))
        {
            std::cerr << "Warning! Found an unused tile " << tileFileName(numTiles) << std::endl;
        }

        if (ok)
        {
            return write360GridHeif(&aHeif, tiles,
                                    mConfig.resultDims.width, mConfig.resultDims.height,
                                    tileDimensions->width, tileDimensions->height,
                                    mConfig.framepacking);
        }
        else
        {
            return nullptr;
        }
    }
}

int process(SourceList aSources,
            std::string aOutputFilename)
{
    bool ok = true;
    HEIFPP::Heif heif;
    HEIFPP::ImageItem* primaryItem = nullptr;
    for (auto source: aSources)
    {
        HEIFPP::ImageItem* currentItem = nullptr;
        currentItem = source->process(heif);
        if (currentItem)
        {
            primaryItem = primaryItem ? primaryItem : currentItem;
        }
        else
        {
            ok = false;
            break;
        }
    }

    if (ok)
    {
        heif.setMajorBrand("mif1");
        heif.addCompatibleBrand("mif1");
        heif.addCompatibleBrand("heic");
        heif.addCompatibleBrand("hevc");

        heif.setPrimaryItem(primaryItem);

        auto res = heif.save(aOutputFilename.c_str());

        if (res != HEIFPP::Result::OK)
        {
            std::cerr << "Could not save OMAF HEIF res: " << (uint32_t) res << std::endl;
            return 2;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        std::cerr << "HEIF not saved" << std::endl;
        return 3;
    }
}


int main(int ac, char** av)
{
    std::cout << "omafimage version " << BuildInfo::Version << " built at " << BuildInfo::Time << std::endl
        << "Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved. " << std::endl << std::endl;

    using namespace LocalCopy::CommandLine;
    using namespace VDD::Utils;
    LocalCopy::CommandLine::Parser parser("omafimage");

    {
        bool help {};

        LocalCopy::Optional<SourceList> sources;
        std::string outputFilename;

        parser
            .add(Arg({ 'h' }, { "help" },
                     new Flag(help),
                     "Provide usage information"))
            .add(Arg({ 's' }, { "single" },
                     new Accumulate<String, SourceList>("{(mono|lr|tb):}full_image.hevc", sources,
                                                        [](std::string aFilename)
                                                        {
                                                            return std::shared_ptr<SourceBase>(new SingleSource(aFilename));
                                                        }),
                     "Add a single full image to the OMAF HEIF file."))
            .add(Arg({ 't' }, { "tiled" },
                     new Accumulate<String, SourceList>("2048x1024{,(mono|lr|tb)}):tileprefix", sources,
                                                        [](std::string aArgs)
                                                        {
                                                            return std::shared_ptr<SourceBase>(new TiledSource(aArgs));
                                                        }),
                     "Add a tiled image. File names of form tileprefix0.hevc,  tileprefix1.hevc etc. are used."))
            .add(Arg({ 'o' }, { "output" },
                     new String("output_file.heic", outputFilename),
                     "Set the output file name"))
            ;

        const auto result = parser.parse({ av + 1, av + ac });

        if (help)
        {
            parser.usage(std::cout);
            return 0;
        }
        else if (!result.isOK())
        {
            parser.usage(std::cout);
            std::cerr << "Error while parsing arguments: " << result.getError() << std::endl;
            return 1;
        }
        else if (!sources)
        {
            std::cerr << "No input provided" << std::endl;
            return 1;
        }
        else
        {
            return process(*sources, outputFilename);
        }
    }
}
