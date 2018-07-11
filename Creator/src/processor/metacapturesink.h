
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
#pragma once

#include <functional>
#include "sink.h"
#include "async/future.h"

namespace VDD
{
    // MetaCaptureSink sinks the configured frame/data and returns its CodedFrameMeta to the provided callback
    // function.
    class MetaCaptureSink : public Sink
    {
    public:
        struct Config
        {
            // in OMAF take the specified one
            bool pickFirstOne;
            StreamId streamId;
        };

        MetaCaptureSink(const Config& aConfig);

        ~MetaCaptureSink();

        Future<std::vector<CodedFrameMeta>> getCodedFrameMeta() const;

        StorageType getPreferredStorageType() const override;

        void consume(const Views&) override;

    private:
        bool mPickFirstOne; //vs pick given stream id
        bool mDone;
        StreamId mStreamId;
        Promise<std::vector<CodedFrameMeta>> mCodedFrameMeta;
    };
}
