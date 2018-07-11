
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
#include "DashProvider/NVRDashRepresentationTile.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"
#include "DashProvider/NVRDashTimelineStreamDynamic.h"
#include "DashProvider/NVRDashTimelineStreamStatic.h"
#include "DashProvider/NVRDashTemplateStreamStatic.h"
#include "DashProvider/NVRDashTemplateStreamDynamic.h"
#include "DashProvider/NVRMPDAttributes.h"
#include "DashProvider/NVRMPDExtension.h"
#include "DashProvider/NVRDashLog.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashRepresentationTile)

    DashRepresentationTile::DashRepresentationTile()
        : DashRepresentation()
    {
        mInitializeIndependently = false;
    }

    DashRepresentationTile::~DashRepresentationTile()
    {
        //clean segments
        while (!mSegments.isEmpty())
        {
            DashSegment* segment = mSegments[0];
            mSegments.removeAt(0);
            OMAF_DELETE_HEAP(segment);
        }
    }

    void_t DashRepresentationTile::clearDownloadedContent()
    {
        while (!mSegments.isEmpty())
        {
            DashSegment* segment = mSegments[0];
            mSegments.removeAt(0);
            OMAF_DELETE_HEAP(segment);
        }
        DashRepresentation::clearDownloadedContent();
    }

    void_t DashRepresentationTile::cleanUpOldSegments(uint32_t aNextSegmentId)
    {
        DashSegment* segment = peekSegment();
        while (segment != OMAF_NULL && segment->getSegmentId() < aNextSegmentId)
        {
            OMAF_LOG_V("cleanUpOldSegments, target segment id %d, oldest %d => discard", aNextSegmentId, segment->getSegmentId());
            // too old, discard
            segment = getSegment();
            OMAF_DELETE_HEAP(segment);
            segment = peekSegment();
        }
    }
    bool_t DashRepresentationTile::hasSegment(uint32_t aSegmentId, size_t& aSegmentSize)
    {
        DashSegment* segment = peekSegment();
        if (segment == OMAF_NULL || segment->getSegmentId() > aSegmentId)
        {
            //TODO if there was a switch, and the old one has the requested segment but the new one doesn't have (as it starts from the next one), we have a deadlock here. Related to a TODO in the selectQuality
            OMAF_LOG_V("segment %d for %s not yet available for concatenation, oldest %d (0 == NULL)", aSegmentId, getId(), (segment != OMAF_NULL ? segment->getSegmentId() : 0));
            return false;
        }
        else if (segment->getSegmentId() < aSegmentId)
        {
            // technically this should not happen, but has been seen. TODO why
            OMAF_LOG_V("Older segment %d vs %d found %s", segment->getSegmentId(), aSegmentId, getId());
            cleanUpOldSegments(aSegmentId);
            // try again
            return hasSegment(aSegmentId, aSegmentSize);
        }
        else
        {
            // Found!
            aSegmentSize = segment->getDataBuffer()->getSize();
            return true;
        }
    }

    Error::Enum DashRepresentationTile::startDownloadABR(uint32_t overrideSegmentId)
    {
        // overrideSegmentId--; // -1 matches with segmentIndex = mVideoBaseAdaptationSet->peekNextSegmentId() + 1; i.e. if we need to download a new segment, we use +1, but if we have it available, we should use the id from extractor
        if (!mSegments.isEmpty() && mSegments.back()->getSegmentId() >= overrideSegmentId)
        {
            OMAF_LOG_V("Some segments for %s exist, start downloading from segment %d", getId(), mSegments.back()->getSegmentId() + 1);
            // start download from the next not loaded segment in sequence
            return DashRepresentation::startDownloadABR(mSegments.back()->getSegmentId() + 1);
        }
        OMAF_LOG_V("No segments for %s exist, start downloading from segment %d", getId(), overrideSegmentId);
        // start download with the original id; parser may still have useful segments, but it is checked in the base class
        return DashRepresentation::startDownloadABR(overrideSegmentId);
    }

    Error::Enum DashRepresentationTile::handleInputSegment(DashSegment* aSegment)
    {
        // store as cannot be parsed alone
        mSegments.add(aSegment);
        return Error::OK;
    }

    DashSegment* DashRepresentationTile::peekSegment() const
    {
        if (mSegments.getSize() > 0)
        {
            // take the oldest one
            return mSegments[0];
        }
        return OMAF_NULL;
    }

    size_t DashRepresentationTile::getNrSegments() const
    {
        return mSegments.getSize();
    }

    DashSegment* DashRepresentationTile::getSegment()
    {
        if (mSegments.getSize() > 0)
        {
            // take the oldest one
            DashSegment* segment = mSegments[0];
            mSegments.removeAt(0);
            return segment;
        }
        return OMAF_NULL;
    }

    void_t DashRepresentationTile::seekWhenSegmentAvailable()
    {
        // do nothing yet
    }

OMAF_NS_END
