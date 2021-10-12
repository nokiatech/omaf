
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
#include <algorithm>

#include "dashomaf.h"

#include "common.h"
#include "controllerconfigure.h"
#include "controllerops.h"
//#include "configreader.h"

#include "async/futureops.h"
#include "async/graphbase.h"
#include "common/utils.h"
#include "log/log.h"
#include "processor/metacapturesink.h"
#include "streamsegmenter/mpdtree.hpp"


namespace VDD
{
    namespace {
        // Very superficial validator; guards against one expected potential future issue
        void validateWithAssert(const StreamSegmenter::MPDTree::OmafMPDRoot& aMpd)
        {
            for (auto& period : aMpd.periods)
            {
                // ensure there are no duplicate adaptation set ids
                std::set<std::uint32_t> usedIds;
                for (auto& adaptationSet : period.adaptationSets)
                {
                    if (adaptationSet.id)
                    {
                        assert(usedIds.count(*adaptationSet.id) == 0);
                        usedIds.insert(*adaptationSet.id);
                    }
                }
            }
        }
    }

    RepresentationId preselectPropertyTagForExtractorId(StreamId aExtractorId)
    {
        using std::to_string;
        return "ext" + to_string(aExtractorId);
    }

    struct TileConfiguration
    {
    };

    DashOmaf::DashOmaf(std::shared_ptr<Log> aLog,
               const ConfigValue& aDashConfig,
               ControllerOps& aControllerOps)
        : Dash(aLog, aDashConfig, aControllerOps)
    {
    }

    DashOmaf::~DashOmaf()
    {
        // make an attempt to check that we're done to detect bugs early (hopefully)
        bool canLock = mMpdWritingMutex.try_lock();
        mMpdWritingMutex.unlock();
        assert(canLock);
    }

    void DashOmaf::setupMpd(Optional<Projection> aProjection)
    {
        if (mDashConfig)
        {
            if (mDashConfig["mpd"] && mDashConfig["mpd"]["filename"])
            {
                std::string templ = readString(mDashConfig["mpd"]["filename"]);
                mMpdFilename = Utils::replace(templ, "$Name$", getBaseName());
            }
            else if (getBaseName() != "")
            {
                mMpdFilename = getBaseName() + ".mpd";
            }
            else
            {
                throw ConfigValueInvalid("Missing name for the MPD or the output name in general", mDashConfig["mpd"]);
            }

            mMpdFilename = Utils::joinPathComponents(getBaseDirectory(), mMpdFilename);

            Utils::ensurePathForFilename(mMpdFilename);

            mMpd.type = StreamSegmenter::MPDTree::RepresentationType::Static;
            mMpd.minBufferTime = StreamSegmenter::MPDTree::Duration{ 3, 1 };
            switch (mDashProfile)
            {
            case DashProfile::Live:
                if (getOutputMode() == OutputMode::OMAFV1)
                {
                    mMpd.profile = StreamSegmenter::MPDTree::DashProfile::Live;
                }
                else
                {
                    mMpd.profile = StreamSegmenter::MPDTree::DashProfile::TiledLive;
                }
                break;
            case DashProfile::OnDemand:
                if (getOutputMode() == OutputMode::OMAFV1)
                {
                    mMpd.profile = StreamSegmenter::MPDTree::DashProfile::OnDemand;
                }
                else
                {
                    mMpd.profile = StreamSegmenter::MPDTree::DashProfile::TiledLive;
                }
                break;
            default:
                assert(false);
            }
            if (aProjection)
            {
                mMpd.projectionFormat = mpdProjectionFormatOfOmafProjectionType(aProjection->projection);
            }
            StreamSegmenter::MPDTree::OmafPeriod period{};
            period.id = getDefaultPeriodId();
            mMpd.periods.push_back(period);
            mMpd.periods.front().entityGroups = std::move(mEntityGroups);
            mEntityGroups.erase(mEntityGroups.begin(), mEntityGroups.end());

            for (auto& viewSegmentSavedSignals : mSegmentSavedSignals)
            {
                Optional<ViewId> viewId = viewSegmentSavedSignals.first;
                for (auto& savedSignal : viewSegmentSavedSignals.second)
                {
                    auto segmenterName = savedSignal.first;
                    connect(*savedSignal.second.combineSignals->getSource(),
                            "MPD writing for \"" + segmenterName + "\"",
                            [this, segmenterName, viewId](const Streams& aStreams) {
                                updateBitrates(viewId, aStreams);
                                collectDurations(viewId, segmenterName, aStreams);
                                writeMpd();
                            });
                }
            }
        }
    }

    void DashOmaf::updateBitrates(Optional<ViewId> aViewId, const Streams& aStreams)
    {
        std::map<std::string, SegmentBitrate> representationIdBitrates;
        size_t sinkIndex = 0u;
        for (auto stream : aStreams)
        {
            if (mSinkIndexToRepresentationId[aViewId].count(sinkIndex))
            {
                if (auto bitrateTag = stream.getMeta().findTag<SegmentBitrateTag>())
                {
                    representationIdBitrates[mSinkIndexToRepresentationId[aViewId].at(sinkIndex)] =
                        bitrateTag->get();
                }
            }
            ++sinkIndex;
        }

        {
            std::unique_lock<std::mutex> lockMpdWriting(mMpdWritingMutex);
            for (auto& period : mMpd.periods)
            {
                for (auto& adaptationSet : period.adaptationSets)
                {
                    for (auto& representation : adaptationSet.representations)
                    {
                        if (representationIdBitrates.count(representation.id))
                        {
                            representation.bandwidth =
                                representationIdBitrates.at(representation.id).average;
                        }
                    }
                }
            }
        }
    }

    namespace {
        template <typename Container, typename Comparator>
        bool lessThanContainer(const Container& a, const Container& b, const Comparator& cmp)
        {
            auto itA = a.begin();
            auto itB = b.begin();
            while (itA != a.end() && itB != b.end())
            {
                if (cmp(*itA, *itB))
                {
                    return true;
                }
                if (cmp(*itB, *itA))
                {
                    return false;
                }
                ++itA;
                ++itB;
            }
            // shorter list compares as less
            return itA == a.end();
        }

