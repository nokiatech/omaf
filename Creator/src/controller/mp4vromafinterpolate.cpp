
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
#include <algorithm>

#include "mp4vromafinterpolate.h"
#include "mp4vromafreaders.h"
#include "config/config.h"

namespace VDD
{
    namespace
    {
        template <typename T>
        T interpolate(const InterpolationContext& aCtx, const T& a, const T& b)
        {
            return static_cast<T>(a * (1 - aCtx.at) + b * aCtx.at);
        }

        template <typename T>
        bool interpolate(const InterpolationContext&, const bool& a, const bool&)
        {
            return a;
        }

        template <typename T>
        Optional<T> interpolateOptional(const InterpolationContext& aCtx, const Optional<T>& a,
                                        const Optional<T>& b);

        MP4VR::OverlayInteraction interpolate(const InterpolationContext&,
                                              const MP4VR::OverlayInteraction& a,
                                              const MP4VR::OverlayInteraction&)
        {
            return a;
        }

        MP4VR::ViewportRelativeOverlay interpolate(const InterpolationContext& aCtx,
                                                   const MP4VR::ViewportRelativeOverlay& a,
                                                   const MP4VR::ViewportRelativeOverlay& b)
        {
            MP4VR::ViewportRelativeOverlay x{a};

            x.rectLeftPercent = interpolate(aCtx, x.rectLeftPercent, b.rectLeftPercent);
            x.rectTopPercent = interpolate(aCtx, x.rectTopPercent, b.rectTopPercent);
            x.rectWidthtPercent = interpolate(aCtx, x.rectWidthtPercent, b.rectWidthtPercent);
            x.rectHeightPercent = interpolate(aCtx, x.rectHeightPercent, b.rectHeightPercent);
            x.disparity = interpolate(aCtx, x.disparity, b.disparity);
            x.relativeDisparityFlag = interpolate(aCtx, x.relativeDisparityFlag, b.relativeDisparityFlag);
            return x;
        }

        MP4VR::SphereRegionProperty interpolate(const InterpolationContext& aCtx,
                                                const MP4VR::SphereRegionProperty& a,
                                                const MP4VR::SphereRegionProperty& b)
        {
            MP4VR::SphereRegionProperty x{a};
            x.centreAzimuth = interpolate(aCtx, x.centreAzimuth, b.centreAzimuth);
            x.centreElevation = interpolate(aCtx, x.centreElevation, b.centreElevation);
            x.centreTilt = interpolate(aCtx, x.centreTilt, b.centreTilt);
            x.azimuthRange = interpolate(aCtx, x.azimuthRange, b.azimuthRange);
            x.elevationRange = interpolate(aCtx, x.elevationRange, b.elevationRange);
            x.interpolate = interpolate(aCtx, x.interpolate, b.interpolate);
            return x;
        }

        MP4VR::ProjectedPictureRegionProperty interpolate(
            const InterpolationContext& aCtx, const MP4VR::ProjectedPictureRegionProperty& a,
            const MP4VR::ProjectedPictureRegionProperty& b)
        {
            MP4VR::ProjectedPictureRegionProperty x{a};
            x.pictureWidth = interpolate(aCtx, x.pictureWidth, b.pictureWidth);
            x.pictureHeight = interpolate(aCtx, x.pictureHeight, b.pictureHeight);
            x.regionWidth = interpolate(aCtx, x.regionWidth, b.regionWidth);
            x.regionHeight = interpolate(aCtx, x.regionHeight, b.regionHeight);
            x.regionTop = interpolate(aCtx, x.regionTop, b.regionTop);
            x.regionLeft = interpolate(aCtx, x.regionLeft, b.regionLeft);
            return x;
        }

