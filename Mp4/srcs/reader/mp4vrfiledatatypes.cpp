
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
#define _SCL_SECURE_NO_WARNINGS

#include "api/reader/mp4vrfiledatatypes.h"

#include <algorithm>

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "dynarrayimpl.hpp"
#include "initialviewingorientationsampleentry.hpp"
#include "overlaysampleentrybox.hpp"

namespace ISOBMFF
{
    template struct DynArray<uint8_t>;
    template struct DynArray<char>;
    template struct DynArray<uint16_t>;
    template struct DynArray<uint32_t>;
    template struct DynArray<uint64_t>;

    template struct DynArray<MP4VR::DecoderSpecificInfo>;
    template struct DynArray<MP4VR::ChannelLayout>;
    template struct DynArray<MP4VR::FourCC>;
    template struct DynArray<MP4VR::TimestampIDPair>;
    template struct DynArray<MP4VR::TypeToTrackIDs>;
    template struct DynArray<MP4VR::SampleInformation>;
    template struct DynArray<MP4VR::TrackInformation>;
    template struct DynArray<MP4VR::SegmentInformation>;
    template struct DynArray<MP4VR::RegionWisePackingRegion>;
    template struct DynArray<MP4VR::CoverageSphereRegion>;
    template struct DynArray<MP4VR::SchemeType>;
    template struct DynArray<MP4VR::SingleOverlayProperty>;
    template struct DynArray<OneViewpointSwitchingStruct>;

    template struct DynArray<MP4VR::AltrEntityGroupProperty>;
    template struct DynArray<MP4VR::OvbgEntityGroupProperty>;
    template struct DynArray<MP4VR::OvalEntityGroupProperty>;
    template struct DynArray<MP4VR::VipoEntityGroupProperty>;

    template struct DynArray<MP4VR::OvbgGroupFlags>;

}  // namespace ISOBMFF

namespace MP4VR
{
    template <class T1, class T2>
    void copySphereRegion(T1& dst, const T2& src)
    {
        dst.centreAzimuth   = src.centreAzimuth;
        dst.centreElevation = src.centreElevation;
        dst.centreTilt      = src.centreTilt;

        dst.azimuthRange   = src.azimuthRange;
        dst.elevationRange = src.elevationRange;

        dst.interpolate = src.interpolate;
    }

    template <class T1, class T2>
    void copyPictureRegion(T1& dst, const T2& src)
    {
        dst.pictureWidth  = src.pictureWidth;
        dst.pictureHeight = src.pictureHeight;
        dst.regionWidth   = src.regionWidth;
        dst.regionHeight  = src.regionHeight;
        dst.regionTop     = src.regionTop;
        dst.regionLeft    = src.regionLeft;
    }

    template <class T1, class T2>
    void copyControlFlagBase(T1& dst, const T2& src)
    {
        dst.doesExist                           = src.doesExist;
        dst.overlayControlEssentialFlag         = src.overlayControlEssentialFlag;
        dst.inheritFromOverlayConfigSampleEntry = src.inheritFromOverlayConfigSampleEntry;
    }

    SphereRelativeOmniOverlay::SphereRelativeOmniOverlay()                                 = default;
    SphereRelativeOmniOverlay::SphereRelativeOmniOverlay(const SphereRelativeOmniOverlay&) = default;
    SphereRelativeOmniOverlay::SphereRelativeOmniOverlay(SphereRelativeOmniOverlay&&)      = default;
    SphereRelativeOmniOverlay& SphereRelativeOmniOverlay::operator=(const SphereRelativeOmniOverlay&) = default;
    SphereRelativeOmniOverlay& SphereRelativeOmniOverlay::operator=(SphereRelativeOmniOverlay&&) = default;
    SphereRelativeOmniOverlay::~SphereRelativeOmniOverlay()                                      = default;

    SphereRelative2DOverlay::SphereRelative2DOverlay()                               = default;
    SphereRelative2DOverlay::SphereRelative2DOverlay(const SphereRelative2DOverlay&) = default;
    SphereRelative2DOverlay::SphereRelative2DOverlay(SphereRelative2DOverlay&&)      = default;
    SphereRelative2DOverlay& SphereRelative2DOverlay::operator=(const SphereRelative2DOverlay&) = default;
    SphereRelative2DOverlay& SphereRelative2DOverlay::operator=(SphereRelative2DOverlay&&) = default;
    SphereRelative2DOverlay::~SphereRelative2DOverlay()                                    = default;

