
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
        const char_t* OMAF_PRESEL_PROPERTY = "urn:mpeg:dash:preselection:2016";
        const char_t* FRAME_PACKING = "urn:mpeg:mpegB:cicp:VideoFramePackingType";

        // Viewport definitions are given as comma-separated list
        const char_t* MPD_VALUE_LIMITER = ",";

    }

OMAF_NS_END
