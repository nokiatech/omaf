
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
#pragma once

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN

class MP4VRMediaPacket;

class IMediaPacketQueue
{
public:
    virtual bool_t hasFilledPackets() const = 0;

    virtual MP4VRMediaPacket* peekFilledPacket() = 0;

    virtual MP4VRMediaPacket* popFilledPacket() = 0;

    virtual void_t pushFilledPacket(MP4VRMediaPacket* packet) = 0;

    virtual bool_t hasEmptyPackets(size_t count) const = 0;

    virtual MP4VRMediaPacket* popEmptyPacket() = 0;

    virtual void_t pushEmptyPacket(MP4VRMediaPacket* packet) = 0;

    virtual void_t reset() = 0;
};

OMAF_NS_END
