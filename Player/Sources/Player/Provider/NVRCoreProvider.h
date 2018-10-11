
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

#include "NVRNamespace.h"
#include "NVRErrorCodes.h"
#include "Provider/NVRCoreProviderSources.h"
#include "OMAFPlayerDataTypes.h"

#include "Media/NVRMediaInformation.h"
#include "Core/Foundation/NVRSemaphore.h"

OMAF_NS_BEGIN
    class AudioInputBuffer;

    namespace PlaybackState
    {
        enum Enum
        {
            INVALID = -1,

            UNKNOWN,
            PLAY,
            PAUSE,
            STOP,

            COUNT
        };
    }

    class CoreProviderObserver
    {
    public:
        
        CoreProviderObserver() {}
        virtual ~CoreProviderObserver() {}
        
        // Emitted when the provider has new content ready to be prepared as textures and rendered.
        // It can be emitted at any point, any number of times.
        // Once it has been emitted (one or more times), one render round will follow.
        // Emitting it during a render inside prepareTextures() will request another render round to be done.
        virtual void_t onRequestUpdate() = 0;
        virtual void_t onErrorOccurred(Error::Enum error) = 0;
    };

    class CoreProvider
    {
    public:
        
        virtual ~CoreProvider() {};

        virtual const CoreProviderSourceTypes& getSourceTypes() = 0;
        virtual Error::Enum prepareSources(HeadTransform currentHeadtransform, float32_t fovHorizontal, float32_t fovVertical) = 0;
        virtual const CoreProviderSources& getSources() = 0;

        virtual Error::Enum setAudioInputBuffer(AudioInputBuffer *inputBuffer) = 0;

        /**
         * Signal to provider that it is used by someone, so that internal resources should not be changed until the usage has stopped.
         *
         * NOTE: enter() will block until the last user has left the area. 
         *
         * Currently only one thread can enter the area at the same time, because there is no use-case where that would be needed. 
         * If more fine grained locking is necessary, read and write locks should be separated and more sophisticated system should
         * be added where getting write-lock would have higher priority in waiting queue (to prevent write lock from starving) 
         * than read-locks and allowing to get multiple read-locks at the same time.
         */
        virtual void enter() = 0;
        virtual void leave() = 0;
    };
OMAF_NS_END


