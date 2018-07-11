
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
#include "noop.h"
#include <fstream>

namespace VDD
{
    NoOp::NoOp(Config aConfig) : mConfig(aConfig)
    {
        // nothing
    }

    NoOp::~NoOp()
    {
        // nothing
    }

    std::vector<Views> NoOp::process(const Views& aStreams)
    {
        std::vector<Views> frames;

        if (mConfig.counter)
        {
            ++*mConfig.counter;
        }

        if (aStreams[0].isEndOfStream())
        {
            mEnd = true;
        }
        if (mEnd)
        {
            frames.push_back({Data(EndOfStream())});
        }
        else
        {
            frames.push_back({aStreams[0]});
        }

        return frames;
    }

}  // namespace VDD
