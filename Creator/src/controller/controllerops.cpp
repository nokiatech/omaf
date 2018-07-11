
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
#include "controllerops.h"

#include "async/throttlenode.h"

namespace VDD
{
    ControllerOps::ControllerOps(GraphBase& aGraph, std::shared_ptr<Log> aLog)
        : mGraph(aGraph)
        , mLog(aLog)
    {
        // nothing
    }

    LogStream& ControllerOps::log(LogLevel aLogLevel)
    {
        return mLog->log(aLogLevel);
    }

    /** @brief Access graph for some low-level ops */
    GraphBase& ControllerOps::getGraph()
    {
        return mGraph;
    }
}
