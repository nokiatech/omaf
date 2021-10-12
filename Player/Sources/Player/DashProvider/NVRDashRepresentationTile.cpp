
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
#include "DashProvider/NVRDashRepresentationTile.h"
#include "DashProvider/NVRDashLog.h"
#include "DashProvider/NVRDashTemplateStreamDynamic.h"
#include "DashProvider/NVRDashTemplateStreamStatic.h"
#include "DashProvider/NVRMPDAttributes.h"
#include "DashProvider/NVRMPDExtension.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashRepresentationTile)

DashRepresentationTile::DashRepresentationTile()
    : DashRepresentation()
    , mSegments(mMemoryAllocator, 10)
{
    mInitializeIndependently = false;
}

DashRepresentationTile::~DashRepresentationTile()
{
    // clean segments
    while (!mSegments.isEmpty())
    {
        DashSegment* segment = mSegments[0];
        mSegments.removeAt(0);
        OMAF_DELETE_HEAP(segment);
    }
}

void_t DashRepresentationTile::clearDownloadedContent(bool_t aResetInitialization)
{
    while (!mSegments.isEmpty())
    {
        DashSegment* segment = mSegments[0];
        mSegments.removeAt(0);
        OMAF_DELETE_HEAP(segment);
    }
    DashRepresentation::clearDownloadedContent(aResetInitialization);
}

void_t DashRepresentationTile::cleanUpOldSegments(uint32_t aNextSegmentId)
{
    if (aNextSegmentId == INVALID_SEGMENT_INDEX)
    {
        // we are clearing everything, can happen e.g. after pause + resume, so need to reset also the download counter
        mLastSegmentId = 0;
    }
    DashSegment* segment = peekSegment();
    while (segment != OMAF_NULL && segment->getSegmentId() < aNextSegmentId)
    {
        OMAF_LOG_V("cleanUpOldSegments for %s, target segment id %d, oldest %d => discard", getId(), aNextSegmentId,
                   segment->getSegmentId());
        // too old, discard
        segment = getSegment();
        OMAF_DELETE_HEAP(segment);
        segment = peekSegment();
    }
}
bool_t DashRepresentationTile::hasSegment(const uint32_t aSegmentId, uint32_t& aOldestSegmentId, size_t& aSegmentSize)
{
    DashSegment* segment = peekSegment();
    if (segment == OMAF_NULL)
    {
        return false;
    }
    else if (segment->getSegmentId() > aSegmentId)
    {
        aOldestSegmentId = segment->getSegmentId();
        return false;
    }
    else if (segment->getSegmentId() < aSegmentId)
    {
        OMAF_LOG_V("Older segment %d vs %d found %s", segment->getSegmentId(), aSegmentId, getId());
        cleanUpOldSegments(aSegmentId);
        // try again
        return hasSegment(aSegmentId, aOldestSegmentId, aSegmentSize);
    }
    else
    {
        // Found!
        aSegmentSize = segment->getDataBuffer()->getSize();
        return true;
    }
}

Error::Enum DashRepresentationTile::startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId,
                                                             uint32_t aNextToBeProcessedSegmentId)
{
    uint32_t lastAvailableSegment = 0;
    if (!mSegments.isEmpty())
    {
        OMAF_LOG_V("startDownloadFromSegment, last ones: %d %d", lastAvailableSegment,
                   mSegments.back()->getSegmentId());
        lastAvailableSegment = mSegments.back()->getSegmentId();
        if (lastAvailableSegment >= aTargetDownloadSegmentId)
        {
            OMAF_LOG_V("Some segments for %s exist, start downloading from segment %d", getId(),
                       lastAvailableSegment + 1);
            // start download from the next not loaded segment in sequence
            aTargetDownloadSegmentId = lastAvailableSegment + 1;
        }
    }
    if (aNextToBeProcessedSegmentId == INVALID_SEGMENT_INDEX)
    {
        OMAF_LOG_V("Cleanup all segments older than the new target %d", aTargetDownloadSegmentId);
        cleanUpOldSegments(aTargetDownloadSegmentId);
    }
    else
    {
        OMAF_LOG_V("Cleanup up to the next to be processed segment %d", aNextToBeProcessedSegmentId);
        cleanUpOldSegments(aNextToBeProcessedSegmentId);
    }
    return DashRepresentation::startDownloadFromSegment(aTargetDownloadSegmentId, aNextToBeProcessedSegmentId);
}

Error::Enum DashRepresentationTile::handleInputSegment(DashSegment* aSegment, bool_t& aReadyForReading)
{
    // store as cannot be parsed alone
    aReadyForReading = false;
    OMAF_LOG_V("handleInputSegment %s %d", getId(), aSegment->getSegmentId());
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

size_t DashRepresentationTile::getNrSegments(uint32_t aNextNeededSegment) const
{
    if (mSegments.isEmpty())
    {
        return 0;
    }
    size_t count = 0;
    uint32_t expectedNextId = mSegments.at(0)->getSegmentId();
    for (SegmentStorage::ConstIterator it = mSegments.begin(); it != mSegments.end(); ++it)
    {
        if ((*it)->getSegmentId() != expectedNextId)
        {
            break;
        }
        else if ((*it)->getSegmentId() >= aNextNeededSegment)
        {
            count++;
            expectedNextId++;
        }
    }
    return count;
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
    mSeekToUsWhenDLComplete = OMAF_UINT64_MAX;
}

OMAF_NS_END
