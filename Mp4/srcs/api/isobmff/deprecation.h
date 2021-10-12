
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
#ifndef ISOBMFF_DEPRECATION_H
#define ISOBMFF_DEPRECATION_H

#if defined(__GNUC__) || defined(__clang__)
#define ISOBMFF_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define ISOBMFF_DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement ISOBMFF_DEPRECATED for this compiler")
#define ISOBMFF_DEPRECATED
#endif

#endif // ISOBMFF_DEPRECATION_H