    OverlaySourceRegion::OverlaySourceRegion()                           = default;
    OverlaySourceRegion::OverlaySourceRegion(const OverlaySourceRegion&) = default;
    OverlaySourceRegion::OverlaySourceRegion(OverlaySourceRegion&&)      = default;
    OverlaySourceRegion& OverlaySourceRegion::operator=(const OverlaySourceRegion&) = default;
    OverlaySourceRegion& OverlaySourceRegion::operator=(OverlaySourceRegion&&) = default;
    OverlaySourceRegion::~OverlaySourceRegion()                                = default;

    RecommendedViewportOverlay::RecommendedViewportOverlay()                                  = default;
    RecommendedViewportOverlay::RecommendedViewportOverlay(const RecommendedViewportOverlay&) = default;
    RecommendedViewportOverlay::RecommendedViewportOverlay(RecommendedViewportOverlay&&)      = default;
    RecommendedViewportOverlay& RecommendedViewportOverlay::operator=(const RecommendedViewportOverlay&) = default;
    RecommendedViewportOverlay& RecommendedViewportOverlay::operator=(RecommendedViewportOverlay&&) = default;
    RecommendedViewportOverlay::~RecommendedViewportOverlay()                                       = default;

    OverlayLayeringOrder::OverlayLayeringOrder()                            = default;
    OverlayLayeringOrder::OverlayLayeringOrder(const OverlayLayeringOrder&) = default;
    OverlayLayeringOrder::OverlayLayeringOrder(OverlayLayeringOrder&&)      = default;
    OverlayLayeringOrder& OverlayLayeringOrder::operator=(const OverlayLayeringOrder&) = default;
    OverlayLayeringOrder& OverlayLayeringOrder::operator=(OverlayLayeringOrder&&) = default;
    OverlayLayeringOrder::~OverlayLayeringOrder()                                 = default;

    OverlayOpacity::OverlayOpacity()                      = default;
    OverlayOpacity::OverlayOpacity(const OverlayOpacity&) = default;
    OverlayOpacity::OverlayOpacity(OverlayOpacity&&)      = default;
    OverlayOpacity& OverlayOpacity::operator=(const OverlayOpacity&) = default;
    OverlayOpacity& OverlayOpacity::operator=(OverlayOpacity&&) = default;
    OverlayOpacity::~OverlayOpacity()                           = default;

    OverlayInteraction::OverlayInteraction()                          = default;
    OverlayInteraction::OverlayInteraction(const OverlayInteraction&) = default;
    OverlayInteraction::OverlayInteraction(OverlayInteraction&&)      = default;
    OverlayInteraction& OverlayInteraction::operator=(const OverlayInteraction&) = default;
    OverlayInteraction& OverlayInteraction::operator=(OverlayInteraction&&) = default;
    OverlayInteraction::~OverlayInteraction()                               = default;

    OverlayPriority::OverlayPriority()                       = default;
    OverlayPriority::OverlayPriority(const OverlayPriority&) = default;
    OverlayPriority::OverlayPriority(OverlayPriority&&)      = default;
    OverlayPriority& OverlayPriority::operator=(const OverlayPriority&) = default;
    OverlayPriority& OverlayPriority::operator=(OverlayPriority&&) = default;
    OverlayPriority::~OverlayPriority()                            = default;

    ViewportRelativeOverlay::ViewportRelativeOverlay()                               = default;
    ViewportRelativeOverlay::ViewportRelativeOverlay(const ViewportRelativeOverlay&) = default;
    ViewportRelativeOverlay::ViewportRelativeOverlay(ViewportRelativeOverlay&&)      = default;
    ViewportRelativeOverlay& ViewportRelativeOverlay::operator=(const ViewportRelativeOverlay&) = default;
    ViewportRelativeOverlay& ViewportRelativeOverlay::operator=(ViewportRelativeOverlay&&) = default;
    ViewportRelativeOverlay::~ViewportRelativeOverlay()                                    = default;

    AssociatedSphereRegion::AssociatedSphereRegion()                              = default;
    AssociatedSphereRegion::AssociatedSphereRegion(const AssociatedSphereRegion&) = default;
    AssociatedSphereRegion::AssociatedSphereRegion(AssociatedSphereRegion&&)      = default;
    AssociatedSphereRegion& AssociatedSphereRegion::operator=(const AssociatedSphereRegion&) = default;
    AssociatedSphereRegion& AssociatedSphereRegion::operator=(AssociatedSphereRegion&&) = default;
    AssociatedSphereRegion::~AssociatedSphereRegion()                                   = default;

