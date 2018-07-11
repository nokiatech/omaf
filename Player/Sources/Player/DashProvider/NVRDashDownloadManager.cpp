
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
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "DashProvider/NVRDashAdaptationSetExtractorDepId.h"
#include "DashProvider/NVRDashAdaptationSetTile.h"
#include "DashProvider/NVRDashAdaptationSetSubPicture.h"
#include "Metadata/NVRMetadataParser.h"
#include "Media/NVRMediaPacket.h"
#include "DashProvider/NVRDashLog.h"

#include "Foundation/NVRClock.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRBandwidthMonitor.h"
#include "VAS/NVRVASTilePicker.h"
#include "VAS/NVRVASTileSetPicker.h"


//#define UNKNOWN_AS_IDENTITY 1
#include <libdash.h>

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashDownloadManager)

static const uint32_t MPD_UPDATE_INTERVAL_MS = 2000;
static const uint32_t MPD_DOWNLOAD_TIMEOUT_MS = 5000;

DashDownloadManager::DashDownloadManager()
    : mMPD(OMAF_NULL)
    , mDashManager(OMAF_NULL)
    , mVideoBaseAdaptationSet(OMAF_NULL)
    , mVideoBaseAdaptationSetStereo(OMAF_NULL)
    , mVideoEnhTiles()
    , mAudioAdaptationSet(OMAF_NULL)
    , mAudioMetadataAdaptationSet(OMAF_NULL)
    , mState(DashDownloadManagerState::IDLE)
    , mStreamType(DashStreamType::INVALID)
    , mMPDUpdateTimeMS(0)
    , mMPDUpdatePeriodMS(MPD_UPDATE_INTERVAL_MS)
    , mTilePicker(OMAF_NULL)
    , mCurrentVideoStreams()
    , mCurrentAudioStreams()
    , mMetadataLoaded(false)
    , mNewVideoStreamsCreated(false)
    , mVideoStreamsChanged(false)
    , mStreamUpdateNeeded(false)
    , mGlobalSubSegmentsSupported(false)
    , mPlaybackStartTimeMs(0)
    , mReselectSources(false)
    , mMPDUpdater(OMAF_NULL)
    , mABREnabled(true)
    , mBandwidthOverhead(0)
    , mBaseLayerDecoderPixelsinSec(0)
    , mTileSetupDone(false)
    , mVASType(VASType::NONE)
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
    mABREnabled = true;
}


void_t DashDownloadManager::disableABR()
{
    mABREnabled = false;
}

void_t DashDownloadManager::setBandwidthOverhead(uint32_t aBandwithOverhead)
{
    mBandwidthOverhead = aBandwithOverhead;
}

uint32_t DashDownloadManager::getCurrentBitrate()
{
    uint32_t bitrate = 0;
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        bitrate += mVideoBaseAdaptationSet->getCurrentBandwidth();
    }
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
    {
        bitrate += mVideoBaseAdaptationSetStereo->getCurrentBandwidth();
    }
    if (mAudioAdaptationSet != OMAF_NULL)
    {
        bitrate += mAudioAdaptationSet->getCurrentBandwidth();
    }
    if (!mVideoEnhTiles.isEmpty())
    {
        VASTileSelection sets = mTilePicker->getLatestTiles();
        for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
        {
            bitrate += (*it)->getAdaptationSet()->getCurrentBandwidth();
        }
    }
    // In OMAF case the bitrates come via base adaptation set(s)

    return bitrate;
}

MediaInformation DashDownloadManager::getMediaInformation()
{
    return mMediaInformation;
}

