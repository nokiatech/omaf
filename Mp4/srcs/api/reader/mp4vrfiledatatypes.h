
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
#ifndef MP4VRFILEDATATYPES_H
#define MP4VRFILEDATATYPES_H

#include <stddef.h>
#include <stdint.h>
#include "mp4vrfileexport.h"

#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif


namespace MP4VR
{
    struct MP4VR_DLL_PUBLIC FourCC
    {
        char value[5];
        inline FourCC()
            : value{}
        {
        }
        inline FourCC(uint32_t v)
        {
            value[0] = char((v >> 24) & 0xff);
            value[1] = char((v >> 16) & 0xff);
            value[2] = char((v >> 8) & 0xff);
            value[3] = char((v >> 0) & 0xff);
            value[4] = '\0';
        }
        inline FourCC(const char* str)
        {
            value[0] = str[0];
            value[1] = str[1];
            value[2] = str[2];
            value[3] = str[3];
            value[4] = '\0';
        }
        inline FourCC(const FourCC& fourcc)
        {
            value[0] = fourcc.value[0];
            value[1] = fourcc.value[1];
            value[2] = fourcc.value[2];
            value[3] = fourcc.value[3];
            value[4] = '\0';
        }
        inline FourCC& operator=(const FourCC& other)
        {
            value[0] = other.value[0];
            value[1] = other.value[1];
            value[2] = other.value[2];
            value[3] = other.value[3];
            value[4] = '\0';
            return *this;
        }
        inline bool operator==(const FourCC& other) const
        {
            return (value[0] == other.value[0]) && (value[1] == other.value[1]) && (value[2] == other.value[2]) &&
                   (value[3] == other.value[3]);
        }
        inline bool operator!=(const FourCC& other) const
        {
            return (value[0] != other.value[0]) || (value[1] != other.value[1]) || (value[2] != other.value[2]) ||
                   (value[3] != other.value[3]);
        }
        inline bool operator<(const FourCC& other) const
        {
            return (value[0] < other.value[0])
                       ? true
                       : (value[0] > other.value[0])
                             ? false
                             : (value[1] < other.value[1])
                                   ? true
                                   : (value[1] > other.value[1])
                                         ? false
                                         : (value[2] < other.value[2])
                                               ? true
                                               : (value[2] > other.value[2])
                                                     ? false
                                                     : (value[3] < other.value[3])
                                                           ? true
                                                           : (value[3] > other.value[3]) ? false : false;
        }
        inline bool operator<=(const FourCC& other) const
        {
            return *this == other || *this < other;
        }
        inline bool operator>=(const FourCC& other) const
        {
            return !(*this < other);
        }
        inline bool operator>(const FourCC& other) const
        {
            return !(*this <= other);
        }
    };

    template <typename T>
    struct MP4VR_DLL_PUBLIC DynArray
    {
        typedef T value_type;
        typedef T* iterator;
        typedef const T* const_iterator;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        size_t size;
        T* elements;
        DynArray();
        DynArray(size_t n);
        DynArray(const DynArray& other);
        DynArray& operator=(const DynArray& other);
        inline T& operator[](size_t index)
        {
            return elements[index];
        }
        inline const T& operator[](size_t index) const
        {
            return elements[index];
        }
        inline T* begin()
        {
            return elements;
        }
        inline T* end()
        {
            return elements + size;
        }
        inline const T* begin() const
        {
            return elements;
        }
        inline const T* end() const
        {
            return elements + size;
        }
        ~DynArray();
    };

    enum TrackSampleType
    {
        out_ref,      // output reference frames
        out_non_ref,  // output non-reference frames
        non_out_ref,  // non-output reference frame
        display,      // all frame samples in the track which are displayed and in display order
        samples,      // all samples in the track in track's entry order
    };

    enum class ViewIdc : uint8_t
    {
        MONOSCOPIC     = 0,
        LEFT           = 1,
        RIGHT          = 2,
        LEFT_AND_RIGHT = 3,
        INVALID        = 0xff
    };

    struct MP4VR_DLL_PUBLIC vrvCProperty
    {
        uint8_t version_major;
        uint8_t version_minor;
        DynArray<uint16_t> sourceIdList;
    };

