
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
#include "sequentialgraph.h"

namespace VDD
{
    SequentialGraph::SequentialGraph() = default;

    SequentialGraph::~SequentialGraph() = default;

    void SequentialGraph::nodeHasOutput(AsyncNode* aNode, const Views& aViews)
    {
        for (auto& callback: getNodeCallbacks(aNode))
        {
            if (callback.viewMask == allViews)
            {
                callback.processor->hasInput(aViews);
            }
            else
            {
                Views callbackViews;
                size_t viewIndex = 0;
                size_t mask = callback.viewMask;
                while (mask)
                {
                    if (mask & 1)
                    {
                        callbackViews.push_back(aViews[viewIndex]);
                    }
                    mask >>= 1;
                    ++viewIndex;
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
