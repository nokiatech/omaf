
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
#include "Foundation/NVRFileUtilities.h"

#include "Foundation/NVRAssetManager.h"
#include "Foundation/NVRDiskManager.h"
#include "Foundation/NVRFileStream.h"
#include "Foundation/NVRFileSystem.h"
#include "Foundation/NVRStorageManager.h"

OMAF_NS_BEGIN
const char_t* SCHEME_PREFIX_STORAGE = "storage://";
const char_t* SCHEME_PREFIX_FILE = "file://";
const char_t* SCHEME_PREFIX_ASSET = "asset://";
const char_t* SCHEME_PREFIX_HTTP = "http://";
const char_t* SCHEME_PREFIX_HTTPS = "https://";

static const FixedString8 DotString = ".";
static const FixedString8 PathSeparatorString = "/";
static const FixedString8 SchemeString = "://";

PathName getFileName(const char_t* uri)
{
    PathName filepath = PathName(uri);

    if (getFileExtension(uri).isEmpty())
    {
        return PathName(&Eos);
    }

    size_t lastPos = filepath.findLast(PathSeparatorString);

    if (lastPos == Npos || lastPos == filepath.getLength())
    {
        return PathName(&Eos);
    }
    else
    {
        size_t start = lastPos + PathSeparatorString.getLength();
        size_t length = filepath.getLength() - start;

        return filepath.substring(start, length);
    }
}

PathName getFileExtension(const char_t* uri)
{
    PathName filepath = PathName(uri);

    size_t lastDotPos = filepath.findLast(DotString);

    if (lastDotPos == Npos)
    {
        return PathName(&Eos);
    }
    else if (lastDotPos == 0)
    {
        // Hidden file, not an extension
        return PathName(&Eos);
    }
    else
    {
        size_t start = lastDotPos + DotString.getLength();
        size_t length = filepath.getLength() - start;

        return filepath.substring(start, length);
    }
}

PathName stripFileExtension(const char_t* uri)
{
    PathName filepath = PathName(uri);

    size_t lastDotPos = filepath.findLast(DotString);

    if (lastDotPos == Npos)
    {
        // Missing extension. Return the filepath as-is
        return filepath;
    }
    else if (lastDotPos == 0)
    {
        // The only . in the filename is the first character -> hidden file, not an extension. Return the filename as-is
        return filepath;
    }
    else
    {
        return filepath.substring(0, lastDotPos);
    }
}

bool_t hasUriScheme(const char_t* uri)
{
    PathName filepath = PathName(uri);

    size_t pos = filepath.findFirst(SchemeString);

    if (pos == Npos)
    {
        return false;
    }

    return true;
}

PathName getUriScheme(const char_t* uri)
{
    PathName filepath = PathName(uri);

    size_t pos = filepath.findFirst(SchemeString);

    if (pos == Npos)
    {
        return PathName(&Eos);
    }
    else
    {
        size_t length = pos + SchemeString.getLength();

        return filepath.substring(0, length);
    }
}

PathName stripUriScheme(const char_t* uri)
{
    PathName filepath = PathName(uri);

    size_t pos = filepath.findFirst(SchemeString);

    if (pos == Npos)
    {
        return filepath;
    }
    else
    {
        size_t start = pos + SchemeString.getLength();
        size_t length = filepath.getLength() - start;

        return filepath.substring(start, length);
    }
}

PathName stripFirstFolder(const char_t* uri)
{
    PathName filepath = PathName(uri);

    size_t pos = filepath.findFirst("/");

    if (pos == Npos)
    {
        return filepath;
    }
    else
    {
        size_t start = pos + 1;
        size_t length = filepath.getLength() - start;

        return filepath.substring(start, length);
    }
}

PathName getAbsolutePath(const char_t* uri)
{
    PathName scheme = getUriScheme(uri);
    PathName filepath = stripUriScheme(uri);

    if (scheme == SCHEME_PREFIX_ASSET)
    {
        return AssetManager::GetFullPath(filepath.getData());
    }
    else if (scheme == SCHEME_PREFIX_STORAGE)
    {
        return StorageManager::GetFullPath(filepath.getData());
    }
    else if (scheme == SCHEME_PREFIX_FILE)
    {
        filepath = stripFirstFolder(filepath);
        return DiskManager::GetFullPath(filepath.getData());
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();

        PathName invalid("Error: Invalid path");
        return invalid;
    }
}

PathName getCachePath(const char_t* uri)
{
    PathName filepath = SCHEME_PREFIX_STORAGE;
    filepath.append(getFileName(uri));

    return filepath;
}

FileBuffer createBufferFromFile(const char_t* filepath)
{
    FileBuffer buffer;

    FileStream* fileStream = FileSystem::open(filepath, FileSystem::AccessMode::READ);

    if (fileStream == OMAF_NULL || !fileStream->isOpen())
    {
        OMAF_ASSERT(false, "Failed to load configuration file");
    }

    buffer.size = (size_t) fileStream->getSize();
    buffer.data = (uint8_t*) malloc(buffer.size + 1);

    fileStream->read(buffer.data, buffer.size);

    buffer.data[buffer.size] = '\0';

    FileSystem::close(fileStream);

    return buffer;
}

void_t destroyBuffer(FileBuffer& buffer)
{
    free(buffer.data);
    buffer.data = OMAF_NULL;

    buffer.size = 0;
}
OMAF_NS_END