        bool lessThanAdaptationSet(const StreamSegmenter::MPDTree::OmafAdaptationSet& a,
                                  const StreamSegmenter::MPDTree::OmafAdaptationSet& b)
        {
            return a.id < b.id;
        }

        bool lessThanPeriod(const StreamSegmenter::MPDTree::OmafPeriod& a,
                           const StreamSegmenter::MPDTree::OmafPeriod& b)
        {
            return a.id < b.id ? true
                          : a.id > b.id ? false
                : lessThanContainer(a.adaptationSets, b.adaptationSets,
                                    lessThanAdaptationSet);
        }
    }  // namespace

    void DashOmaf::writeMpd()
    {
        std::unique_lock<std::mutex> lockMpdWriting(mMpdWritingMutex);

        Utils::ensurePathForFilename(mMpdFilename);
        std::ofstream out(mMpdFilename, std::ofstream::out | std::ofstream::binary);
        if (!out)
        {
            throw CannotOpenFile(mMpdFilename);
        }

        // lock adaptation set additions
        StreamSegmenter::MPDTree::OmafMPDRoot mpdConfigCopy;
        {
            std::unique_lock<std::mutex> lockMpdConfig(mMpdConfigMutex);
            // by this point everything is in, so we can update viewpoint data
            if (mViewpoints.size())
            {
                decltype(mViewpoints) updateViewpoints;
                std::swap(mViewpoints, updateViewpoints);
                for (auto& [adaptationId, viewpoint] : updateViewpoints)
                {
                    updateViewpointInternal(adaptationId, viewpoint);
                }
            }
            mpdConfigCopy = mMpd;
        }
        // produce stable output
        mpdConfigCopy.periods.sort(lessThanPeriod);
        for (auto& period: mpdConfigCopy.periods)
        {
            period.adaptationSets.sort(lessThanAdaptationSet);
        }

        validateWithAssert(mpdConfigCopy);
        mpdConfigCopy.writeXML(out);

        out.close();
    }

    void DashOmaf::collectDurations(Optional<ViewId> aViewId, DashSegmenterName aDashSegmenterName, const Streams& aStreams)
    {
        std::unique_lock<std::mutex> lockMpdWriting(mMpdWritingMutex);
        if (!aStreams.isEndOfStream() && aStreams.front().getContentType() == ContentType::CODED)
        {
            SegmentSaverSignal& saverSignal = mSegmentSavedSignals.at(aViewId).at(aDashSegmenterName);
            auto& segmentsTotalDuration = saverSignal.segmentsTotalDuration;
            segmentsTotalDuration += aStreams.front().getCodedFrameMeta().segmenterMeta.segmentDuration;

            if (!mMpd.mediaPresentationDuration || segmentsTotalDuration > *mMpd.mediaPresentationDuration)
            {
                mMpd.mediaPresentationDuration = StreamSegmenter::MPDTree::Duration(segmentsTotalDuration.num, segmentsTotalDuration.den);
                if (!mMpd.periods.empty())
                {
                    mMpd.periods.front().duration = mMpd.mediaPresentationDuration;
                }
            }
        }
        
        if (mOverrideTotalDuration)
        {
            mMpd.mediaPresentationDuration = StreamSegmenter::MPDTree::Duration((*mOverrideTotalDuration).num, (*mOverrideTotalDuration).den);
            if (!mMpd.periods.empty())
            {
                mMpd.periods.front().duration = mMpd.mediaPresentationDuration;
            }
        }

        if (!aStreams.isEndOfStream() && aStreams.front().getContentType() == ContentType::CODED)
        {
            // maybe not correct if we don't call writeMpd after this function.. But it's probably relevant.
            mLog->log(Info) << "Writing MPD with duration: "
                << aStreams.front().getCodedFrameMeta().segmenterMeta.segmentDuration
                << std::endl;
        }
    }

    void DashOmaf::configureMPDOverlays(StreamSegmenter::MPDTree::OmafAdaptationSet& aAdaptationSet,
                                        std::shared_ptr<DashMPDConfig> aDashMPDConfig,
                                        const std::vector<std::vector<Meta>>& /* aAllMeta */)
    {
        if (aDashMPDConfig->overlays)
        {
            StreamSegmenter::MPDTree::OverlayVideo overlayVideo{};
            Optional<bool> overlayPriorityExists;
            for (auto& singleOverlay : aDashMPDConfig->overlays->overlays)
            {
                StreamSegmenter::MPDTree::OverlayVideoInfo info{};
                info.overlayId = singleOverlay.overlayId;
                if (!overlayPriorityExists)
                {
                    overlayPriorityExists = singleOverlay.overlayPriority.doesExist;
                }
                else if (*overlayPriorityExists != singleOverlay.overlayPriority.doesExist)
                {
                    // not the most well-placed error message: we cannot determine its origin in the
                    // configuration
                    throw ConfigValueReadError(
                        "Either none or all overlay priorities must be provided");
                }

                if (singleOverlay.overlayPriority.doesExist)
                {
                    info.overlayPriority = singleOverlay.overlayPriority.overlayPriority;
                }

                overlayVideo.overlayInfo.push_back(info);
            }
            aAdaptationSet.overlayVideo = overlayVideo;
        }

        if (aDashMPDConfig->adaptationConfig.overlayBackground)
        {
            StreamSegmenter::MPDTree::OverlayBackground overlayBackground{};
            overlayBackground.backgroundIds =
                mapContainer<std::list>(*aDashMPDConfig->adaptationConfig.overlayBackground,
                                        [](const StreamId& aStreamId) { return aStreamId.get(); });
            aAdaptationSet.overlayBackground = overlayBackground;
        }
    }

