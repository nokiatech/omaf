
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
#include "sequencesource.h"

namespace VDD
{
    SequenceSource::SequenceSource(const Config& aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    SequenceSource::~SequenceSource() = default;

    std::vector<Streams> SequenceSource::produce()
    {
        std::vector<Streams> streams;
        if (mIndex >= mConfig.samples.size())
        {
            streams.push_back({Data(EndOfStream())});
        }
        else
        {
            streams.push_back({mConfig.samples[mIndex]});
            ++mIndex;
        }
        return streams;
    }
}  // namespace VDD