    struct MP4VR_DLL_PUBLIC DepthProperty
    {
        uint8_t version_major;
        uint8_t version_minor;
        DynArray<uint16_t> sourceIdList;
    };

    struct MP4VR_DLL_PUBLIC ChannelLayout
    {
        uint8_t speakerPosition;  // is an OutputChannelPosition from ISO/IEC 23001-8
        int16_t azimuth;          // is a signed value in degrees, as defined for LoudspeakerAzimuth in ISO/IEC 23001-8
        int8_t elevation;  // is a signed value, in degrees, as defined for LoudspeakerElevation in ISO/IEC 23001-8
    };

    struct MP4VR_DLL_PUBLIC chnlProperty
    {
        uint8_t streamStructure;  // is a field of flags that define whether the stream has channel (1) or object (2) or
                                  // both or neither
        uint8_t definedLayout;    // is a ChannelConfiguration from ISO/IEC 23001-8;
        uint64_t omittedChannelsMap;  // in case of definedLayout!=0: is present. A bit-map of omitted channels - as
                                      // documented in ISO/IEC 23001-8 ChannelConfiguration
        uint8_t objectCount;          // in case of streamStructure = object (2) the count of objects
        uint16_t channelCount;        // comes from the sample entry
        DynArray<ChannelLayout> channelLayouts;  //  in case of streamStructure = channel (1) the count of channels
    };

    //### OMAF ProjectionFormatBox related types

    enum class RegionWisePackingType : uint8_t
    {
        RECTANGULAR = 0
    };

    struct MP4VR_DLL_PUBLIC RectRegionPacking
    {
        uint32_t projRegWidth;
        uint32_t projRegHeight;
        uint32_t projRegTop;
        uint32_t projRegLeft;
        uint8_t transformType;
        uint16_t packedRegWidth;
        uint16_t packedRegHeight;
        uint16_t packedRegTop;
        uint16_t packedRegLeft;

        uint8_t leftGbWidth;
        uint8_t rightGbWidth;
        uint8_t topGbHeight;
        uint8_t bottomGbHeight;
        bool gbNotUsedForPredFlag;
        uint8_t gbType0;
        uint8_t gbType1;
        uint8_t gbType2;
        uint8_t gbType3;
    };

    struct MP4VR_DLL_PUBLIC RegionWisePackingRegion
    {
        bool guardBandFlag;
        RegionWisePackingType packingType;
        union Region {
            RectRegionPacking rectangular;
        } region;
    };

    struct MP4VR_DLL_PUBLIC RegionWisePackingProperty
    {
        bool constituentPictureMatchingFlag = false;
        uint32_t projPictureWidth;
        uint32_t projPictureHeight;
        uint16_t packedPictureWidth;
        uint16_t packedPictureHeight;
        DynArray<RegionWisePackingRegion> regions;
    };

    enum class CoverageShapeType : uint8_t
    {
        FOUR_GREAT_CIRCLES = 0,
        TWO_AZIMUTH_AND_TWO_ELEVATION_CIRCLES
    };

    struct MP4VR_DLL_PUBLIC CoverageSphereRegion
    {
        ViewIdc viewIdc;  // only valid if CoverageInformationProperty.viewIdcPresenceFlag is set
        int32_t centreAzimuth;
        int32_t centreElevation;
        int32_t centreTilt;
        uint32_t azimuthRange;
        uint32_t elevationRange;
        bool interpolate;
    };

    struct MP4VR_DLL_PUBLIC CoverageInformationProperty
    {
        CoverageShapeType coverageShapeType;
        bool viewIdcPresenceFlag;
        ViewIdc defaultViewIdc;
        DynArray<CoverageSphereRegion> sphereRegions;
    };

    //### Google Spatial Media datatypes start here ###
    // details https://github.com/google/spatial-media/blob/master/docs/spatial-audio-rfc.md
    struct MP4VR_DLL_PUBLIC SpatialAudioProperty
    {
        uint8_t version;          // specifies the version of this box
        uint8_t ambisonicType;    // specifies the type of ambisonic audio represented
        uint32_t ambisonicOrder;  // specifies the order of the ambisonic sound field
        uint8_t
            ambisonicChannelOrdering;  // specifies the channel ordering (i.e., spherical harmonics component ordering)
        uint8_t ambisonicNormalization;  // specifies the normalization (i.e., spherical harmonics normalization)
        DynArray<uint32_t> channelMap;  // maps audio channels in a given audio track to ambisonic component. Array size
                                        // specifies the number of audio channels.
    };

