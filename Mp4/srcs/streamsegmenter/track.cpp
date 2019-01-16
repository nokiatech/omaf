
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
#include <cassert>
#include <map>

#include "api/streamsegmenter/frameproxy.hpp"
#include "api/streamsegmenter/track.hpp"
#include "common/moviebox.hpp"
#include "decodeptsutils.hpp"
#include "mp4access.hpp"
#include "trackbox.hpp"
#include "utils.hpp"

namespace StreamSegmenter
{
    struct Track::Holder
    {
        const MP4Access& mp4;
        std::vector<FrameInfo> frameInfo;
        std::vector<Block> frameBlock;
        FrameData getFrameData(std::size_t aFrameIndex) const;
        size_t getFrameSize(std::size_t aFrameIndex) const;
    };

    Track::AcquireFrameDataFromTrack::AcquireFrameDataFromTrack(std::shared_ptr<Track::Holder> aHolder,
                                                                size_t aFrameIndex)
        : mHolder(aHolder)
        , mFrameIndex(aFrameIndex)
    {
        //  nothing
    }

    Track::AcquireFrameDataFromTrack::~AcquireFrameDataFromTrack()
    {
        // nothing
    }

    FrameData Track::AcquireFrameDataFromTrack::get() const
    {
        return mHolder->getFrameData(mFrameIndex);
    }

    size_t Track::AcquireFrameDataFromTrack::getSize() const
    {
        return mHolder->getFrameSize(mFrameIndex);
    }

    Track::AcquireFrameDataFromTrack* Track::AcquireFrameDataFromTrack::clone() const
    {
        return new AcquireFrameDataFromTrack(mHolder, mFrameIndex);
    }

    FrameData Track::Holder::getFrameData(std::size_t aFrameIndex) const
    {
        assert(aFrameIndex < frameBlock.size());
        const auto storage = mp4.getData(frameBlock.at(aFrameIndex));
        return FrameData(storage.begin(), storage.end());
    }

    size_t Track::Holder::getFrameSize(std::size_t aFrameIndex) const
    {
        assert(aFrameIndex < frameBlock.size());
        return frameBlock.at(aFrameIndex).blockSize;
    }

    Track::Track(Track&& aOther)
        : mHolder(std::move(aOther.mHolder))
        , mTrackMeta(std::move(aOther.mTrackMeta))
    {
        // nothing
    }