void_t DashDownloadManager::getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrEnhLayerCodecs)
{
    aNrBaseLayerCodecs = 0;
    if (mVideoBaseAdaptationSet)
    {
        aNrBaseLayerCodecs++;
    }
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
    {
        aNrBaseLayerCodecs++;
    }

    if (!mVideoEnhTiles.isEmpty())
    {
        // we don't know the viewport yet, so we use guestimates for it
        aNrEnhLayerCodecs = (uint32_t)mTilePicker->getNrVisibleTiles(mVideoEnhTiles, 100.f, 100.f);
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
#if UNKNOWN_AS_IDENTITY == 1
        data.sourceType = SourceType::IDENTITY;
#endif
    }
#if FORCE_IDENTITY==1
    data.sourceType = SourceType::IDENTITY;
#endif

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
    bool_t hasEnhancementLayer = false;
    bool_t framePackedEnhLayer = false;
    SupportingAdaptationSetIds supportingSets;
    RepresentationDependencies dependingRepresentations;
    // As a very first step, we scan through the adaptation sets to see what kind of adaptation sets we do have, 
    // e.g. if some of them are only supporting sets (tiles), as that info is needed when initializing the adaptation sets and representations under each adaptation set
    // Partial sets do not necessarily carry that info themselves, but we can detect it only from extractors, if present - and absense of extractor tells something too
    for (uint32_t index = 0; index < period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = period->GetAdaptationSets().at(index);
        dashComponents.adaptationSet = adaptationSet;
        uint32_t id = 0;
        if (DashAdaptationSetExtractor::isExtractor(dashComponents, id))
        {
            supportingSets = DashAdaptationSetExtractor::hasPreselection(dashComponents, id);
            if (!supportingSets.isEmpty())
            {
                // this adaptation set is a main adaptation set (extractor) and contains a list of supporting partial adaptation sets
                // collect the lists of supporting sets here, since the supporting set needs to know about its role when initialized, and this may be the only way to know it
                mVASType = VASType::EXTRACTOR_PRESELECTIONS_QUALITY;
            }
            else
            {
                dependingRepresentations = DashAdaptationSetExtractor::hasDependencies(dashComponents);
                if (!dependingRepresentations.isEmpty())
                {
                    mVASType = VASType::EXTRACTOR_DEPENDENCIES_QUALITY;
                }
            }
        }
    }

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    for (uint32_t index = 0; index < period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = period->GetAdaptationSets().at(index);
        dashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        Error::Enum result;
        if (!dependingRepresentations.isEmpty())
        {
            if (DashAdaptationSetTile::isTile(dashComponents, dependingRepresentations))
            {
                // a tile that an extractor depends on
                uint32_t adSetId = 0;
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetTile)(*this);
                result = ((DashAdaptationSetTile*)dashAdaptationSet)->initialize(dashComponents, initializationSegmentId, adSetId);
                if (adSetId > 0)
                {
                    // Also dependency-based linking is handled later on with supportingSets. This must be the first branch here.
                    supportingSets.add(adSetId);
                }
            }
            else
            {
                // extractor with dependencies
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetExtractorDepId)(*this);
                result = ((DashAdaptationSetExtractorDepId*)dashAdaptationSet)->initialize(dashComponents, initializationSegmentId);
            }
        }
        else if (!supportingSets.isEmpty())
        {
            if (DashAdaptationSetTile::isTile(dashComponents, supportingSets))
            {
                // a tile that an extractor depends on through Preselections-descriptor
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetTile)(*this);
                result = ((DashAdaptationSetTile*)dashAdaptationSet)->initialize(dashComponents, initializationSegmentId);
            }
            else
            {
                // extractor with Preselections-descriptor
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetExtractor)(*this);
                result = ((DashAdaptationSetExtractor*)dashAdaptationSet)->initialize(dashComponents, initializationSegmentId);
            }
        }
        else
        {
            if (DashAdaptationSetSubPicture::isSubPicture(dashComponents))
            {
                // a sub-picture adaptation set
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetSubPicture)(*this);
                result = ((DashAdaptationSetSubPicture*)dashAdaptationSet)->initialize(dashComponents, initializationSegmentId);
            }
            else
            {
                // "normal" adaptation set
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSet)(*this);
                result = dashAdaptationSet->initialize(dashComponents, initializationSegmentId);
            }
        }
        if (result == Error::OK)
        {
            mAdaptationSets.add(dashAdaptationSet);
            if (DeviceInfo::deviceSupportsLayeredVAS() != DeviceInfo::LayeredVASTypeSupport::NOT_SUPPORTED && dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::VIDEO_ENHANCEMENT))
            {
                if (dashAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
                {
                    framePackedEnhLayer = true;
                }
                hasEnhancementLayer = true;
                mVASType = VASType::SUBPICTURES;
            }
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
    mVideoBaseAdaptationSet = OMAF_NULL;
    mVideoBaseAdaptationSetStereo = OMAF_NULL;

    // Check if there is an extractor set with supporting partial sets, and if so, link them. 
    // This is not (full) dupe of what is done above with supportingSets, as here we have the adaptation set objects created and really link the objects, not just the ids
    for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
    {
        uint8_t nrQualityLevels = 0;
        DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
        if (dashAdaptationSet->getType() == AdaptationSetType::EXTRACTOR && dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HEVC))
        {
            mVideoBaseAdaptationSet = dashAdaptationSet;
            if (!dependingRepresentations.isEmpty())
            {
                // check if it has coverage definitions
                const FixedArray<VASTileViewport*, 32> viewports = ((DashAdaptationSetExtractorDepId*)dashAdaptationSet)->getCoveredViewports();
                if (!viewports.isEmpty())
                {
                    for (FixedArray<VASTileViewport*, 32>::ConstIterator it = viewports.begin(); it != viewports.end(); ++it)
                    {
                        mVideoPartialTiles.add((DashAdaptationSetExtractorDepId*)dashAdaptationSet, **it);
                    }
                }
            }
            sourceid_t sourceId = 0;
            dashAdaptationSet->createVideoSources(sourceId);
            for (uint32_t j = 0; j < mAdaptationSets.getSize(); j++)
            {
                DashAdaptationSet* otherAdaptationSet = mAdaptationSets.at(j);
                if (otherAdaptationSet != dashAdaptationSet && otherAdaptationSet->getType() == AdaptationSetType::TILE && supportingSets.contains(otherAdaptationSet->getId()))
                {
                    ((DashAdaptationSetExtractor*)dashAdaptationSet)->addSupportingSet((DashAdaptationSetTile*)otherAdaptationSet);
                }
            }
        }
        else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::VIDEO_TILE))
        {
            // a tile in OMAF VD scheme. In case this is not a depencencyId-based system (where extractor represents tiles), add to tile selection
            // but do not start/stop adaptation set from this level, but let the extractor handle it
            if (dashAdaptationSet->getCoveredViewport() != OMAF_NULL && dependingRepresentations.isEmpty())
            {
                mVideoPartialTiles.add((DashAdaptationSetTile*)dashAdaptationSet, *dashAdaptationSet->getCoveredViewport());
                uint8_t qualities = ((DashAdaptationSetTile*)dashAdaptationSet)->getNrQualityLevels();
                // find the max # of levels and store it to bitrate controller
                if (qualities > nrQualityLevels)
                {
                    nrQualityLevels = qualities;
                }
            }
        }
        if (nrQualityLevels > 0)
        {
            mBitrateController.setNrQualityLevels(nrQualityLevels);
        }
    }

    if (mVideoBaseAdaptationSet == OMAF_NULL)
    {
        sourceid_t sourceId = 0;
        for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
        {
            DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
            if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::IS_MUXED))
            {
                mVideoBaseAdaptationSet = dashAdaptationSet;    // no enh layer supported in muxed case, also baselayer must be in the same adaptation set
                mAudioAdaptationSet = dashAdaptationSet;
                if (dashAdaptationSet->hasMPDVideoMetadata())
                {
                    dashAdaptationSet->createVideoSources(sourceId);
                }
                break;
            }
            else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_VIDEO))
            {
                if (dashAdaptationSet->isBaselayer())
                {
                    if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::FULL_STEREO)
                    {
                        // high end device: top-bottom or mono stream, 2-track stereo stream, or VAS (base+enh layer) stream
                        if (dashAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED || dashAdaptationSet->getVideoChannel() == StereoRole::MONO || dashAdaptationSet->getVideoChannel() == StereoRole::RIGHT)
                        {
                            // framepacked, right, or mono
                            if (mVideoBaseAdaptationSet == OMAF_NULL)
                            {
                                // not yet assigned a base adaptation set
                                mVideoBaseAdaptationSet = dashAdaptationSet;
                            }
                            else if (hasEnhancementLayer)
                            {
                                if ((mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() > dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight()
                                    && mVideoBaseAdaptationSet->getVideoChannel() == dashAdaptationSet->getVideoChannel())
                                    || (mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::MONO && dashAdaptationSet->getVideoChannel() != StereoRole::MONO))
                                {
                                    // select the smallest res full 360 baselayer
                                    mVideoBaseAdaptationSet = dashAdaptationSet;
                                }
                            }
                            else if ((mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() < dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())
                                || ((mVideoBaseAdaptationSet->getCurrentWidth() == dashAdaptationSet->getCurrentWidth() && mVideoBaseAdaptationSet->getCurrentHeight() == dashAdaptationSet->getCurrentHeight())
                                    && mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::MONO && dashAdaptationSet->getVideoChannel() != StereoRole::MONO))
                            {
                                // no enh layer; select the highest res full 360 stream or a stereo rather than mono in case resolutions are equal
                                mVideoBaseAdaptationSet = dashAdaptationSet;
                            }
                        }
                        else
                        {
                            // left
                            if (mVideoBaseAdaptationSetStereo == OMAF_NULL)
                            {
                                // not yet assigned a base right adaptation set
                                mVideoBaseAdaptationSetStereo = dashAdaptationSet;
                            }
                            else if (hasEnhancementLayer)
                            {
                                if (mVideoBaseAdaptationSetStereo->getCurrentWidth() * mVideoBaseAdaptationSetStereo->getCurrentHeight() > dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())
                                {
                                    // select the smallest res full 360 baselayer
                                    mVideoBaseAdaptationSetStereo = dashAdaptationSet;
                                }
                            }
                            else if (mVideoBaseAdaptationSetStereo->getCurrentWidth() * mVideoBaseAdaptationSetStereo->getCurrentHeight() < dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())
                            {
                                // no enh layer; select the highest res full 360 baselayer
                                mVideoBaseAdaptationSetStereo = dashAdaptationSet;
                            }
                        }
                    }
                    else if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::LIMITED && hasEnhancementLayer)
                    {
                        // mono/framepacked VAS, select the smallest resolution base layer
                        if (dashAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
                        {
                            if (mVideoBaseAdaptationSet == OMAF_NULL ||
                                (mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() > dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight()))
                            {
                                mVideoBaseAdaptationSet = dashAdaptationSet;
                            }
                            // else framepacked base layer is useless for limited VAS case, if enh layer is not framepacked, ignore it
                        }
                        else if (dashAdaptationSet->getVideoChannel() != StereoRole::LEFT &&
                            (mVideoBaseAdaptationSet == OMAF_NULL ||
                            (mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() > dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())))
                        {
                            mVideoBaseAdaptationSet = dashAdaptationSet;
                            // force the adaptationset to mono (if it is mono/framepacked, no impact)
                            mVideoBaseAdaptationSet->forceVideoTo(StereoRole::MONO);
                        }
                    }
                    else
                    {
                        // low end single track device: find largest resolution top-bottom, or use mono. Assuming a single track device don't support enh layer
                        if (dashAdaptationSet->getVideoChannel() != StereoRole::LEFT &&
                            (mVideoBaseAdaptationSet == OMAF_NULL ||
                            (mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() < dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())))
                        {
                            mVideoBaseAdaptationSet = dashAdaptationSet;
                            // force the adaptationset to mono (if it is mono/framepacked, no impact)
                            mVideoBaseAdaptationSet->forceVideoTo(StereoRole::MONO);
                        }
                    }
                }
                else if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::FULL_STEREO)
                {
                    // not base layer, so must be VAS enhancement tile
                    mVideoEnhTiles.add((DashAdaptationSetSubPicture*)dashAdaptationSet, *dashAdaptationSet->getCoveredViewport());
                }
                else if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::LIMITED)
                {
                    // not base layer, so must be VAS enhancement tile
                    // take only right, mono, or framepacked, ignore left eye
                    if (dashAdaptationSet->getVideoChannel() != StereoRole::LEFT)
                    {
                        // force the adaptationset to mono (if it is mono/framepacked, no impact)
                        dashAdaptationSet->forceVideoTo(StereoRole::MONO);
                        mVideoEnhTiles.add((DashAdaptationSetSubPicture*)dashAdaptationSet, *dashAdaptationSet->getCoveredViewport());
                    }
                }
                if (dashAdaptationSet->hasMPDVideoMetadata())
                {
                    dashAdaptationSet->createVideoSources(sourceId);
                    if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::LIMITED && dashAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED && !framePackedEnhLayer)
                    {
                        // special case: we have a frame-packed base layer, but tiles are left-right and we must use them as mono only => force base layer to mono too (download & decode full, but use only half of it)
                        // we do it after creating sources to avoid special flagging in source creation, which is already quite complicated
                        dashAdaptationSet->forceVideoTo(StereoRole::MONO);
                    }
                }
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
                            if (mAdaptationSets.at(j) == mAudioAdaptationSet)
                            {
                                mAudioMetadataAdaptationSet = dashAdaptationSet;
                            }
                        }
                    }
                }
            }
        }
    }

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mMediaInformation.duration = mVideoBaseAdaptationSet->durationMs();
        mMediaInformation.width = mVideoBaseAdaptationSet->getCurrentWidth();
        mMediaInformation.height = mVideoBaseAdaptationSet->getCurrentHeight();
        const MediaContent& mediaContent = mVideoBaseAdaptationSet->getAdaptationSetContent();
        if (mediaContent.matches(MediaContent::Type::AVC))
        {
            mMediaInformation.baseLayerCodec = VideoCodec::AVC;
        }
        else if (mediaContent.matches(MediaContent::Type::HEVC))
        {
            mMediaInformation.baseLayerCodec = VideoCodec::HEVC;
        }
        else
        {
            // default to AVC?
            mMediaInformation.baseLayerCodec = VideoCodec::AVC;
        }
    }
    else
    {
        OMAF_LOG_E("Stream needs to have video");
        mState = DashDownloadManagerState::STREAM_ERROR;
        return Error::NOT_SUPPORTED;
    }

    switch (mVASType)
    {
    case VASType::EXTRACTOR_DEPENDENCIES_QUALITY:
    {
        mTilePicker = OMAF_NEW_HEAP(VASTileSetPicker);
    }
    break;
    case VASType::SUBPICTURES:
    case VASType::EXTRACTOR_PRESELECTIONS_QUALITY:
    {
        mTilePicker = OMAF_NEW_HEAP(VASTilePicker);
    }
    break;
    default:
        // no tile picker
        break;
    }

    if (mAudioAdaptationSet != OMAF_NULL)
    {
        const MediaContent& mediaContent = mAudioAdaptationSet->getAdaptationSetContent();
        if (mediaContent.matches(MediaContent::Type::AUDIO_LOUDSPEAKER))
        {
            mMediaInformation.audioFormat = AudioFormat::LOUDSPEAKER;
        }
    }

    if (!mVideoPartialTiles.isEmpty())
    {
        mGlobalSubSegmentsSupported = false; // we would need to synchronize all tiles on subsegment level
        if (mVASType == VASType::EXTRACTOR_PRESELECTIONS_QUALITY)
        {
            mVideoPartialTiles.allSetsAdded(true);
        }
        else
        {
            // server has defined the tilesets per viewing orientation => 1 tileset at a time => overlap is often intentional and makes no harm
            mVideoPartialTiles.allSetsAdded(false);
        }
        // we start with all tiles in high quality => dependency to adaptation set - assuming it picks the highest quality at first
        mTilePicker->setAllSelected(mVideoPartialTiles);
        mMediaInformation.videoType = VideoType::VIEW_ADAPTIVE;
    }
    else if (!mVideoEnhTiles.isEmpty())
    {
        mGlobalSubSegmentsSupported = DeviceInfo::deviceSupportsSubsegments();
        mVideoEnhTiles.allSetsAdded(true);
        mMediaInformation.videoType = VideoType::VIEW_ADAPTIVE;

        const MediaContent& mediaContent = mVideoEnhTiles.getAdaptationSetAt(0)->getAdaptationSetContent();
        if (mediaContent.matches(MediaContent::Type::AVC))
        {
            mMediaInformation.enchancementLayerCodec = VideoCodec::AVC;
        }
        else if (mediaContent.matches(MediaContent::Type::HEVC))
        {
            mMediaInformation.enchancementLayerCodec = VideoCodec::HEVC;
        }
    }

    if (mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED || mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::MONO)
    {
        // we must not use right track if left is framepacked or mono
        mVideoBaseAdaptationSetStereo = OMAF_NULL;
    }

    if (sourceInfo.sourceDirection != SourceDirection::Enum::MONO || mVideoBaseAdaptationSetStereo != OMAF_NULL || mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
    {
        // sourceDirection is from URI indicating stereo, mVideoBaseAdaptationSetStereo comes from MPD metadata, or framepacked video type from MPD metadata
        OMAF_LOG_D("Stereo stream");
        mMediaInformation.isStereoscopic = true;
    }
    else
    {
        OMAF_LOG_D("Mono stream");
        mMediaInformation.isStereoscopic = false;
    }

    mMPDUpdatePeriodMS = DashUtils::getMPDUpdateInMilliseconds(dashComponents);
    if (mMPDUpdatePeriodMS == 0)
    {
        // the update period is optional parameter; use default value
        mMPDUpdatePeriodMS = MPD_UPDATE_INTERVAL_MS;
    }

    mPublishTime = DashUtils::getRawAttribute(mMPD, "publishTime");
    mMPDUpdateTimeMS = Time::getClockTimeMs();
    mState = DashDownloadManagerState::INITIALIZED;
    mBitrateController.initialize(
        mVideoBaseAdaptationSet,
        mVideoBaseAdaptationSetStereo,
        mAudioAdaptationSet,
        mVideoEnhTiles.getAdaptationSets(),
        mTilePicker);

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

    for (AdaptationSets::Iterator it = mAdaptationSets.begin(); it != mAdaptationSets.end(); ++it)
    {
        OMAF_DELETE_HEAP(*it);
    }
    mAdaptationSets.clear();
    //Make sure these are NULL
    mVideoBaseAdaptationSet = OMAF_NULL;
    mVideoBaseAdaptationSetStereo = OMAF_NULL;
    OMAF_DELETE_HEAP(mTilePicker);
    mVideoEnhTiles.clear();

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

    if (mVideoBaseAdaptationSet != OMAF_NULL)
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
        mVideoBaseAdaptationSet->startDownload(startTime);
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
        {
            mVideoBaseAdaptationSetStereo->startDownload(startTime);
        }
        if (!isVideoAndAudioMuxed() && mAudioAdaptationSet != OMAF_NULL)
        {
            mAudioAdaptationSet->startDownload(startTime);
            if (mAudioMetadataAdaptationSet != OMAF_NULL)
            {
                mAudioMetadataAdaptationSet->startDownload(startTime);
            }
        }

        // this can be effective only after pause + resume. With cold start, allowEnhancement is false
        if (mBitrateController.allowEnhancement() && !mVideoEnhTiles.isEmpty())
        {
            VASTileSelection& sets = mTilePicker->getLatestTiles();
            for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
            {
                OMAF_LOG_D("Start downloading for longitude %f", (*it)->getCoveredViewport().getCenterLongitude());
                (*it)->getAdaptationSet()->startDownload(startTime);
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
    checkVASVideoStreams(currentPlayTimeUs);
    processSegmentDownload();
    Error::Enum result = Error::OK;
    int32_t now = Time::getClockTimeMs();
    if (mState == DashDownloadManagerState::DOWNLOADING && mStreamType == DashStreamType::DYNAMIC)
    {
        bool_t updateRequired = mVideoBaseAdaptationSet->mpdUpdateRequired();
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

    if (mVideoBaseAdaptationSet->isEndOfStream())
    {
        mState = DashDownloadManagerState::END_OF_STREAM;
    }
    if (mVideoBaseAdaptationSet->isError() ||
        (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isError()) ||
        (mAudioAdaptationSet != OMAF_NULL && mAudioAdaptationSet->isError()) ||
        (mAudioMetadataAdaptationSet != OMAF_NULL && mAudioMetadataAdaptationSet->isError()) )
    {
        mState = DashDownloadManagerState::STREAM_ERROR;
    }
    
    if (!mCurrentVideoStreams.isEmpty() && mABREnabled)
    {
        if (mBitrateController.update(mBandwidthOverhead))
        {
            mStreamUpdateNeeded = true;
        }
    }
}

Error::Enum DashDownloadManager::stopDownload()
{
    if (mState != DashDownloadManagerState::DOWNLOADING
        && mState != DashDownloadManagerState::STREAM_ERROR
        && mState != DashDownloadManagerState::CONNECTION_ERROR)
    {
        return Error::INVALID_STATE;
    }

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mVideoBaseAdaptationSet->stopDownload();
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
        {
            mVideoBaseAdaptationSetStereo->stopDownload();
        }

        for (size_t i = 0; i < mVideoEnhTiles.getNrAdaptationSets(); i++)
        {
            mVideoEnhTiles.getAdaptationSetAt(i)->stopDownload();
        }

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
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mVideoBaseAdaptationSet->clearDownloadedContent();
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
        {
            mVideoBaseAdaptationSetStereo->clearDownloadedContent();
        }

        for (size_t i = 0; i < mVideoEnhTiles.getNrAdaptationSets(); i++)
        {
            mVideoEnhTiles.getAdaptationSetAt(i)->clearDownloadedContent();
        }
        if (mTilePicker != OMAF_NULL)
        {
            mTilePicker->reset();
        }

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
    return mVideoBaseAdaptationSet == mAudioAdaptationSet;
}

Error::Enum DashDownloadManager::seekToMs(uint64_t& aSeekMs)
{
    uint64_t seekToTarget = aSeekMs;
    uint64_t seekToResultMs = 0;
    Error::Enum result = Error::OK;

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        result = mVideoBaseAdaptationSet->seekToMs(seekToTarget, seekToResultMs);
        if (result == Error::OK)
        {
            aSeekMs = seekToResultMs;
            if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
            {
                mVideoBaseAdaptationSetStereo->seekToMs(seekToTarget, seekToResultMs);
            }

            for (size_t i = 0; i < mVideoEnhTiles.getNrAdaptationSets(); i++)
            {
                mVideoEnhTiles.getAdaptationSetAt(i)->seekToMs(seekToTarget, seekToResultMs);
            }
        }
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
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        // we assume enhancement layer follows the same seeking properties as the base layer
        return mVideoBaseAdaptationSet->isSeekable();
    }
    else
    {
        return false;
    }
}

uint64_t DashDownloadManager::durationMs() const
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        // we assume enhancement layer has the same duration as the base layer
        return mVideoBaseAdaptationSet->durationMs();
    }
    else
    {
        return 0;
    }
}

