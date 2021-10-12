
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
#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <cinttypes>
#include <streamsegmenter/segmenterapi.hpp>

#include "common/rational.h"
#include "config/config.h"
#include "isobmff/commontypes.h"
#include "reader/mp4vrfiledatatypes.h"
#include "dash.h"
#include "async/future.h"

namespace VDD
{
    const int32_t kStepsInDegree = 65536;
    const int32_t kStepsInGpsDegree = 1 << 23;

    enum class EntityGroupType {
        Altr,
        Ovbg,
        Oval,
        Vipo
    };

    using EntityGroupSpec =
        ISOBMFF::Union<EntityGroupType,
                       StreamSegmenter::Segmenter::AltrEntityGroupSpec,   // altr
                       StreamSegmenter::Segmenter::OvbgEntityGroupSpec,   // ovbg
                       StreamSegmenter::Segmenter::OvalEntityGroupSpec,   // oval
                       StreamSegmenter::Segmenter::VipoEntityGroupSpec>;  // vipo

    using EntityGroupId = StreamSegmenter::IdBase<uint32_t, struct EntityGroupIdTag>;
    using EntityId = StreamSegmenter::IdBase<uint32_t, struct EntityIdTag>;

    using RefIdLabel = StreamSegmenter::IdBase<std::string, class RefIdLabelTag>;
    struct SingleEntityIdReference {
        std::uint32_t entityId{};
        TrackId trackId;
        StreamId adaptationSetId;
        Future<RepresentationId> representationId;
    };

    struct EntityIdReference {
        ParsedValue<RefIdLabel> label;
        std::list<SingleEntityIdReference> references;
        std::set<EntityId> entityIds; // used for detecting duplicates (debugging)
    };

    class ViewpointInfo
    {
    public:
        ViewpointInfo() = default;
        ViewpointInfo(const ConfigValue& aValue);
        ViewpointInfo(const ViewpointInfo&) = default;
        ViewpointInfo& operator=(const ViewpointInfo&) = default;

        ISOBMFF::DynamicViewpointSampleEntry getDyvpSampleEntry() const;

        StreamSegmenter::Segmenter::VipoEntityGroupSpec getVipoEntityGroupSpec(
            std::string aLabel, ViewpointGroupId aViewpointGroupId) const;

        StreamSegmenter::MPDTree::Omaf2Viewpoint getOmaf2Viewpoint(
            std::string aId, std::string aLabel,
            ViewpointGroupId aViewpointGroupId, bool aIsInitial) const;

    private:
        ISOBMFF::DynamicViewpointSampleEntry mDynamicViewpointSampleEntry;
        Optional<ISOBMFF::ViewpointLoopingStruct> mViewpointLooping;
    };

    struct LabelEntityIdMapping {
        std::map<ParsedValue<RefIdLabel>, EntityIdReference> forward;
        std::map<EntityId, ParsedValue<RefIdLabel>> labelByEntityId;

        void addLabel(EntityIdReference aEntityIdReference);

        const EntityIdReference& entityById(const EntityId& aEntityId) const;
    };

    ParsedValue<RefIdLabel> readRefIdLabel(const ConfigValue& aNode);

    using TrackReferences = std::map<std::string, std::list<ParsedValue<RefIdLabel>>>;
    TrackReferences readTrackReferences(const ConfigValue& aNode);

    struct EntityGroupReadContext {
        // Used for generating entity group ids that are unique regarding other ids
        std::function<EntityGroupId(void)> allocateEntityGroupId;

        // Used for resolving labels to entity ids
        LabelEntityIdMapping labelEntityIdMapping;
    };

    void mergeLabelEntityIdMapping(LabelEntityIdMapping& aTo,
                                   const LabelEntityIdMapping& aFrom);

    // c++ lol
    template <typename Container>
    auto makeDynArray(const Container& aContainer) -> MP4VR::DynArray<typename std::remove_const<
        typename std::remove_reference<decltype(*aContainer.begin())>::type>::type>
    {
        MP4VR::DynArray<typename std::remove_const<
            typename std::remove_reference<decltype(*aContainer.begin())>::type>::type>
            r(aContainer.size());
        std::copy(aContainer.begin(), aContainer.end(), r.begin());
        return r;
    }

