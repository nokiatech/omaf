
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
#include "Foundation/NVRHashMap.h"
#include "Foundation/NVRFixedArray.h"

//#include "Metadata/NVRMetadataContainer.h"

OMAF_NS_BEGIN
typedef uint8_t sourceid_t;
typedef uint8_t streamid_t;

    #define MAX_IMAGE_SOURCES 8

    typedef FixedArray<sourceid_t, MAX_IMAGE_SOURCES> ImageSources;

//    typedef HashMap<sourceid_t, AudioMetadataContainer*> AudioSourceMetadataMap;
OMAF_NS_END
