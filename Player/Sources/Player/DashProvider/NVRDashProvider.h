
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

#include "Provider/NVRProviderBase.h"

OMAF_NS_BEGIN

class DashDownloadManager;

class DashProvider
: public ProviderBase
{
public:
    
    DashProvider();
    virtual ~DashProvider();

public: // CoreProvider
    
    virtual const CoreProviderSourceTypes& getSourceTypes();

public: // VideoProvider

    virtual Error::Enum loadAuxiliaryStream(PathName& uri);
    virtual Error::Enum playAuxiliary();
    virtual Error::Enum stopAuxiliary();
    virtual Error::Enum pauseAuxiliary();

    virtual Error::Enum seekToMsAuxiliary(uint64_t seekTargetMS);


    virtual bool_t isSeekable();
    virtual bool_t isSeekableByFrame();

    virtual uint64_t durationMs() const;

protected: // ProviderBase
    
    virtual uint64_t selectSources(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical, CoreProviderSources& base, CoreProviderSources& enhancement);
    virtual bool_t setInitialViewport(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical);
    virtual MP4AudioStream* getAudioStream();
private:
    
    void_t parserThreadCallback();
    void_t handlePendingUserRequest();

    void_t waitForDownloadBuffering();

    void_t clearDownloadedContent(DashDownloadManager* dlManager);
    void_t doSeek();

    void_t doSeekAuxiliary();
    
private:

    DashDownloadManager* mDownloadManager;

    DashDownloadManager* mAuxiliaryDownloadManager;

    CoreProviderSourceTypes mSourceTypes;
};

OMAF_NS_END
