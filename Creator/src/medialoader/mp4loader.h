
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

#include <fstream>
#include <functional>
#include <map>
#include <memory>

#include "common/exceptions.h"
#include "common/optional.h"
#include "processor/source.h"

#include <reader/mp4vrfiledatatypes.h>
#include "medialoader.h"
#include "mp4vrfilestreamgeneric.h"

namespace MP4VR
{
    class MP4VRFileReaderInterface;
    class MP4VRFileStreamInterface;
    struct TrackInformation;
}  // namespace MP4VR

namespace VDD
{
    enum class VRMetadataVersion : int
    {
        Version0,
        Version1
    };

    class MP4LoaderError : public MediaLoaderException
    {
    public:
        MP4LoaderError(int32_t aCode, Optional<std::string> aFilename = Optional<std::string>());
        ~MP4LoaderError() override;

        // override this for better exception messages
        virtual std::string message() const override;

        int32_t getCode() const;

    private:
        int32_t mCode;
        Optional<std::string> mFilename;
    };

    class MP4Loader;

    struct MP4LoaderFrameInfo;

    struct MP4LoaderSegmentKeeper
    {
        std::unique_ptr<MP4VR::StreamInterface> initSegment;
        std::list<std::unique_ptr<MP4VR::StreamInterface>> mediaSegments;
    };

    class MP4LoaderSource: public MediaSource
    {
    public:
        ~MP4LoaderSource() override;

        friend class MP4Loader;

        /** @brief Retrieve the data format of the track */
        CodedFormat getFormat() const override;

        /** @brief Retrieve the number of frames */
        Optional<size_t> getNumberOfFrames() const override;

        /** @brief Retrieve the number of frames */
        Optional<FrameDuration> getDuration() const override;

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
        FrameDuration getCurrentFrameDuration() const override;

        /** @brief Retrieve the frame rate of the track in frames per second */
        FrameRate getFrameRate() const override;

        /** @brief Retrieve the smallest unit of time used when representing times of this track */
        FrameDuration getTimeScale() const override;

        /** @brief Get the width and height of a video track */
        Dimensions getDimensions() const override;

        int getGOPLength(bool& aIsFixed) const override;

        /** @brief Retrieve track references */
        std::list<TrackId> getTrackReferences(const std::string& aFourCC) const override;

        MP4VR::ProjectionFormatProperty getProjection() const override;

        MP4VR::PodvStereoVideoConfiguration getFramePacking() const override;

        /** @brief Retrieve the SphericalVideoV1 property, a Google box */
        Optional<MP4VR::SphericalVideoV1Property> getSphericalVideoV1() const override;

        /** @brief Retrieve the SphericalVideoV2 property, a Google box */
        Optional<MP4VR::SphericalVideoV2Property> getSphericalVideoV2() const override;

        /** @brief Retrieve the StereoScopic3DProperty property, a Google box */
        Optional<MP4VR::StereoScopic3DProperty> getStereoScopic3D() const override;

        bool getTiles(size_t& aTilesX, size_t& aTilesY) override;
        void getCtuSize(int& aCtuSize) override;

        std::vector<Streams> produce() override;

    private:
        MP4LoaderSource(std::shared_ptr<MP4VR::MP4VRFileReaderInterface> aReader,
                        TrackId aTrackId,
                        const MP4VR::TrackInformation& aTrackInfo,
                        std::list<MP4LoaderFrameInfo>&& aFrames,
                        bool aVideoNalStartCodes,
                        size_t aFrameLimit,
                        std::shared_ptr<MP4LoaderSegmentKeeper> aSegmentKeeper);

        /* Used only to ensure the reader object has access to the loaded segments
         * This is not set for the single file case */
        std::shared_ptr<MP4LoaderSegmentKeeper> mSegmentKeeper;

        std::shared_ptr<MP4VR::MP4VRFileReaderInterface> mReader;
        TrackId mTrackId;
        std::unique_ptr<MP4VR::TrackInformation> mTrackInfo;
        std::list<MP4LoaderFrameInfo> mFrames;
        std::list<MP4LoaderFrameInfo>::iterator mFrameIterator;
        bool mVideoNalStartCodes;
        size_t mFrameCounter = 0u;
        size_t mFrameLimit = ~0u;
        FrameDuration mDuration;
    };

    class MP4Loader : public MediaLoader
    {
    public:
        struct Config
        {
            std::string filename; // if reading segmented input, this is the template
            Optional<std::string> initFilename; // if reading segmented input, this is the initialization segment
            Optional<unsigned> startNumber; // if reading segmented input, this is the initial number; if missing, it is 1
        };

        MP4Loader(Config aConfig, bool aVideoNalStartCodes = false);
        ~MP4Loader();

        std::unique_ptr<MediaSource> sourceForTrack(
            TrackId aTrackId, const SourceConfig& aSourceConfig = defaultSourceConfig) override;

        std::set<TrackId> getTracksByFormat(std::set<CodedFormat> aFormats) override;

        std::set<TrackId> getTracksBy(
            std::function<bool(MediaSource& aSource)> aPredicate) override;

    private:
        // mSegments is passed on to MP4LoaderSource for reference-keeping
        std::shared_ptr<MP4LoaderSegmentKeeper> mSegmentKeeper;
        std::shared_ptr<MP4VR::MP4VRFileReaderInterface> mReader;
        bool mVideoNalStartCodes;
    };
}