    OverlayAlphaCompositing::OverlayAlphaCompositing()                               = default;
    OverlayAlphaCompositing::OverlayAlphaCompositing(const OverlayAlphaCompositing&) = default;
    OverlayAlphaCompositing::OverlayAlphaCompositing(OverlayAlphaCompositing&&)      = default;
    OverlayAlphaCompositing& OverlayAlphaCompositing::operator=(const OverlayAlphaCompositing&) = default;
    OverlayAlphaCompositing& OverlayAlphaCompositing::operator=(OverlayAlphaCompositing&&) = default;
    OverlayAlphaCompositing::~OverlayAlphaCompositing()                                    = default;


    SphereRegionWithRangeSample::SphereRegionWithRangeSample() = default;

    SphereRegionWithRangeSample::SphereRegionWithRangeSample(const char* frameData, uint32_t frameLen)
    {
        // create bitstream from frameData
        Vector<uint8_t> vec;
        vec.assign(frameData, frameData + frameLen);
        BitStream bitstr;
        bitstr.write8BitsArray(vec);

        // use reader code from common
        SphereRegionSampleEntryBox::SphereRegionWithRangeSample reader;
        reader.parse(bitstr);

        copySphereRegion(region, reader.regions[0]);
    }

    DynArray<uint8_t> SphereRegionWithRangeSample::toBytes() const
    {
        SphereRegionSampleEntryBox::SphereRegionWithRangeSample writer;
        copySphereRegion(writer.regions[0], region);

        BitStream bitstr;
        writer.write(bitstr);

        DynArray<uint8_t> result(bitstr.getStorage().size());
        std::copy(bitstr.getStorage().begin(), bitstr.getStorage().end(), result.begin());
        return result;
    }

    SphereRegionWithoutRangeSample::SphereRegionWithoutRangeSample() = default;

    SphereRegionWithoutRangeSample::SphereRegionWithoutRangeSample(const char* frameData, uint32_t frameLen)
    {
        // create bitstream from frameData
        Vector<uint8_t> vec;
        vec.assign(frameData, frameData + frameLen);
        BitStream bitstr;
        bitstr.write8BitsArray(vec);

        // use reader code from common
        SphereRegionSampleEntryBox::SphereRegionWithoutRangeSample reader;
        reader.parse(bitstr);

        region.centreAzimuth   = reader.regions[0].centreAzimuth;
        region.centreElevation = reader.regions[0].centreElevation;
        region.centreTilt      = reader.regions[0].centreTilt;
        region.interpolate     = reader.regions[0].interpolate;
    }

    DynArray<uint8_t> SphereRegionWithoutRangeSample::toBytes() const
    {
        SphereRegionSampleEntryBox::SphereRegionWithoutRangeSample writer;
        writer.regions[0].centreAzimuth   = region.centreAzimuth;
        writer.regions[0].centreElevation = region.centreElevation;
        writer.regions[0].centreTilt      = region.centreTilt;
        writer.regions[0].interpolate     = region.interpolate;

        BitStream bitstr;
        writer.write(bitstr);

        DynArray<uint8_t> result(bitstr.getStorage().size());
        std::copy(bitstr.getStorage().begin(), bitstr.getStorage().end(), result.begin());
        return result;
    }

    InitialViewingOrientationSample::InitialViewingOrientationSample() = default;

    InitialViewingOrientationSample::InitialViewingOrientationSample(const char* frameData, uint32_t frameLen)
    {
        // create bitstream from frameData
        Vector<uint8_t> vec;
        vec.assign(frameData, frameData + frameLen);
        BitStream bitstr;
        bitstr.write8BitsArray(vec);

        // use reader code from common
        InitialViewingOrientation::InitialViewingOrientationSample reader;
        reader.parse(bitstr);

        region.centreAzimuth   = reader.regions[0].centreAzimuth;
        region.centreElevation = reader.regions[0].centreElevation;
        region.centreTilt      = reader.regions[0].centreTilt;
        region.interpolate     = reader.regions[0].interpolate;
        refreshFlag            = reader.refreshFlag;
    }

    DynArray<uint8_t> InitialViewingOrientationSample::toBytes() const
    {
        InitialViewingOrientation::InitialViewingOrientationSample writer;
        writer.regions[0].centreAzimuth   = region.centreAzimuth;
        writer.regions[0].centreElevation = region.centreElevation;
        writer.regions[0].centreTilt      = region.centreTilt;
        writer.regions[0].interpolate     = region.interpolate;
        writer.refreshFlag                = refreshFlag;

        BitStream bitstr;
        writer.write(bitstr);

        DynArray<uint8_t> result(bitstr.getStorage().size());
        std::copy(bitstr.getStorage().begin(), bitstr.getStorage().end(), result.begin());
        return result;
    }

