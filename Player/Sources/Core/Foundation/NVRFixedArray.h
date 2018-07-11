
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

#include "Foundation/NVRCompatibility.h"
#include "Platform/OMAFCompiler.h"
#include "Foundation/NVRNonCopyable.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
    template <typename T, size_t N>
    class FixedArray
    {
        public:
            
            class ConstIterator
            {
                friend class FixedArray;
                
                public:
                    
                    const T& operator * () const;
                    const T* operator -> () const;
                    
                    ConstIterator& operator ++ ();
                    ConstIterator& operator -- ();
                    
                    bool_t operator == (const ConstIterator& right) const;
                    bool_t operator != (const ConstIterator& right) const;
                    
                    bool_t operator < (const ConstIterator& right) const;
                    bool_t operator > (const ConstIterator& right) const;
                    bool_t operator <= (const ConstIterator& right) const;
                    bool_t operator >= (const ConstIterator& right) const;
                    
                protected:
                    
                    ConstIterator(const FixedArray<T, N>* array, size_t index);
                    
                protected:
                    
                    FixedArray<T, N>* mArray;
                    
                    size_t mIndex;
            };
            
            class Iterator
            : public ConstIterator
            {
                friend class FixedArray;
                
                public:
                    
                    T& operator * () const;
                    T* operator -> () const;
                    
                    Iterator& operator ++ ();
                    Iterator& operator -- ();
                    
                protected:
                    
                    Iterator(FixedArray<T, N>* array, size_t index);
            };
        
        public:
        
            typedef T ValueType;
        
            typedef T& Reference;
            typedef const T& ConstReference;
        
            typedef T* Pointer;
            typedef const T* ConstPointer;
        
        public:
        
            static Iterator InvalidIterator;
        
        public:
        
            FixedArray();
            FixedArray(const FixedArray<T, N>& array);
        
            ~FixedArray();
        
            T* getData();
            const T* getData() const;
        
            size_t getCapacity() const;
            size_t getSize() const;
        
            bool_t isEmpty() const;
        
            void_t resize(size_t size);
        
            void_t clear();
            
            void_t add(const T& item);
            void_t add(const T* array, size_t size);
            void_t add(const T* array, size_t size, size_t index);
            void_t add(const T& item, size_t index);
            void_t add(const FixedArray<T, N>& array);
        
            bool_t remove(const T& item);
            void_t remove(const ConstIterator& iterator);
        
            void_t removeAt(size_t index, size_t count = 1);
        
            T& operator [] (size_t index);
            const T& operator [] (size_t index) const;
        
            T& at(size_t index);
            const T& at(size_t index) const;

            T& back();
            const T& back() const;

            T& front();
            const T& front() const;
        
            Iterator begin();
            ConstIterator begin() const;
        
            Iterator end();
            ConstIterator end() const;
            
            bool_t contains(const T& item) const;
            bool_t contains(const FixedArray<T, N>& other) const;

            FixedArray<T, N> intersection(const FixedArray<T, N>& other) const;
            FixedArray<T, N> difference(const FixedArray<T, N>& other) const;

            FixedArray<T, N>& operator = (const FixedArray<T, N>& other);

            bool_t operator == (const FixedArray<T, N>& other) const;
            bool_t operator != (const FixedArray<T, N>& other) const;

        private:

            size_t mSize;
        
            T mData[N];
    };
OMAF_NS_END

#include "Foundation/NVRFixedArrayImpl.h"
