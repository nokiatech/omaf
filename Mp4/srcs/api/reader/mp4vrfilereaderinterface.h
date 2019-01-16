
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
#ifndef MP4VRFILEREADERINTERFACE_H
#define MP4VRFILEREADERINTERFACE_H

#include <stdint.h>
#include "mp4vrfileallocator.h"
#include "mp4vrfiledatatypes.h"

namespace MP4VR
{
    class StreamInterface;

    /** @brief Interface for reading an MP4VR file from the file system. */
    class MP4VR_DLL_PUBLIC MP4VRFileReaderInterface
    {
    public:
        /** Make an instance of MP4VRFileReader
         *
         *  If a custom memory allocator has not been set with SetCustomAllocator prior to
         *  calling this, the default allocator will be set into use.
         */
        static MP4VRFileReaderInterface* Create();

        /** Destroy the instance returned by Create */
        static void Destroy(MP4VRFileReaderInterface* mp4vrinterface);

        virtual ~MP4VRFileReaderInterface() = default;

        enum ErrorCode
        {
            OK = 0,
            FILE_OPEN_ERROR,
            FILE_HEADER_ERROR,
            FILE_READ_ERROR,
            UNSUPPORTED_CODE_TYPE,
            INVALID_FUNCTION_PARAMETER,
            INVALID_CONTEXT_ID,
            INVALID_ITEM_ID,
            INVALID_PROPERTY_INDEX,
            INVALID_SAMPLE_DESCRIPTION_INDEX,
            PROTECTED_ITEM,
            UNPROTECTED_ITEM,
            UNINITIALIZED,
            NOT_APPLICABLE,
            MEMORY_TOO_SMALL_BUFFER,
            INVALID_SEGMENT,
            ALLOCATOR_ALREADY_SET
        };

        /** Set an optional custom memory allocator. Call this before calling Create for the
            first time, unless your new allocator is able to release blocks allocated with the
            previous allocator. The allocator is shared by all instances of
            MP4VRFileReaderInterface.

            If you wish to change the allocator after once setting it, you must first set it to
            nullptr, which succeeds always. After this you can change it to the desired value.

            @param [in] customAllocator the allocator to use
            @return ErrorCode: ALLOCATOR_ALREADY_SET if the custom allocator has already been set
                    (possibly automatically upon use); in this case the allocator remains the same
                    as before this call.
        */
        static ErrorCode SetCustomAllocator(CustomAllocator* customAllocator);

    public:  // Interface Methods
        /** Open file for reading and reads the file header information.
         *  @param [in] fileName File to open.
         *  @return ErrorCode: NO_ERROR, FILE_OPEN_ERROR, FILE_READ_ERROR or NOT_APPLICABLE */
        virtual int32_t initialize(const char* fileName) = 0;

        /** Open a stream for reading and reads the file header information.
         *  @param [in] streamInterface Stream to open
         *  @return ErrorCode: NO_ERROR, FILE_OPEN_ERROR, FILE_READ_ERROR or NOT_APPLICABLE */
        virtual int32_t initialize(StreamInterface* streamInterface) = 0;

        /** Close file instance and return to uninitialized state.*/
        virtual void close() = 0;

        /** Get major brand information of the file.
         *  @param [out] majorBrand of the file.
         *  @param [in] initializationSegmentId [optional] In DASH segmented input case what initization segment 'ftyp'
         to get info from.
         *  @param [in] segmentId [optional] In DASH segmented input case if segment specific 'styp' info is needed.
                                             InitializationSegmentId required if segmentId is provided.
         *  @pre initialize() has been called successfully.
         *  @return ErrorCode: NO_ERROR or UNINITIALIZED */
        virtual int32_t getMajorBrand(FourCC& majorBrand,
                                      uint32_t initializationSegmentId = 0,
                                      uint32_t segmentId               = UINT32_MAX) const = 0;

