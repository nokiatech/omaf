
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
#ifndef STREAMSEGMENTER_SEGMENTER_HPP
#define STREAMSEGMENTER_SEGMENTER_HPP

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../isobmff/commontypes.h"
#include "frame.hpp"
#include "optional.hpp"
#include "track.hpp"
#include "union.hpp"

// If you have access to the MP4 file, you may be able to use these to more easily construct initialization data
class MovieHeaderBox;

namespace StreamSegmenter
{
    class MP4Access;

}  // namespace StreamSegmenter

namespace StreamSegmenter
{
    namespace Segmenter
    {
        enum WriterMode
        {
            CLASSIC,
            OMAFV2
        };

        using ImdaId = IdBaseWithAdditions<std::uint32_t, class SequenceTag>;

        struct OmafV2Config
        {
            bool writeSegmentHeader = true;
        };

        using WriterConfig = Utils::Union<WriterMode,
                                          Utils::Empty,  // CLASSIC
                                          OmafV2Config   // OMAFV2
                                          >;

        struct FourCC
        {
            char value[5];
            inline FourCC() noexcept
                : value{}
            {
            }
            inline FourCC(uint32_t v) noexcept
            {
                value[0] = char((v >> 24) & 0xff);
                value[1] = char((v >> 16) & 0xff);
                value[2] = char((v >> 8) & 0xff);
                value[3] = char((v >> 0) & 0xff);
                value[4] = '\0';
            }
            inline FourCC(const char* str) noexcept
            {
                value[0] = str[0];
                value[1] = str[1];
                value[2] = str[2];
                value[3] = str[3];
                value[4] = '\0';
            }
            inline FourCC(const FourCC& fourcc) noexcept
            {
                value[0] = fourcc.value[0];
                value[1] = fourcc.value[1];
                value[2] = fourcc.value[2];
                value[3] = fourcc.value[3];
                value[4] = '\0';
            }
            inline FourCC& operator=(const FourCC& other) noexcept
            {
                value[0] = other.value[0];
                value[1] = other.value[1];
                value[2] = other.value[2];
                value[3] = other.value[3];
                value[4] = '\0';
                return *this;
            }
            inline bool operator==(const FourCC& other) const noexcept
            {
                return (value[0] == other.value[0]) && (value[1] == other.value[1]) && (value[2] == other.value[2]) &&
                       (value[3] == other.value[3]);
            }
            inline bool operator!=(const FourCC& other) const noexcept
            {
                return (value[0] != other.value[0]) || (value[1] != other.value[1]) || (value[2] != other.value[2]) ||
                       (value[3] != other.value[3]);
            }
            inline bool operator<(const FourCC& other) const noexcept
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
            inline bool operator<=(const FourCC& other) const noexcept
            {
                return *this == other || *this < other;
            }
            inline bool operator>=(const FourCC& other) const noexcept
            {
                return !(*this < other);
            }
            inline bool operator>(const FourCC& other) const noexcept
            {
                return !(*this <= other);
            }
        };

        enum class ViewIdc : std::uint8_t
        {
            MONOSCOPIC     = 0,
            LEFT           = 1,
            RIGHT          = 2,
            LEFT_AND_RIGHT = 3,
            INVALID        = 0xff
        };

        typedef std::vector<std::unique_ptr<Frame>> SegmentFrames;

        struct SegmentTrackInfo
        {
            FrameTime t0;  // begin time of the track (composition time)
            TrackMeta trackMeta;
            FrameTime dtsCtsOffset;  // offset that should be compensated with an edit list
        };

        // This is used for collecting the data from the track metadata
        struct TrackOfSegment
        {
            SegmentTrackInfo trackInfo;
            Frames frames;
        };

        typedef std::map<TrackId, TrackOfSegment> TrackSegment;

        typedef IdBase<std::uint32_t, struct TrackGroupIdTag> TrackGroupId;

