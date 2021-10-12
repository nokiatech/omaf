
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
#include <math.h>
#include <sstream>
#include "./tileproducer.h"
#include "processor/data.h"
#include "log/logstream.h"
#include "medialoader/h265memoryinputstream.h"

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

    TileProducer::TileProducer(Config& config)
        : mTileFilter(config.quality, config.projection, config.videoMode, config.resetExtractorLevelIDCTo51)
        , mTileConfig(config.tileConfig)
        , mExtractorMode(config.extractorMode)
        , mAUIndex(0)
        , mTileCount(0)
    {
        mTileCount = config.tileCount;
    }

    std::vector<Streams> TileProducer::process(const Streams& aStreams)
    {
        std::vector<Streams> streams;
        if (aStreams.isEndOfStream())
        {
            for (size_t i = 0; i < mTileConfig.size(); i++)
            {
                Meta meta = defaultMeta;
                meta.attachTag<TrackIdTag>(mTileConfig.at(i).trackId);
                streams.push_back({Data(EndOfStream(), mTileConfig.at(i).streamId, meta)});
            }
        }
        else
        {
            CodedFrameMeta inputMeta = aStreams.front().getCodedFrameMeta();
            if (!inputMeta.decoderConfig.empty())
            {
                const auto& cfg = inputMeta.decoderConfig;
                for (ConfigType configType : std::list<ConfigType>{ ConfigType::VPS, ConfigType::SPS, ConfigType::PPS })
                {
                    if (cfg.count(configType))
                    {
                        auto copy = cfg.at(configType);
                        H265MemoryInputStream input(&copy[0], copy.size());

                        if (!mTileFilter.parseParamSet(input, mTileConfig, inputMeta))
                        {
                            std::string type = (configType == ConfigType::VPS ? "VPS" : (configType == ConfigType::SPS ? "SPS" : "PPS"));
                            throw UnsupportedVideoInput("Problem parsing parameter set: " + type);
                        }
                    }
                }
            }
            H265MemoryInputStream input(reinterpret_cast<const uint8_t*>(aStreams.front().getCPUDataReference().address[0]), aStreams.front().getCPUDataReference().size[0]);

            if (mTileFilter.parseAU(input, mTileCount))
            {
                std::vector<Data> subPictures;
                for (size_t i = 0; i < mTileConfig.size(); i++)
                {
                    mTileFilter.convertToSubpicture(
                        i, mTileConfig.at(i), mAUIndex, subPictures, inputMeta.inCodingOrder,
                        inputMeta.codingTime, inputMeta.presTime, inputMeta.codingIndex,
                        inputMeta.presIndex, inputMeta.duration, mExtractorMode);
                }
                streams.push_back(Streams{subPictures.begin(), subPictures.end()});

                mAUIndex++;
            }
            else
            {
                throw UnsupportedVideoInput("The input mp4 file does not seem to contain expected type of H.265 video.");
            }
        }
        return streams;
    }

}  // namespace VDD
