
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
#include "DashProvider/NVRDashDownloadManager.h"
#include "DashProvider/NVRDashAdaptationSetAudio.h"
#include "DashProvider/NVRDashAdaptationSetOverlay.h"
#include "DashProvider/NVRDashLog.h"
#include "DashProvider/NVRDashPeriodViewpoint.h"
#include "DashProvider/NVRDashSpeedFactorMonitor.h"
#include "DashProvider/NVRDashVideoDownloader.h"
#include "DashProvider/NVRDashVideoDownloaderExtractor.h"
#include "DashProvider/NVRDashVideoDownloaderExtractorDepId.h"
#include "DashProvider/NVRDashVideoDownloaderExtractorMultiRes.h"
#include "Media/NVRMediaPacket.h"
#include "Metadata/NVRMetadataParser.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"

#include "Foundation/NVRClock.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRTime.h"

//#define UNKNOWN_AS_IDENTITY 1

#include <libdash.h>

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashDownloadManager)

static const uint32_t MPD_UPDATE_INTERVAL_MS = 2000;
static const uint32_t MPD_DOWNLOAD_TIMEOUT_MS = 5000;

DashDownloadManager::DashDownloadManager()
    : mMPD(OMAF_NULL)
    , mDashManager(OMAF_NULL)
    , mCurrentViewpoint(OMAF_NULL)
    , mInitializingViewpoint(OMAF_NULL)
    , mDownloadingViewpoint(OMAF_NULL)
    , mCommonToViewpoints(OMAF_NULL)
    , mVideoDownloaderCreated(false)
    , mViewportSetEvent(false, false)
    , mViewportConsumed(false)
    , mState(DashDownloadManagerState::IDLE)
    , mStreamType(DashStreamType::INVALID)
    , mMPDUpdateTimeMS(0)
    , mMPDUpdatePeriodMS(MPD_UPDATE_INTERVAL_MS)
    , mMetadataLoaded(false)
    , mNextSourceIdBase(0)
    , mMPDUpdater(OMAF_NULL)
    , mViewpointInitThread(*this)
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
    if (mCurrentViewpoint && mCurrentViewpoint->videoDownloader)
    {
        mCurrentViewpoint->videoDownloader->enableABR();
    }
}


void_t DashDownloadManager::disableABR()
{
    if (mCurrentViewpoint && mCurrentViewpoint->videoDownloader)
    {
        mCurrentViewpoint->videoDownloader->disableABR();
    }
}

void_t DashDownloadManager::setBandwidthOverhead(uint32_t aBandwithOverhead)
{
    mCurrentViewpoint->videoDownloader->setBandwidthOverhead(aBandwithOverhead);
}

uint32_t DashDownloadManager::getCurrentBitrate()
{
    uint32_t bitrate = 0;
    if (mCurrentViewpoint)
    {
        if (mCurrentViewpoint->videoDownloader != OMAF_NULL)
        {
            bitrate += mCurrentViewpoint->videoDownloader->getCurrentBitrate();
        }
        for (DashAdaptationSet* audio : mCurrentViewpoint->activeAudioAdaptationSets)
        {
            bitrate += audio->getCurrentBandwidth();
        }
    }
    return bitrate;
}

MediaInformation DashDownloadManager::getMediaInformation()
{
    return mMediaInformation;
}

