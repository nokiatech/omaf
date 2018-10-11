
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
#include "DashProvider/NVRDashVideoDownloader.h"
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
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


OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashVideoDownloader)

DashVideoDownloader::DashVideoDownloader()
    : mVideoBaseAdaptationSet(OMAF_NULL)
    , mStreamType(DashStreamType::INVALID)
    , mTilePicker(OMAF_NULL)
    , mCurrentVideoStreams()
    , mNewVideoStreamsCreated(false)
    , mVideoStreamsChanged(false)
    , mStreamUpdateNeeded(false)
    , mReselectSources(false)
    , mABREnabled(true)
    , mBandwidthOverhead(0)
    , mBaseLayerDecoderPixelsinSec(0)
    , mTileSetupDone(false)
{
}

DashVideoDownloader::~DashVideoDownloader()
{
    release();
}

void_t DashVideoDownloader::enableABR()
{
    mABREnabled = true;
}


void_t DashVideoDownloader::disableABR()
{
    mABREnabled = false;
}

void_t DashVideoDownloader::setBandwidthOverhead(uint32_t aBandwithOverhead)
{
    mBandwidthOverhead = aBandwithOverhead;
}

uint32_t DashVideoDownloader::getCurrentBitrate()
{
    uint32_t bitrate = 0;
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        bitrate += mVideoBaseAdaptationSet->getCurrentBandwidth();
    }

    return bitrate;
}

void_t DashVideoDownloader::getMediaInformation(MediaInformation& aMediaInformation)
{
    aMediaInformation.videoType = mMediaInformation.videoType;
    aMediaInformation.isStereoscopic = mMediaInformation.isStereoscopic;
    aMediaInformation.duration = mVideoBaseAdaptationSet->durationMs();
    aMediaInformation.width = mVideoBaseAdaptationSet->getCurrentWidth();
    aMediaInformation.height = mVideoBaseAdaptationSet->getCurrentHeight();
    const MediaContent& mediaContent = mVideoBaseAdaptationSet->getAdaptationSetContent();
    if (mediaContent.matches(MediaContent::Type::AVC))
    {
        aMediaInformation.baseLayerCodec = VideoCodec::AVC;
    }
    else if (mediaContent.matches(MediaContent::Type::HEVC))
    {
        aMediaInformation.baseLayerCodec = VideoCodec::HEVC;
    }
    else
    {
        // default to AVC?
        aMediaInformation.baseLayerCodec = VideoCodec::AVC;
    }
    aMediaInformation.enchancementLayerCodec = mMediaInformation.enchancementLayerCodec;
}

void_t DashVideoDownloader::getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrEnhLayerCodecs)
{
    aNrBaseLayerCodecs = 0;
    if (mVideoBaseAdaptationSet)
    {
        aNrBaseLayerCodecs++;
    }
}

Error::Enum DashVideoDownloader::completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo)
{
    Error::Enum result;

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        aDashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        // "normal" adaptation set
        dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSet)(*this);
        result = dashAdaptationSet->initialize(aDashComponents, initializationSegmentId);
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
        return Error::NOT_SUPPORTED;
    }

    mVideoBaseAdaptationSet = OMAF_NULL;

        sourceid_t sourceId = 0;
        for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
        {
            DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
            if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::IS_MUXED))
            {
                mVideoBaseAdaptationSet = dashAdaptationSet;    // no enh layer supported in muxed case, also baselayer must be in the same adaptation set
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
                if (dashAdaptationSet->hasMPDVideoMetadata())
                {
                    dashAdaptationSet->createVideoSources(sourceId);
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
                        }
                    }
                }
            }
        }

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
    }
    else
    {
        OMAF_LOG_E("Stream needs to have video");
        return Error::NOT_SUPPORTED;
    }

    if (aSourceInfo.sourceDirection != SourceDirection::Enum::MONO || mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
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
    mBitrateController.initialize(
        mVideoBaseAdaptationSet,
        OMAF_NULL,
        TileAdaptationSets(),
        mTilePicker);

    return Error::OK;
}

void_t DashVideoDownloader::release()
{
    stopDownload();

    while (!mAdaptationSets.isEmpty())
    {
        DashAdaptationSet* set = mAdaptationSets.at(0);
        mAdaptationSets.removeAt(0);
        OMAF_DELETE_HEAP(set);
    }

    mAdaptationSets.clear();
    //Make sure these are NULL
    mVideoBaseAdaptationSet = OMAF_NULL;
    OMAF_DELETE_HEAP(mTilePicker);
    mTilePicker = OMAF_NULL;

}

Error::Enum DashVideoDownloader::startDownload(time_t aStartTime, uint32_t aExpectedPingTimeMs)
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        //TODO setting viewing to 0 is needed in all OMAF cases

        mVideoBaseAdaptationSet->startDownload(aStartTime, aExpectedPingTimeMs);
        
        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

bool_t DashVideoDownloader::isMpdUpdateRequired() const
{
    return mVideoBaseAdaptationSet->mpdUpdateRequired();
}

bool_t DashVideoDownloader::isEndOfStream() const
{
    if (mVideoBaseAdaptationSet->isEndOfStream())
    {
        return true;
    }
    return false;
}

bool_t DashVideoDownloader::isError() const
{
    return mVideoBaseAdaptationSet->isError();
}