        MP4VR::SphereRelativeOmniOverlay interpolate(const InterpolationContext& aCtx,
                                                     const MP4VR::SphereRelativeOmniOverlay& a,
                                                     const MP4VR::SphereRelativeOmniOverlay& b)
        {
            MP4VR::SphereRelativeOmniOverlay x{a};
            x.regionIndicationSphereRegion = interpolate(aCtx, x.regionIndicationSphereRegion, b.regionIndicationSphereRegion);

            if (x.regionIndicationSphereRegion)
            {
                // if regionIndicationSphereRegion == true
                x.sphereRegion = interpolate(aCtx, x.sphereRegion, b.sphereRegion);
            }
            else
            {
                // if regionIndicationSphereRegion == false
                x.region = interpolate(aCtx, x.region, b.region);
            }
            x.regionDepthMinus1 = interpolate(aCtx, x.regionDepthMinus1, b.regionDepthMinus1);
            return x;
        }

        MP4VR::RotationProperty interpolate(const InterpolationContext& aCtx, const MP4VR::RotationProperty& a,
                                            const MP4VR::RotationProperty& b)
        {
            MP4VR::RotationProperty x{a};
            x.yaw = interpolate(aCtx, x.yaw, b.yaw);
            x.pitch = interpolate(aCtx, x.pitch, b.pitch);
            x.roll = interpolate(aCtx, x.roll, b.roll);
            return x;
        }

        MP4VR::SphereRelative2DOverlay interpolate(const InterpolationContext& aCtx,
                                                   const MP4VR::SphereRelative2DOverlay& a,
                                                   const MP4VR::SphereRelative2DOverlay& b)
        {
            MP4VR::SphereRelative2DOverlay x{a};
            x.sphereRegion = interpolate(aCtx, x.sphereRegion, b.sphereRegion);
            x.overlayRotation = interpolate(aCtx, x.overlayRotation, b.overlayRotation);
            x.regionDepthMinus1 = interpolate(aCtx, x.regionDepthMinus1, b.regionDepthMinus1);

            return x;
        }

        MP4VR::OverlayOpacity interpolate(const InterpolationContext& aCtx,
                                          const MP4VR::OverlayOpacity& a,
                                          const MP4VR::OverlayOpacity& b)
        {
            MP4VR::OverlayOpacity x{a};
            if (a.doesExist != b.doesExist)
            {
                throw ConfigValuePairInvalid(
                    "Interpolation pairs must both or neither have overlay_opacity",
                    aCtx.valueA, aCtx.valueB);
            }
            x.opacity = interpolate(aCtx, x.opacity, b.opacity);
            return x;
        }

        MP4VR::PackedPictureRegionProperty interpolate(const InterpolationContext& aCtx,
                                                       const MP4VR::PackedPictureRegionProperty& a,
                                                       const MP4VR::PackedPictureRegionProperty& b)
        {
            MP4VR::PackedPictureRegionProperty x{a};
            x.pictureWidth = interpolate(aCtx, a.pictureWidth, b.pictureWidth);
            x.pictureHeight = interpolate(aCtx, a.pictureHeight, b.pictureHeight);
            x.regionWidth = interpolate(aCtx, a.regionWidth, b.regionWidth);
            x.regionHeight = interpolate(aCtx, a.regionHeight, b.regionHeight);
            x.regionTop = interpolate(aCtx, a.regionTop, b.regionTop);
            x.regionLeft = interpolate(aCtx, a.regionLeft, b.regionLeft);
            return x;
        }

        MP4VR::TransformType interpolate(const InterpolationContext&,
                                         const MP4VR::TransformType& a,
                                         const MP4VR::TransformType&)
        {
            MP4VR::TransformType x{a};
            return x;
        }

        MP4VR::OverlaySourceRegion interpolate(const InterpolationContext& aCtx,
                                               const MP4VR::OverlaySourceRegion& a,
                                               const MP4VR::OverlaySourceRegion& b)
        {
            MP4VR::OverlaySourceRegion x{a};
            x.region = interpolate(aCtx, a.region, b.region);
            x.transformType = interpolate(aCtx, a.transformType, b.transformType);
            return x;
        }

        MP4VR::OverlayPriority interpolate(const InterpolationContext& aCtx,
                                           const MP4VR::OverlayPriority& a,
                                           const MP4VR::OverlayPriority& b)
        {
            MP4VR::OverlayPriority x{a};
            x.overlayPriority = interpolate(aCtx, a.overlayPriority, b.overlayPriority);
            return x;
        }

