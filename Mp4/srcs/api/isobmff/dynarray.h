
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
#ifndef ISOBMFF_DYNARRAY_H
#define ISOBMFF_DYNARRAY_H

#include <cstdint>
#include <cstddef>
#include "mp4vrfileexport.h"
#include "deprecation.h"

namespace ISOBMFF
{
    template <typename T>
    struct MP4VR_DLL_PUBLIC DynArray
    {
        typedef T value_type;
        typedef T* iterator;
        typedef const T* const_iterator;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        std::size_t numElements;
        // This is to be replaced with C++ standard library compliant size()-method:
#if !(defined(_MSC_VER))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
        ISOBMFF_DEPRECATED std::size_t& size = numElements;
#if !(defined(_MSC_VER))
#pragma GCC diagnostic pop
#endif
        T* elements;
        DynArray();
        DynArray(std::size_t n);
        DynArray(const DynArray& other);
        DynArray(DynArray&& other);
        DynArray(const T* begin, const T* end);
        DynArray(std::initializer_list<T>);
        DynArray& operator=(const DynArray& other);
        DynArray& operator=(DynArray&& other);
        inline T& operator[](std::size_t index)
        {
            return elements[index];
        }
        inline const T& operator[](std::size_t index) const
        {
            return elements[index];
        }
        inline T* begin()
        {
            return elements;
        }
        inline T* end()
        {
            return elements + numElements;
        }
        inline const T* begin() const
        {
            return elements;
        }
        inline const T* end() const
        {
            return elements + numElements;
        }
        ~DynArray();
    };
}

#endif // ISOBMFF_DYNARRAY_H
