
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
#pragma once
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

#include <mp4vrfiledatatypes.h>
#include <mp4vrfilereaderinterface.h>

#include "Media/NVRMP4VideoStream.h"

#include "NVRErrorCodes.h"

OMAF_NS_BEGIN

    class HeifImageStream : public MP4VideoStream
    {
    public:
        HeifImageStream(MediaFormat* format);
        virtual ~HeifImageStream();

        virtual void_t setTimestamps(MP4VR::MP4VRFileReaderInterface* reader);

        virtual uint32_t getMaxSampleSize() const;

        virtual void_t setTrack(MP4VR::TrackInformation* track);
        // peek, no index changed
        virtual Error::Enum peekNextSample(uint32_t& sample) const;
        virtual Error::Enum peekNextSampleTs(MP4VR::TimestampIDPair& sample) const;
        // also incements index by 1
        virtual Error::Enum getNextSample(uint32_t& sample, uint64_t& timeMs, bool_t& configChanged, uint32_t& configId, bool_t& segmentChanged);
        virtual Error::Enum getNextSampleTs(MP4VR::TimestampIDPair& sample);
        // used when seeking
        virtual bool_t findSampleIdByTime(uint64_t timeMs, uint64_t& finalTimeMs, uint32_t& sampleId);
        virtual bool_t findSampleIdByIndex(uint32_t sampleNr, uint32_t& sampleId);
        virtual void_t setNextSampleId(uint32_t id, bool_t& segmentChanged);
        virtual uint32_t getSamplesLeft() const;

        virtual uint64_t getFrameCount() const;

        virtual bool_t isEoF() const;

        virtual bool_t isDepth();

        virtual bool_t isReferenceFrame();

    public: // new
        // as still images are stored as items, not as tracks, we need a way to stop the reading
        // TODO is this elegant enough?
        void_t setPrimaryAsRead();
        bool_t isPrimaryRead();
    private:
        bool_t mPrimaryRead;
    };
OMAF_NS_END
