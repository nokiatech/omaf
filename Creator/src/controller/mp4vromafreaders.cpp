
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
#include "mp4vromafreaders.h"

#include <cmath>
#include <type_traits>

#include "configreader.h"

namespace VDD
{
    bool anySingleOverlayPropertySet(const MP4VR::SingleOverlayProperty& aProp)
    {
        // clang-format off
        return false
            || aProp.viewportRelativeOverlay.doesExist
            || aProp.sphereRelativeOmniOverlay.doesExist
            || aProp.sphereRelative2DOverlay.doesExist
            || aProp.overlaySourceRegion.doesExist
            || aProp.recommendedViewportOverlay.doesExist
            || aProp.overlayLayeringOrder.doesExist
            || aProp.overlayOpacity.doesExist
            || aProp.overlayInteraction.doesExist
            || aProp.overlayLabel.doesExist
            || aProp.overlayPriority.doesExist
            || aProp.associatedSphereRegion.doesExist
            || aProp.overlayAlphaCompositing.doesExist;
        // clang-format on
    }

    void LabelEntityIdMapping::addLabel(EntityIdReference aEntityIdReference)
    {
        assert(aEntityIdReference.references.size() > 0);
        auto forwardIt = forward.find(aEntityIdReference.label);
        if (forwardIt == forward.end())
        {
            forward[aEntityIdReference.label] = aEntityIdReference;

            for (auto& reference : aEntityIdReference.references)
            {
                assert(reference.entityId);
                assert(labelByEntityId.count(reference.entityId) == 0);
                labelByEntityId[reference.entityId] = aEntityIdReference.label;
            }
        }
        else
        {
            for (auto& reference : aEntityIdReference.references)
            {
                assert(reference.entityId);
                assert(labelByEntityId.count(reference.entityId) == 0 ||
                       labelByEntityId.at(reference.entityId) == aEntityIdReference.label);
                assert(forwardIt->second.entityIds.count(reference.entityId) == 0);
                labelByEntityId[reference.entityId] = aEntityIdReference.label;
                forwardIt->second.entityIds.insert(reference.entityId);
                forwardIt->second.references.push_back(reference);
            }
        }
    }

    const EntityIdReference& LabelEntityIdMapping::entityById(const EntityId& aEntityId) const
    {
        auto it = labelByEntityId.find(aEntityId);
        assert(it != labelByEntityId.end());
        return forward.at(it->second);
    }

    float readFloatPercent(const ConfigValue& valNode)
    {
        float val = readDouble(valNode);
        if (val < 0.0 || val > 100.0)
        {
            throw ConfigValueInvalid("Percentage value must be between 0..100", valNode);
        }
        return val;
    }

    auto readFloatUint360Degrees(int32_t stepsInDegree)
        -> std::function<uint32_t(const ConfigValue& valNode)>
    {
        return [=](const ConfigValue& valNode) {
            auto val = readDouble(valNode);

            // degrees are converted to range between [0, 360*65536 - 1]

            // 1. normalize read degrees to range of [0, 360[
            val = fmod(val, 360.0);  // range -360..360
            val = val + 360.0;       // range 0..720
            val = fmod(val, 360.0);  // back to 0..360

            // 2. quantize float value to 2^16 discreet values per degree
            uint32_t quantizedVal = val * stepsInDegree;

            return quantizedVal;
        };
    }

    auto readFloatUint180Degrees(int32_t stepsInDegree)
        -> std::function<uint32_t(const ConfigValue& valNode)>
    {
        return [=](const ConfigValue& valNode) {
            auto positiveDegrees = readFloatUint360Degrees(stepsInDegree)(valNode);

            if (positiveDegrees > (180 * static_cast<uint32_t>(stepsInDegree)))
            {
                throw ConfigValueInvalid("Degrees out of range. Should be between 0..180", valNode);
            }

            return positiveDegrees;
        };
    }

    auto readFloatSigned180Degrees(int32_t stepsInDegree)
        -> std::function<int32_t(const ConfigValue& valNode)>
    {
        return [=](const ConfigValue& valNode) {
            int32_t positiveDegrees =
                static_cast<int32_t>(readFloatUint360Degrees(stepsInDegree)(valNode));

            if (positiveDegrees >= 180 * stepsInDegree)
            {
                // normalize range from [180..360[ to [-180, 0[
                return -180 * stepsInDegree + (positiveDegrees - (180 * stepsInDegree));
            }

            return positiveDegrees;
        };
    }

    auto readFloatSigned90Degrees(int32_t stepsInDegree)
        -> std::function<int32_t(const ConfigValue& valNode)>
    {
        return [=](const ConfigValue& valNode) {
            int32_t signedDegrees = readFloatSigned180Degrees(stepsInDegree)(valNode);

            if (signedDegrees < (-90 * stepsInDegree) || signedDegrees > (90 * stepsInDegree))
            {
                throw ConfigValueInvalid("Degrees out of range. Should be between -90..90",
                                         valNode);
            }

            return signedDegrees;
        };
    }

    MP4VR::DynArray<char> readDynArrayString(const ConfigValue& aValue)
    {
        return makeDynArray(readString(aValue));
    }

    MP4VR::ProjectedPictureRegionProperty readProjectedPictureRegionProperty(
        const ConfigValue& aValue)
    {
        MP4VR::ProjectedPictureRegionProperty r{};
        r.pictureWidth = readUint32(aValue["picture_width"]);
        r.pictureHeight = readUint32(aValue["picture_height"]);
        r.regionWidth = readUint32(aValue["region_width"]);
        r.regionHeight = readUint32(aValue["region_height"]);
        r.regionTop = readUint32(aValue["region_top"]);
        r.regionLeft = readUint32(aValue["region_left"]);
        return r;
    }

    ISOBMFF::ProjectedPictureRegion readISOBMFFProjectedPictureRegion(
        const ConfigValue& aValue)
    {
        ISOBMFF::ProjectedPictureRegion r{};
        r.pictureWidth = readUint32(aValue["picture_width"]);
        r.pictureHeight = readUint32(aValue["picture_height"]);
        r.regionWidth = readUint32(aValue["region_width"]);
        r.regionHeight = readUint32(aValue["region_height"]);
        r.regionTop = readUint32(aValue["region_top"]);
        r.regionLeft = readUint32(aValue["region_left"]);
        return r;
    }

    MP4VR::PackedPictureRegionProperty readPackedPictureRegionProperty(const ConfigValue& aValue)
    {
        MP4VR::PackedPictureRegionProperty r{};
        r.pictureWidth = readUint16(aValue["packed_picture_width"]);
        r.pictureHeight = readUint16(aValue["packed_picture_height"]);
        r.regionWidth = readUint16(aValue["packed_reg_width"]);
        r.regionHeight = readUint16(aValue["packed_reg_height"]);
        r.regionTop = readUint16(aValue["packed_reg_top"]);
        r.regionLeft = readUint16(aValue["packed_reg_left"]);
        return r;
    }

    ISOBMFF::PackedPictureRegion readISOBMFFPackedPictureRegion(const ConfigValue& aValue)
    {
        ISOBMFF::PackedPictureRegion r{};
        r.pictureWidth = readUint16(aValue["picture_width"]);
        r.pictureHeight = readUint16(aValue["picture_height"]);
        r.regionWidth = readUint16(aValue["region_width"]);
        r.regionHeight = readUint16(aValue["region_height"]);
        r.regionTop = readUint16(aValue["region_top"]);
        r.regionLeft = readUint16(aValue["region_left"]);
        return r;
    }

    MP4VR::SphereRegionProperty readSphereRegionProperty(const ConfigValue& aValue)
    {
        MP4VR::SphereRegionProperty r{};
        r.centreAzimuth =
            readFloatSigned180Degrees(kStepsInDegree)(aValue["centre_azimuth_degrees"]);
        r.centreElevation =
            readFloatSigned90Degrees(kStepsInDegree)(aValue["centre_elevation_degrees"]);

        r.centreTilt =
            aValue["centre_tilt_degrees"]
                ? readFloatSigned180Degrees(kStepsInDegree)(aValue["centre_tilt_degrees"])
                : 0;
        r.azimuthRange = optionWithDefault(aValue, "azimuth_range_degrees",
                                           readFloatUint360Degrees(kStepsInDegree), 0.0);
        r.elevationRange = optionWithDefault(aValue, "elevation_range_degrees",
                                             readFloatUint360Degrees(kStepsInDegree), 0.0);

        r.interpolate = optionWithDefault(aValue, "interpolate_flag", readBool, false);

        return r;
    }