void_t DashDownloadManager::getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrAdditionalCodecs)
{
    if (mCurrentViewpoint && mCurrentViewpoint->videoDownloader)
    {
        mCurrentViewpoint->videoDownloader->getNrRequiredVideoCodecs(aNrBaseLayerCodecs, aNrAdditionalCodecs);
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

    if (mSourceOverride.sourceDirection != SourceDirection::INVALID &&
        mSourceOverride.sourceType != SourceType::INVALID)
    {
        sourceInfo = mSourceOverride;
    }
    else if (!MetadataParser::parseUri(mMPDPath.getData(), sourceInfo))
    {
        OMAF_LOG_D("Videostream format is unknown, using default values");

        sourceInfo.sourceDirection = SourceDirection::MONO;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
#if UNKNOWN_AS_IDENTITY == 1
        sourceInfo.sourceType = SourceType::IDENTITY;
#endif
    }

    OMAF_LOG_D("MPD type: %s", mMPD->GetType().c_str());
    OMAF_LOG_D("Availability start: %s", mMPD->GetAvailabilityStarttime().c_str());

    mStreamType = DashUtils::getDashStreamType(mMPD);

    mMediaInformation.videoType = VideoType::VARIABLE_RESOLUTION;
    mMediaInformation.fileType = MediaFileType::DASH;
    DashProfile::Enum profile = DashUtils::getDashProfile(mMPD);
    if (mMPD->GetRawAttributes().find("xmlns:omaf") != mMPD->GetRawAttributes().end())
    {
        mMediaInformation.fileType = MediaFileType::DASH_OMAF;
    }
    if (profile == DashProfile::LIVE && mStreamType == DashStreamType::DYNAMIC)
    {
        mMediaInformation.streamType = StreamType::LIVE_STREAM;
    }
    else if (profile == DashProfile::LIVE && mStreamType == DashStreamType::STATIC)
    {
        mMediaInformation.streamType = StreamType::LIVE_ON_DEMAND;
    }
    else if (profile == DashProfile::ONDEMAND)
    {
        mMediaInformation.streamType = StreamType::VIDEO_ON_DEMAND;
    }
    else
    {
        mMediaInformation.streamType = StreamType::OTHER;  // best effort based playback
    }


    DashComponents dashComponents;

    dash::mpd::IPeriod* period = mMPD->GetPeriods().at(0);
    dashComponents.mpd = mMPD;
    dashComponents.basicSourceInfo = sourceInfo;
    dashComponents.period = OMAF_NULL;
    dashComponents.adaptationSet = OMAF_NULL;
    dashComponents.representation = OMAF_NULL;
    dashComponents.segmentTemplate = OMAF_NULL;

    // read vipo entity groups from mpd and initialize viewpoints with background
    // and other related adaptation sets (wrapped to lambda to prevent huge stack allocation for DashEntityGroups variable)
    auto parseVipo = [period](DashComponents& dashComponents, auto& vpPeriods, auto& aCurrentVPPeriodIndex) {
        DashEntityGroups vipoGroups;
        if (DashOmafAttributes::getOMAFEntityGroups(dashComponents, "vipo", vipoGroups) == Error::OK)
        {
            for (uint32_t vipoGroupIndex = 0; vipoGroupIndex < vipoGroups.getSize(); vipoGroupIndex++)
            {
                auto& vipo = vipoGroups.at(vipoGroupIndex);

                MpdViewpointInfo vpInfo;
                DashPeriodViewpoint* viewpoint = OMAF_NULL;

                for (uint32_t entityIndex = 0; entityIndex < vipo.second.getSize(); entityIndex++)
                {
                    auto& entity = vipo.second.at(entityIndex);
                    auto adaptationSet = entity.first;

                    // NOTE: hasViewpointInfo actually parses adaptationSet and sets data to vpInfo
                    if (DashAdaptationSet::hasViewpointInfo(adaptationSet, vpInfo))
                    {
                        // lazily create viwpoint only if it has referred adaptation sets
                        // (real reason behind this was that viewpoint info is not stored in entity
                        //  group and we need that to initialize DashPeriodViewpoint object)
                        if (viewpoint == OMAF_NULL)
                        {
                            viewpoint = OMAF_NEW_HEAP(DashPeriodViewpoint)(*period, vpInfo);
                            vpPeriods.add(viewpoint);

                            //       dash should represent that so that doesn't sound necessary
                            if (vpInfo.isInitial == InitialViewpoint::IS_INITIAL)
                            {
                                dashComponents.period = viewpoint;
                                aCurrentVPPeriodIndex = vpPeriods.getSize() - 1;
                            }
                        }
                    }

                    OMAF_ASSERT(viewpoint != OMAF_NULL, "adaptation sets of viewpoint must have viewpointinfo set");
                    
					OMAF_LOG_D("DashPeriodViewpoint %d: Adding adaptation set %d", viewpoint->getViewpointId(), adaptationSet->GetId());
                    viewpoint->addAdaptationSet(adaptationSet);

                    bool isExtractor = DashAdaptationSetExtractor::isExtractor(adaptationSet);

                    // if background video, also add associated adaptation sets
                    if (isExtractor)
                    {
                        AdaptationSetBundleIds preselection = DashAdaptationSetExtractor::hasPreselection(
                            adaptationSet, DashAdaptationSetExtractor::parseId(adaptationSet));

                        for (auto as : dashComponents.mpd->GetPeriods().at(0)->GetAdaptationSets())
                        {
                            if (preselection.partialAdSetIds.contains(as->GetId()))
                            {
                                // optimize all if becomes a problem... (so many nested O(n) operations)
                                if (!viewpoint->GetAdaptationSets().contains(as))
                                {
                                    viewpoint->addAdaptationSet(as);
                                }
                            }
                        }
                    }
                }
            }
        }
    };
	parseVipo(dashComponents, mViewpointPeriods, mCurrentVPPeriodIndex);

	// add default viewpoint if none were found from dash
    if (mViewpointPeriods.getSize() == 0)
    {
        MpdViewpointInfo vpInfo;
        vpInfo.id = "0";
        DashPeriodViewpoint* viewpoint = OMAF_NEW_HEAP(DashPeriodViewpoint)(*period, vpInfo);
        mViewpointPeriods.add(viewpoint);

        // add all adaptation sets to the viewpoint
        for (uint32_t index = 0; index < period->GetAdaptationSets().size(); index++)
        {
            dash::mpd::IAdaptationSet* adaptationSet = period->GetAdaptationSets().at(index);
            viewpoint->addAdaptationSet(adaptationSet);
        }
    }

    // NOTE: ovbg group is pretty useless in DASH because it doesn't tell if the group member should be treated as
    //       an overlay or background or both...    
	auto parseOvbg = [](DashComponents& dashComponents, auto& vpPeriods) {
        DashEntityGroups ovbgGroups;
        if (DashOmafAttributes::getOMAFEntityGroups(dashComponents, "ovbg", ovbgGroups) == Error::OK)
        {
            for (auto& ovbg : ovbgGroups)
            {
                FixedArray<dash::mpd::IAdaptationSet*, 256> overlays;
                FixedArray<dash::mpd::IAdaptationSet*, 256> backgrounds;

                for (auto& entityId : ovbg.second)
                {
                    bool isOverlay = DashAdaptationSetOverlay::isOverlay(entityId.first);

                    if (isOverlay)
                    {
                        overlays.add(entityId.first);
                    }
                    else
                    {
                        // NOTE: since the backgrounds are already added to each viewpoint this will still work
                        // in case where there is overlays and backgrounds in the same adaptation set.
                        backgrounds.add(entityId.first);
                    }
                }

                // go through each viewpoint and if any of the backgound adaptation sets are found,
                // we must include all the overlay adaptation sets in
                for (auto& vp : vpPeriods)
                {
                    for (auto vpAs : vp->GetAdaptationSets())
                    {
                        if (backgrounds.contains(vpAs))
                        {
                            for (auto ovlyAs : overlays)
                            {
                                // need to check, because it is possible that the overlay is already added as a
                                // backgound media
                                if (!vp->GetAdaptationSets().contains(ovlyAs))
                                {
                                    OMAF_LOG_D("Adding overlay adaptation set %d to viewpoint %d", ovlyAs->GetId(), vp->getViewpointId());
                                    vp->addAdaptationSet(ovlyAs);
                                }
                            }

                            // we do not want to try to add the set of overlays multiple times even if there are
                            // multiple backgrounds in the viewpoint matching the ovbg backrounds
                            break;
                        }
                    }
                }
            }
        }
    };
    parseOvbg(dashComponents, mViewpointPeriods);



    //       though it should be read before any real viewpoint so... (it is such a hack) Maybe invp should not
    //       be read, since initial viewpoint info is passed in dash



    // set initial viewpoint if needed
    if (dashComponents.period == OMAF_NULL)
    {
        mCurrentVPPeriodIndex = 0;
        dashComponents.period = mViewpointPeriods.at(mCurrentVPPeriodIndex);
        OMAF_LOG_D("no initial viewpoint, set first one (id: %d) as initial", dashComponents.period->getViewpointId());
    }

    //       (some adaptation sets are anyways shared between some of the viewpoints)
    DashPeriodViewpoint* commonToViewpoints = OMAF_NEW_HEAP(DashPeriodViewpoint)(*period);

    // LibDASH manages the initial-level adaptation sets, just release the container

    OMAF_DELETE_HEAP(commonToViewpoints);
    commonToViewpoints = OMAF_NULL;

    result = initializeViewpoint(mViewpointPeriods.at(mCurrentVPPeriodIndex), true, sourceInfo.sourceDirection);
    mCurrentViewpoint = mDownloadingViewpoint = mInitializingViewpoint;
    mInitializingViewpoint = OMAF_NULL;

    mMPDUpdatePeriodMS = DashUtils::getMPDUpdateInMilliseconds(dashComponents);
    if (mMPDUpdatePeriodMS == 0)
    {
        // the update period is optional parameter; use default value
        mMPDUpdatePeriodMS = MPD_UPDATE_INTERVAL_MS;
    }

    mPublishTime = DashUtils::getRawAttribute(mMPD, "publishTime");
    mMPDUpdateTimeMS = Time::getClockTimeMs();
    mState = DashDownloadManagerState::INITIALIZED;

    return result;
}

Error::Enum DashDownloadManager::initializeViewpoint(DashPeriodViewpoint* aViewpoint,
                                                     bool_t aRequireVideo,
                                                     SourceDirection::Enum aUriSourceDirection)
{
    OMAF_LOG_V("Viewpoint lock locked at %d", Time::getClockTimeMs());

    mVideoDownloaderCreated = false;

    mInitializingViewpoint = OMAF_NEW_HEAP(DashViewpoint);

    OMAF_LOG_D("New selected Viewpoint (dashComponents->period) is %d", reinterpret_cast<intptr_t>(aViewpoint));

    DashComponents dashComponents;
    dashComponents.mpd = mMPD;
    // dashComponents.basicSourceInfo = sourceInfo;
    dashComponents.period = aViewpoint;
    dashComponents.adaptationSet = OMAF_NULL;
    dashComponents.representation = OMAF_NULL;
    dashComponents.segmentTemplate = OMAF_NULL;

	// pNOTE: arser context is actually owned by download manager and dashComponents is just used to pass it around because
	// currently that seems to be the only data that is passed everywhere... we just trust that it is not used after
	
    dashComponents.parserCtx = &mParserCtx;


    mViewportSetEvent.reset();

    // Next, we scan through the adaptation sets within the viewpoint to see what kind of adaptation sets we do
    // have, e.g. if some of them are only supporting sets (tiles), as that info is needed when initializing the
    // adaptation sets and representations under each adaptation set Partial sets do not necessarily carry that info
    // themselves, but we can detect it only from extractors, if present - and absense of extractor tells something
    // too
    VASType::Enum vasType = VASType::NONE;
    RepresentationDependencies dependingRepresentations;

    for (uint32_t index = 0; index < dashComponents.period->GetAdaptationSets().getSize(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = dashComponents.period->GetAdaptationSets().at(index);
        if (DashAdaptationSetExtractor::isExtractor(adaptationSet))
        {
            AdaptationSetBundleIds preselection = DashAdaptationSetExtractor::hasPreselection(
                adaptationSet, DashAdaptationSetExtractor::parseId(adaptationSet));
            if (!preselection.partialAdSetIds.isEmpty())
            {
                if (DashAdaptationSetExtractor::hasMultiResolution(adaptationSet))
                {
                    // there are more than 1 adaptation set with main adaptation set (extractor) type, containing
                    // lists of supporting partial adaptation sets collect the lists of supporting sets here, since
                    // the supporting set needs to know about its role when initialized, and this may be the only
                    // way to know it
                    vasType = VASType::EXTRACTOR_PRESELECTIONS_RESOLUTION;
                    break;
                }
                else
                {
                    // this adaptation set is a main adaptation set (extractor) and contains a list of supporting
                    // partial adaptation sets collect the lists of supporting sets here, since the supporting set
                    // needs to know about its role when initialized, and this may be the only way to know it
                    vasType = VASType::EXTRACTOR_PRESELECTIONS_QUALITY;
                    break;
                }
            }
            else if (DashAdaptationSetExtractor::hasDependencies(adaptationSet, dependingRepresentations) > 0)
            {
                vasType = VASType::EXTRACTOR_DEPENDENCIES_QUALITY;
                break;
            }
        }
    }
    if (vasType == VASType::NONE)
    {
        // No extractor found. Loop again, now searching for sub-pictures
        for (uint32_t index = 0; index < dashComponents.period->GetAdaptationSets().getSize(); index++)
        {
            dash::mpd::IAdaptationSet* adaptationSet = dashComponents.period->GetAdaptationSets().at(index);
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
        mInitializingViewpoint->videoDownloader = OMAF_NEW_HEAP(DashVideoDownloaderExtractorMultiRes);
        break;
    }
    case VASType::EXTRACTOR_PRESELECTIONS_QUALITY:
    {
        mInitializingViewpoint->videoDownloader = OMAF_NEW_HEAP(DashVideoDownloaderExtractor);
        break;
    }
    case VASType::EXTRACTOR_DEPENDENCIES_QUALITY:
    {
        mInitializingViewpoint->videoDownloader = OMAF_NEW_HEAP(DashVideoDownloaderExtractorDepId);
        break;
    }
    case VASType::SUBPICTURES:
    {
        break;
    }
    default:
    {
        mInitializingViewpoint->videoDownloader = OMAF_NEW_HEAP(DashVideoDownloader);
        break;
    }
    }
    if (mInitializingViewpoint->videoDownloader == OMAF_NULL)
    {
        return Error::OUT_OF_MEMORY;
    }
    Error::Enum result = mInitializingViewpoint->videoDownloader->completeInitialization(
        dashComponents, aUriSourceDirection, getNextSourceIdBase());

    if (!aRequireVideo && result == Error::NOT_SUPPORTED)
    {
        OMAF_DELETE_HEAP(mInitializingViewpoint->videoDownloader);
        mInitializingViewpoint->videoDownloader = OMAF_NULL;
    }
    else if (result != Error::OK)
    {
        OMAF_LOG_E("Stream needs to have video");
        mState = DashDownloadManagerState::STREAM_ERROR;
        return Error::NOT_SUPPORTED;
    }

    if (mInitializingViewpoint->videoDownloader)
    {
        mVideoDownloaderCreated = true;
    }

    /*
     * Audio and metadata
     */

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    for (uint32_t index = 0; index < dashComponents.nonVideoAdaptationSets.getSize(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = dashComponents.nonVideoAdaptationSets.at(index);
        dashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        if (DashAdaptationSetAudio::isAudio(dashComponents))
        {
            // audio adaptation set
            dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetAudio)(*this);
        }
        else
        {
            // "normal" adaptation set
            dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSet)(*this);
        }
        result = dashAdaptationSet->initialize(dashComponents, initializationSegmentId);
        if (result == Error::OK)
        {
            mInitializingViewpoint->nonVideoAdaptationSets.add(dashAdaptationSet);
        }
        else
        {
            OMAF_LOG_W("Initializing an adaptation set returned %d, ignoring it", result);
            OMAF_DELETE_HEAP(dashAdaptationSet);
        }
    }

    if (dashComponents.nonVideoAdaptationSets.getSize() == dashComponents.period->GetAdaptationSets().getSize() &&
        mInitializingViewpoint->nonVideoAdaptationSets.isEmpty())
    {
        // No adaptations sets found, return failure
        mState = DashDownloadManagerState::STREAM_ERROR;
        return Error::NOT_SUPPORTED;
    }

    mInitializingViewpoint->backgroundAudioAdaptationSet = OMAF_NULL;

    for (uint32_t index = 0; index < mInitializingViewpoint->nonVideoAdaptationSets.getSize(); index++)
    {
        DashAdaptationSet* dashAdaptationSet = mInitializingViewpoint->nonVideoAdaptationSets.at(index);
        if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::IS_MUXED))
        {
            mInitializingViewpoint->backgroundAudioAdaptationSet = dashAdaptationSet;
            mInitializingViewpoint->allAudioAdaptationSets.add(dashAdaptationSet);
            break;
        }
        else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_VIDEO))
        {
            // handled in video downloader
        }
        else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_AUDIO))
        {
            // check if this really is the background audio; need to check to what audio there are no associations
            // from video

            mInitializingViewpoint->allAudioAdaptationSets.add(dashAdaptationSet);
            if (mInitializingViewpoint->videoDownloader &&
                mInitializingViewpoint->videoDownloader->hasOverlayDashAssociation(
                    dashAdaptationSet, MPDAttribute::kAssociationTypeValueAudio))
            {
                // cannot be background audio
                // always-active overlay audios are detected later, once we have info if an overlay is active or not
                // (mp4-level)
                continue;
            }

            MediaContent selectedAudio;
            selectedAudio.clear();
            uint32_t selectedAudioBandwidth = 0;
            if (mInitializingViewpoint->backgroundAudioAdaptationSet != OMAF_NULL)
            {
                selectedAudio = mInitializingViewpoint->backgroundAudioAdaptationSet->getAdaptationSetContent();
                selectedAudioBandwidth = mInitializingViewpoint->backgroundAudioAdaptationSet->getCurrentBandwidth();
            }

            if (selectedAudio.matches(MediaContent::Type::AUDIO_LOUDSPEAKER))
            {
                if (dashAdaptationSet->getCurrentBandwidth() > selectedAudioBandwidth)
                {
                    mInitializingViewpoint->backgroundAudioAdaptationSet = dashAdaptationSet;
                    mInitializingViewpoint->activeAudioAdaptationSets.add(
                        mInitializingViewpoint->backgroundAudioAdaptationSet);
                }
            }
            else
            {
                mInitializingViewpoint->backgroundAudioAdaptationSet = dashAdaptationSet;
                mInitializingViewpoint->activeAudioAdaptationSets.add(
                    mInitializingViewpoint->backgroundAudioAdaptationSet);
            }
            ((DashAdaptationSetAudio*) mInitializingViewpoint->backgroundAudioAdaptationSet)
                ->setUserActivateable(false);
#if OMAF_LOOP_DASH
            mInitializingViewpoint->backgroundAudioAdaptationSet->setLoopingOn();
#endif
        }
    }
    for (uint32_t index = 0; index < mInitializingViewpoint->nonVideoAdaptationSets.getSize(); index++)
    {
        DashAdaptationSet* dashAdaptationSet = mInitializingViewpoint->nonVideoAdaptationSets.at(index);
        if (dashAdaptationSet->getAdaptationSetContent().matches(
                MediaContent::Type::METADATA_INVO))  // this is the actual INVO
        {
            mInitializingViewpoint->invoMetadataAdaptationSet = dashAdaptationSet;
        }
    }

    if (mInitializingViewpoint->videoDownloader != OMAF_NULL)
    {
        mInitializingViewpoint->videoDownloader->getMediaInformation(mMediaInformation);
    }

    if (mInitializingViewpoint->backgroundAudioAdaptationSet != OMAF_NULL)
    {
        const MediaContent& mediaContent =
            mInitializingViewpoint->backgroundAudioAdaptationSet->getAdaptationSetContent();
        if (mediaContent.matches(MediaContent::Type::AUDIO_LOUDSPEAKER))
        {
            mMediaInformation.audioFormat = AudioFormat::LOUDSPEAKER;
        }
    }

    if (mInitializingViewpoint->videoDownloader != OMAF_NULL)
    {
        OMAF_LOG_V("Wait for viewport set event at %d", Time::getClockTimeMs());
        // wait for viewport info, needed before start
        bool_t waitResult = mViewportSetEvent.wait(1000);
        if (waitResult)
        {
            OMAF_LOG_V("Viewport set at %d", Time::getClockTimeMs());
        }
        else
        {
            OMAF_LOG_V("Viewport set timeout");
        }
    }

    if (isVideoAndAudioMuxed(mInitializingViewpoint))
    {
        OMAF_LOG_W(
            "The stream has video and audio muxed together in the same adaptation set. This is against the DASH "
            "spec "
            "and may not work correctly");
    }

    return Error::OK;
}