    // Common for both V1 and V2, details
    // https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.md
    enum class StereoScopic3DProperty : uint8_t
    {
        MONOSCOPIC                 = 0,
        STEREOSCOPIC_TOP_BOTTOM    = 1,
        STEREOSCOPIC_LEFT_RIGHT    = 2,
        STEREOSCOPIC_STEREO_CUSTOM = 3
    };

    struct PoseDegrees1616FP
    {                     // 16.16 fixed point values measuring rotation in degrees.
        int32_t yawFP;    // counter-clockwise rotation in degrees around the up vector, restricted to -180.0 to 180.0
        int32_t pitchFP;  // counter-clockwise rotation in degrees around the right vector post yaw transform,
                          // restricted to -90.0 to 90.0
        int32_t rollFP;   // clockwise-rotation in degrees around the forward vector post yaw and pitch transform,
                          // restricted to -180.0 to 180.0
    };

    struct CubemapProjection
    {
        uint32_t layout;   // layout of cube faces. The values 0 to 255 are reserved for current and future layouts. See
                           // link above for details of layout.
        uint32_t padding;  // number of pixels to pad from the edge of each cube face.
    };

    struct EquirectangularProjection
    {  // projection bounds use 0.32 fixed point values.
        uint32_t boundsTopFP;
        uint32_t boundsBottomFP;
        uint32_t boundsLeftFP;
        uint32_t boundsRightFP;
    };

    enum class ProjectionType : uint8_t
    {
        UNKOWN          = 0,
        CUBEMAP         = 1,
        EQUIRECTANGULAR = 2,
        MESH            = 3
    };

    // details https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.md
    struct SphericalVideoV2Property
    {
        PoseDegrees1616FP pose;
        ProjectionType projectionType;
        union Projection {
            CubemapProjection cubemap;
            EquirectangularProjection equirectangular;
        } projection;
    };

    // details https://github.com/google/spatial-media/blob/master/docs/spherical-video-rfc.md
    struct SphericalVideoV1Property
    {
        bool spherical;                 // v1.0 always true
        bool stitched;                  // v1.0 always true
        ProjectionType projectionType;  // V1.0 always EQUIRECTANGULAR
        uint32_t sourceCount;
        // StereoMode available through StereoScopic3DProperty if present
        PoseDegrees1616FP initialView;
        uint64_t timestamp;
        uint32_t fullPanoWidthPixels;
        uint32_t fullPanoHeightPixels;
        uint32_t croppedAreaImageWidthPixels;
        uint32_t croppedAreaImageHeightPixels;
        uint32_t croppedAreaLeftPixels;
        uint32_t croppedAreaTopPixels;
    };

    //### Google Spatial Media datatypes end here ###

    enum SeekDirection
    {
        PREVIOUS = 0,
        NEXT     = 1
    };

    enum DecSpecInfoType
    {
        AVC_SPS  = 7,
        AVC_PPS  = 8,
        HEVC_VPS = 32,
        HEVC_SPS = 33,
        HEVC_PPS = 34,
        AudioSpecificConfig
    };

    struct MP4VR_DLL_PUBLIC DecoderSpecificInfo
    {
        DecSpecInfoType decSpecInfoType;  // can be "SPS" and "PPS" for AVC, additional "VPS" for HEVC and "ASC" for
                                          // AudioSpecificConfig
        DynArray<uint8_t> decSpecInfoData;
    };

    struct MP4VR_DLL_PUBLIC TimestampIDPair
    {
        uint64_t timeStamp;
        uint32_t itemId;
    };

    namespace FileFeatureEnum
    {
        enum Feature
        {
            HasAlternateTracks = 1u << 3  // has a alternate tracks.
        };
    }

    typedef uint32_t FeatureBitMask;

    struct MP4VR_DLL_PUBLIC FileInformation
    {
        FeatureBitMask features;  // bitmask of FileFeatureEnum's
    };