    auto readSphereRegionSample(const ISOBMFF::SphereRegionConfigStruct& aConfig)
        -> std::function<ISOBMFF::SphereRegionSample(const ConfigValue& aValue)>
    {
        return [=](const ConfigValue& aValue) {
            ISOBMFF::SphereRegionSample r{};

            if (aValue->type() == Json::arrayValue)
            {
                // OMAF 7.7.2.3 says that there is always exactly 1 region, but for future this allows already multiple
                r.regions =
                    readDynArray("sphere regions", readSphereRegionDynamic(aConfig))(aValue);
            }
            else
            {
                r.regions = {1};
                r.regions[0] = readSphereRegionDynamic(aConfig)(aValue);
            }

            return r;
        };
    }

    ISOBMFF::InitialViewpointSample readInitialViewpointSample(const ConfigValue& aValue)
    {
        ISOBMFF::InitialViewpointSample r{};

        r.idOfInitialViewpoint = readUint32(aValue["id_of_initial_viewpoint"]);

        return r;
    }

    uint16_t readRegiondDepthMinus1FromPercent(const ConfigValue& aValue)
    {
        return percentToFP<uint16_t>(readFloatPercent(aValue));
    }

    bool readToOverlayControlFlagBase(MP4VR::OverlayControlFlagBase& r, const ConfigValue& aValue,
                                      OverlayControlRead aRead)
    {
        r.doesExist = true;
        r.overlayControlEssentialFlag =
            optionWithDefault(aValue, "overlay_control_essential_flag", readBool, false);
        r.inheritFromOverlayConfigSampleEntry =
            aRead == OverlayControlRead::ReadInherited &&
            optionWithDefault(aValue, "inherited", readBool, false);
        return r.inheritFromOverlayConfigSampleEntry;
    }

    bool readToOverlayControlFlagBase(ISOBMFF::OverlayControlFlagBase& r, const ConfigValue& aValue,
                                      OverlayControlRead aRead)
    {
        r.doesExist = true;
        r.overlayControlEssentialFlag =
            optionWithDefault(aValue, "overlay_control_essential_flag", readBool, false);
        r.inheritFromOverlayConfigSampleEntry =
            aRead == OverlayControlRead::ReadInherited &&
            optionWithDefault(aValue, "inherited", readBool, false);
        return r.inheritFromOverlayConfigSampleEntry;
    }

    MP4VR::SphereRelativeOmniOverlay readSphereRelativeOmniOverlay(const ConfigValue& aValue)
    {
        MP4VR::SphereRelativeOmniOverlay r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.timelineChangeFlag = readOptional(readBool)(aValue["timeline_change_flag"]).value_or(false);
        r.regionIndicationSphereRegion =
            optionWithDefault(aValue, "region_indication_sphere_region", readBool, true);
        if (r.regionIndicationSphereRegion)
        {
            // if regionIndicationSphereRegion == true
            r.sphereRegion = readSphereRegionProperty(aValue);
        }
        else
        {
            // if regionIndicationSphereRegion == false
            r.region = readProjectedPictureRegionProperty(aValue);
        }
        r.regionDepthMinus1 = readRegiondDepthMinus1FromPercent(aValue["region_depth_percent"]);

        return r;
    }

    // json schema: OMAFRotationProperty
    MP4VR::RotationProperty readRotationProperty(const ConfigValue& aValue)
    {
        MP4VR::RotationProperty r{};
        r.yaw = readFloatSigned180Degrees(kStepsInDegree)(aValue["yaw_degrees"]);
        r.pitch = readFloatSigned90Degrees(kStepsInDegree)(aValue["pitch_degrees"]);
        r.roll = readFloatSigned180Degrees(kStepsInDegree)(aValue["roll_degrees"]);
        return r;
    }

    MP4VR::SphereRelative2DOverlay readSphereRelative2DOverlay(const ConfigValue& aValue)
    {
        MP4VR::SphereRelative2DOverlay r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.timelineChangeFlag = readOptional(readBool)(aValue["timeline_change_flag"]).value_or(false);
        r.sphereRegion = readSphereRegionProperty(aValue);
        r.overlayRotation = readRotationProperty(aValue);
        r.regionDepthMinus1 = readRegiondDepthMinus1FromPercent(aValue["region_depth_percent"]);
        return r;
    }

    MP4VR::TransformType readTransformType(const ConfigValue& aValue)
    {
        static std::map<std::string, MP4VR::TransformType> mapping{
            {"none", MP4VR::TransformType::NONE},
            {"mirror_horizontal", MP4VR::TransformType::MIRROR_HORIZONTAL},
            {"rotate_ccw_180", MP4VR::TransformType::ROTATE_CCW_180},
            {"rotate_ccw_180_before_mirroring",
             MP4VR::TransformType::ROTATE_CCW_180_BEFORE_MIRRORING},
            {"rotate_ccw_90_before_mirroring",
             MP4VR::TransformType::ROTATE_CCW_90_BEFORE_MIRRORING},
            {"rotate_ccw_90", MP4VR::TransformType::ROTATE_CCW_90},
            {"rotate_ccw_270_before_mirroring",
             MP4VR::TransformType::ROTATE_CCW_270_BEFORE_MIRRORING},
            {"rotate_ccw_270", MP4VR::TransformType::ROTATE_CCW_270}};
        return readMapping("transform type", mapping)(aValue);
    }

    MP4VR::OverlaySourceRegion readOverlaySourceRegion(const ConfigValue& aValue)
    {
        MP4VR::OverlaySourceRegion r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.region = readPackedPictureRegionProperty(aValue);
        r.transformType = readTransformType(aValue["transform_type"]);
        return r;
    }

    MP4VR::RecommendedViewportOverlay readRecommendedViewportOverlay(const ConfigValue& aValue)
    {
        MP4VR::RecommendedViewportOverlay r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.rcvpTrackRefIdx = readUint32(aValue["rcvp_track_ref_idx"]);
        return r;
    }

    MP4VR::OverlayLayeringOrder readOverlayLayeringOrder(const ConfigValue& aValue)
    {
        MP4VR::OverlayLayeringOrder r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.layeringOrder = readInt16(aValue["layering_order"]);
        return r;
    }

    MP4VR::OverlayOpacity readOverlayOpacity(const ConfigValue& aValue)
    {
        MP4VR::OverlayOpacity r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.opacity = readUint8(aValue["opacity_percent"]);
        return r;
    }

    MP4VR::OverlayInteraction readOverlayInteraction(const ConfigValue& aValue)
    {
        MP4VR::OverlayInteraction r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.changePositionFlag = readBool(aValue["change_position_flag"]);
        r.changeDepthFlag = readBool(aValue["change_depth_flag"]);
        r.switchOnOffFlag = readBool(aValue["switch_on_off_flag"]);
        r.changeOpacityFlag = readBool(aValue["change_opacity_flag"]);
        r.resizeFlag = readBool(aValue["resize_flag"]);
        r.rotationFlag = readBool(aValue["rotation_flag"]);
        r.sourceSwitchingFlag = readBool(aValue["source_switching_flag"]);
        r.cropFlag = readBool(aValue["crop_flag"]);
        return r;
    }

    MP4VR::OverlayLabel readOverlayLabel(const ConfigValue& aValue)
    {
        MP4VR::OverlayLabel r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.overlayLabel = readDynArrayString(aValue["overlay_label"]);
        return r;
    }

    MP4VR::OverlayPriority readOverlayPriority(const ConfigValue& aValue)
    {
        MP4VR::OverlayPriority r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.overlayPriority = readUint8(aValue["overlay_priority"]);
        return r;
    }

    ISOBMFF::PackedPictureRegion readPackedPictureRegion(const ConfigValue& aValue)
    {
        ISOBMFF::PackedPictureRegion r{};
        r.pictureWidth = readUint16(aValue["packed_picture_width"]);
        r.pictureHeight = readUint16(aValue["packed_picture_height"]);
        r.regionWidth = readUint16(aValue["packed_reg_width"]);
        r.regionHeight = readUint16(aValue["packed_reg_height"]);
        r.regionTop = readUint16(aValue["packed_reg_top"]);
        r.regionLeft = readUint16(aValue["packed_reg_left"]);
        return r;
    }

    ISOBMFF::ViewportRelative readViewportRelative(const ConfigValue& aValue)
    {
        ISOBMFF::ViewportRelative r{};
        r.rectLeftPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_left_percent"]);
        r.rectTopPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_top_percent"]);
        r.rectWidthtPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_width_percent"]);
        r.rectHeightPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_height_percent"]);
        return r;
    }

