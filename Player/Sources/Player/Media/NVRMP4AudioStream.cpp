
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
#include "Media/NVRMP4AudioStream.h"
#include "Foundation/NVRMemorySystem.h"

OMAF_NS_BEGIN

MP4AudioStream::MP4AudioStream(MediaFormat* format)
    : MP4MediaStream(format)
    , mSpatialAudioType(SpatialAudioType::LEGACY_2D)
    , mChannels(*MemorySystem::DefaultHeapAllocator())
    , mGains(*MemorySystem::DefaultHeapAllocator())
{
    mIsAudio = true;
    mIsVideo = false;
}

MP4AudioStream::~MP4AudioStream()
{
    mChannels.clear();
    mGains.clear();
}

void_t MP4AudioStream::setSpatialAudioType(SpatialAudioType::Enum audioType)
{
    mSpatialAudioType = audioType;
}
SpatialAudioType::Enum MP4AudioStream::getSpatialAudioType()
{
    return mSpatialAudioType;
}
bool_t MP4AudioStream::isSpatialAudioType()
{
    return false;
}

void_t MP4AudioStream::addPosition(uint8_t sourceId, Vector3 position)
{
    mChannels.insert(sourceId, position);
}

void_t MP4AudioStream::addGain(uint8_t sourceId, float32_t gain)
{
    mGains.insert(sourceId, gain);
}
OMAF_NS_END
