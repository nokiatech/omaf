
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

#include "Foundation/NVRDependencies.h"

// MSCV doesn't have variadic copy from C99, so we need to use common behaviour which is just simple assign to make this work!
#ifndef va_copy

#define va_copy(d, s) ((d) = (s))

#endif

