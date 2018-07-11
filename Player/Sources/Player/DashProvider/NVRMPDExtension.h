
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

OMAF_NS_BEGIN

    namespace MPDExtension 
    {
        /*
        * Represents the OMAF-related MPEG-DASH Streaming Extensions.
        */

        extern const char_t* OMAF_PROJECTION_NODE;
        extern const char_t* OMAF_PROJECTION_TYPE;
        extern const char_t* OMAF_PROJECTION_TYPE_EQR;
        extern const char_t* OMAF_PROJECTION_TYPE_CUBE;
        extern const char_t* OMAF_CODECS_PROJECTION_EQR;
        extern const char_t* OMAF_CODECS_PROJECTION_ERCM;
        extern const char_t* OMAF_CC_PROPERTY;
        extern const char_t* OMAF_CC_NODE;
        extern const char_t* OMAF_COVERAGE_NODE;
        extern const char_t* OMAF_RWPK_PROPERTY;
        extern const char_t* OMAF_RWPK_TYPE;
        extern const char_t* OMAF_SRQR_PROPERTY;
        extern const char_t* OMAF_2DQR_PROPERTY;
        extern const char_t* OMAF_SRQR_NODE;
        extern const char_t* OMAF_2DQR_NODE;
        extern const char_t* OMAF_SRQR_QUALITY_NODE;
        extern const char_t* OMAF_PRESEL_PROPERTY;
        extern const char_t* FRAME_PACKING;

        // Viewport definitions are given as comma-separated list
        extern const char_t* MPD_VALUE_LIMITER;


    }


OMAF_NS_END
