
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
#include "Foundation/NVRConstruct.h"
#include "Foundation/NVRMemoryUtilities.h"

OMAF_NS_BEGIN
    template <typename T, size_t N>
    const T& FixedArray<T, N>::ConstIterator::operator * () const
    {
        OMAF_ASSERT_NOT_NULL(mArray);
        OMAF_ASSERT(mIndex < mArray->getSize(), "");
        
        return mArray->at(mIndex);
    }
    
    template <typename T, size_t N>
    const T* FixedArray<T, N>::ConstIterator::operator -> () const
    {
        OMAF_ASSERT_NOT_NULL(mArray);
        OMAF_ASSERT(mIndex < mArray->getSize(), "");
        
        return &mArray->at(mIndex);
    }
    
    template <typename T, size_t N>
    typename FixedArray<T, N>::ConstIterator& FixedArray<T, N>::ConstIterator::operator ++ ()
    {
        ++mIndex;
        
        return *this;
    }
    
    template <typename T, size_t N>
    typename FixedArray<T, N>::ConstIterator& FixedArray<T, N>::ConstIterator::operator -- ()
    {
        --mIndex;
        
        return *this;
    }
    
    template <typename T, size_t N>
    bool_t FixedArray<T, N>::ConstIterator::operator == (const ConstIterator& right) const
    {
        return (mIndex == right.mIndex);
    }
    
    template <typename T, size_t N>
    bool_t FixedArray<T, N>::ConstIterator::operator != (const ConstIterator& right) const
    {
        return (mIndex != right.mIndex);
    }
    
    template <typename T, size_t N>
    bool_t FixedArray<T, N>::ConstIterator::operator < (const ConstIterator& right) const
    {
        return (mIndex < right.mIndex);
    }
    
    template <typename T, size_t N>
    bool_t FixedArray<T, N>::ConstIterator::operator > (const ConstIterator& right) const
    {
        return (mIndex > right.mIndex);
    }
    
    template <typename T, size_t N>
    bool_t FixedArray<T, N>::ConstIterator::operator <= (const ConstIterator& right) const
    {
        return (mIndex <= right.mIndex);
    }
    
    template <typename T, size_t N>
    bool_t FixedArray<T, N>::ConstIterator::operator >= (const ConstIterator& right) const
    {
        return (mIndex >= right.mIndex);
    }
    
    template <typename T, size_t N>
    FixedArray<T, N>::ConstIterator::ConstIterator(const FixedArray<T, N>* array, size_t index)
    : mArray(const_cast<FixedArray<T, N>*>(array))
    , mIndex(index)
    {
    }
    
    
    template <typename T, size_t N>
    T& FixedArray<T, N>::Iterator::operator * () const
    {
        OMAF_ASSERT_NOT_NULL(this->mArray);
        OMAF_ASSERT(this->mIndex < this->mArray->getSize(), "");
        
        return this->mArray->at(this->mIndex);
    }
    
    template <typename T, size_t N>
    T* FixedArray<T, N>::Iterator::operator -> () const
    {
        OMAF_ASSERT_NOT_NULL(this->mArray);
        OMAF_ASSERT(this->mIndex < this->mArray->getSize(), "");
        
        return &this->mArray->at(this->mIndex);
    }
    
    template <typename T, size_t N>
    typename FixedArray<T, N>::Iterator& FixedArray<T, N>::Iterator::operator ++ ()
    {
        OMAF_ASSERT_NOT_NULL(this->mArray);
        
        ++this->mIndex;
        
        return *this;
    }
    
    template <typename T, size_t N>
    typename FixedArray<T, N>::Iterator& FixedArray<T, N>::Iterator::operator -- ()
    {
        OMAF_ASSERT_NOT_NULL(this->mArray);
        
        --this->mIndex;
        
        return *this;
    }
    
    template <typename T, size_t N>
    FixedArray<T, N>::Iterator::Iterator(FixedArray<T, N>* array, size_t index)
    : FixedArray<T, N>::ConstIterator(array, index)
    {
    }
    
    
    template <typename T, size_t N>
    typename FixedArray<T, N>::Iterator FixedArray<T, N>::InvalidIterator(OMAF_NULL, -1);
    
    
    template <typename T, size_t N>
    FixedArray<T, N>::FixedArray()
    : mSize(0)
    {
    }
    
    template <typename T, size_t N>
    FixedArray<T, N>::FixedArray(const FixedArray<T, N>& array)
    : mSize(0)
    {
        add(array.getData(), array.getSize());
    }
    
    template <typename T, size_t N>
    FixedArray<T, N>::~FixedArray()
    {
        DestructArray<T>(mData, mSize);
        
        mSize = 0;
    }

    template <typename T, size_t N>
    T* FixedArray<T, N>::getData()
    {
        return mData;
    }

    template <typename T, size_t N>
    const T* FixedArray<T, N>::getData() const
    {
        return mData;
    }

    template <typename T, size_t N>
    size_t FixedArray<T, N>::getCapacity() const
    {
        return N;
    }
    
    template <typename T, size_t N>
    size_t FixedArray<T, N>::getSize() const
    {
        return mSize;
    }
    
    template <typename T, size_t N>
    bool_t FixedArray<T, N>::isEmpty() const
    {
        return (mSize == 0);
    }
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::resize(size_t size)
    {
        OMAF_ASSERT(size <= N, "");
        
        mSize = size;
    }
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::clear()
    {
        if(mSize > 0)
        {
            DestructArray<T>(mData, mSize);
        }
        
        mSize = 0;
    }
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::add(const T& item)
    {
        OMAF_ASSERT(mSize + 1 <= N, "");
        
        Construct<T>(&mData[mSize]);
        
        mData[mSize] = item;
        mSize++;
    }
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::add(const T* array, size_t size)
    {
        OMAF_ASSERT(mSize + size <= N, "");
        
        CopyAscending<T>(&mData[mSize], array, size);
        
        mSize += size;
    }
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::add(const T* array, size_t size, size_t index)
    {
        OMAF_ASSERT_NOT_NULL(array);
        
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index <= mSize, "");
        OMAF_ASSERT(size != 0, "");
        OMAF_ASSERT(mSize + size <= N, "");
        
        CopyDescending<T>(mData + index + size, mData + index, (mSize - index));
        CopyAscending<T>(mData + index, array, size);
        
        mSize = mSize + size;
    }
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::add(const T& item, size_t index)
    {
        add(&item, 1, index);
    }
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::add(const FixedArray<T, N>& array)
    {
        OMAF_ASSERT(mSize + array.getSize() <= N, "");
        
        size_t size = array.getSize();

        CopyAscending<T>(&mData[mSize], array.getData(), size);
        
        mSize += size;
    }
    
    template <typename T, size_t N>
    bool_t FixedArray<T, N>::remove(const T& item)
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
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::remove(const ConstIterator& iterator)
    {
        OMAF_ASSERT_NOT_NULL(iterator.mArray);
        
        return removeAt(iterator.mIndex, 1);
    }
    
    template <typename T, size_t N>
    void_t FixedArray<T, N>::removeAt(size_t index, size_t count)
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < mSize, "");
        OMAF_ASSERT(count >= 1, "");
        OMAF_ASSERT(index + (count - 1) < mSize, "");
        
        DestructArray<T>(&(mData[index]), count);
        
        for(size_t i = index; i < mSize - count; ++i)
        {
            mData[i] = mData[i + count];
        }
        
        mSize = mSize - count;
        
        if(mSize > 0)
        {
            DestructArray<T>(&(mData[mSize]), count);
        }
    }
    
    template <typename T, size_t N>
    T& FixedArray<T, N>::operator[] (size_t index)
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < mSize, "");
        
        return mData[index];
    }
    
    template <typename T, size_t N>
    const T& FixedArray<T, N>::operator[] (size_t index) const
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < mSize, "");
        
        return mData[index];
    }
    
    template <typename T, size_t N>
    T& FixedArray<T, N>::at(size_t index)
    {
        OMAF_ASSERT(index >= 0, "Index out of range");
        OMAF_ASSERT(index < mSize, "Index out of range");
        return mData[index];
    }
    
    template <typename T, size_t N>
    const T& FixedArray<T, N>::at(size_t index) const
    {
        OMAF_ASSERT(index >= 0, "Index out of range");
        OMAF_ASSERT(index < mSize, "Index out of range");
        return mData[index];
    }

    template <typename T, size_t N>
    T& FixedArray<T, N>::back()
    {
        OMAF_ASSERT(mSize > 0, "Index out of range");
        return mData[mSize - 1];
    }

    template <typename T, size_t N>
    const T& FixedArray<T, N>::back() const
    {
        OMAF_ASSERT(mSize > 0, "Index out of range");
        return mData[mSize - 1];
    }
    
    template <typename T, size_t N>
    T& FixedArray<T, N>::front()
    {
        OMAF_ASSERT(mSize > 0, "Index out of range");
        return mData[0];
    }
    
    template <typename T, size_t N>
    const T& FixedArray<T, N>::front() const
    {
        OMAF_ASSERT(mSize > 0, "Index out of range");
        return mData[0];
    }

    template <typename T, size_t N>
    typename FixedArray<T, N>::Iterator FixedArray<T, N>::begin()
    {
        if(mSize > 0)
        {
            return Iterator(this, 0);
        }
        else
        {
            return Iterator(OMAF_NULL, (size_t)-1);
        }
    }
    
    template <typename T, size_t N>
    typename FixedArray<T, N>::ConstIterator FixedArray<T, N>::begin() const
    {
        if(mSize > 0)
        {
            return ConstIterator(this, 0);
        }
        else
        {
            return ConstIterator(OMAF_NULL, (size_t)-1);
        }
    }
    
    template <typename T, size_t N>
    typename FixedArray<T, N>::Iterator FixedArray<T, N>::end()
    {
        if(mSize > 0)
        {
            return Iterator(this, mSize);
        }
        else
        {
            return Iterator(OMAF_NULL, (size_t)-1);
        }
    }
    
    template <typename T, size_t N>
    typename FixedArray<T, N>::ConstIterator FixedArray<T, N>::end() const
    {
        if(mSize > 0)
        {
            return ConstIterator(this, mSize);
        }
        else
        {
            return ConstIterator(OMAF_NULL, (size_t)-1);
        }
    }

    template <typename T, size_t N>
    bool_t FixedArray<T, N>::contains(const T& item) const
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

    template <typename T, size_t N>
    bool_t FixedArray<T, N>::contains(const FixedArray<T, N>& other) const
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

    template <typename T, size_t N>
    FixedArray<T, N> FixedArray<T, N>::intersection(const FixedArray<T, N>& other) const
    {
        FixedArray<T, N> result;
        typename FixedArray<T, N>::ConstIterator firstIt = begin();
        typename FixedArray<T, N>::ConstIterator firstEndIt = end();
        typename FixedArray<T, N>::ConstIterator secondIt = other.begin();
        typename FixedArray<T, N>::ConstIterator secondEndIt = other.end();

        while (firstIt != firstEndIt && secondIt != secondEndIt)
        {
            if (*firstIt < *secondIt)
            {
                ++firstIt;
            }
            else if (*secondIt < *firstIt)
            {
                ++secondIt;
            }
            else
            {
                result.add(*firstIt);
                ++firstIt;
                ++secondIt;
            }
        }

        return result;
    }

    template <typename T, size_t N>
    FixedArray<T, N> FixedArray<T, N>::difference(const FixedArray<T, N>& other) const
    {
        FixedArray<T, N> result;
        typename FixedArray<T, N>::ConstIterator firstIt = begin();
        typename FixedArray<T, N>::ConstIterator firstEndIt = end();

        for ( ; firstIt != firstEndIt; ++firstIt)
        {
            if (!other.contains(*firstIt))
            {
                result.add(*firstIt);
            }
        }

        return result;
    }

    template <typename T, size_t N>
    FixedArray<T, N>& FixedArray<T, N>::operator = (const FixedArray<T, N>& other)
    {
        OMAF_ASSERT(getCapacity() >= other.getCapacity(), "");

        clear();
        add(other);

        return *this;
    }

    template <typename T, size_t N>
    bool_t FixedArray<T, N>::operator == (const FixedArray<T, N>& other) const
    {
        return getSize() == other.getSize() && contains(other);
    }

    template <typename T, size_t N>
    bool_t FixedArray<T, N>::operator != (const FixedArray<T, N>& other) const
    {
        return getSize() != other.getSize() || !contains(other);
    }
OMAF_NS_END