        typedef IdBaseWithAdditions<std::uint32_t, class SequenceTag> SequenceId;

        typedef RatU64 Duration;

        enum class DataReferenceKind
        {
            Imdt,
            Snim
        };

        // applicable to OMAFv2 tracks which use imda references (imdt or snim)
        struct OMAFv2Link
        {
            ImdaId imdaId;  // can be either an imdt reference of sequence number, depending on dataReference
            DataReferenceKind dataReferenceKind;
        };

        struct Segment
        {
            TrackSegment tracks;
            SequenceId sequenceId;
            FrameTime t0;  // begin time of the segment (composition time)
            Duration duration;

            // omafv2-specific info (imdt, snim)
            Utils::Optional<OMAFv2Link> omafv2;
        };

        typedef std::list<Segment> Segments;

        typedef std::vector<TrackId> TrackIds;

        // This is used while actually writing the data down
        struct SegmentMoofInfo
        {
            SegmentTrackInfo trackInfo;
            std::int32_t moofToDataOffset; // applicable to CLASSIC
        };

        typedef std::map<TrackId, SegmentMoofInfo> SegmentMoofInfos;

        struct TrackFrames
        {
            TrackMeta trackMeta;
            Frames frames;

            TrackFrames()
            {
            }

            TrackFrames(TrackMeta aTrackMeta, Frames aFrames)
                : trackMeta(aTrackMeta)
                , frames(aFrames)
            {
                // nothing
            }

            TrackFrames(TrackFrames&& aOther)
                : trackMeta(std::move(aOther.trackMeta))
                , frames(std::move(aOther.frames))
            {
                // nothing
            }

            TrackFrames(const TrackFrames& aOther) = default;
            TrackFrames& operator=(const TrackFrames&) = default;
        };

        typedef std::vector<TrackFrames> FrameAccessors;

        struct FileTypeBox;
        struct MovieBox;
        struct MediaHeaderBox;
        struct HandlerBox;
        struct TrackHeaderBox;
        struct SampleEntryBox;
        struct Region;
        struct MetaBox;
        struct EntityToGroupBox;

        struct InitSegment
        {
            InitSegment();
            InitSegment(const InitSegment& other);
            InitSegment& operator=(const InitSegment& other);
            InitSegment& operator=(InitSegment&& other);
            ~InitSegment();
            std::unique_ptr<MovieBox> moov;
            std::unique_ptr<FileTypeBox> ftyp;
            std::unique_ptr<MetaBox> meta;
        };

        struct MediaDescription
        {
            uint64_t creationTime;
            uint64_t modificationTime;
            RatU64 duration;
        };

        struct SampleEntry
        {
            SampleEntry();
            virtual ~SampleEntry();
            virtual std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const = 0;
            virtual std::unique_ptr<HandlerBox> makeHandlerBox() const         = 0;

            /** Retrieve sample width for the track header box in 16:16 fixed point format */
            virtual uint32_t getWidthFP() const = 0;

            /** Retrieve sample height for the track header box in 16:16 fixed point format */
            virtual uint32_t getHeightFP() const = 0;
        };

        enum class GoogleVRType
        {
            Mono,
            StereoLR,
            StereoTB
        };

        enum class ProjectionFormat
        {
            Equirectangular = 0,
            Cubemap
        };

        struct RegionWisePackingRegionGuardBand
        {
            std::uint8_t leftWidth;
            std::uint8_t rightWidth;
            std::uint8_t topHeight;
            std::uint8_t bottomHeight;

            bool notUsedForPredFlag;

            std::uint8_t type0;
            std::uint8_t type1;
            std::uint8_t type2;
            std::uint8_t type3;
        };

        struct RegionWisePackingRegion
        {
            virtual ~RegionWisePackingRegion()                 = default;
            virtual uint8_t packingType() const                = 0;
            virtual std::unique_ptr<Region> makeRegion() const = 0;
        };

