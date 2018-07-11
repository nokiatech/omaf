
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
#pragma once

#include <streamsegmenter/segmenterapi.hpp>

#include "common/optional.h"

namespace VDD
{
    static const int MaxNumPlanes = 6;

    using TrackId = StreamSegmenter::TrackId;
    using FrameTime = StreamSegmenter::FrameTime;
    using FrameDuration = StreamSegmenter::FrameDuration;
    using FrameRate = StreamSegmenter::FrameRate;

    typedef std::int64_t Index;
    typedef std::int64_t CodingIndex;

    enum class FrameType
    {
        NA,
        IDR,
        NONIDR,
        // more types directly from the H264 header
    };

    enum class ContentType
    {
        None,
        RAW,
        CODED
    };

    enum class OmafProjectionType
    {
        None,
        EQUIRECTANGULAR,
        CUBEMAP
    };

    enum class RawFormat
    {
        None,
        YUV420,
        RGB888,
        PCM,
        CPUSubDataReferenceTestFormat1,
        CPUSubDataReferenceTestFormat2,
        RawTestFormat1,
        RawTestFormat2,
        RawTestFormat3
    };
    enum class CodedFormat
    {
        None,
        H264,
        H265,
        AAC,
        TimedMetadata,
        H265Extractor
    };

    std::ostream& operator<<(std::ostream&, RawFormat);
    std::istream& operator>>(std::istream&, RawFormat&);

    std::ostream& operator<<(std::ostream&, CodedFormat);
    std::istream& operator >> (std::istream&, CodedFormat&);

    struct RawFormatDescription
    {
        std::uint8_t numPlanes;
        std::uint8_t bitsPerPixel;
        std::uint8_t pixelStrideBits[MaxNumPlanes];
        std::size_t rowStrideOf8[MaxNumPlanes];
    };

    const RawFormatDescription getRawFormatDescription(RawFormat aFormat);

    struct CommonFrameMeta
    {
        Index presIndex;
        FrameDuration duration;
        std::uint32_t width;
        std::uint32_t height;
    };

    struct RawFrameMeta
    {
        Index presIndex;        // presentation index
        FrameTime presTime;
        FrameDuration duration;

        RawFormat format = RawFormat::None;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

    enum class ConfigType {
        SPS,                    // for AVC
        PPS,                    // for AVC
        VPS,                    // for HEVC
        AudioSpecificConfig     // for AAC
    };

    struct Bitrate {
        uint32_t avgBitrate;
        uint32_t maxBitrate;
    };

    struct SegmenterMeta
    {
        // the duration of the produced segment
        FrameDuration segmentDuration;
    };

    // applicable in OMAF
    struct Region
    {
        std::uint32_t projTop;
        std::uint32_t projLeft;
        std::uint32_t projWidth;
        std::uint32_t projHeight;
        int transform;
        std::uint16_t packedTop;
        std::uint16_t packedLeft;
        std::uint16_t packedWidth;
        std::uint16_t packedHeight;
    };
    struct RegionPacking
    {
        bool constituentPictMatching;
        std::uint32_t projPictureWidth;
        std::uint32_t projPictureHeight;
        std::uint16_t packedPictureWidth;
        std::uint16_t packedPictureHeight;

        std::vector<Region> regions;
    };

    // applicable in OMAF
    struct Spherical
    {
        std::int32_t cAzimuth;
        std::int32_t cElevation;
        std::int32_t cTilt;
        std::uint32_t rAzimuth;
        std::uint32_t rElevation;
        //bool interpolate;
    };
    // applicable in OMAF
    struct Quality3d
    {
        // referring to the CodedFrameMeta::SphericalCoverage
        std::uint8_t qualityRank;
        // TODO other parameters
    };

    struct CodedFrameMeta
    {
        Index presIndex;        // presentation index (as in RawFormatMeta)
        CodingIndex codingIndex; // coding index
        FrameTime codingTime;
        FrameTime presTime;
        FrameDuration duration;
        // TODO remove trackId when not needed anymore when Dash can create its own metadata
        TrackId trackId;

        bool inCodingOrder;

        CodedFormat format = CodedFormat::None;

        std::map<ConfigType, std::vector<std::uint8_t>> decoderConfig;

        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::uint8_t channelConfig = 0; // audio channel configuration
        std::uint32_t samplingFrequency = 0; // audio sampling frequency
        Bitrate bitrate = {}; // bitrate information

        FrameType type = FrameType::NA;

        SegmenterMeta segmenterMeta;

        // applicable in OMAF
        OmafProjectionType projection = OmafProjectionType::EQUIRECTANGULAR;
        VDD::Optional<RegionPacking> regionPacking;
        VDD::Optional<Spherical> sphericalCoverage;
        VDD::Optional<Quality3d> qualityRankCoverage;

        bool isIDR() const
        {
            return type == FrameType::IDR;
        }
    };

    /** Attach arbitrary tags to Meta */
    class MetaTag
    {
    public:
        MetaTag() = default;
        virtual ~MetaTag() = default;

        virtual MetaTag* clone() const = 0;
    };

    template <typename Value>
    class ValueTag : public MetaTag
    {
    public:
        ValueTag(Value aTrackId);

        Value get() const;

        MetaTag* clone() const override;
    private:
        Value mValue;
    };

    /** A tag for informing about the track id */
    using TrackIdTag = ValueTag<TrackId>;

    class Meta
    {
    public:
        Meta();
        Meta(const Meta& aMeta);
        Meta(Meta&& aMeta);
        Meta(const RawFrameMeta& aRawFrameMeta);
        Meta(const CodedFrameMeta& aCodedFrameMeta);
        ~Meta() = default;
        Meta& operator=(const Meta&);
        Meta& operator=(Meta&&);

        /** @brief Acquire the content type */
        ContentType getContentType() const;

        /** @brief Get metadata of the frame, such as its time stamps,
            pixel format and index */
        const RawFrameMeta& getRawFrameMeta() const;

        /** @brief Get metadata of the frame, such as its time stamps,
            pixel format and index */
        const CodedFrameMeta& getCodedFrameMeta() const;

        /** @brief Get metadata common for both raw and coded frames,
            such as its time stamps and video dimensions */
        CommonFrameMeta getCommonFrameMeta() const;

        /* Add the given tag (inheriting from MetaTag) to this Data */
        template <typename T> void attachTag(const T& aTag);

        /* is the tag T attached? If so, return it. */
        template <typename T> Optional<T> findTag() const;

    private:
        ContentType mContentType;
        RawFrameMeta mRawFrameMeta;
        CodedFrameMeta mCodedFrameMeta;
        std::list<std::unique_ptr<MetaTag>> mTags;
    };
}

#include "meta.icpp"
