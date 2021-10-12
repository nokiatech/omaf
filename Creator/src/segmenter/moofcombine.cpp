
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
#include <iterator>
#include <cstdint>

#include "moofcombine.h"
#include "segmentercommon.h"
#include "controller/dashomaf.h"

namespace VDD
{
    TooLargeSegment::TooLargeSegment(std::size_t aSize)
        : Exception("TooLargeSegment")
        , mSize(aSize)
    {
        // nothingzxvrbtnymuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu>
    }

    std::string TooLargeSegment::message() const
    {
        return ">~4GB segment, unsupported (" + std::to_string(mSize) + " bytes)";
    }

    MoofCombine::MoofCombine(Config aConfig) : mConfig(aConfig)
    {
        // nothing
    }

    MoofCombine::~MoofCombine() = default;

    std::vector<Streams> MoofCombine::process(const Streams& aStreams)
    {
        bool wasFirst = mFirst;
        mFirst = false;
        if (aStreams.isEndOfStream())
        {
            return {{Data(EndOfStream(),
                          Meta{CodedFrameMeta()}
                              .attachTag(MimeTypeCodecsTag{{"video/mp4 profiles=\"hevd,siti\"", {}}})
                              .attachTag(SegmentRoleTag(SegmentRole::TrackRunSegment)))}};
        }

        std::map<StreamId, std::vector<size_t>> streamIdToDataIndex;

        for (size_t index = 0u; index < aStreams.size(); ++index)
        {
            streamIdToDataIndex[aStreams[index].getStreamId()].push_back(index);

            auto tag = aStreams[index].getMeta().findTag<SegmentRoleTag>();

            // the SegmentRoleTag is required for SingleFileSave
            assert(tag);
        }

        /*
          – Tile Index Segments shall be indicated in DASH MPD as Index Segments of the Representation that carries
          the Base Track and shall contain an 'styp' box including the brand 'sibm'.
          – Tile Data Segments of the same Tile Track shall be declared as a single Representation in DASH MPD and
        */

        std::vector<std::uint8_t> header =
            (mConfig.writeSegmentHeader == WriteSegmentHeader::Always ||
             (mConfig.writeSegmentHeader == WriteSegmentHeader::First && wasFirst)) ||
                    mConfig.generateSidx
                ? makeXtyp(SegmentHeader { mConfig.xtyp, "sibm" })
                : std::vector<std::uint8_t>{};

        // quite a fiddly job to do just to avoid reallocating..
        std::size_t totalSize = 0u;
        for (StreamId streamId : mConfig.order)
        {
            if (streamIdToDataIndex.count(streamId))
            {
                for (size_t index : streamIdToDataIndex.at(streamId))
                {
                    totalSize += aStreams[index].getCPUDataReference().size[0] +
                                 (totalSize == 0 ? header.size() : 0);
                }
            }
        }

        // 32 is not precise.. good enough
        if (totalSize > size_t(~std::uint32_t(0) - 32))
        {
            throw TooLargeSegment(totalSize);
        }

        std::vector<std::uint8_t> data;
        data.reserve(totalSize);

        if (!mConfig.generateSidx)
        {
            std::copy(header.begin(), header.end(), std::back_inserter(data));
        }

        for (StreamId streamId : mConfig.order)
        {
            if (streamIdToDataIndex.count(streamId))
            {
                for (size_t index : streamIdToDataIndex.at(streamId))
                {
                    const CPUDataReference& ref = aStreams[index].getCPUDataReference();
                    auto base = static_cast<const uint8_t*>(ref.address[0]);
                    std::copy(base, base + ref.size[0], std::back_inserter(data));
                }
            }
        }

        Streams streams;

        if (mConfig.generateSidx)
        {
            // SegmentInfoTag is mandatory for generateSidx to work
            auto segmentInfo = aStreams[0].getMeta().findTag<SegmentInfoTag>()->get();
            if (wasFirst) {
                mEarliestPresentationTime = segmentInfo.firstPresentationTime;
            }
            StreamSegmenter::SidxReference sidxRef {};
            sidxRef.referenceType = 0;
            sidxRef.referencedSize = data.size();
            sidxRef.subsegmentDuration = segmentInfo.segmentDuration;
            sidxRef.startsWithSAP = true;
            sidxRef.sapType = 1;
            sidxRef.sapDeltaTime = {0, 1};
            mSidxReferences.push_back(sidxRef);
            std::vector<std::uint8_t> content; // includes ftyp header
            content.reserve(32);
            std::vector<char> sidx =              // comment to force nicer formatting.
                StreamSegmenter::generateSidx(0,  // aReferenceId
                                              mEarliestPresentationTime,
                                              0,  // aFirstOffset
                                              mSidxReferences,
                                              1000  // aReserveTotal
                );
            std::copy(header.begin(), header.end(), std::back_inserter(content));
            std::copy(sidx.begin(), sidx.end(), std::back_inserter(content));

            streams.add(
                Data(CPUDataVector{{std::move(content)}},
                      Meta{CodedFrameMeta()}
                          .attachTag(MimeTypeCodecsTag{{"video/mp4 profiles=\"hevd,siti\"", {}}})
                          .attachTag(SegmentRoleTag(SegmentRole::SegmentIndex))));
        }
        streams.add(
            Data(CPUDataVector{{std::move(data)}},
                  Meta{CodedFrameMeta()}
                      .attachTag(MimeTypeCodecsTag{{"video/mp4 profiles=\"hevd,siti\"", {}}})
                      .attachTag(SegmentRoleTag(SegmentRole::TrackRunSegment))));

        return {{streams}};
    }
}  // namespace VDD