        /** Get minor version information of the file.
         *  @pre initialize() has been called successfully.
         *  @param [out] minorVersion of the file.
         *  @param [in] initializationSegmentId [optional] In DASH segmented input case what initization segment to get
         info from.
         *  @param [in] segmentId [optional] In DASH segmented input case if segment specific 'styp' info is needed.
                                             InitializationSegmentId required if segmentId is provided.
         *  @return ErrorCode: NO_ERROR or UNINITIALIZED */
        virtual int32_t getMinorVersion(uint32_t& minorVersion,
                                        uint32_t initializationSegmentId = 0,
                                        uint32_t segmentId               = UINT32_MAX) const = 0;

        /** Get compatible brands list of the file.
         *  @pre initialize() has been called successfully.
         *  @param [out] compatibleBrands of the file.
         *  @param [in] initializationSegmentId [optional] In DASH segmented input case what initization segment to get
         info from.
         *  @param [in] segmentId [optional] In DASH segmented input case if segment specific 'styp' info is needed.
                                             InitializationSegmentId required if segmentId is provided.
         *  @return ErrorCode: NO_ERROR or UNINITIALIZED */
        virtual int32_t getCompatibleBrands(DynArray<FourCC>& compatibleBrands,
                                            uint32_t initializationSegmentId = 0,
                                            uint32_t segmentId               = UINT32_MAX) const = 0;

        /** Get file information.
         *  This information can be used to further initialize the presentation of the data in the file.
         *  Information also give hints about the way and means to request data from the file.
         *  @pre initialize() has been called successfully.
         *  @param [out] fileinfo FileInformation struct that hold file information.
         *  @param [in] initializationSegmentId [optional] In DASH segmented input case what initization segment to get
         * info from.
         *  @return ErrorCode: NO_ERROR or UNINITIALIZED */
        virtual int32_t getFileInformation(FileInformation& fileinfo, uint32_t initializationSegmentId = 0) const = 0;

        /** Get track information.
         *  These properties can be used to further initialize the presentation of the data in the track.
         *  Properties also give hints about the way and means to request data from the track.
         *
         *  Note! In DASH input case contains information for all parsed initization segments.
         *
         *  @pre initialize() has been called successfully.
         *  @param [out] trackProperties TrackProperties struct that hold track properties information.
         *  @return ErrorCode: NO_ERROR or UNINITIALIZED */
        virtual int32_t getTrackInformations(DynArray<TrackInformation>& trackInfos) const = 0;