    void OverlayConfigProperty::readFromCommon(const OverlayStruct& ovlyStruct)
    {
        numFlagBytes = ovlyStruct.numFlagBytes;

        //       structs from common in this API without duplicating everything.

        overlays = DynArray<SingleOverlayProperty>(ovlyStruct.overlays.size());
        for (size_t i = 0; i < ovlyStruct.overlays.size(); i++)
        {
            auto& srcOverlay                 = ovlyStruct.overlays.at(i);
            SingleOverlayProperty dstOverlay = {};
            dstOverlay.overlayId             = srcOverlay.overlayId;

            // viewportRelativeOverlay
            copyControlFlagBase(dstOverlay.viewportRelativeOverlay, srcOverlay.viewportRelativeOverlay);

            dstOverlay.viewportRelativeOverlay.rectLeftPercent   = srcOverlay.viewportRelativeOverlay.rectLeftPercent;
            dstOverlay.viewportRelativeOverlay.rectTopPercent    = srcOverlay.viewportRelativeOverlay.rectTopPercent;
            dstOverlay.viewportRelativeOverlay.rectWidthtPercent = srcOverlay.viewportRelativeOverlay.rectWidthtPercent;
            dstOverlay.viewportRelativeOverlay.rectHeightPercent = srcOverlay.viewportRelativeOverlay.rectHeightPercent;

            dstOverlay.viewportRelativeOverlay.mediaAlignment = srcOverlay.viewportRelativeOverlay.mediaAlignment;

            dstOverlay.viewportRelativeOverlay.relativeDisparityFlag =
                srcOverlay.viewportRelativeOverlay.relativeDisparityFlag;

            dstOverlay.viewportRelativeOverlay.disparity = srcOverlay.viewportRelativeOverlay.disparity;

            // sphereRelativeOmniOverlay
            copyControlFlagBase(dstOverlay.sphereRelativeOmniOverlay, srcOverlay.sphereRelativeOmniOverlay);

            dstOverlay.sphereRelativeOmniOverlay.timelineChangeFlag =
                srcOverlay.sphereRelativeOmniOverlay.timelineChangeFlag;

            switch (srcOverlay.sphereRelativeOmniOverlay.region.getKey())
            {
            case ISOBMFF::RegionIndicationType::PROJECTED_PICTURE:
            {
                dstOverlay.sphereRelativeOmniOverlay.regionIndicationSphereRegion = false;
                auto& src =
                    srcOverlay.sphereRelativeOmniOverlay.region.at<ISOBMFF::RegionIndicationType::PROJECTED_PICTURE>();

                copyPictureRegion(dstOverlay.sphereRelativeOmniOverlay.region, src);
                break;
            }
            case ISOBMFF::RegionIndicationType::SPHERE:
            {
                dstOverlay.sphereRelativeOmniOverlay.regionIndicationSphereRegion = true;
                auto& src = srcOverlay.sphereRelativeOmniOverlay.region.at<ISOBMFF::RegionIndicationType::SPHERE>();
                copySphereRegion(dstOverlay.sphereRelativeOmniOverlay.sphereRegion, src);
                break;
            }
            }

            dstOverlay.sphereRelativeOmniOverlay.regionDepthMinus1 =
                srcOverlay.sphereRelativeOmniOverlay.regionDepthMinus1;

            // sphereRelative2DOverlay
            copyControlFlagBase(dstOverlay.sphereRelative2DOverlay, srcOverlay.sphereRelative2DOverlay);

            copySphereRegion(dstOverlay.sphereRelative2DOverlay.sphereRegion,
                             srcOverlay.sphereRelative2DOverlay.sphereRegion);

            dstOverlay.sphereRelative2DOverlay.timelineChangeFlag =
                srcOverlay.sphereRelative2DOverlay.timelineChangeFlag;

            dstOverlay.sphereRelative2DOverlay.overlayRotation.yaw =
                srcOverlay.sphereRelative2DOverlay.overlayRotation.yaw;
            dstOverlay.sphereRelative2DOverlay.overlayRotation.pitch =
                srcOverlay.sphereRelative2DOverlay.overlayRotation.pitch;
            dstOverlay.sphereRelative2DOverlay.overlayRotation.roll =
                srcOverlay.sphereRelative2DOverlay.overlayRotation.roll;

            dstOverlay.sphereRelative2DOverlay.regionDepthMinus1 = srcOverlay.sphereRelative2DOverlay.regionDepthMinus1;

            // overlaySourceRegion
            copyControlFlagBase(dstOverlay.overlaySourceRegion, srcOverlay.overlaySourceRegion);

            copyPictureRegion(dstOverlay.overlaySourceRegion.region, srcOverlay.overlaySourceRegion.region);

            dstOverlay.overlaySourceRegion.transformType = srcOverlay.overlaySourceRegion.transformType;

            // recommendedViewportOverlay
            copyControlFlagBase(dstOverlay.recommendedViewportOverlay, srcOverlay.recommendedViewportOverlay);

            // overlayLayeringOrder
            copyControlFlagBase(dstOverlay.overlayLayeringOrder, srcOverlay.overlayLayeringOrder);

            dstOverlay.overlayLayeringOrder.layeringOrder = srcOverlay.overlayLayeringOrder.layeringOrder;

            // overlayOpacity
            copyControlFlagBase(dstOverlay.overlayOpacity, srcOverlay.overlayOpacity);

            dstOverlay.overlayOpacity.opacity = srcOverlay.overlayOpacity.opacity;

            // overlayInteraction
            copyControlFlagBase(dstOverlay.overlayInteraction, srcOverlay.overlayInteraction);

            dstOverlay.overlayInteraction.changePositionFlag  = srcOverlay.overlayInteraction.changePositionFlag;
            dstOverlay.overlayInteraction.changeDepthFlag     = srcOverlay.overlayInteraction.changeDepthFlag;
            dstOverlay.overlayInteraction.switchOnOffFlag     = srcOverlay.overlayInteraction.switchOnOffFlag;
            dstOverlay.overlayInteraction.changeOpacityFlag   = srcOverlay.overlayInteraction.changeOpacityFlag;
            dstOverlay.overlayInteraction.resizeFlag          = srcOverlay.overlayInteraction.resizeFlag;
            dstOverlay.overlayInteraction.rotationFlag        = srcOverlay.overlayInteraction.rotationFlag;
            dstOverlay.overlayInteraction.sourceSwitchingFlag = srcOverlay.overlayInteraction.sourceSwitchingFlag;
            dstOverlay.overlayInteraction.cropFlag            = srcOverlay.overlayInteraction.cropFlag;

            // overlayLabel
            copyControlFlagBase(dstOverlay.overlayLabel, srcOverlay.overlayLabel);

            dstOverlay.overlayLabel.overlayLabel = DynArray<char>(srcOverlay.overlayLabel.overlayLabel.size());
            std::copy(srcOverlay.overlayLabel.overlayLabel.begin(), srcOverlay.overlayLabel.overlayLabel.end(),
                      dstOverlay.overlayLabel.overlayLabel.begin());

            // overlayPriority
            copyControlFlagBase(dstOverlay.overlayPriority, srcOverlay.overlayPriority);

            dstOverlay.overlayPriority.overlayPriority = srcOverlay.overlayPriority.overlayPriority;

            // associatedSphereRegion
            copyControlFlagBase(dstOverlay.associatedSphereRegion, srcOverlay.associatedSphereRegion);

            dstOverlay.associatedSphereRegion.shapeType = srcOverlay.associatedSphereRegion.shapeType;
            copySphereRegion(dstOverlay.associatedSphereRegion.sphereRegion,
                             srcOverlay.associatedSphereRegion.sphereRegion);

            // overlayAlphaCompositing
            copyControlFlagBase(dstOverlay.overlayAlphaCompositing, srcOverlay.overlayAlphaCompositing);

            dstOverlay.overlayAlphaCompositing.alphaBlendingMode = srcOverlay.overlayAlphaCompositing.alphaBlendingMode;

            overlays[i] = dstOverlay;
        }
    }

