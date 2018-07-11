
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
#include "DashProvider/NVRDashAdaptationSet.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"
#include "DashProvider/NVRMPDAttributes.h"
#include "DashProvider/NVRMPDExtension.h"
#include "Foundation/NVRLogger.h"
#include "Media/NVRMediaType.h"
#include "Foundation/NVRTime.h"
#include "DashProvider/NVRDashRepresentationExtractor.h"
#include "DashProvider/NVRDashRepresentationTile.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashAdaptationSet)

    static uint32_t kLatestAdSetId = 0;
    static uint32_t kMaxRemoteAdSetId = 0;

    DashAdaptationSet::DashAdaptationSet(DashAdaptationSetObserver& observer)
    : mMemoryAllocator(*MemorySystem::DefaultHeapAllocator())
    , mObserver(observer)
    , mDuration(0)
    , mMaxBandwidth(0)
    , mMinBandwidth(0)
    , mCurrentRepresentation(OMAF_NULL)
    , mContent()
    , mMetadataAdaptationSet(OMAF_NULL)
    , mNextRepresentation(OMAF_NULL)
    , mDownloadStartTime(INVALID_START_TIME)
    , mIsSeekable(false)
    , mCoveredViewport(OMAF_NULL)
    , mVideoChannel(StereoRole::UNKNOWN)
    , mVideoChannelIndex(0)
    , mSourceType(SourceType::INVALID)
    , mRepresentationLock()
    , mHasSegmentAlignmentId(false)
    , mDashType(DashType::INVALID)
    {

    }

    DashAdaptationSet::~DashAdaptationSet()
    {
        for (Representations::Iterator it = mRepresentations.begin();
            it != mRepresentations.end(); ++it)
        {
            OMAF_DELETE(mMemoryAllocator, *it);
        }
        OMAF_DELETE(mMemoryAllocator, mCoveredViewport);
        kLatestAdSetId = 0;
        kMaxRemoteAdSetId = 0;
    }

    bool_t DashAdaptationSet::externalAdSetIdExists()
    {
        return (kMaxRemoteAdSetId != 0);
    }
    uint32_t DashAdaptationSet::getNextAdSetId()
    {
        return ++kLatestAdSetId;
    }

    AdaptationSetType::Enum DashAdaptationSet::getType() const
    {
        return AdaptationSetType::BASELINE_DASH;
    }

    Error::Enum DashAdaptationSet::initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
    {
        return doInitialize(aDashComponents, aInitializationSegmentId);
    }

    Error::Enum DashAdaptationSet::doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
    {
        mDashType = DashType::BASELINE_DASH;

        // first, analyze what content the adaptation set has
        analyzeContent(aDashComponents);

        mAdaptationSetId = aDashComponents.adaptationSet->GetId();
        if (mAdaptationSetId != 0)
        {
            kLatestAdSetId = mAdaptationSetId;
        }
        // to separate the cases where a) there is no id for the ad set, and b) the id == 0, we use kMaxRemoteAdSetId. If it remains 0 after all adaptation sets are parsed, it means we need to give local ids
        kMaxRemoteAdSetId = max(kMaxRemoteAdSetId, mAdaptationSetId);

        DashComponents nextComponents = aDashComponents;
        // parse video properties. Need to be done only once, so done outside the representations-loop below
        if (mContent.matches(MediaContent::Type::HAS_VIDEO))
        {
            nextComponents.representation = aDashComponents.adaptationSet->GetRepresentation().at(0);
            if (!parseVideoProperties(nextComponents))
            {
                return Error::NOT_SUPPORTED;
            }
        }

        for (uint32_t index = 0; index < aDashComponents.adaptationSet->GetRepresentation().size(); index++)
        {
            dash::mpd::IRepresentation *representation = aDashComponents.adaptationSet->GetRepresentation().at(index);
            nextComponents.representation = representation;


            uint32_t bandwidth = representation->GetBandwidth();
            if ((bandwidth > mMaxBandwidth) || (index == 0))
            {
                mMaxBandwidth = bandwidth;
            }
            if ((bandwidth <= mMinBandwidth) || (index == 0))
            {
                mMinBandwidth = bandwidth;
            }
            DashRepresentation* newrepresentation = createRepresentation(nextComponents, aInitializationSegmentId, bandwidth);

            if (newrepresentation != OMAF_NULL)
            {
                //insert in bitrate order.
                size_t j = 0;
                for (j = 0; j < mRepresentations.getSize(); j++)
                {
                    if (mRepresentations[j]->getBitrate() > bandwidth)
                    {
                        mRepresentations.add(newrepresentation, j);
                        break;
                    }
                }
                if (j == mRepresentations.getSize())
                {
                    mRepresentations.add(newrepresentation);
                }
            }
        }

        if (mRepresentations.isEmpty())
        {
            // No representations found, returning
            return Error::NOT_SUPPORTED;
        }

        if (mContent.matches(MediaContent::Type::HAS_VIDEO))
        {
            //get the lowest bandwidth one... may be rewritten in inherited objects
            mCurrentRepresentation = mRepresentations.at(0);
        }
        else if (mContent.matches(MediaContent::Type::HAS_AUDIO))
        {
            // if conteins only audio / multiple representations of it select the one with highest bandwidth as channel info is optional
            // todo, if multiple audio codecs need to be supported in muxed content then implement support here (though needs work also in representations.
            mCurrentRepresentation = mRepresentations.at(mRepresentations.getSize() - 1); //get the highest bandwidth one..
        }
        else
        {
            mCurrentRepresentation = mRepresentations.at(0); //get the lowest bandwidth one..
        }

        mNextRepresentation = OMAF_NULL;

        OMAF_LOG_D("Adaptation set contains %d representations", mRepresentations.getSize());
        //I believe it is not possible for this to change.. (ie. either all representations are seekable or not!)
        //which comes from the fact that static streams are seekable, dynamic ones are not..
        mIsSeekable = mCurrentRepresentation->isSeekable();
        //and i also believe that duration of each representation should be the same too.
        mDuration = 0;
        if (mIsSeekable)
        {
            OMAF_LOG_D("Adaptation set is seekable");
        }
        for (uint32_t index = 0; index < mRepresentations.getSize(); index++)
        {
            OMAF_LOG_D("Representation id: %s", mRepresentations.at(index)->getId());
            OMAF_LOG_D("Representation bitrate: %d", mRepresentations.at(index)->getBitrate());
            if (mIsSeekable)
            {
                //and seekable streams have durations.. so cache them.
                uint64_t duration = mRepresentations.at(index)->durationMs();
                OMAF_LOG_D("Representation duration: %lld", duration);
                if (duration > mDuration)
                {
                    mDuration = duration;//just in case use the max.
                }
            }
            if (!mContent.matches(MediaContent::Type::VIDEO_EXTRACTOR) && !mContent.matches(MediaContent::Type::VIDEO_TILE)) // extractor track bitrates will be collected later on
            {
                mBitrates.add(mRepresentations.at(index)->getBitrate());
            }
        }

        if (aDashComponents.adaptationSet->SegmentAlignmentIsBoolValue())
        {
            mHasSegmentAlignmentId = false;
        }
        else
        {
            mHasSegmentAlignmentId = true;
            mSegmentAlignmentId = aDashComponents.adaptationSet->GetSegmentAligment();
        }

        OMAF_LOG_D("-------------------------------------------------");
        return Error::OK;
    }

    DashRepresentation* DashAdaptationSet::createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth)
    {
        DashRepresentation* newrepresentation = OMAF_NEW(mMemoryAllocator, DashRepresentation);
        if (newrepresentation->initialize(aDashComponents, aBandwidth, mContent, aInitializationSegmentId, this) != Error::OK)
        {
            OMAF_LOG_W("Non-supported representation skipped");
            OMAF_DELETE(mMemoryAllocator, newrepresentation);
            return OMAF_NULL;
        }
        aInitializationSegmentId++;

        if (mContent.matches(MediaContent::Type::HAS_VIDEO))
        {
            parseVideoViewport(aDashComponents);
        }

        // set cache mode: auto
        newrepresentation->setCacheFillMode(true);

        return newrepresentation;
    }

    bool_t DashAdaptationSet::checkMime(const std::string& aMimeType)
    {
        if (aMimeType.empty())
        {
            return false;
        }
        if (MPDAttribute::VIDEO_MIME_TYPE.compare(aMimeType.c_str()) == ComparisonResult::EQUAL)
        {
            mContent.addType(MediaContent::Type::HAS_VIDEO);
            return true;
        }
        else if (MPDAttribute::AUDIO_MIME_TYPE.compare(aMimeType.c_str()) == ComparisonResult::EQUAL)
        {
            mContent.addType(MediaContent::Type::HAS_AUDIO);
            return true;
        }
        else if (MPDAttribute::META_MIME_TYPE.compare(aMimeType.c_str()) == ComparisonResult::EQUAL)
        {
            mContent.addType(MediaContent::Type::HAS_META);
            return true;
        }
        else if (MPDAttribute::VIDEO_MIME_TYPE_OMAF_VI.compare(aMimeType.c_str()) == ComparisonResult::EQUAL ||
            MPDAttribute::VIDEO_MIME_TYPE_OMAF_VI_ESC.compare(aMimeType.c_str()) == ComparisonResult::EQUAL ||
            MPDAttribute::VIDEO_MIME_TYPE_OMAF_VD.compare(aMimeType.c_str()) == ComparisonResult::EQUAL ||
            MPDAttribute::VIDEO_MIME_TYPE_OMAF_VD_ESC.compare(aMimeType.c_str()) == ComparisonResult::EQUAL)
        {
            mContent.addType(MediaContent::Type::HAS_VIDEO);
            mDashType = DashType::OMAF;
            return true;
        }
        return false;
    }

    bool_t DashAdaptationSet::checkCodecs(const std::string& aCodec)
    {
        if (aCodec.empty())
        {
            return false;
        }
        if (aCodec.find("avc") != std::string::npos)
        {
            mContent.addType(MediaContent::Type::AVC);
            return true;
        }
        else if (aCodec.find("hevc") != std::string::npos || aCodec.find("hev1") != std::string::npos || aCodec.find("hvc1") != std::string::npos)
        {
            mContent.addType(MediaContent::Type::HEVC);
            return true;
        }
        else if (aCodec.find("hvc2") != std::string::npos)
        {
            mContent.addType(MediaContent::Type::HEVC);
            mContent.addType(MediaContent::Type::VIDEO_EXTRACTOR);
            return true;
        }
        return false;
    }

    void_t DashAdaptationSet::analyzeContent(DashComponents aDashComponents)
    {
        for (uint32_t index = 0; index < aDashComponents.adaptationSet->GetContentComponent().size(); index++)
        {
            const std::string& contentType = aDashComponents.adaptationSet->GetContentComponent().at(index)->GetContentType();
            if (MPDAttribute::VIDEO_CONTENT.compare(contentType.c_str()) == ComparisonResult::EQUAL)
            {
                mContent.addType(MediaContent::Type::HAS_VIDEO);
            }
            else if (MPDAttribute::AUDIO_CONTENT.compare(contentType.c_str()) == ComparisonResult::EQUAL)
            {
                mContent.addType(MediaContent::Type::HAS_AUDIO);
            }
            else if (MPDAttribute::METADATA_CONTENT.compare(contentType.c_str()) == ComparisonResult::EQUAL)
            {
                mContent.addType(MediaContent::Type::HAS_META);
            }
        }

        if (aDashComponents.adaptationSet->GetContentComponent().size() == 0)
        {
            checkMime(aDashComponents.adaptationSet->GetMimeType());
        }

        for (size_t codecIndex = 0; codecIndex < aDashComponents.adaptationSet->GetCodecs().size(); codecIndex++)
        {
            checkCodecs(aDashComponents.adaptationSet->GetCodecs().at(codecIndex));
        }
        // In OMAF case, the codecs property is checked later too in representation level, but it is then already too late to affect the preselection.
        // OMAF spec does not clearly mandate the use of codecs in adaptation set level, but that is the place to indicate extractor type, which is then referred to in preselections, which is an adaptation set level feature.

        DashComponents nextComponents = aDashComponents;
        if (!mContent.isSpecified())
        {
            for (uint32_t index = 0; index < aDashComponents.adaptationSet->GetRepresentation().size(); index++)
            {
                dash::mpd::IRepresentation *representation = aDashComponents.adaptationSet->GetRepresentation().at(index);
                nextComponents.representation = representation;
                if (DashUtils::elementHasAttribute(representation, "mimeType"))
                {
                    //has mimetype use that.
                    checkMime(representation->GetMimeType());
                }

                for (size_t codecIndex = 0; codecIndex < representation->GetCodecs().size(); codecIndex++)
                {
                    checkCodecs(representation->GetCodecs().at(codecIndex));
                    // In OMAF case, the codecs property is checked later too
                }
            }
        }
        if (mContent.matches(MediaContent::Type::HAS_VIDEO) && mContent.matches(MediaContent::Type::HAS_AUDIO))
        {
            OMAF_LOG_D("-------------------------------------------------");
            OMAF_LOG_D("Created adaptation set with muxed video and audio");
            mContent.addType(MediaContent::Type::IS_MUXED);
        }
        else if (mContent.matches(MediaContent::Type::HAS_VIDEO))
        {
            OMAF_LOG_D("-------------------------------------------------");
            OMAF_LOG_D("Created adaptation set with video");
        }
        else if (mContent.matches(MediaContent::Type::HAS_AUDIO))
        {
            OMAF_LOG_D("-------------------------------------------------");
            OMAF_LOG_D("Created adaptation set with audio");
        }
        else if (mContent.matches(MediaContent::Type::HAS_META))
        {
            OMAF_LOG_D("-------------------------------------------------");
            OMAF_LOG_D("Created adaptation set with metadata");
        }

        if (mContent.matches(MediaContent::Type::HAS_AUDIO))
        {
            mContent.addType(MediaContent::Type::AUDIO_LOUDSPEAKER);
        }
    }

    uint32_t DashAdaptationSet::getCurrentWidth() const
    {
        if (mCurrentRepresentation != OMAF_NULL)
        {
            return mCurrentRepresentation->getWidth();
        }
        else
        {
            return 0;
        }
    }

    uint32_t DashAdaptationSet::getCurrentHeight() const
    {
        if (mCurrentRepresentation != OMAF_NULL)
        {
            return mCurrentRepresentation->getHeight();
        }
        else
        {
            return 0;
        }
    }

    uint32_t DashAdaptationSet::getCurrentBandwidth() const
    {
        if (mCurrentRepresentation != OMAF_NULL)
        {
            return mCurrentRepresentation->getBitrate();
        }
        else
        {
            return 0;
        }
    }

    float64_t DashAdaptationSet::getCurrentFramerate() const
    {
        if (mCurrentRepresentation != OMAF_NULL)
        {
            return mCurrentRepresentation->getFramerate();
        }
        else
        {
            return 0;
        }
    }

    float64_t DashAdaptationSet::getMinResXFps() const
    {
        // resource usage of the lowest ABR level, relevant if this is base layer used with tiles
        if (mRepresentations.at(0)->getFramerate() > 0.f)
        {
            return mRepresentations.at(0)->getWidth() * mRepresentations.at(0)->getHeight() * mRepresentations.at(0)->getFramerate();
        }
        else
        {
            // there is no framerate in mpd, use default 30
            return mRepresentations.at(0)->getWidth() * mRepresentations.at(0)->getHeight() * 30.f;
        }
    }

    const Bitrates& DashAdaptationSet::getBitrates()
    {
        Spinlock::ScopeLock lock(mRepresentationLock);
        return mBitrates;
    }

    bool_t DashAdaptationSet::isActive() const
    {
        if (mNextRepresentation != OMAF_NULL)
        {
            return mNextRepresentation->isDownloading();
        }
        else
        {
            return mCurrentRepresentation->isDownloading();
        }
    }

    float32_t DashAdaptationSet::getDownloadSpeedFactor()
    {
        if (mCurrentRepresentation != OMAF_NULL)
        {
            float32_t currFactor = mCurrentRepresentation->getDownloadSpeedFactor();
            if (mNextRepresentation != OMAF_NULL)
            {
                return (mNextRepresentation->getDownloadSpeedFactor() + currFactor)/2.0f;
            }
        }
        return 0.0f;
    }

    Error::Enum DashAdaptationSet::startDownload(time_t startTime)
    {
        mDownloadStartTime = startTime;
        if (mNextRepresentation != OMAF_NULL)
        {
            return mNextRepresentation->startDownload(startTime);
        }
        else
        {
            return mCurrentRepresentation->startDownload(startTime);
        }
    }

    Error::Enum DashAdaptationSet::startDownload(uint64_t overridePTSUs, uint32_t overrideSegmentId)
    {
        if (mNextRepresentation != OMAF_NULL)
        {
            return mNextRepresentation->startDownloadWithOverride(overridePTSUs, overrideSegmentId);
        }
        else
        {
            return mCurrentRepresentation->startDownloadWithOverride(overridePTSUs, overrideSegmentId);
        }
    }

    Error::Enum DashAdaptationSet::stopDownload()
    {
        if (mNextRepresentation)
        {
            //stop downloading next representation (ABR)
            mNextRepresentation->stopDownload();
            //and then clear what we got.
            mNextRepresentation->clearDownloadedContent();
            mNextRepresentation = OMAF_NULL;
        }

        mDownloadStartTime = INVALID_START_TIME;
        return mCurrentRepresentation->stopDownload();
    }

    Error::Enum DashAdaptationSet::stopDownloadAsync(bool_t aReset)
    {
        mDownloadStartTime = INVALID_START_TIME;
        if (mNextRepresentation)
        {
            mNextRepresentation->stopDownloadAsync(aReset);

            mNextRepresentation = OMAF_NULL;
        }
        return mCurrentRepresentation->stopDownloadAsync(aReset);
    }

    bool_t DashAdaptationSet::supportsSubSegments() const
    {
        return mCurrentRepresentation->supportsSubSegments();
    }

    Error::Enum DashAdaptationSet::hasSubSegments(uint64_t targetPtsUs, uint32_t overrideSegmentId)
    {
        return mCurrentRepresentation->hasSubSegmentsFor(targetPtsUs, overrideSegmentId);
    }

    Error::Enum DashAdaptationSet::startSubSegmentDownload(uint64_t targetPtsUs, uint32_t overrideSegmentId)
    {
        return mCurrentRepresentation->startSubSegmentDownload(targetPtsUs, overrideSegmentId);
    }

    void_t DashAdaptationSet::clearDownloadedContent()
    {
        if (mNextRepresentation)
        {
            //stop downloading next representation (ABR)
            mCurrentRepresentation->stopDownload();
            //and then clear what we got.
            mNextRepresentation->clearDownloadedContent();
            mNextRepresentation = OMAF_NULL;
        }
        for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
        {
            (*it)->clearDownloadedContent();
        }
    }

    MediaContent& DashAdaptationSet::getAdaptationSetContent()
    {
        return mContent;
    }

    const RepresentationId& DashAdaptationSet::getCurrentRepresentationId()
    {
        return mCurrentRepresentation->getSegmentContent().representationId;
    }

    DashRepresentation* DashAdaptationSet::getCurrentRepresentation()
    {
        return mCurrentRepresentation;
    }

    bool_t DashAdaptationSet::isAssociatedToRepresentation(RepresentationId& aAssociatedTo)
    {
        aAssociatedTo = mCurrentRepresentation->getSegmentContent().associatedToRepresentationId;
        return (aAssociatedTo.getSize() > 0);
    }

    void DashAdaptationSet::setMetadataAdaptationSet(DashAdaptationSet* metadata)
    {
        mMetadataAdaptationSet = metadata;
    }

    DashAdaptationSet* DashAdaptationSet::getMetadataAdaptationSet()
    {
        return mMetadataAdaptationSet;
    }

    const MP4VideoStreams& DashAdaptationSet::getCurrentVideoStreams()
    {
        return mCurrentRepresentation->getCurrentVideoStreams();
    }

    const MP4AudioStreams& DashAdaptationSet::getCurrentAudioStreams()
    {
        return mCurrentRepresentation->getCurrentAudioStreams();
    }

    Error::Enum DashAdaptationSet::readNextVideoFrame(int64_t currentTimeUs)
    {
        bool_t segmentChanged = false;
        Error::Enum result = mCurrentRepresentation->readNextVideoFrame(segmentChanged, currentTimeUs);
        if (segmentChanged)
        {
            mCurrentRepresentation->releaseOldSegments();
        }
        return result;
    }

    Error::Enum DashAdaptationSet::readNextAudioFrame()
    {
        bool_t segmentChanged = false;
        Error::Enum result = mCurrentRepresentation->readNextAudioFrame(segmentChanged);
        if (segmentChanged)
        {
            mCurrentRepresentation->releaseOldSegments();
        }
        return result;
    }

    bool_t DashAdaptationSet::createVideoSources(sourceid_t& firstSourceId)
    {
        // taken from MPD, should be called only once
        if (hasMPDVideoMetadata())
        {
            // create a source for each representation
            for (Representations::Iterator it = mRepresentations.begin();
                it != mRepresentations.end(); ++it)
            {
                if (isBaselayer())
                {
                    (*it)->createVideoSource(firstSourceId, mSourceType, mVideoChannel);
                }
                else
                {
                    (*it)->createVideoSource(firstSourceId, mSourceType, mVideoChannel, *mCoveredViewport);
                }
            }
            return true;
        }
        return false;
    }

    Error::Enum DashAdaptationSet::createMetadataParserLink(DashAdaptationSet* aDependsOn)
    {
        if (aDependsOn->getCurrentRepresentation() == OMAF_NULL)
        {
            return Error::NOT_INITIALIZED;
        }

        Error::Enum error = Error::OK;
        for (Representations::Iterator it = mRepresentations.begin();
            it != mRepresentations.end(); ++it)
        {
            if ((*it)->getSegmentContent().associatedToRepresentationId == aDependsOn->getCurrentRepresentationId())
            {
                (*it)->setAssociatedToRepresentation(aDependsOn->getCurrentRepresentation());
            }
        }
        return error;
    }

    const CoreProviderSources& DashAdaptationSet::getVideoSources()
    {
        // taken from MPD / NVR metadata
        return mCurrentRepresentation->getVideoSources();
    }

    const CoreProviderSourceTypes& DashAdaptationSet::getVideoSourceTypes()
    {
        return mCurrentRepresentation->getVideoSourceTypes();
    }

    bool_t DashAdaptationSet::isSeekable()
    {
        return mIsSeekable;
    }

    uint64_t DashAdaptationSet::durationMs()
    {
        return mDuration;
    }

    bool_t DashAdaptationSet::isBuffering()
    {
        bool_t isBuffering = false;
        if (!mCurrentRepresentation->isDone())
        {
            // in practice the buffering state is reached this way only in the very beginning. After that buffering is detected by empty packet during processing
            isBuffering = !mCurrentRepresentation->getIsInitialized();//  isBuffering();
        }
        return isBuffering;
    }

    bool_t DashAdaptationSet::isEndOfStream()
    {
        if (mCurrentRepresentation->isEndOfStream())// && mCurrentRepresentation->isDone())
        {
            return true;
        }
        return false;
    }

    bool_t DashAdaptationSet::isError()
    {
        return mCurrentRepresentation->isError();
    }

    Error::Enum DashAdaptationSet::seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs)
    {
        Error::Enum result = mCurrentRepresentation->seekToMs(aSeekToTargetMs, aSeekToResultMs);
        // other representations should get the indices right when doing quality / ABR switch

        return result;
    }

    // representations that are functionally identical within the same adaptation set have the same id, so should be enough to return the first one
    const char_t* DashAdaptationSet::getRepresentationId()
    {
        if (mRepresentations.isEmpty())
        {
            return OMAF_NULL;
        }
        for (Representations::Iterator it = mRepresentations.begin();
            it != mRepresentations.end(); ++it)
        {
            const char_t* id = (*it)->getId();
            if (id != OMAF_NULL)
            {
                return id;
            }
        }
        return OMAF_NULL;
    }

    const char_t* DashAdaptationSet::isAssociatedTo()
    {
        if (mRepresentations.isEmpty())
        {
            return OMAF_NULL;
        }
        for (Representations::Iterator it = mRepresentations.begin();
            it != mRepresentations.end(); ++it)
        {
            const char_t* associatedTo = (*it)->isAssociatedTo();
            if (associatedTo != OMAF_NULL)
            {
                // this is used currently only for associating metadata to media
                // we don't support complex cases where representations inside adaptation set have different associations
                return associatedTo;
            }
        }
        return OMAF_NULL;
    }

    uint64_t DashAdaptationSet::getReadPositionUs(uint32_t& segmentIndex)
    {
        if (mCurrentRepresentation)
        {
            return mCurrentRepresentation->getReadPositionUs(segmentIndex);
        }
        else
        {
            // no segments cached yet, start from 0
            segmentIndex = INVALID_SEGMENT_INDEX;
            return 0;
        }
    }

    uint32_t DashAdaptationSet::getLastSegmentId() const
    {
        if (mCurrentRepresentation)
        {
            return mCurrentRepresentation->getLastSegmentId();
        }
        else
        {
            return INVALID_SEGMENT_INDEX;
        }
    }

    uint32_t DashAdaptationSet::getSegmentId()
    {
        if (mCurrentRepresentation)
        {
            return mCurrentRepresentation->getSegmentId();
        }
        else
        {
            return INVALID_SEGMENT_INDEX;
        }
    }

    bool_t DashAdaptationSet::isBaselayer() const
    {
        return !mContent.matches(MediaContent::Type::VIDEO_ENHANCEMENT);
    }

    StereoRole::Enum DashAdaptationSet::getVideoChannel() const
    {
        return mVideoChannel;
    }

    const VASTileViewport* DashAdaptationSet::getCoveredViewport() const
    {
        return mCoveredViewport;
    }

    bool_t DashAdaptationSet::forceVideoTo(StereoRole::Enum aToRole)
    {
        // Works only if metadata is from MPD, can be called also before video source was created. Can't assert it since condition would rely on Representation - or then we introduce some flag in AdaptationSet

        if (aToRole == StereoRole::MONO && (mVideoChannel == StereoRole::LEFT || mVideoChannel == StereoRole::RIGHT || mVideoChannel == StereoRole::FRAME_PACKED))
        {
            mVideoChannel = StereoRole::MONO;
            // loop through all representations
            for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
            {
                (*it)->updateVideoSourceTo(SourcePosition::MONO);
            }
            return true;
        }
        //TODO do we want to support upgrading to stereo? Upgrading the sources should be delayed until the right channel has some data?
        else if ((aToRole == StereoRole::RIGHT || aToRole == StereoRole::FRAME_PACKED) && mVideoChannel == StereoRole::MONO)
        {
            mVideoChannel = aToRole;
            // loop through all representations
            for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
            {
                (*it)->updateVideoSourceTo(SourcePosition::RIGHT);
            }
            return true;
        }
        return false;
    }

    bool_t DashAdaptationSet::hasMPDVideoMetadata() const
    {
        return (mDashType == DashType::DASH_WITH_VIDEO_METADATA || mDashType == DashType::OMAF);
    }

    void_t DashAdaptationSet::onSegmentDownloaded(DashRepresentation *representation)
    {
        if (representation == mCurrentRepresentation)
        {
            // report possible download problems based on download speed factors - upper layer may decide if the report is used (to avoid conflicts with other bandwidth estimation tools)
            if (mContent.matches(MediaContent::Type::HAS_VIDEO))
            {
                // Check if download speed is too low
                float32_t currentDownloadSpeedFactor = representation->getDownloadSpeedFactor();

                if (mContent.matches(MediaContent::Type::VIDEO_ENHANCEMENT))
                {
                    if (currentDownloadSpeedFactor > 0.f && currentDownloadSpeedFactor < 1.2f)
                    {
                        mObserver.onDownloadProblem(IssueType::ENH_LAYER_DELAYED);
                    }
                }
                else
                {
                    if (currentDownloadSpeedFactor > 0.f && currentDownloadSpeedFactor < 1.2f) // separating gives a possibility to have dedicated criteria for both layers
                    {
                        mObserver.onDownloadProblem(IssueType::BASELAYER_DELAYED);
                    }
                }
            }
        }
        if (mContent.matches(MediaContent::Type::VIDEO_ENHANCEMENT))
        {
            OMAF_LOG_D("Downloaded segment for longitude %f", mCoveredViewport->getCenterLongitude());
        }
        else if (mContent.matches(MediaContent::Type::VIDEO_BASE))
        {
            OMAF_LOG_D("Downloaded segment for baselayer");
        }
    }

    void_t DashAdaptationSet::onNewStreamsCreated()
    {
        if (!isBaselayer())
        {
            mObserver.onNewStreamsCreated();
        }
    }

    void_t DashAdaptationSet::onCacheWarning()
    {
        mObserver.onDownloadProblem(IssueType::BASELAYER_BUFFERING);
    }

    bool_t DashAdaptationSet::mpdUpdateRequired()
    {
        bool_t updateRequired = false;
        if (mNextRepresentation != OMAF_NULL)
        {
            updateRequired = mNextRepresentation->mpdUpdateRequired();
        }
        if (!updateRequired)
        {
            updateRequired = mCurrentRepresentation->mpdUpdateRequired();
        }
        return updateRequired;
    }

    Error::Enum DashAdaptationSet::updateMPD(DashComponents dashComponents)
    {
        for (uint32_t index = 0; index < dashComponents.adaptationSet->GetRepresentation().size(); index++)
        {
            dash::mpd::IRepresentation *representation = dashComponents.adaptationSet->GetRepresentation().at(index);

            DashComponents repComponents = dashComponents;
            repComponents.representation = representation;
            // Find the correct item from the ordered list
            for (uint32_t index2 = 0; index2 < mRepresentations.getSize(); index2++)
            {
                if (mRepresentations.at(index2)->getBitrate() == representation->GetBandwidth())
                {
                    mRepresentations.at(index2)->updateMPD(repComponents);
                    break;
                }
                else if (index2 == mRepresentations.getSize() - 1)
                {
                    // No representation found, this is not supported yet
                    return Error::NOT_SUPPORTED;
                }
            }
        }
        return Error::OK;
    }

    bool_t DashAdaptationSet::processSegmentDownload()
    {
        mCurrentRepresentation->processSegmentDownload();

        if (mNextRepresentation != OMAF_NULL)
        {
            //technically the new stream/representation/decoder will get activated HERE!
            //(stream activate gets called when init segment is processed)
            mNextRepresentation->processSegmentDownload();
        }

        // For ABR, check if there's a switch pending and if current has any more packets left for video and if not, switch
        if (mNextRepresentation != OMAF_NULL && mCurrentRepresentation != mNextRepresentation)
        {
            bool_t doTheSwitch = false;
            if (mContent.matches(MediaContent::Type::VIDEO_BASE))
            {
                doTheSwitch = mCurrentRepresentation->isDone();
            }
            else if (mContent.matches(MediaContent::Type::VIDEO_ENHANCEMENT))
            {
                // switch when the new representation is ready
                doTheSwitch = !mNextRepresentation->isBuffering();
            }
            else
            {
                OMAF_ASSERT(false, "Wrong config, OMAF without partial sets??");
            }
            if (doTheSwitch)
            {
                doSwitchRepresentation();
                return true;
            }
        }
        return false;
    }

    void_t DashAdaptationSet::doSwitchRepresentation()
    {
        //deactivate old representations decoder..
        mCurrentRepresentation->clearDownloadedContent();
#ifdef OMAF_PLATFORM_ANDROID
        OMAF_LOG_V("%d Switch done from %d to %d", Time::getClockTimeMs(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getBitrate());
#else
        OMAF_LOG_V("%lld Switch done from %s (%d) to %s (%d)", Time::getClockTimeMs(), mCurrentRepresentation->getId(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getId(), mNextRepresentation->getBitrate());
#endif
        mCurrentRepresentation = mNextRepresentation;
        mNextRepresentation = OMAF_NULL;
    }

    void_t DashAdaptationSet::processSegmentIndexDownload(uint32_t segmentId)
    {
        if (segmentId != INVALID_SEGMENT_INDEX)
        {
            mCurrentRepresentation->processSegmentIndexDownload(segmentId);
        }
    }

    VideoStreamMode::Enum DashAdaptationSet::getVideoStreamMode() const
    {
        const MP4VideoStreams& vs = mCurrentRepresentation->getCurrentVideoStreams();
        if (vs.isEmpty())
        {
            return VideoStreamMode::INVALID;
        }

        return vs[0]->getMode();
    }

    bool_t DashAdaptationSet::getAlignmentId(uint64_t& segmentDurationMs, uint32_t& aAlignmentId)
    {
        if (mCurrentRepresentation != OMAF_NULL)
        {
            mCurrentRepresentation->isSegmentDurationFixed(segmentDurationMs);
        }
        if (mHasSegmentAlignmentId)
        {
            aAlignmentId = mSegmentAlignmentId;
            return true;
        }
        else
        {
            return false;
        }
    }

    uint32_t DashAdaptationSet::getNrOfRepresentations() const
    {
        return (uint32_t)mRepresentations.getSize();
    }

    Error::Enum DashAdaptationSet::getRepresentationParameters(uint32_t aIndex, uint32_t& aWidth, uint32_t& aHeight, float64_t& aFps) const
    {
        if (aIndex >= mRepresentations.getSize())
        {
            return Error::ITEM_NOT_FOUND;
        }
        DashRepresentation* repr = mRepresentations.at(aIndex);
        aWidth = repr->getWidth();
        aHeight = repr->getHeight();
        aFps = repr->getFramerate();

        return Error::OK;
    }

    Error::Enum DashAdaptationSet::setRepresentationNotSupported(uint32_t aIndex)
    {
        if (aIndex >= mRepresentations.getSize())
        {
            return Error::ITEM_NOT_FOUND;
        }
        mRepresentations.at(aIndex)->setNotSupported();

        // remove also from the list of bitrates - assumes the representations have unique bitrates
        Spinlock::ScopeLock lock(mRepresentationLock);
        mBitrates.remove(mRepresentations.at(aIndex)->getBitrate());

        return Error::OK;
    }

    DashRepresentation* DashAdaptationSet::getRepresentationForBitrate(uint32_t bitrate)
    {
        for (size_t index = 0; index < mRepresentations.getSize(); index++)
        {
            if (mRepresentations[index]->getBitrate() == bitrate)
            {
                return mRepresentations[index];
            }
        }
        return OMAF_NULL;
    }

    void_t DashAdaptationSet::selectBitrate(uint32_t aBitrate)
    {
        //TODO in OMAF case, we should aggregate the bitrates from the mSupportingSets and switch all here
        OMAF_LOG_V("selectBitrate %d", aBitrate);
        OMAF_ASSERT(mNextRepresentation == OMAF_NULL, "Trying to select bitrate before previous switch completed");
        if (aBitrate == getCurrentBandwidth())
        {
            if (isActive())
            {
                OMAF_LOG_V("Already using the selected bitrate %d", aBitrate);
            }
            return;
        }
        bool_t active = isActive();
        DashRepresentation* nextRepresentation = getRepresentationForBitrate(aBitrate);
        if (nextRepresentation == mNextRepresentation)
        {
            // Already switching to this bitrate
            OMAF_LOG_V("already switching");
            return;
        }

        if (active)
        {
            uint32_t segmentIndex = mCurrentRepresentation->getLastSegmentId() + 1;
            mCurrentRepresentation->stopDownloadAsync(false);
            if (mNextRepresentation != OMAF_NULL)
            {
                OMAF_LOG_V("NextRepresentation %s was downloading, stop it", mNextRepresentation->getId());
                mNextRepresentation->stopDownload();
            }
            OMAF_LOG_V("Switch to %s", nextRepresentation->getId());
            mNextRepresentation = nextRepresentation;
            if (!mContent.matches(MediaContent::Type::VIDEO_ENHANCEMENT))//What if it is enhancement? When it starts?
            {
                OMAF_LOG_V("Start downloading next %s index %d", mNextRepresentation->getId(), segmentIndex);
                mNextRepresentation->startDownloadABR(segmentIndex);
            }
        }
        else
        {
            OMAF_LOG_V("Inactive, switch to next %s", nextRepresentation->getId());
            mCurrentRepresentation = nextRepresentation;
            mNextRepresentation = OMAF_NULL;
        }
    }

    bool_t DashAdaptationSet::isABRSwitchOngoing()
    {
        if (mNextRepresentation != OMAF_NULL)
        {
            return true;
        }
        return false;
    }

    bool_t DashAdaptationSet::parseVideoProperties(DashComponents& aNextComponents)
    {
        if (mDashType == DashType::OMAF)
        {
            mSourceType = DashOmafAttributes::getOMAFVideoProjection(aNextComponents);
        }
        else
        {
            mSourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
        }

        parseVideoChannel(aNextComponents);

        return true;
    }

    void_t DashAdaptationSet::parseVideoViewport(DashComponents& aNextComponents)
    {
        mContent.addType(MediaContent::Type::VIDEO_BASE);
    }

    bool_t DashAdaptationSet::parseVideoQuality(DashComponents& aNextComponents, uint8_t& aQualityLevel, bool_t& aGlobal, DashRepresentation* aLatestRepresentation)
    {
        return false;
    }

    void_t DashAdaptationSet::parseVideoChannel(DashComponents& aNextComponents)
    {
        mVideoChannel = DashAttributes::getStereoRole(aNextComponents);
        if (mVideoChannel == StereoRole::UNKNOWN)
        {
            // Frame-packing does not make sense with left and right roles defined. OMAF doesn't allow it in representation
            SourceDirection::Enum direction = DashAttributes::getFramePacking(aNextComponents);
            if (direction == SourceDirection::TOP_BOTTOM)
            {
                aNextComponents.basicSourceInfo.sourceDirection = SourceDirection::TOP_BOTTOM;  // Note: we don't know if L is top and R bottom, or vice versa. Would need additional metadata for it. So guessing.
                mVideoChannel = StereoRole::FRAME_PACKED;
                mDashType = DashType::DASH_WITH_VIDEO_METADATA;
            }
            else if (direction == SourceDirection::LEFT_RIGHT)
            {
                aNextComponents.basicSourceInfo.sourceDirection = SourceDirection::LEFT_RIGHT; // Note: we don't know if L channel is on left side of packed frame and R on right, or vice versa. Would need additional metadata for it. So guessing.
                mVideoChannel = StereoRole::FRAME_PACKED;
                mDashType = DashType::DASH_WITH_VIDEO_METADATA;
            }
            else if (aNextComponents.basicSourceInfo.sourceDirection != SourceDirection::MONO && aNextComponents.basicSourceInfo.sourceDirection != SourceDirection::DUAL_TRACK)
            {
                // sourceDirection is interpreted by other means
                mVideoChannel = StereoRole::FRAME_PACKED;
            }
            else
            {
                // it does not have stereo role, and no frame packing, so it has to be mono (it could be 2LR, but we don't properly support stereo stream without a role)
                aNextComponents.basicSourceInfo.sourceDirection = SourceDirection::MONO;
                mVideoChannel = StereoRole::MONO;
            }
        }
        else
        {
            // It has stereo role, so it is not framepacked or mono, but one track for a dual-track stereo
            aNextComponents.basicSourceInfo.sourceDirection = SourceDirection::DUAL_TRACK;
            mDashType = DashType::DASH_WITH_VIDEO_METADATA;
        }
    }

    uint32_t DashAdaptationSet::getId() const
    {
        return mAdaptationSetId;
    }

    bool_t DashAdaptationSet::getIsInitialized() const
    {
        return mCurrentRepresentation->getIsInitialized();
    }

    uint32_t DashAdaptationSet::peekNextSegmentId() const
    {
        if (mCurrentRepresentation)
        {
            DashSegment* segment = mCurrentRepresentation->peekSegment();
            if (segment != OMAF_NULL)
            {
                return segment->getSegmentId();
            }
            else
            {
                return 1;
            }
        }
        return getLastSegmentId();
    }

    uint8_t DashAdaptationSet::getNrQualityLevels() const
    {
        return 1;
    }

    bool_t DashAdaptationSet::isReadyToSwitch(MP4MediaStream& aStream) const
    {
        if (mNextRepresentation != OMAF_NULL)
        {
            // there is a next representation waiting
            if (aStream.peekNextFilledPacket() == OMAF_NULL)
            {
                // this one doesn't have any data available
                return true;
            }
        }

        return false;
    }

OMAF_NS_END