    void DashOmaf::configureMPDWithMeta(std::shared_ptr<DashMPDConfig> aDashMPDConfig,
                                        const std::vector<std::vector<Meta>>& aAllMeta)
    {
        Optional<FrameRate> maxFrameRate;
        Optional<uint32_t> maxWidth;
        Optional<uint32_t> maxHeight;
        Optional<std::string> codecs;

        StreamSegmenter::MPDTree::OmafAdaptationSet adaptationSet;
        configureMPDOverlays(adaptationSet, aDashMPDConfig, aAllMeta);

        adaptationSet.id = aDashMPDConfig->adaptationConfig.adaptationSetId.get();
        adaptationSet.segmentAlignment =
            StreamSegmenter::MPDTree::BoolOrNumber{StreamSegmenter::MPDTree::Number{1}};
        adaptationSet.projectionFormat = aDashMPDConfig->adaptationConfig.projectionFormat;

        if (auto packingType = findOptional(
                aDashMPDConfig->segmenterOutput,
                std::map<PipelineOutput, uint8_t>{{PipelineOutputVideo::TopBottom, 4},
                                                  {PipelineOutputVideo::SideBySide, 3},
                                                  {PipelineOutputVideo::TemporalInterleaving, 5}}))
        {
            StreamSegmenter::MPDTree::VideoFramePacking packing{};
            packing.packingType = *packingType;
            adaptationSet.videoFramePacking = packing;
        }
        adaptationSet.stereoId = aDashMPDConfig->adaptationConfig.stereoId;

        size_t representationIndex = 0;
        auto frameDuration = aAllMeta.front().front().getCodedFrameMeta().duration;
        for (const auto& representationInfo : aDashMPDConfig->adaptationConfig.representations)
        {
            auto& representationConfig = representationInfo.representationConfig;
            auto& meta = aAllMeta[representationIndex];
            const CodedFrameMeta& codedMeta = meta.front().getCodedFrameMeta();

            StreamSegmenter::MPDTree::OmafRepresentation representation;

            for (auto track :
                 representationConfig.output.segmenterAndSaverConfig.segmenterConfig.tracks)
            {
                switch (track.second.type)
                {
                case StreamSegmenter::MediaType::Video:
                {
                    if (!maxFrameRate)
                    {
                        maxFrameRate = frameDuration.per1();
                    }
                    else
                    {
                        maxFrameRate = std::max(*maxFrameRate, frameDuration.per1());
                    }
                    maxFrameRate = maxFrameRate->minimize();

                    if (!maxWidth)
                    {
                        maxWidth = codedMeta.width;
                    }
                    else
                    {
                        maxWidth = std::max(*maxWidth, codedMeta.width);
                    }
                    if (!maxHeight)
                    {
                        maxHeight = codedMeta.height;
                    }
                    else
                    {
                        maxHeight = std::max(*maxHeight, codedMeta.height);
                    }

                    if (!codecs)
                    {
                        // Change of codec not allowed between representations in OMAF, hence no
                        // need to repeat for each representation
                        std::string codec;
                        const std::vector<uint8_t>& sps =
                            codedMeta.decoderConfig.at(VDD::ConfigType::SPS);
                        if (codedMeta.format == CodedFormat::H265)
                        {
                            bool hasProjection = bool(codedMeta.projection);
                            if (hasProjection)
                            {
                                if (codedMeta.qualityRankCoverage)
                                {
                                    // viewport dependent
                                    codec = "resv.podv+ercm.";
                                }
                                else
                                {
                                    // assuming viewport independent
                                    codec = "resv.podv+erpv.";
                                }
                            }
                            codec += "hvc1.";
                            Dash::createHevcCodecsString(sps, codec);
                        }
                        else if (codedMeta.format == CodedFormat::H265Extractor)
                        {
                            codec = "resv.podv+ercm.hvc2.";
                            Dash::createHevcCodecsString(sps, codec);
                        }
                        else
                        {
                            // from ISO-IEC 14496-15 2014 Annex E
                            codec = "avc1.";
                            Dash::createAvcCodecsString(sps, codec);
                        }
                        codecs = codec;
                    }
                    break;
                }

                case StreamSegmenter::MediaType::Audio:
                {
                    representation.audioSamplingRate.push_back(codedMeta.samplingFrequency);
                    if (!codecs)
                    {
                        // Change of codec not allowed between representations in OMAF, hence no
                        // need to repeat for each representation mp4a.40.2 from
                        // https://tools.ietf.org/html/rfc6381#section-3.3 = 40 (identifying MPEG -
                        // 4 audio), 2 (AAC - LC)
                        std::string codec = "mp4a.40.2";
                        codecs = codec;
                    }
                    break;
                }

                case StreamSegmenter::MediaType::Data:
                case StreamSegmenter::MediaType::Other:
                {
                    // invalid?
                    break;
                }
                default:
                    assert(false);
                }
            }

            if (codedMeta.format == CodedFormat::H265 ||
                codedMeta.format == CodedFormat::H265Extractor)
            {
                // for extractor, we don't expect to have quality ranking except for multi-res case
                if (codedMeta.qualityRankCoverage && codedMeta.sphericalCoverage)
                {
                    // add quality
                    StreamSegmenter::MPDTree::OmafSphereRegionWiseQuality srqr;
                    if (codedMeta.projection == ISOBMFF::makeOptional(OmafProjectionType::Equirectangular))
                    {
                        srqr.shapeType = StreamSegmenter::MPDTree::OmafShapeType::TwoAzimuthCircles;
                    }
                    else
                    {
                        srqr.shapeType = StreamSegmenter::MPDTree::OmafShapeType::FourGreatCircles;
                    }
                    Quality3d quality = codedMeta.qualityRankCoverage.get();

                    srqr.remainingArea = quality.remainingArea;
                    srqr.qualityRankingLocal = false;
                    if (quality.qualityType == 0)
                    {
                        srqr.qualityType =
                            StreamSegmenter::MPDTree::OmafQualityType::AllCorrespondTheSame;
                    }
                    else
                    {
                        srqr.qualityType = StreamSegmenter::MPDTree::OmafQualityType::MayDiffer;
                    }

                    for (auto& qInfo : quality.qualityInfo)
                    {
                        StreamSegmenter::MPDTree::OmafQualityInfo info;

                        info.qualityRanking = qInfo.qualityRank;
                        if (quality.qualityType != 0)
                        {
                            info.origHeight = qInfo.origHeight;
                            info.origWidth = qInfo.origWidth;
                        }
                        if (qInfo.sphere)
                        {
                            StreamSegmenter::MPDTree::SphereRegion sphere;
                            sphere.centreAzimuth = qInfo.sphere->cAzimuth;
                            sphere.centreElevation = qInfo.sphere->cElevation;
                            sphere.centreTilt = qInfo.sphere->cTilt;
                            sphere.azimuthRange = qInfo.sphere->rAzimuth;
                            sphere.elevationRange = qInfo.sphere->rElevation;
                            info.sphere = sphere;
                        }
                        srqr.qualityInfos.push_back(info);
                    }
                    if (aDashMPDConfig->segmenterOutput == PipelineOutputVideo::TopBottom ||
                        aDashMPDConfig->segmenterOutput == PipelineOutputVideo::SideBySide ||
                        aDashMPDConfig->segmenterOutput == PipelineOutputVideo::FramePacked ||
                        aDashMPDConfig->segmenterOutput == PipelineOutputVideo::TemporalInterleaving)
                    {
                        // we support only symmetric stereo
                        srqr.defaultViewIdc =
                            StreamSegmenter::MPDTree::OmafViewType::LeftAndRightView;
                    }
                    else if (aDashMPDConfig->segmenterOutput == PipelineOutputVideo::Left)
                    {
                        srqr.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::LeftView;
                    }
                    else if (aDashMPDConfig->segmenterOutput == PipelineOutputVideo::Right)
                    {
                        srqr.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::RightView;
                    }
                    else
                    {
                        srqr.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::Monoscopic;
                    }
                    representation.sphereRegionQualityRanks.push_back(srqr);
                }

                representation.frameRate = frameDuration.per1().minimize();
                representation.width = codedMeta.width;
                representation.height = codedMeta.height;
            }

            if (representationConfig.storeContentComponent)
            {
                switch (*representationConfig.storeContentComponent)
                {
                case StoreContentComponent::TrackId:
                {
                    StreamSegmenter::MPDTree::ContentComponent contentComponent{};
                    auto& tracks = representationConfig.output.segmenterAndSaverConfig.segmenterConfig.tracks;
                    assert(tracks.size() > 0);
                    contentComponent.id = tracks.begin()->first.get();
                    representation.contentComponent.push_back(contentComponent);
                    break;
                }
                }
            }
            representation.id = representationConfig.output.representationId;
            representation.bandwidth =
                (representationConfig.videoBitrate
                     ? static_cast<std::uint64_t>(
                           *representationConfig.videoBitrate *
                           1000)  // We generate rates that are closer to bitrate. So maxBitrate is
                                  // not appropriate at this point.
                     : static_cast<std::uint64_t>(
                           codedMeta.bitrate.avgBitrate));  // bandwidth     //We generate rates that
                                                          // are closer to bitrate. So maxBitrate is
                                                          // not appropriate at this point.

            representation.startWithSAP = 1;

            StreamSegmenter::MPDTree::SegmentTemplate segmTemplate;
            std::string mpdPath = Utils::filenameComponents(mMpdFilename).path;
            if (representationConfig.output.segmentInitSaverConfig ||
                representationConfig.output.segmenterAndSaverConfig.segmentSaverConfig)
            {
                segmTemplate.duration = representationConfig.output.segmenterAndSaverConfig
                                            .segmenterConfig.segmentDuration.num;
                segmTemplate.timescale = representationConfig.output.segmenterAndSaverConfig
                                             .segmenterConfig.segmentDuration.den;
                segmTemplate.startNumber = 1;
                if (auto& segmentInitSaverConfig = representationConfig.output.segmentInitSaverConfig)
                {
                    segmTemplate.initialization =
                        Utils::eliminateCommonPrefix(mpdPath, segmentInitSaverConfig->fileTemplate);
                }
                if (auto& segmentSaverConfig =
                        representationConfig.output.segmenterAndSaverConfig.segmentSaverConfig)
                {
                    segmTemplate.media =
                        Utils::eliminateCommonPrefix(mpdPath, segmentSaverConfig->fileTemplate);
                }
                else
                {
                    assert(false);
                }
                representation.segmentTemplate = segmTemplate;
            }
            else if (auto& singleFileSaverConfig =
                         representationConfig.output.singleFileSaverConfig)
            {
                StreamSegmenter::MPDTree::BaseURL baseURL;
                baseURL.url =
                    Utils::eliminateCommonPrefix(mpdPath, singleFileSaverConfig->filename);
                representation.baseURL = baseURL;
            }
            else
            {
                // we may not always want to save the data
                //assert(false);  // Some kind of media is required
            }
            representation.associationType = {representationConfig.associationType.begin(),
                                              representationConfig.associationType.end()};
            representation.associationId = {representationConfig.associationId.begin(),
                                            representationConfig.associationId.end()};
            if (representationConfig.allMediaAssociationViewpoint)
            {
                representation.allMediaAssociationViewpoint =
                    std::to_string(representationConfig.allMediaAssociationViewpoint->get());
            }

            adaptationSet.viewpoint = aDashMPDConfig->adaptationConfig.viewpoint;

            adaptationSet.representations.push_back(std::move(representation));

            ++representationIndex;
        }

        auto& adSetMeta =
            aAllMeta.front().front();  // most parameters like coding format, rwpk, covi
                                       // remains the same over representations and streams
        auto& adSetCodedMeta = adSetMeta.getCodedFrameMeta();

        if (adSetCodedMeta.format == CodedFormat::H265)
        {
            if (adSetCodedMeta.regionPacking)
            {
                // add RWPK flag
                adaptationSet.regionWisePacking = StreamSegmenter::MPDTree::OmafRegionWisePacking{};
                (*adaptationSet.regionWisePacking)
                    .packingType.push_back(
                        StreamSegmenter::MPDTree::OmafRwpkPackingType::Rectangular);
            }
            if (adSetCodedMeta.sphericalCoverage)
            {
                // add COVI
                StreamSegmenter::MPDTree::SphereRegion sphere;
                sphere.centreAzimuth = adSetCodedMeta.sphericalCoverage.get().cAzimuth;
                sphere.centreElevation = adSetCodedMeta.sphericalCoverage.get().cElevation;
                sphere.azimuthRange = adSetCodedMeta.sphericalCoverage.get().rAzimuth;
                sphere.elevationRange = adSetCodedMeta.sphericalCoverage.get().rElevation;
                StreamSegmenter::MPDTree::OmafContentCoverage covi;
                if (adSetCodedMeta.projection == ISOBMFF::makeOptional(OmafProjectionType::Equirectangular))
                {
                    covi.shapeType = StreamSegmenter::MPDTree::OmafShapeType::TwoAzimuthCircles;
                }
                else
                {
                    covi.shapeType = StreamSegmenter::MPDTree::OmafShapeType::FourGreatCircles;
                }

                StreamSegmenter::MPDTree::OmafCoverageInfo cc;
                cc.region = sphere;
                covi.coverageInfos.push_back(cc);
                if (aDashMPDConfig->segmenterOutput == PipelineOutputVideo::TopBottom ||
                    aDashMPDConfig->segmenterOutput == PipelineOutputVideo::SideBySide ||
                    aDashMPDConfig->segmenterOutput == PipelineOutputVideo::FramePacked)
                {
                    // we support only symmetric stereo
                    covi.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::LeftAndRightView;
                }
                else if (aDashMPDConfig->segmenterOutput == PipelineOutputVideo::Left)
                {
                    covi.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::LeftView;
                }
                else if (aDashMPDConfig->segmenterOutput == PipelineOutputVideo::Right)
                {
                    covi.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::RightView;
                }
                else
                {
                    covi.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::Monoscopic;
                }
                adaptationSet.contentCoverages.push_back(covi);
            }
        }
        // pass aDashMPDConfig->supportingAdSets if it is non-empty
        if (adSetCodedMeta.format == CodedFormat::H265 && aDashMPDConfig->baseAdaptationSetIds.size())
        {
            for (auto baseAdaptationSetId: aDashMPDConfig->baseAdaptationSetIds)
            {
                StreamSegmenter::MPDTree::PreselectionType preselection;
                preselection.tag = preselectPropertyTagForExtractorId(*adaptationSet.id);
                preselection.components.push_back(baseAdaptationSetId.get());
                preselection.components.push_back(
                    aDashMPDConfig->adaptationConfig.adaptationSetId.get());
                adaptationSet.preselection.push_back(preselection);
            }
        }
        else if (adSetCodedMeta.format == CodedFormat::H265Extractor &&
                 aDashMPDConfig->supportingAdSets)
        {
            StreamSegmenter::MPDTree::PreselectionType preselection;
            preselection.tag = preselectPropertyTagForExtractorId(
                aDashMPDConfig->adaptationConfig.adaptationSetId);
            // this is valid for extractor adaptation set only
            preselection.components.push_back(
                aDashMPDConfig->adaptationConfig.adaptationSetId.get());
            for (auto id : aDashMPDConfig->baseAdaptationSetIds)
            {
                preselection.components.push_back(id.get());
            }
            for (auto set : aDashMPDConfig->supportingAdSets.get())
            {
                preselection.components.push_back(set.get());
            }
            adaptationSet.preselection = {preselection};
            if (adSetCodedMeta.regionPacking)
            {
                // add RWPK flag
                adaptationSet.regionWisePacking = StreamSegmenter::MPDTree::OmafRegionWisePacking{};
                (*adaptationSet.regionWisePacking)
                    .packingType.push_back(
                        StreamSegmenter::MPDTree::OmafRwpkPackingType::Rectangular);
            }
        }

        std::string mime;

        if (auto mimeTypeCodecsValueTag = adSetMeta.findTag<MimeTypeCodecsTag>())
        {
            const MimeTypeCodecs& mimeTypeCodecs = mimeTypeCodecsValueTag->get();
            mime = mimeTypeCodecs.mimeType;
            codecs = mimeTypeCodecs.codecs;
        }
        else
        {
            switch (adSetCodedMeta.format)
            {
            case CodedFormat::H265:
            {
                mime = "video/mp4";
                if (adSetCodedMeta.projection)
                {
                    if (getOutputMode() == OutputMode::OMAFV2)
                    {
                        mime += " profiles='hevd,siti'";
                    }
                    else
                    {
                        if (adSetCodedMeta.sphericalCoverage)
                        {
                            mime += " profiles='hevd'";
                        }
                        else
                        {
                            mime += " profiles='hevi'";
                        }
                    }
                }
                break;
            }
            case CodedFormat::H265Extractor:
            {
                mime = "video/mp4";
                if (getOutputMode() == OutputMode::OMAFV2)
                {
                    mime += " profiles='hevd,siti'";
                }
                else
                {
                    mime += " profiles='hevd'";
                }
                break;
            }
            case CodedFormat::H264:
            {
                mime = "video/mp4";
                break;
            }
            case CodedFormat::AAC:
            {
                mime = "audio/mp4";
                break;
            }
            case CodedFormat::TimedMetadataInvo:
            {
                mime = "application/mp4";
                codecs = std::string("invo");
                break;
            }
            case CodedFormat::TimedMetadataDyol:
            {
                mime = "application/mp4";
                codecs = std::string("dyol");
                break;
            }
            case CodedFormat::TimedMetadataDyvp:
            {
                mime = "application/mp4";
                codecs = std::string("dyvp");
                break;
            }
            case CodedFormat::TimedMetadataInvp:
            {
                mime = "application/mp4";
                codecs = std::string("invp");
                break;
            }
            case CodedFormat::TimedMetadataRcvp:
            {
                mime = "application/mp4";
                codecs = std::string("rcvp");
                break;
            }
            case CodedFormat::None:
            {
                assert(false);
                break;
            }
            }
        }

        adaptationSet.mimeType = mime;

        if (codecs)
        {
            adaptationSet.codecs = codecs;
        }
        // adaptationSet.maxBandwidth;
        if (maxFrameRate)
        {
            adaptationSet.maxFramerate = maxFrameRate;
        }
        if (maxHeight)
        {
            adaptationSet.maxHeight = maxHeight;
        }
        if (maxWidth)
        {
            adaptationSet.maxWidth = maxWidth;
        }

        {
            std::unique_lock<std::mutex> lockMpdConfig(mMpdConfigMutex);
            // we have only 1 period
            mMpd.periods.front().adaptationSets.push_back(adaptationSet);
        }
    }

