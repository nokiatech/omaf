
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
#include "Foundation/NVRFixedQueue.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRMutex.h"
#include "Media/NVRIMediaPacketQueue.h"

#define MEDIAPACKETQUEUE_CAPACITY 300

OMAF_NS_BEGIN

    class MemoryAllocator;

    class MP4VRMediaPacket;

    class MediaPacketQueue
    : public IMediaPacketQueue
    {
    public:
        MediaPacketQueue(MemoryAllocator& allocator, uint32_t maxSampleSize);
        ~MediaPacketQueue();

        // TODO: this is probably not safe enough, because this is accessed from renderer and provider threads

    public:
        size_t filledPacketsSize() const;
        size_t emptyPacketsSize() const;
        size_t allocatedPacketsSize() const;
        size_t getCapacity() const;

    public: // IMediaPacketQueue

        virtual bool_t hasFilledPackets() const;
        virtual MP4VRMediaPacket* peekFilledPacket();
        virtual MP4VRMediaPacket* popFilledPacket();
        virtual void_t pushFilledPacket(MP4VRMediaPacket* packet);

        virtual bool_t hasEmptyPackets(size_t count) const;
        virtual MP4VRMediaPacket* popEmptyPacket();
        virtual void_t pushEmptyPacket(MP4VRMediaPacket* packet);

        virtual void_t reset();

    private:

        MemoryAllocator& mAllocator;

        uint32_t mMaxSampleSize;

        // NOTE: Ownership of MP4VRMediaPacket is kept even if packet is popped out temporarily.

        FixedQueue<MP4VRMediaPacket*, MEDIAPACKETQUEUE_CAPACITY> mEmptyPackets;
        FixedQueue<MP4VRMediaPacket*, MEDIAPACKETQUEUE_CAPACITY> mFilledPackets;
        FixedArray<MP4VRMediaPacket*, MEDIAPACKETQUEUE_CAPACITY> mAllocatedPackets;

        mutable Mutex mMutex;

    };

OMAF_NS_END