    MP4VR::DynArray<char> readDynArrayString(const ConfigValue& aValue);

    template <typename Read>
    auto readDynArray(std::string aTypeName, Read aReader)
        -> std::function<MP4VR::DynArray<decltype(aReader({}))>(const ConfigValue& aNode)>
    {
        return [=](const ConfigValue& aNode) {
            (void)aTypeName;  // not used for now
            std::vector<decltype(aReader({}))> r;
            for (const ConfigValue& value : aNode.childValues())
            {
                r.push_back(aReader(value));
            }
            return makeDynArray(r);
        };
    }


    bool anySingleOverlayPropertySet(const MP4VR::SingleOverlayProperty& aProp);

    float readFloatPercent(const ConfigValue& aValue);
    auto readFloatUint360Degrees(int32_t stepsInDegree)
        -> std::function<uint32_t(const ConfigValue& valNode)>;
    auto readFloatUint180Degrees(int32_t stepsInDegree)
        -> std::function<uint32_t(const ConfigValue& valNode)>;
    auto readFloatSigned180Degrees(int32_t stepsInDegree)
        -> std::function<int32_t(const ConfigValue& valNode)>;
    auto readFloatSigned90Degrees(int32_t stepsInDegree)
        -> std::function<int32_t(const ConfigValue& valNode)>;

    template <typename T>
    T percentToFP(float aPercent)
    {
        return static_cast<T>(aPercent * std::numeric_limits<T>::max() / 100);
    }

    template <typename T>
    T readFloatPercentAsFP(const ConfigValue& valNode)
    {
        return percentToFP<T>(readFloatPercent(valNode));
    }

    const auto readUint8Percent = readRangedIntegral("percent", readUint8, 0, 100);

    uint16_t readRegiondDepthMinus1FromPercent(const ConfigValue& aValue);

    MP4VR::ProjectedPictureRegionProperty readProjectedPictureRegionProperty(
        const ConfigValue& aValue);
    MP4VR::PackedPictureRegionProperty readPackedPictureRegionProperty(const ConfigValue& aValue);
    MP4VR::SphereRegionProperty readSphereRegionProperty(const ConfigValue& aValue);
    MP4VR::SphereRelativeOmniOverlay readSphereRelativeOmniOverlay(const ConfigValue& aValue);
    MP4VR::RotationProperty readRotationProperty(const ConfigValue& aValue);
    MP4VR::SphereRelative2DOverlay readSphereRelative2DOverlay(const ConfigValue& aValue);
    MP4VR::TransformType readTransformType(const ConfigValue& aValue);
    MP4VR::OverlaySourceRegion readOverlaySourceRegion(const ConfigValue& aValue);
    MP4VR::RecommendedViewportOverlay readRecommendedViewportOverlay(const ConfigValue& aValue);
    MP4VR::OverlayLayeringOrder readOverlayLayeringOrder(const ConfigValue& aValue);
    MP4VR::OverlayOpacity readOverlayOpacity(const ConfigValue& aValue);
    MP4VR::OverlayInteraction readOverlayInteraction(const ConfigValue& aValue);
    MP4VR::OverlayLabel readOverlayLabel(const ConfigValue& aValue);
    MP4VR::OverlayPriority readOverlayPriority(const ConfigValue& aValue);
    MP4VR::ViewportRelativeOverlay readViewportRelativeOverlay(const ConfigValue& aValue);
    MP4VR::SingleOverlayProperty readSingleOverlayProperty(const ConfigValue& aValue);
    MP4VR::OverlayConfigProperty readOverlayConfigProperty(const ConfigValue& aValue);

    // Named 2 due to an existing definition for reading VDD InitialViewingOrientationSamples in
    // timedmetadata.cpp
    MP4VR::InitialViewingOrientationSample readInitialViewingOrientationSample2(
        const ConfigValue& aValue);
    MP4VR::OverlayConfigSample readOverlayConfigSample(const ConfigValue& aValue);

