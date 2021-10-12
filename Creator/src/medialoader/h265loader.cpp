
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include <algorithm>
#include <iterator>
#include <cmath>

#include "h265loader.h"
#include "common/utils.h"
#include "omaf/parser/h265parser.hpp"
#include "h265segmentedinputstream.h"
#include "h265stdistream.h"

namespace VDD
{
    namespace
    {
        template <typename T>
        T defaultOrError(Optional<T> aValue, Optional<T> aFallback, std::function<void(void)> aThrow)
        {
            if (aValue)
            {
                return *aValue;
            }
            else if (aFallback)
            {
                return *aFallback;
            }
            else
            {
                aThrow();
                assert(false);
                return T();
            }
        }

        // Take NAL data and replace the 0*1-prefix with the content size
        std::vector<uint8_t> replaceNalPrefixWithSize(const std::vector<uint8_t>& aNal)
        {
            std::vector<uint8_t> withSize;

            size_t offset = 0;
            while (offset < aNal.size() && aNal[offset] != 1)
            {
                ++offset;
            }
            ++offset;

            assert(offset < aNal.size());
            uint64_t size = aNal.size() - offset;
            withSize.reserve(size + 4);

            for (size_t index = 0; index < 4; ++index)
            {
                auto bitOffset = 24 - index * 8;
                withSize.push_back(uint8_t((size >> bitOffset) & 0xffu));
            }

            std::copy(aNal.begin() + static_cast<int>(offset), aNal.end(), std::back_inserter(withSize));
            return withSize;
        }
    }

    H265LoaderError::H265LoaderError(std::string aMessage, Optional<std::string> aFilename)
        : MediaLoaderException("H265LoaderError")
        , mMessage(aMessage)
        , mFilename(aFilename)
    {
        // nothing
    }

    std::string H265LoaderError::message() const
    {
        auto message = "Error while processing H265: " + mMessage;
        if (mFilename)
        {
            message += " (" + *mFilename + ")";
        }
        return message;
    }

    H265LoaderError::~H265LoaderError() = default;

    bool getH265Tiles(const Data& aData, size_t& aTilesX, size_t& aTilesY)
    {
        H265::PictureParameterSet pps;
        std::vector<uint8_t> nalUnitRBSP;
        H265Parser::convertToRBSP(aData.getCodedFrameMeta().decoderConfig.at(ConfigType::PPS), nalUnitRBSP, true);
        Parser::BitStream bitstr(nalUnitRBSP);
        H265::NalUnitHeader naluHeader;
        H265Parser::parseNalUnitHeader(bitstr, naluHeader);

        H265Parser::parsePPS(bitstr, pps);
        if (pps.mTilesEnabledFlag)
        {
            aTilesX = static_cast<size_t>(pps.mNumTileColumnsMinus1) + 1;
            aTilesY = static_cast<size_t>(pps.mNumTileRowsMinus1) + 1;
            return true;
        }
        else
        {
            return false;
        }
    }

    void getH265CtuSize(const Data& aData, int& aCtuSize)
    {
        H265::SequenceParameterSet sps;
        std::vector<uint8_t> nalUnitRBSP;
        H265Parser::convertToRBSP(aData.getCodedFrameMeta().decoderConfig.at(ConfigType::SPS), nalUnitRBSP, true);
        Parser::BitStream bitstr(nalUnitRBSP);
        H265::NalUnitHeader naluHeader;
        H265Parser::parseNalUnitHeader(bitstr, naluHeader);

        H265Parser::parseSPS(bitstr, sps);

        aCtuSize = (int)pow(2, (sps.mLog2MinLumaCodingBlockSizeMinus3 + 3) + (sps.mLog2DiffMaxMinLumaCodingBlockSize));
    }

