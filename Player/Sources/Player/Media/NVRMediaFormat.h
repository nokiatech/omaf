
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
#include "Foundation/NVRDataBuffer.h"
#include "Foundation/NVRHashMap.h"

#include <mp4vrfiledatatypes.h>

#include "Media/NVRMimeType.h"

OMAF_NS_BEGIN

    class MediaPacket;
    class MemoryAllocator;

    class MediaFormat
    {
    public:
        enum Type
        {
            INVALID = -1,
            IsVideo,
            IsAudio,
            IsMeta,
            Count
        };

        enum DecoderSpecInfoType
        {
            INVALID_ = -1,
            AVC_HEVC_SPS, 
            AVC_HEVC_PPS,
            HEVC_VPS,
            AudioSpecificConfig,     //< As defined in 1.6.2.1 AudioSpecificConfig of ISO/IEC 14496-3:200X(E)

            COUNT
        };
        typedef HashMap<DecoderSpecInfoType, DataBuffer<uint8_t>*> DecoderConfigMap;

    public:
        MediaFormat(Type aType, const MP4VR::FeatureBitMask& vrFeatures, const char_t* codecFourCC, const MimeType& MimeType);
        virtual ~MediaFormat();

        virtual int64_t durationUs() const;
        virtual float32_t frameRate() const;
        virtual int32_t width() const;
        virtual int32_t height() const;
        virtual const MimeType& getMimeType() const;

        virtual uint8_t* getDecoderConfigInfo(size_t& outSize);
        virtual const DecoderConfigMap& getDecoderConfigInfoMap();

        virtual void_t setDuration(int64_t durationUs);
        virtual void_t setFrameRate(float32_t frameRate);
        virtual void_t setWidth(int32_t width);
        virtual void_t setHeight(int32_t height);

        virtual void_t setDecoderConfigInfo(MP4VR::DynArray<MP4VR::DecoderSpecificInfo>& decSpecInfo, uint32_t id);
        virtual uint32_t getDecoderConfigInfoId();

        virtual MP4VR::FeatureBitMask getTrackVRFeatures() const;

        virtual const char_t* getFourCC() const;

    private:
        MemoryAllocator& mAllocator;

        Type mType;
        FixedString8 mCodecFourCC;
        const MP4VR::FeatureBitMask mTrackVRFeatures;
        // Decoding config info parts (e.g. SPS, PPS) are kept separate
        DecoderConfigMap mDecoderConfigInfoMap;
        // For typical cases, the config info is output as a single uint8 buffer. For memory management, it is owned by the format object
        DataBuffer<uint8_t> mDecoderConfigInfoBuffer;
        int64_t mDurationUs;
        int32_t mWidth;
        int32_t mHeight;
        float32_t mFrameRate;

        const MimeType& mMimeType;

        uint32_t mConfigIndexInMp4;

    };
OMAF_NS_END
