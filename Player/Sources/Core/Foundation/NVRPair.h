
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

#include "Platform/OMAFCompiler.h"

OMAF_NS_BEGIN
    template<typename FirstType, typename SecondType>
    class Pair
    {
        public:
            
            FirstType first;
            SecondType second;
            
        public:
        
            Pair()
            {
            }
        
            Pair(FirstType first, SecondType second)
            : first(first)
            , second(second)
            {
            }
        
            Pair(const Pair& other)
            : first(other.first)
            , second(other.second)
            {
            }
            
            ~Pair()
            {
            }
        
            Pair& operator = (const Pair& other)
            {
                first = other.first;
                second = other.second;
                
                return *this;
            }
        
            bool_t operator == (const Pair& other) const
            {
                return (first == other.first && second == other.second);
            }
            
            bool_t operator != (const Pair& other) const
            {
                return !(first == other.first && second == other.second);
            }
    };
OMAF_NS_END
