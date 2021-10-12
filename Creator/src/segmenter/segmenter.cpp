
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
#include "segmenter.h"
#include <algorithm>
#include <streamsegmenter/segmenterapi.hpp>
#include "common/optional.h"
#include "common/utils.h"

namespace VDD
{
    namespace
    {
        class DataFrameAcquire : public StreamSegmenter::AcquireFrameData
        {
        public:
            DataFrameAcquire(Data aData);
            ~DataFrameAcquire() override;

            StreamSegmenter::FrameData get() const override;
            size_t getSize() const override;
            DataFrameAcquire* clone() const override;

        private:
            Data mData;
        };

        DataFrameAcquire::DataFrameAcquire(Data aData) : mData(aData)
        {
            // nothing
        }

        DataFrameAcquire::~DataFrameAcquire()
        {
            // nothing
        }

        StreamSegmenter::FrameData DataFrameAcquire::get() const
        {
            auto& data = mData.getCPUDataReference();
            StreamSegmenter::FrameData frameData(
                static_cast<const std::uint8_t*>(data.address[0]),
                static_cast<const std::uint8_t*>(data.address[0]) + data.size[0]);
            return frameData;
        }

        size_t DataFrameAcquire::getSize() const
        {
            auto& data = mData.getCPUDataReference();
            return data.size[0];
        }

        DataFrameAcquire* DataFrameAcquire::clone() const
        {
            return new DataFrameAcquire(mData);
        }

        Data dataOfStream(std::ostringstream& aStream, const Meta& aMeta, StreamId aStreamId)
        {
            auto str = aStream.str();
            std::vector<std::vector<uint8_t>> dataVec{ { str.begin(), str.end() } };
            CPUDataVector dataRef(std::move(dataVec));
            return Data(std::move(dataRef), aMeta, aStreamId);
        }
    }  // anonymous namespace

    const StreamId Segmenter::cSingleStreamId = newStreamId("Segmenter::cSingleStreamId");
    const StreamId Segmenter::cImdaStreamId = newStreamId("Segmenter::cImdaStreamId");
    const StreamId Segmenter::cTrackRunStreamId = newStreamId("Segmenter::cTrackRunStreamId");
    const StreamId Segmenter::cSidxStreamId = newStreamId("Segmenter::cSidxStreamId");

    std::string to_string(SegmentRole aRole)
    {
        switch (aRole) {
        case SegmentRole::InitSegment: return "InitSegment";
        case SegmentRole::SegmentIndex: return "SegmentIndex";
        case SegmentRole::TrackRunSegment: return "TrackRunSegment";
        case SegmentRole::ImdaSegment: return "ImdaSegment";
        }
        assert(false);
        return "";
    }

    InvalidTrack::InvalidTrack(TrackId trackId)
        : SegmenterException("InvalidTrack " + Utils::to_string(trackId))
    {
        // nothing
    }

    ExpectedIDRFrame::ExpectedIDRFrame() : SegmenterException("ExpectedIDRFrame")
    {
        // nothing
    }

    namespace {
        Optional<std::function<uint32_t(void)>> makeNextFragmentSequenceNumber(
            const Optional<FragmentSequenceGenerator>& aNextImda)
        {
            Optional<std::function<uint32_t(void)>> r;
            if (aNextImda)
            {
                auto nextImda =
                    std::make_shared<std::function<StreamSegmenter::Segmenter::ImdaId(void)>>(
                        *aNextImda);
                std::function<uint32_t(void)> f = [=]() {  //
                    return uint32_t((*nextImda)().get());
                };
                r = f;
            }
            return r;
        }

        StreamSegmenter::AutoSegmenterConfig makeAutoSegmenterConfig(Segmenter::Config aConfig)
        {
            StreamSegmenter::AutoSegmenterConfig config {};
            config.segmentDuration = aConfig.segmentDuration;
            config.subsegmentDuration = aConfig.subsegmentDuration;
            config.checkIDR = aConfig.checkIDR;
            config.nextFragmentSequenceNumber =
                makeNextFragmentSequenceNumber(aConfig.acquireNextImdaId);
            return config;
        }
    }

