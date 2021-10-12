
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
#ifndef MP4VRFILESTREAMLINUX_HPP_
#define MP4VRFILESTREAMLINUX_HPP_

#include <cstdio>
#include <vector>
#include "api/reader/mp4vrfilestreaminterface.h"
#include "customallocator.hpp"

namespace MP4VR
{
    class LinuxStream : public StreamInterface
    {
    public:
        LinuxStream();
        LinuxStream(const char* filename);

        LinuxStream(const LinuxStream& other) = delete;
        LinuxStream& operator=(const LinuxStream& other) = delete;
        LinuxStream& operator                            =(LinuxStream&& other);

        ~LinuxStream() override;

        /** Returns the number of bytes read. The value of 0 indicates end
        of file.
        @param [buffer] The buffer to write the data into
        @param [size]   The number of bytes to read from the stream
        @returns The number of bytes read, or 0 on EOF. */
        offset_t read(char* buffer, offset_t size) override;

        /** Seeks to the given offset. Should the offset be erronous we'll
        find it out by the next read that will signal EOF.
        @param [offset] Offset to seek into */
        bool absoluteSeek(offset_t offset) override;

        /** Retrieve the current offset of the file.
        @returns The current offset of the file. */
        offset_t tell() override;

        /** Retrieve the size of the current file.

        @returns The current size of the file. Return
        LinuxStream::StreamSize if the file size cannot be determined. */
        offset_t size() override;

    private:
        int m_handle;
        offset_t m_size;

        /** Buffer contents offset from the file beginning. May be
     * negative in case we need to reset buffer contents but preserve
     * position. 
     * m_bufFileOffset + m_bufRead indicates the file offset of the
     * next byte returned by read() */
        offset_t m_bufFileOffset;
        int m_bufRead;  //< Current read offset within the buffer. Is always <= m_bufEnd.
        int m_bufEnd;   //< Current buffer size
        Vector<char> m_buffer;

        void fillBuffer();
    };
}

#endif  // MP4VRFILESTREAMLINUX_HPP_
