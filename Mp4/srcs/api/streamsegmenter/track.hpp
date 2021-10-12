
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
#ifndef STREAMSEGMENTER_FRAMEACCESS_HPP
#define STREAMSEGMENTER_FRAMEACCESS_HPP

#include "frame.hpp"
#include "frameproxy.hpp"
#include "id.hpp"
#include "rational.hpp"

class TrackBox;

namespace StreamSegmenter
{
    class MP4Access;

    typedef IdBase<std::uint32_t, struct TrackIdTag> TrackId;

    class FailedToReadSampleChunkIndex : public std::runtime_error
    {
    public:
        FailedToReadSampleChunkIndex()
            : std::runtime_error("FailedToReadSampleChunkIndex")
        {
        }
    };

    class Track;

    typedef std::list<FrameProxy> Frames;

    enum class MediaType
    {
        Video,
        Audio,
        Data,  // ISOBMFF 12.3.2 uses 'meta' handler type and null media header (nmhd)
        Other
    };

    struct BrandSpec
    {
        std::string majorBrand;
        std::uint32_t minorVersion;
        std::vector<std::string> compatibleBrands;
    };

    struct TrackMeta
    {
        TrackId trackId;
        RatU64 timescale;
        MediaType type;
        ISOBMFF::Optional<BrandSpec> trackType;
        FrameTime dtsCtsOffset; /* if non-zero, a positive number that will reproduced as an edit
                                   list skip in the beginning of the track */
    };

    class Track
    {
    public:
        Track(const MP4Access& aMp4, const TrackBox& aTrackBox, const uint32_t aMovieTimeScale);

        Track(const Track& other) = delete;
        Track(Track&& other);
        void operator=(const Track& other) = delete;
        void operator=(Track&& other) = delete;

        virtual ~Track();

        Frame getFrame(std::size_t aFrameIndex) const;
        FrameData getFrameData(std::size_t aFrameIndex) const;

        Frames getFrames() const;

        TrackMeta getTrackMeta() const;

    private:
        struct Holder;

        class AcquireFrameDataFromTrack : public AcquireFrameData
        {
        public:
            AcquireFrameDataFromTrack(std::shared_ptr<Track::Holder> aHolder, size_t aFrameIndex);

            ~AcquireFrameDataFromTrack();

            FrameData get() const override;
            size_t getSize() const override;

            AcquireFrameDataFromTrack* clone() const override;

        private:
            std::shared_ptr<Track::Holder> mHolder;
            size_t mFrameIndex;
        };

        friend class AcquireFrameDataFromTrack;

        std::shared_ptr<Holder> mHolder;
        TrackMeta mTrackMeta;
    };
}  // namespace StreamSegmenter

#endif  // STREAMSEGMENTER_FRAMEACCESS_HPP
