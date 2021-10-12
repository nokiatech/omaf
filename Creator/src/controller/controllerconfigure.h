
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

#include <streamsegmenter/segmenterapi.hpp>
#include <cassert>

#include "async/asyncnode.h"
#include "common.h"
#include "common/utils.h"
#include "dash.h"
#include "processor/data.h"
#include "videoinput.h"
#include "audio.h"


namespace VDD
{
    class ControllerBase;
    class AsyncNode;
    class ConfigValue;
    class View;

    struct PipelineInfo {
        AsyncNode* source;
        AsyncProcessor* segmentSink; // only applicable for Dash
    };

    enum class PostponeTo {
        Phase3                  /* after entity groups, before final dash setup */
    };

    /** @brief A class that exposes some configuration-related operations about the Controller. This
     * one has low-level access to the Controller object (compared to ControllerOps that works with
     * higher level access to the parameters of Controller) */
    class ControllerConfigure
    {
    public:
        ControllerConfigure() = default;
        virtual ~ControllerConfigure() = default;

        virtual bool isView() const { return false; }

        virtual StreamId newAdaptationSetId()
        {
            assert(false);
            return {};
        }

        virtual TrackId newTrackId() = 0;

        virtual View* getView() const
        {
            return nullptr;
        };

        virtual ViewId getViewId() const
        {
            assert(false);
            return {};
        }

        virtual Optional<ViewLabel> getViewLabel() const
        {
            return {};
        }

        /** @brief Set the input sources and streams */
        virtual void setInput(AsyncNode* /* aInputLeft */, AsyncNode* /* aInputRight */,
                              VideoInputMode /* aInputVideoMode */,
                              FrameDuration /* aFrameDuration */, FrameDuration /* aTimeScale */,
                              VideoGOP /* aGopInfo */)
        {
        }

        /** @brief Is the Controller in mpd-only-mode? */
        virtual bool isMpdOnly() const
        {
            return false;
        }

        /** @brief Retrieve frame duration */
        virtual FrameDuration getFrameDuration() const
        {
            return FrameDuration{};
        }

        /** @see ControllerBase::getDashDurations */
        virtual SegmentDurations getDashDurations() const
        {
            assert(false);
            return {};
        }

        /** @brief Builds the audio dash pipeline */
        virtual Optional<AudioDashInfo> makeAudioDashPipeline(const ConfigValue& /* aDashConfig */,
                                                              std::string /* aAudioName */,
                                                              AsyncNode* /* aAacInput */,
                                                              FrameDuration /* aTimeScale */,
                                                              Optional<PipelineBuildInfo> /* aPipelineBuildInfo */)
        {
            return {};
        }

        /** @see ControllerBase::buildPipeline */
        virtual PipelineInfo buildPipeline(Optional<DashSegmenterConfig> /* aDashOutput */,
                                           PipelineOutput /* aPipelineOutput */,
                                           PipelineOutputNodeMap /* aSources */,
                                           AsyncProcessor* /* aMP4VRSink */,
                                           Optional<PipelineBuildInfo> /* aPipelineBuildInfo */)
        {
            return PipelineInfo{};
        }

        /** Retrieve media placeholders */
        virtual std::list<AsyncProcessor*> getMediaPlaceholders()
        {
            return {};
        }

        virtual Optional<Future<Optional<StreamSegmenter::Segmenter::MetaSpec>>> getFileMeta() const
        {
            return {};
        }

        /** Allow postponing operations until a given time */
        virtual void postponeOperation(PostponeTo aToPhase, std::function<void(void)> aOperation)
        {
            (void) aToPhase;
            (void) aOperation;
            assert(false);
        }

        /** Resolve given RefIdLabels labels to EntityIdReferences */
        virtual std::list<EntityIdReference> resolveRefIdLabels(std::list<ParsedValue<RefIdLabel>> aRefIdLabels)
        {
            (void) aRefIdLabels;
            assert(false);
            return {};
        };

        virtual ControllerConfigure* clone() = 0;
    };

    //
    // Hack to collect input information of multiple input videos
    //
    class ControllerConfigureCapture : public ControllerConfigure
    {
    public:
        ControllerConfigure& mChain;

        ControllerConfigureCapture(ControllerConfigure& aChain) : mChain(aChain)
        {
        }

        std::list<StreamId> mAdaptationSetIds;

        StreamId newAdaptationSetId() override
        {
            auto x = mChain.newAdaptationSetId();
            mAdaptationSetIds.push_back(x);
            return x;
        }

        std::list<TrackId> mTrackIds;

        TrackId newTrackId() override
        {
            auto x = mChain.newTrackId();
            mTrackIds.push_back(x);
            return x;
        }

        void setInput(AsyncNode* aInputLeft, AsyncNode* aInputRight, VideoInputMode aInputVideoMode,
                      FrameDuration aFrameDuration, FrameDuration aTimeScale,
                      VideoGOP aGopInfo) override
        {
            mInputLeft = aInputLeft;
            mInputRight = aInputRight;
            mInputVideoMode = aInputVideoMode;
            mVideoFrameDuration = aFrameDuration;
            mVideoTimeScale = aTimeScale;
            mGopInfo = aGopInfo;
        }

        ControllerConfigureCapture(const ControllerConfigureCapture& aSelf)
            : mChain(aSelf.mChain)
            , mInputLeft(aSelf.mInputLeft)
            , mInputRight(aSelf.mInputRight)
            , mInputVideoMode(aSelf.mInputVideoMode)
            , mVideoFrameDuration(aSelf.mVideoFrameDuration)
            , mVideoTimeScale(aSelf.mVideoTimeScale)
            , mGopInfo(aSelf.mGopInfo)
        {
        }

        ControllerConfigureCapture* clone() override
        {
            return new ControllerConfigureCapture(*this);
        };

        AsyncNode* mInputLeft;
        AsyncNode* mInputRight;
        VideoInputMode mInputVideoMode;
        FrameDuration mVideoFrameDuration;
        FrameDuration mVideoTimeScale;
        VideoGOP mGopInfo;
    };
}  // namespace VDD
