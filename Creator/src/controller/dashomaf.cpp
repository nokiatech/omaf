
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
#include "dashomaf.h"

#include "common.h"
#include "controllerops.h"
#include "controllerconfigure.h"
//#include "configreader.h"

#include "async/graphbase.h"
#include "async/futureops.h"
#include "log/log.h"
#include "processor/metacapturesink.h"
#include "streamsegmenter/mpdtree.hpp"


namespace VDD
{
    struct TileConfiguration
    {
        //TODO dummy definition for fulfilling an API definition. To be removed.
    };

    DashOmaf::DashOmaf(std::shared_ptr<Log> aLog,
               const ConfigValue& aDashConfig,
               ControllerOps& aControllerOps)
        : Dash(aLog, aDashConfig, aControllerOps)
    {
    }

    DashOmaf::~DashOmaf() = default;

    void DashOmaf::setupMpd()
    {
        if (mDashConfig.valid())
        {
            mMpdFilename = readString(mDashConfig["mpd"]["filename"]);

            mMpd.type = StreamSegmenter::MPDTree::RepresentationType::Static;
            mMpd.minBufferTime = StreamSegmenter::MPDTree::Duration{ 3, 1 }; //TODO

            mMpd.projectionFormat = StreamSegmenter::MPDTree::OmafProjectionFormat{};
            mMpd.periods.push_back(StreamSegmenter::MPDTree::OmafPeriod{});

            // TODO: mSegmentSavedSignal is not currently defined for audio-only case
            for (auto& savedSignal : mSegmentSavedSignals)
            {
                auto segmenterName = savedSignal.first;
                connect(*savedSignal.second.combineSignals->getSource(), "MPD writing for \"" + segmenterName + "\"",
                    [this, segmenterName](const Views& aViews)
                {
                    writeMpd(segmenterName, aViews);
                });
            }
        }
    }

