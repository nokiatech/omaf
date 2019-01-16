
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
#include "NVRHeifFileStreamer.h"

OMAF_NS_BEGIN

    HeifFileStreamer::HeifFileStreamer()
      : mFileStream(OMAF_NULL)
    {
    }

    HeifFileStreamer::~HeifFileStreamer()
    {
        if (mFileStream != OMAF_NULL)
        {
            FileSystem::close(mFileStream);
        }

    }

    Error::Enum HeifFileStreamer::open(const PathName &mediaUri)
    {
        mFileStream = FileSystem::open(mediaUri.getData(), FileSystem::AccessMode::READ);
        if (mFileStream == OMAF_NULL)
        {
            return Error::FILE_NOT_FOUND;
        }

        return Error::OK;
    }

    void_t HeifFileStreamer::close()
    {
        if (mFileStream != OMAF_NULL)
        {
            FileSystem::close(mFileStream);
            mFileStream = OMAF_NULL;
        }
    }

    HEIF::StreamInterface::offset_t HeifFileStreamer::read(char *buffer, HEIF::StreamInterface::offset_t size)
    {
        return mFileStream->read(buffer, size);
    }

    bool HeifFileStreamer::absoluteSeek(offset_t offset)
    {
        int64_t current = mFileStream->tell();
        int64_t delta = offset - current;
        return mFileStream->seek(delta);
    }

    HEIF::StreamInterface::offset_t HeifFileStreamer::tell()
    {
        HEIF::StreamInterface::offset_t tempoffset = mFileStream->tell();
        return tempoffset;
    }

    HEIF::StreamInterface::offset_t HeifFileStreamer::size()
    {
        HEIF::StreamInterface::offset_t tempsize = mFileStream->getSize();
        return tempsize;
    }
OMAF_NS_END


