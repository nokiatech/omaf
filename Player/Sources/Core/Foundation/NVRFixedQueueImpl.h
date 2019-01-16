
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRConstruct.h"
#include "Foundation/NVRMemoryUtilities.h"

OMAF_NS_BEGIN
    template <typename T, size_t N>
    typename FixedQueue<T, N>::Iterator FixedQueue<T, N>::InvalidIterator(OMAF_NULL);
    
    template <typename T, size_t N>
    FixedQueue<T, N>::FixedQueue()
    : mHead(0)
    , mTail(N - 1)
    , mSize(0)
    {
    }
    
    template <typename T, size_t N>
    FixedQueue<T, N>::FixedQueue(const FixedQueue<T, N>& queue)
    {
        for (size_t i = 0; i < queue.getSize(); ++i)
        {
            size_t index = (queue.mHead + i) % N;
            
            Construct<T>(&mData[index]);
            mData[index] = queue.mData[index];
        }
        
        mHead = queue.mHead;
        mTail = queue.mTail;
        mSize = queue.mSize;
    }
    
    template <typename T, size_t N>
    FixedQueue<T, N>::~FixedQueue()
    {
        clear();
    }
    
    template <typename T, size_t N>
    size_t FixedQueue<T, N>::getCapacity() const
    {
        return N;
    }
    
    template <typename T, size_t N>
    size_t FixedQueue<T, N>::getSize() const
    {
        return mSize;
    }
    
    template <typename T, size_t N>
    bool_t FixedQueue<T, N>::isEmpty() const
    {
        return (mSize == 0);
    }
    
    template <typename T, size_t N>
    void_t FixedQueue<T, N>::clear()
    {
        if(mSize > 0)
        {
            for (size_t i = 0; i < mSize; ++i)
            {
                size_t index = (mHead + i) % N;
                
                Destruct<T>(&mData[index]);
            }
        }
        
        mSize = 0;
        
        mHead = 0;
        mTail = (N - 1);
    }
    
    template <typename T, size_t N>
    void_t FixedQueue<T, N>::push(const T& object)
    {
        OMAF_ASSERT(mSize + 1 <= N, "");
        
        if (mSize + 1 <= N)
        {
            mTail = (mTail + 1) % N;
            
            Construct<T>(&mData[mTail]);
            mData[mTail] = object;

            mSize++;
        }
    }
    
    template <typename T, size_t N>
    void_t FixedQueue<T, N>::pop()
    {
        OMAF_ASSERT(mSize > 0, "");
        
        if (mSize > 0)
        {
            Destruct<T>(&mData[mHead]);
        
            mHead = (mHead + 1) % N;
            
            mSize--;
        }
    }
    
    template <typename T, size_t N>
    T& FixedQueue<T, N>::front()
    {
        if(mSize > 0)
        {
            return *(mData + mHead);
        }

        OMAF_ASSERT(false, "");

        return *(T*)OMAF_NULL;
    }
    
    template <typename T, size_t N>
    const T& FixedQueue<T, N>::front() const
    {
        if(mSize > 0)
        {
            return *(mData + mHead);
        }

        OMAF_ASSERT(false, "");

        return *(T*)OMAF_NULL;
    }
    
    template <typename T, size_t N>
    T& FixedQueue<T, N>::back()
    {
        if(mSize > 0)
        {
            return *(mData + mTail);
        }

        OMAF_ASSERT(false, "");
        
        return *(T*)OMAF_NULL;
    }
    
    template <typename T, size_t N>
    const T& FixedQueue<T, N>::back() const
    {
        if(mSize > 0)
        {
            return *(mData + mTail);
        }

        OMAF_ASSERT(false, "");
        
        return *(T*)OMAF_NULL;
    }
OMAF_NS_END
