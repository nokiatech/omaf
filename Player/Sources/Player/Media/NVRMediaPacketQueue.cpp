
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
#include "Media/NVRMediaPacketQueue.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRNew.h"

#include "Media/NVRMediaPacket.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(MediaPacketQueue)

MediaPacketQueue::MediaPacketQueue(MemoryAllocator& allocator, uint32_t maxSampleSize)
    : mAllocator(allocator)
    , mMaxSampleSize(maxSampleSize)
    , mEmptyPackets()
    , mFilledPackets()
    , mMutex()
{
}

MediaPacketQueue::~MediaPacketQueue()
{
    for (size_t i = 0; i < mAllocatedPackets.getSize(); ++i)
    {
        OMAF_DELETE(mAllocator, mAllocatedPackets[i]);
    }
    mAllocatedPackets.clear();
    mFilledPackets.clear();
    mEmptyPackets.clear();
}

bool_t MediaPacketQueue::hasFilledPackets() const
{
    Mutex::ScopeLock lock(mMutex);
    return !mFilledPackets.isEmpty();
}

MP4VRMediaPacket* MediaPacketQueue::peekFilledPacket()
{
    Mutex::ScopeLock lock(mMutex);
    if (mFilledPackets.isEmpty())
    {
        return OMAF_NULL;
    }

    MP4VRMediaPacket* packet = mFilledPackets.front();

    return packet;
}

MP4VRMediaPacket* MediaPacketQueue::popFilledPacket()
{
    Mutex::ScopeLock lock(mMutex);
    if (mFilledPackets.isEmpty())
    {
        return OMAF_NULL;
    }

    MP4VRMediaPacket* packet = mFilledPackets.front();

    if (packet != OMAF_NULL)
    {
        mFilledPackets.pop();
    }

    return packet;
}

void_t MediaPacketQueue::pushFilledPacket(MP4VRMediaPacket* packet)
{
    OMAF_ASSERT(packet != OMAF_NULL, "");
    Mutex::ScopeLock lock(mMutex);

    mFilledPackets.push(packet);
}

bool_t MediaPacketQueue::hasEmptyPackets(size_t count) const
{
    Mutex::ScopeLock lock(mMutex);

    // There is room for new filled packets so empty packet(s) must be available.
    return mFilledPackets.getCapacity() - mFilledPackets.getSize() > count;
}

MP4VRMediaPacket* MediaPacketQueue::popEmptyPacket()
{
    Mutex::ScopeLock lock(mMutex);
    MP4VRMediaPacket* packet = OMAF_NULL;

    if (mEmptyPackets.isEmpty())
    {
        OMAF_ASSERT((mAllocatedPackets.getSize() <= MEDIAPACKETQUEUE_CAPACITY), "Too many packets created!");

        packet = OMAF_NEW(mAllocator, MP4VRMediaPacket)(mMaxSampleSize);
        OMAF_ASSERT_NOT_NULL(packet->buffer());
        mAllocatedPackets.add(packet);
    }
    else
    {
        packet = mEmptyPackets.front();
        mEmptyPackets.pop();
    }

    return packet;
}

void_t MediaPacketQueue::pushEmptyPacket(MP4VRMediaPacket* packet)
{
    OMAF_ASSERT(packet != OMAF_NULL, "");
    Mutex::ScopeLock lock(mMutex);
    packet->setDataSize(0);
    mEmptyPackets.push(packet);
}

void_t MediaPacketQueue::reset()
{
    Mutex::ScopeLock lock(mMutex);
    for (size_t i = 0; i < mAllocatedPackets.getSize(); ++i)
    {
        OMAF_DELETE(mAllocator, mAllocatedPackets[i]);
    }
    mAllocatedPackets.clear();
    mFilledPackets.clear();
    mEmptyPackets.clear();
}

size_t MediaPacketQueue::filledPacketsSize() const
{
    return mFilledPackets.getSize();
}

size_t MediaPacketQueue::emptyPacketsSize() const
{
    return mEmptyPackets.getSize();
}

size_t MediaPacketQueue::allocatedPacketsSize() const
{
    return mAllocatedPackets.getSize();
}

size_t MediaPacketQueue::getCapacity() const
{
    return mAllocatedPackets.getCapacity();
}

OMAF_NS_END