    OverlayLabel::OverlayLabel() = default;

    OverlayLabel::~OverlayLabel() = default;

    OverlayLabel::OverlayLabel(const OverlayLabel& other)
    {
        operator=(other);
    }

    OverlayLabel::OverlayLabel(OverlayLabel&& other)
        : OverlayControlFlagBase(other)
        , overlayLabel(std::move(other.overlayLabel))
    {
    }

    OverlayLabel& OverlayLabel::operator=(const OverlayLabel& other)
    {
        OverlayControlFlagBase::operator=(other);

        overlayLabel = other.overlayLabel;
        return *this;
    }

    SingleOverlayProperty::SingleOverlayProperty() = default;

    SingleOverlayProperty::~SingleOverlayProperty() = default;

    SingleOverlayProperty::SingleOverlayProperty(const SingleOverlayProperty& other)
    {
        operator=(other);
    }

    SingleOverlayProperty& SingleOverlayProperty::operator=(const SingleOverlayProperty& other)
    {
        overlayId                  = other.overlayId;
        viewportRelativeOverlay    = other.viewportRelativeOverlay;
        sphereRelativeOmniOverlay  = other.sphereRelativeOmniOverlay;
        sphereRelative2DOverlay    = other.sphereRelative2DOverlay;
        overlaySourceRegion        = other.overlaySourceRegion;
        recommendedViewportOverlay = other.recommendedViewportOverlay;
        overlayLayeringOrder       = other.overlayLayeringOrder;
        overlayOpacity             = other.overlayOpacity;
        overlayInteraction         = other.overlayInteraction;
        overlayLabel               = other.overlayLabel;
        overlayPriority            = other.overlayPriority;
        associatedSphereRegion     = other.associatedSphereRegion;
        overlayAlphaCompositing    = other.overlayAlphaCompositing;

        return *this;
    }

