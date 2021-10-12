
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

#include <functional>
#include "processor/processor.h"

namespace VDD
{
    // SidxAdjuster takes in two streams:
    // - extractor sidx output
    // - tiles moof output
    // and then generates a new SegmentIndex that accounts for including the tiles imda
    // with the understanding that the tiles moof and tile imda will be concatenated
    // to the extractor moof/imda (as described by SegmentIndex) in the end
    class SidxAdjuster : public Processor
    {
    public:
        struct Config {
            // nothing
        };

        static const StreamId constexpr cExtractorSidxInputStreamId{0};
        static const StreamId constexpr cTilesMoofInputStreamId{1};

        SidxAdjuster(const Config& aConfig);

        ~SidxAdjuster();

        StorageType getPreferredStorageType() const override;

        std::vector<Streams> process(const Streams&) override;

    private:
        struct Impl;
        Config mConfig;
        std::unique_ptr<Impl> mImpl;
    };
}
