
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

#include <streamsegmenter/segmenterapi.hpp>

#include "common.h"
#include "async/asyncnode.h"
#include "common/utils.h"
#include "processor/data.h"
#include "videoinput.h"


namespace VDD
{
    class ControllerBase;
    class AsyncNode;
    class ConfigValue;
    struct DashSegmenterConfig;

    /** @brief A class that exposes some configuration-related operations about the Controller. This
     * one has low-level access to the Controller object (compared to ControllerOps that works with
     * higher level access to the parameters of Controller) */
    class ControllerConfigure
    {
    public:
        ControllerConfigure(ControllerBase& aController);

        /** @brief Set the input sources and views */
        void setInput(AsyncNode* aInputLeft, ViewMask aInputLeftMask,
                      AsyncNode* aInputRight, ViewMask aInputRightMask,
                      VideoInputMode aInputVideoMode,
                      FrameDuration aFrameDuration,
                      FrameDuration aTimeScale,
                      VideoGOP aGopInfo,
                      bool aIsStereo);

        /** @brief Is the Controller in mpd-only-mode? */
        bool isMpdOnly() const;

        /** @brief Retrieve frame duration */
        FrameDuration getFrameDuration() const;

        /** @brief Builds the audio dash pipeline */
        void makeAudioDashPipeline(const ConfigValue& aDashConfig,
                                   std::string aAudioName,
                                   AsyncNode* aAacInput,
                                   FrameDuration aTimeScale);

        /** @see ControllerBase::buildPipeline */
        AsyncNode* buildPipeline(std::string aName,
                                 Optional<DashSegmenterConfig> aDashOutput,
                                 PipelineOutput aPipelineOutput,
                                 PipelineOutputNodeMap aSources,
                                 AsyncProcessor* aMP4VRSink);
    private:
        ControllerBase& mController;
    };
}