        struct RwpkRectRegion : public RegionWisePackingRegion
        {
            uint8_t packingType() const override;
            std::unique_ptr<Region> makeRegion() const override;

            std::uint32_t projWidth;
            std::uint32_t projHeight;
            std::uint32_t projTop;
            std::uint32_t projLeft;

            std::uint8_t transformType;

            std::uint16_t packedWidth;
            std::uint16_t packedHeight;
            std::uint16_t packedTop;
            std::uint16_t packedLeft;

            ISOBMFF::Optional<RegionWisePackingRegionGuardBand> guardBand;
        };

        struct RegionWisePacking
        {
            bool constituenPictureMatchingFlag;
            std::uint32_t projPictureWidth;
            std::uint32_t projPictureHeight;
            std::uint16_t packedPictureWidth;
            std::uint16_t packedPictureHeight;

            std::vector<std::shared_ptr<const RegionWisePackingRegion>> regions;
        };

        struct CoverageInformationRegion
        {
            ViewIdc viewIdc;  // written only if viewIdcPresenceFlag is set
            std::int32_t centreAzimuth;
            std::int32_t centreElevation;
            std::int32_t centreTilt;
            std::uint32_t azimuthRange;
            std::uint32_t elevationRange;
            bool interpolate;
        };

        enum class CoverageInformationShapeType
        {
            FourGreatCircles = 0,
            TwoAzimuthAndTwoElevationCircles
        };

        struct CoverageInformation
        {
            CoverageInformationShapeType coverageShape;
            bool viewIdcPresenceFlag;  // true ignores defaultViewIdc and writes viewIdc to each region
            ViewIdc defaultViewIdc;
            std::vector<std::shared_ptr<const CoverageInformationRegion>> regions;
        };

        struct SchemeType
        {
            std::string type;
            std::uint32_t version;
            std::string uri;
        };


        // OMAF specs 7.6.1.2 declares that for podv scheme only allowed stereo vidoe scheme_type is 4
        enum class PodvStereoVideoInfo : std::uint8_t
        {
            TopBottomPacking     = 3,
            SideBySidePacking    = 4,
            TemporalInterleaving = 5
        };

        struct Rotation
        {
            std::int32_t yaw;
            std::int32_t pitch;
            std::int32_t roll;
        };

        struct VisualSampleEntry : public SampleEntry
        {
            // Not available from sps/pps
            std::uint16_t width;
            std::uint16_t height;

            uint32_t getWidthFP() const override;
            uint32_t getHeightFP() const override;

            // if projection format is set, rinf and povd boxes will be written
            ISOBMFF::Optional<ProjectionFormat> projectionFormat;
            ISOBMFF::Optional<RegionWisePacking> rwpk;
            ISOBMFF::Optional<CoverageInformation> covi;
            ISOBMFF::Optional<PodvStereoVideoInfo> stvi;
            ISOBMFF::Optional<Rotation> rotn;

            // ovly box is written to povd for omnivideo tracks and to visual sample entry for 2d tracks
            ISOBMFF::Optional<ISOBMFF::OverlayStruct> ovly;

            std::vector<SchemeType> compatibleSchemes;

        protected:
            void makeAdditionalBoxes(std::unique_ptr<SampleEntryBox>& box) const;
        };

        struct AvcVideoSampleEntry : public VisualSampleEntry
        {
            std::vector<uint8_t> sps;
            std::vector<uint8_t> pps;

            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;
            std::unique_ptr<HandlerBox> makeHandlerBox() const override;
        };

        struct HevcVideoSampleEntry : public VisualSampleEntry
        {
            FourCC sampleEntryType = "hvc1";

            float framerate;

            std::vector<uint8_t> sps;
            std::vector<uint8_t> pps;
            std::vector<uint8_t> vps;

            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;
            std::unique_ptr<HandlerBox> makeHandlerBox() const override;
        };

