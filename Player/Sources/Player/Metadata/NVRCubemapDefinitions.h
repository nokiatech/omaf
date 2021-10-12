
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

namespace
{
    static const int DEFAULT_FACE_ORDER = 0205413;  // player internal format, octal for 0x10B0B
    static const int NUM_FACES = 6;
    static const int BITS_FOR_FACE_ORDER = 3;
    static const int BITS_FOR_FACE_ORIENTATION = 3;
    static const int TOTAL_BITS_NEEDED = (BITS_FOR_FACE_ORDER + BITS_FOR_FACE_ORIENTATION) * NUM_FACES;
    static const int DEFAULT_OMAF_FACE_ORDER = 0425301; // NOTE: octal for 0x22ac1
    static const int DEFAULT_OMAF_FACE_ORIENTATION = 0131000;  // octal for 0xb200
}  // namespace
