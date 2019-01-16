
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
#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <functional>

#include "common/optional.h"
#include "common/exceptions.h"
#include "processor/source.h"

#include <mp4vrfiledatatypes.h>

namespace MP4VR
{
    class MP4VRFileReaderInterface;
    struct TrackInformation;
}

namespace VDD
{
    enum class VRMetadataVersion : int
    {
        Version0,
        Version1
    };

    class MP4LoaderError : public Exception
    {
    public:
        MP4LoaderError(int32_t aCode);
        ~MP4LoaderError() override;

        // override this for better exception messages
        virtual std::string message() const override;

    private:
        int32_t mCode;
    };

    class MP4Loader;

    struct MP4LoaderFrameInfo;

    struct Dimensions
    {
        uint32_t width;
        uint32_t height;

        Dimensions() = default;
    };

    class MP4LoaderSource: public Source
    {
    public:
        ~MP4LoaderSource() override;

        friend class MP4Loader;

        /** @brief Retrieve the data format of the track */
        CodedFormat getFormat() const;

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
        FrameDuration getCurrentFrameDuration() const;

        /** @brief Retrieve the frame rate of the track in frames per second */
        FrameRate getFrameRate() const;

        /** @brief Retrieve the smallest unit of time used when representing times of this track */
        FrameDuration getTimeScale() const;

        /** @brief Get the width and height of a video track */
        Dimensions getDimensions() const;

        int getGOPLength(bool& aIsFixed) const;

        /** @brief Retrieve track references */
        std::list<TrackId> getTrackReferences(const std::string& aFourCC) const;

        MP4VR::ProjectionFormatProperty getProjection() const;

        MP4VR::PodvStereoVideoConfiguration getFramePacking() const;

        /** @brief Retrieve the SphericalVideoV1 property, a Google box */
        Optional<MP4VR::SphericalVideoV1Property> getSphericalVideoV1() const;

        /** @brief Retrieve the SphericalVideoV2 property, a Google box */
        Optional<MP4VR::SphericalVideoV2Property> getSphericalVideoV2() const;

        /** @brief Retrieve the StereoScopic3DProperty property, a Google box */
        Optional<MP4VR::StereoScopic3DProperty> getStereoScopic3D() const;

        bool getTiles(int& aTilesX, int& aTilesY);
        void getCtuSize(int& aCtuSize);

        std::vector<Views> produce() override;

    private:
        MP4LoaderSource(std::shared_ptr<MP4VR::MP4VRFileReaderInterface> aReader,
                        TrackId aTrackId,
                        const MP4VR::TrackInformation& aTrackInfo,
                        std::list<MP4LoaderFrameInfo>&& aFrames,
                        bool aVideoNalStartCodes);

        std::shared_ptr<MP4VR::MP4VRFileReaderInterface> mReader;
        TrackId mTrackId;
        std::unique_ptr<MP4VR::TrackInformation> mTrackInfo;
        std::list<MP4LoaderFrameInfo> mFrames;
        std::list<MP4LoaderFrameInfo>::iterator mFrameIterator;
        bool mVideoNalStartCodes;
    };

    class MP4Loader
    {
    public:
        struct SourceConfig
        {
            Optional<FrameDuration> overrideFrameDuration;
        };

        static SourceConfig defaultSourceConfig;

        struct Config
        {
            std::string filename;
        };

        MP4Loader(Config aConfig, bool aVideoNalStartCodes = false);
        ~MP4Loader();

        std::unique_ptr<MP4LoaderSource> sourceForTrack(TrackId aTrackId, 
                                                        SourceConfig aSourceConfig = defaultSourceConfig);

        std::set<TrackId> getTracksByFormat(std::set<CodedFormat> aFormats);

        std::set<TrackId> getTracksBy(std::function<bool(MP4LoaderSource& aSource)> aPredicate);

    private:
        std::shared_ptr<MP4VR::MP4VRFileReaderInterface> mReader;
        bool mVideoNalStartCodes;
    };
}

