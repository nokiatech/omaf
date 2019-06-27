
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
#include <math.h>
#include <sstream>
#include "./tileproducer.h"
#include "processor/data.h"
#include "log/logstream.h"
#include "mp4loader/mp4loader.h"

namespace VDD {

    WrongTileFilterConfigurationException::WrongTileFilterConfigurationException(std::string aMessage)
        : Exception("WrongTileConfigurationException")
        , mMessage(aMessage)
    {
        // nothing
    }

    std::string WrongTileFilterConfigurationException::message() const
    {
        return mMessage;
    }

    /** 
     *  Read interface to the NAL data stream
     */
    class NalInputStream : public H265InputStream
    {
    public:
        NalInputStream(uint8_t* aData, size_t aDataSize);
        ~NalInputStream();

        uint8_t getNextByte();
        bool eof();
        void rewind(size_t aNrBytes);
    private:
        uint8_t* mDataPtr;
        size_t mIndex;
        size_t mSize;
    };

    NalInputStream::NalInputStream(uint8_t* aData, size_t aDataSize)
        : H265InputStream()
        , mDataPtr(aData)
        , mIndex(0)
        , mSize(aDataSize)
    {
    }
    NalInputStream::~NalInputStream()
    {
    }
    uint8_t NalInputStream::getNextByte()
    {
        return mDataPtr[mIndex++];
    }
    bool NalInputStream::eof()
    {
        return mIndex == mSize;
    }
    void NalInputStream::rewind(size_t aNrBytes)
    {
        assert(mIndex - aNrBytes >= 0);
        mIndex -= aNrBytes;
    }



    TileProducer::TileProducer(Config& config)
        : mTileFilter(config.quality, config.projection, config.resetExtractorLevelIDCTo51)
        , mTileConfig(config.tileConfig)
        , mExtractorMode(config.extractorMode)
        , mAUIndex(0)
        , mTileCount(0)
        , mFrameCountLimit(config.frameCount)
    {
        MP4Loader::Config mp4LoaderConfig = { config.inputFileName };
        MP4Loader mp4Loader(mp4LoaderConfig, true); // bytestream format, as h265parser expects it
        std::set<VDD::CodedFormat> setOfFormats;
        setOfFormats.insert(setOfFormats.end(), CodedFormat::H265);
        auto videoTracks = mp4Loader.getTracksByFormat(setOfFormats);
        if (!videoTracks.size())
        {
            throw UnsupportedVideoInput(config.inputFileName + " does not contain H.265 video.");
        }

        // as the mp4 is expected to be a plain mp4, it doesn't contain any info about e.g. framepacking. That should come via json, and we should be able to trust on it
        // currently only mono input (and output) is supported, framepacked is not supported

        // takes the first track only
        MP4Loader::SourceConfig sourceConfig{};
        mMp4Source = mp4Loader.sourceForTrack(*videoTracks.begin(), sourceConfig);
        mTileCount = config.tileCount;
    }

    std::vector<Views> TileProducer::produce()
    {
        std::vector<Views> views = mMp4Source->produce();
        Views& data = views[0];
        std::vector<Views> subPictures;
        if (data[0].isEndOfStream() || (mFrameCountLimit > 0 && (uint32_t)mAUIndex > mFrameCountLimit))
        {
            for (size_t i = 0; i < mTileConfig.size(); i++)
            {
                Meta meta = defaultMeta;
                meta.attachTag<TrackIdTag>(mTileConfig.at(i).trackId);
                subPictures.push_back({ Data(EndOfStream(), mTileConfig.at(i).streamId, meta) });
            }
        }
        else
        {
            CodedFrameMeta inputMeta = data[0].getCodedFrameMeta();
            if (!inputMeta.decoderConfig.empty())
            {
                const auto& cfg = inputMeta.decoderConfig;
                for (ConfigType configType : std::list<ConfigType>{ ConfigType::VPS, ConfigType::SPS, ConfigType::PPS })
                {
                    if (cfg.count(configType))
                    {
                        auto copy = cfg.at(configType);
                        NalInputStream input(&copy[0], copy.size());

                        if (!mTileFilter.parseParamSet(input, mTileConfig, inputMeta))
                        {
                            std::string type = (configType == ConfigType::VPS ? "VPS" : (configType == ConfigType::SPS ? "SPS" : "PPS"));
                            throw UnsupportedVideoInput("Problem parsing parameter set: " + type);
                        }
                    }
                }
            }
            NalInputStream input((uint8_t*)(data[0].getCPUDataReference().address[0]), data[0].getCPUDataReference().size[0]);

            if (mTileFilter.parseAU(input, mTileCount))
            {
                for (size_t i = 0; i < mTileConfig.size(); i++)
                {
                    mTileFilter.convertToSubpicture(i, mTileConfig.at(i), mAUIndex, subPictures, inputMeta.presTime, inputMeta.codingIndex, inputMeta.duration, mExtractorMode);
                }

                mAUIndex++;
            }
            else
            {
                throw UnsupportedVideoInput("The input mp4 file does not seem to contain expected type of H.265 video.");
            }
        }
        return subPictures;
    }

    void TileProducer::abort()
    {

    }

}  // namespace VDD