    Segmenter::Segmenter(Config aConfig, bool aCreateWriter)
        : mConfig(aConfig)
        , mAutoSegmenter(makeAutoSegmenterConfig(aConfig))
        , mAcquireNextImdaId(aConfig.acquireNextImdaId)
    {
        if (mConfig.singleOutputStream)
        {
            mSingleOutputStream = *mConfig.singleOutputStream;
        }
        else
        {
            switch (mConfig.mode)
            {
            case OutputMode::None:
            case OutputMode::OMAFV1:
                mSingleOutputStream = true;
                break;
            case OutputMode::OMAFV2:
                mSingleOutputStream = false;
                break;
            }
        }
        if (aCreateWriter)
        {
            mSegmentWriter.reset(StreamSegmenter::Writer::create(writerModeOfOutputMode(aConfig.mode)));
        }

        assert(mConfig.tracks.size());
        for (auto trackIdMeta : mConfig.tracks)
        {
            mAutoSegmenter.addTrack(trackIdMeta.first, trackIdMeta.second);
        }
    }

    std::string Segmenter::getGraphVizDescription()
    {
        std::ostringstream st;
        bool first = true;
        if (auto& imda = mConfig.acquireNextImdaId)
        {
            st << "freq seq: " << imda->sequenceBase << "/" << imda->sequenceStep;
        }

        st << "\nsidx: " << (mConfig.sidx ? "true" : "false");
        st << "\nnoMetadataSidx: " << (mConfig.noMetadataSidx ? "true" : "false");
        st << "\nmSingleOutputStream: " << (mSingleOutputStream ? "true" : "false");
        st << "\nsegmentHeader: " << int(mConfig.segmentHeader);

        st << "\nstreams: ";
        for (auto streamId :
             /* sort: */ std::set<StreamId>{mConfig.streamIds.begin(), mConfig.streamIds.end()})
        {
            st << (first ? "" : ",");
            first = false;
            st << streamId;
        }
        st << "\ntracks: ";
        first = true;
        for (auto& [trackId, trackMeta] : mConfig.tracks)
        {
            st << (first ? "" : ",");
            first = false;
            st << trackId;
        }
        return st.str();
    }

    Segmenter::~Segmenter()
    {
        // nothing
    }

    StorageType Segmenter::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    bool Segmenter::detectNonRefFrame(const Data& frame)
    {
        bool flagNonRef = false;
        auto& mydata = frame.getCPUDataReference();
        const uint8_t* curPtr = static_cast<const std::uint8_t*>(mydata.address[0]);
        flagNonRef = (curPtr[4] >> 5)==0;
        return flagNonRef;
    }

