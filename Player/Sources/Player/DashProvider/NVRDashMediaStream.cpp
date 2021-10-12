
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
#include "NVRDashMediaStream.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Media/NVRMP4Parser.h"
#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaPacket.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashMediaStream)

DashMetadataStream::DashMetadataStream(MediaFormat* format, MP4VRParser& aParser)
    : MP4MediaStream(format)
    , mParser(aParser)
{
}

DashMetadataStream::~DashMetadataStream()
{
}


Error::Enum DashMetadataStream::getPropertyOverlayConfig(MP4VR::OverlayConfigProperty& overlayConfigProperty,
                                                         uint32_t aSampleId) const
{
    return mParser.getPropertyOverlayConfig(getTrackId(), aSampleId, overlayConfigProperty);
}


DashVideoStream::DashVideoStream(MediaFormat* format, MP4VRParser& aParser)
    : MP4VideoStream(format)
    , mParser(aParser)
    , mAdaptationSet(OMAF_NULL)
{
}

DashVideoStream::~DashVideoStream()
{
}


Error::Enum DashVideoStream::getPropertyOverlayConfig(MP4VR::OverlayConfigProperty& overlayConfigProperty) const
{
    // Note: the sampleId needs to be a valid sample id, that the parser currently has. For video we
    // can use the next available sample id
    uint32_t sampleId;
    peekNextSample(sampleId);
    return mParser.getPropertyOverlayConfig(getTrackId(), sampleId, overlayConfigProperty);
}

void_t DashVideoStream::setAdaptationSetReference(DashAdaptationSet* aAdaptationSet)
{
    mAdaptationSet = aAdaptationSet;
}

DashAdaptationSet* DashVideoStream::getAdaptationSetReference()
{
    return mAdaptationSet;
}

OMAF_NS_END
