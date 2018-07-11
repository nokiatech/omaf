
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

#include "Platform/OMAFDataTypes.h"
#include "Platform/OMAFCompiler.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRTraits.h"

OMAF_NS_BEGIN

    template <typename T>
    T* Construct(void_t* ptr)
    {
        OMAF_ASSERT_NOT_NULL(ptr);
        
        if (Traits::isPOD<T>::Value)
        {
            return (T*)ptr;
        }
        
        if(ptr != OMAF_NULL)
        {
            return new(ptr) T;
        }

        return OMAF_NULL;
    }
    
    template <typename T>
    T* ConstructArray(void_t* ptr, size_t count)
    {
        OMAF_ASSERT_NOT_NULL(ptr);
        
        if (Traits::isPOD<T>::Value)
        {
            return (T*)ptr;
        }
        
        if(ptr != OMAF_NULL)
        {
            T* array = (T*)ptr;
            T* object = array;
            
            for(size_t i = 0; i < count; ++i)
            {
                new(object) T;
                ++object;
            }
            
            return array;
        }

        return OMAF_NULL;
    }
    
    template <typename T>
    void_t Destruct(void_t* ptr)
    {
        OMAF_ASSERT_NOT_NULL(ptr);
        
        if (Traits::isPOD<T>::Value)
        {
            return;
        }
        
        if(ptr != OMAF_NULL)
        {
            T* object = (T*)ptr;
            object->~T();
        }
    }
    
    template <typename T>
    void_t DestructArray(void_t* ptr, size_t count)
    {
        OMAF_ASSERT_NOT_NULL(ptr);
        
        if (Traits::isPOD<T>::Value)
        {
            return;
        }
        
        T* array = (T*)ptr;
        
        for(size_t i = 0; i < count; ++i)
        {
            T* object = &array[i];
            object->~T();
        }
    }

OMAF_NS_END
