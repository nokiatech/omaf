
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

#include "NVREssentials.h"

#if OMAF_PLATFORM_WINDOWS

OMAF_NS_BEGIN

#define declare(a,b) extern a b;
#include "Graphics/OpenGL/Windows/NVRGLFunctions.inc"
#undef declare

#define glDepthRangef glDepthRange

void InitializeGLFunctions();

OMAF_NS_END

#endif