        /** Get maximum display width from track headers as an rounded down integer.
         *  @pre getTrackInformations() has been called successfully (to get valid track Id).
         *  @param [in] trackId Track ID of a track.
         *  @param [out] displayWidth Maximum display width in pixels.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getDisplayWidth(uint32_t trackId, uint32_t& displayWidth) const = 0;

        /** Get maximum display height from track headers as an rounded down integer.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in] trackId Track ID of a track.
         *  @param [out] displayHeight Maximum display height in pixels.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getDisplayHeight(uint32_t trackId, uint32_t& displayHeight) const = 0;

        /** Get maximum display width from track headers as a fixed-point 16.16 values.
         *  @pre getTrackInformations() has been called successfully (to get valid track Id).
         *  @param [in] trackId Track ID of a track.
         *  @param [out] displayWidth Maximum display width in pixels.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getDisplayWidthFP(uint32_t trackId, uint32_t& displayWidth) const = 0;

        /** Get maximum display height from track headers a fixed-point 16.16 values.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in] trackId Track ID of a track.
         *  @param [out] displayHeight Maximum display height in pixels.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getDisplayHeightFP(uint32_t trackId, uint32_t& displayHeight) const = 0;

        /** Get width of sample.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in] trackId Track ID of a track.
         *  @param [in] sampleId Sample Id can be any sample id in the track designated by the trackId
         *  @param [out] width Width in pixels.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getWidth(uint32_t trackId, uint32_t sampleId, uint32_t& width) const = 0;

        /** Get height of sample.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in] trackId Track ID of a track.
         *  @param [in] sampleId Sample Id can be any sample in the track designated by the trackId
         *  @param [out] height Height in pixels.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getHeight(uint32_t trackId, uint32_t sampleId, uint32_t& height) const = 0;

        /** Get playback duration of media track in seconds.
         *  This considers also edit lists.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in] trackId Track ID of a track.
         *  @param [out] durationInSecs The playback duration of media track in seconds.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getPlaybackDurationInSecs(uint32_t trackId, double& durationInSecs) const = 0;

        /** Get list of samples in the container with the Track Id  and having the requested sampleType.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId Track ID of a track.
         *  @param [in]  TrackSampleType can be the following:
         *                         out_ref : output reference frames
         *                         out_non_ref : output non-reference frames
         *                         non_out_ref : non-output reference frame
         *                         display : all frame samples in the track which are displayed and in display order
         *                         samples : all samples in the track in track's entry order
         *  @param [out] sampleIds Found samples. The order of the sampleIds are as present in the file.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getTrackSampleListByType(uint32_t trackId,
                                                 TrackSampleType sampleType,
                                                 DynArray<uint32_t>& sampleIds) const = 0;

        /** Get trackSampleBoxType of the sample pointed by {trackId,sampleId} pair
         *  The order of the sampleId are as present in the file
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in] trackId Track ID of a track.
         *  @param [in] sampleId Sample Id.
         *  @param [out] trackSampleBoxType   Media track sample description entry type (box type) is returned.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getTrackSampleType(uint32_t trackId, uint32_t sampleId, FourCC& trackSampleBoxType) const = 0;

        /** Get track sample data for given {trackId, sampleId} pair.
         *  Sample Data does not contain initialization or configuration data (i.e. decoder configuration records),
         *  it is pure sample data - except for samples where bytestream headers are inserted and 'hvc1'/'avc1' type s.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId Track ID of a track.
         *  @param [in]  sampleId Sample Id.
         *  @param [in/out] memoryBuffer   Memory buffer where track  data is to be written to.
         *  @param [in/out] memoryBufferSize   Inputs nemory buffer size. Writes size of the read frame as output.
         *  @param [in] videoByteStreamHeaders   Whether method will insert bytestream header (00 00 00 01) over NAL
         * length info in buffer. In case of ErrorCode::MEMORY_TOO_SMALL_BUFFER contains needed memory buffer size in
         * order for client to realloc memoryBuffer.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_ITEM_ID, UNINITIALIZED, FILE_READ_ERROR,
         * MEMORY_TOO_SMALL_BUFFER or UNSUPPORTED_CODE_TYPE */
        virtual int32_t getTrackSampleData(uint32_t trackId,
                                           uint32_t sampleId,
                                           char* memoryBuffer,
                                           uint32_t& memoryBufferSize,
                                           bool videoByteStreamHeaders = true) = 0;

        /** Get track sample data offset and length for given {trackId, sampleId} pair.
         *  Useful if client wants to read sample data straight from its own buffers instead of using
         * getTrackSampleData() which does memory copy.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId Track ID of a track.
         *  @param [in]  sampleId Sample Id.
         *  @param [out] sampleOffset   Sample data offset in its StreamInterface* or in case of offline file from
         * beginning.
         *  @param [out] sampleLength   Sample data size.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_ITEM_ID or UNINITIALIZED */
        virtual int32_t
        getTrackSampleOffset(uint32_t trackId, uint32_t sampleId, uint64_t& sampleOffset, uint32_t& sampleLength) = 0;

        /** Get decoder configuration record information.
         *  This method can be called by Player implementations that require a separate hardware decoder initialization
         *  before the first frame data is fed (e.g. Android)
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId  Track ID of a track.
         *  @param [in]  sampleId  Sample id that current decoder configuration is asked for.
         *  @param [out] decoderInfos  Decoder configuration record information stored in dynamic array containing
         *                             DecoderSpecificInfo struct inside which decSpecInfoType is DecSpecInfoType enum:
         *                                 AVC_SPS, AVC_PPS, HEVC_VPS, HEVC_SPS, HEVC_PPS or AudioSpecificConfig
         *  @return ErrorCode: NO_ERROR, INVALID__ID (indicates not found for trackId and sampleId) or UNINITIALIZED */
        virtual int32_t getDecoderConfiguration(uint32_t trackId,
                                                uint32_t sampleId,
                                                DynArray<DecoderSpecificInfo>& decoderInfos) const = 0;

