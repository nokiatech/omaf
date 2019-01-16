
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

#include "NVRNamespace.h"
#include "Foundation/NVRFileStream.h"
#include "Foundation/NVRPathName.h"

#include <mp4vrfilestreaminterface.h>

#include "NVRErrorCodes.h"

OMAF_NS_BEGIN

    class MP4VRMediaPacket;

    class MP4FileStreamer : public MP4VR::StreamInterface
    {
    public:
        MP4FileStreamer();
        virtual ~MP4FileStreamer();

        Error::Enum open(const PathName& mediaUri);
        void_t close();

        offset_t read(char* buffer, offset_t size);

        bool absoluteSeek(offset_t offset);

        offset_t tell();

        offset_t size();

    private:
        FileStream* mFileStream;
    };
OMAF_NS_END
