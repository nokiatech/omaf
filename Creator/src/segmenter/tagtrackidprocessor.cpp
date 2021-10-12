
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
#include "tagtrackidprocessor.h"

namespace VDD
{
    TagTrackIdProcessor::TagTrackIdProcessor(const Config aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    std::vector<Streams> TagTrackIdProcessor::process(const Streams& aStreams)
    {
        Streams streams;
        for (auto data : aStreams)
        {
            streams.add(data.withTag(TrackIdTag(mConfig.trackId)));
        }
        return { streams };
    }
}