    std::shared_ptr<DashMPDConfig> DashOmaf::getDashMPDConfigByAdaptationsetId(StreamId aAdaptationSetId)
    {
        assert(mDashMPDConfigs.count(aAdaptationSetId));
        return mDashMPDConfigs.find(aAdaptationSetId)->second;
    }

    void DashOmaf::configureMPD(const DashMPDConfig& aDashMPDConfig)
    {
        auto dashMPDConfig = std::make_shared<DashMPDConfig>(aDashMPDConfig);

        StreamId adaptationSetId = dashMPDConfig->adaptationConfig.adaptationSetId;
        assert(adaptationSetId.get() != 0);
        assert(!mDashMPDConfigs.count(adaptationSetId));

        mDashMPDConfigs[adaptationSetId] = dashMPDConfig;

        // postpone setting up MPD until we have the SPS as some info from it is needed in video codecs-type in mpd
        auto metaCaptureCallback =
            [this, dashMPDConfig]
            (const std::tuple<std::vector<std::vector<Meta>>, Optional<TileConfiguration>> aArgs)
            {
                auto& allMeta = std::get<0>(aArgs);
                configureMPDWithMeta(dashMPDConfig, allMeta);
            };

        Future<Optional<TileConfiguration>> tileConfiguration;
        // set the tile configuration future to "no tile configuration"
        Promise<Optional<TileConfiguration>> noTileConfiguration;
        noTileConfiguration.set({});
        tileConfiguration = noTileConfiguration;

        std::list<Future<std::vector<Meta>>> metaCaptures;
        // create capture node to capture meta from each representation of each adaptation set.
        // After all have been captured, the callback above is called
        for (const auto& representation : dashMPDConfig->adaptationConfig.representations)
        {
            // in extractor representation, extractor stream id is the first one in the list, hence
            // front()
            auto metaCapture = Utils::make_unique<MetaCaptureSink>(MetaCaptureSink::Config{});
            metaCaptures.push_back(metaCapture->getMeta());
            StreamFilter streamFilter;
            if (dashMPDConfig->segmenterOutput.getClass() == PipelineClass::Video &&
                !dashMPDConfig->segmenterOutput.isVideoOverlay())
            {
                if (representation.representationConfig.output.segmenterInitConfig.streamIds
                        .empty())
                {
                    streamFilter = StreamFilter();
                }
                else
                {
                    streamFilter = StreamFilter({representation.representationConfig.output
                                                     .segmenterInitConfig.streamIds.front()});
                }
            }
            connect(*representation.encodedFrameSource,
                    *mOps.wrapForGraph("capture metadata\nfor DASH", std::move(metaCapture)), streamFilter).ASYNC_HERE;
        }
        whenAll(whenAllOfContainer(metaCaptures), tileConfiguration).then(metaCaptureCallback);
    }