    ISOBMFF::MediaAlignmentType readMediaAlignmentType(const ConfigValue& aValue)
    {
        static std::map<std::string, ISOBMFF::MediaAlignmentType> mapping{
            {"stretch_to_fill", ISOBMFF::MediaAlignmentType::STRETCH_TO_FILL},
            {"hc_vc_scale", ISOBMFF::MediaAlignmentType::HC_VC_SCALE},
            {"hc_vt_scale", ISOBMFF::MediaAlignmentType::HC_VT_SCALE},
            {"hc_vb_scale", ISOBMFF::MediaAlignmentType::HC_VB_SCALE},
            {"hl_vc_scale", ISOBMFF::MediaAlignmentType::HL_VC_SCALE},
            {"hr_vc_scale", ISOBMFF::MediaAlignmentType::HR_VC_SCALE},
            {"hl_vt_scale", ISOBMFF::MediaAlignmentType::HL_VT_SCALE},
            {"hr_vt_scale", ISOBMFF::MediaAlignmentType::HR_VT_SCALE},
            {"hl_vb_scale", ISOBMFF::MediaAlignmentType::HL_VB_SCALE},
            {"hr_vb_scale", ISOBMFF::MediaAlignmentType::HR_VB_SCALE},
            {"hc_vc_crop", ISOBMFF::MediaAlignmentType::HC_VC_CROP},
            {"hc_vt_crop", ISOBMFF::MediaAlignmentType::HC_VT_CROP},
            {"hc_vb_crop", ISOBMFF::MediaAlignmentType::HC_VB_CROP},
            {"hl_vc_crop", ISOBMFF::MediaAlignmentType::HL_VC_CROP},
            {"hr_vc_crop", ISOBMFF::MediaAlignmentType::HR_VC_CROP},
            {"hl_vt_crop", ISOBMFF::MediaAlignmentType::HL_VT_CROP},
            {"hr_vt_crop", ISOBMFF::MediaAlignmentType::HR_VT_CROP},
            {"hl_vb_crop", ISOBMFF::MediaAlignmentType::HL_VB_CROP},
            {"hr_vb_crop", ISOBMFF::MediaAlignmentType::HR_VB_CROP}};
        return readMapping("media alignment type", mapping)(aValue);
    }

    MP4VR::ViewportRelativeOverlay readViewportRelativeOverlay(const ConfigValue& aValue)
    {
        MP4VR::ViewportRelativeOverlay r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.rectLeftPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_left_percent"]);
        r.rectTopPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_top_percent"]);
        r.rectWidthtPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_width_percent"]);
        r.rectHeightPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_height_percent"]);

        r.relativeDisparityFlag =
            readOptional(readBool)(aValue["relative_disparity_flag"]).value_or(false);
        r.mediaAlignment = readOptional(readMediaAlignmentType)(aValue["media_alignment"])
                               .value_or(ISOBMFF::MediaAlignmentType::STRETCH_TO_FILL);
        r.disparity = readFloatPercentAsFP<int16_t>(aValue["disparity_percent"]);

        return r;
    }

    ISOBMFF::SphereRegionShapeType readSphereRegionShapeType(const ConfigValue& aValue);

    MP4VR::AssociatedSphereRegion readAssociatedSphereRegion(const ConfigValue& aValue)
    {
        MP4VR::AssociatedSphereRegion r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.shapeType = readSphereRegionShapeType(aValue["shape_type"]);
        r.sphereRegion = readSphereRegionProperty(aValue["region"]);
        return r;
    }

    ISOBMFF::AlphaBlendingModeType readAlphaBlendingModeType(const ConfigValue& aValue)
    {
        static std::map<std::string, ISOBMFF::AlphaBlendingModeType> mapping{
            {"source_over", ISOBMFF::AlphaBlendingModeType::SourceOver},
        };
        return readMapping("alpha blending mode type", mapping)(aValue);
    }

    MP4VR::OverlayAlphaCompositing readOverlayAlphaCompositing(const ConfigValue& aValue)
    {
        MP4VR::OverlayAlphaCompositing r{};
        // no support for other keys due
        // if (readToOverlayControlFlagBase(r, aValue))
        // {
        //     return r;
        // }
        r.doesExist = true;
        r.alphaBlendingMode = readAlphaBlendingModeType(aValue);
        return r;
    }

    MP4VR::SingleOverlayProperty readSingleOverlayProperty(const ConfigValue& aValue)
    {
        MP4VR::SingleOverlayProperty r{};
        r.overlayId = readUint16(aValue["id"]);
        r.viewportRelativeOverlay =
            optionWithDefault(aValue, "viewport_relative", readViewportRelativeOverlay, {});
        r.sphereRelativeOmniOverlay =
            optionWithDefault(aValue, "sphere_relative_omni", readSphereRelativeOmniOverlay, {});
        r.sphereRelative2DOverlay =
            optionWithDefault(aValue, "sphere_relative_2d", readSphereRelative2DOverlay, {});
        r.overlaySourceRegion =
            optionWithDefault(aValue, "overlay_source_region", readOverlaySourceRegion, {});
        r.recommendedViewportOverlay = optionWithDefault(aValue, "recommended_viewport_overlay",
                                                         readRecommendedViewportOverlay, {});
        r.overlayLayeringOrder =
            optionWithDefault(aValue, "overlay_layering_order", readOverlayLayeringOrder, {});
        r.overlayOpacity = optionWithDefault(aValue, "overlay_opacity", readOverlayOpacity, {});
        r.overlayInteraction =
            optionWithDefault(aValue, "overlay_interaction", readOverlayInteraction, {});
        r.overlayLabel = optionWithDefault(aValue, "overlay_label", readOverlayLabel, {});
        r.overlayPriority = optionWithDefault(aValue, "overlay_priority", readOverlayPriority, {});
        r.associatedSphereRegion =
            optionWithDefault(aValue, "associated_sphere_region", readAssociatedSphereRegion, {});
        r.overlayAlphaCompositing =
            optionWithDefault(aValue, "overlay_alpha_compositing", readOverlayAlphaCompositing, {});

        return r;
    }

    struct OverlayConfigPropertyInfo {
        std::vector<uint16_t> activeOverlayIds;
        std::vector<MP4VR::SingleOverlayProperty> overlays;
    };

    OverlayConfigPropertyInfo readOverlayConfigPropertyInfo(const ConfigValue& aValue)
    {
        OverlayConfigPropertyInfo r{};
        std::vector<MP4VR::SingleOverlayProperty> overlays;
        for (auto x : readList("overlays", readSingleOverlayProperty)(aValue["config"]))
        {
            if (anySingleOverlayPropertySet(x))
            {
                r.overlays.push_back(x);
            }
            else
            {
                r.activeOverlayIds.push_back(x.overlayId);
            }
        }
        return r;
    }

    MP4VR::InitialViewingOrientationSample readInitialViewingOrientationSample2(
        const ConfigValue& aValue)
    {
        MP4VR::InitialViewingOrientationSample r{};
        r.region = readSphereRegionProperty(aValue);
        r.refreshFlag = optionWithDefault(aValue, "refresh", readBool, false);
        return r;
    }

    MP4VR::OverlayConfigProperty readOverlayConfigProperty(const ConfigValue& aValue)
    {
        MP4VR::OverlayConfigProperty r{};
        r.numFlagBytes = optionWithDefault(aValue, "num_flag_bytes", readUint8, 2);
        std::vector<MP4VR::SingleOverlayProperty> overlays;
        for (auto x : readList("overlays", readSingleOverlayProperty)(aValue["config"]))
        {
            if (anySingleOverlayPropertySet(x))
            {
                overlays.push_back(x);
            }
        }
        r.overlays = makeDynArray(overlays);
        return r;
    }

    MP4VR::OverlayConfigSample readOverlayConfigSample(const ConfigValue& aValue)
    {
        MP4VR::OverlayConfigSample r;
        auto overlayConfigPropertyInfo = readOverlayConfigPropertyInfo(aValue);
        if (overlayConfigPropertyInfo.overlays.size())
        {
            r.addlActiveOverlays = MP4VR::OverlayConfigProperty();
            r.addlActiveOverlays->numFlagBytes = optionWithDefault(aValue, "num_flag_bytes", readUint8, 2);
            r.addlActiveOverlays->overlays = makeDynArray(overlayConfigPropertyInfo.overlays);
        }
        r.activeOverlayIds = makeDynArray(overlayConfigPropertyInfo.activeOverlayIds);

        return r;
    }

