
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
#include "DashProvider/NVRDashDownloadManager.h"
#include "Metadata/NVRMetadataParser.h"
#include "Media/NVRMediaPacket.h"
#include "DashProvider/NVRDashLog.h"
#include "DashProvider/NVRDashVideoDownloader.h"
#include "DashProvider/NVRDashVideoDownloaderExtractor.h"
#include "DashProvider/NVRDashVideoDownloaderExtractorDepId.h"
#include "DashProvider/NVRDashVideoDownloaderExtractorMultiRes.h"
#include "DashProvider/NVRDashVideoDownloaderEnh.h"

#include "Foundation/NVRClock.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRDeviceInfo.h"


#include <libdash.h>

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashDownloadManager)

static const uint32_t MPD_UPDATE_INTERVAL_MS = 2000;
static const uint32_t MPD_DOWNLOAD_TIMEOUT_MS = 5000;

DashDownloadManager::DashDownloadManager()
    : mMPD(OMAF_NULL)
    , mDashManager(OMAF_NULL)
    , mVideoDownloader(OMAF_NULL)
    , mVideoDownloaderCreated(false)
    , mViewportSetEvent(false, false)
    , mAudioAdaptationSet(OMAF_NULL)
    , mAudioMetadataAdaptationSet(OMAF_NULL)
    , mState(DashDownloadManagerState::IDLE)
    , mStreamType(DashStreamType::INVALID)
    , mMPDUpdateTimeMS(0)
    , mMPDUpdatePeriodMS(MPD_UPDATE_INTERVAL_MS)
    , mCurrentAudioStreams()
    , mMetadataLoaded(false)
    , mMPDUpdater(OMAF_NULL)
{
    mDashManager = CreateDashManager();
}

DashDownloadManager::~DashDownloadManager()
{
    release();
    mDashManager->Delete();
}

void_t DashDownloadManager::enableABR()
{
    if (mVideoDownloader)
    {
        mVideoDownloader->enableABR();
    }
}


void_t DashDownloadManager::disableABR()
{
    if (mVideoDownloader)
    {
        mVideoDownloader->disableABR();
    }
}

void_t DashDownloadManager::setBandwidthOverhead(uint32_t aBandwithOverhead)
{
    mVideoDownloader->setBandwidthOverhead(aBandwithOverhead);
}

uint32_t DashDownloadManager::getCurrentBitrate()
{
    uint32_t bitrate = 0;
    if (mVideoDownloader != OMAF_NULL)
    {
        bitrate += mVideoDownloader->getCurrentBitrate();
    }
    if (mAudioAdaptationSet != OMAF_NULL)
    {
        bitrate += mAudioAdaptationSet->getCurrentBandwidth();
    }

    return bitrate;
}

MediaInformation DashDownloadManager::getMediaInformation()
{
    return mMediaInformation;
}

void_t DashDownloadManager::getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrEnhLayerCodecs)
{
    if (mVideoDownloader)
    {
        mVideoDownloader->getNrRequiredVideoCodecs(aNrBaseLayerCodecs, aNrEnhLayerCodecs);
    }
}

Error::Enum DashDownloadManager::init(const PathName& mediaURL, const BasicSourceInfo& sourceOverride)
{
    if (mState != DashDownloadManagerState::IDLE)
    {
        return Error::INVALID_STATE;
    }

    release();

    OMAF_LOG_D("init: URL = %s", mediaURL.getData());
    mPublishTime = "";
    mMPDPath = mediaURL;
    Error::Enum result = Error::OK;
    mMPDUpdater = OMAF_NEW_HEAP(MPDDownloadThread)(mDashManager);
    if (!mMPDUpdater->setUri(mediaURL))
    {
        OMAF_DELETE_HEAP(mMPDUpdater);
        mMPDUpdater = OMAF_NULL;
        mState = DashDownloadManagerState::CONNECTION_ERROR;
        return Error::FILE_NOT_FOUND;
    }
    mState = DashDownloadManagerState::INITIALIZING;
    mSourceOverride = sourceOverride;
    mMPDUpdater->trigger();
    return Error::OK;
}

Error::Enum DashDownloadManager::waitForInitializeCompletion()
{
    if (mState == DashDownloadManagerState::INITIALIZING)
    {
        mMPDUpdater->waitCompletion();
        return completeInitialization();
    }
    else
    {
        return Error::INVALID_STATE;
    }
}

