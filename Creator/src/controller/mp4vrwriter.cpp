
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
#include "mp4vrwriter.h"

#include "common.h"
#include "configreader.h"
#include "controllerops.h"

#include "common/bitgenerator.h"
#include "common/utils.h"
#include "omaf/omafviewingorientation.h"
#include "processor/noop.h"
#include "processor/staticmetadatagenerator.h"
#include "segmenter/segmenternonfrag.h"
#include "segmenter/tagstreamidprocessor.h"
#include "segmenter/tagtrackidprocessor.h"


namespace VDD
{
    MP4VRWriter::Config MP4VRWriter::loadConfig(const ConfigValue& aConfig)
    {
        Config config{};
        config.segmentDuration =
            optionWithDefault(aConfig, "fragment_duration", readSegmentDuration, {1, 1});
        return config;
    }

    MP4VRWriter::MP4VRWriter(ControllerOps& aOps, const Config& aConfig, std::string aName,
                             PipelineMode /* aMode */)
        : mOps(aOps), mLog(aConfig.log)
    {
        mSegmenterConfig = {};
        mSegmenterConfig.segmentDuration = aConfig.segmentDuration;
        mSegmenterConfig.checkIDR = true;
        mSegmenterConfig.sidx = true;
        mFragmented = aConfig.fragmented;
        mSegmenterInitConfig = {};
        mSegmenterInitConfig.fileMeta = aConfig.fileMeta;
        mSingleFileSaveConfig = {};

        if (aName.find(".mp4") == std::string::npos)
        {
            aName.append(".mp4");
        }
        mSingleFileSaveConfig.filename = aName;
    }

    void MP4VRWriter::finalizePipeline()
    {
        assert(!mFinalized);
        mFinalized = true;

        if (mTrackSinks.empty() && mVRTrackSinks.empty())
        {
            // all done already
            return;
        }

        AsyncProcessor* segmenter = nullptr;
        AsyncProcessor* segmenterInit = nullptr;

        // update the extractor segmenter init config; identified by its scal track reference
        for (auto& [trackId, trackInitConfig] : mSegmenterInitConfig.tracks)
        {
            if (trackInitConfig.trackReferences.count("scal"))
            {
                auto extractorTrackId = trackId;
                appendScalTrafIndexMap(mSegmenterConfig.trackToScalTrafIndexMap, mSegmenterInitConfig, extractorTrackId);
            }
        }

        if (mFragmented)
        {
            segmenter = mOps.get().makeForGraph<Segmenter>("MP4VR segmenter", mSegmenterConfig);
            mSegmenterInitConfig.writeToBitstream = true;
            segmenterInit = mOps.get().makeForGraph<SegmenterInit>("MP4VR segmenter init",
                                                                   mSegmenterInitConfig);

            auto singleFileSave = mOps.get().makeForGraph<SingleFileSave>(
                "MP4VR saver\n\"" + mSingleFileSaveConfig.filename + "\"", mSingleFileSaveConfig);
            connect(*segmenterInit, *singleFileSave);
            connect(*segmenter, *singleFileSave);
        }
        else
        {
            // Non-fragmented writer takes care of writing to output stream itself, so it doesn't
            // need a filesaver node. Further, this also means the init segment must be handled by
            // the writer directly, so we can't have it as async node but have to use it as a
            // passive component.
            mSegmenterInitConfig.writeToBitstream = false;
            SegmenterNonFrag::Config config = {mSingleFileSaveConfig.filename, mSegmenterConfig,
                                               mSegmenterInitConfig};
            segmenter = mOps.get().makeForGraph<SegmenterNonFrag>("MP4VR segmenter non frag", config);
        }

        for (auto trackIdProcessor : mTrackSinks)
        {
            if (segmenterInit)
            {
                connect(*trackIdProcessor.second, *segmenterInit);
            }
            connect(*trackIdProcessor.second, *segmenter);
        }

        for (auto vrTrackStreamFilter : mVRTrackSinks)
        {
            if (segmenterInit)
            {
                connect(*vrTrackStreamFilter.first, *segmenterInit, vrTrackStreamFilter.second);
            }
            connect(*vrTrackStreamFilter.first, *segmenter, vrTrackStreamFilter.second);

            eliminate(*vrTrackStreamFilter.first);
        }
    }

