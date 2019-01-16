
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
#include "Foundation/NVRDiskFileStream.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    DiskFileStream::DiskFileStream()
    : mFileHandle(DiskManager::InvalidFileHandle)
    {
    }
    
    DiskFileStream::~DiskFileStream()
    {
        OMAF_ASSERT(mFileHandle == DiskManager::InvalidFileHandle, "");
    }
    
    bool_t DiskFileStream::openImpl(const char_t* filename, FileSystem::AccessMode::Enum mode)
    {
        mFileHandle = DiskManager::Open(filename, mode);
        
        if (mFileHandle == DiskManager::InvalidFileHandle)
        {
            return false;
        }
        
        return true;
    }
    
    void_t DiskFileStream::closeImpl()
    {
        DiskManager::Close(mFileHandle);
        mFileHandle = DiskManager::InvalidFileHandle;
    }

    int64_t DiskFileStream::readImpl(void_t* destination, int64_t bytes)
    {
        return DiskManager::Read(mFileHandle, destination, bytes);
    }

    int64_t DiskFileStream::writeImpl(const void_t* source, int64_t bytes)
    {
        OMAF_UNUSED_VARIABLE(source);
        OMAF_UNUSED_VARIABLE(bytes);
        OMAF_ASSERT_NOT_IMPLEMENTED();
        
        return 0;
    }

    bool_t DiskFileStream::seekImpl(int64_t offset)
    {
        return DiskManager::Seek(mFileHandle, offset);
    }

    int64_t DiskFileStream::tellImpl() const
    {
        return DiskManager::Tell(mFileHandle);
    }
    
    bool_t DiskFileStream::isOpenImpl() const
    {
        return DiskManager::IsOpen(mFileHandle);
    }
    
    int64_t DiskFileStream::getSizeImpl() const
    {
        return DiskManager::GetSize(mFileHandle);
    }
    
    FileSystem::StreamType::Enum DiskFileStream::getTypeImpl() const
    {
        return FileSystem::StreamType::FILE;
    }
OMAF_NS_END
