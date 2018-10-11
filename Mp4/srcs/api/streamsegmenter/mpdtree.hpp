
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
#ifndef STREAMSEGMENTER_MPDTREE_HPP
#define STREAMSEGMENTER_MPDTREE_HPP

#include <ctime>
#include <set>
#include <stdexcept>
#include <string>
#include "optional.hpp"
#include "rational.hpp"
#include "segmenterapi.hpp"

namespace StreamSegmenter
{
    /**
     * An object tree for creating MDP XML files.
     *
     * Different MPDRoot / AdaptationSet and Representation template classes
     * are only meant to allow writing different implementations for separate
     * isobmf extension specifications. For keeping things straightforward and
     * allowing easily further extend sub specificaitions, the specialization
     * with templates should not be used when implementing for example some flag
     * dependent differences how to write XML inside single specification.
     *
     * In other words for example having different subclasses for
     * OmafAdaptationSetWithProjectionFormat and
     * OmafAdaptationSetWithoutProjectionFormat should not be done.
     */
    namespace MPDTree
    {
        // bool class to make distiction between integers and booleans
        // to be able to print them properly
        struct Bool
        {
            bool val;

            Bool(bool aVal)
                : val(aVal)
            {
            }
        };

        struct Number
        {
            std::uint64_t val;

            Number(std::uint64_t aVal)
                : val(aVal)
            {
            }
        };

        // some parts of MPD allows passing booleans or numbers as parameter
        struct BoolOrNumber
        {
            bool boolValue;
            std::uint64_t numberValue;
            bool isNumber;

            BoolOrNumber()
            {
            }

            BoolOrNumber(const Bool& aVal)
                : boolValue(aVal.val)
                , isNumber(false)
            {
            }

            BoolOrNumber(const Number& aVal)
                : numberValue(aVal.val)
                , isNumber(true)
            {
            }

            void operator=(const Bool& other)
            {
                boolValue = other.val;
                isNumber  = false;
            }

            void operator=(const Number& other)
            {
                numberValue = other.val;
                isNumber    = true;
            }
        };

        struct Duration : RatU64
        {
            using RatU64::RatU64;
        };

        enum class RepresentationType
        {
            Static,
            Dynamic
        };

        enum class VideoScanType
        {
            Progressive,
            Interlaced,
            Unknown
        };

        enum class OmafViewType : uint8_t
        {
            Monoscopic       = 0,
            LeftView         = 1,
            RightView        = 2,
            LeftAndRightView = 3
        };

        enum class OmafQualityType : uint8_t
        {
            AllCorrespondTheSame = 0,
            MayDiffer            = 1
        };

        enum class OmafShapeType : uint8_t
        {
            FourGreatCircles  = 0,
            TwoAzimuthCircles = 1,
        };

        enum class OmafProjectionType : uint8_t
        {
            Equirectangular = 0,
            Cubemap         = 1,
        };

        enum class OmafRwpkPackingType : uint8_t
        {
            Rectangular = 0
        };

        // for signaling printing format of different list syntaxes in MPD
        template <typename T>
        class SpaceSeparatedAttrList : public std::list<T>
        {
            using std::list<T>::list;
        };

        template <typename T>
        class CommaSeparatedAttrList : public std::list<T>
        {
            using std::list<T>::list;
        };

        struct SphereRegion
        {
            std::int32_t centreAzimuth   = 0;
            std::int32_t centreElevation = 0;
            std::int32_t centreTilt      = 0;
            std::uint32_t azimuthRange   = 23592960;
            std::uint32_t elevationRange = 11796480;
        };

        struct URLType
        {
            Utils::Optional<std::string> sourceURL;
            Utils::Optional<std::string> range;

            void writeXML(std::ostream& out, std::uint16_t indentLevel, std::string elementName) const;
        };

        struct PreselectionType
        {
            std::string tag;
            SpaceSeparatedAttrList<std::uint32_t> components;

            void writeXML(std::ostream& out, std::uint16_t indentLevel, bool isSupplementalProperty = false) const;
        };

        // list of key value pairs
        typedef std::list<std::pair<std::string, std::string>> AttributeList;