    Track::Track(const MP4Access& aMp4, const TrackBox& aTrackBox, const uint32_t aMovieTimeScale)
        : mHolder(new Holder{aMp4, {}, {}})
    {
        const String& handlerName              = aTrackBox.getMediaBox().getHandlerBox().getName();
        const MediaHeaderBox& mdhdBox          = aTrackBox.getMediaBox().getMediaHeaderBox();
        const SampleTableBox& stblBox          = aTrackBox.getMediaBox().getMediaInformationBox().getSampleTableBox();
        const TrackHeaderBox& trackHeaderBox   = aTrackBox.getTrackHeaderBox();
        const TimeToSampleBox& timeToSampleBox = stblBox.getTimeToSampleBox();
        std::shared_ptr<const CompositionOffsetBox> compositionOffsetBox     = stblBox.getCompositionOffsetBox();
        std::shared_ptr<const CompositionToDecodeBox> compositionToDecodeBox = stblBox.getCompositionToDecodeBox();
        std::shared_ptr<const EditBox> editBox                               = aTrackBox.getEditBox();
        const EditListBox* editListBox           = editBox ? editBox->getEditListBox() : nullptr;
        const SampleToChunkBox& sampleToChunkBox = stblBox.getSampleToChunkBox();
        const auto& chunkOffsets                 = stblBox.getChunkOffsetBox().getChunkOffsets();
        const SampleSizeBox& sampleSizeBox       = stblBox.getSampleSizeBox();
        auto syncSampleBox                       = stblBox.getSyncSampleBox();
        const std::set<std::uint32_t> syncSamples =
            syncSampleBox
                ? Utils::setOfContainerMap([](std::uint32_t x) { return x - 1; }, syncSampleBox->getSyncSampleIds())
                : std::set<std::uint32_t>();
        auto sampleSizes = sampleSizeBox.getEntrySize();

        mTrackMeta.trackId   = trackHeaderBox.getTrackID();
        mTrackMeta.timescale = RatU64(1, mdhdBox.getTimeScale());
        mTrackMeta.type      = handlerName == "VideoHandler"
                              ? MediaType::Video
                              : handlerName == "SoundHandler"
                                    ? MediaType::Audio
                                    : handlerName == "DataHandler" ? MediaType::Data : MediaType::Other;

        DecodePts decodePts;
        decodePts.loadBox(&timeToSampleBox);
        if (compositionOffsetBox)
        {
            decodePts.loadBox(&*compositionOffsetBox);
        }
        if (compositionToDecodeBox)
        {
            decodePts.loadBox(&*compositionToDecodeBox);
        }
        if (editListBox)
        {
            decodePts.loadBox(editListBox, aMovieTimeScale, mdhdBox.getTimeScale());
        }

        decodePts.unravel();

        PTSSampleMap timeToSample = getTimes(decodePts, (uint32_t) mTrackMeta.timescale.den);
        std::multimap<DecodePts::SampleIndex, PTSTime> sampleToTimes;
        for (auto tTs : timeToSample)
        {
            sampleToTimes.insert(std::make_pair(tTs.second, tTs.first));
        }

        std::uint32_t chunkIndex     = 0;
        std::uint32_t prevChunkIndex = 0;

        uint64_t dataOffset = 0;
        for (std::uint32_t sampleIndex = 0; sampleIndex < sampleSizeBox.getSampleCount(); ++sampleIndex)
        {
            auto ctsRange = sampleToTimes.equal_range(sampleIndex);
            FrameCts cts;
            auto ctsIt = ctsRange.first;
            while (ctsIt != ctsRange.second)
            {
                cts.push_back(ctsIt->second);
                ++ctsIt;
            }
            assert(cts.size() == 1);  // assume this for now
            if (!sampleToChunkBox.getSampleChunkIndex(sampleIndex, chunkIndex))
            {
                throw FailedToReadSampleChunkIndex();
            }
            auto nextCts = timeToSample.find(*cts.begin());
            ++nextCts;
            FrameDuration duration;
            if (nextCts == timeToSample.end())
            {
                duration = FrameDuration();
            }
            else
            {
                duration = nextCts->first.cast<FrameDuration>() - cts.begin()->cast<FrameDuration>();
            }
            if (chunkIndex != prevChunkIndex)
            {
                dataOffset     = chunkOffsets.at(chunkIndex - 1);
                prevChunkIndex = chunkIndex;
            }
            bool isIDR = syncSamples.count(sampleIndex) != 0;
            MOVIEFRAGMENTS::SampleFlags sampleFlags{};
            sampleFlags.flags.sample_is_non_sync_sample = !isIDR;

            FrameInfo frameInfo{cts, duration, isIDR, {sampleFlags.flagsAsUInt}, {}};
            dataOffset += sampleSizes[sampleIndex];
            mHolder->frameInfo.push_back(frameInfo);
            mHolder->frameBlock.push_back(Block(dataOffset, sampleSizes[sampleIndex]));
        }

        // std::cout << "sample chunks: " << std::endl;
        // std::uint32_t sampleIndex = 0;
        // std::uint32_t chunkIndex;
        // while (sampleToChunkBox.getSampleChunkIndex(sampleIndex, chunkIndex))
        // {
        //     mHolder->frameInfo.push_back(Block {});
        //     std::cout << sampleIndex << ":" << chunkIndex << " ";
        //     ++sampleIndex;
        // }
        // std::cout << std::endl;
    }

    Track::~Track()
    {
        // nothing
    }

    FrameData Track::getFrameData(std::size_t aFrameIndex) const
    {
        assert(aFrameIndex < mHolder->frameBlock.size());
        const auto frameBlock = mHolder->frameBlock.at(aFrameIndex);
        const auto storage    = mHolder->mp4.getData(frameBlock);
        return FrameData(storage.begin(), storage.end());
    }

    Frame Track::getFrame(std::size_t aFrameIndex) const
    {
        assert(aFrameIndex < mHolder->frameInfo.size());
        const auto frameInfo  = mHolder->frameInfo.at(aFrameIndex);
        const auto frameBlock = mHolder->frameBlock.at(aFrameIndex);
        const auto storage    = mHolder->mp4.getData(frameBlock);
        return Frame{frameInfo, FrameData(storage.begin(), storage.end())};
    }

    Frames Track::getFrames() const
    {
        Frames frames;
        for (size_t frameIndex = 0; frameIndex < mHolder->frameInfo.size(); ++frameIndex)
        {
            const auto frameInfo = mHolder->frameInfo.at(frameIndex);
            auto acquire =
                std::unique_ptr<AcquireFrameDataFromTrack>(new AcquireFrameDataFromTrack(mHolder, frameIndex));
            frames.push_back(FrameProxy{std::move(acquire), frameInfo});
        }
        return frames;
    }

    TrackMeta Track::getTrackMeta() const
    {
        return mTrackMeta;
    }
}
