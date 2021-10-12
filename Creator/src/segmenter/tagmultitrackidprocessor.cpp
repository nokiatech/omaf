
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
#include <sstream>

#include "tagmultitrackidprocessor.h"

namespace VDD
{
    TagMultiTrackIdProcessor::TagMultiTrackIdProcessor(const Config aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    std::vector<Streams> TagMultiTrackIdProcessor::process(const Streams& aStreams)
    {
        Streams streams;
        for (auto data : aStreams)
        {
            streams.add(data.withTag(TrackIdTag(mConfig.mapping.at(data.getStreamId()))));
        }
        return { streams };
    }

    std::string TagMultiTrackIdProcessor::getGraphVizDescription()
    {
        std::ostringstream st;
        bool first = true;
        for (auto m: mConfig.mapping) {
            st << (first ? "" : "\n") << m.first.get() << "=>" << m.second.get();
            first = false;
        }
        return st.str();
    }
}
