
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
#pragma once

#include "processor/source.h"

namespace VDD
{
    class GeneratorSource : public Source
    {
    public:
        struct Config {
            size_t frameCount;  // how many frames to produce
            size_t chunkCount;  // how many of the frames to produce at a time
            size_t dataSize;    // how big is the data returned in bytes
            size_t viewCount;   // how many views have that amount of data
        };
        GeneratorSource(Config aConfig);
        ~GeneratorSource();

        std::vector<Views> produce() override;

    private:
        Config mConfig;
        Data mData;
        size_t mFrameCount = 0;
    };
}
