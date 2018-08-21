
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
#include "Foundation/NVRFixedString.h"
#include "Platform/OMAFDataTypes.h"
#include "Media/NVRMP4ParserDatatypes.h"

#include "Provider/NVRCoreProvider.h"
#include "Foundation/NVRPathName.h"

OMAF_NS_BEGIN

    extern const FixedString16 MPD_SUFFIX;
    extern const FixedString16 MP4_SUFFIX;

    extern const FixedString16 HTTP_PREFIX;

    namespace VideoProviderState
    {
        enum Enum
        {
            INVALID = -1,
            IDLE,
            LOADING,
            LOADED,
            PLAYING,
            BUFFERING,
            PAUSED,
            END_OF_FILE,
            STOPPED,
            CLOSING,
            STREAM_NOT_FOUND,
            CONNECTION_ERROR,
            STREAM_ERROR,
            COUNT
        };
        static const char_t* toString(VideoProviderState::Enum aState)
        {
            switch (aState)
            {
            case VideoProviderState::INVALID:
                return "INVALID";
            case VideoProviderState::IDLE:
                return "IDLE";
            case VideoProviderState::LOADING:
                return "LOADING";
            case VideoProviderState::LOADED:
                return "LOADED";
            case VideoProviderState::PLAYING:
                return "PLAYING";
            case VideoProviderState::BUFFERING:
                return "BUFFERING";
            case VideoProviderState::PAUSED:
                return "PAUSED";
            case VideoProviderState::END_OF_FILE:
                return "END_OF_FILE";
            case VideoProviderState::STOPPED:
                return "STOPPED";
            case VideoProviderState::CLOSING:
                return "CLOSING";
            case VideoProviderState::CONNECTION_ERROR:
                return "CONNECTION_ERROR";
            case VideoProviderState::STREAM_ERROR:
                return "STREAM_ERROR";
            case VideoProviderState::STREAM_NOT_FOUND:
                return "STREAM_NOT_FOUND";
            case VideoProviderState::COUNT:
                return "COUNT";
            default:
                return "";
            }
        }
    }

    class ProviderBase;
    
    class VideoProvider
    : public CoreProvider
    {
    public:

        VideoProvider();
        virtual ~VideoProvider();

    public:  // Inherited from CoreProvider

        virtual const CoreProviderSourceTypes& getSourceTypes();
        virtual Error::Enum prepareSources(HeadTransform currentHeadtransform, float32_t fovHorizontal, float32_t fovVertical);
        virtual const CoreProviderSources& getSources();

        virtual Error::Enum setAudioInputBuffer(AudioInputBuffer *inputBuffer);

    public:  // new methods

        virtual VideoProviderState::Enum getState() const;
        virtual Error::Enum loadSource(const PathName& source, uint64_t initialPositionMS = 0);
        virtual Error::Enum start();
        virtual Error::Enum stop();
        virtual Error::Enum pause();


        virtual Error::Enum loadAuxiliaryStream(PathName& uri);
        virtual VideoProviderState::Enum getAuxiliaryState() const;
        virtual Error::Enum playAuxiliary();
        virtual Error::Enum stopAuxiliary();
        virtual Error::Enum pauseAuxiliary();
        virtual Error::Enum seekToMsAuxiliary(uint64_t seekTargetMS);

        virtual uint64_t durationMsAuxiliary() const;
        virtual uint64_t elapsedTimeMsAuxiliary() const;

        virtual bool_t isSeekable();
        virtual Error::Enum seekToMs(uint64_t& aSeekMs, SeekAccuracy::Enum seekAccuracy);
        virtual Error::Enum seekToFrame(uint64_t frame);

        virtual uint64_t durationMs() const;
        virtual uint64_t elapsedTimeMs() const;

        virtual MediaInformation getMediaInformation() const;

    private:
        bool_t isStreamingSource(const PathName &source) const;
        bool_t isOfflineVideoSource(const PathName &source) const;
        bool_t isOfflineImageSource(const PathName &source) const;

    private:
        ProviderBase* mProviderImpl;

        // Used when there's no actual provider created
        CoreProviderSources mVideoSourcesEmpty;
        CoreProviderSourceTypes mVideoSourceTypesEmpty;

        AudioInputBuffer* mAudioInputBuffer; // Not owned

    };
OMAF_NS_END