        MP4VR::OverlayLabel interpolate(const InterpolationContext&, const MP4VR::OverlayLabel& a,
                                        const MP4VR::OverlayLabel&)
        {
            MP4VR::OverlayLabel x{a};
            return x;
        }

        template <typename T>
        void propertyInterpolate(const InterpolationContext& aCtx, T& aDest, const T& aFrom, const T& aTo)
        {
            if (aFrom.doesExist != !aTo.doesExist || !aFrom.doesExist)
            {
                aDest = aFrom;
            }
            if (aFrom.overlayControlEssentialFlag != aTo.overlayControlEssentialFlag)
            {
                throw ConfigValuePairInvalid(
                    "When interpolating, overlay_control_essential_flag must be the same",
                    aCtx.valueA, aCtx.valueB);
            }
            if (aFrom.inheritFromOverlayConfigSampleEntry !=
                aTo.inheritFromOverlayConfigSampleEntry)
            {
                // in this case just choose the first sample
                aDest = aFrom;
            }
            else if (aFrom.doesExist)
            {
                aDest = interpolate(aCtx, aFrom, aTo);
            }
        }

        void propertyInterpolateNotSupportedIf(bool aCond, const InterpolationContext& aCtx,
                                               const MP4VR::OverlayControlFlagBase& a,
                                               const MP4VR::OverlayControlFlagBase& b,
                                               const std::string& aMessage)
        {
            if (aCond && (a.doesExist || b.doesExist))
            {
                throw ConfigValuePairInvalid(aMessage, aCtx.valueA, aCtx.valueB);
            }
        }

        MP4VR::SingleOverlayProperty interpolate(const InterpolationContext& aCtx,
                                                 const MP4VR::SingleOverlayProperty& a,
                                                 const MP4VR::SingleOverlayProperty& b)
        {
            MP4VR::SingleOverlayProperty x{a};
            propertyInterpolate(aCtx, x.viewportRelativeOverlay, a.viewportRelativeOverlay,
                                b.viewportRelativeOverlay);
            propertyInterpolate(aCtx, x.sphereRelativeOmniOverlay, a.sphereRelativeOmniOverlay,
                                b.sphereRelativeOmniOverlay);
            propertyInterpolate(aCtx, x.sphereRelative2DOverlay, a.sphereRelative2DOverlay,
                                b.sphereRelative2DOverlay);
            propertyInterpolate(aCtx, x.overlaySourceRegion, a.overlaySourceRegion,
                                b.overlaySourceRegion);
            propertyInterpolateNotSupportedIf(
                a.recommendedViewportOverlay.rcvpTrackRefIdx !=
                    b.recommendedViewportOverlay.rcvpTrackRefIdx,
                aCtx, a.recommendedViewportOverlay, b.recommendedViewportOverlay,
                "Interpolating recommended_viewport_overlay not supported");
            propertyInterpolateNotSupportedIf(
                a.overlayLayeringOrder.layeringOrder != b.overlayLayeringOrder.layeringOrder, aCtx,
                a.overlayLayeringOrder, b.overlayLayeringOrder,
                "Interpolating overlay_layering_order not supported");
            propertyInterpolate(aCtx, x.overlayOpacity, a.overlayOpacity, b.overlayOpacity);
            propertyInterpolate(aCtx, x.overlayInteraction, a.overlayInteraction, b.overlayInteraction);
            propertyInterpolate(aCtx, x.overlayLabel, a.overlayLabel, b.overlayLabel);
            propertyInterpolate(aCtx, x.overlayPriority, a.overlayPriority, b.overlayPriority);
            return x;
        }

        MP4VR::OverlayConfigProperty interpolate(const InterpolationContext& aCtx,
                                                 const MP4VR::OverlayConfigProperty& a,
                                                 const MP4VR::OverlayConfigProperty& b)
        {
            MP4VR::OverlayConfigProperty x{a};
            if (a.numFlagBytes != b.numFlagBytes)
            {
                throw ConfigValuePairInvalid("Interpolation pairs must have the same numFlagBytes",
                                         aCtx.valueA, aCtx.valueB);
            }
            for (auto& overlay : x.overlays)
            {
                for (auto& overlay2 : b.overlays)
                {
                    if (overlay2.overlayId == overlay.overlayId)
                    {
                        overlay = interpolate(aCtx, overlay, overlay2);
                        break;
                    }
                }
                // Missing values are just skipped
            }
            return x;
        }

