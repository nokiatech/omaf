
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

#include "processor/data.h"
#include "processor/processor.h"

namespace VDD
{
    // Works only for coded frames, as that's what we currently need
    class TagTrackIdProcessor : public Processor
    {
    public:
        struct Config
        {
            TrackId trackId;
        };

        TagTrackIdProcessor(Config aConfig);

        StorageType getPreferredStorageType() const override
        {
            return StorageType::CPU;
        }

        std::vector<Views> process(const Views& data) override;

    private:
        Config mConfig;
    };
}