        /** Get display timestamp for each  in track.
         *  Timestamps are read from the track sample data.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId  Track ID of a track.
         *  @param [out] timestamps TimestampIDPair struct of <timestamp in milliseconds, sampleId> pair
         *                          Non-output s/frames/samples are not listed here.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getTrackTimestamps(uint32_t trackId, DynArray<TimestampIDPair>& timestamps) const = 0;

        /** Get display timestamps for an sample. An sample may be displayed many times based on the edit list.
         *  Timestamp is read from the track with ID trackIdand sample with ID equal to sampleId.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId  Track ID of a track.
         *  @param [in]  sampleId  Sample Id.
         *  @param [out] timestamps Vector of timestamps in milliseconds. Timestamp truncated integer from float.
         *                          For non-output samples, an empty DynArray.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getTimestampsOfSample(uint32_t trackId,
                                              uint32_t sampleId,
                                              DynArray<uint64_t>& timestamps) const = 0;

        /** Get samples in decoding order.
         *  Timestamp is read from the track with ID trackId and sample with ID equal to sampleId.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId Track ID of a track.
         *  @param [out] sampleDecodingOrder TimestampIDPair struct of <display timestamp in milliseconds, sampleId>
         * pairs. Also complete decoding dependencies are listed here. If an sample ID is present as a decoding
         * dependency for a succeeding frame, its timestamp is set to 0xffffffff.
         *                                 DynArray is sorted by composition time.
         *                                 Timestamps in milliseconds. Timestamp truncated integer from float.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getSamplesInDecodingOrder(uint32_t trackId,
                                                  DynArray<TimestampIDPair>& sampleDecodingOrder) const = 0;

        /** Retrieve sync sample id for given sampleId.
         *  This method should be used to retrieve referenced samples of a sample in a track.
         *  Decoder configuration dependencies are NOT returned by this method as dependent samples.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in] trackId  Track ID of a track.
         *  @param [in] sampleId  Sample Id.
         *  @param [in] direction enum SeekDirection (PREVIOUS or NEXT)
         *  @param [out] syncSampleId   Sample Id of sync sample.
         *                              Can be same as @param [in] sampleId if sample is sync sample and
         * PREVIOUS_SYNCSAMPLE direction is used. Is 0xffffffff if there is no sync samples present or there is no sync
         * samples after current sample id when NEXT_SYNCSAMPLE is used.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t
        getSyncSampleId(uint32_t trackId, uint32_t sampleId, SeekDirection direction, uint32_t& syncSampleId) const = 0;

        /** Get decoder code type for sample
         *  This method can be called by Player implementations that require a separate hardware decoder initialization
         *  before the first frame data is fed.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId     Track ID of a track.
         *  @param [in]  sampleId    Sample Id.
         *  @param [out] decoderCodeType Decoder code type FourCC, e.g. "hvc1" or "avc1"
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getDecoderCodeType(uint32_t trackId, uint32_t sampleId, FourCC& decoderCodeType) const = 0;

        /** Get sample duration.
         *  This method can be to get individual sample durations if getTrackTimestamps() and getTrackSampleListByType()
         * are not suitable.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId     Track ID of a track.
         *  @param [in]  sampleId    Sample Id.
         *  @param [out] sampleDuration Sample duration of sample. In milliseconds. Truncated down to integer from
         * float.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID or UNINITIALIZED */
        virtual int32_t getSampleDuration(uint32_t trackId, uint32_t sampleId, uint32_t& sampleDuration) const = 0;