    void DashOmaf::writeMpd(DashSegmenterName aDashSegmenterName, const Views& aViews)
    {
        std::unique_lock<std::mutex> lockMpdWriting(mMpdWritingMutex);

        if (!aViews[0].isEndOfStream())
        {
            SegmentSaverSignal& saverSignal = mSegmentSavedSignals.at(aDashSegmenterName);
            auto& segmentsTotalDuration = saverSignal.segmentsTotalDuration;
            segmentsTotalDuration += aViews[0].getCodedFrameMeta().segmenterMeta.segmentDuration;

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

        std::ofstream out(mMpdFilename, std::ofstream::out | std::ofstream::binary);
        // lock adaptation set additions
        StreamSegmenter::MPDTree::OmafMPDRoot mpdConfigCopy;
        {
            std::unique_lock<std::mutex> lockMpdConfig(mMpdConfigMutex);
            mpdConfigCopy = mMpd;
        }

        mpdConfigCopy.writeXML(out);

        out.close();

        if (!aViews[0].isEndOfStream())
        {
            mLog->log(Info) << "Written MPD with duration: "
                << aViews[0].getCodedFrameMeta().segmenterMeta.segmentDuration
                << std::endl;
        }
    }

    void DashOmaf::configureMPD(PipelineOutput aSegmenterOutput, size_t aMPDViewIndex,
                            const AdaptationConfig& aAdaptationConfig,
                            Optional<FrameDuration> aVideoFrameDuration,
                            Optional<std::list<StreamId>> aSupportingAdSets)
    {
        // postpone setting up MPD until we have the SPS as some info from it is needed in video codecs-type in mpd
        auto metaCaptureCallback =
            [this, aAdaptationConfig, aMPDViewIndex, aSegmenterOutput, aVideoFrameDuration, aSupportingAdSets]
            (const std::tuple<std::vector<std::vector<CodedFrameMeta>>, Optional<TileConfiguration>> aArgs)
            {
                auto& allMeta = std::get<0>(aArgs);

                Optional<FrameRate> maxFrameRate;
                Optional<uint32_t> maxWidth;
                Optional<uint32_t> maxHeight;
                Optional<std::string> codecs;

                StreamSegmenter::MPDTree::OmafAdaptationSet adaptationSet;

                adaptationSet.id = aAdaptationConfig.adaptationSetId;
                adaptationSet.segmentAlignment = StreamSegmenter::MPDTree::BoolOrNumber{ StreamSegmenter::MPDTree::Number{ 1 } };;
                //adaptationSet.subsegmentAlignment = 

                size_t representationIndex = 0;
                for (const auto& representationInfo : aAdaptationConfig.representations)
                {
                    auto& representationConfig = representationInfo.representationConfig;
                    auto& meta = allMeta[representationIndex];

                    StreamSegmenter::MPDTree::OmafRepresentation representation;

                    for (auto track : representationConfig.output.segmenterAndSaverConfig.segmenterConfig.tracks)
                    {

                        switch (track.second.type)
                        {
                        case StreamSegmenter::MediaType::Video:
                        {
                            if (!maxFrameRate)
                            {
                                maxFrameRate = aVideoFrameDuration->per1();
                            }
                            else
                            {
                                maxFrameRate = std::max(*maxFrameRate, aVideoFrameDuration->per1());
                            }
                            maxFrameRate = maxFrameRate->minimize();

                            if (!maxWidth)
                            {
                                maxWidth = meta[0].width;
                            }
                            else
                            {
                                maxWidth = std::max(*maxWidth, meta[0].width);
                            }
                            if (!maxHeight)
                            {
                                maxHeight = meta[0].height;
                            }
                            else
                            {
                                maxHeight = std::max(*maxHeight, meta[0].height);
                            }

                            if (!codecs)
                            {
                                // Change of codec not allowed between representations in OMAF, hence no need to repeat for each representation
                                std::string codec;
                                const std::vector<uint8_t>& sps = meta[0].decoderConfig.at(VDD::ConfigType::SPS);
                                if (meta[0].format == CodedFormat::H265)
                                {
                                    if (meta[0].qualityRankCoverage)
                                    {
                                        // viewport dependent
                                        codec = "resv.podv+ercm.hvc1.";
                                    }
                                    else
                                    {
                                        // assuming viewport independent
                                        codec = "resv.podv+erpv.hvc1.";
                                    }
                                    Dash::createHevcCodecsString(sps, codec);
                                }
                                else if (meta[0].format == CodedFormat::H265Extractor)
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
                            representation.audioSamplingRate.push_back(meta[0].samplingFrequency);
                            if (!codecs)
                            {
                                // Change of codec not allowed between representations in OMAF, hence no need to repeat for each representation
                                // mp4a.40.2 from https://tools.ietf.org/html/rfc6381#section-3.3 = 40 (identifying MPEG - 4 audio), 2 (AAC - LC)
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

                    if (meta.front().format == CodedFormat::H265)
                    {
                        if (meta.front().qualityRankCoverage && meta.front().sphericalCoverage)
                        {
                            // add quality
                            StreamSegmenter::MPDTree::OmafSphereRegionWiseQuality srqr;
                            if (meta.front().projection == OmafProjectionType::EQUIRECTANGULAR)
                            {
                                srqr.shapeType = StreamSegmenter::MPDTree::OmafShapeType::TwoAzimuthCircles;
                            }
                            else
                            {
                                srqr.shapeType = StreamSegmenter::MPDTree::OmafShapeType::FourGreatCircles;
                            }
                            srqr.remainingArea = false;
                            srqr.qualityRankingLocal = false;
                            srqr.qualityType = StreamSegmenter::MPDTree::OmafQualityType::AllCorrespondTheSame;
                            StreamSegmenter::MPDTree::SphereRegion sphere;
                            sphere.centreAzimuth = meta.front().sphericalCoverage.get().cAzimuth;
                            sphere.centreElevation = meta.front().sphericalCoverage.get().cElevation;
                            sphere.azimuthRange = meta.front().sphericalCoverage.get().rAzimuth;
                            sphere.elevationRange = meta.front().sphericalCoverage.get().rElevation;

                            StreamSegmenter::MPDTree::OmafQualityInfo info;
                            info.sphere = sphere;
                            info.qualityRanking = meta.front().qualityRankCoverage.get().qualityRank;
                            srqr.qualityInfos.push_back(info);
                            srqr.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::Monoscopic;//TODO stereo support
                            representation.sphereRegionQualityRanks.push_back(srqr);
                        }

                        representation.frameRate = aVideoFrameDuration->per1().minimize();
                        representation.width = meta[0].width;
                        representation.height = meta[0].height;
                    }

                        
                    representation.id = representationConfig.output.presentationId;
                    representation.bandwidth = (representationConfig.videoBitrate
                        ? static_cast<std::uint64_t>(*representationConfig.videoBitrate * 1000)       //We generate rates that are closer to bitrate. So maxBitrate is not appropriate at this point.
                        : static_cast<std::uint64_t>(meta[0].bitrate.avgBitrate));                 // bandwidth     //We generate rates that are closer to bitrate. So maxBitrate is not appropriate at this point.

                    representation.startWithSAP = 1;

                    StreamSegmenter::MPDTree::SegmentTemplate segmTemplate;
                    segmTemplate.duration = representationConfig.output.segmenterAndSaverConfig.segmenterConfig.segmentDuration.num;
                    segmTemplate.timescale = representationConfig.output.segmenterAndSaverConfig.segmenterConfig.segmentDuration.den;
                    segmTemplate.startNumber = 1;
                    segmTemplate.initialization = Utils::filenameComponents(representationConfig.output.segmentInitSaverConfig.fileTemplate).basename;
                    segmTemplate.media = Utils::filenameComponents(representationConfig.output.segmenterAndSaverConfig.segmentSaverConfig.fileTemplate).basename;
                    representation.segmentTemplate = segmTemplate;
                    adaptationSet.representations.push_back(std::move(representation));

                    ++representationIndex;
                }

                auto& adSetmeta = allMeta.front().front();// most parameters like coding format, rwpk, covi remains the same over representations and views
                if (adSetmeta.format == CodedFormat::H265)
                {
                    if (adSetmeta.regionPacking)
                    {
                        // add RWPK flag
                        adaptationSet.regionWisePacking = StreamSegmenter::MPDTree::OmafRegionWisePacking{};
                        (*adaptationSet.regionWisePacking).packingType.push_back(StreamSegmenter::MPDTree::OmafRwpkPackingType::Rectangular);
                    }
                    if (adSetmeta.sphericalCoverage)
                    {
                        // add COVI
                        StreamSegmenter::MPDTree::SphereRegion sphere;
                        sphere.centreAzimuth = adSetmeta.sphericalCoverage.get().cAzimuth;
                        sphere.centreElevation = adSetmeta.sphericalCoverage.get().cElevation;
                        sphere.azimuthRange = adSetmeta.sphericalCoverage.get().rAzimuth;
                        sphere.elevationRange = adSetmeta.sphericalCoverage.get().rElevation;
                        StreamSegmenter::MPDTree::OmafContentCoverage covi;
                        if (adSetmeta.projection == OmafProjectionType::EQUIRECTANGULAR)
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
                        covi.defaultViewIdc = StreamSegmenter::MPDTree::OmafViewType::Monoscopic;
                        adaptationSet.contentCoverages.push_back(covi);
                    }
                }
                //pass aSupportingAdSets if it is non-empty 
                if (adSetmeta.format == CodedFormat::H265Extractor && aSupportingAdSets) 
                {
                    StreamSegmenter::MPDTree::PreselectionType preselection;
                    preselection.tag = "ext1";
                    // this is valid for extractor adaptation set only
                    preselection.components.push_back(aAdaptationConfig.adaptationSetId);
                    for (auto set : aSupportingAdSets.get())
                    {
                        preselection.components.push_back(set);
                    }
                    adaptationSet.preselection = preselection;
                    if (adSetmeta.regionPacking)
                    {
                        // add RWPK flag
                        adaptationSet.regionWisePacking = StreamSegmenter::MPDTree::OmafRegionWisePacking{};
                        (*adaptationSet.regionWisePacking).packingType.push_back(StreamSegmenter::MPDTree::OmafRwpkPackingType::Rectangular);
                    }
                }

                std::string mime;
                if (adSetmeta.format == CodedFormat::H265)
                {
                    mime = "video/mp4";
                    if (adSetmeta.sphericalCoverage)
                    {
                        mime += " profiles='hevd'";
                    }
                    else
                    {
                        // TODO is lack of quality/spherical coverity really sufficient criteria for hevi?
                        mime += " profiles='hevi'";
                    }
                }
                else if (adSetmeta.format == CodedFormat::H265Extractor)
                {
                    mime = "video/mp4 profiles='hevd'";
                }
                else if (adSetmeta.format == CodedFormat::H264)
                {
                    mime = "video/mp4";
                }
                else if (adSetmeta.format == CodedFormat::AAC)
                {
                    mime = "audio/mp4";
                }

                adaptationSet.mimeType = mime;

                if (codecs)
                {
                    adaptationSet.codecs = codecs;
                }
                //adaptationSet.maxBandwidth;
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
            };




        // TODO tileConfiguration is not really missing in OMAF case, but codedMeta (SPS, PPS). But the whenAll definition is now based on std::tuple arguments
        Future<Optional<TileConfiguration>> tileConfiguration;
        // set the tile configuration future to "no tile configuration"
        Promise<Optional<TileConfiguration>> noTileConfiguration;
        noTileConfiguration.set({});
        tileConfiguration = noTileConfiguration;

        std::list<Future<std::vector<CodedFrameMeta>>> metaCaptures;
        // create capture node to capture meta from each representation of each adaptation set. After all have been captured, the callback above is called
        for (const auto& representation : aAdaptationConfig.representations)
        {
            // in extractor representation, extractor stream id is the first one in the list, hence front()
            if (representation.representationConfig.output.segmenterInitConfig.streamIds.empty())
            {
                auto metaCapture = Utils::make_unique<MetaCaptureSink>(MetaCaptureSink::Config{ true, 0 });
                metaCaptures.push_back(metaCapture->getCodedFrameMeta());
                connect(*representation.encodedFrameSource, *mOps.wrapForGraph("capture frame size", std::move(metaCapture)));
            }
            else
            {
                auto metaCapture = Utils::make_unique<MetaCaptureSink>(MetaCaptureSink::Config{ false, representation.representationConfig.output.segmenterInitConfig.streamIds.front() });
                metaCaptures.push_back(metaCapture->getCodedFrameMeta());
                connect(*representation.encodedFrameSource, *mOps.wrapForGraph("capture frame size", std::move(metaCapture)));
            }
        }
        whenAll(whenAllOfContainer(metaCaptures), tileConfiguration).then(metaCaptureCallback);
    }

    DashSegmenterConfig DashOmaf::makeDashSegmenterConfig(const ConfigValue& aConfig, PipelineOutput aOutput, const std::string& aName,
        const SegmentDurations& aSegmentDurations,
        bool aDisableMediaOutput,
        FrameDuration aTimeScale, TrackId& aTrackId, Optional<StreamId> aStreamId)
    {
        ConfigValue segmentName = aConfig["segment_name"];

        auto outputName = outputNameForPipelineOutput(aOutput);
        if (segmentName->type() != Json::stringValue && !segmentName[outputName].valid())
        {
            throw ConfigValueInvalid("Missing output " + outputName, aConfig);
        }

        std::string templ;
        if (segmentName->type() == Json::stringValue)
        {
            templ = readString(segmentName);
            templ = Utils::replace(templ, "$Name$", aName + "." + filenameComponentForPipelineOutput(aOutput));
        }
        else
        {
            templ = readString(segmentName[outputName]);
            templ = Utils::replace(templ, "$Name$", aName);
        }

        DashSegmenterConfig c;

        c.segmenterName = aConfig.getName();

        if (templ.find("$Number$") != std::string::npos)
        {
            throw ConfigValueInvalid("Template cannot contain $Number$, you must use $Segment$ instead", segmentName[outputName]);
        }

        if (templ.find("$Segment$") == std::string::npos)
        {
            throw ConfigValueInvalid("Template does not contain $Segment$", segmentName[outputName]);
        }

        c.durations = aSegmentDurations;

        c.presentationId = aName + "." + filenameComponentForPipelineOutput(aOutput);
        c.segmentInitSaverConfig.fileTemplate = Utils::replace(templ, "$Segment$", "init");
        c.segmentInitSaverConfig.disable = aDisableMediaOutput;

        StreamSegmenter::TrackMeta trackMeta{
            aTrackId,
            aTimeScale,
            aOutput == PipelineOutput::Audio ? StreamSegmenter::MediaType::Audio :
            StreamSegmenter::MediaType::Video // type
        };

        c.segmenterAndSaverConfig.segmentSaverConfig.fileTemplate = Utils::replace(templ, "$Segment$", "$Number$");
        c.segmenterAndSaverConfig.segmentSaverConfig.disable = aDisableMediaOutput;
        c.segmenterAndSaverConfig.segmenterConfig = {};
        c.segmenterAndSaverConfig.segmenterConfig.segmentDuration = c.durations.segmentDuration;
        c.segmenterAndSaverConfig.segmenterConfig.subsegmentDuration = c.durations.segmentDuration / FrameDuration{ c.durations.subsegmentsPerSegment, 1 };
        // ensure that the first frame of each segment is infact an IDR frame (or throw ExpectedIDRFrame if not)
        c.segmenterAndSaverConfig.segmenterConfig.checkIDR = true;
        c.segmenterAndSaverConfig.segmenterConfig.tracks[aTrackId] = trackMeta;
        if (aStreamId)
        {
            c.segmenterAndSaverConfig.segmenterConfig.streamIds.push_back(*aStreamId);
        }

        SegmenterInit::TrackConfig trackConfig{};
        trackConfig.meta = trackMeta;
        c.segmenterInitConfig.tracks[aTrackId] = trackConfig;
        c.segmenterInitConfig.mode = OperatingMode::OMAF;
        if (aStreamId)
        {
            c.segmenterInitConfig.streamIds.push_back(*aStreamId);
        }
        return c;
    }

    // In extractor case, we specify also subpicture video tracks for the extractor segmenterInit, but not to the extractor segmenter instance
    void DashOmaf::addAdditionalVideoTracksToInitConfig(TrackId aExtractorTrackId, SegmenterInit::Config& aInitConfig, const TileFilter::OmafTileSets& aTileConfig, FrameDuration aTimeScale)
    {
        for (auto& tile : aTileConfig)
        {
            StreamSegmenter::TrackMeta trackMeta{
                tile.trackId, // trackId
                aTimeScale,
                StreamSegmenter::MediaType::Video // type
            };
            SegmenterInit::TrackConfig trackConfig{};
            trackConfig.meta = trackMeta;

            aInitConfig.tracks[tile.trackId] = trackConfig;
            aInitConfig.tracks.at(aExtractorTrackId).trackReferences["scal"].insert(tile.trackId);
            aInitConfig.streamIds.push_back(tile.streamId);
        }
    }
}
