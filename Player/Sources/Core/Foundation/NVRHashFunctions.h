
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
#pragma once

#include "Foundation/NVRCompatibility.h"
#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
    typedef uint64_t HashValue;
    
    // A hash function for integral types.
    template <typename T>
    class HashFunction
    {
        public:
        
            OMAF_INLINE HashValue operator () (const T& key) const;
    };
    
    // A hash function for pointer types.
    template <typename T>
    class HashFunction<T*>
    {
        public:
        
            OMAF_INLINE HashValue operator () (const T* key) const;
    };
    
    // A hash function for pointer types.
    template <typename T>
    class HashFunction<const T*>
    {
        public:
        
            OMAF_INLINE HashValue operator () (const T* key) const;
    };
    
    // A hash function for string type.
    template <>
    class HashFunction<char_t*>
    {
        public:
        
            HashValue operator () (const char_t* key) const;
    };
    
    // A hash function for string type.
    template <>
    class HashFunction<const char_t*>
    {
        public:
        
            HashValue operator () (const char_t* key) const;
    };
    
    // A hash function for float32 type.
    template <>
    class HashFunction<float32_t>
    {
        public:
        
            OMAF_INLINE HashValue operator () (float32_t key) const;
    };
    
    // A hash function for float64 type.
    template <>
    class HashFunction<float64_t>
    {
        public:
        
            OMAF_INLINE HashValue operator () (float64_t key) const;
    };
OMAF_NS_END

#include "Foundation/NVRHashFunctionsImpl.h"