    SingleOverlayProperty::SingleOverlayProperty(SingleOverlayProperty&& other)
    {
        operator=(other);
    }

    SingleOverlayProperty& SingleOverlayProperty::operator=(SingleOverlayProperty&& other)
    {
        overlayId                  = std::move(other.overlayId);
        viewportRelativeOverlay    = std::move(other.viewportRelativeOverlay);
        sphereRelativeOmniOverlay  = std::move(other.sphereRelativeOmniOverlay);
        sphereRelative2DOverlay    = std::move(other.sphereRelative2DOverlay);
        overlaySourceRegion        = std::move(other.overlaySourceRegion);
        recommendedViewportOverlay = std::move(other.recommendedViewportOverlay);
        overlayLayeringOrder       = std::move(other.overlayLayeringOrder);
        overlayOpacity             = std::move(other.overlayOpacity);
        overlayInteraction         = std::move(other.overlayInteraction);
        overlayLabel               = std::move(other.overlayLabel);
        overlayPriority            = std::move(other.overlayPriority);
        associatedSphereRegion     = std::move(other.associatedSphereRegion);
        overlayAlphaCompositing    = std::move(other.overlayAlphaCompositing);

        return *this;
    }

    OverlayConfigSample::OverlayConfigSample()
    {
    }

    OverlayConfigSample::~OverlayConfigSample() = default;

    OverlayConfigSample::OverlayConfigSample(const char* frameData, uint32_t frameLen)
    {
        fromBytes(reinterpret_cast<const uint8_t*>(frameData), frameLen);
    }

    OverlayConfigSample::OverlayConfigSample(const OverlayConfigSample& other)
    {
        operator=(other);
    }

    OverlayConfigSample& OverlayConfigSample::operator=(const OverlayConfigSample& other)
    {
        auto bytes = other.toBytes();
        fromBytes(&bytes[0], (uint32_t) bytes.numElements);
        return *this;
    }

    void OverlayConfigSample::fromBytes(const uint8_t* frameData, uint32_t frameLen)
    {
        // create bitstream from frameData
        Vector<uint8_t> vec;
        vec.assign(frameData, frameData + frameLen);
        BitStream bitstr;
        bitstr.write8BitsArray(vec);

        // use reader code from common
        OverlaySampleEntryBox::OverlayConfigSample reader;
        reader.parse(bitstr);

        // read parsed data to mp4vr sturctures
        activeOverlayIds = DynArray<uint16_t>(reader.activeOverlayIds.size());
        for (size_t i = 0; i < reader.activeOverlayIds.size(); i++)
        {
            activeOverlayIds[i] = reader.activeOverlayIds[i];
        }

        if (auto& srcAddlOverlays = reader.addlActiveOverlays)
        {
            addlActiveOverlays = MP4VR::OverlayConfigProperty();
            addlActiveOverlays->readFromCommon(*srcAddlOverlays);
        }
        else
        {
            addlActiveOverlays = {};
        }
    }

