
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
#include "DashProvider/NVRDashAdaptationSetAudio.h"
#include "Foundation/NVRLogger.h"


OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashAdaptationSetAudio)


DashAdaptationSetAudio::DashAdaptationSetAudio(DashAdaptationSetObserver& observer)
    : DashAdaptationSet(observer)
    , mUserActivateable(UserActivation::NOT_KNOWN)
{
}

DashAdaptationSetAudio::~DashAdaptationSetAudio()
{
}

AdaptationSetType::Enum DashAdaptationSetAudio::getType() const
{
    return AdaptationSetType::AUDIO;
}

bool_t DashAdaptationSetAudio::isAudio(DashComponents aDashComponents)
{
    if (MPDAttribute::AUDIO_MIME_TYPE.compare(aDashComponents.adaptationSet->GetMimeType().c_str()) ==
        ComparisonResult::EQUAL)
    {
        return true;
    }
    return false;
}

UserActivation::Enum DashAdaptationSetAudio::isUserActivateable()
{
    return mUserActivateable;
}
void_t DashAdaptationSetAudio::setUserActivateable(bool_t aIs)
{
    if (aIs)
    {
        OMAF_LOG_D("setUserActivateable %d", getId());
        mUserActivateable = UserActivation::USER_ACTIVATEABLE;
    }
    else
    {
        OMAF_LOG_D("setUserActivateable NOT %d", getId());
        mUserActivateable = UserActivation::ALWAYS_ON;
        if (!getCurrentAudioStreams().isEmpty())
        {
            // the downloadmanager should activate it immediately
            mObserver.onActivateMe(this);
        }
    }
}

void_t DashAdaptationSetAudio::onNewStreamsCreated(MediaContent& aContent)
{
    if (mUserActivateable == UserActivation::USER_ACTIVATEABLE)
    {
        MP4AudioStreams streams = getCurrentAudioStreams();
        OMAF_ASSERT(streams.getSize() == 1, "No audio streams, but streams created for audio?!?");
        OMAF_LOG_D("onNewStreamsCreated for %d, setUserActivateable %d", getId(), streams.at(0)->getStreamId());
        streams.front()->setUserActivateable();
    }
    else if (mUserActivateable == UserActivation::NOT_KNOWN)
    {
        OMAF_LOG_D("Audio stream created before video");
        // Activation decided when we know the state of the associated video.
        // If not associated to video, should have the state as ALWAYS_ACTIVE from the beginning.
    }
    else
    {
        OMAF_LOG_D("onNewStreamsCreated for %d, onActivateMe %d", getId(),
                   getCurrentAudioStreams().at(0)->getStreamId());
        // the downloadmanager should activate it immediately
        mObserver.onActivateMe(this);
    }
    DashAdaptationSet::onNewStreamsCreated(aContent);
}

OMAF_NS_END
