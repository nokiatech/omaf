
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
#include "mp4vrfilestreamgeneric.h"

#include "mp4vrfilestreamfile.h"

#ifdef VDD_MP4VR_USE_LINUX_FILESTREAM
#include "mp4vrfilestreamlinux.h"
#endif  // VDD_MP4VR_USE_LINUX_FILESTREAM

namespace VDD
{
    std::unique_ptr<MP4VR::StreamInterface> openFile(const char* filename)
    {
        std::unique_ptr<MP4VR::StreamInterface> stream = nullptr;
#ifdef VDD_MP4VR_USE_LINUX_FILESTREAM
        stream.reset(new LinuxStream(filename));
#else
        stream.reset(new FileStream(filename));
#endif  // VDD_MP4VR_USE_LINUX_FILESTREAM
        if (stream->absoluteSeek(0))
        {
            return stream;
        }
        else
        {
            stream.reset();
            return stream;
        }
    }
}