        ISOBMFF::SphereRegionDynamic interpolate(const InterpolationContext& aCtx,
                                                 const ISOBMFF::SphereRegionDynamic& a,
                                                 const ISOBMFF::SphereRegionDynamic& b)
        {
            ISOBMFF::SphereRegionDynamic sample;

            sample.centreAzimuth = interpolate(aCtx, a.centreAzimuth, b.centreAzimuth);
            sample.centreElevation = interpolate(aCtx, a.centreElevation, b.centreElevation);
            sample.centreTilt = interpolate(aCtx, a.centreTilt, b.centreTilt);

            sample.azimuthRange = interpolate(aCtx, a.azimuthRange, b.azimuthRange);
            sample.elevationRange = interpolate(aCtx, a.elevationRange, b.elevationRange);

            sample.interpolate = b.interpolate;

            return sample;
        }

        ISOBMFF::ViewpointPosStruct interpolate(const InterpolationContext& aCtx,
                                                const ISOBMFF::ViewpointPosStruct& a,
                                                const ISOBMFF::ViewpointPosStruct& b)
        {
            ISOBMFF::ViewpointPosStruct r{};
            r.viewpointPosX = interpolate(aCtx, a.viewpointPosX, b.viewpointPosX);
            r.viewpointPosY = interpolate(aCtx, a.viewpointPosY, b.viewpointPosY);
            r.viewpointPosZ = interpolate(aCtx, a.viewpointPosZ, b.viewpointPosZ);
            return r;
        }

        ISOBMFF::ViewpointGpsPositionStruct interpolate(
            const InterpolationContext& aCtx, const ISOBMFF::ViewpointGpsPositionStruct& a,
            const ISOBMFF::ViewpointGpsPositionStruct& b)
        {
            ISOBMFF::ViewpointGpsPositionStruct r{};
            r.viewpointGpsposLongitude =
                interpolate(aCtx, a.viewpointGpsposLongitude, b.viewpointGpsposLongitude);
            r.viewpointGpsposLatitude =
                interpolate(aCtx, a.viewpointGpsposLatitude, b.viewpointGpsposLatitude);
            r.viewpointGpsposAltitude =
                interpolate(aCtx, a.viewpointGpsposAltitude, b.viewpointGpsposAltitude);
            return r;
        }

        ISOBMFF::ViewpointGeomagneticInfoStruct interpolate(
            const InterpolationContext& aCtx, const ISOBMFF::ViewpointGeomagneticInfoStruct& a,
            const ISOBMFF::ViewpointGeomagneticInfoStruct& b)
        {
            ISOBMFF::ViewpointGeomagneticInfoStruct r{};
            r.viewpointGeomagneticYaw =
                interpolate(aCtx, a.viewpointGeomagneticYaw, b.viewpointGeomagneticYaw);
            r.viewpointGeomagneticPitch =
                interpolate(aCtx, a.viewpointGeomagneticPitch, b.viewpointGeomagneticPitch);
            r.viewpointGeomagneticRoll =
                interpolate(aCtx, a.viewpointGeomagneticRoll, b.viewpointGeomagneticRoll);
            return r;
        }

        ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct interpolate(
            const InterpolationContext& aCtx, const ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct& a,
            const ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct& b)
        {
            ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct r{};
            r.viewpointGcsYaw = interpolate(aCtx, a.viewpointGcsYaw, b.viewpointGcsYaw);
            r.viewpointGcsPitch = interpolate(aCtx, a.viewpointGcsPitch, b.viewpointGcsPitch);
            r.viewpointGcsRoll = interpolate(aCtx, a.viewpointGcsRoll, b.viewpointGcsRoll);
            return r;
        }

