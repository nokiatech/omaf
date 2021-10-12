
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
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRMemoryUtilities.h"
#include "Foundation/NVRNew.h"
#include "Math/OMAFMathFunctions.h"

OMAF_NS_BEGIN
template <typename Key, typename Value, typename HashFunction>
const Value& HashMap<Key, Value, HashFunction>::ConstIterator::operator*() const
{
    OMAF_ASSERT_NOT_NULL(mHashMap);
    OMAF_ASSERT_NOT_NULL(mHashMap->mBuckets);

    Bucket* buckets = mHashMap->mBuckets;
    Bucket& bucket = buckets[mBucketIndex];

    return bucket.at(mEntryIndex).second;
}

template <typename Key, typename Value, typename HashFunction>
const Value* HashMap<Key, Value, HashFunction>::ConstIterator::operator->() const
{
    OMAF_ASSERT_NOT_NULL(mHashMap);
    OMAF_ASSERT_NOT_NULL(mHashMap->mBuckets);

    Bucket* buckets = mHashMap->mBuckets;
    Bucket& bucket = buckets[mBucketIndex];

    return &bucket.at(mEntryIndex).second;
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::ConstIterator& HashMap<Key, Value, HashFunction>::ConstIterator::
operator++()
{
    OMAF_ASSERT_NOT_NULL(mHashMap);
    OMAF_ASSERT(mBucketIndex < mHashMap->mBucketCount, "");

    ++mEntryIndex;

    Bucket* buckets = mHashMap->mBuckets;
    OMAF_ASSERT_NOT_NULL(buckets);

    Bucket& bucket = buckets[mBucketIndex];

    if (mEntryIndex >= bucket.getSize())
    {
        mEntryIndex = 0;

        size_t bucketCount = mHashMap->mBucketCount;

        for (size_t bucketIndex = mBucketIndex + 1; bucketIndex < bucketCount; ++bucketIndex)
        {
            if (!buckets[bucketIndex].isEmpty())
            {
                mBucketIndex = bucketIndex;

                return *this;
            }
        }

        mBucketIndex = bucketCount;
    }

    return *this;
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::ConstIterator& HashMap<Key, Value, HashFunction>::ConstIterator::
operator--()
{
    OMAF_ASSERT_NOT_NULL(mHashMap);
    OMAF_ASSERT(mBucketIndex < mHashMap->mBucketCount, "");

    if (mEntryIndex > 0)
    {
        --mEntryIndex;

        return *this;
    }

    Bucket* buckets = mHashMap->mBuckets;
    OMAF_ASSERT_NOT_NULL(buckets);

    size_t bucketIndex = mBucketIndex;

    while (bucketIndex != 0)
    {
        --bucketIndex;

        Bucket& bucket = buckets[bucketIndex];
        size_t entryCount = bucket.getSize();

        if (entryCount != 0)
        {
            mBucketIndex = bucketIndex;
            mEntryIndex = entryCount - 1;

            return *this;
        }
    }

    return *this;
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::ConstIterator::operator==(const ConstIterator& right) const
{
    return (mHashMap == right.mHashMap) && (mBucketIndex == right.mBucketIndex) && (mEntryIndex == right.mEntryIndex);
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::ConstIterator::operator!=(const ConstIterator& right) const
{
    return !(mHashMap == right.mHashMap && mBucketIndex == right.mBucketIndex && mEntryIndex == right.mEntryIndex);
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::ConstIterator::operator<(const ConstIterator& right) const
{
    return (mHashMap < right.mHashMap ||
            (mHashMap == right.mHashMap &&
             (mBucketIndex < right.mBucketIndex ||
              (mBucketIndex == right.mBucketIndex && mEntryIndex < right.mEntryIndex))));
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::ConstIterator::operator>(const ConstIterator& right) const
{
    return (mHashMap > right.mHashMap ||
            (mHashMap == right.mHashMap &&
             (mBucketIndex > right.mBucketIndex ||
              (mBucketIndex == right.mBucketIndex && mEntryIndex > right.mEntryIndex))));
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::ConstIterator::operator<=(const ConstIterator& right) const
{
    return (mHashMap < right.mHashMap ||
            (mHashMap == right.mHashMap &&
             (mBucketIndex < right.mBucketIndex ||
              (mBucketIndex == right.mBucketIndex && mEntryIndex <= right.mEntryIndex))));
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::ConstIterator::operator>=(const ConstIterator& right) const
{
    return (mHashMap > right.mHashMap ||
            (mHashMap == right.mHashMap &&
             (mBucketIndex > right.mBucketIndex ||
              (mBucketIndex == right.mBucketIndex && mEntryIndex >= right.mEntryIndex))));
}

template <typename Key, typename Value, typename HashFunction>
HashMap<Key, Value, HashFunction>::ConstIterator::ConstIterator(const HashMap<Key, Value, HashFunction>* hashMap,
                                                                size_t bucketIndex,
                                                                size_t entryIndex)
    : mHashMap(hashMap)
    , mBucketIndex(bucketIndex)
    , mEntryIndex(entryIndex)
{
}


template <typename Key, typename Value, typename HashFunction>
Value& HashMap<Key, Value, HashFunction>::Iterator::operator*() const
{
    OMAF_ASSERT_NOT_NULL(this->mHashMap);
    OMAF_ASSERT_NOT_NULL(this->mHashMap->mBuckets);

    Bucket* buckets = this->mHashMap->mBuckets;
    Bucket& bucket = buckets[this->mBucketIndex];

    return bucket.at(this->mEntryIndex).second;
}

template <typename Key, typename Value, typename HashFunction>
Value* HashMap<Key, Value, HashFunction>::Iterator::operator->() const
{
    OMAF_ASSERT_NOT_NULL(this->mHashMap);
    OMAF_ASSERT_NOT_NULL(this->mHashMap->mBuckets);

    Bucket* buckets = this->mHashMap->mBuckets;
    Bucket& bucket = buckets[this->mBucketIndex];

    return &bucket.at(this->mEntryIndex).second;
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::Iterator& HashMap<Key, Value, HashFunction>::Iterator::operator++()
{
    OMAF_ASSERT_NOT_NULL(this->mHashMap);
    OMAF_ASSERT(this->mBucketIndex < this->mHashMap->mBucketCount, "");

    ++this->mEntryIndex;

    Bucket* buckets = this->mHashMap->mBuckets;
    OMAF_ASSERT_NOT_NULL(buckets);

    Bucket& bucket = buckets[this->mBucketIndex];

    if (this->mEntryIndex >= bucket.getSize())
    {
        this->mEntryIndex = 0;

        size_t bucketCount = this->mHashMap->mBucketCount;

        for (size_t bucketIndex = this->mBucketIndex + 1; bucketIndex < bucketCount; ++bucketIndex)
        {
            if (!buckets[bucketIndex].isEmpty())
            {
                this->mBucketIndex = bucketIndex;

                return *this;
            }
        }

        this->mBucketIndex = bucketCount;
    }

    return *this;
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::Iterator& HashMap<Key, Value, HashFunction>::Iterator::operator--()
{
    OMAF_ASSERT_NOT_NULL(this->mHashMap);
    OMAF_ASSERT(this->mBucketIndex < this->mHashMap->mBucketCount, "");

    if (this->mEntryIndex > 0)
    {
        --this->mEntryIndex;

        return *this;
    }

    Bucket* buckets = this->mHashMap->mBuckets;
    OMAF_ASSERT_NOT_NULL(buckets);

    size_t bucketIndex = this->mBucketIndex;

    while (bucketIndex != 0)
    {
        --bucketIndex;

        Bucket& bucket = buckets[bucketIndex];
        size_t entryCount = bucket.getSize();

        if (entryCount != 0)
        {
            this->mBucketIndex = bucketIndex;
            this->mEntryIndex = entryCount - 1;

            return *this;
        }
    }

    return *this;
}

template <typename Key, typename Value, typename HashFunction>
HashMap<Key, Value, HashFunction>::Iterator::Iterator(const HashMap<Key, Value, HashFunction>* hashMap,
                                                      size_t bucketIndex,
                                                      size_t entryIndex)
    : HashMap<Key, Value, HashFunction>::ConstIterator(hashMap, bucketIndex, entryIndex)
{
}


template <typename Key, typename Value, typename HashFunction>
const typename HashMap<Key, Value, HashFunction>::Iterator HashMap<Key, Value, HashFunction>::InvalidIterator(OMAF_NULL,
                                                                                                              0,
                                                                                                              0);


template <typename Key, typename Value, typename HashFunction>
HashMap<Key, Value, HashFunction>::HashMap(MemoryAllocator& allocator, size_t bucketCount)
    : mAllocator(allocator)
    , mHashFunction()
    , mBuckets(OMAF_NULL)
    , mBucketCount(max<size_t>(bucketCount, 1))
    , mSize(0)
{
    allocateBuckets();
}

template <typename Key, typename Value, typename HashFunction>
HashMap<Key, Value, HashFunction>::HashMap(const HashMap<Key, Value, HashFunction>& other)
    : mAllocator(other.mAllocator)
    , mHashFunction(other.mHashFunction)
    , mBuckets(OMAF_NULL)
    , mBucketCount(other.mBucketCount)
    , mSize(other.mSize)
{
    allocateBuckets();

    for (size_t bucketIndex = 0; bucketIndex < mBucketCount; ++mBucketCount)
    {
        mBuckets[bucketIndex] = other.mBuckets[bucketIndex];
    }
}

template <typename Key, typename Value, typename HashFunction>
HashMap<Key, Value, HashFunction>::~HashMap()
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    for (size_t bucketIndex = 0; bucketIndex < mBucketCount; ++bucketIndex)
    {
        Bucket* bucket = &mBuckets[bucketIndex];
        Destruct<Bucket>(bucket);
    }

    OMAF_FREE(mAllocator, mBuckets);
    mBuckets = OMAF_NULL;
}

template <typename Key, typename Value, typename HashFunction>
MemoryAllocator& HashMap<Key, Value, HashFunction>::getAllocator() const
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    return mAllocator;
}

template <typename Key, typename Value, typename HashFunction>
size_t HashMap<Key, Value, HashFunction>::getSize() const
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    return mSize;
}

template <typename Key, typename Value, typename HashFunction>
void_t HashMap<Key, Value, HashFunction>::clear()
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    for (size_t bucketIndex = 0; bucketIndex < mBucketCount; ++bucketIndex)
    {
        mBuckets[bucketIndex].clear();
    }

    mSize = 0;
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::contains(const Key& key) const
{
    OMAF_ASSERT_NOT_NULL(mBuckets);
    return find(key) != InvalidIterator;
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::isEmpty() const
{
    return mSize == 0;
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::Iterator HashMap<Key, Value, HashFunction>::insert(const Key& key,
                                                                                               const Value& value)
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    const HashValue hashKey = mHashFunction(key);
    size_t bucketIndex = hashKey % mBucketCount;

    Bucket& entries = mBuckets[bucketIndex];
    size_t entryCount = entries.getSize();

    // Check if key already exists
    for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        if (entries[entryIndex].first == hashKey)
        {
            return InvalidIterator;
        }
    }

    // Key didn't already exists, add it to the table
    entries.add(BucketEntry(hashKey, value));
    ++mSize;

    return Iterator(this, bucketIndex, entryCount);
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::Iterator HashMap<Key, Value, HashFunction>::update(const Key& key,
                                                                                               const Value& value)
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    const HashValue hashKey = mHashFunction(key);
    size_t bucketIndex = hashKey % mBucketCount;

    Bucket& entries = mBuckets[bucketIndex];
    size_t entryCount = entries.getSize();

    // Check if key already exists
    for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        if (entries[entryIndex].first == hashKey)
        {
            entries[entryIndex].second = value;
            return Iterator(this, bucketIndex, entryCount);
        }
    }

    // Key didn't already exists, add it to the table
    entries.add(BucketEntry(hashKey, value));
    ++mSize;

    return Iterator(this, bucketIndex, entryCount);
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::remove(const Key& key)
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    const HashValue hashKey = mHashFunction(key);
    size_t bucketIndex = hashKey % mBucketCount;

    Bucket& entries = mBuckets[bucketIndex];
    size_t entryCount = entries.getSize();

    // Check if key founds and remove element
    for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        if (entries[entryIndex].first == hashKey)
        {
            entries.removeAt(entryIndex);
            --mSize;

            return true;
        }
    }

    // Key didn't found
    return false;
}

template <typename Key, typename Value, typename HashFunction>
bool_t HashMap<Key, Value, HashFunction>::remove(Iterator iterator)
{
    OMAF_ASSERT_NOT_NULL(mBuckets);
    OMAF_ASSERT(iterator.mHashMap == this, "Iterator is pointing to a different container instance");

    // Check if iterator is valid
    if (iterator.mBucketIndex >= mBucketCount)
    {
        return false;
    }

    Bucket& bucket = mBuckets[iterator.mBucketIndex];

    if (iterator.mEntryIndex >= bucket.getSize())
    {
        return false;
    }

    // Iterator was valid, remove element
    bucket.removeAt(iterator.mEntryIndex);

    --mSize;

    return true;
}

template <typename Key, typename Value, typename HashFunction>
void_t HashMap<Key, Value, HashFunction>::swap(HashMap& other)
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    MemoryAllocator& allocator = mAllocator;
    HashFunction hashFunction = mHashFunction;
    Bucket* buckets = mBuckets;
    size_t bucketCount = mBucketCount;
    size_t size = mSize;

    mAllocator = other.getAllocator();
    mHashFunction = other.mHashFunction;
    mBuckets = other.mBuckets;
    mBucketCount = other.mBucketCount;
    mSize = other.mSize;

    other.mAllocator = allocator;
    other.mHashFunction = hashFunction;
    other.mBuckets = buckets;
    other.mBucketCount = bucketCount;
    other.mSize = size;
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::Iterator HashMap<Key, Value, HashFunction>::find(const Key& key)
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    if (mSize == 0)
    {
        return InvalidIterator;
    }

    const HashValue hashKey = mHashFunction(key);
    size_t bucketIndex = hashKey % mBucketCount;

    Bucket& entries = mBuckets[bucketIndex];
    size_t entryCount = entries.getSize();

    for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        if (entries[entryIndex].first == hashKey)
        {
            return Iterator(this, bucketIndex, entryIndex);
        }
    }

    return InvalidIterator;
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::ConstIterator HashMap<Key, Value, HashFunction>::find(const Key& key) const
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    if (mSize != 0)
    {
        const HashValue hashKey = mHashFunction(key);
        size_t bucketIndex = hashKey % mBucketCount;

        Bucket& entries = mBuckets[bucketIndex];
        size_t entryCount = entries.getSize();

        for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
        {
            if (entries[entryIndex].first == hashKey)
            {
                return ConstIterator(this, bucketIndex, entryIndex);
            }
        }
    }

    return InvalidIterator;
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::Iterator HashMap<Key, Value, HashFunction>::begin()
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    if (mSize != 0)
    {
        size_t bucketCount = mBucketCount;

        for (size_t bucketIndex = 0; bucketIndex < bucketCount; ++bucketIndex)
        {
            if (!mBuckets[bucketIndex].isEmpty())
            {
                return Iterator(this, bucketIndex, 0);
            }
        }
    }

    return end();
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::ConstIterator HashMap<Key, Value, HashFunction>::begin() const
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    if (mSize != 0)
    {
        size_t bucketCount = mBucketCount;

        for (size_t bucketIndex = 0; bucketIndex < bucketCount; ++bucketIndex)
        {
            if (!mBuckets[bucketIndex].isEmpty())
            {
                return ConstIterator(this, bucketIndex, 0);
            }
        }
    }

    return end();
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::Iterator HashMap<Key, Value, HashFunction>::end()
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    return Iterator(this, mBucketCount, 0);
}

template <typename Key, typename Value, typename HashFunction>
typename HashMap<Key, Value, HashFunction>::ConstIterator HashMap<Key, Value, HashFunction>::end() const
{
    OMAF_ASSERT_NOT_NULL(mBuckets);

    return ConstIterator(this, mBucketCount, 0);
}

template <typename Key, typename Value, typename HashFunction>
void_t HashMap<Key, Value, HashFunction>::allocateBuckets()
{
    OMAF_ASSERT(mBuckets == OMAF_NULL, "Buckets already allocated");

    mBuckets = (Bucket*) OMAF_ALLOC(mAllocator, OMAF_SIZE_OF(Bucket) * mBucketCount);

    for (size_t bucketIndex = 0; bucketIndex < mBucketCount; ++bucketIndex)
    {
        Bucket* bucket = &mBuckets[bucketIndex];
        bucket = new (bucket) Bucket(mAllocator);
    }

    OMAF_ASSERT_NOT_NULL(mBuckets);
}
OMAF_NS_END