    ISOBMFF::ViewpointPosStruct readViewpointPosStruct(const ConfigValue& aValue)
    {
        ISOBMFF::ViewpointPosStruct r{};
        r.viewpointPosX = readInt32(aValue["x_01mm"]);
        r.viewpointPosY = readInt32(aValue["y_01mm"]);
        r.viewpointPosZ = readInt32(aValue["z_01mm"]);
        return r;
    }

    // json schema: OMAFViewpointGlobalCoordinateSysRotationStruct
    ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct
    readViewpointGlobalCoordinateSysRotationStruct(const ConfigValue& aValue)
    {
        ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct r{};
        r.viewpointGcsYaw = readFloatSigned180Degrees(kStepsInDegree)(aValue["yaw_degrees"]);
        r.viewpointGcsPitch = readFloatSigned90Degrees(kStepsInDegree)(aValue["pitch_degrees"]);
        r.viewpointGcsRoll = readFloatSigned180Degrees(kStepsInDegree)(aValue["roll_degrees"]);
        return r;
    }

    ISOBMFF::ViewpointGpsPositionStruct readViewpointGpsPositionStruct(const ConfigValue& aValue)
    {
        ISOBMFF::ViewpointGpsPositionStruct r{};
        r.viewpointGpsposLongitude =
            readFloatSigned180Degrees(kStepsInGpsDegree)(aValue["longitude_degrees"]);
        r.viewpointGpsposLatitude =
            readFloatSigned90Degrees(kStepsInGpsDegree)(aValue["latitude_degrees"]);
        r.viewpointGpsposAltitude = readInt32(aValue["altitude_mm"]);
        return r;
    }

    // json schema: OMAFViewpointGeomagneticInfoStruct
    ISOBMFF::ViewpointGeomagneticInfoStruct readViewpointGeomagneticInfoStruct(
        const ConfigValue& aValue)
    {
        ISOBMFF::ViewpointGeomagneticInfoStruct r{};
        r.viewpointGeomagneticYaw =
            readFloatSigned180Degrees(kStepsInDegree)(aValue["yaw_degrees"]);
        r.viewpointGeomagneticPitch =
            readFloatSigned90Degrees(kStepsInDegree)(aValue["pitch_degrees"]);
        r.viewpointGeomagneticRoll =
            readFloatSigned180Degrees(kStepsInDegree)(aValue["roll_degrees"]);
        return r;
    }

    template <bool HasRange, bool HasInterpolate>
    void fillSphereRegionRange(ISOBMFF::SphereRegionStatic<HasRange, HasInterpolate>& /* aDst */,
                               const ConfigValue& /* aValue */)
    {
        // nothing
    }

    template <bool HasInterpolate>
    void fillSphereRegionRange(ISOBMFF::SphereRegionStatic<true, HasInterpolate>& aDst,
                               const ConfigValue& aValue)
    {
        aDst.azimuthRange =
            readOptional(readFloatUint360Degrees(kStepsInDegree))(aValue["azimuth_range_degrees"])
                .value_or(0);
        aDst.elevationRange =
            readOptional(readFloatUint360Degrees(kStepsInDegree))(aValue["elevation_range_degrees"])
                .value_or(0);
    }

    template <bool HasRange, bool HasInterpolate>
    void fillSphereRegionInterpolate(
        ISOBMFF::SphereRegionStatic<HasRange, HasInterpolate>& /* aDst */,
        const ConfigValue& /* aValue */)
    {
        // nothing
    }

    template <bool HasRange>
    void fillSphereRegionInterpolate(ISOBMFF::SphereRegionStatic<HasRange, true>& aDst,
                                     const ConfigValue& aValue)
    {
        aDst.interpolate = readOptional(readBool)(aValue["interpolate_flag"]).value_or(false);
    }

    template <bool HasRange, bool HasInterpolate>
    ISOBMFF::SphereRegionStatic<HasRange, HasInterpolate> readSphereRegionStatic(
        const ConfigValue& aValue)
    {
        ISOBMFF::SphereRegionStatic<HasRange, HasInterpolate> r{};
        r.centreAzimuth =
            readFloatSigned180Degrees(kStepsInDegree)(aValue["centre_azimuth_degrees"]);
        r.centreElevation =
            readFloatSigned180Degrees(kStepsInDegree)(aValue["centre_elevation_degrees"]);
        r.centreTilt = readFloatSigned180Degrees(kStepsInDegree)(aValue["centre_tilt_degrees"]);

        fillSphereRegionRange(r, aValue);
        fillSphereRegionInterpolate(r, aValue);

        return r;
    };

    // json schema: OMAFSphereRegionShapeType
    ISOBMFF::SphereRegionShapeType readSphereRegionShapeType(const ConfigValue& aValue)
    {
        static std::map<std::string, ISOBMFF::SphereRegionShapeType> mapping{
            {"four_great_circles", ISOBMFF::SphereRegionShapeType::FourGreatCircles},
            {"two_azimuth_and_two_elevation_circles",
             ISOBMFF::SphereRegionShapeType::TwoAzimuthAndTwoElevationCircles},
        };

        return readMapping("shape type", mapping)(aValue);
    }

    // json schema: OMAFSphereRelativePosition
    ISOBMFF::SphereRelativePosition readSphereRelativePosition(const ConfigValue& aValue)
    {
        ISOBMFF::SphereRelativePosition r{};
        r.shapeType = readSphereRegionShapeType(aValue["shape_type"]);
        r.sphereRegion = readSphereRegionStatic<true, false>(aValue["region"]);
        return r;
    }

    auto readSphereRegionDynamic(const ISOBMFF::SphereRegionContext& aContext) ->
        std::function<ISOBMFF::SphereRegionDynamic(const ConfigValue& aValue)>
    {
        return [=](const ConfigValue& aValue) {
            ISOBMFF::SphereRegionDynamic r{};
            r.centreAzimuth =
                readFloatSigned180Degrees(kStepsInDegree)(aValue["centre_azimuth_degrees"]);
            r.centreElevation =
                readFloatSigned180Degrees(kStepsInDegree)(aValue["centre_elevation_degrees"]);
            r.centreTilt = readFloatSigned180Degrees(kStepsInDegree)(aValue["centre_tilt_degrees"]);

            if (aContext.hasRange)
            {
                fillSphereRegionRange(r, aValue);
            }
            if (aContext.hasInterpolate)
            {
                fillSphereRegionInterpolate(r, aValue);
            }

            return r;
        };
    };

    ISOBMFF::ViewpointTimelineSwitchStruct readViewpointTimelineSwitchStruct(
        const ConfigValue& aValue)
    {
        ISOBMFF::ViewpointTimelineSwitchStruct r{};
        r.minTime = readOptional(readInt32)(aValue["min_time"]);
        r.maxTime = readOptional(readInt32)(aValue["max_time"]);
        if (bool(aValue["absolute_t_offset"]) == bool(aValue["relative_t_offset"]))
        {
            throw ConfigValueInvalid(
                "exactly one of absoluteTOffset or relativeTOffset must be provided (not both or "
                "none)",
                aValue);
        }
        else if (aValue["absolute_t_offset"])
        {
            r.tOffset.set<ISOBMFF::OffsetKind::ABSOLUTE_TIME>(
                readUint32(aValue["absolute_t_offset"]));
        }
        else  // if (aValue["relative_t_offset"])
        {
            r.tOffset.set<ISOBMFF::OffsetKind::RELATIVE_TIME>(
                readInt32(aValue["relative_t_offset"]));
        }
        return r;
    }

