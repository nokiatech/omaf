
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

#include "processor/data.h"
#include "processor/processor.h"

namespace VDD
{
    // Works only for coded frames, as that's what we currently need
    class TagMultiTrackIdProcessor : public Processor
    {
    public:
        struct Config
        {
            std::map<StreamId, TrackId> mapping;
        };

        TagMultiTrackIdProcessor(Config aConfig);

        StorageType getPreferredStorageType() const override
        {
            return StorageType::CPU;
        }

        std::string getGraphVizDescription() override;

        std::vector<Streams> process(const Streams& data) override;

    private:
        Config mConfig;
    };
}
