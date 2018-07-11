
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

#include "graphbase.h"

namespace VDD {
    class SequentialGraph : public GraphBase
    {
    public:
        SequentialGraph();
        virtual ~SequentialGraph();

        /** call .produce() on all AsyncSources. Doesn't convert exceptions to errors, as debugging
         * can be easier that way. */
        bool step(GraphErrors& aErrors) override;

        void abort() override;

    protected:
        void nodeHasOutput(AsyncNode* aNode, const Views& aViews) override;

    private:
        bool mStarted = false;
    };
}
