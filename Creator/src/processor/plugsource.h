
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

#include "processor/source.h"

namespace VDD
{
    // A source that produces EndOfStream. Useful for plugging nodes you might someone create out of
    // convenience, but dynamically you know there's no use for them.
    class PlugSource : public Source
    {
    public:
        struct Config {
            StreamId streamId = StreamIdUninitialized;
        };
        PlugSource(Config aConfig);
        ~PlugSource();

        std::vector<Streams> produce() override;

    private:
        Config mConfig;
    };
}