bool_t DashDownloadManager::updateVideoStreams()
{
    bool_t refreshStillRequired = false;
    OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No current video adaptation set");
    mCurrentVideoStreams.clear();
    mCurrentVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
    size_t count = 1;
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
    {
        mCurrentVideoStreams.add(mVideoBaseAdaptationSetStereo->getCurrentVideoStreams());
        count++;
    }
    if (mCurrentVideoStreams.getSize() < count)
    {
        refreshStillRequired = true;
    }
    if (!mVideoEnhTiles.isEmpty() && mBitrateController.allowEnhancement())
    {
        mCurrentVideoStreams.clear();
        mCurrentVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
        {
            mCurrentVideoStreams.add(mVideoBaseAdaptationSetStereo->getCurrentVideoStreams());
        }

        VASTileSelection sets = mTilePicker->getLatestTiles();
        for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
        {
            if ((*it)->getVideoStreams().getSize() == 0)
            {
                refreshStillRequired = true;
            }
            mCurrentVideoStreams.add((*it)->getVideoStreams());
        }
    }
    return refreshStillRequired;
}

const MP4VideoStreams& DashDownloadManager::getVideoStreams()
{
    if (mCurrentVideoStreams.isEmpty() || mNewVideoStreamsCreated.compareAndSet(false, true))
    {
        updateVideoStreams();
    }
    return mCurrentVideoStreams;
}