        struct Ambisonic
        {
            std::uint8_t type;
            std::uint32_t order;
            std::uint8_t channelOrdering;
            std::uint8_t normalization;
            std::vector<uint32_t> channelMap;
        };

        struct ChannelPosition
        {
            int speakerPosition = 0;  /// see ISO 23001-8

            /// if speaker_position == 126, then these are set:
            int azimuth   = 0;
            int elevation = 0;

            ChannelPosition() = default;
        };

        struct ChannelLayout
        {
            int streamStructure = 0;  /// a bit field: 1 = stream structured, 2 = object structured

            /// Only if stream_structure & 1:
            int layout = 0;
            // if layout == 0, then:
            std::vector<ChannelPosition> positions;
            // if layout != 0, then
            std::set<int> omitted;

            /// only if stream_structure & 2
            int objectCount = 0;

            ChannelLayout() = default;
        };

        struct MP4AudioSampleEntry : public SampleEntry
        {
            uint16_t sampleSize;
            uint16_t channelCount;
            uint32_t sampleRate;
            uint16_t esId;
            uint16_t dependsOnEsId;
            std::string url;
            uint32_t bufferSize;
            uint32_t maxBitrate;
            uint32_t avgBitrate;
            std::string decSpecificInfo;  // tag 5

            bool nonDiegetic;
            ISOBMFF::Optional<Ambisonic> ambisonic;
            ISOBMFF::Optional<ChannelLayout> channelLayout;

            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;
            std::unique_ptr<HandlerBox> makeHandlerBox() const override;
            uint32_t getWidthFP() const override;
            uint32_t getHeightFP() const override;

            MP4AudioSampleEntry() = default;
        };

        struct TimedMetadataSampleEntry : public SampleEntry
        {
            uint32_t getWidthFP() const override;
            uint32_t getHeightFP() const override;

            std::unique_ptr<HandlerBox> makeHandlerBox() const override;
        };

        struct URIMetadataSampleEntry : public TimedMetadataSampleEntry
        {
            std::string uri;
            std::vector<uint8_t> init;
            enum Version
            {
                Version0,
                Version1
            } version;

            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;

            URIMetadataSampleEntry() = default;
        };

        struct InitialViewingOrientationSampleEntry : public TimedMetadataSampleEntry
        {
            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;

            InitialViewingOrientationSampleEntry() = default;
        };

        struct OverlaySampleEntry : public TimedMetadataSampleEntry
        {
            // also writing overlaystruct modifies some internals of it, so this needs to be mutable
            // to be able to use it in makeSampleEntryBox() method
            mutable ISOBMFF::OverlayStruct overlayStruct;

            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;

            OverlaySampleEntry() = default;
        };

        struct DynamicViewpointSampleEntry : public TimedMetadataSampleEntry
        {
            ISOBMFF::DynamicViewpointSampleEntry dynamicViewpointSampleEntry;

            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;

            DynamicViewpointSampleEntry() = default;
        };

        struct InitialViewpointSampleEntry : public TimedMetadataSampleEntry
        {
            ISOBMFF::InitialViewpointSampleEntry initialViewpointSampleEntry;

            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;

            InitialViewpointSampleEntry() = default;
        };

        struct RecommendedViewportSampleEntry : public TimedMetadataSampleEntry
        {
            ISOBMFF::SphereRegionConfigStruct sphereRegionConfig;
            ISOBMFF::RecommendedViewportInfoStruct recommendedViewportInfo;

            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;

            RecommendedViewportSampleEntry() = default;
        };

        /**
         * HEVC2 Extractor track's SampleConstructor NAL unit
         * ISO/IEC FDIS 14496-15:2014(E) A.7.4.1.1
         */
        struct HevcExtractorSampleConstructor
        {
            std::int8_t trackId;
            std::int8_t sampleOffset;
            std::uint32_t dataOffset;
            std::uint32_t dataLength;

            FrameData toFrameData() const;
        };

