
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

#include "Platform/OMAFPlatformDetection.h"

#if OMAF_PLATFORM_ANDROID

#include "Foundation/Android/NVRCompatibility.h"

#elif OMAF_PLATFORM_WINDOWS

#include "Foundation/Windows/NVRCompatibility.h"

#elif OMAF_PLATFORM_UWP

#include "Foundation/UWP/NVRCompatibility.h"

#else

#error Unknown platform

#endif
