
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
#include "DashProvider/NVRDashRepresentationOverlay.h"
#include "DashProvider/NVRDashLog.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashRepresentationOverlay)

DashRepresentationOverlay::DashRepresentationOverlay()
    : DashRepresentation()
{
}

DashRepresentationOverlay::~DashRepresentationOverlay()
{
}

void_t DashRepresentationOverlay::createVideoSource(sourceid_t& sourceId,
                                                    SourceType::Enum sourceType,
                                                    StereoRole::Enum channel)
{
    if (mVideoStreamId == OMAF_UINT8_MAX)
    {
        // when creating sources from MPD data, we need the stream id before streams have been generated
        mVideoStreamId = VideoDecoderManager::getInstance()->generateUniqueStreamID();
    }

    // skip it for now, read it from first segment instead
    mSourceType = sourceType;
    mSourceId = sourceId;
    mRole = channel;
    return;
}

void_t DashRepresentationOverlay::initializeVideoSource() {
    auto ret = getParserInstance()->parseVideoSources(*mVideoStreams[0], mSourceType, mBasicSourceInfo);

	// create video source only now
    if (ret != Error::OK && ret != Error::OK_SKIPPED)
    {
        return;
    }
    DashRepresentation::createVideoSource(mSourceId, mBasicSourceInfo.sourceType, mRole);

    getParserInstance()->setInitialOverlayMetadata(*mVideoStreams.at(0));
}

OMAF_NS_END
