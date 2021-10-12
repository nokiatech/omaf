
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
#include "processor/processor.h"

namespace VDD
{
    /** @brief A Processor that simply copies its input to its output.

        Useful as an example or perhaps a place holder.
    */
    class NoOp : public Processor
    {
    public:
        struct Config
        {
            size_t* counter = nullptr;  // if not null, incremented on each call to process()
        };
        NoOp(Config aConfig);
        ~NoOp() override;

        StorageType getPreferredStorageType() const override
        {
            return StorageType::CPU;
        }

        std::vector<Streams> process(const Streams& data) override;

    private:
        const Config mConfig;
    };
}

