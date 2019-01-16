
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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
    class Source : public ProcessorNodeBase
    {
    public:
        Source();
        virtual ~Source();

        /** @brief Provides data; may return one or multiple results. In
            particular an encoder might first return 0 results, and after
            a while (or after stream end) multiple results.

            Each element is for one frame and each element in the frame is
            for different streams (ie. a tiler might produce one frame with
            multiple streams).

            @throws MismatchingStorageTypeException if the data isn't in
            suitable storage for this processor

            @returns a list of vectors of Data. When the stream ends, for
            each output stream an empty Data() (default-constructed Data)
            is returned.
        */
        virtual std::vector<Views> produce() = 0;

        /** @brief Abort the Source. Next call to produce returns the
         * remaining frames and finally an EndOfStream.
         */
        virtual void abort();

        /** @brief Is this Source aborted? */
        bool isAborted() const;

    private:
        bool mAborted = false;
    };
}