    RepresentationId DashOmaf::disambiguateRepresentationId(RepresentationId aRepresentationId)
    {
        if (mRepresentationIdCount[aRepresentationId]++ == 0)
        {
            return aRepresentationId;
        }
        else
        {
            // double safety mechanism ;).
            return disambiguateRepresentationId(aRepresentationId + "." + std::to_string(mRepresentationIdCount[aRepresentationId]));
        }
    }

    std::string DashOmaf::disambiguateOutputTemplate(std::string aTemplate)
    {
        if (mOutputTemplateCount[aTemplate]++ == 0)
        {
            return aTemplate;
        }
        else
        {
            auto dot = aTemplate.rfind('.');
            if (dot != std::string::npos)
            {
                aTemplate = aTemplate.substr(0, dot) + "." + std::to_string(mOutputTemplateCount[aTemplate]) + aTemplate.substr(dot);
            }
            else
            {
                aTemplate += "." + std::to_string(mOutputTemplateCount[aTemplate]);
            }

            // double safety mechanism ;).
            return disambiguateOutputTemplate(aTemplate);
        }
    }

    DashSegmenterConfig DashOmaf::makeDashSegmenterConfig(
        const ConfigValue& aConfig, PipelineOutput aOutput, const std::string& aNamePrefix,
        DashSegmenterMetaConfig aMetaConfig)
    {
        ConfigValue segmentName = aConfig["segment_name"];
        bool omafv2Output = getOutputMode() == OutputMode::OMAFV2 && useImda(aOutput);

        auto outputNameSelector = outputNameForPipelineOutput(aOutput);
        if (segmentName->type() != Json::stringValue && !segmentName[outputNameSelector])
        {
            throw ConfigValueInvalid("Missing output " + outputNameSelector, aConfig);
        }

        std::string nameAbrSuffix = aMetaConfig.abrIndex == 0u
                                        ? std::string()
                                        : ".abr" + std::to_string(aMetaConfig.abrIndex);

        std::string viewIdComponent = aMetaConfig.viewId ? "." + std::to_string(aMetaConfig.viewId->get()): "";

        std::string templ;
        if (segmentName->type() == Json::stringValue)
        {
            templ = readString(segmentName);
            templ = Utils::replace(templ, "$Name$", aNamePrefix + viewIdComponent + "." + filenameComponentForPipelineOutput(aOutput) + nameAbrSuffix);
        }
        else
        {
            templ = readString(segmentName[outputNameSelector]);
            templ = Utils::replace(templ, "$Name$", aNamePrefix + viewIdComponent + nameAbrSuffix);
        }
        templ = Utils::replace(templ, "$Base$", getBaseName() + viewIdComponent + "." + filenameComponentForPipelineOutput(aOutput) + nameAbrSuffix);
        templ = disambiguateOutputTemplate(templ);

        DashSegmenterConfig c;
        c.viewId = aMetaConfig.viewId;

        c.segmenterName = aConfig.getName();

        if (templ.find("$Number$") != std::string::npos)
        {
            throw ConfigValueInvalid("Template cannot contain $Number$, you must use $Segment$ instead", segmentName[outputNameSelector]);
        }

        if (templ.find("$Segment$") == std::string::npos)
        {
            throw ConfigValueInvalid("Template does not contain $Segment$", segmentName[outputNameSelector]);
        }

        c.durations = aMetaConfig.dashDurations;

        c.representationId = disambiguateRepresentationId(
            (aMetaConfig.viewId ? std::to_string(aMetaConfig.viewId->get()) + "."
                                : std::string("")) +
            aNamePrefix + "." + filenameComponentForPipelineOutput(aOutput) + nameAbrSuffix);

        switch (mDashProfile)
        {
        case DashProfile::Live:
        {
            if (aOutput.getClass() != PipelineClass::IndexSegment)
            {
                Save::Config config{};
                config.fileTemplate = Utils::joinPathComponents(
                    getBaseDirectory(), Utils::replace(templ, "$Segment$", "init"));
                config.disable = aMetaConfig.disableMediaOutput;
                c.segmentInitSaverConfig = config;
            }
            break;
        }
        case DashProfile::OnDemand:
        {
            SingleFileSave::Config config {};
            config.filename = Utils::joinPathComponents(getBaseDirectory(),
                                                        Utils::replace(templ, "$Segment$", "OnDemand"));
            config.disable = aMetaConfig.disableMediaOutput;
            config.requireInitSegment = aOutput.getClass() != PipelineClass::IndexSegment;
            config.expectImdaSegment = useImda(aOutput);
            config.expectTrackRunSegment = getOutputMode() != OutputMode::OMAFV2 ||
                                           aOutput.getClass() == PipelineClass::IndexSegment;
            // note that sidx are generated for index segments by MoofCombine
            config.expectSegmentIndex = true;
            c.singleFileSaverConfig = config;

            break;
        }
        }

        StreamSegmenter::TrackMeta trackMeta{
            aMetaConfig.trackId,
            aMetaConfig.timeScale,
            aOutput.getMediaType(),  // type
            {},                      // trackType
            {}                       // dts cts offset
        };

        switch (mDashProfile)
        {
        case DashProfile::Live:
        {
            Save::Config config {};
            config.fileTemplate = Utils::joinPathComponents(
                getBaseDirectory(), Utils::replace(templ, "$Segment$", "$Number$"));
            config.disable = aMetaConfig.disableMediaOutput;
            c.segmenterAndSaverConfig.segmentSaverConfig = config;
            break;
        }
        case DashProfile::OnDemand:
        {
            // no segment saver config for live case; it uses the same file as for init segment
            break;
        }
        }

        OutputMode segmentOutputMode = !omafv2Output && getOutputMode() == OutputMode::OMAFV2
                                           ? OutputMode::OMAFV1
                                           : getOutputMode();

        c.segmenterAndSaverConfig.segmenterConfig = {};
        // non-video output gets written in OMAFV1 mode
        c.segmenterAndSaverConfig.segmenterConfig.mode = segmentOutputMode;
        // the ondemand segments get a sidx that doesn't include the part for moofs
        c.segmenterAndSaverConfig.segmenterConfig.noMetadataSidx = omafv2Output && !aOutput.isExtractor();
        c.segmenterInitConfig.mode = segmentOutputMode;
        c.segmenterInitConfig.fileMeta = aMetaConfig.fileMeta;
        c.segmenterInitConfig.duration = aMetaConfig.dashDurations.completeDuration;

        c.segmenterAndSaverConfig.segmenterConfig.segmentDuration = c.durations.segmentDuration;
        c.segmenterAndSaverConfig.segmenterConfig.subsegmentDuration = c.durations.segmentDuration / FrameDuration{ c.durations.subsegmentsPerSegment, 1 };
        // ensure that the first frame of each segment is infact an IDR frame (or throw ExpectedIDRFrame if not)
        c.segmenterAndSaverConfig.segmenterConfig.checkIDR = true;
        c.segmenterAndSaverConfig.segmenterConfig.sidx = mDashProfile == DashProfile::OnDemand;

        WriteSegmentHeader writeSegmentHeader;
        switch (getOutputMode())
        {
        case OutputMode::None:
        case OutputMode::OMAFV1:
        {
            switch (mDashProfile)
            {
            case DashProfile::Live:
            {
                writeSegmentHeader = WriteSegmentHeader::All;
                break;
            }
            case DashProfile::OnDemand:
            {
                writeSegmentHeader = WriteSegmentHeader::None;
                break;
            }
            }
            break;
        }
        case OutputMode::OMAFV2:
        {
            if (aOutput.getClass() != PipelineClass::IndexSegment && !useImda(aOutput))
            {
                switch (mDashProfile) {
                case DashProfile::Live: {
                    writeSegmentHeader = WriteSegmentHeader::All;
                    break;
                }
                case DashProfile::OnDemand: {
                    writeSegmentHeader = WriteSegmentHeader::None;
                    break;
                }
                }
            } else {
                writeSegmentHeader = WriteSegmentHeader::None;
            }
            break;
        }
        }

        c.segmenterAndSaverConfig.segmenterConfig.segmentHeader = writeSegmentHeader;

        // reintroduce segment headers in a special way to imda and index segments
        if (getOutputMode() == OutputMode::OMAFV2 &&
            mDashProfile == DashProfile::Live)
        {
            if (useImda(aOutput))
            {
                c.segmenterAndSaverConfig.segmenterConfig.frameSegmentHeader = {
                    SegmentHeader{Xtyp::Styp, "imds"}};
            }
            else if (aOutput.getClass() == PipelineClass::IndexSegment)
            {
                c.segmenterAndSaverConfig.segmenterConfig.frameSegmentHeader = {
                    SegmentHeader{Xtyp::Styp, "sibm"}};
            }
        }

        if (useImda(aOutput))
        {
            c.segmenterAndSaverConfig.segmenterConfig.acquireNextImdaId = aMetaConfig.fragmentSequenceGenerator;
        };

        c.segmenterAndSaverConfig.segmenterConfig.tracks[aMetaConfig.trackId] = trackMeta;
        if (aMetaConfig.streamId)
        {
            c.segmenterAndSaverConfig.segmenterConfig.streamIds.push_back(*aMetaConfig.streamId);
        }

        SegmenterInit::TrackConfig trackConfig{};
        trackConfig.meta = trackMeta;
        trackConfig.pipelineOutput = aOutput;
        trackConfig.overlays = aMetaConfig.overlays;
        trackConfig.trackReferences = aMetaConfig.trackReferences;
        c.segmenterInitConfig.tracks[aMetaConfig.trackId] = trackConfig;
        if (aMetaConfig.streamId)
        {
            c.segmenterInitConfig.streamIds.push_back(*aMetaConfig.streamId);
        }
        return c;
    }