    H265Source::H265Source(H265Source::Config aConfig)
        : mConfig(aConfig)
    {
        if (aConfig.inputStream)
        {
            mH265Stream = aConfig.inputStream;
        }
        else
        {
            mStream.open(mConfig.filename.c_str(), std::ios::binary);
            mH265Stream = std::static_pointer_cast<H265InputStream>(
                std::make_shared<H265StdIStream>(mStream));
        }
    }

    H265Source::~H265Source()
    {
        // nothing
    }

    CodedFormat H265Source::getFormat() const
    {
        return CodedFormat::H265;
    }

    Optional<size_t> H265Source::getNumberOfFrames() const
    {
        return mConfig.frameLimit;
    }

    FrameDuration H265Source::getCurrentFrameDuration() const
    {
        acquireMeta();
        return defaultOrError(mFrameDuration, mConfig.frameDuration, [&]() {
                throw H265LoaderError("Frame rate was not detected, needs to be provided explicitly.",
                                      mConfig.filename);
            });
    }

    FrameRate H265Source::getFrameRate() const
    {
        return getCurrentFrameDuration().per1();
    }

    FrameDuration H265Source::getTimeScale() const
    {
        return getCurrentFrameDuration() * FrameDuration(1u, 256u);
    }

    Dimensions H265Source::getDimensions() const
    {
        acquireMeta();
        return {mMeta.width, mMeta.height};
    }

    int H265Source::getGOPLength(bool& aIsFixed) const
    {
        aIsFixed = bool(mConfig.gopLength);
        return mConfig.gopLength ? *mConfig.gopLength : 0;
    }

    bool H265Source::getTiles(size_t& aTilesX, size_t& aTilesY)
    {
        acquireMeta();
        if (mMeta.tiles)
        {
            aTilesX = mMeta.tilesX;
            aTilesY = mMeta.tilesY;
            return true;
        }
        else
        {
            return false;
        }
    }

    void H265Source::getCtuSize(int& aCtuSize)
    {
        acquireMeta();
        aCtuSize = mMeta.ctuSize;
    }

    void H265Source::State::reset()
    {
        data.clear();
        poc = {};
        hasRAP = false;
        // don't reset decoder config
    }

    Data H265Source::makeFrame()
    {
        // mCurrent.pocOffset gets updated before assigning, so let's keep the code in sequence but
        // store this value before messing up with it
        auto pocOffset = mState.pocOffset;

        CPUDataVector dataVector{{std::move(mState.data)}};
        mState.data.clear();
        if (mState.poc)
        {
            mCodedFrameMeta.presIndex = *mState.poc + pocOffset;
        }
        else
        {
            assert(false);
            mCodedFrameMeta.presIndex = 0;
        }

        mCodedFrameMeta.codingIndex = mCodingIndex;
        mCodedFrameMeta.codingTime = mCodingTime;
        ++mCodingIndex;
        mCodedFrameMeta.decoderConfig = mState.decoderConfig;
        mCodedFrameMeta.inCodingOrder = true;
        mCodedFrameMeta.format = CodedFormat::H265;
        // we'd rather not use acquireMeta form here nor a separate function for directly getting
        // frame duration, so the error message is duplicated.
        mCodedFrameMeta.duration = defaultOrError(mConfig.frameDuration, mFrameDuration, [&]() {
                throw H265LoaderError("Frame rate was not detected, needs to be provided explicitly.",
                                      mConfig.filename);
            });
        mCodedFrameMeta.presTime =
            mCodedFrameMeta.duration.cast<FrameTime>() * FrameTime(mCodedFrameMeta.presIndex, 1);
        mCodedFrameMeta.width = mMeta.width;
        mCodedFrameMeta.height = mMeta.height;
        mCodedFrameMeta.type = mState.hasRAP ? FrameType::IDR : FrameType::NONIDR;
        mCodingTime += mCodedFrameMeta.duration.cast<FrameTime>();

        if (mConfig.gopLength && *mConfig.gopLength > 0 &&
            (mCodedFrameMeta.codingIndex % *mConfig.gopLength == 0) &&
            (mCodedFrameMeta.type != FrameType::IDR))
        {
            throw H265LoaderError("GOP length check for " + std::to_string(*mConfig.gopLength) +
                                      " failed at frame " +
                                      std::to_string(mCodedFrameMeta.codingIndex),
                                  mConfig.filename);
        }

        Data dataObject(std::move(dataVector), mCodedFrameMeta);
        if (!mHasAcquiredMeta)
        {
            // We have this special support for first frame handling, because we
            // hope to be able to process H265 streams later on without iterating
            // through the whole stream
            mMeta.tiles = getH265Tiles(dataObject, mMeta.tilesX, mMeta.tilesY);
            getH265CtuSize(dataObject, mMeta.ctuSize);
            mHasAcquiredMeta = true;
        }

        mState.reset();

        return dataObject;
    }

