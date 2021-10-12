
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
#include "Foundation/NVRFileSystem.h"

#include "Foundation/NVRAssetFileStream.h"
#include "Foundation/NVRAssetManager.h"
#include "Foundation/NVRDiskFileStream.h"
#include "Foundation/NVRDiskManager.h"
#include "Foundation/NVRFileStream.h"
#include "Foundation/NVRFileUtilities.h"
#include "Foundation/NVRMemorySystem.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRStorageFileStream.h"
#include "Foundation/NVRStorageManager.h"

OMAF_NS_BEGIN
bool_t FileSystem::fileExists(const char_t* path)
{
    PathName scheme = getUriScheme(path);
    PathName filepath = stripUriScheme(path);

    if (scheme == SCHEME_PREFIX_ASSET)
    {
        return AssetManager::FileExists(filepath);
    }
    else if (scheme == SCHEME_PREFIX_STORAGE)
    {
        return StorageManager::FileExists(filepath);
    }
    else if (scheme == SCHEME_PREFIX_FILE)
    {
        filepath = stripFirstFolder(filepath);
        return DiskManager::FileExists(filepath);
    }
    else
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();
    }

    return false;
}

bool_t FileSystem::dirExists(const char_t* path)
{
    PathName scheme = getUriScheme(path);
    PathName filepath = stripUriScheme(path);

    if (scheme == SCHEME_PREFIX_ASSET)
    {
        return AssetManager::DirExists(filepath);
    }
    else if (scheme == SCHEME_PREFIX_STORAGE)
    {
        return StorageManager::DirExists(filepath);
    }
    else if (scheme == SCHEME_PREFIX_FILE)
    {
        filepath = stripFirstFolder(filepath);
        return StorageManager::DirExists(filepath);
    }
    else
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();
    }

    return false;
}

FileStream* FileSystem::open(const char_t* filename, AccessMode::Enum mode)
{
    PathName scheme = getUriScheme(filename);
    PathName filepath = stripUriScheme(filename);

    MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

    if (scheme == SCHEME_PREFIX_ASSET)
    {
        AssetFileStream* stream = OMAF_NEW(allocator, AssetFileStream);
        if (!stream->open(filepath, mode))
        {
            OMAF_DELETE(allocator, stream);
            stream = OMAF_NULL;
        }
        return stream;
    }
    else if (scheme == SCHEME_PREFIX_STORAGE)
    {
        StorageFileStream* stream = OMAF_NEW(allocator, StorageFileStream);
        stream->open(filepath, mode);

        return stream;
    }
    else if (scheme == SCHEME_PREFIX_FILE)
    {
        DiskFileStream* stream = OMAF_NEW(allocator, DiskFileStream);
        filepath = stripFirstFolder(filepath);
        stream->open(filepath, mode);

        return stream;
    }
    else
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();

        return OMAF_NULL;
    }
}

void_t FileSystem::close(FileStream* fileStream)
{
    OMAF_ASSERT_NOT_NULL(fileStream);

    if (fileStream != OMAF_NULL)
    {
        fileStream->close();

        MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();
        OMAF_DELETE(allocator, fileStream);
    }
}
OMAF_NS_END
