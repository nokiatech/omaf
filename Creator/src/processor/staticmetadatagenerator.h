
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
#pragma once

#include "processor/processor.h"

namespace VDD
{
    class StaticMetadataGenerator : public Processor
    {
    public:
        struct Config
        {
            CodedFormat metadataType;
            std::vector<std::vector<uint8_t>> metadataSamples;

            // one metadata sample of this length per this duration, can correspond e.g. to GOP
            // length
            FrameDuration sampleDuration;
        };

        StaticMetadataGenerator(const Config& aConfig);
        ~StaticMetadataGenerator();

        StorageType getPreferredStorageType() const override
        {
            return StorageType::CPU;
        }

        std::vector<Streams> process(const Streams& data) override;

    private:
        Config mConfig;
        std::vector<Data> mData;
        size_t mIndex;
        FrameTime mTimeTillNextSample;
    };
}  // namespace VDD
