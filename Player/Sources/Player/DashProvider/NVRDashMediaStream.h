
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
#include "Media/NVRMP4MediaStream.h"
#include "Media/NVRMP4VideoStream.h"

OMAF_NS_BEGIN

class DashAdaptationSet;

/**
 *  DASH-specific variant of MP4MediaStream.
 */
class DashMetadataStream : public MP4MediaStream
{
public:
    DashMetadataStream(MediaFormat* format, MP4VRParser& aParser);
    virtual ~DashMetadataStream();

    virtual Error::Enum getPropertyOverlayConfig(MP4VR::OverlayConfigProperty& overlayConfigProperty,
                                                 uint32_t aSampleId) const;

private:
    const MP4VRParser& mParser;
};

/**
 *  DASH-specific variant of MP4VideoStream. Cannot be inherited from DashMetadataStream, since should extend
 * MP4VideoStream
 */
class DashVideoStream : public MP4VideoStream
{
public:
    DashVideoStream(MediaFormat* format, MP4VRParser& aParser);
    virtual ~DashVideoStream();

    virtual Error::Enum getPropertyOverlayConfig(MP4VR::OverlayConfigProperty& overlayConfigProperty) const;

    virtual void_t setAdaptationSetReference(DashAdaptationSet* aAdaptationSet);
    virtual DashAdaptationSet* getAdaptationSetReference();

private:
    const MP4VRParser& mParser;
    DashAdaptationSet* mAdaptationSet;
};

typedef FixedArray<DashVideoStream*, 256> DashVideoStreams;

OMAF_NS_END
