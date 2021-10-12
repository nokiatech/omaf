
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
#include "mp4vrfilestreamfile.h"

namespace VDD
{
    FileStream::FileStream()
        : m_file(nullptr)
        , m_curOffset(0)
        , m_size(-1)
        , m_buffer()
    {
        // nothing
    }

#if defined(_WIN32) || defined(_WIN64)
#define MP4VR_FSEEK _fseeki64
#define MP4VR_FTELL _ftelli64
#else
#define MP4VR_FSEEK fseek
#define MP4VR_FTELL ftell
#endif

    FileStream::FileStream(const char* filename)
        : m_file(nullptr)
        , m_curOffset(0)
        , m_size(-1)
        , m_buffer()
    {
#if defined(_WIN32) || defined(_WIN64)
        fopen_s(&m_file, filename, "rb");
#else
        m_file = fopen(filename, "rb");
#endif

        if (m_file)
        {
            if (MP4VR_FSEEK(m_file, 0, SEEK_END) == 0)
            {
                m_size = MP4VR_FTELL(m_file);
                if (m_size >= 0)
                {
                    MP4VR_FSEEK(m_file, 0, SEEK_SET);
                    setvbuf(m_file, m_buffer, _IOFBF, sizeof(m_buffer));
                }
            }
        }
        if (m_size < 0 && m_file)
        {
            fclose(m_file);
            m_file = nullptr;
        }
    }

    FileStream& FileStream::operator=(FileStream&& other)
    {
        if (m_file)
        {
            fclose(m_file);
        }
        m_file       = other.m_file;
        other.m_file = nullptr;
        m_curOffset  = other.m_curOffset;
        if (m_file)
        {
            setvbuf(m_file, m_buffer, _IOFBF, sizeof(m_buffer));
        }
        m_size = other.m_size;

        return *this;
    }

    FileStream::~FileStream()
    {
        if (m_file)
        {
            fclose(m_file);
        }
    }

    FileStream::offset_t FileStream::read(char* buffer, offset_t size_)
    {
        if (m_file)
        {
            offset_t n = offset_t(fread(buffer, 1, size_t(size_), m_file));
            if (n > 0)
            {
                m_curOffset += n;
            }
            return n;
        }
        else
        {
            return 0;
        }
    }

    bool FileStream::absoluteSeek(offset_t offset)
    {
        if (m_file)
        {
            m_curOffset = offset;
            return MP4VR_FSEEK(m_file, offset, SEEK_SET) == 0;
        }
        else
        {
            return false;
        }
    }

    FileStream::offset_t FileStream::tell()
    {
        return m_file ? m_curOffset : 0;
    }

    FileStream::offset_t FileStream::size()
    {
        return m_size;
    }

    bool FileStream::isOpen() const
    {
        return !!m_file;
    }
}
