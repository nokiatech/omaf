
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
#include "Media/NVRMediaPacket.h"
#include "Foundation/NVRArray.h"
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemorySystem.h"

#if OMAF_PLATFORM_WINDOWS
// not actually correct but.
typedef unsigned int u_int32_t;
u_int32_t ntohl(u_int32_t v)
{
    unsigned char* t = (unsigned char*) &v;
#define swap(a, b)           \
    {                        \
        unsigned char p = a; \
        a = b;               \
        b = p;               \
    };
    swap(t[0], t[3]);
    swap(t[1], t[2]);
    swap(t[2], t[1]);
    swap(t[3], t[0]);
    return v;
}
#endif

OMAF_NS_BEGIN
OMAF_LOG_ZONE(MP4VRMediaPacket)

MP4VRMediaPacket::MP4VRMediaPacket(const size_t bufferSize)
    : mDecodingTimeUs(-1)
    , mDurationUs(-1)
    , mIsReferenceFrame(false)
    , mBufferSize(bufferSize)
    , mDataSize(0)
    , mPresentationTimeUs(0)
    , mSampleId(0)
    , mReConfigRequired(false)
{
    mBuffer = OMAF_NEW_ARRAY(*MemorySystem::DefaultHeapAllocator(), uint8_t, mBufferSize);
}

MP4VRMediaPacket::~MP4VRMediaPacket()
{
    OMAF_DELETE_ARRAY(*MemorySystem::DefaultHeapAllocator(), mBuffer);
}
void MP4VRMediaPacket::resizeBuffer(size_t bufferSize)
{
    OMAF_DELETE_ARRAY(*MemorySystem::DefaultHeapAllocator(), mBuffer);
    mBufferSize = bufferSize;
    mBuffer = OMAF_NEW_ARRAY(*MemorySystem::DefaultHeapAllocator(), uint8_t, mBufferSize);
    mDataSize = 0;
}
void MP4VRMediaPacket::setIsReferenceFrame(bool_t aIsReferenceFrame)
{
    mIsReferenceFrame = aIsReferenceFrame;
}
bool_t MP4VRMediaPacket::isReferenceFrame() const
{
    return mIsReferenceFrame;
}

size_t MP4VRMediaPacket::copyTo(const size_t srcOffset, uint8_t* destBuffer, const size_t destBufferSize)
{
    OMAF_ASSERT(srcOffset < mBufferSize, "");
    OMAF_ASSERT(OMAF_NULL != destBuffer, "");
    OMAF_ASSERT(destBufferSize > 0, "");

    size_t remainingSize = mDataSize - srcOffset;
    size_t copySize = remainingSize < destBufferSize ? remainingSize : destBufferSize;
    memcpy(destBuffer, (mBuffer + srcOffset), copySize);
    return copySize;
}

OMAF_NS_END
