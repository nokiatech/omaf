
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

#include "generatorsource.h"

namespace VDD
{
    GeneratorSource::GeneratorSource(Config aConfig)
        : mConfig(aConfig)
    {
        std::vector<std::vector<uint8_t>> vec(1);
        vec[0].resize((mConfig.dataSize));
        CPUDataVector datavec(std::move(vec));
        RawFrameMeta meta;
        mData = Data(std::move(datavec), meta);
    }

    GeneratorSource::~GeneratorSource() = default;

    std::vector<Streams> GeneratorSource::produce()
    {
        std::vector<Streams> frames;
        Streams streams;
        if (mFrameCount >= mConfig.frameCount || isAborted())
        {
            mData = Data(EndOfStream());
            for (size_t c = 0; c < mConfig.viewCount; ++c)
            {
                streams.add(mData);
            }
            frames.push_back(streams);
        }
        else
        {
            size_t doFrames = std::min(mConfig.chunkCount, mConfig.frameCount - mFrameCount);
            for (size_t c = 0; c < mConfig.viewCount; ++c)
            {
                streams.add(mData);
            }
            for (size_t c = 0; c < doFrames; ++c)
            {
                frames.push_back(streams);
            }
            mFrameCount += doFrames;
        }
        return frames;
    }
}