    void Segmenter::feed(TrackId aTrackId, const Data& aFrame,
                         StreamSegmenter::FrameDts aDecodingTime,
                         StreamSegmenter::FrameCts aCompositionTime)
    {
        const auto& frameMeta = aFrame.getCodedFrameMeta();
        std::unique_ptr<StreamSegmenter::AcquireFrameData> dataFrameAcquire(
            new DataFrameAcquire(aFrame));
        StreamSegmenter::FrameInfo frameInfo;
        frameInfo.dts = aDecodingTime;
        frameInfo.cts = aCompositionTime;
        frameInfo.duration = frameMeta.duration;
        frameInfo.isIDR = frameMeta.isIDR();
        bool flagNonRefframe = (frameMeta.format == CodedFormat::H264) ? detectNonRefFrame(aFrame) : false;

        // sample flags semantics from ISO/IEC 14496-12:2015(E)
        // is_leading takes one of the following four values:
        //   0: the leading nature of this sample is unknown;
        //   1: this sample is a leading sample that has a dependency before the referenced I - picture(and is therefore not decodable);
        //   2: this sample is not a leading sample;
        //   3: this sample is a leading sample that has no dependency before the referenced I - picture(and is therefore decodable);
        // sample_depends_on takes one of the following four values:
        //   0: the dependency of this sample is unknown;
        //   1: this sample does depend on others(not an I picture);
        //   2: this sample does not depend on others(I picture);
        //   3: reserved
        // sample_is_depended_on takes one of the following four values:
        //   0: the dependency of other samples on this sample is unknown;
        //   1: other samples may depend on this one(not disposable);
        //   2: no other sample depends on this one(disposable);
        //   3: reserved
        // sample_has_redundancy takes one of the following four values:
        //   0: it is unknown whether there is redundant coding in this sample;
        //   1: there is redundant coding in this sample;
        //   2: there is no redundant coding in this sample;
        //   3: reserved
        frameInfo.sampleFlags.flags.reserved = 0;
        frameInfo.sampleFlags.flags.is_leading = (frameInfo.isIDR ? 3 : 0);
        frameInfo.sampleFlags.flags.sample_depends_on = (frameInfo.isIDR ? 2 : 1);
        frameInfo.sampleFlags.flags.sample_is_depended_on = (flagNonRefframe ? 2 : 1);
        frameInfo.sampleFlags.flags.sample_has_redundancy = 0;
        frameInfo.sampleFlags.flags.sample_padding_value = 0;
        frameInfo.sampleFlags.flags.sample_is_non_sync_sample = !frameInfo.isIDR;
        frameInfo.sampleFlags.flags.sample_degradation_priority = 0;

        StreamSegmenter::FrameProxy ssFrame(std::move(dataFrameAcquire), frameInfo);
        mAutoSegmenter.feed(aTrackId, ssFrame);
    }

    std::vector<Streams> Segmenter::process(const Streams& data)
    {
        if (mConfig.streamIds.size() > 0)
        {
            std::list<StreamId>::const_iterator findIter = std::find(mConfig.streamIds.begin(), mConfig.streamIds.end(), data.front().getStreamId());
            if (findIter == mConfig.streamIds.end())
            {
                // not for us, ignore
                std::vector<Streams> frames;
                return frames;
            }
        }

        if (!data.isEndOfStream() && data.front().getExtractors().size() > 0)
        {
            Streams outputData;
            packExtractors(data.front(), outputData);
            return doProcess(outputData);
        }
        else
        {
            return doProcess(data);
        }
    }