void_t DashVideoDownloader::updateStreams(uint64_t currentPlayTimeUs)
{
    //TODO this comes twice now
    processSegmentDownload();

    if (!mCurrentVideoStreams.isEmpty() && mABREnabled)
    {
        if (mBitrateController.update(mBandwidthOverhead))
        {
            mStreamUpdateNeeded = true;
        }
    }
}

Error::Enum DashVideoDownloader::stopDownload()
{

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mVideoBaseAdaptationSet->stopDownload();

        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

void_t DashVideoDownloader::clearDownloadedContent()
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mVideoBaseAdaptationSet->clearDownloadedContent();

        if (mTilePicker != OMAF_NULL)
        {
            mTilePicker->reset();
        }
    }
}

Error::Enum DashVideoDownloader::seekToMs(uint64_t& aSeekMs, uint64_t aSeekResultMs)
{
    uint64_t seekToTarget = aSeekMs;
    Error::Enum result = Error::OK;

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        result = mVideoBaseAdaptationSet->seekToMs(seekToTarget, aSeekResultMs);
        if (result == Error::OK)
        {
            aSeekMs = aSeekResultMs;
        }
    }

    return result;
}

bool_t DashVideoDownloader::isSeekable()
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

uint64_t DashVideoDownloader::durationMs() const
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

bool_t DashVideoDownloader::updateVideoStreams()
{
    bool_t refreshStillRequired = false;
    OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No current video adaptation set");
    mCurrentVideoStreams.clear();
    mCurrentVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
    if (mCurrentVideoStreams.isEmpty())
    {
        OMAF_LOG_V("updateVideoStreams refresh still required");
        refreshStillRequired = true;
    }
    return refreshStillRequired;
}

const MP4VideoStreams& DashVideoDownloader::getVideoStreams()
{
    if (mCurrentVideoStreams.isEmpty() || mNewVideoStreamsCreated.compareAndSet(false, true))
    {
        updateVideoStreams();
    }
    return mCurrentVideoStreams;
}


Error::Enum DashVideoDownloader::readVideoFrames(int64_t currentTimeUs)
{
    return mVideoBaseAdaptationSet->readNextVideoFrame(currentTimeUs);
}

MP4VRMediaPacket* DashVideoDownloader::getNextVideoFrame(MP4MediaStream &stream, int64_t currentTimeUs)
{
    // With Dash we don't need to do anything special here
    return stream.peekNextFilledPacket();
}

const CoreProviderSourceTypes& DashVideoDownloader::getVideoSourceTypes()
{
    OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No current video adaptation set");

    // we have MPD metadata; typically base + enh layers.
    if (mVideoSourceTypes.isEmpty())
    {
        // just 1 entry per type needed, so no need to check from the stereo or from many tiles
        mVideoSourceTypes.add(mVideoBaseAdaptationSet->getVideoSourceTypes());  // just 1 entry per type needed, so no need to check from the stereo
    }
    return mVideoSourceTypes;
}

bool_t DashVideoDownloader::isBuffering()
{
    const CoreProviderSourceTypes& sourceTypes = getVideoSourceTypes();
    if (sourceTypes.isEmpty())
    {
        return true;
    }
    bool_t videoBuffering = mVideoBaseAdaptationSet->isBuffering();
    // Tiles are not checked for buffering, as it is concluded a better UX to show base layer if no tiles are available, than pause video
    if (videoBuffering)
    {
        OMAF_LOG_D("isBuffering = TRUE");
    }
    return videoBuffering;
}

bool_t DashVideoDownloader::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    // this is relevant for video base layer only
    if (mVideoBaseAdaptationSet->isReadyToSignalEoS(aStream))
    {
        return true;
    }
    return false;
}

const MP4VideoStreams& DashVideoDownloader::selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId, bool_t& aViewportSet)
{
    // Non-enh layer case
    if ((mRenderingVideoStreams.isEmpty() || mReselectSources || mVideoStreamsChanged.compareAndSet(false, true)) && mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mRenderingVideoStreams.clear();
        mReselectSources = false;
        mVideoStreamsChanged = false;   // reset in case some other condition triggered too
        size_t count = 1;
        mRenderingVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
        if (mRenderingVideoStreams.getSize() < count)
        {
            //OMAF_LOG_V("Missing base layer streams for rendering: expected %zd now %zd", count, mRenderingVideoStreams.getSize());
            mReselectSources = true;
        }
    }

    aViewportSet = true;
    if (!mRenderingVideoStreams.isEmpty())
    {
        aBaseLayerStreamId = mRenderingVideoStreams.at(0)->getStreamId();
    }
    return mRenderingVideoStreams;
}

void_t DashVideoDownloader::processSegmentDownload()
{
    //OMAF_LOG_D("%lld processSegmentDownload in", Time::getClockTimeMs());

    if (mVideoBaseAdaptationSet->processSegmentDownload())
    {
        // Some ABR stream switches done, update video streams
        updateVideoStreams();
        mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
    }
}

void_t DashVideoDownloader::onNewStreamsCreated()
{
    // need to update mCurrentVideoStreams
    mNewVideoStreamsCreated = true;
}

void_t DashVideoDownloader::onDownloadProblem(IssueType::Enum aIssueType)
{
    if (mBitrateController.reportDownloadProblem(aIssueType))
    {
        mStreamUpdateNeeded = true;
    }
}
OMAF_NS_END