void_t DashDownloadManager::release()
{
    OMAF_DELETE_HEAP(mMPDUpdater);
    mMPDUpdater = OMAF_NULL;
    stopDownload();

    Spinlock::ScopeLock lock(mViewpointSwitchLock);
    if (mCurrentViewpoint)
    {
        if (mDownloadingViewpoint && mCurrentViewpoint != mDownloadingViewpoint)
        {
            releaseViewpoint(mDownloadingViewpoint);
        }
        releaseViewpoint(mCurrentViewpoint);
        mDownloadingViewpoint = OMAF_NULL;
    }
    mVideoDownloaderCreated = false;
    if (mCommonToViewpoints)
    {
        releaseViewpoint(mCommonToViewpoints);
    }

    while (!mViewpointPeriods.isEmpty())
    {
        DashPeriodViewpoint* period = mViewpointPeriods.at(0);
        mViewpointPeriods.removeAt(0);
        OMAF_DELETE_HEAP(period);
    }

    DashSpeedFactorMonitor::destroyInstance();

    delete mMPD;
    mMPD = OMAF_NULL;
    mState = DashDownloadManagerState::IDLE;
}

void_t DashDownloadManager::releaseViewpoint(DashViewpoint*& aViewpoint)
{
    OMAF_LOG_V("releaseViewpoint in");

    while (!aViewpoint->nonVideoAdaptationSets.isEmpty())
    {
        DashAdaptationSet* set = aViewpoint->nonVideoAdaptationSets.at(0);
        aViewpoint->nonVideoAdaptationSets.removeAt(0);
        OMAF_DELETE_HEAP(set);
    }

    aViewpoint->nonVideoAdaptationSets.clear();

    // this clears them all, but updating them from viewpoint & common would be otherwise tricky
    mVideoStreams.clear();
    mCurrentMetadataStreams.clear();
    mCurrentAudioStreams.clear();
    mVideoSources.clear();
    mVideoSourceTypes.clear();

    if (aViewpoint->videoDownloader)
    {
        aViewpoint->videoDownloader->release();
        OMAF_DELETE_HEAP(aViewpoint->videoDownloader);
    }

    aViewpoint->backgroundAudioAdaptationSet = OMAF_NULL;
    aViewpoint->activeAudioAdaptationSets.clear();
    aViewpoint->allAudioAdaptationSets.clear();

    OMAF_DELETE_HEAP(aViewpoint);
    aViewpoint = OMAF_NULL;
}

