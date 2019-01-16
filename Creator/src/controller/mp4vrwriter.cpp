
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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
#include "controllerops.h"
#include "configreader.h"

#include "common/bitgenerator.h"
#include "common/utils.h"
#include "segmenter/tagtrackidprocessor.h"
#include "segmenter/segmenternonfrag.h"
#include "processor/staticmetadatagenerator.h"
#include "omaf/omafviewingorientation.h"


namespace VDD
{

    MP4VRWriter::Config MP4VRWriter::loadConfig(const ConfigValue& aConfig)
    {
        Config config {};
        config.segmentDuration = optionWithDefault(aConfig, "fragment_duration", "Duration of a single fragment", readSegmentDuration, { 1, 1 });
        return config;
    }

    MP4VRWriter::MP4VRWriter(ControllerOps& aOps, const Config& aConfig, std::string aName, PipelineMode aMode)
        : mOps(aOps)
        , mLog(aConfig.log)
    {
        mSegmenterConfig = {};
        mSegmenterConfig.segmentDuration = aConfig.segmentDuration;
        mSegmenterConfig.checkIDR = true;
        mSegmenterConfig.separateSidx = true;
        mFragmented = aConfig.fragmented;
        mSegmenterInitConfig = {};
        mSingleFileSaveConfig = {};

        if (aName.find(".mp4") == std::string::npos)
        {
            aName.append(".mp4");
        }
        mSingleFileSaveConfig.filename = aName;
    }

    void MP4VRWriter::finalizePipeline()
    {
        if (mTrackSinks.empty())
        {
            // all done already
            return;
        }
        if (mFragmented)
        {
            auto segmenter = mOps.get().makeForGraph<Segmenter>("MP4VR segmenter", mSegmenterConfig);
            mSegmenterInitConfig.writeToBitstream = true;
            auto segmenterInit = mOps.get().makeForGraph<SegmenterInit>("MP4VR segmenter init", mSegmenterInitConfig);

            for (auto trackIdProcessor : mTrackSinks)
            {
                connect(*trackIdProcessor.second, *segmenterInit);
                connect(*trackIdProcessor.second, *segmenter);
            }

            auto singleFileSave = mOps.get().makeForGraph<SingleFileSave>("MP4VR saver\n\"" + mSingleFileSaveConfig.filename + "\"", mSingleFileSaveConfig);
            connect(*segmenterInit, *singleFileSave);
            connect(*segmenter, *singleFileSave);
        }
        else
        {
            // Non-fragmented writer takes care of writing to output stream itself, so it doesn't need a filesaver node.
            // Further, this also means the init segment must be handled by the writer directly, so we can't have it as async node
            // but have to use it as a passive component.
            mSegmenterInitConfig.writeToBitstream = false;
            SegmenterNonFrag::Config config = { mSingleFileSaveConfig.filename, mSegmenterConfig, mSegmenterInitConfig };
            auto segmenter = mOps.get().makeForGraph<SegmenterNonFrag>("MP4VR segmenter", config);
            for (auto trackIdProcessor : mTrackSinks)
            {
                connect(*trackIdProcessor.second, *segmenter);
            }
        }
    }

    TrackId MP4VRWriter::createTrackId()
    {
        ++mTrackCount;
        return TrackId(mTrackCount);
    }

