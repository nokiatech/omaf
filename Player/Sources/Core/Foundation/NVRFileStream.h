
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
#include "Foundation/NVRFileSystem.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
class FileStream
{
public:
    FileStream();
    virtual ~FileStream();

    bool_t open(const char_t* filename, FileSystem::AccessMode::Enum mode);
    void_t close();

    int64_t read(void_t* destination, int64_t bytes);
    int64_t write(const void_t* source, int64_t bytes);

    bool_t seek(int64_t offset);

    // Returns negative value if fails
    int64_t tell() const;

    bool_t isOpen() const;

    int64_t getSize() const;

    FileSystem::StreamType::Enum getType() const;

private:
    virtual bool_t openImpl(const char_t* filename, FileSystem::AccessMode::Enum mode) = 0;
    virtual void_t closeImpl() = 0;

    virtual int64_t readImpl(void_t* destination, int64_t bytes) = 0;
    virtual int64_t writeImpl(const void_t* source, int64_t bytes) = 0;

    virtual bool_t seekImpl(int64_t offset) = 0;
    virtual int64_t tellImpl() const = 0;

    virtual bool_t isOpenImpl() const = 0;

    virtual int64_t getSizeImpl() const = 0;

    virtual FileSystem::StreamType::Enum getTypeImpl() const = 0;
};
OMAF_NS_END
