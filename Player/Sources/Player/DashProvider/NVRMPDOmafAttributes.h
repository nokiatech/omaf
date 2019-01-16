
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
#pragma once

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRFixedString.h"
#include "Media/NVRMimeType.h"
#include "Media/NVRMediaType.h"
#include "DashProvider/NVRDashUtils.h"
#include "VAS/NVRVASViewport.h"
#include "NVRMPDAttributes.h"

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
}

struct AdaptationSetBundleIds
{
    uint32_t mainAdSetId;
    FixedArray<uint32_t, 128> partialAdSetIds;
};
typedef FixedArray<AdaptationSetBundleIds, 128> SupportingAdaptationSetIds;
/*
 * As we don't have std::map, let's use an array of structs to store representation dependencies. TODO multiple extractor case not supported by this 
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

namespace DashOmafAttributes
{
    SourceType::Enum getOMAFVideoProjection(DashComponents& aDashComponents);
    Error::Enum getOMAFVideoViewport(DashComponents& aDashComponents, DashAttributes::Coverage& aCoverage, SourceType::Enum aProjection);
    Error::Enum hasOMAFVideoRWPK(DashComponents& aDashComponents);
    Error::Enum getOMAFQualityRankingType(const std::vector<dash::xml::INode *>& aNodes, bool_t& aMultiResolution);
    Error::Enum getOMAFQualityRanking(const std::vector<dash::xml::INode *>& aNodes, VASTileViewport& aViewPort, uint8_t& aQualityIndex, bool_t& aGlobal);
    bool_t getOMAFPreselection(DashComponents& aDashComponents, uint32_t aMyId);
    bool_t getOMAFPreselection(DashComponents& aDashComponents, uint32_t aMyId, AdaptationSetBundleIds& aReferredAdaptationSets);
    bool_t getOMAFRepresentationDependencies(DashComponents& aDashComponents, RepresentationDependencies& aDependingRepresentationIds);
};

OMAF_NS_END