void_t DashDownloadManager::checkVASVideoStreams(uint64_t currentPTS)
{
    if (!mVideoPartialTiles.isEmpty())
    {
        // OMAF
        bool_t selectionUpdated = false;
        VASTileSelection droppedTiles;
        VASTileSelection additionalTiles;
        VASTileSelection tiles = mTilePicker->getLatestTiles(selectionUpdated, droppedTiles, additionalTiles);

        if (selectionUpdated)
        {
            uint32_t segmentIndex = mVideoBaseAdaptationSet->peekNextSegmentId();   // this does not and should not consider any download latencies, since we may have the right segment already cached from previous tile switches
            if (mVASType == VASType::EXTRACTOR_PRESELECTIONS_QUALITY)
            {
                // single resolution - multiple qualities, supporting sets identified with Preselections, so each of them contains coverage info

                // first switch dropped tiles to lower quality
                uint8_t nrLevels = 0;
                uint8_t level = 0;
                if (mBitrateController.getQualityLevelBackground(level, nrLevels))
                {
                    for (VASTileSelection::Iterator it = droppedTiles.begin(); it != droppedTiles.end(); ++it)
                    {
                        ((DashAdaptationSetTile*)(*it)->getAdaptationSet())->selectQuality(level, nrLevels, segmentIndex);
                    }
                }
                // then switch the selected new tiles to higher quality
                if (mBitrateController.getQualityLevelForeground(level, nrLevels))
                {
                    for (VASTileSelection::Iterator it = additionalTiles.begin(); it != additionalTiles.end(); ++it)
                    {
                        ((DashAdaptationSetTile*)(*it)->getAdaptationSet())->selectQuality(level, nrLevels, segmentIndex);
                    }
                }
            }
            else if (mVASType == VASType::EXTRACTOR_DEPENDENCIES_QUALITY)
            {
                if (!additionalTiles.isEmpty())
                {
                    OMAF_LOG_V("Tile selection updated, select representations accordingly");
                    // dependencyId based extractor, so extractor contains the coverage info
                    ((DashAdaptationSetExtractorDepId*)mVideoBaseAdaptationSet)->selectRepresentation(&additionalTiles.front()->getCoveredViewport(), segmentIndex);
                }
            }
        }
        return;
    }
    else if (mVideoEnhTiles.isEmpty())
    {
        if (mStreamUpdateNeeded)
        {
            mStreamUpdateNeeded = updateVideoStreams();
            mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
        }
        return;
    }

    bool_t selectionUpdated = false;
    VASTileSelection droppedTiles;
    VASTileSelection additionalTiles;
    // read the latest selection, selection is done in renderer thread
    VASTileSelection tiles = mTilePicker->getLatestTiles(selectionUpdated, droppedTiles, additionalTiles);

    // First stop the download on any dropped tiles
    if (!droppedTiles.isEmpty())
    {
        // stop downloading dropped ones
        for (VASTileSelection::Iterator it = droppedTiles.begin(); it != droppedTiles.end(); ++it)
        {
            OMAF_LOG_D("stop downloading representation %s with center at %f longitude", (*it)->getAdaptationSet()->getRepresentationId(), (*it)->getCoveredViewport().getCenterLongitude());
            (*it)->getAdaptationSet()->stopDownloadAsync(true);
        }
        mStreamUpdateNeeded = true;
    }

    if (mBitrateController.allowEnhancement())
    {
        // We have bandwidth to download the enhancement layers. Note! in OMAF case we return already above

        // If it's not yet being downloaded, start it
        uint32_t baseSegmentIndex = 0;
        time_t startTime = 0;
        uint32_t maxSegmentId = 0;
        uint64_t targetPtsUs = 0;
        bool_t initializeValues = true;
        for (VASTileSelection::Iterator it = tiles.begin(); it != tiles.end(); ++it)
        {
            if (!(*it)->getAdaptationSet()->isActive())
            {
                if (initializeValues)
                {
                    initializeValues = false;
                    targetPtsUs = mVideoBaseAdaptationSet->getReadPositionUs(baseSegmentIndex);
                    if (targetPtsUs == 0)
                    {
                        targetPtsUs = currentPTS;
                    }
                    uint64_t baseSegmentDurationMs = 0;
                    uint32_t baseAlignment = 0;

                    uint64_t tileSegmentDurationMs = 0;
                    uint32_t tileAlignment = 0;
                    if (baseSegmentIndex == INVALID_SEGMENT_INDEX)
                    {
                        // base layer is not a suitable reference yet, let the segmentstreamer pick the first one suitable for the current playback time
                        startTime = time(0);
#if OMAF_PLATFORM_ANDROID
                        OMAF_LOG_D("Start time for tiles: %d", startTime);
#else
                        OMAF_LOG_D("Start time for tiles: %lld", startTime);
#endif
                    }
                    else if (mVideoEnhTiles.getAdaptationSetAt(0)->getAlignmentId(tileSegmentDurationMs, tileAlignment) && mVideoBaseAdaptationSet->getAlignmentId(baseSegmentDurationMs, baseAlignment) && baseAlignment == tileAlignment && baseSegmentDurationMs == tileSegmentDurationMs)
                    {
                        // base layer and enh layers seem to be aligned; use the base layer index as it should be always showing the correct read position
                        maxSegmentId = baseSegmentIndex;
                    }
                    else if (tileSegmentDurationMs > 0)
                    {
                        // Do some math
                        maxSegmentId = (uint32_t)((targetPtsUs / (tileSegmentDurationMs * 1000)) + 1);
                    }
                    else
                    {
                        // must be timeline mode; segments don't have fixed duration, so leave maxSegmentId = 0 and use the timestamp
                    }
                }

                if (baseSegmentIndex == INVALID_SEGMENT_INDEX)
                {
                    // let the segmentstreamer pick the first one suitable for the current playback time
                    (*it)->getAdaptationSet()->startDownload(startTime);
                }
                else
                {
                    // Not currently downloading, start a download
                    OMAF_LOG_V("start downloading new adaptation set %s at %f longitude",
                        (*it)->getAdaptationSet()->getRepresentationId(),
                        (*it)->getCoveredViewport().getCenterLongitude());
                    if (mGlobalSubSegmentsSupported && (*it)->getAdaptationSet()->supportsSubSegments())
                    {
                        OMAF_LOG_V("Segment ID from baselayer: %d, selected segment id for tiles: %d, target pts us %lld start by checking subsegments", baseSegmentIndex, maxSegmentId, targetPtsUs);
                        Error::Enum error = (*it)->getAdaptationSet()->hasSubSegments(targetPtsUs, maxSegmentId);
                        if (error == Error::OK)
                        {
                            (*it)->getAdaptationSet()->startSubSegmentDownload(targetPtsUs, maxSegmentId);
                            mStreamUpdateNeeded = true;
                            continue;
                        }
                        (*it)->getAdaptationSet()->startDownload(targetPtsUs, maxSegmentId);
                    }
                    else
                    {
                        OMAF_LOG_V("Segment ID from baselayer: %d, selected segment id for tiles: %d, target pts us %lld", baseSegmentIndex, maxSegmentId, targetPtsUs);
                        (*it)->getAdaptationSet()->startDownload(targetPtsUs, maxSegmentId);
                    }
                }
                mStreamUpdateNeeded = true;
            }
        }
    }
    else
    {
        // OMAF_LOG_D("No bandwidth for tiles so stopping them!");
        // No bandwidth for tiles so stop them all
        for (VASTileSelection::Iterator it = tiles.begin(); it != tiles.end(); ++it)
        {
            if ((*it)->getAdaptationSet()->isActive())
            {
                OMAF_LOG_V("Stopped downloading a tile because of no bandwidth");
                (*it)->getAdaptationSet()->stopDownloadAsync(true);
                mStreamUpdateNeeded = true;
            }
        }
    }

    if (mStreamUpdateNeeded)
    {
        mStreamUpdateNeeded = updateVideoStreams();
        mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
    }
    if (mCurrentVideoStreams.getSize() > 2)
    {
        // shuffle the stream order to balance the load; keep the base layer(s) as first
        // index must be volatile, otherwise compiler in Android release build makes removeAt as an infinite loop
        volatile size_t index = 1;
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
        {
            index = 2;
        }
        MP4VideoStream* tmp = mCurrentVideoStreams.at(index);
        mCurrentVideoStreams.removeAt(index, 1);
        mCurrentVideoStreams.add(tmp);
        //OMAF_LOG_D("First tile stream %d out of %zd", mCurrentVideoStreams[index]->getStreamId(), mCurrentVideoStreams.getSize())
    }
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
    Error::Enum result = Error::OK;
    result = mVideoBaseAdaptationSet->readNextVideoFrame(currentTimeUs);

    // base 1 may return EOS, when switching ABR, but that must not prevent base2 to read video as it would trigger EOS on base 2 too
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
    {
        Error::Enum result2 = Error::OK;
        result2 = mVideoBaseAdaptationSetStereo->readNextVideoFrame(currentTimeUs);
        if (result2 != Error::OK && result2 != Error::END_OF_FILE)
        {
            // other error than end of file
            return result2;
        }
    }
    if (result != Error::OK && result != Error::END_OF_FILE)
    {
        // other error than end of file
        return result;
    }

    if (!mVideoEnhTiles.isEmpty() && mBitrateController.allowEnhancement())
    {
        // use here the picker variant that minimizes changes to current set
        VASTileSelection& sets = mTilePicker->getLatestTiles();
        for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
        {
            // read enh tiles if there is data available. 
            // Ignore the result, as tiles are not mandatory for player operation, and connection, parsing etc problems are detected elsewhere.
            // If there is no data, it becomes visible in getNextVideoFrame as an empty packet

            (*it)->getAdaptationSet()->readNextVideoFrame(currentTimeUs);
        }
    }

    return result;
}

