
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

#include <memory>

#include "async/asyncnode.h"
#include "log/log.h"

namespace VDD
{
    struct AuxConfig;

    class ControllerOps
    {
    public:
        ControllerOps(GraphBase& aGraph, std::shared_ptr<Log> aLog);
        ~ControllerOps() = default;

        /** @brief Given a ProcessorNodeBase, instantiate one and wrap it with a AsyncNode wrapper
         * class.
         */
        template <typename SyncClass, typename Wrapper = typename AsyncNodeWrapperTraits<SyncClass>::Wrapper, typename Config>
        Wrapper* makeForGraph(const std::string& aName, const Config& aConfig);

        /** @brief Same as makeForGraph, but for pre-constructed nodes */
        template <typename SyncClass, typename Wrapper = typename AsyncNodeWrapperTraits<SyncClass>::Wrapper>
        Wrapper* wrapForGraph(const std::string& aName, std::unique_ptr<SyncClass>&& aSyncNode);

        /** @brief If neither of the previous are suitable, at least use this to ensure the common
         * configuration (setting logger) is performed.. */
        template <typename Async>
        Async* configureForGraph(std::unique_ptr<Async>&& async);

        /** @brief Perform logging */
        LogStream& log(LogLevel);

        /** @brief Access graph for some low-level ops */
        GraphBase& getGraph();

    private:
        GraphBase& mGraph;
        std::shared_ptr<Log> mLog;
    };
}

#include "controllerops.icpp"

