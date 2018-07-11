
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
#pragma once

#include <fstream>
#include <map>
#include <set>

#include <streamsegmenter/segmenterapi.hpp>
#include <streamsegmenter/track.hpp>

#include "metadata.h"
#include "controller/common.h"

#include "common/optional.h"
#include "common/exceptions.h"
#include "processor/processor.h"

namespace VDD
{
    class UnsupportedCodecException : public Exception
    {
    public:
        UnsupportedCodecException();
    };

    enum class OperatingMode
    {
        None,
        OMAF
    };

    class SegmenterInit : public Processor
    {
    public:
        struct TrackConfig
        {
            StreamSegmenter::TrackMeta meta;
            std::map<std::string, std::set<TrackId>> trackReferences;
            PipelineOutput pipelineOutput;
        };

        struct Config
        {
            /** Provide information for each track. In particular, the
             * (output) track id and time scale of the track.  An entry
             * for each input track must exist here, or otherwise
             * InvalidTrack will be thrown */
            std::map<TrackId, TrackConfig> tracks;

            bool fragmented = true;

            bool writeToBitstream = true;

            /** Needed to collect metadata from other tracks before creating the extractor track
             */
            bool packedSubPictures = false;  //TODO or some better way?

            OperatingMode mode = OperatingMode::OMAF;

            std::list<StreamId> streamIds;// stream ids is just for gatekeeping; what streams to process and what to skip. No need to have exact mapping to tracks
        };

        SegmenterInit(Config aConfig);
        ~SegmenterInit() override;

        /** SegmenterInit requires the data to be available in CPU memory */
        StorageType getPreferredStorageType() const override;

        std::vector<Views> process(const Views& data) override;

        /** Prepare init segment, used only in case we don't use this class as asyncnode but as a passive component,
         *  serving the main mp4 producer node (e.g. non-fragmented output)
         */
        Optional<StreamSegmenter::Segmenter::InitSegment> prepareInitSegment();

    private:
        /** Information about the tracks, in particular the sample types */
        StreamSegmenter::Segmenter::TrackDescriptions mTrackDescriptions;

        /** Are we next processing the first frame of the track? */
        std::set<TrackId> mFirstFrameRemaining;

        /** Configuration */
        const Config mConfig;

        std::string mOmafVideoTrackBrand = "";
        std::string mOmafAudioTrackBrand = "";

    private: 
        void addH264VideoTrack(TrackId aTrackId, CodedFrameMeta& aMeta);
        void addH265VideoTrack(TrackId aTrackId, CodedFrameMeta& aMeta);
        void addAACTrack(TrackId aTrackId, CodedFrameMeta& aMeta, const TrackConfig& aTrackConfig);
        void addTimeMetadataTrack(TrackId aTrackId, const CodedFrameMeta& aMeta);
        void addH265ExtractorTrack(TrackId aTrackId, CodedFrameMeta& aMeta);
        StreamSegmenter::Segmenter::InitSegment makeInitSegment(bool aFragmented);

        void fillOmafStructures(TrackId aTrackId, CodedFrameMeta& aMeta, StreamSegmenter::Segmenter::HevcVideoSampleEntry& aSampleEntry, StreamSegmenter::TrackMeta& aTrackMeta);
    };
}
