
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
#ifndef COMMONTYPES_HPP
#define COMMONTYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "dynarray.h"
#include "mp4vrfileexport.h"
#include "optional.h"
#include "union.h"

#define ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(NAME) \
    NAME();                                     \
    NAME(const NAME&);                          \
    NAME(NAME&&);                               \
    NAME& operator=(const NAME&);               \
    NAME& operator=(NAME&&);                    \
    ~NAME();

namespace ISOBMFF
{
    class BitStream;

    enum class ViewIdcType : std::uint8_t
    {
        MONOSCOPIC     = 0,
        LEFT           = 1,
        RIGHT          = 2,
        LEFT_AND_RIGHT = 3,
        INVALID        = 0xff
    };

    enum class SphereRegionShapeType : std::uint8_t
    {
        FourGreatCircles                 = 0,
        TwoAzimuthAndTwoElevationCircles = 1
    };

    struct MP4VR_DLL_PUBLIC Rotation
    {
        std::int32_t yaw;
        std::int32_t pitch;
        std::int32_t roll;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };

    struct MP4VR_DLL_PUBLIC SphereRegionRange
    {
        std::uint32_t azimuthRange;
        std::uint32_t elevationRange;

        // name chosen as not to collide with SphereRegionStatic
        void writeOpt(BitStream& bitstr) const;
        void parseOpt(BitStream& bitstr);
        std::uint16_t sizeOpt() const;
    };

    struct MP4VR_DLL_PUBLIC SphereRegion : public SphereRegionRange
    {
        std::int32_t centreAzimuth;
        std::int32_t centreElevation;
        std::int32_t centreTilt;
        bool interpolate;

        void write(BitStream& bitstr, bool hasRange) const;
        void parse(BitStream& bitstr, bool hasRange);
    };

    template <bool SelectThis, typename T>
    struct MP4VR_DLL_PUBLIC SelectIf
    {
        void writeOpt(BitStream&) const
        {
        }
        void parseOpt(BitStream&)
        {
        }
        std::uint16_t sizeOpt() const
        {
            return 0u;
        }
    };

    template <typename T>
    struct MP4VR_DLL_PUBLIC SelectIf<true, T> : public T
    {
    };

    struct MP4VR_DLL_PUBLIC SphereRegionInterpolate
    {
        bool interpolate;

        // name chosen as not to collide with SphereRegionStatic
        void writeOpt(BitStream& bitstr) const;
        void parseOpt(BitStream& bitstr);
        std::uint16_t sizeOpt() const;
    };

    template <bool HasRange, bool HasInterpolate>
    struct MP4VR_DLL_PUBLIC SphereRegionStatic : SelectIf<HasRange, SphereRegionRange>,
                                                 SelectIf<HasInterpolate, SphereRegionInterpolate>
    {
        std::int32_t centreAzimuth;
        std::int32_t centreElevation;
        std::int32_t centreTilt;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        std::uint16_t size() const;
    };

    struct MP4VR_DLL_PUBLIC SphereRegionConfigStruct
    {
        SphereRegionShapeType shapeType;
        Optional<SphereRegionRange> staticRange;
    };

    struct MP4VR_DLL_PUBLIC SphereRegionContext
    {
        bool hasInterpolate {};
        bool hasRange {};

        SphereRegionContext();
        SphereRegionContext(const SphereRegionConfigStruct& aConfig);
    };

    struct MP4VR_DLL_PUBLIC SphereRegionDynamic : SphereRegionStatic<true, true>
    {
        void write(BitStream& bitstr) const = delete;
        void parse(BitStream& bitstr) = delete;

        void write(BitStream& bitstr, const SphereRegionContext& aSampleContext) const;
        void parse(BitStream& bitstr, const SphereRegionContext& aSampleContext);

        std::uint16_t size() const = delete;
        std::uint16_t size(const SphereRegionContext& aSampleContext) const;
    };

    /**
     * Overlay related types
     */

    struct MP4VR_DLL_PUBLIC PackedPictureRegion
    {
        std::uint16_t pictureWidth;
        std::uint16_t pictureHeight;
        std::uint16_t regionWidth;
        std::uint16_t regionHeight;
        std::uint16_t regionTop;
        std::uint16_t regionLeft;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };

    struct MP4VR_DLL_PUBLIC ProjectedPictureRegion
    {
        std::uint32_t pictureWidth;
        std::uint32_t pictureHeight;
        std::uint32_t regionWidth;
        std::uint32_t regionHeight;
        std::uint32_t regionTop;
        std::uint32_t regionLeft;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
        std::uint16_t size() const;
    };

