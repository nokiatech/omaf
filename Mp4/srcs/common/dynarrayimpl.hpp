
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
#include "api/isobmff/dynarray.h"

// for instantiating DynArray for the types in the API
#include "api/reader/mp4vrfiledatatypes.h"

#include "customallocator.hpp"

namespace ISOBMFF
{
#if !(defined(_MSC_VER))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    template <typename T>
    DynArray<T>::~DynArray()
    {
        CUSTOM_DELETE_ARRAY(elements, T);
    }

    template <typename T>
    DynArray<T>::DynArray()
        : numElements(0)
        , elements(nullptr)
    {
    }

    template <typename T>
    DynArray<T>::DynArray(size_t n)
        : numElements(n)
        , elements(CUSTOM_NEW_ARRAY(T, n))
    {
    }

    template <typename T>
    DynArray<T>::DynArray(const DynArray& other)
        : numElements(other.numElements)
        , elements(CUSTOM_NEW_ARRAY(T, other.numElements))
    {
        std::copy(other.elements, other.elements + other.numElements, elements);
    }

    template <typename T>
    DynArray<T>::DynArray(DynArray&& other)
        : numElements(other.numElements)
        , elements(other.elements)
    {
        other.numElements = 0;
        other.elements = nullptr;
    }

    template <typename T>
    DynArray<T>::DynArray(const T* begin, const T* end)
        : numElements(static_cast<size_t>(std::distance(begin, end)))
        , elements(CUSTOM_NEW_ARRAY(T, static_cast<size_t>(std::distance(begin, end))))
    {
        std::copy(begin, end, elements);
    }

    template <typename T>
    DynArray<T>::DynArray(std::initializer_list<T> init)
        : numElements(static_cast<size_t>(std::distance(init.begin(), init.end())))
        , elements(CUSTOM_NEW_ARRAY(T, static_cast<size_t>(std::distance(init.begin(), init.end()))))
    {
        std::copy(init.begin(), init.end(), elements);
    }

    template <typename T>
    DynArray<T>& DynArray<T>::operator=(const DynArray<T>& other)
    {
        if (this != &other)
        {
            CUSTOM_DELETE_ARRAY(elements, T);
            numElements     = other.numElements;
            elements = CUSTOM_NEW_ARRAY(T, numElements);
            std::copy(other.elements, other.elements + other.numElements, elements);
        }
        return *this;
    }

    template <typename T>
    DynArray<T>& DynArray<T>::operator=(DynArray<T>&& other)
    {
        if (this != &other)
        {
            CUSTOM_DELETE_ARRAY(elements, T);
            numElements = other.numElements;
            elements = other.elements;
            other.numElements = 0;
            other.elements = nullptr;
        }
        return *this;
    }
#if !(defined(_MSC_VER))
#pragma GCC diagnostic pop
#endif
}