Error::Enum DashDownloadManager::readAudioFrames()
{
    OMAF_ASSERT(mAudioAdaptationSet != OMAF_NULL, "No current audio adaptation set");
    return mAudioAdaptationSet->readNextAudioFrame();
}

MP4VRMediaPacket* DashDownloadManager::getNextVideoFrame(MP4MediaStream &stream, int64_t currentTimeUs)
{
    // With Dash we don't need to do anything special here
    return stream.peekNextFilledPacket();
}

MP4VRMediaPacket* DashDownloadManager::getNextAudioFrame(MP4MediaStream &stream)
{
    // With Dash we don't need to do anything special here
    return stream.peekNextFilledPacket();
}

const CoreProviderSourceTypes& DashDownloadManager::getVideoSourceTypes()
{
    OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No current video adaptation set");

    // we have MPD metadata; typically base + enh layers.
    if (mVideoSourceTypes.isEmpty())
    {
        // just 1 entry per type needed, so no need to check from the stereo or from many tiles
        mVideoSourceTypes.add(mVideoBaseAdaptationSet->getVideoSourceTypes());  // just 1 entry per type needed, so no need to check from the stereo

        if (!mVideoEnhTiles.isEmpty())
        {
            // just 1 entry per type needed
            mVideoSourceTypes.add(mVideoEnhTiles.getAdaptationSetAt(0)->getVideoSourceTypes());
        }

    }
    return mVideoSourceTypes;
}

