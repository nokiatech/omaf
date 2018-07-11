
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
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRMemoryUtilities.h"
#include "Foundation/NVRNew.h"

OMAF_NS_BEGIN
    template <typename T>
    const T& Array<T>::ConstIterator::operator * () const
    {
        OMAF_ASSERT_NOT_NULL(mArray);
        OMAF_ASSERT(mIndex < mArray->getSize(), "");
        
        return mArray->at(mIndex);
    }
    
    template <typename T>
    const T* Array<T>::ConstIterator::operator -> () const
    {
        OMAF_ASSERT_NOT_NULL(mArray);
        OMAF_ASSERT(mIndex < mArray->getSize(), "");
        
        return &mArray->at(mIndex);
    }
    
    template <typename T>
    typename Array<T>::ConstIterator& Array<T>::ConstIterator::operator ++ ()
    {
        ++mIndex;
        
        return *this;
    }
    
    template <typename T>
    typename Array<T>::ConstIterator& Array<T>::ConstIterator::operator -- ()
    {
        --mIndex;
        
        return *this;
    }
    
    template <typename T>
    bool_t Array<T>::ConstIterator::operator == (const ConstIterator& right) const
    {
        return (mIndex == right.mIndex);
    }
    
    template <typename T>
    bool_t Array<T>::ConstIterator::operator != (const ConstIterator& right) const
    {
        return (mIndex != right.mIndex);
    }
    
    template <typename T>
    bool_t Array<T>::ConstIterator::operator < (const ConstIterator& right) const
    {
        return (mIndex < right.mIndex);
    }
    
    template <typename T>
    bool_t Array<T>::ConstIterator::operator > (const ConstIterator& right) const
    {
        return (mIndex > right.mIndex);
    }
    
    template <typename T>
    bool_t Array<T>::ConstIterator::operator <= (const ConstIterator& right) const
    {
        return (mIndex <= right.mIndex);
    }
    
    template <typename T>
    bool_t Array<T>::ConstIterator::operator >= (const ConstIterator& right) const
    {
        return (mIndex >= right.mIndex);
    }
    
    template <typename T>
    Array<T>::ConstIterator::ConstIterator(const Array<T>* array, size_t index)
    : mArray(const_cast<Array<T>*>(array))
    , mIndex(index)
    {
    }
    
    
    template <typename T>
    T& Array<T>::Iterator::operator * () const
    {
        OMAF_ASSERT_NOT_NULL(this->mArray);
        OMAF_ASSERT(this->mIndex < this->mArray->getSize(), "");
        
        return this->mArray->at(this->mIndex);
    }
    
    template <typename T>
    T* Array<T>::Iterator::operator -> () const
    {
        OMAF_ASSERT_NOT_NULL(this->mArray);
        OMAF_ASSERT(this->mIndex < this->mArray->getSize(), "");
        
        return &this->mArray->at(this->mIndex);
    }
    
    template <typename T>
    typename Array<T>::Iterator& Array<T>::Iterator::operator ++ ()
    {
        OMAF_ASSERT_NOT_NULL(this->mArray);
        
        ++this->mIndex;
        
        return *this;
    }
    
    template <typename T>
    typename Array<T>::Iterator& Array<T>::Iterator::operator -- ()
    {
        OMAF_ASSERT_NOT_NULL(this->mArray);
        
        --this->mIndex;
        
        return *this;
    }
    
    template <typename T>
    Array<T>::Iterator::Iterator(Array<T>* array, size_t index)
    : Array<T>::ConstIterator(array, index)
    {
    }
    
    
    template <typename T>
    const typename Array<T>::Iterator Array<T>::InvalidIterator(OMAF_NULL, -1);

    
    template <typename T>
    Array<T>::Array(MemoryAllocator& allocator, size_t capacity)
    : mAllocator(allocator)
    , mSize(0)
    , mCapacity(capacity)
    , mData(OMAF_NULL)
    {
        if(mCapacity > 0)
        {
            mData = OMAF_NEW_ARRAY(mAllocator, T, mCapacity);
            OMAF_ASSERT_NOT_NULL(mData);
        }
    }
    
    template <typename T>
    Array<T>::Array(const Array<T>& array)
    : mAllocator(array.mAllocator)
    , mSize(array.mSize)
    , mCapacity(array.mCapacity)
    , mData(OMAF_NULL)
    {
        mSize = array.getSize();
        mCapacity = mSize;
        
        if(mCapacity > 0)
        {
            mData = OMAF_NEW_ARRAY(mAllocator, T, mCapacity);
            OMAF_ASSERT_NOT_NULL(mData);
            
            const T* data = array.getData();
            OMAF_ASSERT_NOT_NULL(data);
            
            CopyAscending<T>(mData, data, mSize);
        }
    }
    
    template <typename T>
    Array<T>::~Array()
    {
        if(mData != OMAF_NULL)
        {
            OMAF_DELETE_ARRAY(mAllocator, mData);
            mData = OMAF_NULL;
        }
        
        mSize = 0;
        mCapacity = 0;
    }
    
    template <typename T>
    MemoryAllocator& Array<T>::getAllocator()
    {
        return mAllocator;
    }
    
    template <typename T>
    T* Array<T>::getData()
    {
        return mData;
    }
    
    template <typename T>
    const T* Array<T>::getData() const
    {
        return mData;
    }
    
    template <typename T>
    size_t Array<T>::getCapacity() const
    {
        return mCapacity;
    }
    
    template <typename T>
    size_t Array<T>::getSize() const
    {
        return mSize;
    }
    
    template <typename T>
    bool_t Array<T>::isEmpty() const
    {
        return (mSize == 0);
    }
    
    template <typename T>
    void_t Array<T>::clear()
    {
        if(mData != NULL)
        {
            DestructArray<T>(mData, mSize);
        }
        
        mSize = 0;
    }
    
    template <typename T>
    void_t Array<T>::reserve(size_t size)
    {
        OMAF_ASSERT(size > 0, "");
        
        if(size > mCapacity)
        {
            mCapacity = size;
            
            T* data = OMAF_NEW_ARRAY(mAllocator, T, mCapacity);
            OMAF_ASSERT_NOT_NULL(data);
            
            if(mData != OMAF_NULL)
            {
                CopyAscending<T>(data, mData, mSize);
                
                OMAF_DELETE_ARRAY(mAllocator, mData);
            }
            
            mData = data;
        }
    }
    
    template <typename T>
    void_t Array<T>::resize(size_t size)
    {
        OMAF_ASSERT(size > 0, "");
        
        if(size != mSize)
        {
            if(size < mSize)
            {
                DestructArray<T>(mData + size, mSize - size);
            }
            else
            {
                reserve(size);
            }
            
            mSize = size;
        }
    }
    
    template <typename T>
    void_t Array<T>::add(const T& item)
    {
        if(mSize + 1 > mCapacity)
        {
            if (mCapacity == 0)
            {
                mCapacity = 2;
            }
            else
            {
                mCapacity *= 2;
            }
            
            T* data = OMAF_NEW_ARRAY(mAllocator, T, mCapacity);
            OMAF_ASSERT_NOT_NULL(data);
            
            if(mData != OMAF_NULL)
            {
                CopyAscending<T>(data, mData, mSize);
                
                OMAF_DELETE_ARRAY(mAllocator, mData);
            }
            
            mData = data;
        }
        
        mData[mSize] = item;
        mSize++;
    }
    
    template <typename T>
    void_t Array<T>::add(const T* array, size_t size)
    {
        if(mSize + size > mCapacity)
        {
            size_t delta = size - (mCapacity - mSize);
            mCapacity += delta;
            
            T* data = OMAF_NEW_ARRAY(mAllocator, T, mCapacity);
            OMAF_ASSERT_NOT_NULL(data);
            
            if(mData != OMAF_NULL)
            {
                CopyAscending<T>(data, mData, mSize);
                
                OMAF_DELETE_ARRAY(mAllocator, mData);
            }
            
            mData = data;
        }
        
        CopyAscending<T>(&mData[mSize], array, size);
        
        mSize += size;
    }
    
    template <typename T>
    void_t Array<T>::add(const T* array, size_t size, size_t index)
    {
        if (array == OMAF_NULL || size == 0)
        {
            return;
        }
        
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index <= mSize, ""); // End of array add
        OMAF_ASSERT(size != 0, "");
        
        size_t newSize = mSize + size;
        
        if(newSize > mCapacity)
        {
            mCapacity = newSize;
            
            T* data = OMAF_NEW_ARRAY(mAllocator, T, mCapacity);
            OMAF_ASSERT_NOT_NULL(data);
            
            // Copy new data
            CopyAscending<T>(data + index, array, size);
            
            // Copy old data
            if(mData != OMAF_NULL)
            {
                CopyAscending<T>(data, mData, index);
                CopyAscending<T>(data + index + size, mData + index, (mSize - index));
                
                OMAF_DELETE_ARRAY(mAllocator, mData); // Delete after old data is copied due to self copy case!
            }
            
            mData = data;
        }
        else
        {
            CopyDescending<T>(mData + index + size, mData + index, (mSize - index));
            CopyAscending<T>(mData + index, array, size);
        }
        
        mSize = newSize;
    }
    
    template <typename T>
    void_t Array<T>::add(const T& item, size_t index)
    {
        add(&item, 1, index);
    }
    
    template <typename T>
    void_t Array<T>::add(const Array<T>& array)
    {
        add(array.getData(), array.getSize());
    }

    template <typename T>
    bool_t Array<T>::remove(const T& item)
    {
        for(size_t i = 0; i < mSize; ++i)
        {
            if(mData[i] == item)
            {
                size_t index = i;
                size_t count = 1;
                
                DestructArray<T>(&(mData[index]), count);
                
                for(size_t j = index; j < mSize - count; ++j)
                {
                    mData[j] = mData[j + count];
                }
                
                mSize = mSize - count;
                
                if(mSize > 0)
                {
                    DestructArray<T>(&(mData[mSize]), count);
                }
                
                return true;
            }
        }
        
        return false;
    }
    
    template <typename T>
    void_t Array<T>::removeAt(size_t index, size_t count)
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < mSize, "");
        OMAF_ASSERT(count >= 1, "");
        OMAF_ASSERT(index + (count - 1) < mSize, "");

        DestructArray<T>(&(mData[index]), count);

        for (size_t i = index; i < mSize - count; ++i)
        {
            mData[i] = mData[i + count];
        }

        mSize = mSize - count;

        if (mSize > 0)
        {
            DestructArray<T>(&(mData[mSize]), count);
        }
    }

    template <typename T>
    T& Array<T>::at(size_t index)
    {
        return mData[index];
    }
    
    template <typename T>
    const T& Array<T>::at(size_t index) const
    {
        return mData[index];
    }

    template <typename T>
    T& Array<T>::back()
    {
        return mData[mSize - 1];
    }

    template <typename T>
    const T& Array<T>::back() const
    {
        return mData[mSize - 1];
    }

    template <typename T>
    T Array<T>::pop(size_t index)
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < mSize, "");

        T item = mData[index];
        removeAt(index);
        
        return item;
    }

    template <typename T>
    T& Array<T>::operator[] (size_t index)
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < mSize, "");

        return mData[index];
    }

    template <typename T>
    const T& Array<T>::operator[] (size_t index) const
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < mSize, "");

        return mData[index];
    }

    template <typename T>
    typename Array<T>::Iterator Array<T>::begin()
    {
        if (mSize > 0)
        {
            return Iterator(this, 0);
        }
        else
        {
            return Iterator(OMAF_NULL, -1);
        }
    }

    template <typename T>
    typename Array<T>::ConstIterator Array<T>::begin() const
    {
        if (mSize > 0)
        {
            return ConstIterator(this, 0);
        }
        else
        {
            return ConstIterator(OMAF_NULL, -1);
        }
    }

    template <typename T>
    typename Array<T>::Iterator Array<T>::end()
    {
        if (mSize > 0)
        {
            return Iterator(this, mSize);
        }
        else
        {
            return Iterator(OMAF_NULL, -1);
        }
    }

    template <typename T>
    typename Array<T>::ConstIterator Array<T>::end() const
    {
        if (mSize > 0)
        {
            return ConstIterator(this, mSize);
        }
        else
        {
            return ConstIterator(OMAF_NULL, -1);
        }
    }

    template <typename T>
    bool_t Array<T>::contains(const T& item) const
    {
        for (size_t i = 0; i < mSize; i++)
        {
            if (at(i) == item)
            {
                return true;
            }
        }
        
        return false;
    }

    template <typename T>
    bool_t Array<T>::contains(const Array<T>& other) const
    {
        // this need to contain all items from other to get true as result
        for (size_t i = 0; i < other.getSize(); i++)
        {
            if (!contains(other.at(i)))
            {
                return false;
            }
        }
        
        return true;
    }
OMAF_NS_END
