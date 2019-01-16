
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
#include "DashProvider/NVRDashAdaptationSetViewportDep.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"
#include "VAS/NVRLatencyLog.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashAdaptationSetViewportDep)

    DashAdaptationSetViewportDep::DashAdaptationSetViewportDep(DashAdaptationSetObserver& observer)
    : DashAdaptationSet(observer)
    , mStartSegment(0)
    {
    }

    DashAdaptationSetViewportDep::~DashAdaptationSetViewportDep()
    {
    }

    void_t DashAdaptationSetViewportDep::startedDownloadFromSegment(uint32_t aTargetDownloadSegmentId)
    {
        mStartSegment = aTargetDownloadSegmentId;
        mStartTimeMs = Time::getClockTimeMs();
    }

    void_t DashAdaptationSetViewportDep::switchSegmentDownloaded(uint32_t aSwitchedSegment)
    {
        if (mStartSegment > 0 && aSwitchedSegment >= mStartSegment)
        {
            uint32_t now = Time::getClockTimeMs();
            uint32_t segments = aSwitchedSegment - mStartSegment + 1;
            uint32_t duration = (now - mStartTimeMs) / segments;
            if (mSwitchTimesMs.getSize() >= mSwitchTimesMs.getCapacity())
            {
                mSwitchTimesMs.removeAt(0);
            }
            mSwitchTimesMs.add(duration);
            OMAF_LOG_D("switchSegmentDownloaded, target %d actual %d within %d ms/segment", mStartSegment, aSwitchedSegment, duration);
            mStartSegment = 0;
        }
    }

    uint32_t DashAdaptationSetViewportDep::estimateSegmentIdForSwitch(uint32_t aNextSegmentNotDownloaded)
    {
        uint32_t segmentIndex = 0;
        // Check where the read head is now (possibly gives +1 segment). T
        // This does not and should not consider any download latencies, since we may have the right segment already cached from previous tile switches
        OMAF_LOG_V("estimateSegmentIdForSwitch using %s", getCurrentRepresentationId().getData());
        segmentIndex = peekNextSegmentId();

        if (segmentIndex == 0 || segmentIndex == INVALID_SEGMENT_INDEX)
        {
            // switching earlier than the current adaptation set was taken into use, fallback to the given segment index
            OMAF_LOG_V("estimateSegmentIdForSwitch using %s - no segments available, set index to %d", getCurrentRepresentationId().getData(), aNextSegmentNotDownloaded);
            segmentIndex = aNextSegmentNotDownloaded;
        }
        else
        {
            // try to estimate the switch position based on download latency history, this may result in overlapped segments but should help with switching latency

            // round up as download always takes more than 0 ms
            uint64_t segmentDurationMs = 0;
            if (getCurrentRepresentation()->isSegmentDurationFixed(segmentDurationMs))
            {
                uint32_t downloadSkip = 0;
                if (!mSwitchTimesMs.isEmpty())
                {
                    //gets avg of largest and smallest 
                    uint32_t largest = mSwitchTimesMs.at(0);
                    uint32_t smallest = mSwitchTimesMs.at(0);
                    for (SwitchTimes::ConstIterator it = mSwitchTimesMs.begin(); it != mSwitchTimesMs.end(); ++it)
                    {
                        if (*it > largest)
                        {
                            largest = *it;
                        }
                        else if (*it < smallest)
                        {
                            smallest = *it;
                        }
                    }
                    uint64_t avgSwitchTimeMs = (uint64_t)((largest + smallest) / 2.f);//TODO this includes both low and high Q representations
                    // This now assumes the playhead is in the middle of the previously concatenated segment. 
                    // We can't get the position of the real playhead easily here, because provider buffers data to video decoder, and hence we'd need to know the position of the rendering playhead, not the reader head. 
                    // Further, if the stream is encoded with hierarchical B-tree, the read position doesn't tell much as the segment may be emptied right after it was concatenated.
                    uint64_t skipTimeMs = segmentDurationMs / 2;
                    while (skipTimeMs < avgSwitchTimeMs)
                    {
                        skipTimeMs += segmentDurationMs;
                        downloadSkip++;
                    }
                    OMAF_LOG_V("estimateSegmentIdForSwitch - largest %d ms smallest %d ms", largest, smallest);
                    LATENCY_LOG_D("estimateSegmentIdForSwitch: current %d next dl %d skip %d, largest %d ms smallest %d ms", segmentIndex, aNextSegmentNotDownloaded, downloadSkip, largest, smallest);
                }
                else
                {
                    OMAF_LOG_V("estimateSegmentIdForSwitch - no data to estimate");
                    downloadSkip = 1;
                }
                segmentIndex += downloadSkip;
                // ensure there are no gaps when switching from current to next
                segmentIndex = min(segmentIndex, aNextSegmentNotDownloaded);
                OMAF_LOG_D("estimateSegmentIdForSwitch: %d", segmentIndex);
            }
            else
            {
                //TODO??
            }
        }
        OMAF_ASSERT((int32_t)segmentIndex > 0, "Segment index <= 0!");
        return segmentIndex;
    }

    bool_t DashAdaptationSetViewportDep::readyToSwitch(DashRepresentation* aRepresentation, uint32_t aNextNeededSegment)
    {
        if (aRepresentation == OMAF_NULL)
        {
            return false;
        }
        DashSegment* segment = aRepresentation->peekSegment();
        if (segment == OMAF_NULL)
        {
            OMAF_LOG_V("readyToSwitch %s no segment found for %d, not ready", aRepresentation->getId(), aNextNeededSegment);
            return false;
        }
        else if (segment->getSegmentId() > aNextNeededSegment)
        {
            OMAF_LOG_V("readyToSwitch %s found only newer %d vs %d, not ready", aRepresentation->getId(), segment->getSegmentId(), aNextNeededSegment);
            return false;
        }
        else if (segment->getSegmentId() < aNextNeededSegment)
        {
            OMAF_LOG_V("readyToSwitch %s found older %d vs %d, cleanup and try again", aRepresentation->getId(), segment->getSegmentId(), aNextNeededSegment);
            aRepresentation->cleanUpOldSegments(aNextNeededSegment);
            // recursively try again
            return readyToSwitch(aRepresentation, aNextNeededSegment);
        }
        uint64_t bufferedDataMs = 0;
        uint64_t thresholdMs = 500;
        uint64_t segmentDurationMs = 0;
        if (aRepresentation->isSegmentDurationFixed(segmentDurationMs))
        {
            float32_t sf = getDownloadSpeedFactor();
            if (sf == 0.f)
            {
                sf = 1.f;
            }
            bufferedDataMs = aRepresentation->getNrSegments(aNextNeededSegment) * segmentDurationMs;
            thresholdMs = max((uint64_t)(segmentDurationMs / sf), (uint64_t)400); // e.g. duration = 1sec, speed factor = 4 (250 msec per segment) => threshold = max(250, 400)
            OMAF_LOG_V("readyToSwitch %s: buffered %lld, threshold %lld, factor %f", aRepresentation->getId(), bufferedDataMs, thresholdMs, sf);
        }
        else
        {
            // in timeline mode the durations are not fixed, so needs some other logic here
            //bufferedDataMs = mNextRepresentation->getNrSegments() * XX
        }

        if (bufferedDataMs > thresholdMs)
        {
            // switching to a cached segment, measurement doesn't make sense
            OMAF_LOG_D("readyToSwitch to %s at %d", aRepresentation->getId(), aNextNeededSegment);
            LATENCY_LOG_D("switched to %s: buffered %lld, threshold %lld, factor %f", aRepresentation->getId(), bufferedDataMs, thresholdMs);
            return true;
        }
        return false;
    }


OMAF_NS_END
