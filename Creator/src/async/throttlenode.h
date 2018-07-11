
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
#include "asyncnode.h"

namespace VDD
{
    class GraphBase;

    class ThrottleNode : public AsyncProcessor
    {
    public:
        struct Config
        {
            // track this resource
            std::reference_wrapper<DataAllocations> dataAllocations;

            // when dataAllocations->size exceeds this, stop accepting input
            size_t sizeLimit;
        };

        ThrottleNode(GraphBase& aGraphBase, std::string aName, Config aConfig);

        bool isBlocked() const override;

        void hasInput(const Views& aViews) override;

    private:
        Config mConfig;
    };
}