    public:  // MP4VR specific methods
        /** Get Channel Layout box contents for VR Audio Loudspeaker tracks ('chnl')
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId      Track ID of a track.
         *  @param [in]  sampleId     An sample id.
         *  @param [out] chnlproperty chnlProperty struct containing channellayout box 'chnl' information.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertyChnl(uint32_t trackId, uint32_t sampleId, chnlProperty& chnlproperty) const = 0;

        /** Get Google Spatial Audio Box ('SA3D') information
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId              Track ID of a track.
         *  @param [in]  sampleId             An sample id.
         *  @param [out] spatialaudioproperty SpatialAudioProperty struct containing ambisonics audio information.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertySpatialAudio(uint32_t trackId,
                                                uint32_t sampleId,
                                                SpatialAudioProperty& spatialaudioproperty) const = 0;

        /** Get Google Spatial Media - Spherical Video - Stereoscopic3D information (both V1 and V2 supported).
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId              Track ID of a track.
         *  @param [in]  sampleId             An sample id.
         *  @param [out] stereoscopicproperty StereoScopic3DV2Property struct containing information about stereoscopic
         * rendering in this video track.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertyStereoScopic3D(uint32_t trackId,
                                                  uint32_t sampleId,
                                                  StereoScopic3DProperty& stereoscopicproperty) const = 0;

        /** Get Google Spatial Media - Spherical Video V1 information
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId           Track ID of a track.
         *  @param [in]  sampleId          An sample id.
         *  @param [out] sphericalproperty SphericalVideoV1Property struct containing information about spherical video
         * content contained in this video track.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertySphericalVideoV1(uint32_t trackId,
                                                    uint32_t sampleId,
                                                    SphericalVideoV1Property& sphericalproperty) const = 0;

        /** Get Google Spatial Media - Spherical Video V2 information
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId           Track ID of a track.
         *  @param [in]  sampleId          An sample id.
         *  @param [out] sphericalproperty SphericalVideoV2Property struct containing information about spherical video
         * content contained in this video track.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertySphericalVideoV2(uint32_t trackId,
                                                    uint32_t sampleId,
                                                    SphericalVideoV2Property& sphericalproperty) const = 0;

        /** Get OMAF Region-Wise Packing data
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId      Track ID of a track.
         *  @param [in]  sampleId     An sample id.
         *  @param [out] rwpkProperty RegionWisePackingProperty struct containing information about spherical video
         * content contained in this video track.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertyRegionWisePacking(uint32_t trackId,
                                                     uint32_t sampleId,
                                                     RegionWisePackingProperty& rwpkProperty) const = 0;

        /** Get OMAF Coverage information data
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId      Track ID of a track.
         *  @param [in]  sampleId     An sample id.
         *  @param [out] coviProperty CoverageInformationProperty struct containing information about spherical video
         * content contained in this video track.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertyCoverageInformation(uint32_t trackId,
                                                       uint32_t sampleId,
                                                       CoverageInformationProperty& coviProperty) const = 0;

        /** Get OMAF povd box projection format information
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId      Track ID of a track.
         *  @param [in]  sampleId     An sample id.
         *  @param [out] projectionFormatProperty ProjectionFormatProperty struct containing details about projection
         * format.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertyProjectionFormat(uint32_t trackId,
                                                    uint32_t sampleId,
                                                    ProjectionFormatProperty& projectionFormatProperty) const = 0;

        /** Get OMAF scheme type and compatible scheme type information
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId      Track ID of a track.
         *  @param [in]  sampleId     An sample id.
         *  @param [out] schemeTypesProperty SchemeTypesProperty struct containing information about scheme type and
         * compatible scheme types.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertySchemeTypes(uint32_t trackId,
                                               uint32_t sampleId,
                                               SchemeTypesProperty& schemeTypesProperty) const = 0;

        /** Get OMAF stvi box information, only podv scheme supported.
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId      Track ID of a track.
         *  @param [in]  sampleId     An sample id.
         *  @param [out] stereoVideoProperty Information how stereo video frames are organized.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t
        getPropertyStereoVideoConfiguration(uint32_t trackId,
                                            uint32_t sampleId,
                                            PodvStereoVideoConfiguration& stereoVideoProperty) const = 0;

        /** Get OMAF podv rotation information
         *  @pre getTrackInformations() has been called successfully.
         *  @param [in]  trackId      Track ID of a track.
         *  @param [in]  sampleId     An sample id.
         *  @param [out] rotation     Rotation information of podv sample descriptor.
         *  @return ErrorCode: NO_ERROR, INVALID_CONTEXT_ID, INVALID_SAMPLE_DESCRIPTION_INDEX or UNINITIALIZED */
        virtual int32_t getPropertyRotation(uint32_t trackId, uint32_t sampleId, Rotation& rotationProperty) const = 0;

