
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
#include "Foundation/NVRLogger.h"
#include "Media/NVRMediaFormat.h"
#include "Foundation/NVRStringUtilities.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(MediaFormat)

    MediaFormat::MediaFormat(Type aType, const MP4VR::FeatureBitMask& vrFeatures, const char_t* codecFourCC, const MimeType& mimeType)
        : mAllocator(*MemorySystem::DefaultHeapAllocator())
        , mType(aType)
        , mCodecFourCC(codecFourCC) 
        , mTrackVRFeatures(vrFeatures)
        , mDecoderConfigInfoMap(mAllocator)
        , mDecoderConfigInfoBuffer(DataBuffer<uint8_t>(*MemorySystem::DefaultHeapAllocator(), 0))
        , mDurationUs(0)
        , mWidth(0)
        , mHeight(0)
        , mFrameRate(0)
        , mMimeType(mimeType)
        , mConfigIndexInMp4(0)
    {
    }

    MediaFormat::~MediaFormat()
    {
        for (DecoderConfigMap::Iterator it = mDecoderConfigInfoMap.begin(); it != mDecoderConfigInfoMap.end(); ++it)
        {
            DataBuffer<uint8_t>* info = *it;
            info->clear();
            OMAF_DELETE(mAllocator, info);
        }
        mDecoderConfigInfoMap.clear();
    }

    int64_t MediaFormat::durationUs() const
    {
        return mDurationUs;
    }

    float32_t MediaFormat::frameRate() const
    {
        return mFrameRate;
    }

    int32_t MediaFormat::width() const
    {
        return mWidth;
    }

    int32_t MediaFormat::height() const
    {
        return mHeight;
    }

    const MimeType& MediaFormat::getMimeType() const
    {
        if (mMimeType.compare(UNKNOWN_MIME_TYPE) != ComparisonResult::EQUAL)
        {
            // the MIME-type was overwritten
            return mMimeType;
        }
        if (mType == IsAudio)
        {
            // Mime type
            if (mCodecFourCC.findFirst("mp4a") != Npos && mDecoderConfigInfoMap.contains(AudioSpecificConfig))
            {
                return AUDIO_AAC_MIME_TYPE;
            }
            else if (mCodecFourCC.findFirst("mp4a") != Npos) //mp3 has the same fourCC but if there is no decSpecInfo, it seems to be mp3
            {
                return AUDIO_MP3_MIME_TYPE;
            }
            else
            {
                return UNKNOWN_AUDIO_MIME_TYPE;
            }
        }
        else if (mType == IsVideo)
        {
            // Mime type
            if (mCodecFourCC.findFirst("avc1") != Npos || mCodecFourCC.findFirst("avc3") != Npos)
            {
                return VIDEO_H264_MIME_TYPE;
            }
            else if (mCodecFourCC.findFirst("hev1") != Npos || mCodecFourCC.findFirst("hvc1") != Npos || mCodecFourCC.findFirst("hvc2") != Npos)
            {
                return VIDEO_HEVC_MIME_TYPE;
            }
            else
            {
                return UNKNOWN_VIDEO_MIME_TYPE;
            }
        }

        // Any other MIME-types?
        return UNKNOWN_MIME_TYPE;
    }

    // get decoder config info as a single buffer. Ownership is not transferred
    uint8_t* MediaFormat::getDecoderConfigInfo(size_t& outSize)
    {
        size_t size = 0;
        for (DecoderConfigMap::ConstIterator it = mDecoderConfigInfoMap.begin(); it != mDecoderConfigInfoMap.end(); ++it)
        {
            size += (*it)->getSize();
        }
        mDecoderConfigInfoBuffer.clear();
        mDecoderConfigInfoBuffer.reAllocate(size);
        outSize = 0;
        DecoderConfigMap::ConstIterator it = mDecoderConfigInfoMap.find(DecoderSpecInfoType::AudioSpecificConfig);
        if (it != DecoderConfigMap::InvalidIterator)
        {
            memcpy(mDecoderConfigInfoBuffer.getDataPtr() + outSize, (*it)->getDataPtr(), (*it)->getSize());
            outSize += (*it)->getSize();
        }
        else
        {
            it = mDecoderConfigInfoMap.find(DecoderSpecInfoType::HEVC_VPS);
            if (it != DecoderConfigMap::InvalidIterator)
            {
                memcpy(mDecoderConfigInfoBuffer.getDataPtr() + outSize, (*it)->getDataPtr(), (*it)->getSize());
                outSize += (*it)->getSize();
            }
            it = mDecoderConfigInfoMap.find(DecoderSpecInfoType::AVC_HEVC_SPS);
            if (it != DecoderConfigMap::InvalidIterator)
            {
                memcpy(mDecoderConfigInfoBuffer.getDataPtr() + outSize, (*it)->getDataPtr(), (*it)->getSize());
                outSize += (*it)->getSize();
            }
            it = mDecoderConfigInfoMap.find(DecoderSpecInfoType::AVC_HEVC_PPS);
            if (it != DecoderConfigMap::InvalidIterator)
            {
                memcpy(mDecoderConfigInfoBuffer.getDataPtr() + outSize, (*it)->getDataPtr(), (*it)->getSize());
                outSize += (*it)->getSize();
            }

        }
        mDecoderConfigInfoBuffer.setSize(outSize);
        return mDecoderConfigInfoBuffer.getDataPtr();
    }

    const MediaFormat::DecoderConfigMap& MediaFormat::getDecoderConfigInfoMap()
    {
        return mDecoderConfigInfoMap;
    }

    void_t MediaFormat::setDuration(int64_t durationUs)
    {
        mDurationUs = durationUs;
    }

    void_t MediaFormat::setFrameRate(float32_t frameRate)
    {
        mFrameRate = frameRate;
    }

    void_t MediaFormat::setWidth(int32_t width)
    {
        mWidth = width;
    }

    void_t MediaFormat::setHeight(int32_t height)
    {
        mHeight = height;
    }

    void_t MediaFormat::setDecoderConfigInfo(MP4VR::DynArray<MP4VR::DecoderSpecificInfo>& decSpecInfo, uint32_t configId)
    {
        for (DecoderConfigMap::Iterator it = mDecoderConfigInfoMap.begin(); it != mDecoderConfigInfoMap.end(); ++it)
        {
            DataBuffer<uint8_t>* info = *it;
            info->clear();
            OMAF_DELETE(mAllocator, info);
        }
        mDecoderConfigInfoMap.clear();

        for (MP4VR::DecoderSpecificInfo* info = decSpecInfo.begin(); info != decSpecInfo.end(); info++)
        {
            DataBuffer<uint8_t>* dInfo = OMAF_NEW(mAllocator, DataBuffer<uint8_t>)(mAllocator, info->decSpecInfoData.size);
            dInfo->setData(info->decSpecInfoData.begin(), info->decSpecInfoData.size);
            DecoderSpecInfoType decSpecInfoType;
            switch (info->decSpecInfoType)
            {
            case MP4VR::AVC_SPS:
            case MP4VR::HEVC_SPS:
                decSpecInfoType = AVC_HEVC_SPS;
                break;
            case MP4VR::AVC_PPS:
            case MP4VR::HEVC_PPS:
                decSpecInfoType = AVC_HEVC_PPS;
                break;
            case MP4VR::HEVC_VPS:
                decSpecInfoType = HEVC_VPS;
                break;
            case MP4VR::AudioSpecificConfig:
                decSpecInfoType = AudioSpecificConfig;
                break;
            default:
                OMAF_ASSERT(false, "Unsupported dec spec info");
            }
            mDecoderConfigInfoMap.insert(decSpecInfoType, dInfo);
        }
        mConfigIndexInMp4 = configId;
    }

    uint32_t MediaFormat::getDecoderConfigInfoId()
    {
        return mConfigIndexInMp4;
    }

    MP4VR::FeatureBitMask MediaFormat::getTrackVRFeatures() const
    {
        return mTrackVRFeatures;
    }

    const char_t* MediaFormat::getFourCC() const
    {
        return mCodecFourCC.getData();
    }
OMAF_NS_END