    enum class TransformType
    {
        NONE                            = 0,
        MIRROR_HORIZONTAL               = 1,
        ROTATE_CCW_180                  = 2,
        ROTATE_CCW_180_BEFORE_MIRRORING = 3,
        ROTATE_CCW_90_BEFORE_MIRRORING  = 4,
        ROTATE_CCW_90                   = 5,
        ROTATE_CCW_270_BEFORE_MIRRORING = 6,
        ROTATE_CCW_270                  = 7
    };

    struct MP4VR_DLL_PUBLIC OverlayControlFlagBase
    {
        bool doesExist                   = false;
        bool overlayControlEssentialFlag = false;

        // if inheritFromOverlayConfigSampleEntry is set, then size is written as zero and properties
        // from OverlayConfigSampleEntry are used
        bool inheritFromOverlayConfigSampleEntry = false;

        virtual ~OverlayControlFlagBase();

        /**
         * Returns size of control flag struct in case if inheritFromOverlayConfigSampleEntry is not set
         */
        virtual std::uint16_t size() const = 0;

        /**
         * In case if size is zero, overlay strcuture properties should be read from
         * OverlayConfigSampleEntry stored in VisualSample
         *
         * @return true if writing section was complete
         */
        bool write(BitStream& bitstr) const;

        /**
         * @return parsedSize or 0 in case if nothing is left to parse
         */
        std::uint16_t parse(BitStream& bitstr);
    };

    /**
     * HC = horizontally align center
     * HL = horizontally align left
     * HR = horizontally align right
     * VC = vertically align center
     * VT = vertically align top
     * VB = vertically align bottom
     *
     * STRETCH_TO_FILL = stretch source media without maintaining aspect
     *                   ratio to fill the whole overlay area
     *
     * SCALE = scale source media to fit inside overlay
     *         area maintaining aspect ratio
     *
     * CROP = scale and crop source so that it maintains
     *        aspect ratio and fills the overlay area completely
     */
    enum class MediaAlignmentType
    {
        STRETCH_TO_FILL = 0,

        HC_VC_SCALE = 1,
        HC_VT_SCALE = 2,
        HC_VB_SCALE = 3,
        HL_VC_SCALE = 4,
        HR_VC_SCALE = 5,
        HL_VT_SCALE = 6,
        HR_VT_SCALE = 7,
        HL_VB_SCALE = 8,
        HR_VB_SCALE = 9,

        HC_VC_CROP = 10,
        HC_VT_CROP = 11,
        HC_VB_CROP = 12,
        HL_VC_CROP = 13,
        HR_VC_CROP = 14,
        HL_VT_CROP = 15,
        HR_VT_CROP = 16,
        HL_VB_CROP = 17,
        HR_VB_CROP = 18
    };

    struct MP4VR_DLL_PUBLIC ViewportRelativeOverlay : OverlayControlFlagBase
    {
        std::uint16_t rectLeftPercent;
        std::uint16_t rectTopPercent;
        std::uint16_t rectWidthtPercent;
        std::uint16_t rectHeightPercent;

        MediaAlignmentType mediaAlignment;
        bool relativeDisparityFlag;