    ISOBMFF::TransitionEffect readTransitionEffect(const ConfigValue& aValue)
    {
        ISOBMFF::TransitionEffect r{};
        using T = ISOBMFF::TransitionEffectType;  // shorten the amount of characters a lot..

        static std::map<std::string, T> effectTypeMapping{
            {"zoom_in_effect", T::ZOOM_IN_EFFECT},
            {"walk_though_effect", T::WALK_THOUGH_EFFECT},
            {"fade_to_black_effect", T::FADE_TO_BLACK_EFFECT},
            {"mirror_effect", T::MIRROR_EFFECT},
            {"video_transition_track_id", T::VIDEO_TRANSITION_TRACK_ID},
            {"video_transition_url", T::VIDEO_TRANSITION_URL}};

        switch (readMapping("transition effect type",
                            effectTypeMapping)(aValue["transition_effect_type"]))
        {
        case T::ZOOM_IN_EFFECT:
            r.transitionEffect.set<T::ZOOM_IN_EFFECT>({});
            break;
        case T::WALK_THOUGH_EFFECT:
            r.transitionEffect.set<T::WALK_THOUGH_EFFECT>({});
            break;
        case T::FADE_TO_BLACK_EFFECT:
            r.transitionEffect.set<T::FADE_TO_BLACK_EFFECT>({});
            break;
        case T::MIRROR_EFFECT:
            r.transitionEffect.set<T::MIRROR_EFFECT>({});
            break;
        case T::VIDEO_TRANSITION_TRACK_ID:
            r.transitionEffect.set<T::VIDEO_TRANSITION_TRACK_ID>(
                readUint32(aValue["video_track_id"]));
            break;
        case T::VIDEO_TRANSITION_URL:
            r.transitionEffect.set<T::VIDEO_TRANSITION_URL>(
                readDynArrayString(aValue["video_url"]));
            break;
        case T::RESERVED:
            r.transitionEffect.set<T::RESERVED>({});
            break;
        }
        return r;
    }

    ISOBMFF::ViewingOrientation readViewingOrientation(const ConfigValue& aValue)
    {
        ISOBMFF::ViewingOrientation r;
        static std::map<std::string, ISOBMFF::ViewingOrientationMode> mapping{
            {"default", ISOBMFF::ViewingOrientationMode::DEFAULT},
            {"viewport", ISOBMFF::ViewingOrientationMode::VIEWPORT},
            {"no_influence", ISOBMFF::ViewingOrientationMode::NO_INFLUENCE}};
        switch (readMapping("viewing orientation mode", mapping)(aValue["mode"]))
        {
        case ISOBMFF::ViewingOrientationMode::DEFAULT:
            r.set<ISOBMFF::ViewingOrientationMode::DEFAULT>({});
            break;
        case ISOBMFF::ViewingOrientationMode::VIEWPORT:
            r.set<ISOBMFF::ViewingOrientationMode::VIEWPORT>(
                readSphereRegionStatic<false, false>(aValue["sphere_region"]));
            break;
        case ISOBMFF::ViewingOrientationMode::NO_INFLUENCE:
            r.set<ISOBMFF::ViewingOrientationMode::NO_INFLUENCE>({});
            break;
        case ISOBMFF::ViewingOrientationMode::RESERVED:
            // basically let's satisfy compiler warnings
            r.set<ISOBMFF::ViewingOrientationMode::RESERVED>({});
            break;
        }
        return r;
    }

    // json schema: OMAFViewpointSwitchRegionStruct
    ISOBMFF::ViewpointSwitchRegionStruct readViewpointSwitchRegionStruct(const ConfigValue& aValue)
    {
        bool isSet = false;
        ISOBMFF::ViewpointSwitchRegionStruct r{};
        const char* errorMessage =
            "Exactly one of \"viewport_relative\", \"sphere_relative\" or \"ref_overlay_id\" "
            "must be set for viewpoint_switch_region";

        if (auto viewportRelative = readOptional(readViewportRelative)(aValue["viewport_relative"]))
        {
            r.region.set<ISOBMFF::ViewpointRegionType::VIEWPORT_RELATIVE>(*viewportRelative);
            isSet = true;
        }

        if (auto sphereRelative =
                readOptional(readSphereRelativePosition)(aValue["sphere_relative"]))
        {
            if (isSet)
            {
                throw ConfigValueInvalid(errorMessage, aValue);
            }
            r.region.set<ISOBMFF::ViewpointRegionType::SPHERE_RELATIVE>(*sphereRelative);
            isSet = true;
        }

        if (auto refOverlayId = readOptional(readUint16)(aValue["ref_overlay_id"]))
        {
            if (isSet)
            {
                throw ConfigValueInvalid(errorMessage, aValue);
            }
            r.region.set<ISOBMFF::ViewpointRegionType::OVERLAY>(*refOverlayId);
            isSet = true;
        }

        if (isSet)
        {
            return r;
        }
        else
        {
            throw ConfigValueInvalid(errorMessage, aValue);
        }
    }

    ISOBMFF::OneViewpointSwitchingStruct readOneViewpointSwitchingStruct(const ConfigValue& aValue)
    {
        ISOBMFF::OneViewpointSwitchingStruct r{};
        r.viewingOrientation = readViewingOrientation(aValue["viewing_orientation"]);
        r.destinationViewpointId = readUint16(aValue["destination_viewpoint_id"]);
        r.viewpointTimelineSwitch =
            readOptional(readViewpointTimelineSwitchStruct)(aValue["viewpoint_timeline_switch"]);
        r.transitionEffect = readOptional(readTransitionEffect)(aValue["transition_effect"]);
        r.viewpointSwitchRegions = makeDynArray(
            readOptional(readList("viewpoint_switch_regions", readViewpointSwitchRegionStruct))(
                aValue["viewpoint_switch_regions"])
                .value_or(std::list<ISOBMFF::ViewpointSwitchRegionStruct>()));
        return r;
    }

    ISOBMFF::ViewpointSwitchingListStruct readViewpointSwitchingListStruct(
        const ConfigValue& aValue)
    {
        ISOBMFF::ViewpointSwitchingListStruct r{};
        r.viewpointSwitching =
            makeDynArray(readList("viewpoint switching", readOneViewpointSwitchingStruct)(aValue));
        return r;
    }

    ISOBMFF::ViewpointLoopingStruct readViewpointLoopingStruct(const ConfigValue& aValue)
    {
        ISOBMFF::ViewpointLoopingStruct r{};
        r.maxLoops = readOptional(readInt8)(aValue["max_loops"]);
        r.loopActivationTime = readOptional(readInt32)(aValue["loop_activation_time"]);
        r.loopStartTime = readOptional(readInt32)(aValue["loop_start_time"]);
        r.loopExitStruct = readOptional(readViewpointSwitchingListStruct)(aValue["loop_exit_struct"]);
        return r;
    }

    ISOBMFF::DynamicViewpointSampleEntry readDynamicViewpointSampleEntry(const ConfigValue& aValue)
    {
        ISOBMFF::DynamicViewpointSampleEntry r{};
        r.viewpointPosStruct = readOptional(readViewpointPosStruct)(aValue["position"]);
        r.viewpointGlobalCoordinateSysRotationStruct =
            readOptional(readViewpointGlobalCoordinateSysRotationStruct)(
                aValue["global_coordinate_sys_rotation"]);
        r.dynamicVwptGroupFlag = readOptional(readBool)(aValue["dynamic_vwpt_group_flag"]).value_or(false);
        r.viewpointGpsPositionStruct =
            readOptional(readViewpointGpsPositionStruct)(aValue["gps_position"]);
        r.viewpointGeomagneticInfoStruct =
            readOptional(readViewpointGeomagneticInfoStruct)(aValue["geomagnetic_info"]);
        r.viewpointSwitchingListStruct =
            readOptional(readViewpointSwitchingListStruct)(aValue["viewpoint_switching"]);
        return r;
    }

    ViewpointInfo::ViewpointInfo(const ConfigValue& aValue)
    {
        ISOBMFF::DynamicViewpointSampleEntry& r = mDynamicViewpointSampleEntry;
        r.viewpointPosStruct = readOptional(readViewpointPosStruct)(aValue["position"]);
        r.viewpointGlobalCoordinateSysRotationStruct =
            readOptional(readViewpointGlobalCoordinateSysRotationStruct)(
                aValue["global_coordinate_sys_rotation"]);
        r.dynamicVwptGroupFlag = readBool(aValue["dynamic_vwpt_group_flag"]);
        r.viewpointGpsPositionStruct =
            readOptional(readViewpointGpsPositionStruct)(aValue["gps_position"]);
        r.viewpointGeomagneticInfoStruct =
            readOptional(readViewpointGeomagneticInfoStruct)(aValue["geomagnetic_info"]);
        r.viewpointSwitchingListStruct =
            readOptional(readViewpointSwitchingListStruct)(aValue["viewpoint_switching"]);
        mViewpointLooping =
            readOptional(readViewpointLoopingStruct)(aValue["viewpoint_looping"]);
    }

    ISOBMFF::DynamicViewpointSampleEntry ViewpointInfo::getDyvpSampleEntry() const
    {
        return mDynamicViewpointSampleEntry;
    }