Error::Enum DashDownloadManager::checkInitialize()
{
    if (mState == DashDownloadManagerState::INITIALIZING)
    {
        if (mMPDUpdater->isReady())
        {
            return completeInitialization();
        }
        else
        {
            return Error::NOT_READY;
        }
    }
    else
    {
        return Error::INVALID_STATE;
    }
}

Error::Enum DashDownloadManager::completeInitialization()
{
    Error::Enum result;
    OMAF_ASSERT(mMPDUpdater->isReady(), "Calling complete init when not ready");

    mMPD = mMPDUpdater->getResult(result);
    if (result != Error::OK)
    {
        if (result == Error::NOT_SUPPORTED)
        {
            mState = DashDownloadManagerState::STREAM_ERROR;
        }
        else
        {
            mState = DashDownloadManagerState::CONNECTION_ERROR;
        }
        return result;
    }

    BasicSourceInfo sourceInfo;

    if (mSourceOverride.sourceDirection != SourceDirection::INVALID && mSourceOverride.sourceType != SourceType::INVALID)
    {
        sourceInfo = mSourceOverride;
    }
    else if (!MetadataParser::parseUri(mMPDPath.getData(), sourceInfo))
    {
        OMAF_LOG_D("Videostream format is unknown, using default values");

        sourceInfo.sourceDirection = SourceDirection::MONO;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }

    OMAF_LOG_D("MPD type: %s", mMPD->GetType().c_str());
    OMAF_LOG_D("Availability start: %s", mMPD->GetAvailabilityStarttime().c_str());

    mStreamType = DashUtils::getDashStreamType(mMPD);

    mMediaInformation.videoType = VideoType::VARIABLE_RESOLUTION;
    mMediaInformation.fileType = MediaFileType::DASH;
    if (mMPD->GetRawAttributes().find("xmlns:omaf") != mMPD->GetRawAttributes().end())
    {
        mMediaInformation.fileType = MediaFileType::DASH_OMAF;
    }
    if (mStreamType == DashStreamType::DYNAMIC)
    {
        mMediaInformation.streamType = StreamType::LIVE_STREAM;
    }
    else
    {
        mMediaInformation.streamType = StreamType::VIDEO_ON_DEMAND;
    }

    DashComponents dashComponents;

    // TODO: Add support for more than 1 period
    dash::mpd::IPeriod* period = mMPD->GetPeriods().at(0);
    dashComponents.mpd = mMPD;
    dashComponents.period = period;
    dashComponents.basicSourceInfo = sourceInfo;
    dashComponents.adaptationSet = OMAF_NULL;
    dashComponents.representation = OMAF_NULL;
    dashComponents.segmentTemplate = OMAF_NULL;

    // As a very first step, we scan through the adaptation sets to see what kind of adaptation sets we do have, 
    // e.g. if some of them are only supporting sets (tiles), as that info is needed when initializing the adaptation sets and representations under each adaptation set
    // Partial sets do not necessarily carry that info themselves, but we can detect it only from extractors, if present - and absense of extractor tells something too
    VASType::Enum vasType = VASType::NONE;
    for (uint32_t index = 0; index < period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = period->GetAdaptationSets().at(index);
        dashComponents.adaptationSet = adaptationSet;
        if (DashAdaptationSetExtractor::isExtractor(dashComponents))
        {
            AdaptationSetBundleIds preselection = DashAdaptationSetExtractor::hasPreselection(dashComponents, DashAdaptationSetExtractor::parseId(dashComponents));
            if (!preselection.partialAdSetIds.isEmpty())
            {
                if (DashAdaptationSetExtractor::hasMultiResolution(dashComponents))
                {
                    // there are more than 1 adaptation set with main adaptation set (extractor) type, containing lists of supporting partial adaptation sets
                    // collect the lists of supporting sets here, since the supporting set needs to know about its role when initialized, and this may be the only way to know it
                    vasType = VASType::EXTRACTOR_PRESELECTIONS_RESOLUTION;
                    break;
                }
                else
                {
                    // this adaptation set is a main adaptation set (extractor) and contains a list of supporting partial adaptation sets
                    // collect the lists of supporting sets here, since the supporting set needs to know about its role when initialized, and this may be the only way to know it
                    vasType = VASType::EXTRACTOR_PRESELECTIONS_QUALITY;
                    break;
                }
            }
            else if (!DashAdaptationSetExtractor::hasDependencies(dashComponents).isEmpty())
            {
                vasType = VASType::EXTRACTOR_DEPENDENCIES_QUALITY;
                break;
            }
        }
    }
    if (vasType == VASType::NONE)
    {
        // No extractor found. Loop again, now searching for sub-pictures
        // TODO should also identify 2-track stereo, and that should atm go to subpictures
        for (uint32_t index = 0; index < period->GetAdaptationSets().size(); index++)
        {
            dash::mpd::IAdaptationSet* adaptationSet = period->GetAdaptationSets().at(index);
            dashComponents.adaptationSet = adaptationSet;
            if (DashAdaptationSetSubPicture::isSubPicture(dashComponents))
            {
                vasType = VASType::SUBPICTURES;
            }
        }
    }
    // Create video downloader
    switch (vasType)
    {
    case VASType::EXTRACTOR_PRESELECTIONS_RESOLUTION:
    {
        mVideoDownloader = OMAF_NEW_HEAP(DashVideoDownloaderExtractorMultiRes);
        break;
    }
    case VASType::EXTRACTOR_PRESELECTIONS_QUALITY:
    {
        mVideoDownloader = OMAF_NEW_HEAP(DashVideoDownloaderExtractor);
        break;
    }
    case VASType::EXTRACTOR_DEPENDENCIES_QUALITY:
    {
        mVideoDownloader = OMAF_NEW_HEAP(DashVideoDownloaderExtractorDepId);
        break;
    }
    case VASType::SUBPICTURES:
    {
        mVideoDownloader = OMAF_NEW_HEAP(DashVideoDownloaderEnh);
        break;
    }
    default:
    {
        mVideoDownloader = OMAF_NEW_HEAP(DashVideoDownloader);
        break;
    }
    }
    mVideoDownloader->completeInitialization(dashComponents, sourceInfo);
    mVideoDownloaderCreated = true;

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    for (uint32_t index = 0; index < period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = period->GetAdaptationSets().at(index);
        dashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        // "normal" adaptation set
        dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSet)(*this);
        result = dashAdaptationSet->initialize(dashComponents, initializationSegmentId);
        if (result == Error::OK)
        {
            mAdaptationSets.add(dashAdaptationSet);
        }
        else
        {
            OMAF_LOG_W("Initializing an adaptation set returned %d, ignoring it", result);
            OMAF_DELETE_HEAP(dashAdaptationSet);
        }
    }

    if (mAdaptationSets.isEmpty())
    {
        // No adaptations sets found, return failure
        mState = DashDownloadManagerState::STREAM_ERROR;
        return Error::NOT_SUPPORTED;
    }

    mAudioAdaptationSet = OMAF_NULL;

    for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
    {
        DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
        if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::IS_MUXED))
        {
            mAudioAdaptationSet = dashAdaptationSet;
            break;
        }
        else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_VIDEO))
        {
            // handled in video downloader
                
        }
        else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_AUDIO))
        {
            MediaContent selectedAudio;
            selectedAudio.clear();
            uint32_t selectedAudioBandwidth = 0;
            if (mAudioAdaptationSet != OMAF_NULL)
            {
                selectedAudio = mAudioAdaptationSet->getAdaptationSetContent();
                selectedAudioBandwidth = mAudioAdaptationSet->getCurrentBandwidth();
            }

            if (selectedAudio.matches(MediaContent::Type::AUDIO_LOUDSPEAKER))
            {
                if (dashAdaptationSet->getCurrentBandwidth() > selectedAudioBandwidth)
                {
                    mAudioAdaptationSet = dashAdaptationSet;
                }
            }
            else 
            {
                mAudioAdaptationSet = dashAdaptationSet;
            }
        }

        for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
        {
            DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
            if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_META))
            {// find which adaptation set/representation this metadata adaptation set is associated to
                RepresentationId associatedTo;
                if (dashAdaptationSet->isAssociatedToRepresentation(associatedTo))
                {
                    for (size_t j = 0; j < mAdaptationSets.getSize(); j++)
                    {
                        if (mAdaptationSets.at(j)->getCurrentRepresentationId() == associatedTo)
                        {
                            //TODO invo or some other timed metadata?
                        }
                    }
                }
            }
        }
    }

    if (mVideoDownloader != OMAF_NULL)
    {
        mVideoDownloader->getMediaInformation(mMediaInformation);
    }
    else
    {
        OMAF_LOG_E("Stream needs to have video");
        mState = DashDownloadManagerState::STREAM_ERROR;
        return Error::NOT_SUPPORTED;
    }

    if (mAudioAdaptationSet != OMAF_NULL)
    {
        const MediaContent& mediaContent = mAudioAdaptationSet->getAdaptationSetContent();
        if (mediaContent.matches(MediaContent::Type::AUDIO_LOUDSPEAKER))
        {
            mMediaInformation.audioFormat = AudioFormat::LOUDSPEAKER;
        }
    }

    mMPDUpdatePeriodMS = DashUtils::getMPDUpdateInMilliseconds(dashComponents);
    if (mMPDUpdatePeriodMS == 0)
    {
        // the update period is optional parameter; use default value
        mMPDUpdatePeriodMS = MPD_UPDATE_INTERVAL_MS;
    }

    // wait for viewport info, needed before start
    mViewportSetEvent.wait(1000);

    mPublishTime = DashUtils::getRawAttribute(mMPD, "publishTime");
    mMPDUpdateTimeMS = Time::getClockTimeMs();
    mState = DashDownloadManagerState::INITIALIZED;

    if (isVideoAndAudioMuxed())
    {
        OMAF_LOG_W("The stream has video and audio muxed together in the same adaptation set. This is against the DASH spec and may not work correctly");
    }

    return Error::OK;
}

