
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

OMAF_NS_BEGIN

class MP4VRMediaPacket;

/*
 *  An interface class for managing media packet queues
 */
class MP4VRMediaPacketQueue
{
public:
    virtual bool_t hasFilledPackets() const = 0;
    virtual MP4VRMediaPacket* getNextFilledPacket() = 0;
    virtual MP4VRMediaPacket* peekNextFilledPacket() = 0;
    virtual bool_t popFirstFilledPacket() = 0;
    virtual void_t storeFilledPacket(MP4VRMediaPacket* packet) = 0;
    virtual MP4VRMediaPacket* getNextEmptyPacket() = 0;
    virtual void_t returnEmptyPacket(MP4VRMediaPacket* packet) = 0;
    virtual void_t resetPackets() = 0;
};

OMAF_NS_END