    // In extractor case, we specify also subpicture video tracks for the extractor segmenterInit, but not to the extractor segmenter instance
    void DashOmaf::addAdditionalVideoTracksToExtractorInitConfig(
        TrackId aExtractorTrackId, SegmenterInit::Config& aInitConfig, StreamFilter& aStreamFilter,
        const OmafTileSets& aTileConfig, FrameDuration aTimeScale,
        const std::list<StreamId>& aAdSetIds, const PipelineOutput& aOutput)
    {
        for (auto& tile : aTileConfig)
        {
            // finds which the tile ids is used for extractor
            if (std::find(aAdSetIds.begin(), aAdSetIds.end(), tile.streamId) != aAdSetIds.end())
            {
                StreamSegmenter::TrackMeta trackMeta{
                    tile.trackId,  // trackId
                    aTimeScale,
                    StreamSegmenter::MediaType::Video,  // type
                    {},                                 // trackType
                    {}                                  // dts cts offset
                };
                SegmenterInit::TrackConfig trackConfig{};
                trackConfig.pipelineOutput = aOutput.withExtractorSet();
                trackConfig.meta = trackMeta;
                trackConfig.alte = tile.trackGroupId;

                aInitConfig.tracks[tile.trackId] = trackConfig;
                // track group ids are put into trackids for scal references
                aInitConfig.tracks.at(aExtractorTrackId)
                    .trackReferences["scal"]
                    .insert(TrackId(tile.trackGroupId.get()));
                aInitConfig.streamIds.push_back(tile.streamId);
                aStreamFilter.add(tile.streamId);
            }
        }
    }