    TrackId MP4VRWriter::createTrackId()
    {
        ++mTrackIdCount;
        return TrackId(mTrackIdCount);
    }

    StreamId MP4VRWriter::createStreamId()
    {
        ++mStreamIdCount;
        return StreamId(mStreamIdCount);
    }

    MP4VRWriter::Sink MP4VRWriter::createSink(
        Optional<VRVideoConfig> aVRVideoConfig, const PipelineOutput& aPipelineOutput,
        FrameDuration aTimeScale,
        const std::map<std::string, std::set<TrackId>>& aTrackReferences)
    {
        if (!mFragmented && aVRVideoConfig && aVRVideoConfig.get().mode == OutputMode::OMAFV1 &&
            aVRVideoConfig.get().ids.size() > 0)
        {
            // We have track ids, so we don't need to create any TagTrackIdProcessor, but can create
            // the segmenter directly. Fragmented case is more complicated for this, since it would
            // need a separate segmenterInit sink, and hence we can't return just the segmenter. We
            // would need e.g. NoOp processor and finalizePipeline call, but NoOp would need to
            // handle EoS differently than it does now.

            StreamFilter streamFilter;
            std::set<TrackId> trackIds;

            for (auto id : aVRVideoConfig.get().ids)
            {
                StreamSegmenter::TrackMeta trackMeta{};
                trackMeta.trackId = id.second;
                trackIds.insert(trackMeta.trackId);
                trackMeta.timescale = aTimeScale;
                trackMeta.type = StreamSegmenter::MediaType::Video;
                mSegmenterConfig.tracks.insert({id.second, trackMeta});
                mSegmenterConfig.streamIds.push_back(id.first);
                streamFilter.add(id.first);

                SegmenterInit::TrackConfig trackInitConfig{};
                trackInitConfig.meta = trackMeta;
                trackInitConfig.overlays = aVRVideoConfig->overlays;
                trackInitConfig.pipelineOutput = aPipelineOutput;
                trackInitConfig.trackReferences = aTrackReferences;
                mSegmenterInitConfig.tracks.insert({id.second, trackInitConfig});
                mSegmenterInitConfig.streamIds.push_back(id.first);
            }
            if (aVRVideoConfig.get().packedSubPictures)
            {
                mSegmenterInitConfig.packedSubPictures = true;
            }
            if (aVRVideoConfig->extractor)
            {
                // special hack for invo manipulation: just return extractor track ids in this case
                trackIds.clear();

                // add extractor track
                StreamSegmenter::TrackMeta trackMeta{};
                trackMeta.trackId = aVRVideoConfig->extractor->second;
                trackIds.insert(trackMeta.trackId);
                trackMeta.timescale = aTimeScale;
                trackMeta.type = StreamSegmenter::MediaType::Video;
                mSegmenterConfig.tracks.insert({aVRVideoConfig->extractor->second, trackMeta});
                mSegmenterConfig.streamIds.push_back(aVRVideoConfig->extractor->first);
                streamFilter.add(aVRVideoConfig->extractor->first);

                SegmenterInit::TrackConfig trackInitConfig{};
                trackInitConfig.meta = trackMeta;
                trackInitConfig.overlays = aVRVideoConfig->overlays;
                trackInitConfig.pipelineOutput = aPipelineOutput;
                for (auto id : aVRVideoConfig.get().ids)
                {
                    trackInitConfig.trackReferences["scal"].insert(id.second);
                }
                mSegmenterInitConfig.tracks.insert(
                    {aVRVideoConfig->extractor->second, std::move(trackInitConfig)});
                mSegmenterInitConfig.streamIds.push_back(aVRVideoConfig->extractor->first);
            }
            mSegmenterConfig.log = mLog;
            mSegmenterInitConfig.writeToBitstream = false;
            mSegmenterInitConfig.mode = OutputMode::OMAFV1;

            // Postpone the creation of segmenter so all the config gets in; instead provide this
            // forwarding-only proxy
            auto proxy = mOps.get().makeForGraph<NoOp>("VR Video proxy", NoOp::Config{});
            mVRTrackSinks.push_back({proxy, streamFilter});
            return {proxy, trackIds};
        }
        else
        {
            // fragmented or non-fragmented, but without trackids, so either OMAF VI, or non-OMAF

            auto trackId = createTrackId();
            auto streamId = createStreamId();

            TagTrackIdProcessor::Config tagTrackIdConfig{trackId};
            auto tagTrack = mOps.get().makeForGraph<TagTrackIdProcessor>(
                "track id:=" + Utils::to_string(trackId.get()), tagTrackIdConfig);
            TagStreamIdProcessor::Config tagStreamIdConfig{streamId};
            auto tagStream = mOps.get().makeForGraph<TagStreamIdProcessor>(
                "stream id:=" + Utils::to_string(streamId), tagStreamIdConfig);
            connect(*tagTrack, *tagStream);

            StreamSegmenter::TrackMeta trackMeta{};
            trackMeta.trackId = trackId;
            trackMeta.timescale = aTimeScale;
            trackMeta.type = aPipelineOutput.getMediaType();
            mSegmenterConfig.tracks.insert({trackId, trackMeta});
            mSegmenterConfig.streamIds.push_back(streamId);
            mSegmenterConfig.log = mLog;

            SegmenterInit::TrackConfig trackInitConfig{};
            trackInitConfig.meta = trackMeta;
            if (aVRVideoConfig)
            {
                trackInitConfig.overlays = aVRVideoConfig->overlays;
            }
            trackInitConfig.pipelineOutput = aPipelineOutput;
            trackInitConfig.trackReferences = aTrackReferences;

            mSegmenterInitConfig.tracks.insert({trackId, trackInitConfig});
            mSegmenterInitConfig.streamIds.push_back(streamId);
            if (aVRVideoConfig)
            {
                mSegmenterInitConfig.mode = aVRVideoConfig.get().mode;
            }

            mTrackSinks.insert({trackId, tagStream});

            // There was a disabled fragment for generating testing data here.

            return {tagTrack, {trackId}};
        }
    }

