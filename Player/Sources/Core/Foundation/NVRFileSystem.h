
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
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
    class FileStream;
    
    namespace FileSystem
    {
        namespace AccessMode
        {
            enum Enum
            {
                INVALID = -1,
                
                READ = 1 << 0,
                WRITE = 1 << 1,
                READ_WRITE = READ | WRITE,
                
                COUNT
            };
        }
        
        namespace CreateMode
        {
            enum Enum
            {
                INVALID = -1,
                
                CREATE,
                APPEND,
                
                COUNT
            };
        }
        
        namespace StreamType
        {
            enum Enum
            {
                INVALID = -1,
                
                ASSET,
                STORAGE,
                FILE,

                COUNT
            };
        }

        bool_t fileExists(const char_t* path);
        bool_t dirExists(const char_t* path);

        FileStream* open(const char_t* filename, AccessMode::Enum mode);
        void_t close(FileStream* fileStream);
    }
OMAF_NS_END
