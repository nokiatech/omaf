
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
#include "sequentialgraph.h"

namespace VDD
{
    SequentialGraph::SequentialGraph() = default;

    SequentialGraph::~SequentialGraph() = default;

    void SequentialGraph::nodeHasOutput(AsyncNode* aNode, const Streams& aStreams)
    {
        for (auto& callback: getNodeCallbacks(aNode))
        {
            if (callback.streamFilter.allStreams())
            {
                callback.processor->hasInput(aStreams);
            }
            else
            {
                Streams callbackViews;
                auto set = callback.streamFilter.asSet();
                for (const auto& view: aStreams)
                {
                    if (set.count(view.getStreamId())) {
                        callbackViews.add(view);
                    }
                }
                callback.processor->hasInput(callbackViews);
            }
        }
    }

    bool SequentialGraph::step(GraphErrors&)
    {
        if (!mStarted)
        {
            mStarted = true;
            for (AsyncSource* source: getSources())
            {
                source->graphStarted();
            }
        }

        bool anyActive = false;
        for (AsyncSource* source: getSources())
        {
            if (source->isActive())
            {
                anyActive = true;
                source->produce();
            }
        }
        return anyActive;
    }

    void SequentialGraph::abort()
    {
        for (AsyncSource* source: getSources())
        {
            if (source->isActive())
            {
                source->abort();
            }
        }
    }
}
