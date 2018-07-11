
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

    std::vector<Views> TagTrackIdProcessor::process(const Views& aViews)
    {
        Views views;
        for (auto data : aViews)
        {
            views.push_back(data.withTag(TrackIdTag(mConfig.trackId)));
        }
        return { views };
    }
}