    public:  // MP4VR segment parsing methods (DASH/Streaming)
        /** Parse Initialization Segment
         *
         *  Note that the init segment id must be globally unique per an instance of
         *  MP4VRFileReaderInterface and must be different from any segment id as well.
         *
         *  @param [in]  streamInterface   StreamInterface*  Interface to read initialization segment from.
         *  @param [in]  initSegmentId     uint32_t Segment Id of the Initialization Segment*
         *  @return ErrorCode: NO_ERROR */
        virtual int32_t parseInitializationSegment(StreamInterface* streamInterface, uint32_t initSegmentId) = 0;

        /** Invalidate Segment
         *  Invalidates the data buffer pointer to given initialization segment id - data from this segment can no
         * longer be read.
         *
         *  Note! Also invalidates all media segments parsed using this initialization id.
         *
         *  @pre Initialization segment with initSegmentId has been parsed with MP4VRFileReaderInterface using
         * parseInitializeSegment.
         *  @param [in]  initSegmentId     uint32_t Segment Id of the Initialization Segment*
         *  @return ErrorCode: NO_ERROR, INVALID_SEGMENT*/
        virtual int32_t invalidateInitializationSegment(uint32_t initSegmentId) = 0;

        /** Parse Segment
         *
         *  Note that the segment id must be globally unique per an instance of
         *  MP4VRFileReaderInterface and must be different from any init segment id as well.
         *
         *  initSegmentId must refer to an initialization segment previously parsed successfully.
         *
         *  @pre At least one Initialize Segment has been parsed before feeding in segment.
         *  @param [in]  streamInterface   StreamInterface*  Interface to read segment from.
         *  @param [in]  initSegmentId   uint32_t Segment Id of the Initialize Segment this segment depends on.
         *  @param [in]  segmentId       uint32_t Segment Id of the segment being fed to method through segmentData
         * pointer
         *  @param [in]  earliestPTSinTS uint64_t Optional - in case of feeding partial segment without 'sidx' box this
         * can be used to give earliest presentation time in timescale for samples.
         *  @return ErrorCode: NO_ERROR */
        virtual int32_t parseSegment(StreamInterface* streamInterface,
                                     uint32_t initSegmentId,
                                     uint32_t segmentId,
                                     uint64_t earliestPTSinTS = UINT64_MAX) = 0;

        /** Invalidate Segment
         *  Invalidates the data buffer pointer to given media segment id - data from this segment can no longer be
         * read.
         *
         *  Note! Must be called for all fed media segments if client is seeking.
         *
         *  @pre Segment with segmentId has been parsed with MP4VRFileReaderInterface using parseSegment.
         *  @param [in]  segmentId     uint32_t Segment Id of the Initialize Segment
         *  @return ErrorCode: NO_ERROR, INVALID_SEGMENT*/
        virtual int32_t invalidateSegment(uint32_t initSegmentId, uint32_t segmentId) = 0;

        /** Get Segment Index
         *  Provides segment index contents for use with DASH ISO Base Media File Format On-Demand profile.
         *
         *  Returned array will contain segmentId for each of the byte ranges, that must be used to feed
         *  those byte ranges through parseInitializationSegment() to parse the file.
         *
         *  @pre Byte range which contains ISO BMFF On-Demand profile initialization byte range has been parsed
         *       with MP4VRFileReaderInterface using parseInitializeSegment.
         *  @param [in]  initSegmentId     uint32_t Segment Id of the Initialization Segment
         *  @param [out] segmentIndex    Array of SegmentInformation struct that hold segment index information.
         *  @return ErrorCode: NO_ERROR, INVALID_SEGMENT*/
        virtual int32_t getSegmentIndex(uint32_t initSegmentId, DynArray<SegmentInformation>& segmentIndex) = 0;

        /** Parse Segment Index ('sidx' box) from given stream.
         *
         *  @param [in]  streamInterface   StreamInterface*  Interface to read segment from.
         *  @param [out] segmentIndex    Array of SegmentInformation struct that hold segment index information.
         *  @return ErrorCode: NO_ERROR, INVALID_SEGMENT*/
        virtual int32_t parseSegmentIndex(StreamInterface* streamInterface,
                                          DynArray<SegmentInformation>& segmentIndex) = 0;
    };
}  // namespace MP4VR

#endif /* MP4VRFILEREADERINTERFACE_H */
