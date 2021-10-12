
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
#include "staticmetadatagenerator.h"

namespace VDD
{
    StaticMetadataGenerator::StaticMetadataGenerator(const Config& aConfig)
        : mConfig(aConfig)
        , mIndex(0)
    {
        for (auto& sample : mConfig.metadataSamples)
        {
            std::vector<std::vector<uint8_t>> dataVec{ sample };
            CPUDataVector dataRef(std::move(dataVec));
            mData.push_back(Data(std::move(dataRef), CodedFrameMeta()));
        }
    }

    StaticMetadataGenerator::~StaticMetadataGenerator() = default;

    std::vector<Streams> StaticMetadataGenerator::process(const Streams& aData)
    {
        std::vector<Streams> streams;
        if (aData.isEndOfStream())
        {
            streams = { aData };
        }
        else
        {
            if (mTimeTillNextSample <= FrameTime(0, 1))
            {
                mTimeTillNextSample += mConfig.sampleDuration.cast<FrameTime>();
                CodedFrameMeta codedMeta = aData.front().getCodedFrameMeta();
                codedMeta.format = mConfig.metadataType;
                codedMeta.duration = mConfig.sampleDuration;

                Meta meta(codedMeta);
                mIndex %= mData.size();
                streams = {{Data(mData[mIndex++], meta)}};
            }
            else
            {
                // fine, no streams to return
            }
            mTimeTillNextSample -= aData.front().getCodedFrameMeta().duration.cast<FrameTime>();
        }
        return streams;
    }
}  // namespace VDD
