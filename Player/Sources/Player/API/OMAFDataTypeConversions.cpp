
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
#include "API/OMAFDatatypeConversions.h"

namespace OMAF
{
    OMAF::Result::Enum convertResult(Private::Error::Enum result)
    {
        switch (result)
        {
            case Private::Error::OK:                            return OMAF::Result::OK;
            case Private::Error::OK_SKIPPED:
            case Private::Error::OK_NO_CHANGE:                  return OMAF::Result::NO_CHANGE;
            case Private::Error::OK_FILE_PARTIALLY_SUPPORTED:   return OMAF::Result::FILE_PARTIALLY_SUPPORTED;
            case Private::Error::END_OF_FILE:                   return OMAF::Result::END_OF_FILE;
            case Private::Error::OUT_OF_MEMORY:                 return OMAF::Result::OUT_OF_MEMORY;
            case Private::Error::OPERATION_FAILED:              return OMAF::Result::OPERATION_FAILED;
            case Private::Error::ALREADY_SET:
            case Private::Error::INVALID_STATE:
            case Private::Error::NOT_INITIALIZED:               return OMAF::Result::INVALID_STATE;
            case Private::Error::ITEM_NOT_FOUND:                return OMAF::Result::ITEM_NOT_FOUND;
            case Private::Error::BUFFER_OVERFLOW:               return OMAF::Result::BUFFER_OVERFLOW;
            case Private::Error::NOT_SUPPORTED:                 return OMAF::Result::NOT_SUPPORTED;
            case Private::Error::INVALID_DATA:                  return OMAF::Result::INVALID_DATA;
            case Private::Error::FILE_NOT_FOUND:                return OMAF::Result::FILE_NOT_FOUND;
            case Private::Error::FILE_OPEN_FAILED:              return OMAF::Result::FILE_OPEN_FAILED;
            case Private::Error::FILE_NOT_MP4:
            case Private::Error::FILE_NOT_SUPPORTED:
            default:
                return OMAF::Result::OPERATION_FAILED;

        }
    }
    
    Private::AudioOutputRange::Enum convertAudioOutputRange(AudioOutputRange::Enum omafRange)
    {
        switch (omafRange)
        {
            case OMAF::AudioOutputRange::INVALID:    return Private::AudioOutputRange::INVALID;
            case OMAF::AudioOutputRange::COUNT:      return Private::AudioOutputRange::COUNT;
                
            case OMAF::AudioOutputRange::FLOAT_1:    return Private::AudioOutputRange::FLOAT_1;
            case OMAF::AudioOutputRange::FLOAT_16:   return Private::AudioOutputRange::FLOAT_16;
            case OMAF::AudioOutputRange::INT16:      return Private::AudioOutputRange::INT16;
  
            default:                                return Private::AudioOutputRange::INVALID;
        }
    }
    
    OMAF::AudioReturnValue::Enum convertAudioResult(Private::AudioReturnValue::Enum nvrReturnValue)
    {
        switch (nvrReturnValue)
        {
            case Private::AudioReturnValue::INVALID:            return OMAF::AudioReturnValue::INVALID;
            case Private::AudioReturnValue::COUNT:              return OMAF::AudioReturnValue::COUNT;
                
            case Private::AudioReturnValue::OK:                 return OMAF::AudioReturnValue::OK;
            case Private::AudioReturnValue::END_OF_FILE:        return OMAF::AudioReturnValue::END_OF_FILE;
            case Private::AudioReturnValue::OUT_OF_SAMPLES:     return OMAF::AudioReturnValue::OUT_OF_SAMPLES;
            case Private::AudioReturnValue::FAIL:               return OMAF::AudioReturnValue::FAIL;
            case Private::AudioReturnValue::NOT_INITIALIZED:    return OMAF::AudioReturnValue::NOT_INITIALIZED;

            default:                                        return OMAF::AudioReturnValue::INVALID;
        }
    }
    
