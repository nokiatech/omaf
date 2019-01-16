
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
#include "mp4vrfilereaderutil.hpp"
#include "avcdecoderconfigrecord.hpp"
#include "cleanaperturebox.hpp"
#include "customallocator.hpp"
#include "hevcdecoderconfigrecord.hpp"

namespace MP4VR
{
    /* *********************************************************************** */
    /* ************************* Helper functions **************************** */
    /* *********************************************************************** */

    FourCCInt getRawItemType(const MetaBox& metaBox, const uint32_t itemId)
    {
        return metaBox.getItemInfoBox().getItemById(itemId).getItemType();
    }

    chnlPropertyInternal makeChnl(const ChannelLayoutBox& channelLayoutBox)
    {
        chnlPropertyInternal chnl{};
        chnl.streamStructure    = channelLayoutBox.getStreamStructure();
        chnl.definedLayout      = channelLayoutBox.getDefinedLayout();
        chnl.omittedChannelsMap = channelLayoutBox.getOmittedChannelsMap();
        chnl.objectCount        = channelLayoutBox.getObjectCount();
        chnl.channelCount       = channelLayoutBox.getChannelCount();
        chnl.channelLayouts.clear();

        if (chnl.channelCount > 0 && chnl.streamStructure == 1 && chnl.definedLayout == 0)
        {
            Vector<ChannelLayoutBox::ChannelLayout> layout = channelLayoutBox.getChannelLayouts();
            for (std::uint16_t i = 0; i < layout.size(); i++)
            {
                ChannelLayout channelPosition;
                channelPosition.speakerPosition = layout[i].speakerPosition;
                channelPosition.azimuth         = layout[i].azimuth;
                channelPosition.elevation       = layout[i].elevation;
                chnl.channelLayouts.push_back(channelPosition);
            }
        }

        return chnl;
    }

    SA3DPropertyInternal makeSA3D(const SpatialAudioBox& spatialAudioBox)
    {
        SA3DPropertyInternal sa3d{};
        sa3d.version                  = spatialAudioBox.getVersion();
        sa3d.ambisonicType            = spatialAudioBox.getAmbisonicType();
        sa3d.ambisonicOrder           = spatialAudioBox.getAmbisonicOrder();
        sa3d.ambisonicChannelOrdering = spatialAudioBox.getAmbisonicChannelOrdering();
        sa3d.ambisonicNormalization   = spatialAudioBox.getAmbisonicNormalization();
        sa3d.channelMap.clear();

        Vector<uint32_t> channelMap = spatialAudioBox.getChannelMap();
        for (std::uint16_t i = 0; i < channelMap.size(); i++)
        {
            sa3d.channelMap.push_back(channelMap.at(i));
        }

        return sa3d;
    }

    StereoScopic3DProperty makest3d(const Stereoscopic3D* stereoscopic3DBox)
    {
        StereoScopic3DProperty st3d(StereoScopic3DProperty::MONOSCOPIC);
        if (stereoscopic3DBox != nullptr)
        {
            Stereoscopic3D::V2StereoMode v2stereomode = stereoscopic3DBox->getStereoMode();
            if (v2stereomode == Stereoscopic3D::V2StereoMode::STEREOSCOPIC_TOPBOTTOM)
            {
                st3d = StereoScopic3DProperty::STEREOSCOPIC_TOP_BOTTOM;
            }
            else if (v2stereomode == Stereoscopic3D::V2StereoMode::STEREOSCOPIC_LEFTRIGHT)
            {
                st3d = StereoScopic3DProperty::STEREOSCOPIC_LEFT_RIGHT;
            }
            else if (v2stereomode == Stereoscopic3D::V2StereoMode::STEREOSCOPIC_STEREOCUSTOM)
            {
                st3d = StereoScopic3DProperty::STEREOSCOPIC_STEREO_CUSTOM;
            }
            else
            {
                st3d = StereoScopic3DProperty::MONOSCOPIC;
            }
        }
        return st3d;
    }

