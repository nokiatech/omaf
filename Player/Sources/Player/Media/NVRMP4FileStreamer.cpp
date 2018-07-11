
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
#include "Media/NVRMP4FileStreamer.h"

OMAF_NS_BEGIN
    MP4FileStreamer::MP4FileStreamer()
            : mFileStream(OMAF_NULL)
    {

    }

    MP4FileStreamer::~MP4FileStreamer()
    {
        if (mFileStream != OMAF_NULL)
        {
            FileSystem::close(mFileStream);
        }

    }

    Error::Enum MP4FileStreamer::open(const PathName &mediaUri)
    {
        //PathName absolutePath = getAbsolutePath(mediaUri.getData());
        //OMAF_LOG_D("Opening %s", absolutePath.getData());

        mFileStream = FileSystem::open(mediaUri.getData(), FileSystem::AccessMode::READ);
        if (mFileStream == OMAF_NULL)
        {
            return Error::FILE_NOT_FOUND;
        }

        return Error::OK;
    }

    void_t MP4FileStreamer::close()
    {
        if (mFileStream != OMAF_NULL)
        {
            FileSystem::close(mFileStream);
            mFileStream = OMAF_NULL;
        }
    }

    MP4VR::StreamInterface::offset_t MP4FileStreamer::read(char *buffer, MP4VR::StreamInterface::offset_t size)
    {
        return mFileStream->read(buffer, size);
    }

    bool MP4FileStreamer::absoluteSeek(offset_t offset)
    {
        int64_t current = mFileStream->tell();
        int64_t delta = offset - current;
        return mFileStream->seek(delta);
    }

    MP4VR::StreamInterface::offset_t MP4FileStreamer::tell()
    {
        MP4VR::StreamInterface::offset_t tempoffset = mFileStream->tell();
        return tempoffset;
    }

    MP4VR::StreamInterface::offset_t MP4FileStreamer::size()
    {
        MP4VR::StreamInterface::offset_t tempsize = mFileStream->getSize();
        return tempsize;
    }
OMAF_NS_END