    bool H265Source::frameLimitEncountered() const
    {
        return mConfig.frameLimit && mCodingIndex >= Index(*mConfig.frameLimit);
    }

    std::vector<Streams> H265Source::produceDirect()
    {
        std::vector<Streams> streams;
        if (!mFinished)
        {
            std::vector<uint8_t> currNalUnitData;
            std::vector<uint8_t> rbsp;

            while (!mH265Stream->eof() && streams.size() == 0 && !frameLimitEncountered())
            {
                currNalUnitData.clear();
                auto type = H265Parser::readNextNalUnit(*mH265Stream, currNalUnitData);
                if (currNalUnitData.size() == 0)
                {
                    // skip the nal header to scan for the next one
                    while (mH265Stream->getNextByte() == 0 && !mH265Stream->eof())
                    {
                        // skip
                    }
                }
                else
                {
                    auto copyData = [&]() {
                        if (mConfig.videoNalStartCodes)
                        {
                            std::copy(currNalUnitData.begin(), currNalUnitData.end(),
                                      std::back_inserter(mState.data));
                        }
                        else
                        {
                            std::vector<uint8_t> withSize =
                                replaceNalPrefixWithSize(currNalUnitData);
                            std::copy(withSize.begin(), withSize.end(),
                                      std::back_inserter(mState.data));
                        }
                    };

                    H265::NalUnitHeader naluHeader;
                    rbsp.clear();
                    H265Parser::convertToRBSP(currNalUnitData, rbsp);
                    Parser::BitStream bitstr(rbsp);
                    H265Parser::parseNalUnitHeader(bitstr, naluHeader);
                    if (type == H265::H265NalUnitType::SPS)
                    {
                        H265::SequenceParameterSet sps;
                        H265Parser::parseSPS(bitstr, sps);
                        mMeta.width = sps.mPicWidthInLumaSamples;
                        mMeta.height = sps.mPicHeightInLumaSamples;

                        mSpss.push_back(sps);
                        mSpssForH265Parser.push_back(&*mSpss.rbegin());

                        if (sps.mVuiParametersPresentFlag &&
                            sps.mVuiParameters.mVuiTimingInfoPresentFlag)
                        {
                            mFrameDuration = FrameDuration(sps.mVuiParameters.mVuiNumUnitsInTick,
                                                           sps.mVuiParameters.mVuiTimeScale);
                        }

                        mState.decoderConfig[ConfigType::SPS] = currNalUnitData;
                    }
                    else if (type == H265::H265NalUnitType::VPS)
                    {
                        mState.decoderConfig[ConfigType::VPS] = currNalUnitData;
                    }
                    else if (type == H265::H265NalUnitType::PPS)
                    {
                        H265::PictureParameterSet pps;
                        H265Parser::parsePPS(bitstr, pps);
                        mPpss.push_back(pps);
                        mPpssForH265Parser.push_back(&*mPpss.rbegin());

                        mState.decoderConfig[ConfigType::PPS] = currNalUnitData;
                    }
                    else if (type == H265::H265NalUnitType::SUFFIX_SEI)
                    {
                        copyData();
                    }
                    else if (H265Parser::isVclNaluType(type))
                    {
                        bool isFirstVclNalu = H265Parser::isFirstVclNaluInPic(currNalUnitData);
                        bool wasFirstFrame = false;

                        if (!mFirstFrameFound)
                        {
                            mFirstFrameFound = isFirstVclNalu;
                            wasFirstFrame = true;
                        }
                        else if (isFirstVclNalu)
                        {
                            // We now have the complete previous nal unit
                            streams.push_back(Streams({makeFrame()}));
                        }

                        // ITU-T H.265 v3 (04/2015) definition 3.62
                        bool isIdrPicture = type == H265::H265NalUnitType::CODED_SLICE_IDR_W_RADL ||
                                            type == H265::H265NalUnitType::CODED_SLICE_IDR_N_LP;

                        if (isFirstVclNalu && isIdrPicture)
                        {
                            mPrevPicOrderCntLsb = 0u;
                            mPrevPicOrderCntMsb = 0u;
                            mState.poc = 0;

                            // and adjust the poc offset for next frames
                            mState.pocOffset += mState.largestPoc + (wasFirstFrame ? 0 : 1);
                            mState.largestPoc = 0;
                        }

                        H265::SliceHeader sliceHeader;
                        H265Parser::parseSliceHeader(bitstr, sliceHeader, type, mSpssForH265Parser,
                                                     mPpssForH265Parser);

                        if (!isIdrPicture)
                        {
                            mState.poc = H265Parser::decodePoc(
                                sliceHeader, naluHeader, mPrevPicOrderCntLsb, mPrevPicOrderCntMsb);
                            mState.largestPoc = std::max(*mState.poc, mState.largestPoc);
                        }

                        copyData();

                        // ITU-T H.265 v3 (04/2015) definition 3.114
                        // bool isRAP = naluHeader.mNuhLayerId == 0 && isIdrPicture;
                        mState.hasRAP = mState.hasRAP || isIdrPicture;
                    }
                }
            }
            if (mH265Stream->eof() && mState.data.size() && !frameLimitEncountered())
            {
                streams.push_back(Streams({makeFrame()}));
            }

            if (mH265Stream->eof() || frameLimitEncountered())
            {
                mFinished = true;
            }
        }
        if (mFinished)
        {
            if (mConfig.frameLimit && mCodingIndex < Index(*mConfig.frameLimit))
            {
                throw H265LoaderError("Stream terminated prematurely at frame " +
                                          std::to_string(mCodingIndex) + " of " +
                                          std::to_string(*mConfig.frameLimit),
                                      mConfig.filename);
            }
            streams.push_back(Streams({Data(EndOfStream())}));
        }
        return streams;
    }