void_t DashDownloadManager::release()
{
    OMAF_DELETE_HEAP(mMPDUpdater);
    mMPDUpdater = OMAF_NULL;
    stopDownload();

    // first delete extractors, as they may have linked segments from supporting sets
    for (size_t i = 0; i < mAdaptationSets.getSize(); )
    {
        if (mAdaptationSets.at(i)->getType() == AdaptationSetType::EXTRACTOR)
        {
            DashAdaptationSet* set = mAdaptationSets.at(i);
            mAdaptationSets.removeAt(i);
            OMAF_DELETE_HEAP(set);
        }
        else
        {
            i++;
        }
    }
    // then delete the rest 
    while (!mAdaptationSets.isEmpty())
    {
        DashAdaptationSet* set = mAdaptationSets.at(0);
        mAdaptationSets.removeAt(0);
        OMAF_DELETE_HEAP(set);
    }

    mAdaptationSets.clear();
    if (mVideoDownloader != OMAF_NULL)
    {
        mVideoDownloader->release();
        OMAF_DELETE_HEAP(mVideoDownloader);
    }

    mAudioAdaptationSet = OMAF_NULL;

    delete mMPD;
    mMPD = OMAF_NULL;
    mState = DashDownloadManagerState::IDLE;
}

Error::Enum DashDownloadManager::startDownload()
{
    if (mState != DashDownloadManagerState::INITIALIZED
        && mState != DashDownloadManagerState::STOPPED)
    {
        return Error::INVALID_STATE;
    }

    if (mVideoDownloader != OMAF_NULL)
    {
        if (mStreamType == DashStreamType::DYNAMIC)
        {
            Error::Enum result = updateMPD(true);
            if (result != Error::OK)
            {
                return result;
            }
        }

        time_t startTime = time(0);
        OMAF_LOG_D("Start time: %d", (uint32_t)startTime);

        mVideoDownloader->startDownload(startTime, mMPDUpdater->mMpdDownloadTimeMs);

        if (!isVideoAndAudioMuxed() && mAudioAdaptationSet != OMAF_NULL)
        {
            mAudioAdaptationSet->startDownload(startTime);
            if (mAudioMetadataAdaptationSet != OMAF_NULL)
            {
                mAudioMetadataAdaptationSet->startDownload(startTime);
            }
        }


        mState = DashDownloadManagerState::DOWNLOADING;
        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

void_t DashDownloadManager::updateStreams(uint64_t currentPlayTimeUs)
{
    mVideoDownloader->updateStreams(currentPlayTimeUs);
    //TODO this comes twice now for video
    processSegmentDownload();

    Error::Enum result = Error::OK;
    int32_t now = Time::getClockTimeMs();
    if (mState == DashDownloadManagerState::DOWNLOADING && mStreamType == DashStreamType::DYNAMIC)
    {
        bool_t updateRequired = mVideoDownloader->isMpdUpdateRequired();
        if (!isVideoAndAudioMuxed() && !updateRequired && mAudioAdaptationSet != OMAF_NULL)
        {
            updateRequired = mAudioAdaptationSet->mpdUpdateRequired();
        }
        // check if either the main video or audio streams are running out of info what segments to download (timeline mode) or required update period is exceeded
        if (updateRequired || (now - mMPDUpdateTimeMS > (int32_t)mMPDUpdatePeriodMS))
        {
            result = updateMPD(false);
            if (result != Error::OK)
            {
                // updateMPD failed, the DownloadManager state is updated inside the updateMPD so just return
                return;
            }
        }
    }

    if (mVideoDownloader->isEndOfStream())
    {
        OMAF_LOG_V("Download manager goes to EOS");
        mState = DashDownloadManagerState::END_OF_STREAM;
    }
    if (mVideoDownloader->isError() ||
        (mAudioAdaptationSet != OMAF_NULL && mAudioAdaptationSet->isError()) ||
        (mAudioMetadataAdaptationSet != OMAF_NULL && mAudioMetadataAdaptationSet->isError()) )
    {
        mState = DashDownloadManagerState::STREAM_ERROR;
    }
    
    //mVideoDownloader->setBandwidthOverhead(mBandwidthOverhead);
}

Error::Enum DashDownloadManager::stopDownload()
{
    if (mState != DashDownloadManagerState::DOWNLOADING
        && mState != DashDownloadManagerState::STREAM_ERROR
        && mState != DashDownloadManagerState::CONNECTION_ERROR)
    {
        return Error::INVALID_STATE;
    }

    if (mVideoDownloader != OMAF_NULL)
    {
        mVideoDownloader->stopDownload();

        if (!isVideoAndAudioMuxed() && mAudioAdaptationSet != OMAF_NULL)
        {
            mAudioAdaptationSet->stopDownload();
            if (mAudioMetadataAdaptationSet != OMAF_NULL)
            {
                mAudioMetadataAdaptationSet->stopDownload();
            }
        }
        mState = DashDownloadManagerState::STOPPED;
        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

void_t DashDownloadManager::clearDownloadedContent()
{
    if (mVideoDownloader != OMAF_NULL)
    {
        mVideoDownloader->clearDownloadedContent();

        if (!isVideoAndAudioMuxed() && mAudioAdaptationSet != OMAF_NULL)
        {
            //order matters
            if (mAudioMetadataAdaptationSet != OMAF_NULL)
            {
                mAudioMetadataAdaptationSet->clearDownloadedContent();
            }
            mAudioAdaptationSet->clearDownloadedContent();
        }
    }
}

DashDownloadManagerState::Enum DashDownloadManager::getState()
{
    return mState;
}

bool_t DashDownloadManager::isVideoAndAudioMuxed()
{
    if (!mAudioAdaptationSet)
    {
        return false;
    }
    return mAudioAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::IS_MUXED);
}

Error::Enum DashDownloadManager::seekToMs(uint64_t& aSeekMs)
{
    uint64_t seekToTarget = aSeekMs;
    uint64_t seekToResultMs = 0;
    Error::Enum result = Error::OK;

    if (mVideoDownloader != OMAF_NULL)
    {
        result = mVideoDownloader->seekToMs(seekToTarget, seekToResultMs);
    }

    // treat mAudioAdaptationSet as separate from mVideoBaseAdaptationSet in case of audio only stream.
    if (!isVideoAndAudioMuxed() && mAudioAdaptationSet && result == Error::OK)
    {
        result = mAudioAdaptationSet->seekToMs(seekToTarget, seekToResultMs);
        aSeekMs = seekToResultMs;
        if (mAudioMetadataAdaptationSet != OMAF_NULL)
        {
            mAudioMetadataAdaptationSet->seekToMs(seekToTarget, seekToResultMs);
        }
    }
    return result;
}

bool_t DashDownloadManager::isSeekable()
{
    return mVideoDownloader->isSeekable();
}

uint64_t DashDownloadManager::durationMs() const
{
    return mVideoDownloader->durationMs();
}

const MP4VideoStreams& DashDownloadManager::getVideoStreams()
{
    return mVideoDownloader->getVideoStreams();
}


const MP4AudioStreams& DashDownloadManager::getAudioStreams()
{
    if (mAudioAdaptationSet != OMAF_NULL && mCurrentAudioStreams.isEmpty())
    {
        mCurrentAudioStreams.add(mAudioAdaptationSet->getCurrentAudioStreams());
    }
    return mCurrentAudioStreams;
}

Error::Enum DashDownloadManager::readVideoFrames(int64_t currentTimeUs)
{
    return mVideoDownloader->readVideoFrames(currentTimeUs);
}

Error::Enum DashDownloadManager::readAudioFrames()
{
    OMAF_ASSERT(mAudioAdaptationSet != OMAF_NULL, "No current audio adaptation set");
    return mAudioAdaptationSet->readNextAudioFrame();
}

MP4VRMediaPacket* DashDownloadManager::getNextVideoFrame(MP4MediaStream &stream, int64_t currentTimeUs)
{
    return mVideoDownloader->getNextVideoFrame(stream, currentTimeUs);
}

MP4VRMediaPacket* DashDownloadManager::getNextAudioFrame(MP4MediaStream &stream)
{
    // With Dash we don't need to do anything special here
    return stream.peekNextFilledPacket();
}

const CoreProviderSourceTypes& DashDownloadManager::getVideoSourceTypes()
{
    OMAF_ASSERT(mVideoDownloader != OMAF_NULL, "No current video adaptation set");

    return mVideoDownloader->getVideoSourceTypes();
}

bool_t DashDownloadManager::isDynamicStream()
{
    return mStreamType == DashStreamType::DYNAMIC;
}

bool_t DashDownloadManager::isBuffering()
{
    const CoreProviderSourceTypes& sourceTypes = getVideoSourceTypes();
    if (sourceTypes.isEmpty())
    {
        return true;
    }
    bool_t videoBuffering = mVideoDownloader->isBuffering();

    bool_t audioBuffering = false;
    if (mAudioAdaptationSet != OMAF_NULL)
    {
        audioBuffering = mAudioAdaptationSet->isBuffering();
    }
    if (videoBuffering)
    {
        OMAF_LOG_D("isBuffering = TRUE");
    }
    return videoBuffering || audioBuffering;
}

bool_t DashDownloadManager::isEOS() const
{
    if (mState == DashDownloadManagerState::END_OF_STREAM)
    {
        return true;
    }
    return mVideoDownloader->isEndOfStream();
}

bool_t DashDownloadManager::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    return mVideoDownloader->isReadyToSignalEoS(aStream);
}

bool_t DashDownloadManager::setInitialViewport(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height)
{
    if (mVideoDownloaderCreated)
    {
        streamid_t id = 0;
        OMAF_LOG_V("setInitialViewingOrientation, longitude %f", longitude);
        bool_t aViewportSet = false;
        mVideoDownloader->selectSources(longitude, latitude, roll, width, height, id, aViewportSet);
        if (aViewportSet)
        {
            mViewportSetEvent.signal();
            return true;
        }
    }
    return false;
}

const MP4VideoStreams& DashDownloadManager::selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId)
{
    bool_t set;
    return mVideoDownloader->selectSources(longitude, latitude, roll, width, height, aBaseLayerStreamId, set);
}

DashDownloadManager::MPDDownloadThread::MPDDownloadThread(dash::IDASHManager *aDashManager)
    : mAllocator(*MemorySystem::DefaultHeapAllocator()),
    mDashManager(aDashManager),
    mData(mAllocator,64*1024)
{
    mConnection = Http::createHttpConnection(mAllocator);
}
DashDownloadManager::MPDDownloadThread::~MPDDownloadThread()
{
    OMAF_DELETE(mAllocator, mConnection);//dtor aborts and waits if needed.
}

bool_t DashDownloadManager::MPDDownloadThread::setUri(const PathName& aUri)
{
    mMPDPath = aUri;
    return mConnection->setUri(aUri);
}

void DashDownloadManager::MPDDownloadThread::trigger()
{
    HttpConnectionState::Enum state = mConnection->getState().connectionState;
    OMAF_ASSERT(!isBusy(), "MPDUpdater busy.");
    mStart = Clock::getMilliseconds();
    OMAF_LOG_V("[%p] MPD download start %lld", this, mStart);
    mConnection->setTimeout(MPD_DOWNLOAD_TIMEOUT_MS);
    mConnection->get(&mData);
}
void DashDownloadManager::MPDDownloadThread::waitCompletion()
{
    mConnection->waitForCompletion();
}
bool DashDownloadManager::MPDDownloadThread::isBusy()
{
    const HttpConnectionState::Enum& state = mConnection->getState().connectionState;
    if (state == HttpConnectionState::IDLE || state == HttpConnectionState::INVALID) return false;
    return true;
}
bool DashDownloadManager::MPDDownloadThread::isReady()
{
    const HttpConnectionState::Enum& state = mConnection->getState().connectionState;
    if (state == HttpConnectionState::IDLE) return true;
    if (state == HttpConnectionState::FAILED) return true;
    if (state == HttpConnectionState::ABORTED) return true;
    if (state == HttpConnectionState::COMPLETED) return true;
    return false;
}
dash::mpd::IMPD* DashDownloadManager::MPDDownloadThread::getResult(Error::Enum& aResult)
{
    const HttpRequestState& state = mConnection->getState();
    if ((state.connectionState == HttpConnectionState::COMPLETED) && ((state.httpStatus >= 200)&&(state.httpStatus<300)))
    {
        uint64_t ps = Clock::getMilliseconds();
        mMpdDownloadTimeMs = (uint32_t)(ps - mStart);
        OMAF_LOG_V("[%p] MPD downloading took %lld", this, mMpdDownloadTimeMs);
        dash::mpd::IMPD* newMPD = mDashManager->Open((const char*)mData.getDataPtr(), mData.getSize(), mMPDPath.getData());
        OMAF_LOG_V("[%p] MPD parsing took %lld", this, Clock::getMilliseconds() - ps);
        if (newMPD != OMAF_NULL)
        {
            const std::vector<std::string>& profiles = newMPD->GetProfiles();
            bool_t foundSupportedProfile = false;
            for (int i = 0; i < profiles.size(); i++)
            {
                if (profiles[i] == "urn:mpeg:dash:profile:isoff-live:2011"
                    || profiles[i] == "urn:mpeg:dash:profile:isoff-on-demand:2011"
                    || profiles[i] == "urn:mpeg:dash:profile:isoff-main:2011"
                    || profiles[i] == "urn:mpeg:dash:profile:full:2011")
                {
                    //Supported profile.
                    OMAF_LOG_V("Supported profile:%s", profiles[i].c_str());
                    foundSupportedProfile = true;
                }
                else
                {
                    OMAF_LOG_D("Skipping unsupported profile:%s", profiles[i].c_str());
                }
            }
            if (!foundSupportedProfile)
            {
                OMAF_LOG_W("None of the profiles listed in the MPD are supported. Unexpected things may happen.");
            }
            mConnection->setUri(mMPDPath);
            aResult = Error::OK;
            return newMPD;
        }
        else
        {
            OMAF_LOG_W("new MPD is NULL!");
        }
    }
    else if (state.connectionState == HttpConnectionState::COMPLETED)
    {
        OMAF_LOG_E("Can't find MPD file");
        aResult = Error::FILE_NOT_FOUND;
    }
    else
    {
        OMAF_LOG_E("HTTP request failed");
        aResult = Error::OPERATION_FAILED;
        // recreate the mConnection, since if it is in failed state, it stays in that forever. We may still want to retry
        OMAF_DELETE(mAllocator, mConnection);//dtor aborts and waits if needed.
        mConnection = Http::createHttpConnection(mAllocator);
    }

    mConnection->setUri(mMPDPath);
    return OMAF_NULL;
}

Error::Enum DashDownloadManager::updateMPD(bool_t synchronous)
{
    Error::Enum result = Error::OK;
    bool_t triggered = false;
    if (mMPDUpdater->isBusy() == false)
    {
        mMPDUpdater->trigger();
        triggered = true;
    }
    if (synchronous)
    {
        if (triggered)
        {
            // Fetching a new one, so just need to wait for one complete
            mMPDUpdater->waitCompletion();
        }
        else
        {
            // Wasn't triggered so there must be an old one mixing up
            mMPDUpdater->waitCompletion();
            Error::Enum result;
            dash::mpd::IMPD* newMPD = mMPDUpdater->getResult(result);
            // Who reads yesterday's MPDs...
            if (newMPD != OMAF_NULL)
            {
                delete newMPD;
            }
            mMPDUpdater->trigger();
            mMPDUpdater->waitCompletion();
        }
    }
    if (mMPDUpdater->isReady())
    {
        uint64_t start = Clock::getMilliseconds();
        bool retry = false;
        dash::mpd::IMPD* newMPD = mMPDUpdater->getResult(result);
        if (result == Error::FILE_NOT_FOUND || result == Error::OPERATION_FAILED || newMPD == OMAF_NULL)
        {
            OMAF_LOG_D("[%p] mpd download failed(FILE_NOT_FOUND?)", this);
            if (synchronous)
            {
                mState = DashDownloadManagerState::STREAM_ERROR;
                retry = false;
                // mpd download error is fatal in synchronous mode (when starting / resuming playback)
            }
            else
            {
                //TODO: add a retry counter here. it is not fatal if mpd update fails.
                result = Error::OK;
                retry = true;
            }
        }
        else if (result == Error::NOT_SUPPORTED)
        {
            // MPD has changed to something we don't support
            OMAF_LOG_D("[%p] mpd download failed(NOT_SUPPORTED?)", this);
            mState = DashDownloadManagerState::STREAM_ERROR;
            retry = false;
        }
        else //if (result == Error::OK)
        {
            const std::string& newPublishTime = DashUtils::getRawAttribute(newMPD, "publishTime");
            // Ignore the publish time check since some encoders & CDNs don't update the value
            //if (newPublishTime != mPublishTime)
            {
                OMAF_LOG_V("[%p] MPD updated %s != %s", this, mPublishTime.c_str(), newPublishTime.c_str());
                // TODO: Support more than a single period
                dash::mpd::IPeriod* period = newMPD->GetPeriods().at(0);

                if (period == OMAF_NULL)
                {
                    OMAF_LOG_D("[%p] NOT_SUPPORTED (null period)", this);
                    mState = DashDownloadManagerState::STREAM_ERROR;
                    return Error::NOT_SUPPORTED;
                }

                // For now we don't allow new adaptation sets at this point
                if (period->GetAdaptationSets().size() != mAdaptationSets.getSize())
                {
                    OMAF_LOG_E("[%p] MPD update error, adaptation set count changed", this);
                    mState = DashDownloadManagerState::STREAM_ERROR;
                    return Error::NOT_SUPPORTED;
                }

                for (uint32_t index = 0; index < period->GetAdaptationSets().size(); index++)
                {
                    DashComponents components;
                    components.mpd = newMPD;
                    components.period = period;
                    components.adaptationSet = period->GetAdaptationSets().at(index);
                    components.representation = OMAF_NULL;
                    components.basicSourceInfo.sourceDirection = SourceDirection::INVALID;
                    components.basicSourceInfo.sourceType = SourceType::INVALID;
                    mAdaptationSets.at(index)->updateMPD(components);
                }
                delete mMPD;
                mMPD = newMPD;
                mPublishTime = newPublishTime;
                mMPDUpdateTimeMS = Time::getClockTimeMs();
            }
            /*
            else
            {
                const std::string& newPublishTime = DashUtils::getRawAttribute(newMPD, "publishTime");
                OMAF_LOG_D("[%p] mpd not updated %s == %s", this, mPublishTime.c_str(), newPublishTime.c_str());
                delete newMPD;
                newMPD = OMAF_NULL;
                retry = true;
            }*/
            OMAF_LOG_V("[%p] MPD update took %lld", this, Clock::getMilliseconds() - start);
        }
        if (retry)
        {
            //adjust update time, so that we only wait for half of the normal update interval)
            mMPDUpdateTimeMS = Time::getClockTimeMs() - (mMPDUpdatePeriodMS / 2);
        }
    }
    return Error::OK;
}

void_t DashDownloadManager::processSegmentDownload()
{
    //OMAF_LOG_D("%lld processSegmentDownload in", Time::getClockTimeMs());
    if (mAudioAdaptationSet != OMAF_NULL)
    {
        if (mAudioMetadataAdaptationSet != OMAF_NULL)
        {
            mAudioMetadataAdaptationSet->processSegmentDownload();
        }

        if (mAudioAdaptationSet->processSegmentDownload())
        {
            // update audio streams
            mCurrentAudioStreams.clear();
            mCurrentAudioStreams.add(mAudioAdaptationSet->getCurrentAudioStreams());
        }
    }
    mVideoDownloader->processSegmentDownload();
}

void_t DashDownloadManager::onNewStreamsCreated()
{
}

void_t DashDownloadManager::onDownloadProblem(IssueType::Enum aIssueType)
{
    // Bitrate controller is in video downloader, report also possible audio / metadata issues there
    mVideoDownloader->onDownloadProblem(aIssueType);
}
OMAF_NS_END