    StreamSegmenter::Segmenter::VipoEntityGroupSpec ViewpointInfo::getVipoEntityGroupSpec(
        std::string aLabel, ViewpointGroupId aViewpointGroupId) const
    {
        StreamSegmenter::Segmenter::VipoEntityGroupSpec vp{};

        vp.viewpointGroup.vwptGroupId = aViewpointGroupId.get();

        if (aLabel.size())
        {
            vp.viewpointGroup.vwptGroupDescription = {&aLabel[0], (&aLabel[0] + aLabel.size())};
        }
        if (mDynamicViewpointSampleEntry.viewpointPosStruct)
        {
            vp.viewpointPos = *mDynamicViewpointSampleEntry.viewpointPosStruct;
        }
        vp.viewpointGpsPosition = mDynamicViewpointSampleEntry.viewpointGpsPositionStruct;
        vp.viewpointGeomagneticInfo = mDynamicViewpointSampleEntry.viewpointGeomagneticInfoStruct;

        if (mDynamicViewpointSampleEntry.viewpointGlobalCoordinateSysRotationStruct)
        {
            vp.viewpointGlobalCoordinateSysRotation =
                *mDynamicViewpointSampleEntry.viewpointGlobalCoordinateSysRotationStruct;
        }
        vp.viewpointSwitchingList = mDynamicViewpointSampleEntry.viewpointSwitchingListStruct;
        vp.viewpointLooping = mViewpointLooping;

        return vp;
    }

    StreamSegmenter::MPDTree::Omaf2Viewpoint ViewpointInfo::getOmaf2Viewpoint(
        std::string aId, std::string aLabel, ViewpointGroupId aViewpointGroupId,
        bool aIsInitial) const
    {
        StreamSegmenter::MPDTree::Omaf2Viewpoint vp{};
        vp.id = aId;
        vp.viewpointInfo.label = aLabel;
        StreamSegmenter::MPDTree::Omaf2ViewpointInfo& vpi = vp.viewpointInfo;
        if (auto& vps = mDynamicViewpointSampleEntry.viewpointPosStruct)
        {
            vpi.position.x = vps->viewpointPosX;
            vpi.position.y = vps->viewpointPosY;
            vpi.position.z = vps->viewpointPosZ;
        }

        vp.viewpointInfo.groupInfo.groupId = aViewpointGroupId.get();
        vp.viewpointInfo.initialViewpoint = aIsInitial ? true : Optional<bool>();
        return vp;
    }

    ViewpointInfo readViewpointInfo(const ConfigValue& aValue)
    {
        return ViewpointInfo(aValue);
    }

    ISOBMFF::DynamicViewpointGroup readDynamicViewpointGroup(const ConfigValue& aValue)
    {
        ISOBMFF::DynamicViewpointGroup r{};
        r.viewpointGroupStruct = readOptional(readViewpointGroupStruct)(aValue);
        return r;
    }

    auto readDynamicViewpointSample(ISOBMFF::DynamicViewpointSampleEntry aSampleEntry)
        -> std::function<ISOBMFF::DynamicViewpointSample(const ConfigValue& aValue)>
    {
        return [=](const ConfigValue& aValue) {
            ISOBMFF::DynamicViewpointSample r{};
            r.viewpointPosStruct = readViewpointPosStruct(aValue["position"]);
            r.viewpointGpsPositionStruct =
                !aSampleEntry.viewpointGpsPositionStruct
                    ? makeOptional(readViewpointGpsPositionStruct(aValue["gps_position"]))
                    : Optional<ISOBMFF::ViewpointGpsPositionStruct>();
            r.viewpointGeomagneticInfoStruct =
                !aSampleEntry.viewpointGeomagneticInfoStruct
                    ? makeOptional(readViewpointGeomagneticInfoStruct(aValue["geomagnetic_info"]))
                    : Optional<ISOBMFF::ViewpointGeomagneticInfoStruct>();
            r.viewpointGlobalCoordinateSysRotationStruct =
                !aSampleEntry.viewpointGlobalCoordinateSysRotationStruct
                    ? makeOptional(readViewpointGlobalCoordinateSysRotationStruct(
                          aValue["global_coordinate_sys_rotation"]))
                    : Optional<ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct>();
            auto dynamicViewpointGroup = aValue["viewpoint_group"];
            if (aSampleEntry.dynamicVwptGroupFlag)
            {
                r.dynamicViewpointGroup = readDynamicViewpointGroup(dynamicViewpointGroup);
            }
            else if (dynamicViewpointGroup)
            {
                throw ConfigValueInvalid(
                    "Cannot set viewpoint_group unless dynamic_vwpt_group_flag is set "
                    "in the sample entry",
                    dynamicViewpointGroup);
            }

            r.viewpointSwitchingListStruct =
                readOptional(readViewpointSwitchingListStruct)(aValue["viewpoint_switching"]);
            return r;
        };
    }

    std::string readViewpointLabel(const ConfigValue& aConfig)
    {
        return readString(aConfig);
    }

    ISOBMFF::ViewpointGroupStruct<true> readViewpointGroupStruct(const ConfigValue& aValue)
    {
        ISOBMFF::ViewpointGroupStruct<true> r{};
        r.vwptGroupId = readUint8(aValue["id"]);
        r.vwptGroupDescription = readOptional(readDynArrayString)(aValue["description"]).value_or({});
        return r;
    }

    ISOBMFF::SphereRegionRange readSphereRegionRange(const ConfigValue& aValue)
    {
        ISOBMFF::SphereRegionRange r{};
        r.azimuthRange = readFloatUint360Degrees(kStepsInDegree)(aValue["azimuth_range_degrees"]);
        r.elevationRange =
            readFloatUint180Degrees(kStepsInDegree)(aValue["elevation_range_degrees"]);
        return r;
    }

    ISOBMFF::SphereRegionConfigStruct readSphereRegionConfig(const ConfigValue& aValue)
    {
        ISOBMFF::SphereRegionConfigStruct r{};
        r.shapeType = readSphereRegionShapeType(aValue["shape_type"]);
        r.staticRange = readOptional(readSphereRegionRange)(aValue["static_range"]);
        return r;
    }

    ISOBMFF::RecommendedViewportInfoStruct readRecommendedViewportInfo(const ConfigValue& aValue)
    {
        ISOBMFF::RecommendedViewportInfoStruct r{};

        r.type = readUint8(aValue["type"]);
        r.description = readDynArrayString(aValue["description"]);

        return r;
    }

    EntityGroupType readEntityGroupType(const ConfigValue& aValue)
    {
        static std::map<std::string, EntityGroupType> mapping{
            {"altr", EntityGroupType::Altr},
            {"ovbg", EntityGroupType::Ovbg},
            {"oval", EntityGroupType::Oval},
            {"vipo", EntityGroupType::Vipo}
        };

        return readMapping("entity group type", mapping)(aValue);
    }

    void mergeLabelEntityIdMapping(LabelEntityIdMapping& aTo,
                                   const LabelEntityIdMapping& aFrom)
    {
        for (auto& keyValue: aFrom.forward)
        {
            const ParsedValue<RefIdLabel>& key = keyValue.first;
            if (aTo.forward.count(key))
            {
                throw ConfigConflictError("Duplicate use of entity id " + key->get() + " at ",
                                          aTo.forward.find(key)->first.getConfigValue(),
                                          key.getConfigValue());
            }
            aTo.addLabel(keyValue.second);
        }
    }

    ParsedValue<RefIdLabel> readRefIdLabel(const ConfigValue& aNode)
    {
        return ParsedValue<RefIdLabel>(aNode, [](const ConfigValue& aNode) {  //
            return RefIdLabel(readString(aNode));
        });
    }

    TrackReferences readTrackReferences(const ConfigValue& aNode)
    {
        TrackReferences r;
        if (aNode->type() != Json::objectValue)
        {
            throw ConfigKeyNotObject(aNode.getPath());
        }
        for (auto node : aNode.childValues())
        {
            auto fourcc = node.getName();
            if (fourcc.size() != 4)
            {
                throw ConfigValueInvalid("Key must be a fourcc, " + fourcc + " given", aNode);
            }
            auto references = readList("list of track references", readRefIdLabel)(node);
            r[fourcc] = references;
        }
        return r;
    }

    auto readEntityIdReference(const EntityGroupReadContext& aReadContext)
        -> std::function<EntityIdReference(const ConfigValue& aValue)>
    {
        return [&](const ConfigValue& aValue) {
            auto name = readRefIdLabel(aValue);
            if (aReadContext.labelEntityIdMapping.forward.count(name))
            {
                return aReadContext.labelEntityIdMapping.forward.at(name);
            }
            else
            {
                throw ConfigValueInvalid(
                    "Entity id " + name->get() + " does not refer to an existing entity", aValue);
            }
        };
    }

