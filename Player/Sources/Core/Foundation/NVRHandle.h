
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

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"
#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN

template <typename HandleType, size_t IndexBits, size_t GenerationBits>
class GenericHandle
{
public:
    typedef HandleType TypeValue;

    static const GenericHandle<HandleType, IndexBits, GenerationBits> Invalid;

public:
    operator HandleType() const
    {
        return id;
    }

    bool_t isValid() const
    {
        return id != Invalid.id;
    }

    OMAF_INLINE bool_t operator==(const GenericHandle& other) const
    {
        return id == other.id;
    }

    OMAF_INLINE bool_t operator!=(const GenericHandle& other) const
    {
        return id != other.id;
    }

public:
    union {
        struct
        {
            HandleType index : IndexBits;            // Slot index
            HandleType generation : GenerationBits;  // Generation counter, zero generation is invalid
        };

        HandleType id;
    };

public:
    GenericHandle()
        : id(0)
    {
    }

    GenericHandle(const GenericHandle& other)
        : id(other.id)
    {
    }

    GenericHandle(HandleType handle)
        : id(handle)
    {
    }

    GenericHandle(HandleType index, HandleType generation)
        : index(index)
        , generation(generation)
    {
    }
};

template <typename HandleType, size_t IndexBits, size_t GenerationBits>
const GenericHandle<HandleType, IndexBits, GenerationBits>
    GenericHandle<HandleType, IndexBits, GenerationBits>::Invalid(0);

OMAF_NS_END
