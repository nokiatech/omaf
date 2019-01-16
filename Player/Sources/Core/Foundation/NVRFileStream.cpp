
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
#include "Foundation/NVRFileStream.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRFileUtilities.h"
#include "Foundation/NVRAssetManager.h"
#include "Foundation/NVRStorageManager.h"

OMAF_NS_BEGIN
    FileStream::FileStream()
    {
        
    }
    
    FileStream::~FileStream()
    {
        
    }

    bool_t FileStream::open(const char_t* filename, FileSystem::AccessMode::Enum mode)
    {
        return openImpl(filename, mode);
    }

    void_t FileStream::close()
    {
        closeImpl();
    }

    int64_t FileStream::read(void_t* destination, int64_t bytes)
    {
        return readImpl(destination, bytes);
    }

    int64_t FileStream::write(const void_t* source, int64_t bytes)
    {
        return writeImpl(source, bytes);
    }

    bool_t FileStream::seek(int64_t offset)
    {
        return seekImpl(offset);
    }
    
    int64_t FileStream::tell() const
    {
        return tellImpl();
    }
    
    bool_t FileStream::isOpen() const
    {
        return isOpenImpl();
    }
    
    int64_t FileStream::getSize() const
    {
        return getSizeImpl();
    }
    
    FileSystem::StreamType::Enum FileStream::getType() const
    {
        return getTypeImpl();
    }
OMAF_NS_END
