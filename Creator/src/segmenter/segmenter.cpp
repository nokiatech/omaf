
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

    }  // anonymous namespace

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
        StreamSegmenter::AutoSegmenterConfig makeAutoSegmenterConfig(Segmenter::Config aConfig)
        {
            StreamSegmenter::AutoSegmenterConfig config {};
            config.segmentDuration = aConfig.segmentDuration;
            config.subsegmentDuration = aConfig.subsegmentDuration;
            config.checkIDR = aConfig.checkIDR;
            return config;
        }
    }

    Segmenter::Segmenter(Config aConfig, bool aCreateWriter)
        : mConfig(aConfig)
        , mAutoSegmenter(makeAutoSegmenterConfig(aConfig))
    {
        if (aCreateWriter)
        {
            mSegmentWriter.reset(StreamSegmenter::Writer::create());
            if (mConfig.separateSidx)
            {
                mSegmentWriter->setWriteSegmentHeader(false);
            }
        }

        for (auto trackIdMeta : mConfig.tracks)
        {
            mAutoSegmenter.addTrack(trackIdMeta.first, trackIdMeta.second);
        }
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

    void Segmenter::feed(TrackId aTrackId, const Data& aFrame, StreamSegmenter::FrameCts aCompositionTime)
    {
        const auto& frameMeta = aFrame.getCodedFrameMeta();
        std::unique_ptr<StreamSegmenter::AcquireFrameData> dataFrameAcquire(
            new DataFrameAcquire(aFrame));
        StreamSegmenter::FrameInfo frameInfo;
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

    std::vector<Views> Segmenter::process(const Views& data)
    {
        if (mConfig.streamIds.size() > 0)
        {
            std::list<StreamId>::const_iterator findIter = std::find(mConfig.streamIds.begin(), mConfig.streamIds.end(), data[0].getStreamId());
            if (findIter == mConfig.streamIds.end())
            {
                // not for us, ignore
                std::vector<Views> frames;
                return frames;
            }
        }

        if (!data[0].isEndOfStream() && data[0].getExtractors().size() > 0)
        {
            std::vector<Data> outputData;
            packExtractors(data[0], outputData);
            //TODO do we need to loop here, since doProcess processes only outputData[0]
            return doProcess(outputData);
        }
        else
        {
            return doProcess(data);
        }
    }

    std::vector<Views> Segmenter::doProcess(const Views& data)
    {
        const Data& frame = data[0];

        TrackId trackId = 1;
        if (auto trackIdTag = frame.getMeta().findTag<TrackIdTag>())
        {
            trackId = trackIdTag->get();
        }


        TrackInfo& trackInfo = mTrackInfo[trackId];

        if (frame.isEndOfStream())
        {
            // TODO: do something less drastic, though this is something to know about
            assert(trackInfo.pendingFrames.size() == 0);
            if (!trackInfo.end)
            {
                if (!trackInfo.firstFrame)
                {
                    mAutoSegmenter.feedEnd(trackId);
                }
                trackInfo.end = true;
            }
        }
        else if (frame.getCodedFrameMeta().inCodingOrder)
        {
            auto frameMeta = frame.getCodedFrameMeta();
            StreamSegmenter::FrameCts frameCts;
            if (frameMeta.presTime == FrameTime(0, 1))
            {
                if (trackInfo.firstFrame)
                {
                    trackInfo.nextCodingTime = FrameTime(frameMeta.codingIndex, 1) * frameMeta.duration.cast<FrameTime>();
                }
                frameCts = { trackInfo.nextCodingTime };
                trackInfo.nextCodingTime += frameMeta.duration.cast<FrameTime>();
            }
            else
            {
                frameCts = { frameMeta.presTime };
            }
            trackInfo.lastPresIndex = frameMeta.presIndex;
            trackInfo.firstFrame = false;
            try
            {
                feed(trackId, frame, frameCts);
            }
            catch (StreamSegmenter::ExpectedIDRFrame&)
            {
                std::cout << "track id " << trackId
                    << " pres index " << frameMeta.presIndex
                    << " was not IDR!"
                    << std::endl;
                throw;
            }
        }
        else
        {
            assert(!trackInfo.end);

            // This assumes input frames come in presentation order. Their presIndex then determines the
            // order they are presented in, and once we have all the required frames to present all
            // known frames, we flush all our frames to the autosegmenter in decoding order.

            // Perhaps this logic should be moved to autosegmenter? It would need the coding time to
            // be explicitly indicated in that case.

            const auto& frameMeta = frame.getCodedFrameMeta();
            if (trackInfo.firstFrame)
            {
                // Pick the first coding index; typically it's -1, so the time ends up at -frame_duration
                trackInfo.nextCodingTime = FrameTime(frameMeta.codingIndex, 1) * frameMeta.duration.cast<FrameTime>();
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
            if (trackInfo.firstFrame ||
                frameMeta.presIndex == trackInfo.lastPresIndex + 1 ||
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
                trackInfo.lastPresIndex = trackInfo.pendingFrames.rbegin()->second.frame.getCodedFrameMeta().presIndex;

                std::map<CodingIndex, PendingFrameInfo> framesInCodingOrder;

                // the frames are in presentation order. assign their composition times (presentation times)
                for (auto& pending: trackInfo.pendingFrames)
                {
                    pending.second.compositionTime = { mTrackInfo[trackId].nextFramePresTime };
                    mTrackInfo[trackId].nextFramePresTime += frame.getCodedFrameMeta().duration.cast<FrameTime>();
                    framesInCodingOrder[pending.second.frame.getCodedFrameMeta().codingIndex] = pending.second;
                }

                for (auto& pending: framesInCodingOrder)
                {
                    auto cm = pending.second.frame.getCodedFrameMeta();
                    try
                    {
                        feed(trackId, pending.second.frame, pending.second.compositionTime);
                    }
                    catch (StreamSegmenter::ExpectedIDRFrame&)
                    {
                        std::cout << "track id " << trackId
                                  << " pres index " << cm.presIndex
                                  << " was not IDR!"
                                  << std::endl;
                        throw;
                    }
                }

                trackInfo.pendingFrames.clear();
                trackInfo.numConsecutiveFrames = 0;
            }

            trackInfo.firstFrame = false;
        }

        std::vector<Views> frames;

        if (mWaitForInitSegment)
        {
            return frames;
        }

        std::list<StreamSegmenter::Segmenter::Segments> segments = mAutoSegmenter.extractSegmentsWithSubsegments();

        if (segments.size())
        {
            for (auto& segment : segments)
            {
                writeSegment(segment, frames);
            }
        }

        bool allEnd = true;
        for (auto& track : mTrackInfo)
        {
            allEnd = allEnd && track.second.end;
        }
        if (allEnd)
        {
            Meta meta;
            meta.attachTag(SegmentRoleTag(SegmentRole::TrackRunSegment));
            frames.push_back({Data(EndOfStream(), meta)});
        }

        return frames;
    }

    void Segmenter::writeSegment(StreamSegmenter::Segmenter::Segments& aSegment, std::vector<Views>& aFrames)
    {
        std::ostringstream frameStream;
        std::unique_ptr<std::ostringstream> sidxStream;

        if (mConfig.separateSidx)
        {
            sidxStream.reset(new std::ostringstream);
            if (!mSidxWriter)
            {
                mSidxWriter = mSegmentWriter->newSidxWriter(1000);
            }
            if (mSidxWriter)
            {
                mSidxWriter->setOutput(sidxStream.get());
            }
        }
        else
        {
            //mSidxWriter = mSegmentWriter.newSidxWriter();
        }

        // TODO: this is still inefficient. Optimal solution would compose segments while
        // preserving sharing with Data. A better-than-this solution would provide streambuf
        // that would construct a std::vector<uint_t> directly.

        mSegmentWriter->writeSubsegments(frameStream, aSegment);

        StreamSegmenter::Segmenter::Duration segmentDuration;
        for (auto& subsegment : aSegment)
        {
            segmentDuration += subsegment.duration;
        }

        {
            std::string frameString(frameStream.str());
            std::vector<std::vector<uint8_t>> dataVec{ { frameString.begin(), frameString.end() } };
            CPUDataVector dataRef(std::move(dataVec));
            CodedFrameMeta codedMeta{};
            codedMeta.segmenterMeta.segmentDuration = segmentDuration;
            Meta meta(codedMeta);
            meta.attachTag(SegmentRoleTag(SegmentRole::TrackRunSegment));
            aFrames.push_back({ Data(std::move(dataRef), meta) });
        }

        if (mConfig.separateSidx)
        {
            std::string sidxString(sidxStream->str());
            std::vector<std::vector<uint8_t>> dataVec{ { sidxString.begin(), sidxString.end() } };
            CPUDataVector dataRef(std::move(dataVec));
            CodedFrameMeta codedMeta{};
            Meta meta(codedMeta);
            meta.attachTag(SegmentRoleTag(SegmentRole::SegmentIndex));
            aFrames.push_back({ Data(std::move(dataRef), meta) });
        }

        return;
    }

    void Segmenter::packExtractors(const Data& aFrame, std::vector<Data>& aOutputData)
    {
        StreamSegmenter::Segmenter::HevcExtractorTrackFrameData extractorData;
        for (auto extractor : aFrame.getExtractors())
        {
            extractorData.nuhTemporalIdPlus1 = extractor.nuhTemporalIdPlus1;
            std::vector<Extractor::SampleConstruct>::iterator sampleConstruct;
            std::vector<Extractor::InlineConstruct>::iterator inlineConstruct;

            // We loop through both constructors, until both of them are empty. They are often interleaved, but not
            // always through the whole sequence.
            for (sampleConstruct = extractor.sampleConstruct.begin(),
                inlineConstruct = extractor.inlineConstruct.begin();
                sampleConstruct != extractor.sampleConstruct.end() ||
                inlineConstruct != extractor.inlineConstruct.end();)
            {
                StreamSegmenter::Segmenter::HevcExtractor outputExtractor;
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
                    outputExtractor.sampleConstructor->trackId = sampleConstruct->trackId;  // Note: this refers to the index in the track references. It works if trackIds are 1-based and contiguous, as the spec expects the index is 1-based. 
                    sampleConstruct++;
                }
                extractorData.samples.push_back(outputExtractor);
            }
        }
        std::vector<std::vector<std::uint8_t>> matrix(1, std::move(extractorData.toFrameData()));
        Meta meta(aFrame.getCodedFrameMeta());
        if (auto trackIdTag = aFrame.getMeta().findTag<TrackIdTag>())
        {
            meta.attachTag<TrackIdTag>(trackIdTag->get());
        }
        aOutputData.push_back(std::move(Data(CPUDataVector(std::move(matrix)), meta, aFrame.getStreamId())));
    }

}  // namespace VDD
