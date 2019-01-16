
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

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"
#include "Foundation/NVRHandle.h"

OMAF_NS_BEGIN

template <typename Type, size_t SlotBits, size_t GenerationBits, size_t N>
class HandleAllocator
{ 
    public:

        typedef GenericHandle<Type, SlotBits, GenerationBits> HandleType;
        
    public:
        
        static const Type INVALID_GENERATION = 0;
        static const Type DEFAULT_GENERATION = 1;
        
        static const Type NUM_SLOTS = (1 << SlotBits);
        static const Type NUM_GENERATIONS = (1 << GenerationBits);
        
        HandleAllocator()
        : mNumFreeHandles(N)
        {
            OMAF_ASSERT(N <= NUM_SLOTS, "");
                
            for (size_t index = 0; index < N; ++index)
            {
                HandleType& handle = mFreeHandles[index];
                handle.index = (Type)index;
                handle.generation = INVALID_GENERATION;
                    
                mGenerations[index] = DEFAULT_GENERATION;
            }
        }
            
        ~HandleAllocator()
        {
        }
            
        HandleType allocate()
        {
            if (mNumFreeHandles > 0)
            {
                HandleType& handle = mFreeHandles[mNumFreeHandles - 1];
                handle.generation = mGenerations[handle.index];
                    
                --mNumFreeHandles;
                    
                return handle;
            }
                
            return HandleType::Invalid;
        }
            
        void_t release(HandleType handle)
        {
            if (isValid(handle))
            {
                mFreeHandles[mNumFreeHandles] = HandleType(handle.index, INVALID_GENERATION);
                    
                mNumFreeHandles++;
                mGenerations[handle.index] = (mGenerations[handle.index] + 1) % NUM_GENERATIONS;
                    
                if (mGenerations[handle.index] == INVALID_GENERATION)
                {
                    mGenerations[handle.index] = DEFAULT_GENERATION;
                }
            }
        }
            
        bool_t isValid(HandleType handle)
        {
            return (mGenerations[handle.index] == handle.generation);
        }

        void_t reset()
        {
            for (size_t index = 0; index < N; ++index)
            {
                HandleType& handle = mFreeHandles[index];
                handle.index = (Type)index;
                handle.generation = INVALID_GENERATION;

                mGenerations[index] = DEFAULT_GENERATION;
            }
        }
            
    private:
            
        Type mNumFreeHandles;
        HandleType mFreeHandles[N];
            
        Type mGenerations[N];
};

OMAF_NS_END
