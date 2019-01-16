
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

OMAF_NS_BEGIN
    extern const char_t* SCHEME_PREFIX_STORAGE;
    extern const char_t* SCHEME_PREFIX_ASSET;
	extern const char_t* SCHEME_PREFIX_FILE;
	extern const char_t* SCHEME_PREFIX_HTTP;
    extern const char_t* SCHEME_PREFIX_HTTPS;

    PathName getFileName(const char_t* uri);

    PathName getFileExtension(const char_t* uri);
    PathName stripFileExtension(const char_t* uri);

    bool_t hasUriScheme(const char_t* uri);
    PathName getUriScheme(const char_t* uri);
    PathName stripUriScheme(const char_t* uri);
	PathName stripFirstFolder(const char_t* uri);

    PathName getAbsolutePath(const char_t* uri);

    PathName getCachePath(const char_t* uri);

    struct FileBuffer
    {
        uint8_t* data;
        size_t size;
    };
    
    FileBuffer createBufferFromFile(const char_t* filepath);
    void_t destroyBuffer(FileBuffer& buffer);
OMAF_NS_END