        struct MPDNode
        {
            // virtual method for writing sub elements of this XML node
            // if there are multiple implementations of the same element
            // this can be called by subclass to write common parts
            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const = 0;

            // virtual method for writing complete element
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const = 0;
        };

        struct OmafProjectionFormat : public MPDNode
        {
            SpaceSeparatedAttrList<OmafProjectionType> projectionType = {OmafProjectionType::Equirectangular};

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // omaf:qualityInfo
        struct OmafQualityInfo : public MPDNode
        {
            std::uint8_t qualityRanking = 0;

            // should be set only if qualityType is QualityType::MayDiffer
            Utils::Optional<std::uint16_t> origWidth;
            Utils::Optional<std::uint16_t> origHeight;

            // should be set only if default view idc was not set
            Utils::Optional<OmafViewType> viewIdc;

            // only written if remainingArea == false
            Utils::Optional<SphereRegion> sphere;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // oamf:sphRegionQuality
        struct OmafSphereRegionWiseQuality : public MPDNode
        {
            // attributes
            OmafShapeType shapeType  = OmafShapeType::FourGreatCircles;
            Bool remainingArea       = false;
            Bool qualityRankingLocal = false;
            // mandatory field without implicit default when element is read
            OmafQualityType qualityType = OmafQualityType::AllCorrespondTheSame;
            Utils::Optional<OmafViewType> defaultViewIdc;

