
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
#include "Provider/NVRCoreProviderSources.h"

#include <mp4vrfiledatatypes.h>

OMAF_NS_BEGIN

namespace OmafMetadata
{
    Error::Enum parseOmafEquirectRegions(const MP4VR::RegionWisePackingProperty& aRwpk, RegionPacking& aRegionPacking, SourceDirection::Enum aSourceDirection);
    Error::Enum parseOmafCubemapFaceInfo(const MP4VR::RegionWisePackingProperty& aRwpk, SourceDirection::Enum aSourceDirection, uint32_t& aFaceOrderBits, uint32_t& aFaceOrientationBits);
}


OMAF_NS_END
