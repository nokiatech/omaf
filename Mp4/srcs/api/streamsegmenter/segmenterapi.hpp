
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

#include "frame.hpp"
#include "optional.hpp"
#include "track.hpp"

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
        struct FourCC
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

        struct Segment
        {
            TrackSegment tracks;
            SequenceId sequenceId;
            FrameTime t0;  // begin time of the segment (composition time)
            Duration duration;
        };

        typedef std::list<Segment> Segments;

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

        class Metadata
        {
        public:
            Metadata();
            ~Metadata();

            void setSoftware(const std::string& aSoftware);
            void setVersion(const std::string& aVersion);
            void setOther(const std::string& aKey, const std::string& aValue);

            std::string getSoftware() const;
            std::string getVersion() const;

            std::map<std::string, std::string> getOthers() const;

        private:
            std::string mSoftware;
            std::string mVersion;
            std::map<std::string, std::string> mOthers;
        };

        struct InitSegment
        {
            InitSegment();
            InitSegment(const InitSegment& other);
            InitSegment& operator=(const InitSegment& other);
            InitSegment& operator=(InitSegment&& other);
            ~InitSegment();
            std::unique_ptr<MovieBox> moov;
            std::unique_ptr<FileTypeBox> ftyp;
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

            Utils::Optional<RegionWisePackingRegionGuardBand> guardBand;
        };

        struct RegionWisePacking
        {
            bool constituenPictureMatchingFlag;
            std::uint32_t projPictureWidth;
            std::uint32_t projPictureHeight;
            std::uint16_t packedPictureWidth;
            std::uint16_t packedPictureHeight;

            std::vector<std::unique_ptr<RegionWisePackingRegion>> regions;
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
            std::vector<std::unique_ptr<CoverageInformationRegion>> regions;
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

            // if these are set, also generate restricted info scheme
            Utils::Optional<ProjectionFormat> projectionFormat;
            Utils::Optional<RegionWisePacking> rwpk;
            Utils::Optional<CoverageInformation> covi;
            Utils::Optional<PodvStereoVideoInfo> stvi;
            Utils::Optional<Rotation> rotn;
            std::vector<SchemeType> compatibleSchemes;

        protected:
            void makePovdBoxes(std::unique_ptr<SampleEntryBox>& box) const;
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
            Utils::Optional<Ambisonic> ambisonic;
            Utils::Optional<ChannelLayout> channelLayout;

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
            std::unique_ptr<HandlerBox> makeHandlerBox() const override;

            URIMetadataSampleEntry() = default;
        };

        struct InitialViewingOrientationSampleEntry : public TimedMetadataSampleEntry
        {
            std::unique_ptr<SampleEntryBox> makeSampleEntryBox() const override;
            std::unique_ptr<HandlerBox> makeHandlerBox() const override;

            InitialViewingOrientationSampleEntry() = default;
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
            Utils::Optional<HevcExtractorSampleConstructor> sampleConstructor;
            Utils::Optional<HevcExtractorInlineConstructor> inlineConstructor;

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
            std::map<std::string, std::set<TrackId>> trackReferences;  // map<track reference type, trackids>
            Utils::Optional<std::uint16_t> alternateGroup;
            Utils::Optional<OBSP> obsp;

            // only applicaple to video tracks
            Utils::Optional<GoogleVRType> googleVRType;
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

        struct MovieDescription
        {
            uint64_t creationTime;
            uint64_t modificationTime;
            RatU64 duration;
            std::vector<int32_t> matrix;
            Utils::Optional<BrandSpec> fileType;
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

        void writeSegmentHeader(std::ostream& aOut);
        void writeInitSegment(std::ostream& aOut, const InitSegment& aInitSegment);
        void writeTrackRunWithData(std::ostream& aOut, const Segment& aSegment);
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

        virtual Utils::Optional<SidxInfo> writeSidx(std::ostream& aOutput,
                                                    Utils::Optional<std::ostream::pos_type> aPosition) = 0;

        virtual void setOutput(std::ostream* aOutput) = 0;
    };

    class Writer
    {
    public:
        static Writer* create();
        static void destruct(Writer*);

        /** @brief If you are writing segments in individual files and you're using segment
         * indices, call this before each write.
         *
         * @param aSidxWriter a pointer to a SidxWriter owned by Writer
         * @param aOutput a pointer to an alternative output stream, instead the provided to Writer
         */
        virtual SidxWriter* newSidxWriter(size_t aExpectedSize = 0) = 0;

        virtual void setWriteSegmentHeader(bool aWriteSegmentHeader) = 0;

        virtual void writeInitSegment(std::ostream& aOut, const Segmenter::InitSegment& aInitSegment)       = 0;
        virtual void writeSegment(std::ostream& aOut, const Segmenter::Segment aSegment)                    = 0;
        virtual void writeSubsegments(std::ostream& aOut, const std::list<Segmenter::Segment> aSubsegments) = 0;

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
