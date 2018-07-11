
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
    class FixedQueue
    {
        public:
        
            typedef T* Iterator;
            typedef const T* ConstIterator;
        
            static Iterator InvalidIterator;
        
        public:
        
            FixedQueue();
            FixedQueue(const FixedQueue<T, N>& queue);
            
            ~FixedQueue();
        
            size_t getCapacity() const;
            size_t getSize() const;
        
            bool_t isEmpty() const;
        
            void_t clear();
        
            void_t push(const T& object);
            void_t pop();

            T& front();
            const T& front() const;

            T& back();
            const T& back() const;
        
        private:
        
            OMAF_NO_ASSIGN(FixedQueue);
            
        private:

            size_t mHead;
            size_t mTail;
        
            size_t mSize;
            T mData[N];
    };
OMAF_NS_END

#include "Foundation/NVRFixedQueueImpl.h"
