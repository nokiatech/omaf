
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

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRNonCopyable.h"

#include "API/OMAFIArray.h"

OMAF_NS_BEGIN
template <typename T>
class Array : public IArray<T>
{
public:
    class ConstIterator
    {
        friend class Array;

    public:
        const T& operator*() const;
        const T* operator->() const;

        ConstIterator& operator++();
        ConstIterator& operator--();

        bool_t operator==(const ConstIterator& right) const;
        bool_t operator!=(const ConstIterator& right) const;

        bool_t operator<(const ConstIterator& right) const;
        bool_t operator>(const ConstIterator& right) const;
        bool_t operator<=(const ConstIterator& right) const;
        bool_t operator>=(const ConstIterator& right) const;

    protected:
        ConstIterator(const Array<T>* array, size_t index);

    protected:
        Array<T>* mArray;

        size_t mIndex;
    };

    class Iterator : public ConstIterator
    {
        friend class Array;

    public:
        T& operator*() const;
        T* operator->() const;

        Iterator& operator++();
        Iterator& operator--();

    protected:
        Iterator(Array<T>* array, size_t index);
    };

public:
    typedef T ValueType;

    typedef T& Reference;
    typedef const T& ConstReference;

    typedef T* Pointer;
    typedef const T* ConstPointer;

public:
    static const Iterator InvalidIterator;

public:
    Array(MemoryAllocator& allocator, size_t capacity = 0);
    Array(const Array<T>& array);

    ~Array();

    MemoryAllocator& getAllocator();

    T* getData();
    const T* getData() const;

    size_t getCapacity() const;
    size_t getSize() const;

    bool_t isEmpty() const;

    void_t clear();

    void_t reserve(size_t size);
    void_t resize(size_t size);

    void_t add(const T& item);
    void_t add(const T* array, size_t size);
    void_t add(const T* array, size_t size, size_t index);
    void_t add(const T& item, size_t index);
    void_t add(const Array<T>& array);

    bool_t remove(const T& item);
    void_t removeAt(size_t index, size_t count = 1);

    T& operator[](size_t index);
    const T& operator[](size_t index) const;

    T& at(size_t index);
    const T& at(size_t index) const;

    T& back();
    const T& back() const;

    T pop(size_t index);

    Iterator begin();
    ConstIterator begin() const;

    Iterator end();
    ConstIterator end() const;

    bool_t contains(const T& item) const;
    bool_t contains(const Array<T>& other) const;

private:
    OMAF_NO_DEFAULT(Array);
    OMAF_NO_ASSIGN(Array);

private:
    MemoryAllocator& mAllocator;

    size_t mSize;
    size_t mCapacity;
    T* mData;
};
OMAF_NS_END
#include "Foundation/NVRArrayImpl.h"