    DynArray<uint8_t> OverlayConfigSample::toBytes() const
    {
        OverlaySampleEntryBox::OverlayConfigSample writer;
        BitStream bitstr;

        // copy everything again between internal / external structs
        writer.activeOverlayIds       = std::vector<uint16_t>(activeOverlayIds.begin(), activeOverlayIds.end());

        if (addlActiveOverlays)
        {
            writer.addlActiveOverlays = OverlayStruct();
            writer.addlActiveOverlays->numFlagBytes = addlActiveOverlays->numFlagBytes;

            for (auto& srcOverlay : addlActiveOverlays->overlays)
            {
                SingleOverlayStruct dstOverlay;

                dstOverlay.overlayId = srcOverlay.overlayId;

                // viewportRelativeOverlay
                copyControlFlagBase(dstOverlay.viewportRelativeOverlay, srcOverlay.viewportRelativeOverlay);

                dstOverlay.viewportRelativeOverlay.rectLeftPercent = srcOverlay.viewportRelativeOverlay.rectLeftPercent;
                dstOverlay.viewportRelativeOverlay.rectTopPercent  = srcOverlay.viewportRelativeOverlay.rectTopPercent;
                dstOverlay.viewportRelativeOverlay.rectWidthtPercent =
                    srcOverlay.viewportRelativeOverlay.rectWidthtPercent;
                dstOverlay.viewportRelativeOverlay.rectHeightPercent =
                    srcOverlay.viewportRelativeOverlay.rectHeightPercent;
                dstOverlay.viewportRelativeOverlay.mediaAlignment = srcOverlay.viewportRelativeOverlay.mediaAlignment;
                dstOverlay.viewportRelativeOverlay.relativeDisparityFlag =
                    srcOverlay.viewportRelativeOverlay.relativeDisparityFlag;
                dstOverlay.viewportRelativeOverlay.disparity = srcOverlay.viewportRelativeOverlay.disparity;

                // sphereRelativeOmniOverlay
                copyControlFlagBase(dstOverlay.sphereRelativeOmniOverlay, srcOverlay.sphereRelativeOmniOverlay);

                dstOverlay.sphereRelativeOmniOverlay.timelineChangeFlag =
                    srcOverlay.sphereRelativeOmniOverlay.timelineChangeFlag;

                if (srcOverlay.sphereRelativeOmniOverlay.doesExist)
                {
                    if (srcOverlay.sphereRelativeOmniOverlay.regionIndicationSphereRegion)
                    {
                        auto& dst =
                            dstOverlay.sphereRelativeOmniOverlay.region.set<ISOBMFF::RegionIndicationType::SPHERE>({});
                        copySphereRegion(dst, srcOverlay.sphereRelativeOmniOverlay.sphereRegion);
                    }
                    else
                    {
                        auto& dst = dstOverlay.sphereRelativeOmniOverlay.region
                                        .set<ISOBMFF::RegionIndicationType::PROJECTED_PICTURE>({});
                        copyPictureRegion(dst, srcOverlay.sphereRelativeOmniOverlay.region);
                    }
                    dstOverlay.sphereRelativeOmniOverlay.regionDepthMinus1 =
                        srcOverlay.sphereRelativeOmniOverlay.regionDepthMinus1;
                }

                // sphereRelative2DOverlay
                copyControlFlagBase(dstOverlay.sphereRelative2DOverlay, srcOverlay.sphereRelative2DOverlay);

                if (dstOverlay.sphereRelative2DOverlay.doesExist)
                {
                    copySphereRegion(dstOverlay.sphereRelative2DOverlay.sphereRegion,
                                     srcOverlay.sphereRelative2DOverlay.sphereRegion);

                    dstOverlay.sphereRelative2DOverlay.timelineChangeFlag =
                        srcOverlay.sphereRelative2DOverlay.timelineChangeFlag;

                    dstOverlay.sphereRelative2DOverlay.overlayRotation.yaw =
                        srcOverlay.sphereRelative2DOverlay.overlayRotation.yaw;
                    dstOverlay.sphereRelative2DOverlay.overlayRotation.pitch =
                        srcOverlay.sphereRelative2DOverlay.overlayRotation.pitch;
                    dstOverlay.sphereRelative2DOverlay.overlayRotation.roll =
                        srcOverlay.sphereRelative2DOverlay.overlayRotation.roll;

                    dstOverlay.sphereRelative2DOverlay.regionDepthMinus1 =
                        srcOverlay.sphereRelative2DOverlay.regionDepthMinus1;
                }

                // overlaySourceRegion
                copyControlFlagBase(dstOverlay.overlaySourceRegion, srcOverlay.overlaySourceRegion);
                copyPictureRegion(dstOverlay.overlaySourceRegion.region, srcOverlay.overlaySourceRegion.region);
                dstOverlay.overlaySourceRegion.transformType = srcOverlay.overlaySourceRegion.transformType;

                // recommendedViewportOverlay
                copyControlFlagBase(dstOverlay.recommendedViewportOverlay, srcOverlay.recommendedViewportOverlay);

                // overlayLayeringOrder
                copyControlFlagBase(dstOverlay.overlayLayeringOrder, srcOverlay.overlayLayeringOrder);
                dstOverlay.overlayLayeringOrder.layeringOrder = srcOverlay.overlayLayeringOrder.layeringOrder;

                // overlayOpacity
                copyControlFlagBase(dstOverlay.overlayOpacity, srcOverlay.overlayOpacity);
                dstOverlay.overlayOpacity.opacity = srcOverlay.overlayOpacity.opacity;

                // overlayInteraction
                copyControlFlagBase(dstOverlay.overlayInteraction, srcOverlay.overlayInteraction);

                dstOverlay.overlayInteraction.changePositionFlag  = srcOverlay.overlayInteraction.changePositionFlag;
                dstOverlay.overlayInteraction.changeDepthFlag     = srcOverlay.overlayInteraction.changeDepthFlag;
                dstOverlay.overlayInteraction.switchOnOffFlag     = srcOverlay.overlayInteraction.switchOnOffFlag;
                dstOverlay.overlayInteraction.changeOpacityFlag   = srcOverlay.overlayInteraction.changeOpacityFlag;
                dstOverlay.overlayInteraction.resizeFlag          = srcOverlay.overlayInteraction.resizeFlag;
                dstOverlay.overlayInteraction.rotationFlag        = srcOverlay.overlayInteraction.rotationFlag;
                dstOverlay.overlayInteraction.sourceSwitchingFlag = srcOverlay.overlayInteraction.sourceSwitchingFlag;
                dstOverlay.overlayInteraction.cropFlag            = srcOverlay.overlayInteraction.cropFlag;

                // overlayLabel
                copyControlFlagBase(dstOverlay.overlayLabel, srcOverlay.overlayLabel);
                dstOverlay.overlayLabel.overlayLabel = std::string(srcOverlay.overlayLabel.overlayLabel.begin(),
                                                                   srcOverlay.overlayLabel.overlayLabel.end());
                // overlayPriority
                copyControlFlagBase(dstOverlay.overlayPriority, srcOverlay.overlayPriority);
                dstOverlay.overlayPriority.overlayPriority = srcOverlay.overlayPriority.overlayPriority;

                // associatedSphereRegion
                copyControlFlagBase(dstOverlay.associatedSphereRegion, srcOverlay.associatedSphereRegion);

                dstOverlay.associatedSphereRegion.shapeType = srcOverlay.associatedSphereRegion.shapeType;
                copySphereRegion(dstOverlay.associatedSphereRegion.sphereRegion,
                                 srcOverlay.associatedSphereRegion.sphereRegion);

                // overlayAlphaCompositing
                copyControlFlagBase(dstOverlay.overlayAlphaCompositing, srcOverlay.overlayAlphaCompositing);
                dstOverlay.overlayAlphaCompositing.alphaBlendingMode =
                    srcOverlay.overlayAlphaCompositing.alphaBlendingMode;

                writer.addlActiveOverlays->overlays.push_back(dstOverlay);
            }
        }

        writer.write(bitstr);
        auto bitVector = bitstr.getStorage();
        // this feels so wrong every time I do it...
        auto retVal = DynArray<uint8_t>(bitVector.size());
        for (size_t i = 0; i < bitVector.size(); i++)
        {
            retVal[i] = bitVector[i];
        }

        return retVal;
    }