        // disparity in pixels or in percent, in case of percent
        // signed 16 bit value is normalized to range -100..100
        std::int16_t disparity;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewportRelativeOverlay);
    };

    enum class RegionIndicationType
    {
        PROJECTED_PICTURE = 0,
        SPHERE            = 1
    };

    template class MP4VR_DLL_PUBLIC Union<RegionIndicationType,
                                          /* PROJECTED_PICTURE */
                                          ProjectedPictureRegion,
                                          /* SPHERE */
                                          SphereRegionStatic<
                                              /* HasRange */ true,
                                              /* HasInterpolate */ true>>;

    using RegionType = Union<RegionIndicationType,
                             /* PROJECTED_PICTURE */
                             ProjectedPictureRegion,
                             /* SPHERE */
                             SphereRegionStatic<
                                 /* HasRange */ true,
                                 /* HasInterpolate */ true>>;

    struct MP4VR_DLL_PUBLIC SphereRelativeOmniOverlay : OverlayControlFlagBase
    {
        RegionType region;
        bool timelineChangeFlag;

        std::uint16_t regionDepthMinus1;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(SphereRelativeOmniOverlay);
    };

    struct MP4VR_DLL_PUBLIC SphereRelative2DOverlay : OverlayControlFlagBase
    {
        SphereRegionStatic</* HasRange */ true, /* HasInterpolate */ true> sphereRegion;
        bool timelineChangeFlag;

        Rotation overlayRotation;

        std::uint16_t regionDepthMinus1;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        SphereRelative2DOverlay();
        SphereRelative2DOverlay(const SphereRelative2DOverlay&);
        SphereRelative2DOverlay(SphereRelative2DOverlay&&);
        SphereRelative2DOverlay& operator=(const SphereRelative2DOverlay&);
        SphereRelative2DOverlay& operator=(SphereRelative2DOverlay&&);
        ~SphereRelative2DOverlay();
    };

    struct MP4VR_DLL_PUBLIC OverlaySourceRegion : OverlayControlFlagBase
    {
        PackedPictureRegion region;
        TransformType transformType;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OverlaySourceRegion);
    };

    struct MP4VR_DLL_PUBLIC RecommendedViewportOverlay : OverlayControlFlagBase
    {
        // actually empty and the class is a container for OverlayControlFlagBase

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(RecommendedViewportOverlay);
    };

    struct MP4VR_DLL_PUBLIC OverlayLayeringOrder : OverlayControlFlagBase
    {
        std::int16_t layeringOrder;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OverlayLayeringOrder);
    };

    struct MP4VR_DLL_PUBLIC OverlayOpacity : OverlayControlFlagBase
    {
        std::uint8_t opacity;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OverlayOpacity);
    };

    struct MP4VR_DLL_PUBLIC OverlayInteraction : OverlayControlFlagBase
    {
        bool changePositionFlag;
        bool changeDepthFlag;
        bool switchOnOffFlag;
        bool changeOpacityFlag;
        bool resizeFlag;
        bool rotationFlag;
        bool sourceSwitchingFlag;
        bool cropFlag;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OverlayInteraction);
    };

    struct MP4VR_DLL_PUBLIC OverlayLabel : OverlayControlFlagBase
    {
        std::string overlayLabel;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OverlayLabel);
    };

    struct MP4VR_DLL_PUBLIC OverlayPriority : OverlayControlFlagBase
    {
        std::uint8_t overlayPriority;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OverlayPriority);
    };

    struct MP4VR_DLL_PUBLIC AssociatedSphereRegion : OverlayControlFlagBase
    {
        SphereRegionShapeType shapeType;
        SphereRegionStatic</* HasRange */ true, /* HasInterpolate */ true> sphereRegion;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(AssociatedSphereRegion);
    };

    enum class AlphaBlendingModeType : std::uint8_t
    {
        SourceOver = 0  // vu = m * alpha + vi * (1 - alpha)
    };

    struct MP4VR_DLL_PUBLIC OverlayAlphaCompositing : OverlayControlFlagBase
    {
        AlphaBlendingModeType alphaBlendingMode;

        std::uint16_t size() const;
        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OverlayAlphaCompositing);
    };

    struct MP4VR_DLL_PUBLIC SingleOverlayStruct
    {
        std::uint16_t overlayId;

        // all allowed control flags
        // .doesExist == true is set for controls that are written / read from file
        // (UniquePtr didn't work easily inside vector and common module doesn't
        // have Utils::Optional, so had to write custom solution)
        ViewportRelativeOverlay viewportRelativeOverlay;
        SphereRelativeOmniOverlay sphereRelativeOmniOverlay;
        SphereRelative2DOverlay sphereRelative2DOverlay;
        OverlaySourceRegion overlaySourceRegion;
        RecommendedViewportOverlay recommendedViewportOverlay;
        OverlayLayeringOrder overlayLayeringOrder;
        OverlayOpacity overlayOpacity;
        OverlayInteraction overlayInteraction;
        OverlayLabel overlayLabel;
        OverlayPriority overlayPriority;
        AssociatedSphereRegion associatedSphereRegion;
        OverlayAlphaCompositing overlayAlphaCompositing;

        void write(BitStream& bitstr, std::uint8_t flagByteCount) const;
        void parse(BitStream& bitstr, std::uint8_t flagByteCount);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(SingleOverlayStruct);
    };

    /**
     * This struct can exist in 4 places
     *
     * 1. Inside povd box of a video track
     * 2. Inside dyol metadata sample entry
     * 3. Inside visual media sample entry
     * 4. As inline data inside metadata sample
     *
     * Each layer can override data from earlier one partially or completely.
     */
    struct MP4VR_DLL_PUBLIC OverlayStruct
    {
        std::uint8_t numFlagBytes;
        std::vector<SingleOverlayStruct> overlays;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OverlayStruct);
    };

    // Reference material: OMAFv2-WD6-Draft01-SM.pdf
    //
    // aligned(8) ViewpointPosStruct()
    // {
    //     signed int(32) viewpoint_pos_x;
    //     signed int(32) viewpoint_pos_y;
    //     signed int(32) viewpoint_pos_z;
    // }

    struct MP4VR_DLL_PUBLIC ViewpointPosStruct
    {
        std::int32_t viewpointPosX;
        std::int32_t viewpointPosY;
        std::int32_t viewpointPosZ;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointPosStruct);
    };

    // aligned(8) class ViewpointGpsPositionStruct()
    // {
    //     signed int(32) viewpoint_gpspos_longitude;
    //     signed int(32) viewpoint_gpspos_latitude;
    //     signed int(32) viewpoint_gpspos_altitude;
    // }

    struct MP4VR_DLL_PUBLIC ViewpointGpsPositionStruct
    {
        std::int32_t viewpointGpsposLongitude;
        std::int32_t viewpointGpsposLatitude;
        std::int32_t viewpointGpsposAltitude;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointGpsPositionStruct);
    };

    // aligned(8) class ViewpointGeomagneticInfoStruct()
    // {
    //     signed int(32) viewpoint_geomagnetic_yaw;
    //     signed int(32) viewpoint_geomagnetic_pitch;
    //     signed int(32) viewpoint_geomagnetic_roll;
    // }

    struct MP4VR_DLL_PUBLIC ViewpointGeomagneticInfoStruct
    {
        std::int32_t viewpointGeomagneticYaw;
        std::int32_t viewpointGeomagneticPitch;
        std::int32_t viewpointGeomagneticRoll;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointGeomagneticInfoStruct);
    };

    // aligned(8) class ViewpointGlobalCoordinateSysRotationStruct()
    // {
    //     signed int(32) viewpoint_gcs_yaw;
    //     signed int(32) viewpoint_gcs_pitch;
    //     signed int(32) viewpoint_gcs_roll;
    // }

    struct MP4VR_DLL_PUBLIC ViewpointGlobalCoordinateSysRotationStruct
    {
        std::int32_t viewpointGcsYaw;
        std::int32_t viewpointGcsPitch;
        std::int32_t viewpointGcsRoll;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointGlobalCoordinateSysRotationStruct);
    };

    // aligned(8) class ViewpointGroupStruct()
    // {
    //     unsigned int(8) vwpt_group_id;
    //     utf8string vwpt_group_description;
    // }

    struct MP4VR_DLL_PUBLIC ViewpointGroupDescription
    {
         DynArray<char> vwptGroupDescription;

         void writeOpt(BitStream& bitstr) const;
         void parseOpt(BitStream& bitstr);
         std::uint16_t sizeOpt() const;
    };

    template <bool GroupDescrIncludedFlag>
    struct MP4VR_DLL_PUBLIC ViewpointGroupStruct:
        SelectIf<GroupDescrIncludedFlag, ViewpointGroupDescription>
    {
        std::uint8_t vwptGroupId;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        //ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointGroupStruct<GroupDescrIncludedFlag>);
    };

    // aligned(8) class ViewpointTimelineSwitchStruct()
    // {
    //     unsigned int(1) absolute_relative_t_offset_flag;
    //     unsigned int(1) min_time_flag;
    //     unsigned int(1) max_time_flag;
    //     bits(4) reserved = 0;
    //     // time window for activation of the switching
    //     if (min_time_flag)
    //         signed int(32) t_min;
    //     if (max_time_flag)
    //         signed int(32) t_max;
    //     if (absolute_relative_t_offset_flag == 0)
    //         unsigned int(32) absolute_t_offset;
    //     else
    //         unsigned int(32) relative_t_offset;
    // }

    template class MP4VR_DLL_PUBLIC Optional<std::int32_t>;
    template class MP4VR_DLL_PUBLIC Optional<std::uint32_t>;
    template class MP4VR_DLL_PUBLIC Optional<std::string>;

    // named thus as not to conflict with #defines ABSOLUTE and RELATIVE from somewhere on the Windows VRSDK build
    enum class OffsetKind
    {
        ABSOLUTE_TIME = 0,
        RELATIVE_TIME = 1
    };

    template class MP4VR_DLL_PUBLIC Union<OffsetKind, std::uint32_t, std::int32_t>;

    struct MP4VR_DLL_PUBLIC ViewpointTimelineSwitchStruct
    {
        /* unsigned int(1) absolute_relative_t_offset_flag; */
        /* unsigned int(1) min_time_flag; */
        /* unsigned int(1) max_time_flag; */
        // time window for activation of the switching
        // if (min_time_flag)
        Optional<std::int32_t> minTime;
        // if (max_time_flag)
        Optional<std::int32_t> maxTime;
        // if (absolute_relative_t_offset_flag == 0) .. else ..
        Union<OffsetKind, std::uint32_t, std::int64_t> tOffset;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointTimelineSwitchStruct);
    };

    // aligned(8) class ViewpointSwitchingListStruct()
    // {
    //     unsigned int(8) num_viewpoint_switching;
    //     for (i = 0; i < num_viewpoint_switching; i++)
    //     {
    //         unsigned int(16) destination_viewpoint_id;
    //         unsigned int(1) destination_viewport_flag;
    //         unsigned int(1) transition_effect_flag;
    //         unsigned int(1) timeline_switching_offset_flag;
    //         bit(5) reserved = 0;
    //         // which viewport to switch to in the destination viewpoint
    //         if (destination_viewport_flag)
    //             SphereRegionStruct(0, 0);
    //         // definition of the destination viewport as a sphere region
    //         if (timeline_switching_offset_flag)
    //             ViewpointTimelineSwitchStruct();
    //         if (transition_effect_flag)
    //         {
    //             unsigned int(8) transition_effect_type;
    //             if (transition_effect_type == 4)
    //                 unsigned int(32) transition_video_track_id;
    //             if (transition_effect_type == 5)
    //                 utf8string transition_video_URL;
    //         }
    //     }
    // }

    /*     unsigned int(16) destination_viewpoint_id; */
    /*     unsigned int(1) destination_viewport_flag; */
    /*     unsigned int(1) transition_effect_flag; */
    /*     unsigned int(1) timeline_switching_offset_flag; */
    /*     bit(5) reserved = 0; */
    /*     // which viewport to switch to in the destination viewpoint */
    /*     if (destination_viewport_flag) */
    /*         SphereRegionStruct(0, 0); */
    /*     // definition of the destination viewport as a sphere region */
    /*     if (timeline_switching_offset_flag) */
    /*         ViewpointTimelineSwitchStruct(); */
    /*     if (transition_effect_flag) */
    /*     { */
    /*         unsigned int(8) transition_effect_type; */
    /*         if (transition_effect_type == 4) */
    /*             unsigned int(32) transition_video_track_id; */
    /*         if (transition_effect_type == 5) */
    /*             utf8string transition_video_URL; */
    /*     } */
    enum class TransitionEffectType
    {
        ZOOM_IN_EFFECT,             // 0
        WALK_THOUGH_EFFECT,         // 1
        FADE_TO_BLACK_EFFECT,       // 2
        MIRROR_EFFECT,              // 3
        VIDEO_TRANSITION_TRACK_ID,  // 4
        VIDEO_TRANSITION_URL,       // 5
        RESERVED                    // 6..
    };

    template class MP4VR_DLL_PUBLIC Union<TransitionEffectType,
                                          Empty,           // ZOOM_IN_EFFECT
                                          Empty,           // WALK_THOUGH_EFFECT
                                          Empty,           // FADE_TO_BLACK_EFFECT
                                          Empty,           // MIRROR_EFFECT
                                          std::uint32_t,   // VIDEO_TRANSITION_TRACK_ID
                                          DynArray<char>,  // VIDEO_TRANSITION_URL
                                          Empty>;          // RESERVED

    struct MP4VR_DLL_PUBLIC TransitionEffect
    {
        Union<TransitionEffectType,
              Empty,           // ZOOM_IN_EFFECT
              Empty,           // WALK_THOUGH_EFFECT
              Empty,           // FADE_TO_BLACK_EFFECT
              Empty,           // MIRROR_EFFECT
              std::uint32_t,   // VIDEO_TRANSITION_TRACK_ID
              DynArray<char>,  // VIDEO_TRANSITION_URL
              Empty>           // RESERVED
            transitionEffect;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(TransitionEffect);
    };

    template class MP4VR_DLL_PUBLIC Optional<SphereRegionStatic<true, false>>;
    template class MP4VR_DLL_PUBLIC Optional<ViewpointTimelineSwitchStruct>;
    template class MP4VR_DLL_PUBLIC Optional<TransitionEffect>;
    template class MP4VR_DLL_PUBLIC Optional<ViewpointGroupStruct<false>>;
    template class MP4VR_DLL_PUBLIC Optional<ViewpointGroupStruct<true>>;
    template class MP4VR_DLL_PUBLIC Optional<ViewpointPosStruct>;
    template class MP4VR_DLL_PUBLIC Optional<ViewpointGlobalCoordinateSysRotationStruct>;
    template class MP4VR_DLL_PUBLIC Optional<ViewpointGpsPositionStruct>;
    template class MP4VR_DLL_PUBLIC Optional<ViewpointGeomagneticInfoStruct>;

    enum class ViewpointRegionType
    {
        VIEWPORT_RELATIVE,  // viewport relative position
        SPHERE_RELATIVE,    // sphere relative position
        OVERLAY             // overlay
    };

    struct MP4VR_DLL_PUBLIC ViewportRelative
    {
        std::uint16_t rectLeftPercent;
        std::uint16_t rectTopPercent;
        std::uint16_t rectWidthtPercent;
        std::uint16_t rectHeightPercent;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewportRelative);
    };

    template class MP4VR_DLL_PUBLIC Optional<std::int8_t>;
    template class MP4VR_DLL_PUBLIC Optional<std::uint16_t>;

    struct MP4VR_DLL_PUBLIC SphereRelativePosition
    {
        SphereRegionShapeType shapeType;
        SphereRegionStatic<true, /* HasInterpolate */ false> sphereRegion;
    };

    template class MP4VR_DLL_PUBLIC Union<ViewpointRegionType /* key */,
                                          ViewportRelative /* viewport relative overlay */,
                                          SphereRelativePosition /* sphere relative position */,
                                          uint16_t /* ref_overlay_id */>;

    struct MP4VR_DLL_PUBLIC ViewpointSwitchRegionStruct
    {
        Union<ViewpointRegionType /* key */,
              ViewportRelative /* viewport relative position */,
              SphereRelativePosition /* sphere relative position */,
              uint16_t /* ref_overlay_id */>
            region;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointSwitchRegionStruct);
    };

    // Shortened name a bit from ViewingOrientationInDestinationViewportMode..
    enum class ViewingOrientationMode
    {
        DEFAULT = 0,       // Default OMAF viewport switching process shall be used (recommended viewport if present, or
                           // default viewport otherwise).
        VIEWPORT = 1,      // The specific viewport specified by the contained SphereRegionStruct() shall be used after
                           // transitioning to the new viewpoint.
        NO_INFLUENCE = 2,  // The viewpoint switch does not influence the current viewing orientation logic.
        RESERVED     = 3,  // Reserved (for use by future extensions of ISO/IEC 23090-2).
    };

    template class MP4VR_DLL_PUBLIC Optional<ViewpointSwitchRegionStruct>;

    template class MP4VR_DLL_PUBLIC
        Union<ViewingOrientationMode,
              /* DEFAULT */ Empty,
              /* VIEWPORT */ SphereRegionStatic<false, false>,  // definition of the destination
                                                                // viewport as a sphere region
              /* NO_INFLUENCE */ Empty,
              /* RESERVED */ Empty>;

    using ViewingOrientation = Union<ViewingOrientationMode,
                                     /* DEFAULT */ Empty,
                                     /* VIEWPORT */ SphereRegionStatic<false, false>,  // definition of the destination
                                                                                       // viewport as a sphere region
                                     /* NO_INFLUENCE */ Empty,
                                     /* RESERVED */ Empty>;

    struct MP4VR_DLL_PUBLIC OneViewpointSwitchingStruct
    {
        std::uint32_t destinationViewpointId;
        /* bool transition_effect_flag; */
        /* bool timeline_switching_offset_flag; */
        /* bool viewpoint_switch_region_flag;; */
        // bit(3) reserved = 0;
        ViewingOrientation viewingOrientation;

        /* if (timeline_switching_offset_flag) */
        Optional<ViewpointTimelineSwitchStruct> viewpointTimelineSwitch;
        /* if (transition_effect_flag) */
        /* { */
        Optional<TransitionEffect> transitionEffect;
        /* } */
        /* if (viewpoint_switch_region_flag) */
        /* { */
        DynArray<ViewpointSwitchRegionStruct> viewpointSwitchRegions;
        /* } */

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(OneViewpointSwitchingStruct);
    };

    struct MP4VR_DLL_PUBLIC ViewpointSwitchingListStruct
    {
        DynArray<OneViewpointSwitchingStruct> viewpointSwitching;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointSwitchingListStruct);
    };
    template class MP4VR_DLL_PUBLIC Optional<ViewpointSwitchingListStruct>;

    struct MP4VR_DLL_PUBLIC ViewpointLoopingStruct
    {
        Optional<std::int8_t> maxLoops;  // -1 for infinite loops
        Optional<std::int32_t> loopActivationTime;
        Optional<std::int32_t> loopStartTime;
        Optional<ViewpointSwitchingListStruct> loopExitStruct;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);

        ISOBMFF_DECLARE_DEFAULT_FUNCTIONS(ViewpointLoopingStruct);
    };

    // aligned(8) DynamicViewpointSample()
    // {
    //     ViewpointPosStruct();
    //     if (dynamic_vwpt_gps_flag)
    //         ViewpointGpsPositionStruct();
    //     if (dynamic_vwpt_geomagnetic_info_flag)
    //         ViewpointGeomagneticInfoStruct();
    //     if (dynamic_gcs_rotation_flag)
    //         ViewpointGlobalCoordinateSysRotationStruct();
    //     if (dynamic_vwpt_group_flag)
    //     {
    //         unsigned int(1) vwpt_group_flag;
    //         bit(7) reserved = 0;
    //         if (vwpt_group_flag)
    //             ViewpointGroupStruct();
    //     }
    // }

    // this doesn't exist with this name in the spec; it's used as a building block for DynamicViewpointSample
    struct MP4VR_DLL_PUBLIC DynamicViewpointGroup
    {
       Optional<ViewpointGroupStruct<true>> viewpointGroupStruct;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };
    template class MP4VR_DLL_PUBLIC Optional<DynamicViewpointGroup>;

    struct MP4VR_DLL_PUBLIC DynamicViewpointSampleEntry
    {
        Optional<ViewpointPosStruct> viewpointPosStruct;
        Optional<ViewpointGlobalCoordinateSysRotationStruct> viewpointGlobalCoordinateSysRotationStruct;
        bool dynamicVwptGroupFlag = false;
        Optional<ViewpointGpsPositionStruct> viewpointGpsPositionStruct;
        Optional<ViewpointGeomagneticInfoStruct> viewpointGeomagneticInfoStruct;
        Optional<ViewpointSwitchingListStruct> viewpointSwitchingListStruct;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };

    struct MP4VR_DLL_PUBLIC DynamicViewpointSample
    {
        using Context = DynamicViewpointSampleEntry;

        ViewpointPosStruct viewpointPosStruct;
        Optional<ViewpointGpsPositionStruct> viewpointGpsPositionStruct;
        Optional<ViewpointGeomagneticInfoStruct> viewpointGeomagneticInfoStruct;
        Optional<ViewpointGlobalCoordinateSysRotationStruct> viewpointGlobalCoordinateSysRotationStruct;
        Optional<DynamicViewpointGroup> dynamicViewpointGroup;
        Optional<ViewpointSwitchingListStruct> viewpointSwitchingListStruct;

        // write fields only in sample but skip fields that are in Context
        void write(BitStream& bitstr, const Context& aSampleContext) const;

        // parse will fill all fields; missing fields are filled from Context
        void parse(BitStream& bitstr, const Context& aSampleContext);
    };

    struct MP4VR_DLL_PUBLIC ViewpointInformationStruct
    {
        ViewpointPosStruct viewpointPosStruct;
        ViewpointGroupStruct<true> viewpointGroupStruct;
        ViewpointGlobalCoordinateSysRotationStruct viewpointGlobalCoordinateSysRotationStruct;
        Optional<ViewpointGpsPositionStruct> viewpointGpsPositionStruct;
        Optional<ViewpointGeomagneticInfoStruct> viewpointGeomagneticInfoStruct;
        Optional<ViewpointSwitchingListStruct> viewpointSwitchingListStruct;
        Optional<ViewpointLoopingStruct> viewpointLoopingStruct;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };

    struct MP4VR_DLL_PUBLIC InitialViewpointSampleEntry
    {
        std::uint32_t idOfInitialViewpoint;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };

    struct MP4VR_DLL_PUBLIC RecommendedViewportInfoStruct
    {
        std::uint8_t type;
        DynArray<char> description;
    };

    struct MP4VR_DLL_PUBLIC SphereRegionSample
    {
        using Context = SphereRegionContext;

        DynArray<SphereRegionDynamic> regions;

        void write(BitStream& bitstr, const Context& aSampleContext) const;
        void parse(BitStream& bitstr, const Context& aSampleContext);
    };

    struct MP4VR_DLL_PUBLIC InitialViewpointSample
    {
        std::uint32_t idOfInitialViewpoint;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };

    /* aligned(8) class SpatialRelationship2DSourceBox extends FullBox('2dsr', 0, 0) */
    /* { */
    /*     unsigned int(32) total_width; */
    /*     unsigned int(32) total_height; */
    /*     unsigned int(32) source_id; */
    /* } */

    struct MP4VR_DLL_PUBLIC SpatialRelationship2DSourceData
    {
        std::uint32_t totalWidth;
        std::uint32_t totalHeight;
        std::uint32_t sourceId;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };

    /* aligned(8) class SubPictureRegionBox extends FullBox('sprg', 0, 0) */
    /* { */
    /*     unsigned int(16) object_x; */
    /*     unsigned int(16) object_y; */
    /*     unsigned int(16) object_width; */
    /*     unsigned int(16) object_height; */
    /*     bit(14) reserved = 0; */
    /*     unsigned int(1) track_not_alone_flag; */
    /*     unsigned int(1) track_not_mergeable_flag; */
    /* } */

    struct MP4VR_DLL_PUBLIC SubPictureRegionData
    {
        std::uint16_t objectX;
        std::uint16_t objectY;
        std::uint16_t objectWidth;
        std::uint16_t objectHeight;
        bool trackNotAloneFlag;
        bool trackNotMergeableFlag;

        void write(BitStream& bitstr) const;
        void parse(BitStream& bitstr);
    };

    /* aligned(8) class SpatialRelationship2DDescriptionBox extends TrackGroupTypeBox('2dcc') */
    /* { */
    /*     // track_group_id is inherited from TrackGroupTypeBox; */
    /*     SpatialRelationship2DSourceBox(); */
    /*     // mandatory, must be first */
    /*     SubPictureRegionBox(); */
    /*     // optional */
    /* } */

    struct MP4VR_DLL_PUBLIC SpatialRelationship2DDescriptionData
    {
        SpatialRelationship2DSourceData spatialRelationship2DSourceData;
        SubPictureRegionData subPictureRegionData;
    };

    namespace Utils
    {
        template <typename T>
        void parseOptionalIf(bool aFlag, Optional<T>& aResult, BitStream& aBitStream, T (BitStream::*aRead)())
        {
            if (aFlag)
            {
                aResult = makeOptional((aBitStream.*aRead)());
            }
            else
            {
                aResult.clear();
            }
        }

        template <typename T>
        void parseOptionalIf(bool aFlag, Optional<T>& aResult, BitStream& aBitStream)
        {
            if (aFlag)
            {
                aResult = makeOptional(T());
                aResult->parse(aBitStream);
            }
            else
            {
                aResult.clear();
            }
        }

        template <typename T, typename Context>
        void parseOptionalIf(bool aFlag, Optional<T>& aResult, BitStream& aBitStream, const Context& aContext)
        {
            if (aFlag)
            {
                aResult = makeOptional(T());
                aResult->parse(aBitStream, aContext);
            }
            else
            {
                aResult.clear();
            }
        }

        template <typename T>
        void writeOptional(const Optional<T>& aValue, BitStream& aBitStream, void (BitStream::*aWrite)(const T))
        {
            if (aValue)
            {
                (aBitStream.*aWrite)(*aValue);
            }
        }

        template <typename T>
        void writeOptional(const Optional<T>& aValue, BitStream& aBitStream)
        {
            if (aValue)
            {
                aValue->write(aBitStream);
            }
        }

        template <typename T, typename Context>
        void writeOptional(const Optional<T>& aValue, BitStream& aBitStream, const Context& aContext)
        {
            if (aValue)
            {
                aValue->write(aBitStream, aContext);
            }
        }

        DynArray<char> parseDynArrayString(BitStream& bitstr);

        void writeDynArrayString(BitStream& bitstr, const DynArray<char>& aString);
    }  // namespace Utils

    /* The purpose of these functions - and the classes above - is to hide the BitStream class from the client

       Available functions:

       isobmffFromBytes, with context ISOBMFF::DynamicViewpointSample: ISOBMFF::DynamicViewpointSample,
       isobmffFromBytes, without context: ISOBMFF::DynamicViewpointSampleEntry,
    */

    template <typename T>
    T isobmffFromBytes(const uint8_t* aFrameData, uint32_t aFrameLen);
    template <typename T>
    T isobmffFromBytes(const uint8_t* aFrameData, uint32_t aFrameLen, const typename T::Context& aContext);

    template <typename T>
    DynArray<uint8_t> isobmffToBytes(const T&);
    template <typename T>
    DynArray<uint8_t> isobmffToBytes(const T&, const typename T::Context& aContext);
}  // namespace ISOBMFF

#undef ISOBMFF_DECLARE_DEFAULT_FUNCTIONS

#endif