    ISOBMFF::PackedPictureRegion readPackedPictureRegion(const ConfigValue& aValue);
    ISOBMFF::TransitionEffect readTransitionEffect(const ConfigValue& aValue);
    ISOBMFF::ViewpointPosStruct readViewpointPosStruct(const ConfigValue& aValue);
    ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct
    readViewpointGlobalCoordinateSysRotationStruct(const ConfigValue& aValue);
    ISOBMFF::ViewpointGpsPositionStruct readViewpointGpsPositionStruct(const ConfigValue& aValue);
    ISOBMFF::ViewpointGeomagneticInfoStruct readViewpointGeomagneticInfoStruct(
        const ConfigValue& aValue);
    ISOBMFF::ViewpointSwitchingListStruct readViewpointSwitchingListStruct(
        const ConfigValue& aValue);
    ISOBMFF::ViewpointLoopingStruct readViewpointLoopingStruct(
        const ConfigValue& aValue);
    ViewpointInfo readViewpointInfo(const ConfigValue& aValue);
    ISOBMFF::ViewpointTimelineSwitchStruct readViewpointTimelineSwitchStruct(
        const ConfigValue& aValue);
    ISOBMFF::DynamicViewpointSampleEntry readDynamicViewpointSampleEntry(const ConfigValue& aValue);
    auto readDynamicViewpointSample(ISOBMFF::DynamicViewpointSampleEntry aSampleEntry)
        -> std::function<ISOBMFF::DynamicViewpointSample(const ConfigValue& aValue)>;
    ISOBMFF::DynamicViewpointGroup readDynamicViewpointGroup(const ConfigValue& aValue);
    std::string readViewpointLabel(const ConfigValue& aValue);
    ISOBMFF::ViewpointGroupStruct<true> readViewpointGroupStruct(const ConfigValue& aValue);

    ISOBMFF::SphereRegionRange readSphereRegionRange(const ConfigValue& aValue);
    ISOBMFF::SphereRegionConfigStruct readSphereRegionConfig(const ConfigValue& aValue);

    ISOBMFF::InitialViewpointSample readInitialViewpointSample(const ConfigValue& aValue);

    ISOBMFF::RecommendedViewportInfoStruct readRecommendedViewportInfo(const ConfigValue& aValue);

    ISOBMFF::MediaAlignmentType readMediaAlignmentType(const ConfigValue& aValue);

    template <bool HasRange, bool HasInterpolate>
    ISOBMFF::SphereRegionStatic<HasRange, HasInterpolate> readSphereRegionStatic(
        const ConfigValue& aValue);

    auto readSphereRegionDynamic(const ISOBMFF::SphereRegionContext& aContext) ->
        std::function<ISOBMFF::SphereRegionDynamic(const ConfigValue& aValue)>;

    auto readSphereRegionSample(const ISOBMFF::SphereRegionConfigStruct& aConfig)
        -> std::function<ISOBMFF::SphereRegionSample(const ConfigValue& aValue)>;

    auto readEntityIdReference(const EntityGroupReadContext& aReadContext)
        -> std::function<EntityIdReference(const ConfigValue& aValue)>;

    auto readEntityGroups(const EntityGroupReadContext& aReadContext)
        -> std::function<StreamSegmenter::Segmenter::MetaSpec(const ConfigValue& aValue)>;

    ISOBMFF::OverlayStruct readOverlayStruct(const ConfigValue& aValue);

    ISOBMFF::AlphaBlendingModeType readAlphaBlendingModeType(const ConfigValue& aValue);

    ISOBMFF::AssociatedSphereRegion readISOBMFFAssociatedSphereRegion(const ConfigValue& aValue);

    ISOBMFF::ProjectedPictureRegion readISOBMFFProjectedPictureRegion(const ConfigValue& aValue);

    enum class OverlayControlRead
    {
        NoReadInherited,
        ReadInherited
    };

    bool readToOverlayControlFlagBase(MP4VR::OverlayControlFlagBase& r, const ConfigValue& aValue,
                                      OverlayControlRead aRead = OverlayControlRead::ReadInherited);
    bool readToOverlayControlFlagBase(ISOBMFF::OverlayControlFlagBase& r, const ConfigValue& aValue,
                                      OverlayControlRead aRead = OverlayControlRead::ReadInherited);

    /* StreamSegmenter::Segmenter::OvbgEntityGroupSpec readOvbgEntityGroup(const ConfigValue&
     * aValue); */

    /* StreamSegmenter::Segmenter::OvalEntityGroupSpec readOvalEntityGroup(const ConfigValue& aValue); */



}  // namespace VDD