    std::vector<Streams> Segmenter::doProcess(const Streams& data)
    {
        const Data& frame = data.front();

        TrackId trackId;
        if (auto trackIdTag = frame.getMeta().findTag<TrackIdTag>())
        {
            trackId = trackIdTag->get();
        }
        else
        {
            // track id required
            assert(false);
        }

        assert(mConfig.tracks.count(trackId));
        TrackInfo& trackInfo = mTrackInfo[trackId];
        if (data.isEndOfStream())
        {
            if (trackInfo.pendingFrames.size())
            {
                log(LogLevel::Info)
                    << "Warning! Track handled by node " << getId()
                    << " has pending frames at finish. May be a bug, a "
                       "problem in the input file, or inappropriately set frame_count."
                    << std::endl;
            }

            if (!trackInfo.end)
            {
                if (!trackInfo.firstFrame)
                {
                    mAutoSegmenter.feedEnd(trackId);
                }
                trackInfo.end = true;
            }
        }
        else {
            const auto& frameMeta = frame.getCodedFrameMeta();
            trackInfo.totalDuration += frameMeta.duration;

            if (frame.getCodedFrameMeta().inCodingOrder)
            {
                StreamSegmenter::FrameCts frameCts;
                if (frameMeta.presTime == FrameTime(0, 1))
                {
                    if (trackInfo.firstFrame)
                    {
                        trackInfo.nextCodingTime = FrameTime(frameMeta.codingIndex, 1) *
                                                   frameMeta.duration.cast<FrameTime>();
                    }
                    frameCts = {trackInfo.nextCodingTime};
                    trackInfo.nextCodingTime += frameMeta.duration.cast<FrameTime>();
                }
                else
                {
                    frameCts = {frameMeta.presTime};
                }
                trackInfo.lastPresIndex = frameMeta.presIndex;
                trackInfo.firstFrame = false;
                try
                {
                    feed(trackId, frame, frameMeta.codingTime, frameCts);
                }
                catch (StreamSegmenter::ExpectedIDRFrame&)
                {
                    std::cout << "track id " << trackId << " pres index " << frameMeta.presIndex
                              << " was not IDR!" << std::endl;
                    throw;
                }
            }
            else
            {
                assert(!trackInfo.end);

                // This assumes input frames come in presentation order. Their presIndex then
                // determines the order they are presented in, and once we have all the required
                // frames to present all known frames, we flush all our frames to the autosegmenter
                // in decoding order.

                // Perhaps this logic should be moved to autosegmenter? It would need the coding
                // time to be explicitly indicated in that case.
                if (trackInfo.firstFrame)
                {
                    // Pick the first coding index; typically it's -1, so the time ends up at
                    // -frame_duration
                    trackInfo.nextCodingTime =
                        FrameTime(frameMeta.codingIndex, 1) * frameMeta.duration.cast<FrameTime>();
                }

                trackInfo.pendingFrames[frameMeta.presIndex] = {};
                auto& pendingFrame = trackInfo.pendingFrames[frameMeta.presIndex];
                pendingFrame.frame = frame;
                pendingFrame.codingTime = trackInfo.nextCodingTime;
                // compositionTime not known yet

                trackInfo.nextCodingTime += frameMeta.duration.cast<FrameTime>();

                // - for each PendingFrameInfo hasPreviousFrame is true if
                //   - we have a frame with the previous presentation index
                //   - or it's the first frame
                //   - or we have previously written the preceding frame
                if (trackInfo.firstFrame || frameMeta.presIndex == trackInfo.lastPresIndex + 1 ||
                    trackInfo.pendingFrames.count(frameMeta.presIndex - 1))
                {
                    trackInfo.pendingFrames[frameMeta.presIndex].hasPreviousFrame = true;
                    ++trackInfo.numConsecutiveFrames;
                }

                if (trackInfo.pendingFrames.count(frameMeta.presIndex + 1))
                {
                    trackInfo.pendingFrames[frameMeta.presIndex + 1].hasPreviousFrame = true;
                    ++trackInfo.numConsecutiveFrames;
                }

                // OK, we have received a set of frames that is complete, that is, it has no gaps in
                // presentation time and therefore no gaps in coding times either
                if (trackInfo.pendingFrames.size() == trackInfo.numConsecutiveFrames)
                {
                    trackInfo.lastPresIndex = trackInfo.pendingFrames.rbegin()
                                                  ->second.frame.getCodedFrameMeta()
                                                  .presIndex;

                    std::map<CodingIndex, PendingFrameInfo> framesInCodingOrder;

                    // the frames are in presentation order. assign their composition times
                    // (presentation times)
                    for (auto& pending : trackInfo.pendingFrames)
                    {
                        pending.second.compositionTime = {mTrackInfo[trackId].nextFramePresTime};
                        mTrackInfo[trackId].nextFramePresTime +=
                            frame.getCodedFrameMeta().duration.cast<FrameTime>();
                        framesInCodingOrder[pending.second.frame.getCodedFrameMeta().codingIndex] =
                            pending.second;
                    }

                    for (auto& pending : framesInCodingOrder)
                    {
                        auto cm = pending.second.frame.getCodedFrameMeta();
                        try
                        {
                            feed(trackId, pending.second.frame, pending.second.codingTime,
                                 pending.second.compositionTime);
                        }
                        catch (StreamSegmenter::ExpectedIDRFrame&)
                        {
                            std::cout << "track id " << trackId << " pres index " << cm.presIndex
                                      << " was not IDR!" << std::endl;
                            throw;
                        }
                    }

                    trackInfo.pendingFrames.clear();
                    trackInfo.numConsecutiveFrames = 0;
                }

                trackInfo.firstFrame = false;
            }
        }

        std::vector<Streams> frames;

        if (mWaitForInitSegment)
        {
            return frames;
        }

        std::list<StreamSegmenter::Segmenter::Segments> segments = mAutoSegmenter.extractSegmentsWithSubsegments();

        if (segments.size())
        {
            for (StreamSegmenter::Segmenter::Segments& subsegments : segments)
            {
                for (StreamSegmenter::Segmenter::Segment& segment : subsegments)
                {
                    // sequences in sync
                    if (mAcquireNextImdaId)
                    {
                        StreamSegmenter::Segmenter::OMAFv2Link omafv2;
                        omafv2.imdaId = segment.sequenceId;
                        omafv2.dataReferenceKind = StreamSegmenter::Segmenter::DataReferenceKind::Snim;
                        segment.omafv2 = omafv2;
                    }
                }
                writeSegment(subsegments, frames);
            }
        }

        bool allEnd = true;
        for (auto& track : mTrackInfo)
        {
            allEnd = allEnd && track.second.end;
        }
        if (allEnd)
        {
            Streams streams;
            if (mSingleOutputStream)
            {
                streams.add(Data(EndOfStream(),
                                 Meta().attachTag(SegmentRoleTag(SegmentRole::ImdaSegment)),
                                 cSingleStreamId));
            }
            else
            {
                streams.add(Data(EndOfStream(),
                                 Meta().attachTag(SegmentRoleTag(SegmentRole::ImdaSegment)),
                                 cImdaStreamId));
                streams.add(Data(EndOfStream(),
                                 Meta()
                                     .attachTag(SegmentInfoTag(SegmentInfo{
                                         {0, 1},  // firstPresentationTime
                                         {0, 1}   // segmentDuration
                                     }))
                                     .attachTag(SegmentRoleTag(SegmentRole::TrackRunSegment)),
                                 cTrackRunStreamId));
            }
            if (mConfig.sidx)
            {
                streams.add(Data(EndOfStream(),
                                 Meta().attachTag(SegmentRoleTag(SegmentRole::SegmentIndex)),
                                 cSidxStreamId));
            }
            frames.push_back(streams);
        }

        return frames;
    }

