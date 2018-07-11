
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
#pragma once

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRHashMap.h"
#include "Math/OMAFVector3.h"

#include "NVRMP4MediaStream.h"

#include "Media/NVRMP4MediaStream.h"

class MediaPacket;

OMAF_NS_BEGIN

    class MediaFormat;

    namespace SpatialAudioType
    {
        enum Enum
        {
            INVALID = -1,
            LEGACY_2D,

            COUNT
        };

    }

    class MP4AudioStream : public MP4MediaStream
    {
    public:
        MP4AudioStream(MediaFormat* format);
        virtual ~MP4AudioStream();

        void_t setSpatialAudioType(SpatialAudioType::Enum audioType);
        SpatialAudioType::Enum getSpatialAudioType();
        bool_t isSpatialAudioType();

        void_t addPosition(uint8_t sourceId, Vector3 position);
        void_t addGain(uint8_t sourceId, float32_t gain);

    private:
        SpatialAudioType::Enum mSpatialAudioType;

        HashMap<uint8_t, Vector3> mChannels;
        HashMap<uint8_t, float32_t> mGains;

    };

    typedef FixedArray<MP4AudioStream*, 4> MP4AudioStreams;

OMAF_NS_END
