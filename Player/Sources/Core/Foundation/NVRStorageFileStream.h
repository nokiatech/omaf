
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
#pragma once

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRFileStream.h"
#include "Foundation/NVRFileSystem.h"
#include "Foundation/NVRStorageManager.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
class StorageFileStream : public FileStream
{
public:
    StorageFileStream();
    ~StorageFileStream();

    virtual bool_t openImpl(const char_t* filename, FileSystem::AccessMode::Enum mode);
    virtual void_t closeImpl();

    virtual int64_t readImpl(void_t* destination, int64_t bytes);
    virtual int64_t writeImpl(const void_t* source, int64_t bytes);

    virtual bool_t seekImpl(int64_t offset);
    virtual int64_t tellImpl() const;

    virtual bool_t isOpenImpl() const;

    virtual int64_t getSizeImpl() const;

    virtual FileSystem::StreamType::Enum getTypeImpl() const;

private:
    StorageManager::FileHandle mFileHandle;
};
OMAF_NS_END
