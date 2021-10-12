
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

#include "NVRNamespace.h"

OMAF_NS_BEGIN
namespace Error
{
    enum Enum
    {
        OK = 0,
        OK_SKIPPED,  // no real error, but operation skipped
        OK_NO_CHANGE,
        OK_FILE_PARTIALLY_SUPPORTED,
        END_OF_FILE,

        // generic errors
        OUT_OF_MEMORY = 100,
        OPERATION_FAILED,

        // state errors
        INVALID_STATE = 200,
        NOT_INITIALIZED,
        ITEM_NOT_FOUND,
        BUFFER_OVERFLOW,
        NOT_READY,

        // input not accepted
        NOT_SUPPORTED = 300,
        INVALID_DATA,
        ALREADY_SET,

        // operational errors
        FILE_NOT_FOUND = 400,
        FILE_OPEN_FAILED,  // system refused to open it, e.g. file sharing issue?
        FILE_NOT_MP4,
        FILE_NOT_SUPPORTED,  // it can be mp4, but not one we support, or then it is some completely unknown format
        SEGMENT_CHANGE_FAILED,
        NETWORK_ACCESS_FAILED
    };
}
OMAF_NS_END
