
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
#include "controllerconfigure.h"

#include "dash.h"
#include "controllerbase.h"
#include "async/throttlenode.h"

namespace VDD
{
    ControllerConfigure::ControllerConfigure(ControllerBase& aController)
        : mController(aController)
    {
    }

    FrameDuration ControllerConfigure::getFrameDuration() const
    {
        return mController.mVideoFrameDuration;
    }

    void ControllerConfigure::setInput(AsyncNode* aInputLeft, ViewMask aInputLeftMask,
                                       AsyncNode* aInputRight, ViewMask aInputRightMask,
                                       VideoInputMode aInputVideoMode,
                                       FrameDuration aFrameDuration,
                                       FrameDuration aTimeScale,
                                       VideoGOP aGopInfo,
                                       bool aIsStereo)
    {
        mController.mInputLeft = aInputLeft;
        mController.mInputLeftMask = aInputLeftMask;
        mController.mInputRight = aInputRight;
        mController.mInputRightMask = aInputRightMask;
        mController.mInputVideoMode = aInputVideoMode;
        mController.mIsStereo = aIsStereo;
        mController.mVideoFrameDuration = aFrameDuration;
        mController.mVideoTimeScale = aTimeScale;
        mController.mGopInfo = aGopInfo;
    }

    bool ControllerConfigure::isMpdOnly() const
    {
        return false;   // this is a legacy functionality, only related to DASH and a minor external use case of it, not relevant for the mp4controller, should we get rid of it?
    }

    void ControllerConfigure::makeAudioDashPipeline(const ConfigValue& aDashConfig,
                                                    std::string aAudioName,
                                                    AsyncNode* aAacInput,
                                                    FrameDuration aTimeScale)
    {
        mController.makeAudioDashPipeline(aDashConfig,
                                          aAudioName,
                                          aAacInput,
                                          aTimeScale);
    }

    AsyncNode* ControllerConfigure::buildPipeline(std::string aName,
                                                  Optional<DashSegmenterConfig> aDashOutput,
                                                  PipelineOutput aPipelineOutput,
                                                  PipelineOutputNodeMap aSources,
                                                  AsyncProcessor* aMP4VRSink)
    {
        return mController.buildPipeline(aName,
                                         aDashOutput,
                                         aPipelineOutput,
                                         aSources,
                                         aMP4VRSink);
    }
}
