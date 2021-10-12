
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
#ifndef STREAMSEGMENTER_MPDTREE_HPP
#define STREAMSEGMENTER_MPDTREE_HPP

#include <ctime>
#include <set>
#include <stdexcept>
#include <string>
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

        enum class DashProfile
        {
            Live,
            OnDemand,
            TiledLive
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


        /* <xs:simpleType name="Range1">
         *   <xs:restriction base="xs:int">
         *     <xs:minInclusive value="-11796480"/>
         *     <xs:maxInclusive value="11796479"/>
         *   </xs:restriction>
         * </xs:simpleType> */
        using Range1 = std::int32_t;

        /* <xs:simpleType name="Range2">
         *   <xs:restriction base="xs:int">
         *     <xs:minInclusive value="-5898240"/>
         *     <xs:maxInclusive value="5898240"/>
         *   </xs:restriction>
         * </xs:simpleType> */
        using Range2 = std::int32_t;

        /* <xs:simpleType name="HRange">
         *   <xs:restriction base="xs:unsignedInt">
         *     <xs:minInclusive value="0"/>
         *     <xs:maxInclusive value="23592960"/>
         *   </xs:restriction>
         * </xs:simpleType> */
        using HRange = std::uint32_t;

        /* <xs:simpleType name="VRange">
         *   <xs:restriction base="xs:unsignedInt">
         *      <xs:minInclusive value="0"/>
         *      <xs:maxInclusive value="11796480"/>
         *   </xs:restriction>
         * </xs:simpleType> */
        using VRange = std::uint32_t;

        struct SphereRegion
        {
            Range1 centreAzimuth   = 0;
            Range2 centreElevation = 0;
            Range1 centreTilt      = 0;
            HRange azimuthRange    = 23592960;
            VRange elevationRange  = 11796480;
        };

        struct URLType
        {
            ISOBMFF::Optional<std::string> sourceURL;
            ISOBMFF::Optional<std::string> range;

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
            virtual ~MPDNode() = default;

            // Do you need to write something in the upper level before writing the rest of the inner xml? This happens
            // with omaf adaptation sets and property ordering.
            virtual void writeInnerXMLStage1(std::ostream& out, std::uint16_t indentLevel) const;

            // virtual method for writing sub elements of this XML node
            // if there are multiple implementations of the same element
            // this can be called by subclass to write common parts
            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const = 0;

            // virtual method for writing complete element
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const = 0;
        };

        struct OmafProjectionFormat : public MPDNode
        {
            SpaceSeparatedAttrList<OmafProjectionType> projectionType = {OmafProjectionType::Equirectangular};

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // omaf:qualityInfo
        struct OmafQualityInfo : public MPDNode
        {
            std::uint8_t qualityRanking = 0;

            // should be set only if qualityType is QualityType::MayDiffer
            ISOBMFF::Optional<std::uint16_t> origWidth;
            ISOBMFF::Optional<std::uint16_t> origHeight;

            // should be set only if default view idc was not set
            ISOBMFF::Optional<OmafViewType> viewIdc;

            // only written if remainingArea == false
            ISOBMFF::Optional<SphereRegion> sphere;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
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
            ISOBMFF::Optional<OmafViewType> defaultViewIdc;

            // child elements
            std::list<OmafQualityInfo> qualityInfos;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct OmafRegionWisePacking : public MPDNode
        {
            SpaceSeparatedAttrList<OmafRwpkPackingType> packingType;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct VideoFramePacking : public MPDNode
        {
            std::uint8_t packingType;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct BaseURL : public MPDNode
        {
            std::string url;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // Overlays

        struct OverlayVideoInfo
        {
            std::uint8_t overlayId;
            ISOBMFF::Optional<std::uint8_t> overlayPriority;
        };

        struct OverlayVideo : public MPDNode
        {
            bool isSupplementalProperty = true;  // essential when false
            std::list<OverlayVideoInfo> overlayInfo;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct OverlayBackground : public MPDNode
        {
            std::list<std::uint32_t> backgroundIds;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // Viewpoints

        struct Omaf2ViewpointGeomagneticInfo : public MPDNode
        {
            ISOBMFF::Optional<Range1> yaw;
            ISOBMFF::Optional<Range2> pitch;
            ISOBMFF::Optional<Range1> roll;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2ViewpointPosition : public MPDNode
        {
            ISOBMFF::Optional<int> x;
            ISOBMFF::Optional<int> y;
            ISOBMFF::Optional<int> z;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2ViewpointGPSPosition : public MPDNode
        {
            ISOBMFF::Optional<int> longitude;
            ISOBMFF::Optional<int> latitude;
            ISOBMFF::Optional<int> altitude;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2ViewpointGroupInfo : public MPDNode
        {
            uint8_t groupId = 0u;
            ISOBMFF::Optional<std::string> groupDescription;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2ViewpointRelative : public MPDNode
        {
            uint16_t rectLeftPct = 0;
            uint16_t rectTopPct = 0;
            uint16_t rectWidthPct = 100;
            uint16_t rectHeightPct = 100;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2SphereRegion : public MPDNode
        {
            Range1 centreAzimuth   = 0;
            Range2 centreElevation = 0;
            Range1 centreTilt      = 0;
            ISOBMFF::Optional<HRange> azimuthRange;
            ISOBMFF::Optional<VRange> elevationRange;
            uint8_t shapeType = 0;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2OneViewpointSwitchRegion : public MPDNode
        {
            ISOBMFF::Optional<Omaf2ViewpointRelative> vpRelative;
            ISOBMFF::Optional<Omaf2SphereRegion> sphereRegion;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        using RestDatType = uint8_t; /* valid range: [0..2] */

        struct Omaf2ViewpointSwitchRegion : public MPDNode
        {
            std::list<Omaf2OneViewpointSwitchRegion> regions;
            RestDatType regionType;
            ISOBMFF::Optional<std::uint8_t> refOverlayId;
            std::string id;

			/**
             * ViewpointInfo.SwitchingInfo.SwitchRegion@id from which the parent Adaptation Set 
			 * in the Period whose identifier is ViewpointInfo.SwitchingInfo.SwitchRegion@period
			 * shall be selected as to be played back after the current Period
			 */
            ISOBMFF::Optional<std::string> region;
            std::string period;
            ISOBMFF::Optional<std::string> label;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2ViewpointSwitchingInfo : public MPDNode
        {
            std::list<Omaf2ViewpointSwitchRegion> switchRegions;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2ViewpointInfo : public MPDNode
        {
            // Attributes
            ISOBMFF::Optional<std::string> label;
            ISOBMFF::Optional<bool> initialViewpoint;

            // Elements
            Omaf2ViewpointPosition position;
            ISOBMFF::Optional<Omaf2ViewpointGPSPosition> gpsPosition;
            ISOBMFF::Optional<Omaf2ViewpointGeomagneticInfo> geomagneticInfo;
            Omaf2ViewpointGroupInfo groupInfo;
            ISOBMFF::Optional<Omaf2ViewpointSwitchingInfo> switchingInfo;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Omaf2Viewpoint : public MPDNode
        {
            std::string id;
            Omaf2ViewpointInfo viewpointInfo;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
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
            ISOBMFF::Optional<std::string> profiles;
            ISOBMFF::Optional<std::uint32_t> width;
            ISOBMFF::Optional<std::uint32_t> height;
            ISOBMFF::Optional<RatU64> sar;
            ISOBMFF::Optional<FrameRate> frameRate;
            // space separated list of 1 or 2 decimal numbers (rate or rate range)
            SpaceSeparatedAttrList<std::uint32_t> audioSamplingRate;
            ISOBMFF::Optional<std::string> mimeType;
            ISOBMFF::Optional<std::string> segmentProfiles;
            ISOBMFF::Optional<std::string> codecs;
            ISOBMFF::Optional<double> maximumSAPPeriod;
            ISOBMFF::Optional<std::uint32_t> startWithSAP;  // min: 0 max: 6
            ISOBMFF::Optional<double> maxPlayoutRate;
            ISOBMFF::Optional<Bool> codingDependency;
            ISOBMFF::Optional<VideoScanType> scanType;
            ISOBMFF::Optional<std::uint32_t> selectionPriority;
            ISOBMFF::Optional<std::string> tag;

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
            ISOBMFF::Optional<std::uint64_t> t;
            ISOBMFF::Optional<std::uint64_t> n;
            ISOBMFF::Optional<std::uint64_t> k;
            ISOBMFF::Optional<std::int32_t> r;
        };

        struct SegmentTimeline : public MPDNode
        {
            // list of S elements
            std::list<SegmentTimelineEntry> ss;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // implement support for DASH 5.3.9.2
        struct SegmentBaseInformation : public MPDNode
        {
            // attributes
            ISOBMFF::Optional<std::uint32_t> timescale;
            ISOBMFF::Optional<std::uint64_t> presentationTimeOffset;
            ISOBMFF::Optional<std::uint64_t> presentationDuration;
            ISOBMFF::Optional<Duration> timeShiftBufferDepth;
            ISOBMFF::Optional<std::string> indexRange;
            ISOBMFF::Optional<Bool> indexRangeExact;
            ISOBMFF::Optional<double> availabilityTimeOffset;
            ISOBMFF::Optional<Bool> availabilityTimeComplete;

            // child elements
            ISOBMFF::Optional<URLType> initElement;          /// <Initialization ...>
            ISOBMFF::Optional<URLType> representationIndex;  /// <RepresentationIndex ...>

            virtual AttributeList getXMLAttributes() const;
            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // also DASH 5.3.9.2
        struct SegmentBase : public SegmentBaseInformation
        {
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct MultipleSegmentBaseInformation : public SegmentBaseInformation
        {
            // attributes
            ISOBMFF::Optional<std::uint32_t> duration;
            ISOBMFF::Optional<std::uint32_t> startNumber;

            // child elements
            ISOBMFF::Optional<SegmentTimeline> segmentTimeLine;
            ISOBMFF::Optional<URLType> bitstreamSwitchingElement;

            virtual AttributeList getXMLAttributes() const override;
            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // inconsistent naming convention)
        struct SegmentList : public MultipleSegmentBaseInformation
        {
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // DASH 5.3.9.4
        struct SegmentTemplate : public MultipleSegmentBaseInformation
        {
            // attributes
            ISOBMFF::Optional<std::string> media;
            ISOBMFF::Optional<std::string> index;
            ISOBMFF::Optional<std::string> initialization;
            ISOBMFF::Optional<std::string> bitstreamSwitching;

            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        //------------------------------------------------------------------------------
        //
        // Association
        //
        //------------------------------------------------------------------------------

        struct Association : MPDNode
        {
            CommaSeparatedAttrList<std::string> associationKindList;
            std::list<std::string> associations;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        //------------------------------------------------------------------------------
        //
        // Representations
        //
        //------------------------------------------------------------------------------
        struct ContentComponent : public MPDNode
        {
            ISOBMFF::Optional<std::uint64_t> id;
            ISOBMFF::Optional<std::string> contentType;

            // not implemented:
            // <xs:complexType name="ContentComponentType">
            // <xs:sequence>
            //   <xs:element name="Accessibility" type="DescriptorType" minOccurs="0" maxOccurs="unbounded"/>
            //   <xs:element name="Role" type="DescriptorType" minOccurs="0" maxOccurs="unbounded"/>
            //   <xs:element name="Rating" type="DescriptorType" minOccurs="0" maxOccurs="unbounded"/>
            //   <xs:element name="Viewpoint" type="DescriptorType" minOccurs="0" maxOccurs="unbounded"/>
            //   <xs:any namespace="##other" processContents="lax" minOccurs="0" maxOccurs="unbounded"/>
            // </xs:sequence>
            // <xs:attribute name="lang" type="xs:language"/>
            // <xs:attribute name="par" type="RatioType"/>
            // <xs:anyAttribute namespace="##other" processContents="lax"/>

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct Representation : public MPDNodeWithCommonAttributes
        {
            std::string id          = "default";
            std::uint64_t bandwidth = 0;  // in bits per second.

            ISOBMFF::Optional<std::uint32_t> qualityRanking;

            SpaceSeparatedAttrList<std::string> dependencyId;
            SpaceSeparatedAttrList<std::string> associationId;
            SpaceSeparatedAttrList<std::string> associationType;
            SpaceSeparatedAttrList<std::string> mediaStreamStructureId;
            ISOBMFF::Optional<std::string> allMediaAssociationViewpoint;

            ISOBMFF::Optional<SegmentBase> segmentBase;
            ISOBMFF::Optional<SegmentList> segmentList;
            ISOBMFF::Optional<SegmentTemplate> segmentTemplate;
            ISOBMFF::Optional<BaseURL> baseURL;
            std::list<ContentComponent> contentComponent;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };


        struct OmafRepresentation : public Representation
        {
            ISOBMFF::Optional<OmafRegionWisePacking> regionWisePacking;
            ISOBMFF::Optional<OmafProjectionFormat> projectionFormat;

            // At most one SRQR descriptor for each sphRegionQuality@shape_type
            // value of 0 and 1 may be present at representation level.
            std::list<OmafSphereRegionWiseQuality> sphereRegionQualityRanks;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
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
            ISOBMFF::Optional<std::uint32_t> id;
            ISOBMFF::Optional<std::uint32_t> group;

            ISOBMFF::Optional<std::string> lang;
            ISOBMFF::Optional<std::string> contentType;
            ISOBMFF::Optional<RatU64> par;
            ISOBMFF::Optional<std::uint32_t> minBandwidth;
            ISOBMFF::Optional<std::uint32_t> maxBandwidth;
            ISOBMFF::Optional<std::uint32_t> minWidth;
            ISOBMFF::Optional<std::uint32_t> maxWidth;
            ISOBMFF::Optional<std::uint32_t> minHeight;
            ISOBMFF::Optional<std::uint32_t> maxHeight;
            ISOBMFF::Optional<FrameRate> minFramerate;
            ISOBMFF::Optional<FrameRate> maxFramerate;

            ISOBMFF::Optional<BoolOrNumber> segmentAlignment;
            ISOBMFF::Optional<Bool> bitstreamSwitching;
            ISOBMFF::Optional<Bool> subsegmentAlignment;
            ISOBMFF::Optional<std::uint32_t> subsegmentStartsWithSAP;  // sap type 0..6

            std::list<std::string> viewpoints;

            std::list<PreselectionType> preselection;

            // sub elements
            ISOBMFF::Optional<std::string> stereoId;

            //
            ISOBMFF::Optional<VideoFramePacking> videoFramePacking;

            ISOBMFF::Optional<OverlayVideo> overlayVideo;
            ISOBMFF::Optional<OverlayBackground> overlayBackground;

            ISOBMFF::Optional<Omaf2Viewpoint> viewpoint;

            // @note add support for Roles
            // std::list<std::string> roles;

            std::list<RepresentationType> representations;

            virtual AttributeList getXMLAttributes() const override;

            // avoid difficult order coding
            virtual void writeInnerXMLStage1(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct ISOBMFAdaptationSet : public AdaptationSet<Representation>
        {
        };

        // omaf:coverageInfo
        struct OmafCoverageInfo : MPDNode
        {
            ISOBMFF::Optional<OmafViewType> viewIdc;
            SphereRegion region;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // omaf:cc
        struct OmafContentCoverage : public MPDNode
        {
            OmafShapeType shapeType = OmafShapeType::FourGreatCircles;

            ISOBMFF::Optional<OmafViewType> defaultViewIdc;

            std::list<OmafCoverageInfo> coverageInfos;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        // AdaptationSet with omaf specific elements / attributes
        struct OmafAdaptationSet : public AdaptationSet<OmafRepresentation>
        {
            ISOBMFF::Optional<OmafRegionWisePacking> regionWisePacking;
            ISOBMFF::Optional<OmafProjectionFormat> projectionFormat;

            // create SupplementalProperty "urn:mpeg:mpegI:omaf:2017:cc" with given coverage infos
            std::list<OmafContentCoverage> contentCoverages;

            // At most one SRQR descriptor for each sphRegionQuality@shape_type
            // value of 0 and 1 may be present at adaptation set level.
            std::list<OmafSphereRegionWiseQuality> sphereRegionQualityRanks;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        //------------------------------------------------------------------------------
        //
        // EntityGroup
        //
        //------------------------------------------------------------------------------
        struct EntityId: public MPDNode
        {
            std::uint32_t adaptationSetId;
            std::string representationId;

            void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        struct EntityGroup: public MPDNode
        {
            std::string groupType; // 4cc
            uint32_t groupId;
            std::list<EntityId> entities;
            SpaceSeparatedAttrList<std::uint16_t> refOverlayId;
            AttributeList attributes; // other attributes, specific to the particular type

            void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
            void writeXML(std::ostream& out, std::uint16_t indentLevel) const override;
        };

        //------------------------------------------------------------------------------
        //
        // Period
        //
        //------------------------------------------------------------------------------
        template <class AdaptationSetType>
        struct Period : public MPDNode
        {
            ISOBMFF::Optional<std::string> id;
            ISOBMFF::Optional<Duration> start;
            ISOBMFF::Optional<Duration> duration;

            std::list<AdaptationSetType> adaptationSets;
            std::list<EntityGroup> entityGroups;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel) const override;
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
            ISOBMFF::Optional<std::time_t> availabilityStartTime;

            // mandatory for RepresentationType::Dynamic
            ISOBMFF::Optional<std::time_t> publishTime;

            // optional, This attribute shall be present when neither
            // the attribute MPD@minimumUpdatePeriod nor the
            // Period@duration of the last Period are present.
            ISOBMFF::Optional<Duration> mediaPresentationDuration;

            std::list<PeriodType> periods;

            DashProfile profile = DashProfile::Live;

            // write common parts of MPD element attributelist
            virtual AttributeList getXMLAttributes() const;

            // write inner elements but no headers for subclasses to to write common parts
            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel = 0) const override;
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
            ISOBMFF::Optional<OmafRegionWisePacking> regionWisePacking;
            ISOBMFF::Optional<OmafProjectionFormat> projectionFormat;

            virtual void writeInnerXMLStage2(std::ostream& out, std::uint16_t indentLevel = 0) const override;
            virtual void writeXML(std::ostream& out, std::uint16_t indentLevel = 0) const override;
        };
    }  // namespace MPDTree
}  // namespace StreamSegmenter

#endif  // STREAMSEGMENTER_MPDTREE_HPP
