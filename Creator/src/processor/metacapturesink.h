
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

#include <functional>
#include "sink.h"
#include "async/future.h"

namespace VDD
{
    // MetaCaptureSink sinks the first frame and returns its Meta to the provided callback
    // function.
    class MetaCaptureSink : public Sink
    {
    public:
        struct Config
        {
            // no configuration
        };

        MetaCaptureSink(const Config& aConfig);

        ~MetaCaptureSink();

        Future<std::vector<Meta>> getMeta() const;

        StorageType getPreferredStorageType() const override;

        void consume(const Streams&) override;

    private:
        bool mFirst = true;
        Promise<std::vector<Meta>> mMeta;
    };
}