    StereoScopic3DProperty makest3d(const SphericalVideoV1Box& stereoscopic3DBox)
    {
        StereoScopic3DProperty st3d(StereoScopic3DProperty::MONOSCOPIC);
        SphericalVideoV1Box::V1StereoMode v1stereomode = stereoscopic3DBox.getGlobalMetadata().stereoMode;
        if (v1stereomode == SphericalVideoV1Box::V1StereoMode::STEREOSCOPIC_TOP_BOTTOM)
        {
            st3d = StereoScopic3DProperty::STEREOSCOPIC_TOP_BOTTOM;
        }
        else if (v1stereomode == SphericalVideoV1Box::V1StereoMode::STEREOSCOPIC_LEFT_RIGHT)
        {
            st3d = StereoScopic3DProperty::STEREOSCOPIC_LEFT_RIGHT;
        }
        else  // set UNDEFINED also as MONOSCOPIC (default in spec).
        {
            st3d = StereoScopic3DProperty::MONOSCOPIC;
        }
        return st3d;
    }

    SphericalVideoV1Property makeSphericalVideoV1Property(const SphericalVideoV1Box& sphericalVideoBox)
    {
        SphericalVideoV1Property sphericalVideoV1Property{};
        sphericalVideoV1Property.spherical         = true;  // always true for V1.0
        sphericalVideoV1Property.stitched          = true;  // always true for V1.0
        sphericalVideoV1Property.projectionType    = ProjectionType::EQUIRECTANGULAR;
        sphericalVideoV1Property.sourceCount       = sphericalVideoBox.getGlobalMetadata().sourceCount;
        sphericalVideoV1Property.initialView.yawFP = sphericalVideoBox.getGlobalMetadata().initialViewHeadingDegrees
                                                     << 16;
        sphericalVideoV1Property.initialView.pitchFP = sphericalVideoBox.getGlobalMetadata().initialViewPitchDegrees
                                                       << 16;
        sphericalVideoV1Property.initialView.rollFP = sphericalVideoBox.getGlobalMetadata().initialViewRollDegrees
                                                      << 16;
        sphericalVideoV1Property.timestamp            = sphericalVideoBox.getGlobalMetadata().timestamp;
        sphericalVideoV1Property.fullPanoWidthPixels  = sphericalVideoBox.getGlobalMetadata().fullPanoWidthPixels;
        sphericalVideoV1Property.fullPanoHeightPixels = sphericalVideoBox.getGlobalMetadata().fullPanoHeightPixels;
        sphericalVideoV1Property.croppedAreaImageWidthPixels =
            sphericalVideoBox.getGlobalMetadata().croppedAreaImageWidthPixels;
        sphericalVideoV1Property.croppedAreaImageHeightPixels =
            sphericalVideoBox.getGlobalMetadata().croppedAreaImageHeightPixels;
        sphericalVideoV1Property.croppedAreaLeftPixels = sphericalVideoBox.getGlobalMetadata().croppedAreaLeftPixels;
        sphericalVideoV1Property.croppedAreaTopPixels  = sphericalVideoBox.getGlobalMetadata().croppedAreaTopPixels;
        return sphericalVideoV1Property;
    }

    SphericalVideoV2Property makesv3d(const SphericalVideoV2Box* sphericalVideoBox)
    {
        SphericalVideoV2Property sv3d{};
        if (sphericalVideoBox != nullptr)
        {
            sv3d.pose.yawFP   = sphericalVideoBox->getProjectionBox().getProjectionHeaderBox().getPose().yaw;
            sv3d.pose.pitchFP = sphericalVideoBox->getProjectionBox().getProjectionHeaderBox().getPose().pitch;
            sv3d.pose.rollFP  = sphericalVideoBox->getProjectionBox().getProjectionHeaderBox().getPose().roll;

            Projection::ProjectionType type = sphericalVideoBox->getProjectionBox().getProjectionType();
            if (type == Projection::ProjectionType::CUBEMAP)
            {
                sv3d.projectionType = ProjectionType::CUBEMAP;
                sv3d.projection.cubemap.layout =
                    sphericalVideoBox->getProjectionBox().getCubemapProjectionBox().getLayout();
                sv3d.projection.cubemap.padding =
                    sphericalVideoBox->getProjectionBox().getCubemapProjectionBox().getPadding();
            }
            else if (type == Projection::ProjectionType::EQUIRECTANGULAR)
            {
                sv3d.projectionType                         = ProjectionType::EQUIRECTANGULAR;
                sv3d.projection.equirectangular.boundsTopFP = sphericalVideoBox->getProjectionBox()
                                                                  .getEquirectangularProjectionBox()
                                                                  .getProjectionBounds()
                                                                  .top_0_32_FP;
                sv3d.projection.equirectangular.boundsBottomFP = sphericalVideoBox->getProjectionBox()
                                                                     .getEquirectangularProjectionBox()
                                                                     .getProjectionBounds()
                                                                     .bottom_0_32_FP;
                sv3d.projection.equirectangular.boundsLeftFP = sphericalVideoBox->getProjectionBox()
                                                                   .getEquirectangularProjectionBox()
                                                                   .getProjectionBounds()
                                                                   .left_0_32_FP;
                sv3d.projection.equirectangular.boundsRightFP = sphericalVideoBox->getProjectionBox()
                                                                    .getEquirectangularProjectionBox()
                                                                    .getProjectionBounds()
                                                                    .right_0_32_FP;
            }
            else if (type == Projection::ProjectionType::MESH)
            {
                sv3d.projectionType = ProjectionType::MESH;
            }
            else
            {
                sv3d.projectionType = ProjectionType::UNKOWN;
            }
        }
        return sv3d;
    }

