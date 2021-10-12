
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

#include "DashProvider/NVRDashUtils.h"
#include "Foundation/NVRFixedString.h"
#include "Media/NVRMediaType.h"
#include "Media/NVRMimeType.h"
#include "NVRDashViewpointInfo.h"
#include "NVRMPDAttributes.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "VAS/NVRVASViewport.h"

OMAF_NS_BEGIN

namespace MPDAttribute
{
    /*
     * Represents the MPEG-DASH Streaming common attributes and elements.
     */
    extern const MimeType VIDEO_MIME_TYPE_OMAF_VI;
    extern const MimeType VIDEO_MIME_TYPE_OMAF_VI_ESC;
    extern const MimeType VIDEO_MIME_TYPE_OMAF_VD;
    extern const MimeType VIDEO_MIME_TYPE_OMAF_VD_ESC;
    extern const MimeType VIDEO_MIME_TYPE_OMAF2_VD;
    extern const MimeType VIDEO_MIME_TYPE_OMAF2_VD_ESC;
}  // namespace MPDAttribute

struct AdaptationSetBundleIds
{
    uint32_t mainAdSetId;
    FixedArray<uint32_t, 256> partialAdSetIds;
};
typedef FixedArray<AdaptationSetBundleIds, 128> SupportingAdaptationSetIds;
/*
 * As we don't have std::map, let's use an array of structs to store representation dependencies.
 */
typedef FixedArray<FixedString128, 64> DependableRepresentations;
struct DashDependency
{
    FixedString128 ownRepresentationId;
    // max 64 representations that this one depends on
    DependableRepresentations dependendableRepresentations;
};
// max 8 representations for extractor adaptation set
typedef FixedArray<DashDependency, 8> RepresentationDependencies;

// list of all representations belonging to each viewpoint

/// Single vipo entityid in dash poinst to an adaptation set and representation id pair
typedef std::pair<dash::mpd::IAdaptationSet*, dash::mpd::IRepresentation*> EntityId;

/// For now maximum number of supported background media for viewpoint is 256
typedef std::pair<uint32_t, FixedArray<EntityId, 256>> EntityGroupIds;

/// Support reading 64 vipo entity groups... could be better API to get count first and
/// to query each group separately
typedef FixedArray<EntityGroupIds, 64> DashEntityGroups;

namespace DashOmafAttributes
{
    SourceType::Enum getOMAFVideoProjection(DashComponents& aDashComponents);
	Error::Enum getOMAFVideoViewport(DashComponents& aDashComponents,
                                     DashAttributes::Coverage& aCoverage,
                                     SourceType::Enum aProjection);
    Error::Enum hasOMAFVideoRWPK(DashComponents& aDashComponents);
    Error::Enum getOMAFQualityRankingType(const std::vector<dash::xml::INode*>& aNodes, bool_t& aMultiResolution);
    Error::Enum getOMAFQualityRanking(const std::vector<dash::xml::INode*>& aNodes,
                                      VASTileViewport& aViewPort,
                                      uint8_t& aQualityIndex,
                                      bool_t& aGlobal);
    bool_t getOMAFPreselection(dash::mpd::IAdaptationSet* aAdaptationSet, uint32_t aMyId);
    bool_t getOMAFPreselection(dash::mpd::IAdaptationSet* aAdaptationSet,
                               uint32_t aMyId,
                               AdaptationSetBundleIds& aReferredAdaptationSets);
    bool_t getOMAFRepresentationDependencies(dash::mpd::IAdaptationSet* aAdaptationSet,
                                             RepresentationDependencies& aDependingRepresentationIds);

    bool_t getOMAFOverlay(dash::mpd::IAdaptationSet* aAdaptationSet);
    bool_t getOMAFCoverageInfo(dash::mpd::IAdaptationSet* aAdaptationSet);
    bool_t getOMAFOverlayAssociations(dash::mpd::IAdaptationSet* aAdaptationSet,
                                      FixedArray<uint32_t, 64>& aAssociatedIds);

    bool_t parseViewpoint(dash::mpd::IAdaptationSet* aAdaptationSet, MpdViewpointInfo& aViewpointInfo);

    /**
     * Get complete list of vipo entity groups and and contained representations
     */
    Error::Enum getOMAFEntityGroups(DashComponents& aDashComponents, std::string groupType, DashEntityGroups& anEntityGroups);

};  // namespace DashOmafAttributes

OMAF_NS_END
