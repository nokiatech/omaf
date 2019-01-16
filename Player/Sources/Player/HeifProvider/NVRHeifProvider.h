
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
#include "Platform/OMAFDataTypes.h"

#include "Provider/NVRProviderBase.h"
#include "NVRHeifMediaStreamManager.h"


OMAF_NS_BEGIN

class MP4MediaStreamManager;

class HeifProvider
: public ProviderBase
{
public:
    
    HeifProvider();
    virtual ~HeifProvider();

public: // CoreProvider
    
    virtual const CoreProviderSourceTypes& getSourceTypes();

public: // VideoProvider
    
    virtual bool_t isSeekable();

    virtual bool_t isSeekableByFrame();

    virtual uint64_t durationMs() const;

    virtual Error::Enum next();

protected: // ProviderBase
    
    virtual uint64_t selectSources(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical, CoreProviderSources& required, CoreProviderSources& optional);

    virtual MP4AudioStream* getAudioStream();

private:
    
    virtual void_t parserThreadCallback();
    void_t handlePendingUserRequest();
    Error::Enum openFile(PathName& uri);
    
private:
    HeifMediaStreamManager* mMediaStreamManager;
    bool_t mRequestImageChange;
};

OMAF_NS_END
