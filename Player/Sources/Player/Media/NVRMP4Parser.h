
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
#include "Foundation/NVRMutex.h"
#include "Foundation/NVRFixedQueue.h"

#include <mp4vrfiledatatypes.h>

#include "Provider/NVRSources.h"
#include "Metadata/NVRMetadataParser.h"

#include "Media/NVRMediaFormat.h"
#include "Media/NVRMP4FileStreamer.h"
#include "Media/NVRMediaInformation.h"
#include "Media/NVRMP4ParserDatatypes.h"
#include "Media/NVRMP4VideoStream.h"
#include "Media/NVRMP4AudioStream.h"

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
    #include "DashProvider/NVRDashMediaSegment.h"
#endif

namespace MP4VR {
    class MP4VRFileReaderInterface;
}

OMAF_NS_BEGIN


    struct SubSegment
    {
        uint32_t segmentId;
        uint64_t earliestPresentationTimeMs;
        uint64_t startByte;
        uint64_t endByte;
    };

    class MP4AudioStream;
    class MP4VideoStream;
    class MP4MediaStream;
    class MP4VRMediaPacket;
#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
    class MP4SegmentStreamer;
    static const uint32_t SEGMENT_INDEX_COUNT_MAX = 10;
    static const uint32_t INIT_SEGMENT_COUNT_MAX = 10;

    typedef FixedArray<MP4VR::DynArray<MP4VR::SegmentInformation>*, SEGMENT_INDEX_COUNT_MAX*2> SegmentIndexes;

    typedef uint32_t InitSegmentId;
    typedef Pair<InitSegmentId, MP4SegmentStreamer*> InitSegment;
    typedef FixedArray<InitSegment, INIT_SEGMENT_COUNT_MAX> InitSegments;
    typedef FixedArray<SegmentContent, INIT_SEGMENT_COUNT_MAX> InitSegmentsContent;

    typedef FixedQueue<MP4SegmentStreamer*, 100> MediaSegments;
    typedef HashMap<InitSegmentId, MediaSegments*> MediaSegmentMap;
#endif

    class MP4VRParser 
    {

    public:

        MP4VRParser();
        virtual ~MP4VRParser();

        /**
         * Opens a given input.
         * @param mediaUri The media URI to open.
         * @return true if successful, false otherwise.
         */
        virtual Error::Enum openInput(const PathName& mediaUri, MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
        virtual Error::Enum openSegmentedInput(MP4Segment* mp4Segment, bool_t aClientAssignVideoStreamId);
        virtual Error::Enum addSegment(MP4Segment* mp4Segment, MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);
        virtual Error::Enum addSegmentIndex(MP4Segment* mp4Segment);
        virtual Error::Enum hasSegmentIndexFor(uint32_t segmentId, uint64_t presentationTimeUs);
        virtual Error::Enum getSegmentIndexFor(uint32_t segmentId, uint64_t presentationTimeUs, SubSegment& subSegment);
        virtual Error::Enum removeSegment(uint32_t initSegmentId, uint32_t segmentId, MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);
        virtual bool_t getNewestSegmentId(uint32_t initSegmentId, uint32_t& segmentId);
        virtual uint64_t getReadPositionUs(MP4VideoStreams& videoStreams, uint32_t& segmentIndex);
        virtual bool_t readyForSegment(MP4VideoStreams& videoStreams, uint32_t aSegmentId);

        uint32_t releaseSegmentsUntil(uint32_t segmentId, MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);
        void_t releaseUsedSegments(MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);
        bool_t releaseAllSegments(MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);
        bool_t hasSegmentIndexForSegmentId(uint32_t segmentId);
#endif
        /**
         * Returns read frame as a MediaPacket.
         */
        virtual Error::Enum readFrame(MP4MediaStream& stream, bool_t& segmentChanged, int64_t currentTimeUs = -1);

        virtual Error::Enum readInitialMetadata(const PathName& mediaUri, MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);

        virtual Error::Enum parseVideoSources(MP4VideoStream& aVideoStream, SourceType::Enum aSourceType, BasicSourceInfo& aBasicSourceInfo);
        virtual const CoreProviderSources& getVideoSources();
        virtual const CoreProviderSourceTypes& getVideoSourceTypes();

        virtual MetadataParser& getMetadataParser();

        const MediaInformation& getMediaInformation() const;

        /**
         * Seeks to a given position.
         * @param seekPosUs the seek position in micro seconds. 
         * @param seekPosFrame where to seek to.
         * @return true if successful, false otherwise.
         */
        virtual bool_t seekToUs(uint64_t& seekPosUs, MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams, SeekDirection::Enum mode, SeekAccuracy::Enum accuracy);
        /**
        * Seeks to a given position.
        * @param seekFrameNr video frame where to seek to. 
        * @return true if successful, false otherwise.
        */
        virtual bool_t seekToFrame(int32_t seekFrameNr, uint64_t& seekPosUs, MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);// , SeekType mode);

        virtual uint64_t getNextVideoTimestampUs(MP4MediaStream& stream);

        virtual int64_t seekToSyncFrame(uint32_t sampleId, MP4MediaStream* stream, SeekDirection::Enum mode, bool_t& segmentChanged);

        virtual bool_t isEOS(const MP4AudioStreams& audioStreams, const MP4VideoStreams& videoStreams) const;

    protected:
        virtual Error::Enum prepareFile(MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams, uint32_t initSegmentId = 0);
        virtual Error::Enum createStreams(MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams, FileFormatType::Enum fileFormat);
        virtual Error::Enum updateStreams(MP4AudioStreams& audioStreams, MP4VideoStreams& videoStreams);

        virtual bool_t readAACAudioMetadata(MP4AudioStream& stream);

    private:
        MemoryAllocator& mAllocator;

        MP4VR::MP4VRFileReaderInterface* mReader;

        bool_t mTimedMetadata;

        // holder for the tracks; accessed via streams who have handle to their corresponding track
        MP4VR::DynArray<MP4VR::TrackInformation>* mTracks;

        MetadataParser mMetaDataParser;

        MP4FileStreamer mFileStream;

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
        InitSegments mInitSegments;
        MediaSegmentMap mMediaSegments;
        SegmentIndexes mSegmentIndexes;
        InitSegmentsContent mInitSegmentsContent;
#endif
        Mutex mReaderMutex;
        MediaInformation mMediaInformation;

        bool_t mClientAssignVideoStreamId;
    };
OMAF_NS_END

