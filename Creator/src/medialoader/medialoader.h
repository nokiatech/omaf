
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

#include <reader/mp4vrfiledatatypes.h>

#include "processor/source.h"

namespace VDD
{
    struct Dimensions
    {
        uint32_t width;
        uint32_t height;

        Dimensions() = default;
    };

    class MediaLoaderException : public Exception
    {
    public:
        MediaLoaderException(std::string aWhat) : Exception(aWhat)
        {
        }
    };

    class MediaSource : public Source
    {
    public:
        /** @brief Retrieve the data format of the track */
        virtual CodedFormat getFormat() const = 0;

        /** @brief Retrieve the number of frames */
        virtual Optional<size_t> getNumberOfFrames() const = 0;

        /** @brief Accessor for getting the frame duration without using produce, typically called
         * before consuming any frames, when the mFrameIterator still points to the beginning. Used
         * in the Controller's MP4 loader to determine the frame rate in order to calculate the
         * configuration for DASH segmenter and the video encoder's GOP.
         *
         * It is not guaranteed that the frame duration is fixed, so the aforementioned calculations
         * may turn out to be incorrect.
         *
         * The value can be overridden with the optional argument of MP4Loader::sourceForTrack.
         *
         * Returns {0, 0} if the track is empty.
         */
        virtual FrameDuration getCurrentFrameDuration() const = 0;

        /** @brief Retrieve the frame rate of the track in frames per second */
        virtual FrameRate getFrameRate() const = 0;

        /** @brief Retrieve the smallest unit of time used when representing times of this track */
        virtual FrameDuration getTimeScale() const = 0;

        /** @brief Get the width and height of a video track */
        virtual Dimensions getDimensions() const = 0;

        /** @brief Retrieve the complete duration of the media */
        virtual Optional<FrameDuration> getDuration() const = 0;

        virtual int getGOPLength(bool& aIsFixed) const = 0;

        /** @brief Retrieve track references */
        virtual std::list<TrackId> getTrackReferences(const std::string& aFourCC) const;

        virtual MP4VR::ProjectionFormatProperty getProjection() const;

        virtual MP4VR::PodvStereoVideoConfiguration getFramePacking() const;

        /** @brief Retrieve the SphericalVideoV1 property, a Google box */
        virtual Optional<MP4VR::SphericalVideoV1Property> getSphericalVideoV1() const;

        /** @brief Retrieve the SphericalVideoV2 property, a Google box */
        virtual Optional<MP4VR::SphericalVideoV2Property> getSphericalVideoV2() const;

        /** @brief Retrieve the StereoScopic3DProperty property, a Google box */
        virtual Optional<MP4VR::StereoScopic3DProperty> getStereoScopic3D() const;

        virtual bool getTiles(size_t& aTilesX, size_t& aTilesY) = 0;
        virtual void getCtuSize(int& aCtuSize) = 0;
    };

    class MediaLoader
    {
    public:
        struct SourceConfig
        {
            Optional<FrameDuration> overrideFrameDuration;
            Optional<size_t> frameLimit;
        };

        static const SourceConfig defaultSourceConfig;

        MediaLoader();
        virtual ~MediaLoader();

        virtual std::unique_ptr<MediaSource> sourceForTrack(
            TrackId aTrackId, const SourceConfig& aSourceConfig = defaultSourceConfig) = 0;

        virtual std::set<TrackId> getTracksByFormat(std::set<CodedFormat> aFormats) = 0;

        virtual std::set<TrackId> getTracksBy(
            std::function<bool(MediaSource& aSource)> aPredicate) = 0;
    };
}  // namespace VDD