    RegionWisePackingPropertyInternal makerwpk(const RegionWisePackingBox& rwpkBox)
    {
        RegionWisePackingPropertyInternal rwpkProperty{};
        rwpkProperty.constituentPictureMatchingFlag = rwpkBox.getConstituentPictureMatchingFlag();
        rwpkProperty.packedPictureHeight            = rwpkBox.getPackedPictureHeight();
        rwpkProperty.packedPictureWidth             = rwpkBox.getPackedPictureWidth();
        rwpkProperty.projPictureHeight              = rwpkBox.getProjPictureHeight();
        rwpkProperty.projPictureWidth               = rwpkBox.getProjPictureWidth();

        for (auto& boxRegion : rwpkBox.getRegions())
        {
            RegionWisePackingRegion region{};
            region.guardBandFlag = boxRegion->guardBandFlag;
            region.packingType   = (RegionWisePackingType) boxRegion->packingType;

            if (region.packingType == RegionWisePackingType::RECTANGULAR)
            {
                RectRegionPacking rectRegion{};

                rectRegion.projRegWidth         = boxRegion->rectangularPacking->projRegWidth;
                rectRegion.projRegHeight        = boxRegion->rectangularPacking->projRegHeight;
                rectRegion.projRegTop           = boxRegion->rectangularPacking->projRegTop;
                rectRegion.projRegLeft          = boxRegion->rectangularPacking->projRegLeft;
                rectRegion.transformType        = boxRegion->rectangularPacking->transformType;
                rectRegion.packedRegWidth       = boxRegion->rectangularPacking->packedRegWidth;
                rectRegion.packedRegHeight      = boxRegion->rectangularPacking->packedRegHeight;
                rectRegion.packedRegTop         = boxRegion->rectangularPacking->packedRegTop;
                rectRegion.packedRegLeft        = boxRegion->rectangularPacking->packedRegLeft;
                rectRegion.leftGbWidth          = boxRegion->rectangularPacking->leftGbWidth;
                rectRegion.rightGbWidth         = boxRegion->rectangularPacking->rightGbWidth;
                rectRegion.topGbHeight          = boxRegion->rectangularPacking->topGbHeight;
                rectRegion.bottomGbHeight       = boxRegion->rectangularPacking->bottomGbHeight;
                rectRegion.gbNotUsedForPredFlag = boxRegion->rectangularPacking->gbNotUsedForPredFlag;
                rectRegion.gbType0              = boxRegion->rectangularPacking->gbType0;
                rectRegion.gbType1              = boxRegion->rectangularPacking->gbType1;
                rectRegion.gbType2              = boxRegion->rectangularPacking->gbType2;
                rectRegion.gbType3              = boxRegion->rectangularPacking->gbType3;

                region.region.rectangular = rectRegion;
            }

            rwpkProperty.regions.push_back(region);
        }

        return rwpkProperty;
    }

    CoverageInformationPropertyInternal makecovi(const CoverageInformationBox& coviBox)
    {
        CoverageInformationPropertyInternal coviProperty{};
        coviProperty.coverageShapeType   = (CoverageShapeType) coviBox.getCoverageShapeType();
        coviProperty.defaultViewIdc      = (ViewIdc) coviBox.getDefaultViewIdc();
        coviProperty.viewIdcPresenceFlag = coviBox.getViewIdcPresenceFlag();

        for (auto& boxRegion : coviBox.getSphereRegions())
        {
            CoverageSphereRegion region{};
            region.viewIdc         = (ViewIdc) boxRegion->viewIdc;
            region.centreAzimuth   = boxRegion->region.centreAzimuth;
            region.centreElevation = boxRegion->region.centreElevation;
            region.centreTilt      = boxRegion->region.centreTilt;
            region.azimuthRange    = boxRegion->region.azimuthRange;
            region.elevationRange  = boxRegion->region.elevationRange;
            region.interpolate     = boxRegion->region.interpolate;

            coviProperty.sphereRegions.push_back(region);
        }

        return coviProperty;
    }

