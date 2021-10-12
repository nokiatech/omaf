
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
#include "sidxadjuster.h"

#include <streamsegmenter/autosegmenter.hpp>
#include <streamsegmenter/segmenterapi.hpp>
#include <streamsegmenter/track.hpp>

#include "segmenter.h"

namespace VDD
{
    struct SidxAdjuster::Impl {
        std::vector<uint32_t> adjustments;
    };

    SidxAdjuster::SidxAdjuster(const Config& aConfig)
        : mConfig(aConfig)
        , mImpl(Utils::make_unique<Impl>())
    {
        (void) mConfig;
    }

    SidxAdjuster::~SidxAdjuster() = default;

    StorageType SidxAdjuster::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    std::vector<Streams> SidxAdjuster::process(const Streams& aStreams)
    {
        assert(aStreams.size() == 2);
        if (aStreams.isEndOfStream())
        {
            Data data(EndOfStream(), Meta().attachTag(SegmentRoleTag(SegmentRole::SegmentIndex)));
            return {{data}};
        }
        else
        {
            auto& extractorSidxInput = aStreams[StreamId(cExtractorSidxInputStreamId)];
            auto& tilesMoofInput = aStreams[StreamId(cTilesMoofInputStreamId)];

            assert(extractorSidxInput.getMeta().findTag<SegmentRoleTag>()->get() ==
                   SegmentRole::SegmentIndex);
            assert(tilesMoofInput.getMeta().findTag<SegmentRoleTag>()->get() ==
                   SegmentRole::TrackRunSegment);

            const CPUDataReference& tilesData = tilesMoofInput.getCPUDataReference();
            mImpl->adjustments.push_back(tilesData.size[0]);

            const CPUDataReference& sidxData = extractorSidxInput.getCPUDataReference();
            auto sidxDataAddr = static_cast<const char*>(sidxData.address[0]);
            auto newSidx = StreamSegmenter::patchSidxSegmentSizes(
                sidxDataAddr, sidxDataAddr + sidxData.size[0], mImpl->adjustments,
                Segmenter::cSidxSpaceReserve);

            std::vector<std::vector<std::uint8_t>> llData{{{newSidx.begin(), newSidx.end()}}};
            CPUDataVector data(std::move(llData));
            return {{Data(data, Meta().attachTag(SegmentRoleTag(SegmentRole::SegmentIndex)))}};
        }
    }
}