bool_t DashDownloadManager::isDynamicStream()
{
    return mStreamType == DashStreamType::DYNAMIC;
}

bool_t DashDownloadManager::isBuffering()
{
    bool_t videoBuffering = mVideoBaseAdaptationSet->isBuffering();
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
    {
        videoBuffering |= mVideoBaseAdaptationSetStereo->isBuffering();
    }
    // Tiles are not checked for buffering, as it is concluded a better UX to show base layer if no tiles are available, than pause video
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
    return (mState == DashDownloadManagerState::END_OF_STREAM);
}

bool_t DashDownloadManager::isReadyToSwitch(MP4MediaStream& aStream) const
{
    // this is relevant for video base layer only
    if (mVideoBaseAdaptationSet->isReadyToSwitch(aStream))
    {
        return true;
    }
    else if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
    {
        return mVideoBaseAdaptationSetStereo->isReadyToSwitch(aStream);
    }
    return false;
}

const MP4VideoStreams& DashDownloadManager::selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId)
{
    if (!mTileSetupDone)
    {
        if (!mVideoEnhTiles.isEmpty())
        {
            mBaseLayerDecoderPixelsinSec = (uint32_t)(mVideoBaseAdaptationSet->getMinResXFps());
            if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
            {
                mBaseLayerDecoderPixelsinSec += (uint32_t)(mVideoBaseAdaptationSet->getMinResXFps());
            }

            // need to initialize the picker from renderer thread too (now we know the viewport width & height)
            mTilePicker->setupTileRendering(mVideoEnhTiles, width, height, mBaseLayerDecoderPixelsinSec);
            mTileSetupDone = true;
        }
        else if (!mVideoPartialTiles.isEmpty())
        {
            // need to initialize the picker from renderer thread too (now we know the viewport width & height)
            mTilePicker->setupTileRendering(mVideoPartialTiles, width, height, mBaseLayerDecoderPixelsinSec);
            mTileSetupDone = true;
        }
    }

    if (!mVideoEnhTiles.isEmpty() && mBitrateController.allowEnhancement())
    {
        if (mTilePicker->setRenderedViewPort(longitude, latitude, roll, width, height) || mRenderingVideoStreams.isEmpty() || mReselectSources || mVideoStreamsChanged.compareAndSet(false, true))
        {
            mReselectSources = false;
            mVideoStreamsChanged = false;   // reset in case some other condition triggered too

            // collect streams that match with the selected sources
            mRenderingVideoStreams.clear();

            size_t count = 1;
            mRenderingVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
            if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
            {
                count = 2;
                mRenderingVideoStreams.add(mVideoBaseAdaptationSetStereo->getCurrentVideoStreams());
            }
            if (mRenderingVideoStreams.getSize() < count)
            {
                // a base stream is not available yet => trigger to recheck next time
                //OMAF_LOG_V("Missing base layer streams for rendering: expected %zd now %zd", count, mRenderingVideoStreams.getSize());
                mReselectSources = true;
            }
            // do the tile picking 
            VASTileSelection& tiles = mTilePicker->pickTiles(mVideoEnhTiles, mBaseLayerDecoderPixelsinSec);
            for (VASTileSelection::Iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                count = mRenderingVideoStreams.getSize();
                mRenderingVideoStreams.add((*it)->getVideoStreams());
                if (count == mRenderingVideoStreams.getSize())
                {
                    // a stream is not available yet => trigger to recheck next time
                    OMAF_LOG_V("Stream for a tile not yet available");
                    mReselectSources = true;
                }
            }

            //OMAF_LOG_D("Updated rendering streams, count %zd", mRenderingVideoStreams.getSize());
        }
        // else no change, use previous streams
    }
    else
    {
        if (!mVideoPartialTiles.isEmpty())
        {
            // OMAF VD case
            if (mTilePicker->setRenderedViewPort(longitude, latitude, roll, width, height))
            {
                VASTileSelection& tiles = mTilePicker->pickTiles(mVideoPartialTiles, mBaseLayerDecoderPixelsinSec);

            }
        }
        // Non-enh layer case
        if (mRenderingVideoStreams.isEmpty() || mReselectSources || mVideoStreamsChanged.compareAndSet(false, true))
        {
            mRenderingVideoStreams.clear();
            mReselectSources = false;
            mVideoStreamsChanged = false;   // reset in case some other condition triggered too
            size_t count = 1;
            if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
            {
                // we need two streams; if we don't have both of them yet, this triggers to re-check next time
                count++;
            }
            mRenderingVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
            if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
            {
                mRenderingVideoStreams.add(mVideoBaseAdaptationSetStereo->getCurrentVideoStreams());
            }
            if (mRenderingVideoStreams.getSize() < count)
            {
                //OMAF_LOG_V("Missing base layer streams for rendering: expected %zd now %zd", count, mRenderingVideoStreams.getSize());
                mReselectSources = true;
            }
        }
    }

    if (!mRenderingVideoStreams.isEmpty())
    {
        aBaseLayerStreamId = mRenderingVideoStreams.at(0)->getStreamId();
    }
    return mRenderingVideoStreams;
}