    namespace
    {
        void fillInEntityGroupToSpecCommon(const EntityGroupReadContext& /* aReadContext */,
                                           StreamSegmenter::Segmenter::EntityToGroupSpec& aCommon,
                                           const ConfigValue& aValue)
        {
            // Don't allocate dynamically a new group id:
            // aCommon.groupId = aReadContext.allocateEntityGroupId().get();

            // instead use user-provided ones:
            aCommon.groupId = readUint32(aValue["group_id"]);

        }

        StreamSegmenter::Segmenter::AltrEntityGroupSpec
            readEntityGroupSpecAltr(const EntityGroupReadContext& aReadContext,
                                    const ConfigValue& aValue)
        {
            StreamSegmenter::Segmenter::AltrEntityGroupSpec r;
            fillInEntityGroupToSpecCommon(aReadContext, r, aValue);

            using Item = EntityIdReference;

            // use the same structure as with others
            auto readItem = [&](const ConfigValue& aValue) -> Item {
                Item item{};
                item = readEntityIdReference(aReadContext)(aValue["entity_id"]);
                return item;
            };

            for (const auto& entity : readList("entities", readItem)(aValue["items"]))
            {
                for (auto& reference: entity.references) {
                    r.entityIds.push_back(reference.entityId);
                }
            }

            return r;
        }

        StreamSegmenter::Segmenter::OvbgEntityGroupSpec readEntityGroupSpecOvbg(
            const EntityGroupReadContext& aReadContext, const ConfigValue& aValue)
        {
            StreamSegmenter::Segmenter::OvbgEntityGroupSpec r;
            fillInEntityGroupToSpecCommon(aReadContext, r, aValue);

            r.sphereDistanceInMm = readUint32(aValue["unit_sphere_distance_in_mm"]);

            using Item = std::pair<EntityIdReference, StreamSegmenter::Segmenter::OvbgGroupFlags>;

            auto readItem = [&](const ConfigValue& aValue) -> Item {
                Item item{};
                item.first = readEntityIdReference(aReadContext)(aValue["entity_id"]);
                item.second.overlayFlag = readBool(aValue["overlay_flag"]);
                item.second.backgroundFlag = readBool(aValue["background_flag"]);
                item.second.overlaySubset = readOptional(readVector("overlay subset", readUint32))(aValue["ovbg_overlay_ids"]);
                if (auto numOverlayIds = readOptional(readSize)(aValue["ovbg_num_overlays"]))
                {
                    if (*numOverlayIds !=
                        (item.second.overlaySubset ? item.second.overlaySubset->size() : size_t(0)))
                    {
                        throw ConfigValueInvalid(
                            "ovbg_num_overlays is optional, but if given, it must match the number "
                            "of ovbg_overlay_ids",
                            aValue["ovbg_num_overlays"]);
                    }
                }
                return item;
            };

            for (const auto& entity : readList("entities", readItem)(aValue["items"]))
            {
                for (auto& reference: entity.first.references) {
                    r.entityIds.push_back(reference.entityId);
                    r.entityFlags.push_back(entity.second);
                }
            }

            return r;
        }

        StreamSegmenter::Segmenter::OvalEntityGroupSpec readEntityGroupSpecOval(
            const EntityGroupReadContext& aReadContext, const ConfigValue& aValue)
        {
            StreamSegmenter::Segmenter::OvalEntityGroupSpec r;
            fillInEntityGroupToSpecCommon(aReadContext, r, aValue);

            using Item = std::pair<EntityIdReference, std::uint16_t>;

            auto readItem = [&](const ConfigValue& aValue) -> Item {
                Item item{};
                item.first = readEntityIdReference(aReadContext)(aValue["entity_id"]);
                item.second = readUint16(aValue["ref_overlay_id"]);
                return item;
            };

            for (const auto& entity : readList("entities", readItem)(aValue["items"]))
            {
                for (auto& reference: entity.first.references) {
                    r.entityIds.push_back(reference.entityId);
                    r.refOverlayIds.push_back(entity.second);
                }
            }

            return r;
        }

        StreamSegmenter::Segmenter::VipoEntityGroupSpec readEntityGroupSpecVipo(
            const EntityGroupReadContext& aReadContext, const ConfigValue& aValue)
        {
            StreamSegmenter::Segmenter::VipoEntityGroupSpec r;
            fillInEntityGroupToSpecCommon(aReadContext, r, aValue);

            r.viewpointId = readUint32(aValue["group_id"]);
            r.viewpointLabel = readString(aValue["viewpoint_label"]);
            r.viewpointPos = readViewpointPosStruct(aValue["position"]);
            r.viewpointGroup = readViewpointGroupStruct(aValue["viewpoint_group"]);
            r.viewpointGlobalCoordinateSysRotation = readViewpointGlobalCoordinateSysRotationStruct(
                aValue["global_coordinate_sys_rotation"]);
            r.viewpointGpsPosition =
                readOptional(readViewpointGpsPositionStruct)(aValue["gps_position"]);
            r.viewpointGeomagneticInfo =
                readOptional(readViewpointGeomagneticInfoStruct)(aValue["geomagnetic_info"]);
            r.viewpointSwitchingList =
                readOptional(readViewpointSwitchingListStruct)(aValue["viewpoint_switching"]);
            r.viewpointLooping =
                readOptional(readViewpointLoopingStruct)(aValue["viewpoint_looping"]);

            using Item = EntityIdReference;

            auto readItem = [&](const ConfigValue& aValue) -> Item {
                Item item{};
                item = readEntityIdReference(aReadContext)(aValue["entity_id"]);
                return item;
            };

            for (const auto& entity : readList("entities", readItem)(aValue["items"]))
            {
                for (auto& reference: entity.references) {
                    r.entityIds.push_back(reference.entityId);
                }
            }

            return r;
        }

        // written in a funky way to co-operate easily with readList
        auto readEntityGroup(const EntityGroupReadContext& aReadContext)
            -> std::function<EntityGroupSpec(const ConfigValue& aValue)>
        {
            return [&](const ConfigValue& aValue) {
                EntityGroupSpec spec;

                switch (readEntityGroupType(aValue["type"]))
                {
                case EntityGroupType::Altr:
                    spec.set<EntityGroupType::Altr>(readEntityGroupSpecAltr(aReadContext, aValue));
                    break;
                case EntityGroupType::Ovbg:
                    spec.set<EntityGroupType::Ovbg>(readEntityGroupSpecOvbg(aReadContext, aValue));
                    break;
                case EntityGroupType::Oval:
                    spec.set<EntityGroupType::Oval>(readEntityGroupSpecOval(aReadContext, aValue));
                    break;
                case EntityGroupType::Vipo:
                    spec.set<EntityGroupType::Vipo>(readEntityGroupSpecVipo(aReadContext, aValue));
                    break;
                }

                return spec;
            };
        }
    }  // namespace

    auto readEntityGroups(const EntityGroupReadContext& aReadContext)
        -> std::function<StreamSegmenter::Segmenter::MetaSpec(const ConfigValue& aValue)>
    {
        return [&](const ConfigValue& aValue) {
            // ISOBMFF::Optional<const StreamSegmenter::Segmenter::MetaSpec> r;
            StreamSegmenter::Segmenter::MetaSpec r{};

            for (const auto& group :
                 readList("entity groups", readEntityGroup(aReadContext))(aValue))
            {
                switch (group.getKey())
                {
                case EntityGroupType::Altr:
                    r.altrGroups.push_back(group.at<EntityGroupType::Altr>());
                    break;
                case EntityGroupType::Ovbg:
                    r.ovbgGroups.push_back(group.at<EntityGroupType::Ovbg>());
                    break;
                case EntityGroupType::Oval:
                    r.ovalGroups.push_back(group.at<EntityGroupType::Oval>());
                    break;
                case EntityGroupType::Vipo:
                    r.vipoGroups.push_back(group.at<EntityGroupType::Vipo>());
                    break;
                }
            }

            return r;
        };
    }

    ISOBMFF::AssociatedSphereRegion readISOBMFFAssociatedSphereRegion(const ConfigValue& aValue)
    {
        ISOBMFF::AssociatedSphereRegion r{};
        if (readToOverlayControlFlagBase(r, aValue))
        {
            return r;
        }
        r.shapeType = readSphereRegionShapeType(aValue["shape_type"]);
        r.sphereRegion = readSphereRegionStatic</* HasRange */ true, /* HasInterpolate */ true>(aValue["region"]);
        return r;
    }

