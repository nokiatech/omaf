
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

#include <functional>
#include "processor.h"

namespace VDD
{
    // FilterProcessor gets the metadata from the first frame and passes it to the the first frame and returns its
    // CodedFrameMeta to the provided callback function.
    class FilterProcessor : public Processor
    {
    public:
        struct Config {
            std::function<bool(const Streams& meta)> filter;
        };

        FilterProcessor(const Config& aConfig);

        ~FilterProcessor();

        StorageType getPreferredStorageType() const override;

        std::vector<Streams> process(const Streams&) override;

    private:
        Config mConfig;
    };
}
