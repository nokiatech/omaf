
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
#include <fstream>
#include <iostream>
#include <memory>
#include "api/streamsegmenter/autosegmenter.hpp"
#include "api/streamsegmenter/segmenterapi.hpp"
#include "config.hpp"
#include "moviebox.hpp"
#include "mp4access.hpp"
#include "utils.hpp"

#include "json.hh"

#if 0

namespace
{
    struct SegmentWriterDestructor
    {
        void operator()(StreamSegmenter::Writer* aWriter)
        {
            StreamSegmenter::Writer::destruct(aWriter);
        }
    };

    struct Processor
    {
        StreamSegmenter::MPD::AdaptationSet adaptationSet;
        std::unique_ptr<StreamSegmenter::AutoSegmenter> autoSegmenter;
        StreamSegmenter::Segmenter::InitSegment initSegment;

        Processor() = default;
        Processor(StreamSegmenter::MPD::AdaptationSet&& aAdaptationSet,
                  std::unique_ptr<StreamSegmenter::AutoSegmenter>&& aAutoSegmenter,
                  StreamSegmenter::Segmenter::InitSegment&& aInitSegment)
            : adaptationSet(std::move(aAdaptationSet))
            , autoSegmenter(std::move(aAutoSegmenter))
            , initSegment(std::move(aInitSegment))
        {
            // nothing
        }

        Processor(Processor&& aOther)
            : adaptationSet(std::move(aOther.adaptationSet))
            , autoSegmenter(std::move(aOther.autoSegmenter))
            , initSegment(std::move(aOther.initSegment))
        {
            // nothing
        }

        const Processor& operator=(const Processor&) = delete;
    };