    SegmentBitrate Segmenter::calculateBitrate() const
    {
        SegmentBitrate bitrate {};

        if (mTrackInfo.size())
        {
            FrameDuration longestDuration;
            for (const auto& trackIdTrackInfo : mTrackInfo)
            {
                const auto& trackInfo = trackIdTrackInfo.second;
                longestDuration = std::max(longestDuration, trackInfo.totalDuration);
            }

            if (longestDuration > FrameDuration())
            {
                bitrate.average = static_cast<uint64_t>(mTotalNumberOfBits / longestDuration.asDouble());
            }
        }

        return bitrate;
    }

    Meta Segmenter::segmentBitRateMeta(StreamSegmenter::Segmenter::Duration aSegmentDuration) const
    {
        CodedFrameMeta codedMeta{};
        codedMeta.segmenterMeta.segmentDuration = aSegmentDuration;
        Meta meta(codedMeta);
        meta.attachTag(SegmentBitrateTag(calculateBitrate()));
        return meta;
    }

    void Segmenter::writeSegment(StreamSegmenter::Segmenter::Segments& aSegment,
                                 std::vector<Streams>& aResultFrames)
    {
        bool wasFirst = mFirstSegment;
        mFirstSegment = false;
        std::ostringstream metaStream;
        std::ostringstream frameStream;
        std::unique_ptr<std::ostringstream> sidxStream;

        switch (mConfig.segmentHeader) {
        case WriteSegmentHeader::None:
        {
            mSegmentWriter->setWriteSegmentHeader(false);
            break;
        }
        case WriteSegmentHeader::First:
        {
            mSegmentWriter->setWriteSegmentHeader(wasFirst);
            break;
        }
        case WriteSegmentHeader::All:
        {
            mSegmentWriter->setWriteSegmentHeader(true);
            break;
        }
        }

        if (auto hd = mConfig.frameSegmentHeader)
        {
            auto x = makeXtyp(*hd);
            frameStream.write(reinterpret_cast<const char*>(&x[0]), x.size());
        }

        SegmentInfo segmentInfo {};

        if (aSegment.size())
        {
            auto& first = aSegment.front();
            segmentInfo.firstPresentationTime = first.t0;
            for (auto& seg : aSegment)
            {
                segmentInfo.segmentDuration += seg.duration;
            };
        };

        if (mConfig.sidx)
        {
            sidxStream.reset(new std::ostringstream);
            if (!mSidxWriter)
            {
                mSidxWriter = mSegmentWriter->newSidxWriter(cSidxSpaceReserve);
            }
            if (mSidxWriter && mSingleOutputStream)
            {
                mSidxWriter->setOutput(sidxStream.get());
            }
        }
        else
        {
            //mSidxWriter = mSegmentWriter.newSidxWriter();
        }

        // preserving sharing with Data. A better-than-this solution would provide streambuf
        // that would construct a std::vector<uint_t> directly.

        if (mSingleOutputStream)
        {
            mSegmentWriter->writeSubsegments(frameStream, aSegment,
                                             StreamSegmenter::WriterFlags::ALL);
        }
        else
        {
            mSegmentWriter->writeSubsegments(
                metaStream, aSegment,
                // well this is ugly
                StreamSegmenter::WriterFlags(
                    int(StreamSegmenter::WriterFlags::METADATA) |
                    (mConfig.noMetadataSidx ? int(StreamSegmenter::WriterFlags::SKIPSIDX) : 0)));
            mSegmentWriter->writeSubsegments(
                frameStream, aSegment,
                StreamSegmenter::WriterFlags(
                    int(StreamSegmenter::WriterFlags::MEDIADATA) |
                    // we cannot use appending if we already skipped the first write
                    (mConfig.noMetadataSidx ? 0 : int(StreamSegmenter::WriterFlags::APPEND))));
            if (sidxStream)
            {
                mSegmentWriter->writeSubsegments(*sidxStream, aSegment,
                                                 StreamSegmenter::WriterFlags::SIDX);
            }
        }

        StreamSegmenter::Segmenter::Duration segmentDuration;
        for (auto& subsegment : aSegment)
        {
            segmentDuration += subsegment.duration;
            assert(subsegment.duration.num > 0);
        }

        // eli täällä pitäisi tehdä kaksi datavirtaa: toinen jossa on media ja imda-laatikot ja
        // toinen jossa on moof ja tämä moof menee sitten extractorin kanssa talteen
        mTotalNumberOfBits += size_t(frameStream.tellp()) * 8;

        if (mSingleOutputStream)
        {
            aResultFrames.push_back(
                {dataOfStream(frameStream,
                              segmentBitRateMeta(segmentDuration)
                                  .attachTag(SegmentInfoTag(segmentInfo))
                                  .attachTag(SegmentRoleTag(SegmentRole::TrackRunSegment)),
                              cSingleStreamId)});
        }
        else
        {
            CodedFrameMeta codedMeta{};
            codedMeta.segmenterMeta.segmentDuration = segmentDuration;

            aResultFrames.push_back(
                {dataOfStream(frameStream,
                              segmentBitRateMeta(segmentDuration)
                                  .attachTag(SegmentRoleTag(SegmentRole::ImdaSegment)),
                              cImdaStreamId),
                 dataOfStream(metaStream,
                              Meta(codedMeta)
                                  .attachTag(SegmentInfoTag(segmentInfo))
                                  .attachTag(SegmentRoleTag(SegmentRole::TrackRunSegment)),
                              cTrackRunStreamId)});
        }

        if (mConfig.sidx)
        {
            CodedFrameMeta codedMeta{};
            Meta meta(codedMeta);
            meta.attachTag(SegmentRoleTag(SegmentRole::SegmentIndex));
            aResultFrames.push_back({ dataOfStream(*sidxStream, std::move(meta), cSidxStreamId) });
        }

        return;
    }

