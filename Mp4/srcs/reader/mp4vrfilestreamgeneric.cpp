
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
#include "mp4vrfilestreamgeneric.hpp"
#include "customallocator.hpp"

#include "mp4vrfilestreamfile.hpp"

#ifdef MP4VR_USE_LINUX_FILESTREAM
#include "mp4vrfilestreamlinux.hpp"
#endif  // MP4VR_USE_LINUX_FILESTREAM

namespace MP4VR
{
    StreamInterface* openFile(const char* filename)
    {
#ifdef MP4VR_USE_LINUX_FILESTREAM
        return CUSTOM_NEW(LinuxStream, (filename));
#else
        return CUSTOM_NEW(FileStream, (filename));
#endif  // MP4VR_USE_LINUX_FILESTREAM
    }
}
