
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
#include <memory>

#include "processor/sink.h"

namespace VDD
{
    /** @brief A Processor that simply copies its input to its output.

        Useful as an example or perhaps a place holder.
    */
    class DebugSink : public Sink
    {
    public:
        struct Config
        {
            bool log = false;
        };
        DebugSink(Config aConfig);
        ~DebugSink() override;

        StorageType getPreferredStorageType() const override
        {
            return StorageType::CPU;
        }

        void consume(const Streams& data) override;

        std::string getGraphVizDescription() override;

    private:
        Config mConfig;
        struct Info;
        std::unique_ptr<Info> mInfo;
    };
}