    namespace TrackFeatureEnum
    {
        enum Feature
        {
            IsVideoTrack            = 1u,       ///< The track is an Video track.
            IsAudioTrack            = 1u << 1,  ///< The track is an Audio track.
            IsMetadataTrack         = 1u << 2,  ///< The track is an Metadata track.
            HasAlternatives         = 1u << 3,  ///< The track has alternative track or tracks
            HasSampleGroups         = 1u << 4,  ///< The track has SampleToGroupBoxes
            HasAssociatedDepthTrack = 1u << 5   ///< The video track has associated Video Depth track ('vdep' tref)
        };

        enum VRFeature  // these are additional VR related features on top of Feature's of the track above.
        {
            IsAudioLSpeakerChnlStructTrack = 1u << 2,  ///< The track is a Loudspeaker channel track with ChannelLayout
                                                       ///< 'chnl' box which is channelStructured
            IsVRGoogleSpatialAudioTrack =
                1u
                << 8,  ///< Google Spatial Audio Box (SA3D)(Ambisonics)
                       ///< https://github.com/google/spatial-media/blob/master/docs/spatial-audio-rfc.md#spatial-audio-rfc-draft
            IsVRGoogleNonDiegeticAudioTrack = 1u << 9,  ///< Google Non-Diegetic Audio Box (SAND)
            HasVRGoogleStereoscopic3D =
                1u << 12,  ///< The track has Google Spatial Media / Spherical Video V1/V2 Stereoscopic 3D Video
            HasVRGoogleV1SpericalVideo =
                1u << 13,  ///< The track has Google Spatial Media / Spherical Video V1 Spherical Video
            HasVRGoogleV2SpericalVideo =
                1u << 14,  ///< The track has Google Spatial Media / Spherical Video V2 Spherical Video
        };
    }  // namespace TrackFeatureEnum

    struct MP4VR_DLL_PUBLIC TypeToTrackIDs
    {
        FourCC type;
        DynArray<uint32_t> trackIds;
    };

    // Sample Flags Field as defined in 8.8.3.1 of ISO/IEC 14496-12:2015(E)
    struct SampleFlagsType
    {
        uint32_t reserved : 4, is_leading : 2, sample_depends_on : 2, sample_is_depended_on : 2,
            sample_has_redundancy : 2, sample_padding_value : 3, sample_is_non_sync_sample : 1,
            sample_degradation_priority : 16;
    };

    union SampleFlags {
        uint32_t flagsAsUInt;
        SampleFlagsType flags;
    };

    /** @brief SampleType enumeration to indicate the type of the frame. */
    enum SampleType
    {
        OUTPUT_NON_REFERENCE_FRAME,
        OUTPUT_REFERENCE_FRAME,
        NON_OUTPUT_REFERENCE_FRAME
    };

    struct MP4VR_DLL_PUBLIC SampleInformation
    {
        uint32_t sampleId;                ///< based on the sample's entry order in the sample table
        FourCC sampleEntryType;           ///< coming from SampleDescriptionBox (codingname)
        uint32_t sampleDescriptionIndex;  ///< coming from SampleDescriptionBox index (sample_description_index)
        SampleType sampleType;            ///< coming from sample groupings
        uint32_t initSegmentId;           ///< Initialization Segment ID of the sample.
        uint32_t segmentId;  ///< Segment ID of the sample. Can be used to help client to know segment boundaries.
        uint64_t earliestTimestamp;    ///< earliest timestamps of the sample
        SampleFlags sampleFlags;       ///< Sample Flags Field as defined in 8.8.3.1 of ISO/IEC 14496-12:2015(E)
        uint64_t sampleDurationTS;     ///< Sample duration in time scale units
        uint64_t earliestTimestampTS;  ///< earliest timestamps of the sample in time scale units
    };

    struct MP4VR_DLL_PUBLIC Rational
    {
        uint64_t num;
        uint64_t den;
    };

    struct MP4VR_DLL_PUBLIC TrackTypeInformation
    {
        FourCC majorBrand;      ///< Major brand of track
        uint32_t minorVersion;  ///< Minor version
        DynArray<FourCC> compatibleBrands;
    };