    void H265Source::acquireMeta() const
    {
        auto& mutable_this = const_cast<H265Source&>(*this);
        if (!mutable_this.mHasAcquiredMeta)
        {
            auto streams = mutable_this.produceDirect();
            std::copy(std::make_move_iterator(streams.begin()), std::make_move_iterator(streams.end()),
                      std::back_inserter(mutable_this.mFirstFrameQueue));
            if (!mutable_this.mHasAcquiredMeta)
            {
                throw H265LoaderError("Failed to read meta data from H265 file", mConfig.filename);
            }
        }
    }

    std::vector<Streams> H265Source::produce()
    {
        std::vector<Streams> streams;
        if (mFirstFrameQueue.size())
        {
            streams = std::move(mFirstFrameQueue);
            mFirstFrameQueue.clear();
        }
        else
        {
            streams = produceDirect();
        }
        return streams;
    }

    void H265Source::abort()
    {
        mFinished = true;
    }

    H265Loader::H265Loader(Config aConfig, bool aVideoNalStartCodes)
        : mConfig(aConfig)
        , mVideoNalStartCodes(aVideoNalStartCodes)
    {
        // nothing
    }

    H265Loader::~H265Loader() = default;

    namespace
    {
        class SegmentedFilesOpener;

        class SegmentedFilesStream : public H265SegmentedInputStream
        {
        public:
            SegmentedFilesStream(OpenStream aOpenStream, std::shared_ptr<SegmentedFilesOpener> aOpener)
                : H265SegmentedInputStream(aOpenStream)
                , mOpener(aOpener)
            {
                // nothing
            }

