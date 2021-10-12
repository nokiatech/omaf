
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
#include "throttlenode.h"

namespace VDD
{
    ThrottleNode::ThrottleNode(GraphBase& aGraphBase, std::string aName, Config aConfig)
        : AsyncProcessor(aGraphBase, aName)
        , mConfig(aConfig)
    {
        // nothing
    }

    void ThrottleNode::hasInput(const Streams& aStreams)
    {
        for (auto& view : aStreams)
        {
            view.setDataAllocations(mConfig.dataAllocations.get());
        }
        if (aStreams.isEndOfStream())
        {
            setInactive();
        }
        hasOutput(aStreams);
    }

    bool ThrottleNode::isBlocked() const
    {
        return mConfig.dataAllocations.get().size > static_cast<std::int64_t>(mConfig.sizeLimit);
    }
}