    void Segmenter::packExtractors(const Data& aFrame, Streams& aOutputData)
    {
        std::vector<std::uint8_t> extractorNALUnits;
        TrackId extractorTrackId = 1;

        if (auto trackIdTag = aFrame.getMeta().findTag<TrackIdTag>())
        {
            extractorTrackId = trackIdTag->get();
        }
        else
        {
            // track id required
            assert(false);
        }

        if (aFrame.getStorageType() != StorageType::Empty)
        {
            // first place possible other NAL units (at least SEI)
            auto& data = aFrame.getCPUDataReference();
            extractorNALUnits.insert(extractorNALUnits.begin(),
                                     static_cast<const std::uint8_t*>(data.address[0]),
                                     static_cast<const std::uint8_t*>(data.address[0]) + data.size[0]);
        }
        for (auto extractor : aFrame.getExtractors())
        {
            StreamSegmenter::Segmenter::HevcExtractorTrackFrameData extractorData;
            extractorData.nuhTemporalIdPlus1 = extractor.nuhTemporalIdPlus1;
            std::vector<Extractor::SampleConstruct>::iterator sampleConstruct;
            std::vector<Extractor::InlineConstruct>::iterator inlineConstruct;

            // We loop through both constructors, until both of them are empty. They are often interleaved, but not
            // always through the whole sequence.
            StreamSegmenter::Segmenter::HevcExtractor outputExtractor;
            for (sampleConstruct = extractor.sampleConstruct.begin(),
                inlineConstruct = extractor.inlineConstruct.begin();
                sampleConstruct != extractor.sampleConstruct.end() ||
                inlineConstruct != extractor.inlineConstruct.end();)
            {
                if (inlineConstruct != extractor.inlineConstruct.end() &&
                    (sampleConstruct == extractor.sampleConstruct.end() ||
                    (*inlineConstruct).idx < (*sampleConstruct).idx))
                {
                    outputExtractor.inlineConstructor = StreamSegmenter::Segmenter::HevcExtractorInlineConstructor{};
                    outputExtractor.inlineConstructor->inlineData = std::move(inlineConstruct->inlineData);
                    inlineConstruct++;
                }
                else if (sampleConstruct != extractor.sampleConstruct.end())
                {
                    outputExtractor.sampleConstructor = StreamSegmenter::Segmenter::HevcExtractorSampleConstructor{};
                    outputExtractor.sampleConstructor->sampleOffset = 0;
                    outputExtractor.sampleConstructor->dataOffset = sampleConstruct->dataOffset;
                    outputExtractor.sampleConstructor->dataLength = sampleConstruct->dataLength;

                    ScalTrefIndex scalTrefIndex =
                        mConfig.trackToScalTrafIndexMap.at(extractorTrackId)
                            .at(sampleConstruct->trackId);

                    // Note: this refers to the 1-based index in the track references.
                    outputExtractor.sampleConstructor->trackId = scalTrefIndex.get();
                    sampleConstruct++;
                    // now we have a full extractor: either just a sample constructor, or inline+sample constructor pair
                    extractorData.samples.push_back(outputExtractor);
                    const StreamSegmenter::FrameData& nal = extractorData.toFrameData();
                    extractorNALUnits.insert(extractorNALUnits.end(), nal.begin(), nal.end());
                }
            }
        }
        std::vector<std::vector<std::uint8_t>> matrix(1, std::move(extractorNALUnits));
        Meta meta(aFrame.getCodedFrameMeta());
        if (auto trackIdTag = aFrame.getMeta().findTag<TrackIdTag>())
        {
            meta.attachTag<TrackIdTag>(trackIdTag->get());
        }
        aOutputData.add(Data(CPUDataVector(std::move(matrix)), meta, aFrame.getStreamId()));
    }

}  // namespace VDD
