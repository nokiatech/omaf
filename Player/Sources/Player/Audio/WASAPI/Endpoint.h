
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
class AudioRendererAPI;
namespace WASAPIImpl
{
    class Context;
    class Endpoint
    {
    public:
        virtual ~Endpoint(){};
        virtual bool_t init(int32_t aSamplerate, int32_t aChannels) = 0;
        virtual int64_t latencyUs() = 0;
        virtual void stop() = 0;
        virtual void pause() = 0;
        virtual uint64_t position() = 0;
        virtual bool_t newsamples(AudioRendererAPI* audio, bool_t push) = 0;
    };
}  // namespace WASAPIImpl
OMAF_NS_END
