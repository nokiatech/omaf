
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

#include "NVRNamespace.h"

#include "NVRPlayer.h"
#include "OMAFPlayerDataTypes.h"
#include "Media/NVRMediaInformation.h"

namespace OMAF
{
    OMAF::Result::Enum convertResult(Private::Error::Enum result);
    OMAF::VideoPlaybackState::Enum convertVideoPlaybackState(Private::VideoProviderState::Enum providerState);

    OMAF::AudioReturnValue::Enum convertAudioResult(Private::AudioReturnValue::Enum nvrReturnValue);

    OMAF::StreamType::Enum convertStreamType(Private::StreamType::Enum type);
    OMAF::MediaInformation convertMediaInformation(Private::MediaInformation information);

    Private::SeekAccuracy::Enum convertSeekAccuracy(OMAF::SeekAccuracy::Enum accuracy);
}