        template <typename T>
        Optional<T> interpolateOptional(const InterpolationContext& aCtx, const Optional<T>& a,
                                        const Optional<T>& b)
        {
            if (!!a != !!b)
            {
                throw ConfigValuePairInvalid(
                    "Optional values must either be both or neither set when interpolating",
                    aCtx.valueA, aCtx.valueB);
            }
            if (a)
            {
                return makeOptional(interpolate(aCtx, *a, *b));
            }
            else
            {
                return a;
            }
        }
    }  // namespace

    ISOBMFF::SphereRegionSample interpolate(const InterpolationContext& aCtx, const ISOBMFF::SphereRegionSample& a,
                                            const ISOBMFF::SphereRegionSample& b)
    {
        ISOBMFF::SphereRegionSample sample;
        if (a.regions.numElements != b.regions.numElements)
        {
            throw ConfigValuePairInvalid(
                "Sphere region samples must all have the sane numebr of regions", aCtx.valueA, aCtx.valueB);
        }
        std::vector<ISOBMFF::SphereRegionDynamic> regions;
        regions.reserve(a.regions.numElements);
        for (size_t index = 0; index < a.regions.numElements; ++index)
        {
            regions.push_back(interpolate(aCtx, a.regions[index], b.regions[index]));
        }
        sample.regions = makeDynArray(regions);
        return sample;
    }

    MP4VR::OverlayConfigSample interpolate(const InterpolationContext& aCtx,
                                           const MP4VR::OverlayConfigSample& a,
                                           const MP4VR::OverlayConfigSample& b)
    {
        MP4VR::OverlayConfigSample x{a};
        x.activeOverlayIds = a.activeOverlayIds;
        x.addlActiveOverlays = a.addlActiveOverlays;
        if (!!a.addlActiveOverlays != !!b.addlActiveOverlays)
        {
            x.addlActiveOverlays = a.addlActiveOverlays;
        }
        else
        {
            x.addlActiveOverlays =
                interpolateOptional(aCtx, a.addlActiveOverlays, b.addlActiveOverlays);
        }
        return x;
    }

    MP4VR::InitialViewingOrientationSample interpolate(
        const InterpolationContext& aCtx, const MP4VR::InitialViewingOrientationSample& a,
        const MP4VR::InitialViewingOrientationSample& b)
    {
        MP4VR::InitialViewingOrientationSample x{a};
        x.refreshFlag = interpolate(aCtx, a.refreshFlag, b.refreshFlag);
        x.region = interpolate(aCtx, a.region, b.region);
        return x;
    }

    ISOBMFF::InitialViewpointSample interpolate(
        const InterpolationContext& /*aCtx 0..1*/, const ISOBMFF::InitialViewpointSample& a,
        const ISOBMFF::InitialViewpointSample&)
    {
        return a;
    }

    ISOBMFF::DynamicViewpointSample interpolate(const InterpolationContext& aCtx,
                                                const ISOBMFF::DynamicViewpointSample& a,
                                                const ISOBMFF::DynamicViewpointSample& b)
    {
        ISOBMFF::DynamicViewpointSample r{a};
        r.viewpointPosStruct = interpolate(aCtx, a.viewpointPosStruct, b.viewpointPosStruct);
        r.viewpointGpsPositionStruct = interpolateOptional(aCtx, a.viewpointGpsPositionStruct, b.viewpointGpsPositionStruct);
        r.viewpointGeomagneticInfoStruct = interpolateOptional(aCtx, a.viewpointGeomagneticInfoStruct, b.viewpointGeomagneticInfoStruct);
        r.viewpointGlobalCoordinateSysRotationStruct = interpolateOptional(aCtx, a.viewpointGlobalCoordinateSysRotationStruct, b.viewpointGlobalCoordinateSysRotationStruct);
        r.dynamicViewpointGroup = a.dynamicViewpointGroup;
        r.viewpointSwitchingListStruct = a.viewpointSwitchingListStruct;
        return r;
    }

    InterpolationInstructions readInterpolationInstructions(const ConfigValue& aValue)
    {
        InterpolationInstructions r{};
        r.frameInterval = readGeneric<FrameDuration>("interpolation interval")(aValue["interval"]);
        return r;
    }

}  // namespace VDD