        private:
            std::shared_ptr<SegmentedFilesOpener> mOpener;
        };

        class FileStream : public H265InputStream
        {
        public:
            FileStream(std::string aFilename)
                : mStdStream(aFilename.c_str(), std::ios::binary)
                , mH265Stream(mStdStream)
            {
                // nothing
            }

            bool ok() const
            {
                return !!mStdStream;
            }

            uint8_t getNextByte() override
            {
                return mH265Stream.getNextByte();
            }

            bool eof() override
            {
                return mH265Stream.eof();
            }

            void rewind(size_t aCount) override
            {
                return mH265Stream.rewind(aCount);
            }

        private:
            std::ifstream mStdStream;
            H265StdIStream mH265Stream;
        };

        class SegmentedFilesOpener
        {
        public:
            SegmentedFilesOpener(std::string aTemplate, unsigned aStartNumber)
                : mTemplate(aTemplate)
                , mStartNumber(aStartNumber)
            {
                // nothing
            }

            std::unique_ptr<H265InputStream> operator()(size_t aChunk)
            {
                auto fileStream = Utils::make_unique<FileStream>(
                    Utils::replace(mTemplate, "$Number$", std::to_string(aChunk + mStartNumber)));

                if (fileStream->ok())
                {
                    return fileStream;
                }
                else
                {
                    return {};
                }
            }

            std::shared_ptr<H265InputStream> getInputStream(std::shared_ptr<SegmentedFilesOpener> aSelf)
            {
                // this is required so we get to hook the object itself as auxuliary data to be kept around but the object we return
                assert(aSelf.get() == this);

                return std::static_pointer_cast<H265InputStream>(std::make_shared<SegmentedFilesStream>(*this, aSelf));
            }

        private:
            std::string mTemplate;
            unsigned mStartNumber;
        };
    }  // namespace

    std::unique_ptr<MediaSource> H265Loader::sourceForTrack(TrackId aTrackId,
                                                            const SourceConfig& aSourceConfig)
    {
        if (aTrackId.get() == 1)
        {
            H265Source::Config config {};
            config.filename = mConfig.filename;
            config.frameDuration = aSourceConfig.overrideFrameDuration;
            config.frameLimit = aSourceConfig.frameLimit;
            config.videoNalStartCodes = mVideoNalStartCodes;
            config.gopLength = mConfig.gopLength;
            if (mConfig.filename.find("$Number$") != std::string::npos)
            {
                auto opener = std::make_shared<SegmentedFilesOpener>(
                    mConfig.filename, mConfig.startNumber ? *mConfig.startNumber : 1u);
                config.inputStream = opener->getInputStream(opener);
            }
            return Utils::make_unique<H265Source>(config);
        }
        else
        {
            return {};
        }
    }

    std::set<TrackId> H265Loader::getTracksByFormat(std::set<CodedFormat> aFormats)
    {
        if (aFormats.count(CodedFormat::H265))
        {
            return {1};
        }
        else
        {
            return {};
        }
    }

    std::set<TrackId> H265Loader::getTracksBy(std::function<bool(MediaSource& aSource)> aPredicate)
    {
        H265Source::Config config {};
        config.filename = mConfig.filename;
        config.frameDuration = FrameDuration(1, 1);
        config.videoNalStartCodes = false;
        H265Source source(config);
        if (aPredicate(source))
        {
            return {1};
        }
        else
        {
            return {};
        }
    }

}  // namespace VDD
