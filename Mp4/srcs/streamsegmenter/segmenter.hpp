
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
#ifndef STREAMSEGMENTER_SEGMENTERIMPL_HPP
#define STREAMSEGMENTER_SEGMENTERIMPL_HPP

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "api/streamsegmenter/segmenterapi.hpp"
#include "common/compositionoffsetbox.hpp"
#include "common/sampletochunkbox.hpp"

// If you have access to the MP4 file, you may be able to use these to more easily construct initialization data
class MovieHeaderBox;

namespace StreamSegmenter
{
    class SidxWriterImpl : public SidxWriter
    {
    public:
        SidxWriterImpl(size_t aExpectedSize);
        ~SidxWriterImpl() override;

        void addSubsegment(Segmenter::Segment aSubsegment) override;

        void setFirstSubsegmentOffset(std::streampos aFirstSubsegmentOffset) override;

        void addSubsegmentSize(std::streampos aSubsegmentOffset) override;

        void updateSubsegmentSize(int aRelative, std::streampos aSubsegmentSize) override;

        ISOBMFF::Optional<SidxInfo> writeSidx(std::ostream& aOutput,
                                              ISOBMFF::Optional<std::ostream::pos_type> aPosition) override;

        void setOutput(std::ostream* aOutput) override;

    private:
        std::ostream* mOutput = nullptr;
        std::streampos mFirstSubsegmentOffset;
        size_t mExpectedSize;
        std::list<Segmenter::Segment> mSubsegments;
        std::list<std::streampos> mSubsegmentSizes;
    };

    class WriterImpl : public Writer
    {
    public:
        WriterImpl(Segmenter::WriterMode aMode);
        ~WriterImpl();

        /** @brief If you are writing segments in individual files and you're using segment
         * indices, call this before each write.
         *
         * @param aSidxWriter a pointer to a SidxWriter owned by Writer
         * @param aOutput a pointer to an alternative output stream, instead the provided to Writer
         */
        SidxWriter* newSidxWriter(size_t aExpectedSize = 0) override;

        void setWriteSegmentHeader(bool aWriteSegmentHeader) override;

        void writeInitSegment(std::ostream& aOut, const Segmenter::InitSegment& aInitSegment) override;
        void writeSegment(std::ostream& aOut, const Segmenter::Segment aSegment, WriterFlags aFlags) override;
        void writeSubsegments(std::ostream& aOut,
                              const std::list<Segmenter::Segment> aSubsegments,
                              WriterFlags aFlags) override;

    private:
        std::unique_ptr<SidxWriter> mSidxWriter;
        bool mWriteSegmentHeader = true;
        Segmenter::WriterMode mMode;
        StreamSegmenter::Segmenter::WriterConfig writerConfig() const;
    };

    class MovieWriterImpl : public MovieWriter
    {
    public:
        MovieWriterImpl(std::ostream& aOut);
        ~MovieWriterImpl();

        void writeInitSegment(const Segmenter::InitSegment& aInitSegment) override;

        void writeSegment(const Segmenter::Segment& aSegment) override;

        void finalize() override;

    private:
        std::ostream& mOut;
        ISOBMFF::Optional<Segmenter::InitSegment> mInitSegment;
        bool mFinalized = false;

        struct Info
        {
            TrackBox* trackBox;
            std::list<std::uint32_t> sampleSizes;
            std::list<std::uint64_t> chunkOffsets;
            std::list<SampleToChunkBox::ChunkEntry> chunks;
            std::list<CompositionOffsetBox::EntryVersion0> compositionOffsets;
            FrameTime decodingTime;                      // used to calculate offset to composition time
            bool anyNonSyncSample = false;               // was any of the samples other than sync?
            std::list<std::uint32_t> syncSampleIndices;  // list of all sync samples
            std::uint32_t sampleIndex = 0;
            FrameTime dtsCtsOffset;
            std::uint64_t duration = 0;
        };

        std::streampos mMdatHeaderOffset;

        std::map<TrackId, Info> mTrackInfo;
    };

}  // namespace StreamSegmenter

#endif  // STREAMSEGMENTER_SEGMENTERIMPL_HPP