        /**
         * HEVC2 Extractor track's InlineConstructor NAL unit
         * ISO/IEC FDIS 14496-15:2014(E) A.7.5.1.1
         */
        struct HevcExtractorInlineConstructor
        {
            std::vector<std::uint8_t> inlineData;

            FrameData toFrameData() const;
        };

        /**
         * Helper class to represent either HEVC2 Extractor track's SampleConstructor or InlineConstructor NAL unit
         * ISO/IEC FDIS 14496-15:2014(E) A.7.2
         */
        struct HevcExtractor
        {
            ISOBMFF::Optional<HevcExtractorSampleConstructor> sampleConstructor;
            ISOBMFF::Optional<HevcExtractorInlineConstructor> inlineConstructor;

            FrameData toFrameData() const;
        };

        /**
         * Helper struct to group multiple HEVC2 Extractor track's NAL units
         * which should be written as single frame to track
         */
        struct HevcExtractorTrackFrameData
        {
            std::uint8_t nuhTemporalIdPlus1;
            std::vector<HevcExtractor> samples;

            FrameData toFrameData() const;
        };

        struct OBSP
        {
            TrackGroupId groupId;
        };

        struct ALTE
        {
            TrackGroupId groupId;
        };

        struct TrackDescription
        {
            TrackDescription();
            TrackDescription(TrackDescription&&);
            TrackDescription(TrackMeta, MediaDescription, const SampleEntry&);
            TrackDescription(TrackMeta,
                             std::list<std::unique_ptr<SampleEntryBox>>,
                             std::unique_ptr<MediaHeaderBox>&&,
                             std::unique_ptr<HandlerBox>&&,
                             std::unique_ptr<TrackHeaderBox>&&);
            ~TrackDescription();

            TrackMeta trackMeta;
            std::list<std::unique_ptr<SampleEntryBox>> sampleEntryBoxes;
            std::unique_ptr<MediaHeaderBox> mediaHeaderBox;
            std::unique_ptr<HandlerBox> handlerBox;
            std::unique_ptr<TrackHeaderBox> trackHeaderBox;
            // could possibly point to track group ids as well..
            std::map<std::string, std::set<TrackId>> trackReferences;  // map<track reference type, trackids>
            ISOBMFF::Optional<std::uint16_t> alternateGroup;

            // only one of these can be set:
            ISOBMFF::Optional<OBSP> obsp;

            // alternate track group
            ISOBMFF::Optional<ALTE> alte;

            // only applicaple to video tracks
            ISOBMFF::Optional<GoogleVRType> googleVRType;

            // omafv2-specific info (imdt, snim)
            Utils::Optional<OMAFv2Link> omafv2;
        };

        typedef std::map<TrackId, TrackDescription> TrackDescriptions;

        /** @brief Given an MP4, construct a description of its tracks that is
         * decoupled from the MP4 itself */
        TrackDescriptions trackDescriptionsOfMp4(const MP4Access& aMp4);

        struct SegmenterConfig
        {
            Duration segmentDuration;
            SequenceId baseSequenceId;
        };

        struct EntityToGroupSpec
        {
            uint32_t groupId;
            std::vector<uint32_t> entityIds;

            ///< fill basebox fields
            void baseFill(EntityToGroupBox& boxToFill) const;
            virtual std::unique_ptr<EntityToGroupBox> makeEntityToGroupBox() const = 0;
            virtual ~EntityToGroupSpec();
        };

        struct AltrEntityGroupSpec : public EntityToGroupSpec
        {
            std::unique_ptr<EntityToGroupBox> makeEntityToGroupBox() const override;
        };

        struct OvbgGroupFlags
        {
            bool overlayFlag;
            bool backgroundFlag;
            ISOBMFF::Optional<std::vector<uint32_t>> overlaySubset;
        };

        struct OvbgEntityGroupSpec : public EntityToGroupSpec
        {
            uint32_t sphereDistanceInMm;
            std::vector<OvbgGroupFlags> entityFlags;

