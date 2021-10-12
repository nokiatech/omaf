
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

#include <list>
#include <vector>

#include "processornodebase.h"

#include "processor/data.h"
#include "processor/processor.h"

namespace VDD
{
    class Sink : public ProcessorNodeBase
    {
    public:
        Sink();
        virtual ~Sink();

        /** @bried What is the preferred input storage format? Input not
            in this storage may be rejected by Processor
        */
        virtual StorageType getPreferredStorageType() const = 0;

        /** @brief Consumes data;

            Each element in the input data is one view. Typically there
            are one or two streams. (Monoscopic or stereoscopic.)
        */
        virtual void consume(const Streams& input) = 0;
    };
}