    struct MP4VR_DLL_PUBLIC TrackInformation
    {
        uint32_t initSegmentId;  ///< init segment id for this track information / 0 for local files.
        uint32_t trackId;
        uint32_t alternateGroupId;
        FeatureBitMask features;                       ///< bitmask of TrackFeatureEnum::Feature
        FeatureBitMask vrFeatures;                     ///< bitmask of TrackFeatureEnum::VRFeature
        DynArray<char> trackURI;                       ///< track URI for timed metadata tracks.
        DynArray<uint32_t> alternateTrackIds;          ///< other track' IDs with same alternateGroupId.
        DynArray<TypeToTrackIDs> referenceTrackIds;    ///< <reference_type, reference track IDs>
        DynArray<TypeToTrackIDs> trackGroupIds;        ///< <group_type, group track IDs>
        DynArray<SampleInformation> sampleProperties;  ///< SampleInformation for each of the samples inside the track.
        uint32_t maxSampleSize;   ///< Size of largest sample inside the track (can be used to allocate client side read
                                  ///< buffer).
        uint32_t timeScale;       ///< Time scale of the track; useful for video stream procsesing purposes
        Rational frameRate;       ///< Frames per second
        bool hasTypeInformation;  ///< Signals if optional fields, majorBrand, minorVerion and compatibleBrands are used
        TrackTypeInformation type;  ///< Track's brand, version and compatible brands
    };

    struct MP4VR_DLL_PUBLIC SegmentInformation
    {
        uint32_t segmentId;        ///< segmentId for this DASH ISOBMFF On-Demand profile file byte range
        uint32_t referenceId;      ///< referenceId provides the stream ID for the reference stream
        uint32_t timescale;        ///< the timescale, in ticks per second, for the earliestPTS and duration fields
        bool referenceType;        ///< referenceType: 1=other sidx box, 0=media content
        uint64_t earliestPTSinTS;  ///< earliest PTS in timescale
        uint32_t durationInTS;     ///< byte range duration in milliseconds
        uint64_t startDataOffset;  ///< distance in bytes, in the file containing media, to start offset of segment byte
                                   ///< range
        uint32_t dataSize;         ///< distance in bytes from the startDataOffset to the end of the segment
        bool startsWithSAP;        ///< indicates whether the segment start with a Stream Access Point (SAP)
        uint8_t SAPType;           ///< SAP type as specified in 8.16.3.3 of ISO/IEC 14496-12:2015(E)
    };

    struct MP4VR_DLL_PUBLIC SchemeType
    {
        FourCC type;         ///< Scheme type
        uint32_t version;    ///< Scheme version
        DynArray<char> uri;  ///< Optional URI
    };

    struct MP4VR_DLL_PUBLIC SchemeTypesProperty
    {
        SchemeType mainScheme;
        DynArray<SchemeType> compatibleSchemeTypes;
    };

    enum OmafProjectionType
    {
        EQUIRECTANGULAR = 0,
        CUBEMAP
    };

    struct MP4VR_DLL_PUBLIC ProjectionFormatProperty
    {
        OmafProjectionType format;
    };

    enum PodvStereoVideoConfiguration
    {
        TOP_BOTTOM_PACKING    = 3,
        SIDE_BY_SIDE_PACKING  = 4,
        TEMPORAL_INTERLEAVING = 5,
        MONOSCOPIC            = 0x8f  ///< special value for indicating that stvi box was not found
    };

    struct MP4VR_DLL_PUBLIC Rotation
    {
        int32_t yaw;
        int32_t pitch;
        int32_t roll;
    };

    struct MP4VR_DLL_PUBLIC SphereRegionProperty
    {
        int32_t centreAzimuth;
        int32_t centreElevation;
        int32_t centreTilt;
        uint32_t azimuthRange;    // not always used
        uint32_t elevationRange;  // not always used
        bool interpolate;
    };

    struct MP4VR_DLL_PUBLIC InitialViewingOrientationSample
    {
        SphereRegionProperty region = {};
        bool refreshFlag;

        InitialViewingOrientationSample() = default;
        InitialViewingOrientationSample(char* frameData, uint32_t frameLen);
    };

}  // namespace MP4VR

#endif /* MP4VRFILEDATATYPES_H */