            virtual std::unique_ptr<EntityToGroupBox> makeEntityToGroupBox() const;
        };

        struct OvalEntityGroupSpec : public EntityToGroupSpec
        {
            std::vector<uint16_t> refOverlayIds;

            virtual std::unique_ptr<EntityToGroupBox> makeEntityToGroupBox() const;
        };

        struct VipoEntityGroupSpec : public EntityToGroupSpec
        {
            uint32_t viewpointId;
            std::string viewpointLabel;
            ISOBMFF::ViewpointPosStruct viewpointPos;
            ISOBMFF::ViewpointGroupStruct<true> viewpointGroup;
            ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct viewpointGlobalCoordinateSysRotation;

            ISOBMFF::Optional<ISOBMFF::ViewpointGpsPositionStruct> viewpointGpsPosition;
            ISOBMFF::Optional<ISOBMFF::ViewpointGeomagneticInfoStruct> viewpointGeomagneticInfo;
            ISOBMFF::Optional<ISOBMFF::ViewpointSwitchingListStruct> viewpointSwitchingList;
            ISOBMFF::Optional<ISOBMFF::ViewpointLoopingStruct> viewpointLooping;

            virtual std::unique_ptr<EntityToGroupBox> makeEntityToGroupBox() const;
        };

        struct MetaSpec
        {
            std::vector<AltrEntityGroupSpec> altrGroups;
            std::vector<OvbgEntityGroupSpec> ovbgGroups;
            std::vector<OvalEntityGroupSpec> ovalGroups;
            std::vector<VipoEntityGroupSpec> vipoGroups;
        };

        struct MovieDescription
        {
            uint64_t creationTime;
            uint64_t modificationTime;
            RatU64 duration;
            std::vector<int32_t> matrix;
            ISOBMFF::Optional<BrandSpec> fileType;
            ISOBMFF::Optional<MetaSpec> fileMeta;

            // written if not zero; typically the same as duration; not the duration of an individual fragment :D
            // fragment. ISO 14496-12 2015 8.8.2 "Movie Extends Header Box"
            RatU64 fragmentDuration;
        };

        /** @brief Use this for constructing the init segment per given
         * parameters */
        InitSegment makeInitSegment(const TrackDescriptions& aTrackDescriptions,
                                    const MovieDescription& aMovieDescription,
                                    bool aFragmented);

        /** @brief Construct move description for the use with makeInitSegment
         * from a pre-existing MovieHeaderBox. Usable only if you have access
         * to the MP4VR box parser library. */
        MovieDescription makeMovieDescription(const ::MovieHeaderBox& aMovieHeaderBox);

        extern const WriterConfig& defaultWriterConfig;
        void writeSegmentHeader(std::ostream& aOut, const WriterConfig& aConfig = defaultWriterConfig);
        void writeInitSegment(std::ostream& aOut, const InitSegment& aInitSegment);
        void writeTrackRunWithData(std::ostream& aOut,
                                   const Segment& aSegment,
                                   const WriterConfig& aConfig = defaultWriterConfig);
        void writeMoof(std::ostream& aOut,
                       const TrackIds& aTrackIds,
                       const Segment& aSegment,
                       const SegmentMoofInfos& aSegmentMoofInfos,
                       const std::map<TrackId, Frames>& aFrames,
                       const StreamSegmenter::Segmenter::WriterConfig& aConfig);
        Segments makeSegmentsOfTracks(FrameAccessors&& aFrameAccessors, const SegmenterConfig& aSegmenterConfig);
        Segments makeSegmentsOfMp4(const MP4Access& aMp4, const SegmenterConfig& aSegmenterConfig);

    }  // namespace Segmenter

    struct SidxInfo
    {
        std::ostream::pos_type position;
        size_t size;
    };