    namespace
    {
        ISOBMFF::ViewportRelativeOverlay readISOBMFFViewportRelativeOverlay(const ConfigValue& aValue)
        {
            ISOBMFF::ViewportRelativeOverlay r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.rectLeftPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_left_percent"]);
            r.rectTopPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_top_percent"]);
            r.rectWidthtPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_width_percent"]);
            r.rectHeightPercent = readFloatPercentAsFP<uint16_t>(aValue["rect_height_percent"]);

            r.relativeDisparityFlag =
                readOptional(readBool)(aValue["relative_disparity_flag"]).value_or(false);
            r.mediaAlignment = readOptional(readMediaAlignmentType)(aValue["media_alignment"])
                                   .value_or(ISOBMFF::MediaAlignmentType::STRETCH_TO_FILL);
            r.disparity = readFloatPercentAsFP<int16_t>(aValue["disparity_percent"]);

            return r;
        }

        ISOBMFF::SphereRelativeOmniOverlay readISOBMFFSphereRelativeOmniOverlay(const ConfigValue& aValue)
        {
            ISOBMFF::SphereRelativeOmniOverlay r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.timelineChangeFlag = readOptional(readBool)(aValue["timeline_change_flag"]).value_or(false);
            if (aValue["sphere_region"])
            {
                r.region.set<ISOBMFF::RegionIndicationType::SPHERE>(
                    readSphereRegionStatic<true, true>(aValue["sphere_region"]));
            }
            else
            {
                r.region.set<ISOBMFF::RegionIndicationType::PROJECTED_PICTURE>(
                    readISOBMFFProjectedPictureRegion(aValue["projected_picture"]));
            }
            r.regionDepthMinus1 = readRegiondDepthMinus1FromPercent(aValue["region_depth_percent"]);

            return r;
        }

        ISOBMFF::Rotation readRotation(const ConfigValue& aValue)
        {
            ISOBMFF::Rotation r{};
            r.yaw = readFloatSigned180Degrees(kStepsInDegree)(aValue["yaw_degrees"]);
            r.pitch = readFloatSigned90Degrees(kStepsInDegree)(aValue["pitch_degrees"]);
            r.roll = readFloatSigned180Degrees(kStepsInDegree)(aValue["roll_degrees"]);
            return r;
        }

        ISOBMFF::SphereRelative2DOverlay readISOBMFFSphereRelative2DOverlay(const ConfigValue& aValue)
        {
            ISOBMFF::SphereRelative2DOverlay r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.timelineChangeFlag = readOptional(readBool)(aValue["timeline_change_flag"]).value_or(false);
            r.sphereRegion = readSphereRegionStatic<true, true>(aValue);
            r.overlayRotation = readRotation(aValue);
            r.regionDepthMinus1 = readRegiondDepthMinus1FromPercent(aValue["region_depth_percent"]);
            return r;
        }

        ISOBMFF::OverlaySourceRegion readISOBMFFOverlaySourceRegion(const ConfigValue& aValue)
        {
            ISOBMFF::OverlaySourceRegion r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.region = readISOBMFFPackedPictureRegion(aValue);
            r.transformType = readTransformType(aValue["transform_type"]);
            return r;
        }

        ISOBMFF::RecommendedViewportOverlay readISOBMFFRecommendedViewportOverlay(const ConfigValue& aValue)
        {
            ISOBMFF::RecommendedViewportOverlay r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            return r;
        }

        ISOBMFF::OverlayLayeringOrder readISOBMFFOverlayLayeringOrder(const ConfigValue& aValue)
        {
            ISOBMFF::OverlayLayeringOrder r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.layeringOrder = readInt16(aValue["layering_order"]);
            return r;
        }

        ISOBMFF::OverlayOpacity readISOBMFFOverlayOpacity(const ConfigValue& aValue)
        {
            ISOBMFF::OverlayOpacity r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.opacity = readUint8(aValue["opacity_percent"]);
            return r;
        }

        ISOBMFF::OverlayInteraction readISOBMFFOverlayInteraction(const ConfigValue& aValue)
        {
            ISOBMFF::OverlayInteraction r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.changePositionFlag = readBool(aValue["change_position_flag"]);
            r.changeDepthFlag = readBool(aValue["change_depth_flag"]);
            r.switchOnOffFlag = readBool(aValue["switch_on_off_flag"]);
            r.changeOpacityFlag = readBool(aValue["change_opacity_flag"]);
            r.resizeFlag = readBool(aValue["resize_flag"]);
            r.rotationFlag = readBool(aValue["rotation_flag"]);
            r.sourceSwitchingFlag = readBool(aValue["source_switching_flag"]);
            r.cropFlag = readBool(aValue["crop_flag"]);
            return r;
        }

        ISOBMFF::OverlayLabel readISOBMFFOverlayLabel(const ConfigValue& aValue)
        {
            ISOBMFF::OverlayLabel r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.overlayLabel = readString(aValue["overlay_label"]);
            return r;
        }

        ISOBMFF::OverlayPriority readISOBMFFOverlayPriority(const ConfigValue& aValue)
        {
            ISOBMFF::OverlayPriority r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.overlayPriority = readUint8(aValue["overlay_priority"]);
            return r;
        }

        ISOBMFF::AssociatedSphereRegion readISOBMFFAssociatedSphereRegion(const ConfigValue& aValue)
        {
            ISOBMFF::AssociatedSphereRegion r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.shapeType = readSphereRegionShapeType(aValue["shape_type"]);
            r.sphereRegion = readSphereRegionStatic<true, true>(aValue["region"]);
            return r;
        }

        ISOBMFF::OverlayAlphaCompositing readISOBMFFOverlayAlphaCompositing(
            const ConfigValue& aValue)
        {
            ISOBMFF::OverlayAlphaCompositing r{};
            if (readToOverlayControlFlagBase(r, aValue))
            {
                return r;
            }
            r.alphaBlendingMode = readAlphaBlendingModeType(aValue);
            return r;
        }

        ISOBMFF::SingleOverlayStruct readISOBMFFSingleOverlayStruct(const ConfigValue& aValue)
        {
            ISOBMFF::SingleOverlayStruct r;
            // cool, copy paste
            r.overlayId = readUint16(aValue["id"]);
            r.viewportRelativeOverlay =
                optionWithDefault(aValue, "viewport_relative", readISOBMFFViewportRelativeOverlay, {});
            r.sphereRelativeOmniOverlay = optionWithDefault(aValue, "sphere_relative_omni",
                                                            readISOBMFFSphereRelativeOmniOverlay, {});
            r.sphereRelative2DOverlay =
                optionWithDefault(aValue, "sphere_relative_2d", readISOBMFFSphereRelative2DOverlay, {});
            r.overlaySourceRegion =
                optionWithDefault(aValue, "overlay_source_region", readISOBMFFOverlaySourceRegion, {});
            r.recommendedViewportOverlay = optionWithDefault(aValue, "recommended_viewport_overlay",
                                                             readISOBMFFRecommendedViewportOverlay, {});
            r.overlayLayeringOrder =
                optionWithDefault(aValue, "overlay_layering_order", readISOBMFFOverlayLayeringOrder, {});
            r.overlayOpacity = optionWithDefault(aValue, "overlay_opacity", readISOBMFFOverlayOpacity, {});
            r.overlayInteraction =
                optionWithDefault(aValue, "overlay_interaction", readISOBMFFOverlayInteraction, {});
            r.overlayLabel = optionWithDefault(aValue, "overlay_label", readISOBMFFOverlayLabel, {});
            r.overlayPriority =
                optionWithDefault(aValue, "overlay_priority", readISOBMFFOverlayPriority, {});
            r.associatedSphereRegion = optionWithDefault(aValue, "associated_sphere_region",
                                                         readISOBMFFAssociatedSphereRegion, {});
            r.overlayAlphaCompositing = optionWithDefault(aValue, "overlay_alpha_compositing",
                                                          readISOBMFFOverlayAlphaCompositing, {});

            return r;
        }
    }

    ISOBMFF::OverlayStruct readOverlayStruct(const ConfigValue& aValue)
    {
        ISOBMFF::OverlayStruct r {};

        r.numFlagBytes = optionWithDefault(aValue, "num_flag_bytes", readUint8, 2);
        r.overlays = readVector("overlays", readISOBMFFSingleOverlayStruct)(aValue["overlays"]);

        return r;
    }

}  // namespace VDD
