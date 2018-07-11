
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
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRHashFunctions.h"
#include "Foundation/NVRPair.h"
#include "Foundation/NVRArray.h"

OMAF_NS_BEGIN

    template < typename Key, typename Value, typename HashFunc = HashFunction<Key> >
    class HashMap
    {
        public:
            
            class ConstIterator
            {
                friend class HashMap;
                
                public:
                    
                    const Value& operator * () const;
                    const Value* operator -> () const;
                    
                    ConstIterator& operator ++ ();
                    ConstIterator& operator -- ();
                
                    bool_t operator == (const ConstIterator& right) const;
                    bool_t operator != (const ConstIterator& right) const;
                    
                    bool_t operator < (const ConstIterator& right) const;
                    bool_t operator > (const ConstIterator& right) const;
                    bool_t operator <= (const ConstIterator& right) const;
                    bool_t operator >= (const ConstIterator& right) const;
                    
                protected:
                
                    ConstIterator(const HashMap<Key, Value, HashFunc>* hashMap, size_t bucketIndex, size_t entryIndex);
                    
                protected:
                    
                    const HashMap<Key, Value, HashFunc>* mHashMap;
                    
                    size_t mBucketIndex;
                    size_t mEntryIndex;
            };
        
            class Iterator
            : public ConstIterator
            {
                friend class HashMap;
                
                public:
                    
                    Value& operator * () const;
                    Value* operator -> () const;
                    
                    Iterator& operator ++ ();
                    Iterator& operator -- ();
                    
                protected:
                
                    Iterator(const HashMap<Key, Value, HashFunc>* hashMap, size_t bucketIndex, size_t entryIndex);
            };
        
        public:
        
            static const size_t DefaultBucketCount = 37;
        
            static const Iterator InvalidIterator;
        
        public:

            HashMap(MemoryAllocator& allocator, size_t bucketCount = DefaultBucketCount);
            HashMap(const HashMap<Key, Value, HashFunc>& other);
        
            ~HashMap();
        
            MemoryAllocator& getAllocator() const;
        
            size_t getSize() const;
            void_t clear();

            bool_t contains(const Key& key) const;
            bool_t isEmpty() const;
        
            Iterator insert(const Key& key, const Value& value);

            Iterator update(const Key& key, const Value& value);

            bool_t remove(const Key& key);
            bool_t remove(Iterator iterator);
        
            void_t swap(HashMap& hashMap);

            Iterator find(const Key& key);
            ConstIterator find(const Key& key) const;
        
            Iterator begin();
            ConstIterator begin() const;
        
            Iterator end();
            ConstIterator end() const;

        private:
        
            OMAF_NO_DEFAULT(HashMap);
            OMAF_NO_ASSIGN(HashMap);
        
        private:
        
            void_t allocateBuckets();
        
        private:
            
            MemoryAllocator& mAllocator;
        
            typedef Pair<HashValue, Value> BucketEntry;
            typedef Array<BucketEntry> Bucket;
        
            HashFunc mHashFunction;
        
            Bucket* mBuckets;
            size_t mBucketCount;
        
            size_t mSize;
    };
OMAF_NS_END

#include "Foundation/NVRHashMapImpl.h"
