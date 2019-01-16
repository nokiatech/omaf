
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
#include "Foundation/NVRStorageFileStream.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    StorageFileStream::StorageFileStream()
    : mFileHandle(StorageManager::InvalidFileHandle)
    {
    }
    
    StorageFileStream::~StorageFileStream()
    {
        OMAF_ASSERT(mFileHandle == StorageManager::InvalidFileHandle, "");
    }
    
    bool_t StorageFileStream::openImpl(const char_t* filename, FileSystem::AccessMode::Enum mode)
    {
        mFileHandle = StorageManager::Open(filename, mode);
        
        if (mFileHandle == StorageManager::InvalidFileHandle)
        {
            return false;
        }
        
        return true;
    }
    
    void_t StorageFileStream::closeImpl()
    {
        StorageManager::Close(mFileHandle);
        mFileHandle = StorageManager::InvalidFileHandle;
    }

    int64_t StorageFileStream::readImpl(void_t* destination, int64_t bytes)
    {
        return StorageManager::Read(mFileHandle, destination, bytes);
    }

    int64_t StorageFileStream::writeImpl(const void_t* source, int64_t bytes)
    {
        OMAF_UNUSED_VARIABLE(source);
        OMAF_UNUSED_VARIABLE(bytes);
        OMAF_ASSERT_NOT_IMPLEMENTED();
        
        return 0;
    }

    bool_t StorageFileStream::seekImpl(int64_t offset)
    {
        return StorageManager::Seek(mFileHandle, offset);
    }

    int64_t StorageFileStream::tellImpl() const
    {
        return StorageManager::Tell(mFileHandle);
    }
    
    bool_t StorageFileStream::isOpenImpl() const
    {
        return StorageManager::IsOpen(mFileHandle);
    }
    
    int64_t StorageFileStream::getSizeImpl() const
    {
        return StorageManager::GetSize(mFileHandle);
    }
    
    FileSystem::StreamType::Enum StorageFileStream::getTypeImpl() const
    {
        return FileSystem::StreamType::STORAGE;
    }
OMAF_NS_END