    void autoSegmenter(const StreamSegmenter::Config::Config& config)
    {
        using namespace StreamSegmenter;

        std::ifstream mp4stream(config.input.file);
        StreamSegmenter::MP4Access mp4(mp4stream);

        ::MovieBox moov;
        mp4.parseBox(moov);
        Segmenter::TrackDescriptions allTrackDescriptions;

        {
            using namespace Segmenter;
            TrackMeta trackMeta{};
            MediaDescription mediaDescription;
            AvcVideoSampleEntry sampleEntry;

            trackMeta.trackId   = 1;
            trackMeta.timescale = config.timescale;

            sampleEntry.width  = config.width;
            sampleEntry.height = config.height;

            sampleEntry.sps = {0x67, 0x64, 0x00, 0x1F, 0xAC, 0xD9, 0x40, 0x50, 0x05, 0xBB, 0x01, 0x10, 0x00,
                               0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x03, 0x03, 0x20, 0xF1, 0x83, 0x19, 0x60};
            sampleEntry.pps = {0x68, 0xEB, 0xE3, 0xCB, 0x22, 0xC0};
            allTrackDescriptions.insert(
                std::make_pair(trackMeta.trackId, TrackDescription(trackMeta, mediaDescription, sampleEntry)));
        }

        const auto& trackBoxes = moov.getTrackBoxes();

        std::map<TrackId, Processor> processors;

        auto writeMPD = [&]() {
            MPD::Config mpd{
                MPD::RepresentationType::Static,  // type
                Utils::none,                      // availabilityStartTime
                Utils::none,                      // publishTime
                Utils::none,                      // minimumUpdatePeriod
                Segmenter::Duration(),            // minBufferTime
                Segmenter::Duration(),            // mediaPresentationDuration
                Utils::none,                      // start
                Segmenter::Duration(),            // duration
                {}                                // adaptationSets
            };

            for (auto& processor : processors)
            {
                mpd.adaptationSets.push_back(processor.second.adaptationSet);
            }

            {
                std::ofstream out(config.mpd.outputMPD, std::ofstream::out | std::ofstream::binary);
                MPD::writeMPD(out, mpd);
            }
        };

        std::unique_ptr<StreamSegmenter::Writer, SegmentWriterDestructor> writer;
        writer.reset(StreamSegmenter::Writer::create());

        Segmenter::SequenceId sequence = 0;
        auto writeSegments = [&](MPD::AdaptationSet& aAdaptationSet, const Segmenter::InitSegment& aInitSegment,
                                 const std::list<Segmenter::Segments>& aSegments) {
            if (aSegments.size())
            {
                for (const std::list<Segmenter::Segment>& subsegments : aSegments)
                {
                    auto representation   = aAdaptationSet.representations.front();
                    auto& firstSubsegment = subsegments.front();
                    auto& lastSubsegment  = subsegments.back();
                    FrameTime t0          = firstSubsegment.t0.minimize();
                    FrameTime t1 =
                        (lastSubsegment.t0 + lastSubsegment.duration.cast<decltype(lastSubsegment.t0)>()).minimize();
                    shareDenominator(t0, t1);
                    std::cout << "Writing subsegments from time " << t0 << " .. " << t1 << " = " << t1 - t0 << " ("
                              << (t1 - t0).asDouble() << ")" << std::endl;
                    {
                        std::ofstream out(
                            MPD::applyTemplate(representation.segmentTemplate.segmentTemplate, {sequence}),
                            std::ofstream::out | std::ofstream::binary);
                        ++sequence;
                        writer->writeSubsegments(out, subsegments);
                    }

                    MPD::Representation curRepresentation{
                        representation.representationId,
                        makeSubRepIdInfo(trackBoxes, MPD::maxFrameRateOfSegments(subsegments).get()),  // initSegment
                        subsegments,                                                                   // segment
                        1000000,                                                                       // bandwidth
                        representation.segmentTemplate,  // segmentTemplate
                        {}};
                    aAdaptationSet.representations.clear();
                    aAdaptationSet.representations.push_back(curRepresentation);
                    {
                        std::ofstream out(representation.segmentTemplate.initSegment,
                                          std::ofstream::out | std::ofstream::binary);
                        Segmenter::writeInitSegment(out, aInitSegment);
                    }
                }
                writeMPD();
            }
        };

        for (auto trackBox : trackBoxes)
        {
            const auto& trackHeader = trackBox->getTrackHeaderBox();
            TrackId trackId         = trackHeader.getTrackID();
            Track track(mp4, *trackBox, moov.getMovieHeaderBox().getTimeScale());

            Segmenter::TrackDescriptions trackDescriptions;
            Segmenter::TrackDescriptions::iterator trackDescriptionsIt = allTrackDescriptions.find(trackId);
            assert(trackDescriptionsIt != allTrackDescriptions.end());
            trackDescriptions.insert(std::make_pair(trackId, std::move(trackDescriptionsIt->second)));
            allTrackDescriptions.erase(trackDescriptionsIt);

            Segmenter::MovieDescription movieDescription;
            movieDescription.matrix            = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            Segmenter::InitSegment initSegment = makeInitSegment(trackDescriptions, movieDescription, true);

            MPD::SegmentTemplate segmentTemplate{
                config.segmentDuration,                                               // segmentDuration
                "out-init-" + Utils::to_string(trackId.get()) + ".mp4",               // initSegment
                "track-" + Utils::to_string(trackId.get()) + "-segment-$Number$.m4s"  // segmentTemplate
            };
            MPD::Representation representation{Utils::to_string(trackId.get()),                 // representationId
                                               makeSubRepIdInfo(trackBoxes, FrameRate(30, 1)),  // initSegment
                                               std::list<Segmenter::Segment>(),                 // segment
                                               1000000,                                         // bandwidth
                                               segmentTemplate,                                 // segmentTemplate
                                               {}};
            MPD::AdaptationSet adaptationSet{
                Utils::none,                 // RoleStereoId
                Utils::none,                 // VideoProfileType
                Utils::none,                 // VideoTileViewport
                Utils::none,                 // VideoFramePacking
                FrameRate(),                 // maxFrameRate
                {std::move(representation)}  // representations
            };

            AutoSegmenterConfig autoSegmenterConfig;
            autoSegmenterConfig.checkIDR        = trackId == TrackId(1);
            autoSegmenterConfig.segmentDuration = representation.segmentTemplate.duration;

            Processor processor{std::move(adaptationSet), Utils::make_unique<AutoSegmenter>(autoSegmenterConfig),
                                std::move(initSegment)};

            processors.insert(std::make_pair(trackId, std::move(processor)));

            processors[trackId].autoSegmenter->addTrack(trackId, track.getTrackMeta());
        }

        for (auto trackBox : trackBoxes)
        {
            const auto& trackHeader = trackBox->getTrackHeaderBox();
            TrackId trackId         = trackHeader.getTrackID();
            Track track(mp4, *trackBox, moov.getMovieHeaderBox().getTimeScale());
            Processor& processor = processors[trackId];
            std::cout << "Processing track " << trackId << std::endl;
            for (const auto& frame : track.getFrames())
            {
                switch (processor.autoSegmenter->feed(trackId, frame))
                {
                case AutoSegmenter::Action::KeepFeeding:
                {
                    // ok, keep feeding
                }
                break;

                case AutoSegmenter::Action::ExtractSegment:
                {
                    writeSegments(processor.adaptationSet, processor.initSegment,
                                  processor.autoSegmenter->extractSegmentsWithSubsegments());
                }
                break;
                }
            }
            processor.autoSegmenter->feedEnd(trackId);
            writeSegments(processor.adaptationSet, processor.initSegment,
                          processor.autoSegmenter->extractSegmentsWithSubsegments());
        }
    }

}  // anonymous namespace

#endif

int main(int aC, char** aV)
{
    using namespace StreamSegmenter;

    std::unique_ptr<std::ifstream> fileStream;
    if (aC > 1)
    {
        fileStream.reset(new std::ifstream(aV[1]));
    }

#if 1
    std::ifstream configStream(aV[1]);
    if (!configStream)
    {
        throw ConfigError("Cannot load configuration file");
    }

    Json::Reader reader;
    Json::Value rootJson;
    if (!reader.parse(configStream, rootJson))
    {
        throw ConfigError("Unable to parse configuration:" + reader.getFormattedErrorMessages());
    }
    Config::Config config = Config::load(rootJson);

//    autoSegmenter(config);
#endif

#if 0
    ::MovieBox moov;
    mp4.parseBox(moov);

    for (auto track: moov.getTrackBoxes()) {
        MediaBox& mediaBox = track->getMediaBox();
        MediaInformationBox& mediaInformationBox = mediaBox.getMediaInformationBox();
        SampleTableBox& sampleTableBox = mediaInformationBox.getSampleTableBox();
        SampleDescriptionBox& sampleDescriptionBox = sampleTableBox.getSampleDescriptionBox();
        std::cout << "Track: " << track->getTrackHeaderBox().getTrackID();
        std::cout << std::endl << " stsd: ";
        dumpBox(std::cout, sampleDescriptionBox);
        std::cout << std::endl;
    }
#endif
}
