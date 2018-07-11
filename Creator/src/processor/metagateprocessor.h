
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
#include "processor.h"

namespace VDD
{
    // MetaGateProcessor gets the metadata from the first frame and passes it to the the first frame and returns its
    // CodedFrameMeta to the provided callback function.
    class MetaGateProcessor : public Processor
    {
    public:
        struct Config {
            std::function<bool(std::vector<CommonFrameMeta> meta)> metaCallback;
        };

        MetaGateProcessor(const Config& aConfig);

        ~MetaGateProcessor();

        StorageType getPreferredStorageType() const override;

        std::vector<Views> process(const Views&) override;

    private:
        Config mConfig;
        bool mFirst = true;
        bool mAllowPass = false;
    };
}
