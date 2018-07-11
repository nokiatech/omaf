
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
#include "Foundation/NVRAssetFileStream.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    AssetFileStream::AssetFileStream()
    : mFileHandle(AssetManager::InvalidFileHandle)
    {
    }
    
    AssetFileStream::~AssetFileStream()
    {
        OMAF_ASSERT(mFileHandle == AssetManager::InvalidFileHandle, "");
    }
    
    bool_t AssetFileStream::openImpl(const char_t* filename, FileSystem::AccessMode::Enum mode)
    {
        mFileHandle = AssetManager::Open(filename, mode);
        
        if (mFileHandle == AssetManager::InvalidFileHandle)
        {
            return false;
        }
        
        return true;
    }
    
    void_t AssetFileStream::closeImpl()
    {
        AssetManager::Close(mFileHandle);
        mFileHandle = AssetManager::InvalidFileHandle;
    }

    int64_t AssetFileStream::readImpl(void_t* destination, int64_t bytes)
    {
        return AssetManager::Read(mFileHandle, destination, bytes);
    }

    int64_t AssetFileStream::writeImpl(const void_t* source, int64_t bytes)
    {
        OMAF_UNUSED_VARIABLE(source);
        OMAF_UNUSED_VARIABLE(bytes);
        OMAF_ASSERT_NOT_IMPLEMENTED();

        return 0;
    }

    bool_t AssetFileStream::seekImpl(int64_t offset)
    {
        return AssetManager::Seek(mFileHandle, offset);
    }

    int64_t AssetFileStream::tellImpl() const
    {
        return AssetManager::Tell(mFileHandle);
    }
    
    bool_t AssetFileStream::isOpenImpl() const
    {
        return AssetManager::IsOpen(mFileHandle);
    }
    
    int64_t AssetFileStream::getSizeImpl() const
    {
        return AssetManager::GetSize(mFileHandle);
    }
    
    FileSystem::StreamType::Enum AssetFileStream::getTypeImpl() const
    {
        return FileSystem::StreamType::ASSET;
    }
OMAF_NS_END