    Private::PlaybackMode::Enum convertAudioPlaybackMode(AudioPlaybackMode::Enum omafMode)
    {
        switch (omafMode)
        {
            case OMAF::AudioPlaybackMode::INVALID:                   return Private::PlaybackMode::INVALID;
            case OMAF::AudioPlaybackMode::COUNT:                     return Private::PlaybackMode::COUNT;
                
            case OMAF::AudioPlaybackMode::HEADPHONES:                return Private::PlaybackMode::HEADPHONES;
            case OMAF::AudioPlaybackMode::HEADTRACKED_HEADPHONES:    return Private::PlaybackMode::HEADTRACKED_HEADPHONES;

            default:                                                return Private::PlaybackMode::INVALID;
        }
    }
    
    VideoPlaybackState::Enum convertVideoPlaybackState(Private::VideoProviderState::Enum providerState)
    {
        switch (providerState)
        {
            case Private::VideoProviderState::INVALID:
            case Private::VideoProviderState::IDLE:
            case Private::VideoProviderState::STOPPED:
            {
                return OMAF::VideoPlaybackState::IDLE;
            }

            case Private::VideoProviderState::LOADING:
            {
                return OMAF::VideoPlaybackState::LOADING;
            }

            case Private::VideoProviderState::PLAYING:
            {
                return OMAF::VideoPlaybackState::PLAYING;
            }
                
            case Private::VideoProviderState::BUFFERING:
            {
                return OMAF::VideoPlaybackState::BUFFERING;
            }
            
            case Private::VideoProviderState::LOADED:
            case Private::VideoProviderState::PAUSED:
            {
                return OMAF::VideoPlaybackState::PAUSED;
            }

            case Private::VideoProviderState::END_OF_FILE:
            {
                return OMAF::VideoPlaybackState::END_OF_FILE;
            }

            case Private::VideoProviderState::STREAM_ERROR:
            {
                return OMAF::VideoPlaybackState::STREAM_ERROR;
            }
                
            case Private::VideoProviderState::CONNECTION_ERROR:
            {
                return OMAF::VideoPlaybackState::CONNECTION_ERROR;
            }

            case Private::VideoProviderState::STREAM_NOT_FOUND:
            {
                return OMAF::VideoPlaybackState::STREAM_NOT_FOUND;
            }
                
            default:
            {
                return OMAF::VideoPlaybackState::INVALID;
            }
        }
    }

    OMAF::StreamType::Enum convertStreamType(Private::StreamType::Enum type)
    {
        switch (type)
        {
        case Private::StreamType::LIVE_STREAM:
            return OMAF::StreamType::LIVE_STREAM;
        case Private::StreamType::LOCAL_FILE:
            return OMAF::StreamType::LOCAL_FILE;
        case Private::StreamType::VIDEO_ON_DEMAND:
            return OMAF::StreamType::VIDEO_ON_DEMAND;
        default:
            return OMAF::StreamType::INVALID;
        }
    }

    OMAF::MediaInformation convertMediaInformation(Private::MediaInformation information)
    {
        OMAF::MediaInformation mediaInformation;
        mediaInformation.width = information.width;
        mediaInformation.height = information.height;
        mediaInformation.duration = information.duration;
        mediaInformation.isStereoscopic = information.isStereoscopic;
        mediaInformation.streamType = convertStreamType(information.streamType);
        return mediaInformation;
    }

    Private::SeekAccuracy::Enum convertSeekAccuracy(OMAF::SeekAccuracy::Enum accuracy)
    {
        switch (accuracy)
        {
        case OMAF::SeekAccuracy::FRAME_ACCURATE:
            return Private::SeekAccuracy::FRAME_ACCURATE;
        case OMAF::SeekAccuracy::NEAREST_SYNC_FRAME:
            return Private::SeekAccuracy::NEAREST_SYNC_FRAME;
        default:
            return Private::SeekAccuracy::INVALID;
        }

    }
}
