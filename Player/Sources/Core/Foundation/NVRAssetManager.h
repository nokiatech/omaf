
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
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRPathName.h"
#include "Foundation/NVRFileSystem.h"

OMAF_NS_BEGIN
    namespace AssetManager
    {
        typedef void_t* FileHandle;
        extern FileHandle InvalidFileHandle;

        bool_t FileExists(const char_t* path);
        bool_t DirExists(const char_t* path);
        
        PathName GetFullPath(const char_t* filename);
        
        FileHandle Open(const char_t* filename, FileSystem::AccessMode::Enum mode);
        void_t Close(FileHandle handle);

        int64_t Read(FileHandle handle, void_t* destination, int64_t bytes);

        bool_t Seek(FileHandle handle, int64_t offset);
        int64_t Tell(FileHandle handle);
        
        bool_t IsOpen(FileHandle handle);
        
        int64_t GetSize(FileHandle handle);
    }
OMAF_NS_END
