
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

#include "DashProvider/NVRDashRepresentation.h"
#include "NVRNamespace.h"

OMAF_NS_BEGIN

typedef Array<DashSegment*> SegmentStorage;

class DashRepresentationTile : public DashRepresentation
{
public:
    DashRepresentationTile();
    virtual ~DashRepresentationTile();

    virtual void_t clearDownloadedContent(bool_t aResetInitialization = true);
    virtual Error::Enum startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId,
                                                 uint32_t aNextToBeProcessedSegmentId);

    DashSegment* peekSegment() const;
    size_t getNrSegments(uint32_t aNextNeededSegment) const;
    DashSegment* getSegment();
    void_t cleanUpOldSegments(uint32_t aNextSegmentId);
    bool_t hasSegment(const uint32_t aSegmentId, uint32_t& aOldestSegmentId, size_t& aSegmentSize);
    void_t seekWhenSegmentAvailable();

protected:
    Error::Enum handleInputSegment(DashSegment* aSegment, bool_t& aReadyForReading);

private:
    SegmentStorage mSegments;
};
OMAF_NS_END
