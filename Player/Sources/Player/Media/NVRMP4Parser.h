
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

#include "Foundation/NVRFixedQueue.h"
#include "Foundation/NVRMutex.h"
#include "NVRNamespace.h"

#include <reader/mp4vrfiledatatypes.h>

#include "Metadata/NVRMetadataParser.h"

#include "Media/NVRMP4AudioStream.h"
#include "Media/NVRMP4FileStreamer.h"
#include "Media/NVRMP4ParserDatatypes.h"
#include "Media/NVRMP4VideoStream.h"
#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaInformation.h"

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
#include "DashProvider/NVRDashMediaSegment.h"
#endif

namespace MP4VR
{
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

typedef FixedArray<MP4VR::DynArray<MP4VR::SegmentInformation>*, SEGMENT_INDEX_COUNT_MAX * 2> SegmentIndexes;

typedef uint32_t InitSegmentId;
typedef Pair<InitSegmentId, MP4SegmentStreamer*> InitSegment;
typedef FixedArray<InitSegment, INIT_SEGMENT_COUNT_MAX> InitSegments;

typedef FixedQueue<MP4SegmentStreamer*, 1000> MediaSegments;
typedef HashMap<InitSegmentId, MediaSegments*> MediaSegmentMap;
#endif

class MP4StreamCreator
{
public:
    virtual MP4MediaStream* createStream(MediaFormat* aFormat) const = 0;
    virtual MP4VideoStream* createVideoStream(MediaFormat* aFormat) const = 0;
    virtual MP4AudioStream* createAudioStream(MediaFormat* aFormat) const = 0;
    virtual void_t destroyStream(MP4MediaStream* aStream) const = 0;
};

class MP4VRParser
{
public:
    MP4VRParser(const MP4StreamCreator& aStreamCreator, ParserContext* ctx);
    virtual ~MP4VRParser();

    /**
     * Opens a given input.
     * @param mediaUri The media URI to open.
     * @return true if successful, false otherwise.
     */
    virtual Error::Enum openInput(const PathName& mediaUri,
                                  MP4AudioStreams& audioStreams,
                                  MP4VideoStreams& videoStreams,
                                  MP4MetadataStreams& aMetadataStreams);

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
    virtual Error::Enum openSegmentedInput(MP4Segment* mp4Segment, bool_t aClientAssignVideoStreamId);
    virtual Error::Enum addSegment(MP4Segment* mp4Segment,
                                   MP4AudioStreams& audioStreams,
                                   MP4VideoStreams& videoStreams,
                                   MP4MetadataStreams& aMetadataStreams);
    virtual Error::Enum addSegmentIndex(MP4Segment* mp4Segment, int32_t aAcceptedMinNumSegment = 1);
    virtual Error::Enum hasSegmentIndexFor(uint32_t segmentId, uint64_t presentationTimeUs);
    virtual Error::Enum getSegmentIndexFor(uint32_t segmentId, uint64_t presentationTimeUs, SubSegment& subSegment);
    virtual Error::Enum getSubSegmentInfo(uint32_t& aSegmentDurationMs,
                                          uint32_t& aSegmentCount,
                                          uint32_t& aTotalDurationMs);
    virtual Error::Enum getSubSegmentForIndex(uint32_t subSegmentId, SubSegment& subSegment);
    virtual Error::Enum removeSegment(uint32_t initSegmentId,
                                      uint32_t segmentId,
                                      MP4AudioStreams& audioStreams,
                                      MP4VideoStreams& videoStreams,
                                      MP4MetadataStreams& aMetadataStreams);
    virtual bool_t getNewestSegmentId(uint32_t initSegmentId, uint32_t& segmentId);
    virtual uint64_t getReadPositionUs(MP4VideoStream* videoStream, uint32_t& segmentIndex);
    virtual uint64_t getReadPositionUs(MP4VideoStreams& videoStreams, uint32_t& segmentIndex);

