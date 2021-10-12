
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

#include "asyncnode.h"

namespace VDD
{
    // A mechanism to synchronize multiple frame sources (ie. different MP4 loaders) so that none of
    // the sources may produce more frames than another. Once one source produces EndOfStream, all other sources
    // also produce EndOfStream and the rest of incoming frames are ignored.

    class MediaStepLockState;

    class MediaStepLockProcessor : public AsyncProcessor
    {
    public:
        MediaStepLockProcessor(GraphBase& aGraphBase, int aId,
                               std::shared_ptr<MediaStepLockState> aState);
        ~MediaStepLockProcessor() override = default;

        void hasInput(const Streams& streams) override;
        bool isBlocked() const override;

    private:
        friend class MediaStepLockState;

        int mId;
        bool mPaused = false;
        std::shared_ptr<MediaStepLockState> mState;

        // These aren't actually required functionality-wise, but used for ensuring the
        // functionality isn't misused too badly :).
        std::set<StreamId> mKnownStreams;
        std::set<StreamId> mActiveStreams;
        bool mEncounteredEndOfStream = false;

        void setPaused(bool aPaused);
        void produceEOS();
    };

    class MediaStepLock
    {
    public:
        struct Config
        {
            int throttleLimit; // a node is permitted to be this many frames ahead of others
        };

        static const Config defaultConfig;

        MediaStepLock(GraphBase&, const Config& aConfig = defaultConfig);

        virtual ~MediaStepLock() = default;

        // The node is stored in the graph
        std::unique_ptr<MediaStepLockProcessor> get();

        void operator=(const MediaStepLock&) = delete;
        MediaStepLock(const MediaStepLock&) = delete;
        MediaStepLock(MediaStepLock&&) = delete;

    private:
        GraphBase& mGraph;
        std::shared_ptr<MediaStepLockState> mState;
    };

}