    void DashOmaf::addBasePreselectionToAdaptationSets(const std::list<StreamId>& aAdSetIds,
                                                       StreamId aBaseAdaptationId)
    {
        for (auto id : aAdSetIds)
        {
            mDashMPDConfigs.at(id)->baseAdaptationSetIds.push_back(aBaseAdaptationId);
        }
    }

    void DashOmaf::addPartialAdaptationSetsToMultiResExtractor(const TileDirectionConfig& aDirection, std::list<StreamId>& aAdSetIds, StreamId /*aBaseAdaptationId*/)
    {
        aAdSetIds.clear();
        for (auto& column : aDirection.tiles)
        {
            for (auto& gridTile : column)
            {
                for (auto& tile : gridTile.tile)
                {
                    auto id = tile.ids.first; 
                    aAdSetIds.push_back(id);
                }
            }
        }
    }

    bool DashOmaf::useImda(PipelineOutput aPipelineOutput) const
    {
        return getOutputMode() == OutputMode::OMAFV2 &&
               aPipelineOutput.getClass() == PipelineClass::Video &&
               !aPipelineOutput.isVideoOverlay();
    }

    void DashOmaf::updateViewpointInternal(
        uint32_t aAdaptationSetId, const StreamSegmenter::MPDTree::Omaf2Viewpoint aViewpoint)
    {
        // we have only 1 period
        assert(mMpd.periods.size() == 1);
        auto& period = mMpd.periods.front();
        bool found = false;

        // Find the appropriate representation and update its MPD to include the viewpoint
        // information
        for (auto& group : period.entityGroups)
        {
            if (group.groupType == "vipo")
            {
                for (auto& entityId : group.entities)
                {
                    for (auto& adaptationSet : period.adaptationSets)
                    {
                        if (adaptationSet.id && *adaptationSet.id == entityId.adaptationSetId &&
                            *adaptationSet.id == aAdaptationSetId)
                        {
                            found = true;
                            adaptationSet.viewpoint = aViewpoint;
                        }
                    }
                }
            }
        }
        if (!found) {
            // not found this time; let's try again later
            mViewpoints[aAdaptationSetId] = aViewpoint;
        }
    }

    void DashOmaf::updateViewpoint(uint32_t aAdaptationSetId,
                                   const StreamSegmenter::MPDTree::Omaf2Viewpoint aViewpoint)
    {
        std::unique_lock<std::mutex> lockMpdConfig(mMpdConfigMutex);

        if (mMpd.periods.size() == 0)
        {
            mViewpoints[aAdaptationSetId] = aViewpoint;
        }
        else
        {
            updateViewpointInternal(aAdaptationSetId, aViewpoint);
        }
    }

    void DashOmaf::addEntityGroup(StreamSegmenter::MPDTree::EntityGroup aGroup)
    {
        std::unique_lock<std::mutex> lockMpdConfig(mMpdConfigMutex);
        // we have only 1 period
        if (mMpd.periods.size() == 0)
        {
            mEntityGroups.push_back(aGroup);
        }
        else
        {
            assert(mEntityGroups.size() == 0);
            auto& period = mMpd.periods.front();
            period.entityGroups.push_back(std::move(aGroup));
        }
    }
}  // namespace VDD