Error::Enum DashDownloadManager::startDownload()
{
    if (mState != DashDownloadManagerState::INITIALIZED && mState != DashDownloadManagerState::STOPPED)
    {
        return Error::INVALID_STATE;
    }

    time_t startTime = time(0);
    OMAF_LOG_D("Start time: %d", (uint32_t) startTime);

    if (mCommonToViewpoints)
    {
        if (mCommonToViewpoints->videoDownloader)
        {
            mCommonToViewpoints->videoDownloader->startDownload(startTime, mMPDUpdater->mMpdDownloadTimeMs);
        }

        if (!isVideoAndAudioMuxed(mCommonToViewpoints) && !mCommonToViewpoints->allAudioAdaptationSets.isEmpty())
        {
            // start all instead of active ones, to ensure we have streams for all
            for (DashAdaptationSet* audio : mCommonToViewpoints->allAudioAdaptationSets)
            {
                audio->startDownload(startTime);
            }
        }
        if (mCommonToViewpoints->invoMetadataAdaptationSet != OMAF_NULL)
        {
            mCommonToViewpoints->invoMetadataAdaptationSet->startDownload(startTime);
        }
    }

    if (mDownloadingViewpoint && mDownloadingViewpoint->videoDownloader != OMAF_NULL)
    {
        if (mStreamType == DashStreamType::DYNAMIC)
        {
            Error::Enum result = updateMPD(true);
            if (result != Error::OK)
            {
                return result;
            }
        }

        mDownloadingViewpoint->videoDownloader->startDownload(startTime, mMPDUpdater->mMpdDownloadTimeMs);

        if (!isVideoAndAudioMuxed(mDownloadingViewpoint) && !mDownloadingViewpoint->allAudioAdaptationSets.isEmpty())
        {
            // start all instead of active ones, to ensure we have streams for all
            for (DashAdaptationSet* audio : mDownloadingViewpoint->allAudioAdaptationSets)
            {
                audio->startDownload(startTime);
            }
        }
        if (mDownloadingViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
        {
            mDownloadingViewpoint->invoMetadataAdaptationSet->startDownload(startTime);
        }

        mState = DashDownloadManagerState::DOWNLOADING;
        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

Error::Enum DashDownloadManager::startDownload(DashViewpoint* aViewpoint, uint32_t& aTargetTimeMs)
{
    if (mState != DashDownloadManagerState::INITIALIZED && mState != DashDownloadManagerState::STOPPED)
    {
        return Error::INVALID_STATE;
    }

    if (aViewpoint && aViewpoint->videoDownloader != OMAF_NULL)
    {
        if (mStreamType == DashStreamType::DYNAMIC)
        {
            Error::Enum result = updateMPD(true);
            if (result != Error::OK)
            {
                return result;
            }
        }
        aViewpoint->videoDownloader->startDownloadFromTimestamp(aTargetTimeMs, mMPDUpdater->mMpdDownloadTimeMs);

        if (!isVideoAndAudioMuxed(aViewpoint) && !aViewpoint->allAudioAdaptationSets.isEmpty())
        {
            // start all instead of active ones, to ensure we have streams for all
            for (DashAdaptationSet* audio : aViewpoint->allAudioAdaptationSets)
            {
                audio->startDownloadFromTimestamp(aTargetTimeMs, 0);
            }
        }
        if (aViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
        {
            aViewpoint->invoMetadataAdaptationSet->startDownloadFromTimestamp(aTargetTimeMs, 0);
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
    if (mState == DashDownloadManagerState::END_OF_STREAM || mState == DashDownloadManagerState::CONNECTION_ERROR ||
        mState == DashDownloadManagerState::STREAM_ERROR || mState == DashDownloadManagerState::STOPPED)
    {
        return;
    }
    Spinlock::ScopeLock lock(mViewpointSwitchLock);

    mDownloadingViewpoint->videoDownloader->updateStreams(currentPlayTimeUs);
    processSegmentDownload(mDownloadingViewpoint);
    if (mDownloadingViewpoint != mCurrentViewpoint)
    {
        processSegmentDownload(mCurrentViewpoint);
    }
    if (mCommonToViewpoints != OMAF_NULL)
    {
        if (mCommonToViewpoints->videoDownloader)
        {
            mCommonToViewpoints->videoDownloader->updateStreams(currentPlayTimeUs);
        }
        processSegmentDownload(mCommonToViewpoints);
    }

    Error::Enum result = Error::OK;
    int32_t now = Time::getClockTimeMs();
    if (mState == DashDownloadManagerState::DOWNLOADING && mStreamType == DashStreamType::DYNAMIC)
    {

        bool_t updateRequired = mDownloadingViewpoint->videoDownloader->isMpdUpdateRequired();
        if (!isVideoAndAudioMuxed(mDownloadingViewpoint) && !updateRequired &&
            !mDownloadingViewpoint->activeAudioAdaptationSets.isEmpty())
        {
            for (DashAdaptationSet* audio : mDownloadingViewpoint->activeAudioAdaptationSets)
            {
                updateRequired = audio->mpdUpdateRequired();
            }
        }
        // check if either the main video or audio streams are running out of info what segments to download
        // (timeline mode) or required update period is exceeded
        if (updateRequired || (now - mMPDUpdateTimeMS > (int32_t) mMPDUpdatePeriodMS))
        {
            result = updateMPD(false);
            if (result != Error::OK)
            {
                // updateMPD failed, the DownloadManager state is updated inside the updateMPD so just return
                return;
            }
        }
    }

    if (mDownloadingViewpoint->videoDownloader->isEndOfStream())
    {
        OMAF_LOG_V("Download manager goes to EOS");
        mState = DashDownloadManagerState::END_OF_STREAM;
    }
    if (mDownloadingViewpoint->videoDownloader->isError() ||
        (mDownloadingViewpoint->backgroundAudioAdaptationSet != OMAF_NULL &&
         mDownloadingViewpoint->backgroundAudioAdaptationSet->isError()) ||
        (mDownloadingViewpoint->invoMetadataAdaptationSet != OMAF_NULL &&
         mDownloadingViewpoint->invoMetadataAdaptationSet->isError()))
    {
        mState = DashDownloadManagerState::STREAM_ERROR;
    }
}
void_t DashDownloadManager::processSegments()
{
    mCurrentViewpoint->videoDownloader->processSegments();
    // by definition, we don't process mDownloadingViewpoint, only download it
    if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
    {
        mCommonToViewpoints->videoDownloader->processSegments();
    }
}

Error::Enum DashDownloadManager::stopDownload()
{
    if (mState != DashDownloadManagerState::DOWNLOADING && mState != DashDownloadManagerState::STREAM_ERROR &&
        mState != DashDownloadManagerState::CONNECTION_ERROR)
    {
        return Error::INVALID_STATE;
    }

    if (mDownloadingViewpoint && mDownloadingViewpoint->videoDownloader != OMAF_NULL)
    {
        mDownloadingViewpoint->videoDownloader->stopDownload();

        if (mDownloadingViewpoint != mCurrentViewpoint)
        {
            mCurrentViewpoint->videoDownloader->stopDownload();
        }
        if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
        {
            mCommonToViewpoints->videoDownloader->stopDownload();
        }

        if (!isVideoAndAudioMuxed(mDownloadingViewpoint))
        {
            for (DashAdaptationSet* audio : mDownloadingViewpoint->allAudioAdaptationSets)
            {
                audio->stopDownload();
            }
            if (mDownloadingViewpoint != mCurrentViewpoint)
            {
                for (DashAdaptationSet* audio : mCurrentViewpoint->allAudioAdaptationSets)
                {
                    audio->stopDownload();
                }
            }
            if (mCommonToViewpoints)
            {
                for (DashAdaptationSet* audio : mCommonToViewpoints->allAudioAdaptationSets)
                {
                    audio->stopDownload();
                }
            }
        }
        if (mDownloadingViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
        {
            mDownloadingViewpoint->invoMetadataAdaptationSet->stopDownload();
            if (mDownloadingViewpoint != mCurrentViewpoint)
            {
                mCurrentViewpoint->invoMetadataAdaptationSet->stopDownload();
            }
        }
        if (mCommonToViewpoints && mCommonToViewpoints->invoMetadataAdaptationSet != OMAF_NULL)
        {
            mCommonToViewpoints->invoMetadataAdaptationSet->stopDownload();
        }
        mState = DashDownloadManagerState::STOPPED;

        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

Error::Enum DashDownloadManager::stopDownloadCurrentViewpointAsync(uint32_t& aLastTimeMs)
{
    if (mState != DashDownloadManagerState::DOWNLOADING && mState != DashDownloadManagerState::STREAM_ERROR &&
        mState != DashDownloadManagerState::CONNECTION_ERROR)
    {
        return Error::INVALID_STATE;
    }

    if (mDownloadingViewpoint && mDownloadingViewpoint->videoDownloader != OMAF_NULL)
    {
        mDownloadingViewpoint->videoDownloader->stopDownloadAsync(aLastTimeMs);

        if (!isVideoAndAudioMuxed(mDownloadingViewpoint))
        {
            for (DashAdaptationSet* audio : mDownloadingViewpoint->allAudioAdaptationSets)
            {
                audio->stopDownloadAsync(false, true);
            }
        }
        if (mDownloadingViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
        {
            mDownloadingViewpoint->invoMetadataAdaptationSet->stopDownloadAsync(false, true);
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
    if (mCurrentViewpoint != mDownloadingViewpoint)
    {
        doClearDownloadedContent(mCurrentViewpoint);
    }
    doClearDownloadedContent(mDownloadingViewpoint);
    if (mCommonToViewpoints)
    {
        doClearDownloadedContent(mCommonToViewpoints);
    }
}

void_t DashDownloadManager::doClearDownloadedContent(DashViewpoint* aViewpoint)
{
    if (!aViewpoint)
    {
        return;
    }
    if (aViewpoint->videoDownloader != OMAF_NULL)
    {
        aViewpoint->videoDownloader->clearDownloadedContent();
    }
    if (!isVideoAndAudioMuxed(aViewpoint))
    {
        for (DashAdaptationSet* audio : aViewpoint->allAudioAdaptationSets)
        {
            audio->clearDownloadedContent();
        }
    }
    if (aViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
    {
        aViewpoint->invoMetadataAdaptationSet->clearDownloadedContent();
    }
}

DashDownloadManagerState::Enum DashDownloadManager::getState()
{
    return mState;
}

bool_t DashDownloadManager::isVideoAndAudioMuxed(DashViewpoint* aViewpoint)
{
    if (!aViewpoint || !aViewpoint->backgroundAudioAdaptationSet)
    {
        return false;
    }
    return aViewpoint->backgroundAudioAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::IS_MUXED);
}

Error::Enum DashDownloadManager::seekToMs(uint64_t& aSeekMs)
{
    Error::Enum result = Error::OK;

    if (mDownloadingViewpoint)
    {
        result = doSeekToMs(mDownloadingViewpoint, aSeekMs);
    }
    if (mCommonToViewpoints)
    {
        result = doSeekToMs(mDownloadingViewpoint, aSeekMs);
    }
    return result;
}

Error::Enum DashDownloadManager::doSeekToMs(DashViewpoint* aViewpoint, uint64_t& aSeekMs)
{
    uint64_t seekToTarget = aSeekMs;
    uint64_t seekToResultMs = 0;
    Error::Enum result = Error::OK;

    if (aViewpoint->videoDownloader != OMAF_NULL)
    {
        result = aViewpoint->videoDownloader->seekToMs(seekToTarget, seekToResultMs);
    }

    // treat mBackgroundAudioAdaptationSet as separate from mVideoBaseAdaptationSet in case of audio only stream.
    if (!isVideoAndAudioMuxed(aViewpoint) && aViewpoint->backgroundAudioAdaptationSet &&
        result == Error::OK)
    {
        result = aViewpoint->backgroundAudioAdaptationSet->seekToMs(seekToTarget, seekToResultMs);
        aSeekMs = seekToResultMs;
    }
    if (aViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
    {
        aViewpoint->invoMetadataAdaptationSet->seekToMs(seekToTarget, seekToResultMs);
    }

    return result;
}

bool_t DashDownloadManager::isSeekable()
{
    return mDownloadingViewpoint->videoDownloader->isSeekable();
}

uint64_t DashDownloadManager::durationMs() const
{
    return mCurrentViewpoint->videoDownloader->durationMs();
}

const MP4VideoStreams& DashDownloadManager::getVideoStreams()
{
    if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
    {
        mVideoStreams.clear();
        mVideoStreams.add(doGetVideoStreams(mCurrentViewpoint));
        mVideoStreams.add(doGetVideoStreams(mCommonToViewpoints));
        return mVideoStreams;
    }
    else
    {
        return doGetVideoStreams(mCurrentViewpoint);
    }
}

const MP4VideoStreams& DashDownloadManager::doGetVideoStreams(DashViewpoint* aViewpoint)
{
    return aViewpoint->videoDownloader->getVideoStreams();
}

void_t DashDownloadManager::getAudioStreams(MP4AudioStreams& aMainAudioStreams,
                                            MP4AudioStreams& aAdditionalAudioStreams)
{
    doGetAudioStreams(mCurrentViewpoint, aMainAudioStreams, aAdditionalAudioStreams);
    if (mCommonToViewpoints)
    {
        doGetAudioStreams(mCommonToViewpoints, aMainAudioStreams, aAdditionalAudioStreams);
    }
}

void_t DashDownloadManager::doGetAudioStreams(DashViewpoint* aViewpoint,
                                              MP4AudioStreams& aMainAudioStreams,
                                              MP4AudioStreams& aAdditionalAudioStreams)
{
    if (aViewpoint->backgroundAudioAdaptationSet != OMAF_NULL)
    {
        aMainAudioStreams.add(aViewpoint->backgroundAudioAdaptationSet->getCurrentAudioStreams());
    }
    if (aViewpoint->activeAudioAdaptationSets.getSize() > 0)
    {
        for (DashAdaptationSet* audio : aViewpoint->activeAudioAdaptationSets)
        {
            if (audio != aViewpoint->backgroundAudioAdaptationSet)
            {
                aAdditionalAudioStreams.add(audio->getCurrentAudioStreams());
            }
        }
    }
}

const MP4AudioStreams& DashDownloadManager::getAudioStreams()
{
    if (mCurrentAudioStreams.isEmpty())
    {
        doGetAudioStreams(mCurrentViewpoint);
        if (mCommonToViewpoints)
        {
            doGetAudioStreams(mCommonToViewpoints);
        }
    }
    return mCurrentAudioStreams;
}

void_t DashDownloadManager::doGetAudioStreams(DashViewpoint* aViewpoint)
{
    if (!aViewpoint->activeAudioAdaptationSets.isEmpty())
    {
        for (DashAdaptationSet* audio : aViewpoint->activeAudioAdaptationSets)
        {
            mCurrentAudioStreams.add(audio->getCurrentAudioStreams());
        }
    }
}

const MP4MetadataStreams& DashDownloadManager::getMetadataStreams()
{
    if (mCurrentMetadataStreams.isEmpty())
    {
        doGetMetadataStreams(mCurrentViewpoint);
        if (mCommonToViewpoints)
        {
            doGetMetadataStreams(mCommonToViewpoints);
        }
    }
    return mCurrentMetadataStreams;
}

void_t DashDownloadManager::doGetMetadataStreams(DashViewpoint* aViewpoint)
{
    if (aViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
    {
        mCurrentMetadataStreams.add(aViewpoint->invoMetadataAdaptationSet->getCurrentMetadataStreams());
    }
    if (aViewpoint->videoDownloader)
    {
        mCurrentMetadataStreams.add(aViewpoint->videoDownloader->getMetadataStreams());
    }
}

DashAdaptationSet* DashDownloadManager::findAssociatedAdaptationSet(DashViewpoint* aViewpoint, streamid_t aStreamId)
{
    const char_t* representationId =
        aViewpoint->videoDownloader->findDashAssociationForStream(aStreamId, MPDAttribute::kAssociationTypeValueAudio);

    for (DashAdaptationSet* audio : aViewpoint->allAudioAdaptationSets)
    {
        if (StringCompare(audio->getCurrentRepresentationId(), representationId) == ComparisonResult::EQUAL)
        {
            return audio;
        }
    }
    return OMAF_NULL;
}

bool_t DashDownloadManager::stopOverlayAudioStream(streamid_t aVideoStreamId, streamid_t& aStoppedAudioStreamId)
{
    if (!doStopOverlayAudioStream(mCurrentViewpoint, aVideoStreamId, aStoppedAudioStreamId))
    {
        if (mCommonToViewpoints)
        {
            if (doStopOverlayAudioStream(mCommonToViewpoints, aVideoStreamId, aStoppedAudioStreamId))
            {
                updateBandwidth();
                return true;
            }
        }
        return false;
    }
    updateBandwidth();
    return true;
}

bool_t DashDownloadManager::doStopOverlayAudioStream(DashViewpoint* aViewpoint,
                                                     streamid_t aVideoStreamId,
                                                     streamid_t& aStoppedAudioStreamId)
{
    if (aViewpoint->allAudioAdaptationSets.isEmpty() ||
        (aViewpoint->activeAudioAdaptationSets.getSize() == 1 &&
         aViewpoint->activeAudioAdaptationSets.at(0) == aViewpoint->backgroundAudioAdaptationSet))
    {
        return false;
    }
    DashAdaptationSet* audio = findAssociatedAdaptationSet(aViewpoint, aVideoStreamId);

    if (audio != OMAF_NULL)
    {
        if (aViewpoint->activeAudioAdaptationSets.contains(audio))
        {
            // disabling an overlay that had audio playing
            // reset audio playhead to the beginning of the stream
            uint64_t start = 0;
            uint64_t actual = 0;
            audio->seekToMs(start, actual);  // also resets packets

            uint32_t sampleId = 0;
            audio->getCurrentAudioStreams().at(0)->findSampleIdByIndex(0, sampleId);
            bool_t segmentChanged = false;
            OMAF_LOG_V("Reset audio stream %d", audio->getCurrentAudioStreams().at(0)->getStreamId());
            audio->getCurrentAudioStreams().at(0)->setNextSampleId(sampleId, segmentChanged);
            audio->stopDownloadAsync(false, true);

            aViewpoint->activeAudioAdaptationSets.remove(audio);
            mCurrentAudioStreams.remove(audio->getCurrentAudioStreams().at(0));
            aStoppedAudioStreamId = audio->getCurrentAudioStreams().at(0)->getStreamId();
            return true;
        }
        else
        {
            OMAF_LOG_D("The closed overlay didn't have audio playing");
        }
    }
    return false;
}

MP4AudioStream* DashDownloadManager::switchAudioStream(streamid_t aVideoStreamId,
                                                       streamid_t& aStoppedAudioStreamId,
                                                       bool_t aPreviousOverlayStopped)
{
    aStoppedAudioStreamId = InvalidStreamId;
    MP4AudioStream* newAudioStream =
        doSwitchAudioStream(mCurrentViewpoint, aVideoStreamId, aStoppedAudioStreamId, aPreviousOverlayStopped);

    if (!newAudioStream || aStoppedAudioStreamId == InvalidStreamId)
    {
        if (mCommonToViewpoints)
        {
            MP4AudioStream* newAudioStream2 = doSwitchAudioStream(mCommonToViewpoints, aVideoStreamId,
                                                                  aStoppedAudioStreamId, aPreviousOverlayStopped);
            if (newAudioStream2)
            {
                updateBandwidth();
                return newAudioStream2;
            }
        }
    }
    updateBandwidth();
    return newAudioStream;
}

MP4AudioStream* DashDownloadManager::doSwitchAudioStream(DashViewpoint* aViewpoint,
                                                         streamid_t aVideoStreamId,
                                                         streamid_t& aStoppedAudioStreamId,
                                                         bool_t aPreviousOverlayStopped)
{
    DashAdaptationSet* newAudio = findAssociatedAdaptationSet(mCurrentViewpoint, aVideoStreamId);

    if (newAudio != OMAF_NULL || aPreviousOverlayStopped)
    {
        // the new one has audio => stop the current overlay audio
        DashAdaptationSetAudio* currentOverlayAudio = OMAF_NULL;
        if (!mCurrentViewpoint->activeAudioAdaptationSets.isEmpty())
        {
            if (mCurrentViewpoint->activeAudioAdaptationSets.at(0) == mCurrentViewpoint->backgroundAudioAdaptationSet)
            {
                if (mCurrentViewpoint->activeAudioAdaptationSets.getSize() > 1)
                {
                    // take the last one as audios are in stack
                    // other
                    // => fix the dependency with a common data structure or something?
                    currentOverlayAudio = (DashAdaptationSetAudio*) mCurrentViewpoint->activeAudioAdaptationSets.back();
                }
            }
            else
            {
                // This is assuming the latest one is played in AACAudioRenderer
                currentOverlayAudio = (DashAdaptationSetAudio*) mCurrentViewpoint->activeAudioAdaptationSets.back();
            }
            if (currentOverlayAudio != OMAF_NULL)
            {
                if (currentOverlayAudio->isUserActivateable() == UserActivation::ALWAYS_ON)
                {
                    OMAF_LOG_V("Overlay is always on, don't turn off audio (ad set %d)", currentOverlayAudio->getId());
                    currentOverlayAudio = OMAF_NULL;
                }
            }
        }

        if (currentOverlayAudio != OMAF_NULL)
        {
            OMAF_LOG_V("Stop currently playing audio %d",
                       currentOverlayAudio->getCurrentAudioStreams().at(0)->getStreamId());
            // reset audio playhead of the current overlay to the beginning of the stream
            uint64_t start = 0;
            uint64_t actual = 0;
            currentOverlayAudio->seekToMs(start, actual);  // also resets packets

            uint32_t sampleId = 0;
            currentOverlayAudio->getCurrentAudioStreams().at(0)->findSampleIdByIndex(0, sampleId);
            bool_t segmentChanged = false;
            OMAF_LOG_V("Reset audio stream %d", currentOverlayAudio->getCurrentAudioStreams().at(0)->getStreamId());
            currentOverlayAudio->getCurrentAudioStreams().at(0)->setNextSampleId(sampleId, segmentChanged);
            currentOverlayAudio->stopDownloadAsync(false, true);

            mCurrentViewpoint->activeAudioAdaptationSets.remove(currentOverlayAudio);
            mCurrentAudioStreams.remove(currentOverlayAudio->getCurrentAudioStreams().at(0));
            aStoppedAudioStreamId = currentOverlayAudio->getCurrentAudioStreams().at(0)->getStreamId();
        }
        if (newAudio)
        {
            mCurrentViewpoint->activeAudioAdaptationSets.add(newAudio);
            OMAF_LOG_D("Activated audio adaptation set %d", newAudio->getId());
            if (newAudio->getCurrentAudioStreams().isEmpty())
            {
                // for this reason, we start downloading all streams in the beginning and process their segments
                OMAF_LOG_W("No audio stream ready yet");
                return OMAF_NULL;
            }
            newAudio->startDownload(0);  // restart in case the overlay was stopped; startTime is relevant with
                                         // live, but assuming overlays always start from 0
            mCurrentAudioStreams.add(newAudio->getCurrentAudioStreams().at(0));
            return newAudio->getCurrentAudioStreams().at(0);
        }
    }
    return OMAF_NULL;
}

bool_t DashDownloadManager::switchOverlayVideoStream(streamid_t aVideoStreamId,
                                                     int64_t currentTimeUs,
                                                     bool_t& aPreviousOverlayStopped)
{
    aPreviousOverlayStopped = false;
    bool_t started = mCurrentViewpoint->videoDownloader->switchOverlayVideoStream(aVideoStreamId, currentTimeUs,
                                                                                  aPreviousOverlayStopped);

    if (!started || !aPreviousOverlayStopped)
    {
        if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
        {
            bool_t started2 = mCommonToViewpoints->videoDownloader->switchOverlayVideoStream(
                aVideoStreamId, currentTimeUs, aPreviousOverlayStopped);

            updateBandwidth();
            return started2;
        }
    }
    return started;
}

bool_t DashDownloadManager::stopOverlayVideoStream(streamid_t aVideoStreamId)
{
    if (mCurrentViewpoint->videoDownloader->stopOverlayVideoStream(aVideoStreamId))
    {
        return true;
    }
    if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
    {
        if (mCommonToViewpoints->videoDownloader->stopOverlayVideoStream(aVideoStreamId))
        {
            updateBandwidth();
            return true;
        }
    }
    return false;
}

size_t DashDownloadManager::getNrOfViewpoints() const
{
    return mViewpointPeriods.getSize();
}

Error::Enum DashDownloadManager::getViewpointId(size_t aIndex, uint32_t& aId) const
{
    if (mViewpointPeriods.isEmpty() || aIndex >= mViewpointPeriods.getSize())
    {
        return Error::ITEM_NOT_FOUND;
    }
    aId = mViewpointPeriods.at(aIndex)->getViewpointId();
    return Error::OK;
}

Error::Enum DashDownloadManager::setViewpoint(uint32_t aDestinationId)
{
    if (mViewpointPeriods.isEmpty())
    {
        return Error::ITEM_NOT_FOUND;
    }
    else if (aDestinationId == mViewpointPeriods.at(mCurrentVPPeriodIndex)->getViewpointId())
    {
        return Error::OK_NO_CHANGE;
    }

    size_t index = 0;
    for (; index < mViewpointPeriods.getSize(); index++)
    {
        if (mViewpointPeriods[index]->getViewpointId() == aDestinationId)
        {
            break;
        }
    }
    if (index >= mViewpointPeriods.getSize())
    {
        return Error::ITEM_NOT_FOUND;
    }

    OMAF_LOG_D("setViewpoint to index %d id %d at %d", index, mViewpointPeriods.at(index)->getViewpointId(),
               Time::getClockTimeMs());

    // stop the current one
    stopDownloadCurrentViewpointAsync(mTargetSwitchTimeMs);
    uint32_t newTimelineMs = mTargetSwitchTimeMs;
    OMAF_LOG_D("setViewpoint stop completed at %d", Time::getClockTimeMs());

    // read switching info
    const ViewpointSwitchingList& viewpointInfo = mCurrentViewpoint->videoDownloader->getViewpointSwitchConfig();

    if (!viewpointInfo.isEmpty())
    {
        for (size_t i = 0; i < viewpointInfo.getSize(); i++)
        {
            if (viewpointInfo[i].destinationViewpoint == aDestinationId)
            {
                // destination matches, use this switch-info
                if (viewpointInfo[i].absoluteOffsetUsed)
                {
                    newTimelineMs = viewpointInfo[i].absoluteOffset;
                    OMAF_LOG_D("Set new timeline: abs %d", newTimelineMs);
                }
                else
                {
                    newTimelineMs += viewpointInfo[i].relativeOffset;
                    OMAF_LOG_D("Set new timeline: rel %d, result %d", viewpointInfo[i].relativeOffset, newTimelineMs);
                }
                break;
            }
        }
    }

    mCurrentVPPeriodIndex = index;
    if (1)
    {
        OMAF_LOG_D("Switching viewpoint without playing the cached data");
        // This way we ensure the current viewpoint playback is stopped when switching.
        doClearDownloadedContent(mCurrentViewpoint);
        // By changing the mTargetSwitchTimeMs, we enable to switch the viewpoint
        // as soon as the new viewpoint is ready. The new viewpoint is still started at newTimelineMs.
        mTargetSwitchTimeMs = 0;
    }

    OMAF_LOG_D("setViewpoint initialize new viewpoint at %d", Time::getClockTimeMs());
    return mViewpointInitThread.start(newTimelineMs);
}

size_t DashDownloadManager::getCurrentViewpointIndex() const
{
    return mCurrentVPPeriodIndex;
}


Error::Enum DashDownloadManager::readVideoFrames(int64_t currentTimeUs)
{
    if (mCurrentViewpoint || mCommonToViewpoints)
    {
        Error::Enum result = mCurrentViewpoint->videoDownloader->readVideoFrames(currentTimeUs);
        if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
        {
            result = mCommonToViewpoints->videoDownloader->readVideoFrames(currentTimeUs);
        }
        return result;
    }
    else
    {
        return Error::INVALID_STATE;
    }
}

Error::Enum DashDownloadManager::readAudioFrames()
{
    /*OMAF_ASSERT(!(mCurrentViewpoint->activeAudioAdaptationSets.isEmpty() ||
                    mCommonToViewpoints && mCommonToViewpoints->activeAudioAdaptationSets.isEmpty())
        , "No current audio adaptation set");
        */
    Error::Enum result = Error::OK;
    for (DashAdaptationSet* audio : mCurrentViewpoint->activeAudioAdaptationSets)
    {
        Error::Enum latest = audio->readNextAudioFrame();
        if (latest != Error::OK)
        {
            result = latest;
        }
    }
    if (mCommonToViewpoints)
    {
        for (DashAdaptationSet* audio : mCommonToViewpoints->activeAudioAdaptationSets)
        {
            Error::Enum latest = audio->readNextAudioFrame();
            if (latest != Error::OK)
            {
                result = latest;
            }
        }
    }
    return result;
}

Error::Enum DashDownloadManager::readMetadata(int64_t aCurrentTimeUs)
{
    Error::Enum result = Error::OK;
    if (mCurrentViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
    {
        result = mCurrentViewpoint->invoMetadataAdaptationSet->readMetadataFrame(aCurrentTimeUs);
    }
    else
    {
        result = mCurrentViewpoint->videoDownloader->readMetadataFrame(aCurrentTimeUs);
    }
    if (mCommonToViewpoints)
    {
        if (mCommonToViewpoints->invoMetadataAdaptationSet != OMAF_NULL)
        {
            result = mCommonToViewpoints->invoMetadataAdaptationSet->readMetadataFrame(aCurrentTimeUs);
        }
        else if (mCommonToViewpoints->videoDownloader)
        {
            result = mCommonToViewpoints->videoDownloader->readMetadataFrame(aCurrentTimeUs);
        }
    }
    return result;
}

MP4VRMediaPacket* DashDownloadManager::getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs)
{
    if (mCurrentViewpoint->videoDownloader)
    {
        return mCurrentViewpoint->videoDownloader->getNextVideoFrame(stream, currentTimeUs);
    }
    else if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
    {
        return mCommonToViewpoints->videoDownloader->getNextVideoFrame(stream, currentTimeUs);
    }
    else
    {
        return OMAF_NULL;
    }
}

MP4VRMediaPacket* DashDownloadManager::getNextAudioFrame(MP4MediaStream& stream)
{
    // With Dash we don't need to do anything special here
    return stream.peekNextFilledPacket();
}

MP4VRMediaPacket* DashDownloadManager::getMetadataFrame(MP4MediaStream& aStream, int64_t aCurrentTimeUs)
{
    MP4VRMediaPacket* packet = aStream.peekNextFilledPacket();
    if (packet != OMAF_NULL)
    {
        if (packet->presentationTimeUs() <= aCurrentTimeUs || aCurrentTimeUs < 0)
        {
            return packet;
        }
    }
    return OMAF_NULL;
}

Error::Enum DashDownloadManager::updateMetadata(const MP4MediaStream& metadataStream,
                                                const MP4VRMediaPacket& metadataFrame)
{
    if (mCurrentViewpoint->videoDownloader->updateMetadata(metadataStream, metadataFrame) == Error::OK_SKIPPED)
    {
        if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
        {
            return mCommonToViewpoints->videoDownloader->updateMetadata(metadataStream, metadataFrame);
        }
    }
    return Error::OK;
}

const CoreProviderSourceTypes& DashDownloadManager::getVideoSourceTypes()
{
    // OMAF_LOG_V("getVideoSourceTypes in at %d", Time::getClockTimeMs());
    Spinlock::ScopeLock lock(mViewpointSwitchLock);
    OMAF_ASSERT(mCurrentViewpoint->videoDownloader != OMAF_NULL, "No current video downloader");
    if (mVideoSourceTypes.isEmpty())
    {
        mVideoSourceTypes.add(mCurrentViewpoint->videoDownloader->getVideoSourceTypes());

        if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
        {
            mVideoSourceTypes.add(mCommonToViewpoints->videoDownloader->getVideoSourceTypes());
        }
    }

    return mVideoSourceTypes;
}

bool_t DashDownloadManager::isDynamicStream()
{
    return mStreamType == DashStreamType::DYNAMIC;
}

bool_t DashDownloadManager::notReadyToGo()
{
    const CoreProviderSourceTypes& sourceTypes = getVideoSourceTypes();
    if (sourceTypes.isEmpty())
    {
        return true;
    }
    return isBuffering();
}

bool_t DashDownloadManager::isBuffering()
{
    if (mState == DashDownloadManagerState::END_OF_STREAM || mState == DashDownloadManagerState::CONNECTION_ERROR ||
        mState == DashDownloadManagerState::STOPPED || mState == DashDownloadManagerState::STREAM_ERROR)
    {
        return false;
    }

    bool_t videoBuffering = mCurrentViewpoint->videoDownloader->isBuffering();

    bool_t audioBuffering = false;
    if (!mCurrentViewpoint->activeAudioAdaptationSets.isEmpty())
    {
        for (DashAdaptationSet* audio : mCurrentViewpoint->activeAudioAdaptationSets)
        {
            audioBuffering |= audio->isBuffering();
        }
    }
    if ((!videoBuffering || !audioBuffering) && mCommonToViewpoints)
    {
        if (mCommonToViewpoints->videoDownloader)
        {
            videoBuffering = mCommonToViewpoints->videoDownloader->isBuffering();
        }

        if (!mCommonToViewpoints->activeAudioAdaptationSets.isEmpty())
        {
            for (DashAdaptationSet* audio : mCommonToViewpoints->activeAudioAdaptationSets)
            {
                audioBuffering |= audio->isBuffering();
            }
        }
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
    if (mCurrentViewpoint->videoDownloader->isEndOfStream())
    {
        if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
        {
            // if both exists, both need to be EoS
            return mCommonToViewpoints->videoDownloader->isEndOfStream();
        }
    }
    return false;
}

bool_t DashDownloadManager::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    return mCurrentViewpoint->videoDownloader->isReadyToSignalEoS(aStream);
}

const CoreProviderSources& DashDownloadManager::getPastBackgroundVideoSourcesAt(uint64_t aPts)
{
    return mCurrentViewpoint->videoDownloader->getPastBackgroundVideoSourcesAt(aPts);
}

bool_t DashDownloadManager::setInitialViewport(float64_t longitude,
                                               float64_t latitude,
                                               float64_t roll,
                                               float64_t width,
                                               float64_t height)
{
    if (mVideoDownloaderCreated && mInitializingViewpoint)
    {
        OMAF_LOG_V("setInitialViewingOrientation, longitude %f", longitude);
        bool_t aViewportSet = false;
        mInitializingViewpoint->videoDownloader->selectSources(longitude, latitude, roll, width, height, aViewportSet);
        if (aViewportSet)
        {
            mViewportSetEvent.signal();
            return true;
        }
    }
    return false;
}

const CoreProviderSources& DashDownloadManager::getAllSources()
{
    Spinlock::ScopeLock lock(mViewpointSwitchLock);
    mVideoSources.clear();
    if (mCurrentViewpoint && mCurrentViewpoint->videoDownloader != OMAF_NULL)
    {
        mVideoSources.add(mCurrentViewpoint->videoDownloader->getAllSources());
        if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
        {
            mVideoSources.add(mCommonToViewpoints->videoDownloader->getAllSources());
        }
    }
    return mVideoSources;
}

const MP4VideoStreams& DashDownloadManager::selectSources(float64_t longitude,
                                                          float64_t latitude,
                                                          float64_t roll,
                                                          float64_t width,
                                                          float64_t height)
{
    Spinlock::ScopeLock lock(mViewpointSwitchLock);
    bool_t set;
    return mCurrentViewpoint->videoDownloader->selectSources(longitude, latitude, roll, width, height, set);
}

DashDownloadManager::MPDDownloadThread::MPDDownloadThread(dash::IDASHManager* aDashManager)
    : mAllocator(*MemorySystem::DefaultHeapAllocator())
    , mDashManager(aDashManager)
    , mData(mAllocator, 64 * 1024)
{
    mConnection = Http::createHttpConnection(mAllocator);
}
DashDownloadManager::MPDDownloadThread::~MPDDownloadThread()
{
    OMAF_DELETE(mAllocator, mConnection);  // dtor aborts and waits if needed.
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
    if (state == HttpConnectionState::IDLE || state == HttpConnectionState::INVALID)
        return false;
    return true;
}
bool DashDownloadManager::MPDDownloadThread::isReady()
{
    const HttpConnectionState::Enum& state = mConnection->getState().connectionState;
    if (state == HttpConnectionState::IDLE)
        return true;
    if (state == HttpConnectionState::FAILED)
        return true;
    if (state == HttpConnectionState::ABORTED)
        return true;
    if (state == HttpConnectionState::COMPLETED)
        return true;
    return false;
}
dash::mpd::IMPD* DashDownloadManager::MPDDownloadThread::getResult(Error::Enum& aResult)
{
    const HttpRequestState& state = mConnection->getState();
    if ((state.connectionState == HttpConnectionState::COMPLETED) &&
        ((state.httpStatus >= 200) && (state.httpStatus < 300)))
    {
        uint64_t ps = Clock::getMilliseconds();
        mMpdDownloadTimeMs = (uint32_t)(ps - mStart);
        OMAF_LOG_V("[%p] MPD downloading took %lld", this, mMpdDownloadTimeMs);
        dash::mpd::IMPD* newMPD =
            mDashManager->Open((const char*) mData.getDataPtr(), mData.getSize(), mMPDPath.getData());
        OMAF_LOG_V("[%p] MPD parsing took %lld", this, Clock::getMilliseconds() - ps);
        if (newMPD != OMAF_NULL)
        {
            const std::vector<std::string>& profiles = newMPD->GetProfiles();
            bool_t foundSupportedProfile = false;
            for (int i = 0; i < profiles.size(); i++)
            {
				
                if (profiles[i] == "urn:mpeg:dash:profile:isoff-live:2011" ||
                    profiles[i] == "urn:mpeg:dash:profile:isoff-on-demand:2011" ||
                    profiles[i] == "urn:mpeg:dash:profile:isoff-main:2011" ||
                    profiles[i] == "urn:mpeg:dash:profile:full:2011")
                {
                    // Supported profile - best effort; may or may not work
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
        // recreate the mConnection, since if it is in failed state, it stays in that forever.
        // We may still want to retry
        OMAF_DELETE(mAllocator, mConnection);  // dtor aborts and waits if needed.
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
        else  // if (result == Error::OK)
        {
            const std::string& newPublishTime = DashUtils::getRawAttribute(newMPD, "publishTime");
            // Ignore the publish time check since some encoders & CDNs don't update the value
            // if (newPublishTime != mPublishTime)
            {
                OMAF_LOG_V("[%p] MPD updated %s != %s", this, mPublishTime.c_str(), newPublishTime.c_str());
                dash::mpd::IPeriod* period = newMPD->GetPeriods().at(0);

                if (period == OMAF_NULL)
                {
                    OMAF_LOG_D("[%p] NOT_SUPPORTED (null period)", this);
                    mState = DashDownloadManagerState::STREAM_ERROR;
                    return Error::NOT_SUPPORTED;
                }


                // enabling the else branch
#if 1
                delete newMPD;
                OMAF_ASSERT_NOT_IMPLEMENTED();

#else
                // For now we don't allow new adaptation sets at this point
                if (period->GetAdaptationSets().size() !=.getSize())
                {
                    OMAF_LOG_E("[%p] MPD update error, adaptation set count changed", this);
                    mState = DashDownloadManagerState::STREAM_ERROR;
                    return Error::NOT_SUPPORTED;
                }

                // viewpoints, either as audio/meta or located further down in videodownloader
                // Assuming the order remains the same is not safe
                for (uint32_t index = 0; index < period->GetAdaptationSets().size(); index++)
                {
                    DashComponents components;
                    components.mpd = newMPD;
                    //                    components.period = period;
                    components.adaptationSet = period->GetAdaptationSets().at(index);
                    components.representation = OMAF_NULL;
                    components.basicSourceInfo.sourceDirection = SourceDirection::INVALID;
                    components.basicSourceInfo.sourceType = SourceType::INVALID;
                    //    mNonVideoAdaptationSets.at(index)->updateMPD(components);
                }
                delete mMPD;
                mMPD = newMPD;
                mPublishTime = newPublishTime;
                mMPDUpdateTimeMS = Time::getClockTimeMs();
#endif
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
            // adjust update time, so that we only wait for half of the normal update interval)
            mMPDUpdateTimeMS = Time::getClockTimeMs() - (mMPDUpdatePeriodMS / 2);
        }
    }
    return Error::OK;
}

void_t DashDownloadManager::processSegmentDownload(DashViewpoint* aViewpoint)
{
    // OMAF_LOG_D("%lld processSegmentDownload in", Time::getClockTimeMs());
    // use all instead of active, to ensure we have streams for all.
    if (!aViewpoint->allAudioAdaptationSets.isEmpty())
    {
        bool_t requiresUpdate = false;
        for (DashAdaptationSet* audio : aViewpoint->allAudioAdaptationSets)
        {
            requiresUpdate |= audio->processSegmentDownload();
        }
        if (requiresUpdate)
        {
            // update audio streams
            mCurrentAudioStreams.clear();
            for (DashAdaptationSet* audio : aViewpoint->allAudioAdaptationSets)
            {
                mCurrentAudioStreams.add(audio->getCurrentAudioStreams());
            }
        }
    }
    if (aViewpoint->videoDownloader)
    {
        aViewpoint->videoDownloader->processSegmentDownload();
    }

    if (aViewpoint->invoMetadataAdaptationSet != OMAF_NULL)
    {
        if (aViewpoint->invoMetadataAdaptationSet->processSegmentDownload())
        {
            mCurrentMetadataStreams.clear();
            mCurrentMetadataStreams.add(aViewpoint->invoMetadataAdaptationSet->getCurrentMetadataStreams());
        }
    }
}

void_t DashDownloadManager::onNewStreamsCreated(MediaContent& aContent)
{
}

void_t DashDownloadManager::onDownloadProblem(IssueType::Enum aIssueType)
{
    // Bitrate controller is in video downloader, report also possible audio / metadata issues there
    mDownloadingViewpoint->videoDownloader->onDownloadProblem(aIssueType);
}

void_t DashDownloadManager::onActivateMe(DashAdaptationSet* aMe)
{
    OMAF_ASSERT(aMe->getType() == AdaptationSetType::AUDIO, "onActivateMe is only supported for audio");
    if (!mDownloadingViewpoint->activeAudioAdaptationSets.contains(aMe))
    {
        mDownloadingViewpoint->activeAudioAdaptationSets.add(aMe);
    }
    else if (mCommonToViewpoints && mCommonToViewpoints->activeAudioAdaptationSets.contains(aMe))
    {
        mCommonToViewpoints->activeAudioAdaptationSets.add(aMe);
    }
}

bool_t DashDownloadManager::isViewpointSwitchInitiated()
{
    if (!mViewportConsumed)
    {
        return false;
    }
    OMAF_LOG_V("mViewportConsumed is signaled");
    return true;
}

bool_t DashDownloadManager::isViewpointSwitchReadyToComplete(uint32_t aCurrentPlayTimeMs)
{
    // OMAF_LOG_V("isViewpointSwitchReadyToComplete with play position %d, target switch position %d",
    // aCurrentPlayTimeMs, mTargetSwitchTimeMs);
    bool_t switchedViewpoint = false;
    if (mDownloadingViewpoint != mCurrentViewpoint)
    {
        // check the state of current and next video downloader
        if (!mDownloadingViewpoint->videoDownloader->isBuffering())
        {
            OMAF_LOG_V("isViewpointSwitchReadyToComplete new viewpoint ready");
            if (aCurrentPlayTimeMs >= mTargetSwitchTimeMs)
            {
                OMAF_LOG_V("isViewpointSwitchReadyToComplete YES at %d", Time::getClockTimeMs());
                return true;
            }
        }
    }
    return false;
}

void_t DashDownloadManager::completeViewpointSwitch(bool_t& aBgAudioContinues)
{
    Spinlock::ScopeLock lock(mViewpointSwitchLock);
    releaseViewpoint(mCurrentViewpoint);
    mCurrentViewpoint = mDownloadingViewpoint;
    OMAF_LOG_V("Viewpoint switch completed at %d", Time::getClockTimeMs());
    if (mCommonToViewpoints && mCommonToViewpoints->backgroundAudioAdaptationSet)
    {
        aBgAudioContinues = true;
    }
    else
    {
        aBgAudioContinues = false;
    }
    updateBandwidth();
}

bool_t DashDownloadManager::getViewpointGlobalCoordSysRotation(ViewpointGlobalCoordinateSysRotation& aRotation) const
{
    if (mCurrentViewpoint && mCurrentViewpoint->videoDownloader)
    {
        return mCurrentViewpoint->videoDownloader->getViewpointGlobalCoordSysRotation(aRotation);
    }
    return false;
}

bool_t DashDownloadManager::getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const
{
    if (mCurrentViewpoint && mCurrentViewpoint->videoDownloader)
    {
        return mCurrentViewpoint->videoDownloader->getViewpointUserControls(aSwitchControls);
    }
    return false;
}


DashDownloadManager::ViewpointInitThread::ViewpointInitThread(DashDownloadManager& aHost)
    : mHost(aHost)
    , mTrigger(false, false)
    , mRunning(true)
{
    Thread::EntryFunction function;
    function.bind<DashDownloadManager::ViewpointInitThread,
                  &DashDownloadManager::ViewpointInitThread::viewpointThreadEntry>(this);

    mViewpointInitThread.setPriority(Thread::Priority::LOW);
    mViewpointInitThread.setName("DashDownloadManager::viewpointInitThread");
    mViewpointInitThread.start(function);
}

DashDownloadManager::ViewpointInitThread::~ViewpointInitThread()
{
    mRunning = false;
    mTrigger.signal();
    mViewpointInitThread.join();
}

Error::Enum DashDownloadManager::ViewpointInitThread::start(uint32_t aStartTimeMs)
{
    mHost.mViewportConsumed = false;
    mStartTimeMs = aStartTimeMs;
    mTrigger.signal();
    return Error::OK;
}

Thread::ReturnValue DashDownloadManager::ViewpointInitThread::viewpointThreadEntry(const Thread& thread,
                                                                                   void_t* userData)
{
    while (mRunning)
    {
        mTrigger.wait();
        if (!mRunning)
        {
            break;
        }
        mHost.initializeViewpoint(mHost.mViewpointPeriods.at(mHost.mCurrentVPPeriodIndex), true,
                                  SourceDirection::INVALID);
        uint32_t targetTimeMs = mStartTimeMs;
        mHost.startDownload(mHost.mInitializingViewpoint, targetTimeMs);
        if (mStartTimeMs == mHost.mTargetSwitchTimeMs)
        {
            // no timeline change, but adjusting to the next segment boundary
            mHost.mTargetSwitchTimeMs = targetTimeMs;
            OMAF_LOG_V("Adjusting switch time to %d", targetTimeMs);
        }
        else
        {
            // timeline change, hence no need to maintain continuous timeline and hence no switching time limits
            // => switch as soon as possible (set as low as possible)
            // we still need to play the old one until there is enough buffered data for the new viewpoint, but that is
            // handled via the isBuffering() checks that precede the timeline check
            mHost.mTargetSwitchTimeMs = 0;
        }
        mHost.mViewportConsumed = true;
        while (mHost.mInitializingViewpoint->videoDownloader->isBuffering())
        {
            mHost.processSegmentDownload(mHost.mInitializingViewpoint);
            Thread::sleep(5);
        }
        mHost.mViewpointSwitchLock.lock();
        OMAF_LOG_D("Switching to new downloading viewpoint");
        mHost.mDownloadingViewpoint = mHost.mInitializingViewpoint;
        mHost.mInitializingViewpoint = OMAF_NULL;
        mHost.mViewpointSwitchLock.unlock();
        mTrigger.reset();
    }

    return 0;
}

sourceid_t DashDownloadManager::getNextSourceIdBase()
{
    sourceid_t current = mNextSourceIdBase;
    // 1000 should be enough per viewpoint.
    // You need 1 per tile, and you can have e.g. 16 extractors, each having 16 tiles
    // == 256 sources.
    // ABR will not multiply that, since extractors are not expected to have multiple representations
    mNextSourceIdBase += 1000;
    return current;
}

void_t DashDownloadManager::updateBandwidth()
{
    // update bandwidth usage outside the main video downloader
    uint32_t bitrate = 0;
    for (DashAdaptationSet* audio : mCurrentViewpoint->activeAudioAdaptationSets)
    {
        bitrate += audio->getCurrentBandwidth();
    }
    if (mCommonToViewpoints && mCommonToViewpoints->videoDownloader)
    {
        bitrate += mCommonToViewpoints->videoDownloader->getCurrentBitrate();
    }
    mCurrentViewpoint->videoDownloader->setBandwidthOverhead(bitrate);
}

OMAF_NS_END