    void MP4VRWriter::createVRMetadataGenerator(TrackId aTrackId,
                                                VRVideoConfig /* caVRVideoConfig */,
                                                FrameDuration aTimeScale,
                                                FrameDuration aRandomAccessDuration)
    {
        // test code for initial viewing orientation
        auto metadataTrackId = createTrackId();

        StreamSegmenter::TrackMeta trackMeta{};
        trackMeta.trackId = metadataTrackId;
        trackMeta.timescale = aTimeScale;
        trackMeta.type = StreamSegmenter::MediaType::Data;
        mSegmenterConfig.tracks.insert({metadataTrackId, trackMeta});

        SegmenterInit::TrackConfig trackInitConfig{};
        trackInitConfig.meta = trackMeta;
        trackInitConfig.trackReferences["cdsc"].insert(aTrackId);
        mSegmenterInitConfig.tracks.insert({metadataTrackId, trackInitConfig});

        StaticMetadataGenerator::Config generatorConfig = {};
        generatorConfig.metadataType = CodedFormat::TimedMetadataInvo;

        InitialViewingOrientationSample sample1;
        sample1.cAzimuth = -90;
        sample1.cElevation = -30;
        sample1.cTilt = 0;
        sample1.refreshFlag = true;

        InitialViewingOrientationSample sample2;
        sample2.cAzimuth = 90;
        sample2.cElevation = 30;
        sample2.cTilt = 0;
        sample2.refreshFlag = true;

        generatorConfig.metadataSamples.push_back(sample1.toBitstream());
        generatorConfig.metadataSamples.push_back(sample2.toBitstream());
        generatorConfig.sampleDuration =
            aRandomAccessDuration;  // set GOP length as the rate, i.e. 1 metadata sample per GOP
        auto generator = mOps.get().makeForGraph<StaticMetadataGenerator>(
            "MP4VR metadata generator", generatorConfig);
        connect(*mTrackSinks.at(aTrackId), *generator);

        TagTrackIdProcessor::Config tagTrackIdConfig{metadataTrackId};
        auto tag = mOps.get().makeForGraph<TagTrackIdProcessor>(
            "track id:=" + Utils::to_string(metadataTrackId.get()), tagTrackIdConfig);

        connect(*generator, *tag);

        mTrackSinks.insert({metadataTrackId, tag});
    }

}  // namespace VDD
