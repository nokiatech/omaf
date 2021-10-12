
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

#include "Foundation/NVRFixedString.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
namespace DeviceInfo
{
    void initialize();
    void shutdown();
    const FixedString256& getOSName();
    const FixedString256& getOSVersion();
    const FixedString256& getDeviceModel();
    const FixedString256& getDeviceType();
    const FixedString256& getDevicePlatformInfo();
    const FixedString256& getDevicePlatformId();
    FixedString256 getUniqueId();
    const FixedString256& getAppId();
    const FixedString256& getAppBuildNumber();
    bool_t deviceSupports2VideoTracks();
    bool_t deviceSupportsHEVC();
    bool_t deviceCanReconfigureVideoDecoder();

    namespace LayeredVASTypeSupport
    {
        enum Enum
        {
            UNDEFINED = -1,
            NOT_SUPPORTED,
            // Mono or framepacked VAS => 2-track VAS converted on-the-fly to mono
            LIMITED,
            // Stereo 2-track VAS, includes also mono and framepacked, AND full 2-track 360 without VAS
            FULL_STEREO,
            COUNT
        };
    }
    LayeredVASTypeSupport::Enum deviceSupportsLayeredVAS();
    uint32_t maxLayeredVASTileCount();
    uint32_t maxDecodedPixelCountPerSecond();
    bool_t deviceSupportsSubsegments();

    bool_t isCurrentDeviceSupported();


}  // namespace DeviceInfo
OMAF_NS_END