    class SidxWriter
    {
    public:
        SidxWriter();
        virtual ~SidxWriter();

        virtual void addSubsegment(Segmenter::Segment aSubsegments) = 0;

        virtual void setFirstSubsegmentOffset(std::streampos aFirstSubsegmentOffset) = 0;

        virtual void addSubsegmentSize(std::streampos aSubsegmentSize) = 0;

        // Update the previous segment to include this size
        // relative=0 -> update the latest
        // relative=1 -> update the one before that, etc
        virtual void updateSubsegmentSize(int relative, std::streampos aSubsegmentSize) = 0;

        virtual ISOBMFF::Optional<SidxInfo> writeSidx(std::ostream& aOutput,
                                                      ISOBMFF::Optional<std::ostream::pos_type> aPosition) = 0;

        virtual void setOutput(std::ostream* aOutput) = 0;
    };

    /** Given a segment index contents with two iterators aBegin and aEnd, patch it so that for first segment its
        size is increased by aSizes[0], for the second aSize[1], etc, as far as there are aSizes around.

        Returns the new index ISOBMFF-serialized segment
     */
    std::vector<char> patchSidxSegmentSizes(const char* aBegin,
                                            const char* aEnd,
                                            const std::vector<uint32_t> aAdjustments,
                                            size_t aReserveTotal);

    struct SidxReference {
        bool          referenceType;
        uint32_t      referencedSize; // to store 31 bit value
        FrameDuration subsegmentDuration;
        bool          startsWithSAP;
        uint8_t       sapType;        // to store 3 bit value
        FrameDuration sapDeltaTime;   // to store 28 bit value
    };

    std::vector<char> generateSidx(uint32_t aReferenceId,
                                   FrameTime aEarliestPresentationTime,
                                   uint64_t aFirstOffset,
                                   std::vector<SidxReference> aReferences,
                                   size_t aReserveTotal);

    enum class WriterFlags: int
    {
        METADATA = 1,
        MEDIADATA = 2,
        SIDX = 4,
        ALL = 7,
        APPEND = 8, // append this data to the previous segment when not writing all at once (for sidx maintenance purposes)
        SKIPSIDX = 16 // writing data that should not go to sidx, e.g. the moof for media?
    };

    class Writer
    {
    public:
        static Writer* create(Segmenter::WriterMode mode = Segmenter::CLASSIC);
        static void destruct(Writer*);

        /** @brief If you are writing segments in individual files and you're using segment
         * indices, call this before each write.
         *
         * @param aSidxWriter a pointer to a SidxWriter owned by Writer
         * @param aOutput a pointer to an alternative output stream, instead the provided to Writer
         */
        virtual SidxWriter* newSidxWriter(size_t aExpectedSize = 0) = 0;

        virtual void setWriteSegmentHeader(bool aWriteSegmentHeader) = 0;

        virtual void writeInitSegment(std::ostream& aOut, const Segmenter::InitSegment& aInitSegment) = 0;
        virtual void writeSegment(std::ostream& aOut,
                                  const Segmenter::Segment aSegment,
                                  WriterFlags flags = WriterFlags::ALL)                              = 0;
        virtual void writeSubsegments(std::ostream& aOut,
                                      const std::list<Segmenter::Segment> aSubsegments,
                                      WriterFlags flags = WriterFlags::ALL)                          = 0;

    protected:
        Writer();
        virtual ~Writer();
    };

    class MovieWriter
    {
    public:
        static MovieWriter* create(std::ostream& aOut);
        static void destruct(MovieWriter*);

        virtual void writeInitSegment(const Segmenter::InitSegment& aInitSegment) = 0;

        virtual void writeSegment(const Segmenter::Segment& aSegment) = 0;

        virtual void finalize() = 0;

    protected:
        MovieWriter();
        virtual ~MovieWriter();
    };

}  // namespace StreamSegmenter

#endif  // STREAMSEGMENTER_SEGMENTER_HPP
