
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
#include "Provider/NVRVideoProvider.h"

#include "Provider/NVRProviderBase.h"
#include "Provider/NVRMP4VRProvider.h"
#include "Foundation/NVRFileSystem.h"

#include "DashProvider/NVRDashProvider.h"
#include "HeifProvider/NVRHeifProvider.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(VideoProvider)

    const FixedString16 MPD_SUFFIX = FixedString16(".mpd");
    const FixedString16 MP4_SUFFIX = FixedString16(".mp4");
    const FixedString16 HEIF_SUFFIX = FixedString16(".heic");
    const FixedString16 HTTP_PREFIX = "http://";
    const FixedString16 HTTPS_PREFIX = "https://";
    const FixedString32 MPD_FORMAT = "format=mpd-time-csf";

    VideoProvider::VideoProvider()
    : mProviderImpl(OMAF_NULL)
    , mAudioInputBuffer(OMAF_NULL)
    {
    }

    VideoProvider::~VideoProvider()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            mProviderImpl->stop();
            OMAF_DELETE_HEAP(mProviderImpl);
        }
    }

    const CoreProviderSourceTypes& VideoProvider::getSourceTypes()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->getSourceTypes();
        }
        else
        {
            return mVideoSourceTypesEmpty;
        }
    }

    const CoreProviderSources& VideoProvider::getSources()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->getSources();
        }
        else
        {
            return mVideoSourcesEmpty;
        }
    }

    Error::Enum VideoProvider::prepareSources(HeadTransform currentHeadtransform, float32_t fovHorizontal, float32_t fovVertical)
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->prepareSources(currentHeadtransform, fovHorizontal, fovVertical);
        }
        else
        {
            return Error::NOT_INITIALIZED;
        }
    }
    
    Error::Enum VideoProvider::setAudioInputBuffer(AudioInputBuffer *inputBuffer)
    {
        mAudioInputBuffer = inputBuffer;
        
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->setAudioInputBuffer(mAudioInputBuffer);
        }
        return Error::OK;
    }

    void VideoProvider::enter() {
        mProviderImpl->enter();
    }

    void VideoProvider::leave() {
        mProviderImpl->leave();
    }

    VideoProviderState::Enum VideoProvider::getState() const
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->getState();
        }
        else
        {
            return VideoProviderState::INVALID;
        }
    }

    Error::Enum VideoProvider::loadSource(const PathName& source, uint64_t initialPosition)
    {
        if (mProviderImpl != OMAF_NULL && mProviderImpl->getState() != VideoProviderState::STOPPED)
        {
            return Error::INVALID_STATE;
        }
        if (mProviderImpl != OMAF_NULL)
        {
            OMAF_DELETE_HEAP(mProviderImpl);
            mProviderImpl = OMAF_NULL;
        }

        if (!isStreamingSource(source))
        {
            if (!FileSystem::fileExists(source.getData()))
            {
                return Error::FILE_NOT_FOUND;
            }

            if (isOfflineVideoSource(source))
            {
                mProviderImpl = OMAF_NEW_HEAP(MP4VRProvider)();
            }
            else if (isOfflineImageSource(source))
            {
                mProviderImpl = OMAF_NEW_HEAP(HeifProvider)();
            }
            else
            {
                return Error::FILE_NOT_SUPPORTED;
            }
        }
        else if (source.findFirst(MPD_SUFFIX) != Npos || source.findFirst(MPD_FORMAT) != Npos)
        {
            mProviderImpl = OMAF_NEW_HEAP(DashProvider)();
        }
        else
        {
            return Error::FILE_NOT_SUPPORTED;
        }

        if (mProviderImpl != OMAF_NULL)
        {
            if (mAudioInputBuffer != OMAF_NULL)
            {
                Error::Enum result = mProviderImpl->setAudioInputBuffer(mAudioInputBuffer);
                if (result != Error::OK)
                {
                    return result;
                }
            }

            return mProviderImpl->loadSource(source, initialPosition);
        }
        return Error::NOT_SUPPORTED;
    }

    Error::Enum VideoProvider::start()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->start();
        }
        return Error::INVALID_STATE;
    }

    Error::Enum VideoProvider::stop()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            Error::Enum result = mProviderImpl->stop();
            
            OMAF_DELETE_HEAP(mProviderImpl);
            mProviderImpl = OMAF_NULL;
            
            return result;
        }

        return Error::INVALID_STATE;
    }

    Error::Enum VideoProvider::pause()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->pause();
        }
        return Error::INVALID_STATE;
    }

    Error::Enum VideoProvider::next()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->next();
        }

        return Error::INVALID_STATE;
    }

    bool_t VideoProvider::isSeekable()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->isSeekable();
        }
        return false;
    }

    Error::Enum VideoProvider::seekToMs(uint64_t& aSeekMs, SeekAccuracy::Enum seekAccuracy)
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->seekToMs(aSeekMs, seekAccuracy);
        }
        return Error::INVALID_STATE;
    }

    Error::Enum VideoProvider::seekToFrame(uint64_t frame)
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->seekToFrame(frame);
        }
        else
        {
            OMAF_UNUSED_VARIABLE(frame);
            
            return Error::INVALID_STATE;
        }
    }

    uint64_t VideoProvider::durationMs() const
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->durationMs();
        }
        return 0;
    }

    uint64_t VideoProvider::elapsedTimeMs() const
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->elapsedTimeMs();
        }

        return 0;
    }

    MediaInformation VideoProvider::getMediaInformation() const
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->getMediaInformation();
        }
        else
        {
            return MediaInformation();
        }
    }

    Error::Enum VideoProvider::loadAuxiliaryStream(PathName &uri)
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->loadAuxiliaryStream(uri);
        }
        else
        {
            return Error::NOT_SUPPORTED;
        }
    }

    Error::Enum VideoProvider::playAuxiliary()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->playAuxiliary();
        }
        else
        {
            return Error::NOT_SUPPORTED;
        }
    }

    Error::Enum VideoProvider::pauseAuxiliary()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->pauseAuxiliary();
        }
        else
        {
            return Error::NOT_SUPPORTED;
        }
    }
    Error::Enum VideoProvider::stopAuxiliary()
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->stopAuxiliary();
        }
        else
        {
            return Error::NOT_SUPPORTED;
        }
    }

    Error::Enum VideoProvider::seekToMsAuxiliary(uint64_t seekTargetMS)
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->seekToMsAuxiliary(seekTargetMS);
        }
        else
        {
            return Error::NOT_SUPPORTED;
        }
    }

    VideoProviderState::Enum VideoProvider::getAuxiliaryState() const
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->getAuxiliaryState();
        }
        else
        {
            return VideoProviderState::INVALID;
        }
    }


    uint64_t VideoProvider::durationMsAuxiliary() const
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->durationMsAuxiliary();
        }
        return 0;
    }

    uint64_t VideoProvider::elapsedTimeMsAuxiliary() const
    {
        if (mProviderImpl != OMAF_NULL)
        {
            return mProviderImpl->elapsedTimeMsAuxiliary();
        }
        return 0;
    }

    bool_t VideoProvider::isStreamingSource(const PathName &source) const
    {
        return source.findFirst(HTTP_PREFIX) != Npos || source.findFirst(HTTPS_PREFIX) != Npos;
    }

    bool_t VideoProvider::isOfflineVideoSource(const PathName &source) const
    {
        return source.findFirst(MP4_SUFFIX) != Npos;
    }

    bool_t VideoProvider::isOfflineImageSource(const PathName &source) const
    {
        return source.findFirst(HEIF_SUFFIX) != Npos;
    }

OMAF_NS_END
