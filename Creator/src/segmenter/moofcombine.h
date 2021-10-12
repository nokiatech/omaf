
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

#include "common/exceptions.h"
#include "processor/processor.h"
#include "segmenter/segmenter.h"

namespace VDD
{
    class TooLargeSegment : public Exception
    {
    public:
        TooLargeSegment(size_t aSize);

        std::string message() const override;

    private:
        size_t mSize;
    };

    /** A method combine a bunch of Moofs from multiple slots to something that can be saved */
    class MoofCombine : public Processor
    {
    public:
        enum class WriteSegmentHeader
        {
            Never,
            First,
            Always
        };

        struct Config
        {
            /** Pick these stream ids in order to write */
            std::vector<StreamId> order;

            WriteSegmentHeader writeSegmentHeader = WriteSegmentHeader::Always;

            Xtyp xtyp = Xtyp::Styp;

            // if enabled, a sidx is generated.
            // this requires input data to contain SegmentInfoTag.
            bool generateSidx = false;
        };

        MoofCombine(Config aConfig);
        ~MoofCombine() override;

        StorageType getPreferredStorageType() const override
        {
            return StorageType::Fragmented;
        }

        // returns an empty Data upon processing data; this is useful for determining when the data
        // has been saved
        std::vector<Streams> process(const Streams&) override;

    private:
        Config mConfig;
        bool mFirst = true;

        StreamSegmenter::FrameTime mEarliestPresentationTime;
        std::vector<StreamSegmenter::SidxReference> mSidxReferences;
    };
}
