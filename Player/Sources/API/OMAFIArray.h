
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

#include "Platform/OMAFDataTypes.h"

namespace OMAF
{
    template <typename T>
    class IArray
    {
    public:
        virtual ~IArray() {};
        virtual const T& operator [] (size_t index) const = 0;
        virtual const T& at(size_t index) const = 0;

        virtual size_t getSize() const = 0;
        virtual bool_t isEmpty() const = 0;
    };
}