    uint32_t releaseSegmentsUntil(uint32_t segmentId,
                                  MP4AudioStreams& audioStreams,
                                  MP4VideoStreams& videoStreams,
                                  MP4MetadataStreams& aMetadataStreams);
    void_t releaseUsedSegments(MP4AudioStreams& audioStreams,
                               MP4VideoStreams& videoStreams,
                               MP4MetadataStreams& aMetadataStreams);
    bool_t releaseAllSegments(MP4AudioStreams& audioStreams,
                              MP4VideoStreams& videoStreams,
                              MP4MetadataStreams& aMetadataStreams,
                              bool_t aResetInitialization);
    bool_t hasSegmentIndexForSegmentId(uint32_t segmentId);
#endif
    /**
     * Returns read frame as a MediaPacket.
     */
    virtual Error::Enum readFrame(MP4MediaStream& stream, bool_t& segmentChanged, int64_t currentTimeUs = -1);
    virtual Error::Enum readTimedMetadataFrame(MP4MediaStream& stream, bool_t& aSegmentChanged, int64_t currentTimeUs);

    virtual Error::Enum readInitialMetadata(const PathName& mediaUri,
                                            MP4AudioStreams& audioStreams,
                                            MP4VideoStreams& videoStreams);
    void_t setInitialOverlayMetadata(MP4VideoStream& aStream);

    virtual Error::Enum parseVideoSources(MP4VideoStream& aVideoStream,
                                          SourceType::Enum aSourceType,
                                          BasicSourceInfo& aBasicSourceInfo);
    virtual Error::Enum parseVideoSources(MP4VideoStream& aVideoStream, BasicSourceInfo& aBasicSourceInfo);
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
    virtual bool_t seekToUs(uint64_t& seekPosUs,
                            MP4AudioStreams& audioStreams,
                            MP4VideoStreams& videoStreams,
                            SeekDirection::Enum mode,
                            SeekAccuracy::Enum accuracy);
    /**
     * Seeks to a given position.
     * @param seekFrameNr video frame where to seek to.
     * @return true if successful, false otherwise.
     */
    virtual bool_t seekToFrame(int32_t seekFrameNr,
                               uint64_t& seekPosUs,
                               MP4AudioStreams& audioStreams,
                               MP4VideoStreams& videoStreams);  // , SeekType mode);

    virtual uint64_t getNextVideoTimestampUs(MP4MediaStream& stream);

    virtual int64_t
    seekToSyncFrame(uint32_t sampleId, MP4MediaStream* stream, SeekDirection::Enum mode, bool_t& segmentChanged);

    virtual bool_t isEOS(const MP4AudioStreams& audioStreams, const MP4VideoStreams& videoStreams) const;

    virtual Error::Enum getPropertyOverlayConfig(uint32_t aTrackId,
                                                 uint32_t sampleId,
                                                 MP4VR::OverlayConfigProperty& overlayConfigProperty) const;
    virtual Error::Enum getPropertyGroupsList(MP4VR::GroupsListProperty& aEntityGroups) const;

protected:
    virtual Error::Enum prepareFile(MP4AudioStreams& aAudioStreams,
                                    MP4VideoStreams& aVideoStreams,
                                    MP4MetadataStreams& aMetadataStreams,
                                    uint32_t initSegmentId = 0);
    virtual Error::Enum createStreams(MP4AudioStreams& aAudioStreams,
                                      MP4VideoStreams& aVideoStreams,
                                      MP4MetadataStreams& aMetadataStreams,
                                      FileFormatType::Enum fileFormat);
    virtual void_t configureStream(MP4MediaStream* aStream, MP4VR::TrackInformation* aTrack, bool_t aSetId);
    virtual Error::Enum updateStreams(MP4AudioStreams& aAudioStreams,
                                      MP4VideoStreams& aVideoStreams,
                                      MP4MetadataStreams& aMetadataStreams);

    virtual bool_t readAACAudioMetadata(MP4AudioStream& stream);

private:
    ParserContext* mCtx;
    MemoryAllocator& mAllocator;
    const MP4StreamCreator& mStreamCreator;

    MP4VR::MP4VRFileReaderInterface* mReader;

    // holder for the tracks; accessed via streams who have handle to their corresponding track
    MP4VR::DynArray<MP4VR::TrackInformation>* mTracks;

    MetadataParser mMetaDataParser;

    MP4FileStreamer mFileStream;

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
    InitSegments mInitSegments;
    MediaSegmentMap mMediaSegments;
    SegmentIndexes mSegmentIndexes;
#endif
    Mutex mReaderMutex;
    MediaInformation mMediaInformation;

    bool_t mClientAssignVideoStreamId;

    uint32_t mTimestampBaseMs;
    uint32_t mNewTimestampBaseMs;
    uint32_t mNewTimestampBaseSegmentId;
};
OMAF_NS_END