bool_t DashDownloadManager::hasMPDVideoMetadata() const
{
    return mVideoBaseAdaptationSet->hasMPDVideoMetadata();
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
        OMAF_LOG_V("[%p] MPD downloading took %lld", this, ps - mStart);
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
    bool_t videoStreamsChanged = false;
    videoStreamsChanged = mVideoBaseAdaptationSet->processSegmentDownload();
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
    {
        videoStreamsChanged |= mVideoBaseAdaptationSetStereo->processSegmentDownload();
    }
    uint32_t segmentId = 0;
    if (!mVideoEnhTiles.isEmpty() && mBitrateController.allowEnhancement())
    {
        VASTileSelection& sets = mTilePicker->getLatestTiles();
        for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
        {
            videoStreamsChanged |= (*it)->getAdaptationSet()->processSegmentDownload();
        }

        VASTileSelection &allTiles = mTilePicker->getAll(mVideoEnhTiles);
        uint32_t lastSegmentId = 0;
        for (VASTileSelection::Iterator it = allTiles.begin(); it != allTiles.end(); ++it)
        {
            lastSegmentId = (*it)->getAdaptationSet()->getLastSegmentId();
            if (lastSegmentId > segmentId) // TODO: JPH: what about after seek... will some of the tiles have permanently high segment id?
            {
                segmentId = lastSegmentId;
            }
        }
    }

    if (videoStreamsChanged)
    {
        // Some ABR stream switches done, update video streams
        updateVideoStreams();
        mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
    }

    if (mGlobalSubSegmentsSupported)
    {
        VASTileSelection &nonActiveSets = mTilePicker->getCurrentNonSelected(mVideoEnhTiles);
        for (VASTileSelection::Iterator it = nonActiveSets.begin();
                it != nonActiveSets.end(); ++it)
        {
            if (segmentId > 0 && (*it)->getAdaptationSet()->supportsSubSegments())
            {
                (*it)->getAdaptationSet()->processSegmentIndexDownload(segmentId);
            }
        }
    }
}

void_t DashDownloadManager::onNewStreamsCreated()
{
    // need to update mCurrentVideoStreams
    mNewVideoStreamsCreated = true;
}

void_t DashDownloadManager::onDownloadProblem(IssueType::Enum aIssueType)
{
    if (mBitrateController.reportDownloadProblem(aIssueType))
    {
        mStreamUpdateNeeded = true;
    }
}
OMAF_NS_END
