
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
#pragma once

#include <fstream>
#include <map>
#include <set>

#include <streamsegmenter/segmenterapi.hpp>
#include <streamsegmenter/track.hpp>

#include "async/future.h"
#include "controller/common.h"
#include "metadata.h"
#include "segmentercommon.h"

#include "common/exceptions.h"
#include "common/optional.h"
#include "processor/processor.h"

namespace VDD
{
    class UnsupportedCodecException : public Exception
    {
    public:
        UnsupportedCodecException();
    };

    // Use a particular sample entry
    using SampleEntryTag = ValueTag<std::shared_ptr<StreamSegmenter::Segmenter::SampleEntry>>;

    class SegmenterInit : public Processor
    {
    public:
        struct TrackConfig
        {
            StreamSegmenter::TrackMeta meta;
            std::map<std::string, std::set<TrackId>> trackReferences;
            PipelineOutput pipelineOutput;
            Optional<ISOBMFF::OverlayStruct> overlays;
            Optional<TrackGroupId> alte;
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

            /** Needed to collect metadata from other tracks before creating the
             * extractor track
             */
            bool packedSubPictures = false;

            OutputMode mode = OutputMode::OMAFV1;

            std::list<StreamId> streamIds;  // stream ids is just for gatekeeping; what
                                            // streams to process and what to skip. No
                                            // need to have exact mapping to tracks

            // Promise as this information is available later
            Optional<Future<Optional<StreamSegmenter::Segmenter::MetaSpec>>> fileMeta;

            // if we know the media duration up front, we can write it to the header
            Optional<StreamSegmenter::RatU64> duration;

            // What to stick in front of frame segments. NOTE: dropping header is not supported by
            // mp4vr, so before that support is added, this will result in duplicate header.
            Optional<SegmentHeader> frameSegmentHeader;
        };

        SegmenterInit(Config aConfig);
        ~SegmenterInit() override;

        /** SegmenterInit requires the data to be available in CPU memory */
        StorageType getPreferredStorageType() const override;

        std::vector<Streams> process(const Streams &data) override;

        /** Prepare init segment, used only in case we don't use this class as
         * asyncnode but as a passive component,
         *  serving the main mp4 producer node (e.g. non-fragmented output)
         */
        Optional<StreamSegmenter::Segmenter::InitSegment> prepareInitSegment();

        std::string getGraphVizDescription() override;

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
        void addH264VideoTrack(TrackId aTrackId, const Meta& aMeta);
        void addH265VideoTrack(TrackId aTrackId, const Meta& aMeta);
        void addAACTrack(TrackId aTrackId, const Meta& aMeta, const TrackConfig &aTrackConfig);
        // TOOD: merge addInvoTimeMetadataTrack with addGenericTimedMetadataTrack; might even work
        // now as-is if InitialViewingOrientationSampleEntry is property set
        void addInvoTimeMetadataTrack(TrackId aTrackId, const Meta& aMeta);
        void addGenericTimedMetadataTrack(TrackId aTrackId, const Meta& aMeta);
        void addH265ExtractorTrack(TrackId aTrackId, const Meta& aMeta);
        StreamSegmenter::Segmenter::InitSegment makeInitSegment(bool aFragmented);

        void fillOmafStructures(TrackId aTrackId, const CodedFrameMeta &aMeta,
                                StreamSegmenter::Segmenter::HevcVideoSampleEntry &aSampleEntry,
                                StreamSegmenter::TrackMeta &aTrackMeta);

        void updateTrackDescription(StreamSegmenter::Segmenter::TrackDescription& aTrackDescription,
                                    const SegmenterInit::TrackConfig& aTrackConfig,
                                    const Meta&) const;
    };

    void appendScalTrafIndexMap(TrackToScalTrafIndexMap& aMap,
                                const SegmenterInit::Config& aSegmenterInitConfig,
                                TrackId aExtractorTrackId);

}  // namespace VDD