    bool isImageItemType(FourCCInt type)
    {
        static const Set<FourCCInt> IMAGE_TYPES = {"avc1", "hvc1", "hev1", "grid", "iovl", "iden"};

        return (IMAGE_TYPES.find(type) != IMAGE_TYPES.end());
    }


    bool doReferencesFromItemIdExist(const MetaBox& metaBox, const std::uint32_t itemId, FourCCInt referenceType)
    {
        const Vector<SingleItemTypeReferenceBox> references =
            metaBox.getItemReferenceBox().getReferencesOfType(referenceType);
        for (const auto& singleItemTypeReferenceBox : references)
        {
            if (singleItemTypeReferenceBox.getFromItemID() == itemId)
            {
                return true;
            }
        }
        return false;
    }


    bool doReferencesToItemIdExist(const MetaBox& metaBox, const std::uint32_t itemId, FourCCInt referenceType)
    {
        const Vector<SingleItemTypeReferenceBox> references =
            metaBox.getItemReferenceBox().getReferencesOfType(referenceType);
        for (const auto& singleItemTypeReferenceBox : references)
        {
            const Vector<uint32_t> toIds = singleItemTypeReferenceBox.getToItemIds();
            const auto id                = find(toIds.cbegin(), toIds.cend(), itemId);
            if (id != toIds.cend())
            {
                return true;
            }
        }
        return false;
    }

    ParameterSetMap makeDecoderParameterSetMap(const AvcDecoderConfigurationRecord& record)
    {
        Vector<uint8_t> sps;
        Vector<uint8_t> pps;
        record.getOneParameterSet(sps, AvcNalUnitType::SPS);
        record.getOneParameterSet(pps, AvcNalUnitType::PPS);

        ParameterSetMap parameterSetMap;
        parameterSetMap.insert(std::pair<DecSpecInfoType, DataVector>(DecSpecInfoType::AVC_SPS, std::move(sps)));
        parameterSetMap.insert(std::pair<DecSpecInfoType, DataVector>(DecSpecInfoType::AVC_PPS, std::move(pps)));

        return parameterSetMap;
    }

    ParameterSetMap makeDecoderParameterSetMap(const HevcDecoderConfigurationRecord& record)
    {
        Vector<uint8_t> sps;
        Vector<uint8_t> pps;
        Vector<uint8_t> vps;
        record.getOneParameterSet(sps, HevcNalUnitType::SPS);
        record.getOneParameterSet(pps, HevcNalUnitType::PPS);
        record.getOneParameterSet(vps, HevcNalUnitType::VPS);

        ParameterSetMap parameterSetMap;
        parameterSetMap.insert(std::pair<DecSpecInfoType, DataVector>(DecSpecInfoType::HEVC_SPS, std::move(sps)));
        parameterSetMap.insert(std::pair<DecSpecInfoType, DataVector>(DecSpecInfoType::HEVC_PPS, std::move(pps)));
        parameterSetMap.insert(std::pair<DecSpecInfoType, DataVector>(DecSpecInfoType::HEVC_VPS, std::move(vps)));

        return parameterSetMap;
    }

    ParameterSetMap makeDecoderParameterSetMap(const ElementaryStreamDescriptorBox& record)
    {
        Vector<uint8_t> decoderSpecInfo;
        ParameterSetMap parameterSetMap;
        if (record.getOneParameterSet(decoderSpecInfo))
        {
            // Only handle AudioSpecificConfig from 1.6.2.1 AudioSpecificConfig of ISO/IEC 14496-3:200X(E) subpart 1
            parameterSetMap.insert(std::pair<DecSpecInfoType, DataVector>(DecSpecInfoType::AudioSpecificConfig,
                                                                          std::move(decoderSpecInfo)));
        }
        return parameterSetMap;
    }

    bool isAnyLinkedToWithType(const TrackPropertiesMap& trackPropertiesMap, ContextId trackId, FourCCInt referenceType)
    {
        for (const auto& trackProperties : trackPropertiesMap)
        {
            for (const auto& reference : trackProperties.second.referenceTrackIds)
            {
                if (reference.first == referenceType)
                {
                    if (std::find(reference.second.begin(), reference.second.end(), trackId) != reference.second.end())
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

}  // namespace MP4VR
