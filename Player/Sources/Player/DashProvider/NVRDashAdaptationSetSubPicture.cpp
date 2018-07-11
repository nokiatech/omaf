
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
#include "DashProvider/NVRDashAdaptationSetSubPicture.h"
#include "DashProvider/NVRMPDAttributes.h"
#include "DashProvider/NVRMPDExtension.h"
#include "Foundation/NVRLogger.h"
#include "Media/NVRMediaType.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashAdaptationSetSubPicture)

    DashAdaptationSetSubPicture::DashAdaptationSetSubPicture(DashAdaptationSetObserver& observer)
    : DashAdaptationSet(observer)
    {

    }

    DashAdaptationSetSubPicture::~DashAdaptationSetSubPicture()
    {
    }

    bool_t DashAdaptationSetSubPicture::isSubPicture(DashComponents aDashComponents)
    {
        //TODO how to detect this in OMAF? has a coverage/quality indicating less than 360, but there are no extractors, but there is a 360 video?
        return false;
    }

    AdaptationSetType::Enum DashAdaptationSetSubPicture::getType() const
    {
        return AdaptationSetType::SUBPICTURE;
    }


    DashRepresentation* DashAdaptationSetSubPicture::createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth)
    {
        DashRepresentation* newrepresentation = DashAdaptationSet::createRepresentation(aDashComponents, aInitializationSegmentId, aBandwidth);

        // set cache mode: "manual", i.e. monitor also other parameters than download cache
        newrepresentation->setCacheFillMode(false);

        return newrepresentation;
    }


    bool_t DashAdaptationSetSubPicture::parseVideoProperties(DashComponents& aNextComponents)
    {
        //TODO 
        return false;
    }

    void_t DashAdaptationSetSubPicture::parseVideoViewport(DashComponents& aNextComponents)
    {
        //TODO

        mContent.addType(MediaContent::Type::VIDEO_BASE);
    }

    void_t DashAdaptationSetSubPicture::doSwitchRepresentation()
    {
#ifdef OMAF_PLATFORM_ANDROID
        OMAF_LOG_V("%d Switch done from %d to %d", Time::getClockTimeMs(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getBitrate());
#else
        OMAF_LOG_V("%lld Switch done from %s (%d) to %s (%d)", Time::getClockTimeMs(), mCurrentRepresentation->getId(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getId(), mNextRepresentation->getBitrate());
#endif
        mCurrentRepresentation = mNextRepresentation;
        mNextRepresentation = OMAF_NULL;
    }

OMAF_NS_END
