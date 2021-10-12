
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

#include "Provider/NVRProviderBase.h"

OMAF_NS_BEGIN

class MP4MediaStreamManager;

class MP4VRProvider : public ProviderBase
{
public:
    MP4VRProvider();
    virtual ~MP4VRProvider();

public:  // CoreProvider
    virtual const CoreProviderSourceTypes& getSourceTypes();

public:  // VideoProvider
    virtual bool_t isSeekable();

    virtual bool_t isSeekableByFrame();

    virtual uint64_t durationMs() const;

protected:  // ProviderBase
    virtual uint64_t selectSources(HeadTransform headTransform,
                                   float32_t fovHorizontal,
                                   float32_t fovVertical,
                                   CoreProviderSources& required,
                                   CoreProviderSources& optional);
    virtual MP4AudioStream* getAudioStream();

    virtual const CoreProviderSources& getAllSources() const;

private:
    virtual void_t parserThreadCallback();
    void_t handlePendingUserRequest();
    Error::Enum openFile(PathName& uri);
    void_t doSeek();

private:
    MP4MediaStreamManager* mMP4MediaStreamManager;
};

OMAF_NS_END
