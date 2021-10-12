
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
#include "NVRMPDExtension.h"

OMAF_NS_BEGIN

namespace MPDExtension
{
    /*
     * Represents the OMAF-related MPEG-DASH Streaming Extensions.
     */

    const char_t* OMAF_PROJECTION_NODE = "urn:mpeg:mpegI:omaf:2017:pf";
    const char_t* OMAF_PROJECTION_TYPE = "omaf:projection_type";
    const char_t* OMAF_PROJECTION_TYPE_EQR = "0";
    const char_t* OMAF_PROJECTION_TYPE_CUBE = "1";
    const char_t* OMAF_CODECS_PROJECTION_EQR = "resv.podv+erpv";
    const char_t* OMAF_CODECS_PROJECTION_ERCM = "resv.podv+ercm";
    const char_t* OMAF_CODECS_PROJECTION_2D_HVC1 = "hvc1";
    const char_t* OMAF_CC_PROPERTY = "urn:mpeg:mpegI:omaf:2017:cc";
    const char_t* OMAF_CC_NODE = "omaf:cc";
    const char_t* OMAF_COVERAGE_NODE = "omaf:coverageInfo";
    const char_t* OMAF_RWPK_PROPERTY = "urn:mpeg:mpegI:omaf:2017:rwpk";
    const char_t* OMAF_RWPK_TYPE = "omaf:packing_type";
    const char_t* OMAF_SRQR_PROPERTY = "urn:mpeg:mpegI:omaf:2017:srqr";
    const char_t* OMAF_2DQR_PROPERTY = "urn:mpeg:mpegI:omaf:2017:2dqr";
    const char_t* OMAF_SRQR_NODE = "omaf:sphRegionQuality";
    const char_t* OMAF_2DQR_NODE = "omaf:twoDRegionQuality";
    const char_t* OMAF_SRQR_QUALITY_NODE = "omaf:qualityInfo";
    const char_t* OMAF_ENTITY_GROUP_NODE = "omaf2:EntityGroup";
    const char_t* OMAF_ENTITY_ID_NODE = "omaf2:EntityIdList";
    const char_t* OMAF_PRESEL_PROPERTY = "urn:mpeg:dash:preselection:2016";    
    const char_t* FRAME_PACKING = "urn:mpeg:mpegB:cicp:VideoFramePackingType";
    const char_t* OMAF_OVERLAY_INDICATOR_2018 = "urn:mpeg:mpegI:omaf:2018:ovly";
    const char_t* OMAF_OVERLAY_INDICATOR = "urn:mpeg:mpegI:omaf:2020:ovly";
    const char_t* OMAF_OVERLAY_ASSOCIATION_2018 = "urn:mpeg:mpegI:omaf:2018:assoc";
    const char_t* OMAF_OVERLAY_ASSOCIATION = "urn:mpeg:mpegI:omaf:2020:assoc";
    const char_t* OMAF_OVERLAY_ASSOCIATION_KIND_LIST = "associationKindList";
    const char_t* OMAF_OVERLAY_ASSOCIATION_BG = "ovbg";
    const char_t* OMAF_OVERLAY_ASSOCIATION_BG_TOKEN = "/Period/AdaptationSet";
    const char_t* OMAF_VIEWPOINT_SCHEME_2018 = "urn:mpeg:mpegI:omaf:2018:vwpt";
    const char_t* OMAF_VIEWPOINT_SCHEME = "urn:mpeg:mpegI:omaf:2020:vwpt";
    const char_t* OMAF_VIEWPOINT_INFO = "omaf2:ViewpointInfo";
    const char_t* OMAF_VIEWPOINT_INFO_GROUP = "omaf2:GroupInfo";
    const char_t* OMAF_VIEWPOINT_INFO_POSITION = "omaf2:Position";
    const char_t* OMAF_VIEWPOINT_INFO_SWITCHING_INFO = "omaf2:SwitchingInfo";
    const char_t* OMAF_VIEWPOINT_INFO_SWITCH_REGION = "omaf2:SwitchRegion";
    const char_t* OMAF_VIEWPOINT_INFO_VP_RELATIVE = "omaf2:VpRelative";
    const char_t* OMAF_VIEWPOINT_INFO_SPHERE_REGION = "omaf2:SphereRegion";
    const char_t* OMAF_VIEWPOINT_INFO_INITIAL = "initialViewpoint";
    const char_t* OMAF_VIEWPOINT_INFO_LABEL = "label";
    const char_t* OMAF_VIEWPOINT_ENTITY_GROUP_NAME = "vipo";
    const char_t* OMAF2_ENTITY_GROUP_PROPERTY = "urn:mpeg:mpegI:omaf:2020:etgb";

}  // namespace MPDExtension

OMAF_NS_END