            // child elements
            std::list<OmafQualityInfo> qualityInfos;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct OmafRegionWisePacking : public MPDNode
        {
            SpaceSeparatedAttrList<OmafRwpkPackingType> packingType;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct VideoFramePacking : public MPDNode
        {
            std::uint8_t packingType;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };
        //------------------------------------------------------------------------------
        //
        // Common attributes from Dash spec 5.3.7

        //
        //------------------------------------------------------------------------------
        struct MPDNodeWithCommonAttributes : public MPDNode
        {
            // all optional, because it is not mandatory to define these attributes
            // only in every AdaptationSet, Representation and SubPresentation elements
            Utils::Optional<std::string> profiles;
            Utils::Optional<std::uint32_t> width;
            Utils::Optional<std::uint32_t> height;
            Utils::Optional<RatU64> sar;
            Utils::Optional<FrameRate> frameRate;
            // space separated list of 1 or 2 decimal numbers (rate or rate range)
            SpaceSeparatedAttrList<std::uint32_t> audioSamplingRate;
            Utils::Optional<std::string> mimeType;
            Utils::Optional<std::string> segmentProfiles;
            Utils::Optional<std::string> codecs;
            Utils::Optional<double> maximumSAPPeriod;
            Utils::Optional<std::uint32_t> startWithSAP;  // min: 0 max: 6
            Utils::Optional<double> maxPlayoutRate;
            Utils::Optional<Bool> codingDependency;
            Utils::Optional<VideoScanType> scanType;
            Utils::Optional<std::uint32_t> selectionPriority;
            Utils::Optional<std::string> tag;

            virtual AttributeList getXMLAttributes() const;
        };

        //------------------------------------------------------------------------------
        //
        // Segments
        //
        //------------------------------------------------------------------------------

        // S elements
        struct SegmentTimelineEntry
        {
            SegmentTimelineEntry(std::uint64_t dIn);
            SegmentTimelineEntry(std::uint64_t dIn, std::uint64_t tIn);
            SegmentTimelineEntry(std::uint64_t dIn, std::uint64_t tIn, std::uint64_t nIn);
            SegmentTimelineEntry(std::uint64_t dIn, std::uint64_t tIn, std::uint64_t nIn, std::uint64_t kIn);
            SegmentTimelineEntry(std::uint64_t dIn,
                                 std::uint64_t tIn,
                                 std::uint64_t nIn,
                                 std::uint64_t kIn,
                                 std::int32_t rIn);

            std::uint64_t d;
            Utils::Optional<std::uint64_t> t;
            Utils::Optional<std::uint64_t> n;
            Utils::Optional<std::uint64_t> k;
            Utils::Optional<std::int32_t> r;
        };

        struct SegmentTimeline : public MPDNode
        {
            // list of S elements
            std::list<SegmentTimelineEntry> ss;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // implement support for DASH 5.3.9.2
        struct SegmentBaseInformation : public MPDNode
        {
            // attributes
            Utils::Optional<std::uint32_t> timescale;
            Utils::Optional<std::uint64_t> presentationTimeOffset;
            Utils::Optional<std::uint64_t> presentationDuration;
            Utils::Optional<Duration> timeShiftBufferDepth;
            Utils::Optional<std::string> indexRange;
            Utils::Optional<Bool> indexRangeExact;
            Utils::Optional<double> availabilityTimeOffset;
            Utils::Optional<Bool> availabilityTimeComplete;

            // child elements
            Utils::Optional<URLType> initElement;          /// <Initialization ...>
            Utils::Optional<URLType> representationIndex;  /// <RepresentationIndex ...>

            virtual AttributeList getXMLAttributes() const;
            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // also DASH 5.3.9.2
        struct SegmentBase : public SegmentBaseInformation
        {
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct MultipleSegmentBaseInformation : public SegmentBaseInformation
        {
            // attributes
            Utils::Optional<std::uint32_t> duration;
            Utils::Optional<std::uint32_t> startNumber;

            // child elements
            Utils::Optional<SegmentTimeline> segmentTimeLine;
            Utils::Optional<URLType> bitstreamSwitchingElement;

            virtual AttributeList getXMLAttributes() const;
            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // DASH 5.3.9.3 @todo add implementation for segment lists (might be needed if referring data in CDN with
        // inconsistent naming convention)
        struct SegmentList : public MultipleSegmentBaseInformation
        {
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // DASH 5.3.9.4
        struct SegmentTemplate : public MultipleSegmentBaseInformation
        {
            // attributes
            Utils::Optional<std::string> media;
            Utils::Optional<std::string> index;
            Utils::Optional<std::string> initialization;
            Utils::Optional<std::string> bitstreamSwitching;

            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        //------------------------------------------------------------------------------
        //
        // Representations
        //
        //------------------------------------------------------------------------------

        struct Representation : public MPDNodeWithCommonAttributes
        {
            std::string id          = "default";
            std::uint64_t bandwidth = 0;  // in bits per second.

            Utils::Optional<std::uint32_t> qualityRanking;

            SpaceSeparatedAttrList<std::string> dependencyId;
            SpaceSeparatedAttrList<std::string> associationId;
            SpaceSeparatedAttrList<std::string> associationType;
            SpaceSeparatedAttrList<std::string> mediaStreamStructureId;

            Utils::Optional<SegmentBase> segmentBase;
            Utils::Optional<SegmentList> segmentList;
            Utils::Optional<SegmentTemplate> segmentTemplate;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };


        struct OmafRepresentation : public Representation
        {
            Utils::Optional<OmafRegionWisePacking> regionWisePacking;
            Utils::Optional<OmafProjectionFormat> projectionFormat;

            // At most one SRQR descriptor for each sphRegionQuality@shape_type
            // value of 0 and 1 may be present at representation level.
            std::list<OmafSphereRegionWiseQuality> sphereRegionQualityRanks;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        //------------------------------------------------------------------------------
        //
        // Adaptation sets
        //
        //------------------------------------------------------------------------------

        template <class RepresentationType>
        struct AdaptationSet : public MPDNodeWithCommonAttributes
        {
            // element attributes
            Utils::Optional<std::uint32_t> id;
            Utils::Optional<std::uint32_t> group;

            Utils::Optional<std::string> lang;
            Utils::Optional<std::string> contentType;
            Utils::Optional<RatU64> par;
            Utils::Optional<std::uint32_t> minBandwidth;
            Utils::Optional<std::uint32_t> maxBandwidth;
            Utils::Optional<std::uint32_t> minWidth;
            Utils::Optional<std::uint32_t> maxWidth;
            Utils::Optional<std::uint32_t> minHeight;
            Utils::Optional<std::uint32_t> maxHeight;
            Utils::Optional<FrameRate> minFramerate;
            Utils::Optional<FrameRate> maxFramerate;

            Utils::Optional<BoolOrNumber> segmentAlignment;
            Utils::Optional<Bool> bitstreamSwitching;
            Utils::Optional<Bool> subsegmentAlignment;
            Utils::Optional<std::uint32_t> subsegmentStartsWithSAP;  // sap type 0..6

            std::list<std::string> viewpoints;

            Utils::Optional<PreselectionType> preselection;

            // sub elements
            Utils::Optional<std::string> stereoId;

            // 
            Utils::Optional<VideoFramePacking> videoFramePacking;

            // @note add support for Roles
            // std::list<std::string> roles;

            std::list<RepresentationType> representations;

            virtual AttributeList getXMLAttributes() const;
            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct ISOBMFAdaptationSet : public AdaptationSet<Representation>
        {
        };

        // omaf:coverageInfo
        struct OmafCoverageInfo : MPDNode
        {
            Utils::Optional<OmafViewType> viewIdc;
            SphereRegion region;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // omaf:cc
        struct OmafContentCoverage : public MPDNode
        {
            OmafShapeType shapeType = OmafShapeType::FourGreatCircles;

            Utils::Optional<OmafViewType> defaultViewIdc;

            std::list<OmafCoverageInfo> coverageInfos;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // AdaptationSet with omaf specific elements / attributes
        struct OmafAdaptationSet : public AdaptationSet<OmafRepresentation>
        {
            Utils::Optional<OmafRegionWisePacking> regionWisePacking;
            Utils::Optional<OmafProjectionFormat> projectionFormat;

            // create SupplementalProperty "urn:mpeg:mpegI:omaf:2017:cc" with given coverage infos
            std::list<OmafContentCoverage> contentCoverages;

            // At most one SRQR descriptor for each sphRegionQuality@shape_type
            // value of 0 and 1 may be present at adaptation set level.
            std::list<OmafSphereRegionWiseQuality> sphereRegionQualityRanks;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        //------------------------------------------------------------------------------
        //
        // Period
        //
        //------------------------------------------------------------------------------
        template <class AdaptationSetType>
        struct Period : public MPDNode
        {
            Utils::Optional<std::string> id;
            Utils::Optional<Duration> start;
            Utils::Optional<Duration> duration;

            std::list<AdaptationSetType> adaptationSets;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct ISOBMFPeriod : public Period<ISOBMFAdaptationSet>
        {
            // should be left empty and these parts must be implemented in base class
        };

        // Omaf spec period
        struct OmafPeriod : public Period<OmafAdaptationSet>
        {
            // nothing specific for omaf here (except adaptation set type)
        };

        //------------------------------------------------------------------------------
        //
        // MPD Root nodes
        //
        //------------------------------------------------------------------------------

        // @note using templates to avoid unique pointer lists for adaptation sets / representations etc.
        template <class PeriodType>
        struct MPDRoot : public MPDNode
        {
            RepresentationType type = RepresentationType::Static;
            Duration minBufferTime;

            // mandatory for RepresentationType::Dynamic
            Utils::Optional<std::time_t> availabilityStartTime;

            // mandatory for RepresentationType::Dynamic
            Utils::Optional<std::time_t> publishTime;

            // optional, This attribute shall be present when neither
            // the attribute MPD@minimumUpdatePeriod nor the
            // Period@duration of the last Period are present.
            Utils::Optional<Duration> mediaPresentationDuration;

            std::list<PeriodType> periods;

            // write common parts of MPD element attributelist
            virtual AttributeList getXMLAttributes() const;

            // write inner elements but no headers for subclasses to to write common parts
            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel = 0) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel = 0) const override;
        };

        // ISO/IEC 23009-1
        struct ISOBMFMPDRoot : public MPDRoot<ISOBMFPeriod>
        {
            // should be left empty and these parts must be implemented in base class
        };

        // Omaf spec
        struct OmafMPDRoot : public MPDRoot<OmafPeriod>
        {
            Utils::Optional<OmafRegionWisePacking> regionWisePacking;
            Utils::Optional<OmafProjectionFormat> projectionFormat;

            virtual void writeInnerXML(std::ostream& out, std::uint16_t indentLevel = 0) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel = 0) const override;
        };
    }  // namespace MPDTree
}  // namespace StreamSegmenter

#endif  // STREAMSEGMENTER_MPDTREE_HPP
