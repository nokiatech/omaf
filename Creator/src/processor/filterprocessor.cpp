
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
#include "filterprocessor.h"

namespace VDD
{
    FilterProcessor::FilterProcessor(const Config& aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    FilterProcessor::~FilterProcessor() = default;

    StorageType FilterProcessor::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    std::vector<Streams> FilterProcessor::process(const Streams& aStreams)
    {
        if (mConfig.filter(aStreams))
        {
            return { aStreams };
        }
        else
        {
            return { };
        }
    }
}