    AsyncProcessor* MP4VRWriter::createSink(Optional<VRVideoConfig> aVRVideoConfig,
                                            PipelineOutput aPipelineOutput,
                                            FrameDuration aTimeScale, FrameDuration aRandomAccessDuration)
    {
        if (!mFragmented && aVRVideoConfig && aVRVideoConfig.get().mode == OperatingMode::OMAF && aVRVideoConfig.get().ids.size() > 0)
        {
            // We have track ids, so we don't need to create any TagTrackIdProcessor, but can create the segmenter directly.
            // Fragmented case is more complicated for this, since it would need a separate segmenterInit sink, and hence we can't return just the segmenter. 
            // We would need e.g. NoOp processor and finalizePipeline call, but NoOp would need to handle EoS differently than it does now.

            for (auto id : aVRVideoConfig.get().ids)
            {
                StreamSegmenter::TrackMeta trackMeta{};
                trackMeta.trackId = id.second;
                trackMeta.timescale = aTimeScale;
                trackMeta.type = StreamSegmenter::MediaType::Video;
                mSegmenterConfig.tracks.insert({ id.second, trackMeta });
                mSegmenterConfig.streamIds.push_back(id.first);

                SegmenterInit::TrackConfig trackInitConfig{};
                trackInitConfig.meta = trackMeta;
                mSegmenterInitConfig.tracks.insert({ id.second, trackInitConfig });
                mSegmenterInitConfig.streamIds.push_back(id.first);
            }
            if (aVRVideoConfig.get().packedSubPictures)
            {
                mSegmenterInitConfig.packedSubPictures = true;
            }
            if (aVRVideoConfig->extractor)
            {
                // add extractor track
                StreamSegmenter::TrackMeta trackMeta{};
                trackMeta.trackId = aVRVideoConfig->extractor->second;
                trackMeta.timescale = aTimeScale;
                trackMeta.type = StreamSegmenter::MediaType::Video;
                mSegmenterConfig.tracks.insert({ aVRVideoConfig->extractor->second, trackMeta });
                mSegmenterConfig.streamIds.push_back(aVRVideoConfig->extractor->first);

                SegmenterInit::TrackConfig trackInitConfig{};
                trackInitConfig.meta = trackMeta;
                trackInitConfig.pipelineOutput = aPipelineOutput;
                for (auto id : aVRVideoConfig.get().ids)
                {
                    trackInitConfig.trackReferences["scal"].insert(id.second);
                }
                mSegmenterInitConfig.tracks.insert({ aVRVideoConfig->extractor->second, std::move(trackInitConfig) });
                mSegmenterInitConfig.streamIds.push_back(aVRVideoConfig->extractor->first);

            }
            mSegmenterConfig.log = mLog;
            mSegmenterInitConfig.writeToBitstream = false;
            mSegmenterInitConfig.mode = OperatingMode::OMAF;
            SegmenterNonFrag::Config config = { mSingleFileSaveConfig.filename, mSegmenterConfig, mSegmenterInitConfig };
            auto segmenter = mOps.get().makeForGraph<SegmenterNonFrag>("MP4VR segmenter", config);
            return segmenter;
        }
        else
        {
            // fragmented or non-fragmented, but without trackids, so either OMAF VI, or non-OMAF

            auto trackId = createTrackId();

            TagTrackIdProcessor::Config tagTrackIdConfig{ trackId };
            auto tag =
                mOps.get().makeForGraph<TagTrackIdProcessor>(
                    "track id:=" + Utils::to_string(trackId.get()),
                    tagTrackIdConfig);

            StreamSegmenter::TrackMeta trackMeta{};
            trackMeta.trackId = trackId;
            trackMeta.timescale = aTimeScale;
            trackMeta.type = aVRVideoConfig ? StreamSegmenter::MediaType::Video : StreamSegmenter::MediaType::Audio;
            mSegmenterConfig.tracks.insert({ trackId, trackMeta });
            mSegmenterConfig.log = mLog;

            SegmenterInit::TrackConfig trackInitConfig{};
            trackInitConfig.meta = trackMeta;
            trackInitConfig.pipelineOutput = aPipelineOutput;

            mSegmenterInitConfig.tracks.insert({ trackId, trackInitConfig });
            if (aVRVideoConfig)
            {
                mSegmenterInitConfig.mode = aVRVideoConfig.get().mode;
            }

            mTrackSinks.insert({ trackId, tag });

            // for testing only - to generate e.g. fake initial viewing orientations
            if (aVRVideoConfig && false)
            {
                createVRMetadataGenerator(trackId, *aVRVideoConfig, aTimeScale, aRandomAccessDuration);
            }

            return tag;
        }

    }

    void MP4VRWriter::createVRMetadataGenerator(TrackId aTrackId, VRVideoConfig aVRVideoConfig, FrameDuration aTimeScale, FrameDuration aRandomAccessDuration)
    {
        // test code for initial viewing orientation
        auto metadataTrackId = createTrackId();

        StreamSegmenter::TrackMeta trackMeta{};
        trackMeta.trackId = metadataTrackId;
        trackMeta.timescale = aTimeScale;
        trackMeta.type = StreamSegmenter::MediaType::Data;
        mSegmenterConfig.tracks.insert({ metadataTrackId, trackMeta });

        SegmenterInit::TrackConfig trackInitConfig{};
        trackInitConfig.meta = trackMeta;
        trackInitConfig.trackReferences["cdsc"].insert(aTrackId);
        mSegmenterInitConfig.tracks.insert({ metadataTrackId, trackInitConfig });

        StaticMetadataGenerator::Config generatorConfig = {};
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
        generatorConfig.mediaToMetaSampleRate = aRandomAccessDuration.num;//set GOP length as the rate, i.e. 1 metadata sample per GOP
        auto generator = mOps.get().makeForGraph<StaticMetadataGenerator>("MP4VR metadata generator", generatorConfig);
        connect(*mTrackSinks.at(aTrackId), *generator);

        TagTrackIdProcessor::Config tagTrackIdConfig{ metadataTrackId };
        auto tag =
            mOps.get().makeForGraph<TagTrackIdProcessor>(
                "track id:=" + Utils::to_string(metadataTrackId.get()),
                tagTrackIdConfig);

        connect(*generator, *tag);

        mTrackSinks.insert({ metadataTrackId, tag });
    }

}