    // DynamicViewpointSample::DynamicViewpointSample() = default;
    // DynamicViewpointSample::DynamicViewpointSample(const ISOBMFF::DynamicViewpointSample& aSample) :
    //     sample(aSample)
    // {
    //     // nothing
    // }

    // DynamicViewpointSample::DynamicViewpointSample(const char* aFrameData,
    //                                                uint32_t aFrameLen,
    //                                                const ISOBMFF::DynamicViewpointSampleContext& aContext)
    // {
    //     fromBytes(reinterpret_cast<const std::uint8_t*>(aFrameData), aFrameLen, aContext);
    // }

    // void DynamicViewpointSample::fromBytes(const uint8_t* aFrameData,
    //                                        uint32_t aFrameLen,
    //                                        const ISOBMFF::DynamicViewpointSampleContext& aContext)
    // {
    //     BitStream bs({aFrameData, aFrameData + aFrameLen});
    //     sample.parse(bs, aContext);
    // }

    // DynArray<uint8_t> DynamicViewpointSample::toBytes(const ISOBMFF::DynamicViewpointSampleContext& aContext) const
    // {
    //     BitStream bs;
    //     sample.write(bs, aContext);
    //     auto& storage = bs.getStorage();
    //     if (storage.size())
    //     {
    //         return {&storage[0], &storage[0] + storage.size()};
    //     }
    //     else
    //     {
    //         return {};
    //     }
    // }
}  // namespace MP4VR
