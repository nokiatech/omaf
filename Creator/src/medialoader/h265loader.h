
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
#include <list>
#include <map>
#include <memory>
#include <functional>

#include "medialoader.h"

#include "common/optional.h"
#include "common/exceptions.h"
#include "processor/source.h"

class H265Parser;
class H265InputStream;

namespace VDD
{
    class H265LoaderError : public MediaLoaderException
    {
    public:
        H265LoaderError(std::string aMessage, Optional<std::string> aFilename = Optional<std::string>());
        ~H265LoaderError() override;

        // override this for better exception messages
        virtual std::string message() const override;

    private:
        std::string mMessage;
        Optional<std::string> mFilename;
    };

    class H265Loader;

    class H265StdIStream;

    class H265Source : public MediaSource
    {
    public:
        struct Config
        {
            std::string filename;
            std::shared_ptr<H265InputStream> inputStream; // if set, use this as the input stream
            Optional<FrameDuration> frameDuration;
            bool videoNalStartCodes = false;
            Optional<int> gopLength;  // used for checking that the GOP indeed is the given number,
                                      // as well as implementing getGOPLength
            Optional<size_t> frameLimit;
        };

        H265Source(Config aConfig);
        ~H265Source();

        CodedFormat getFormat() const override;

        Optional<size_t> getNumberOfFrames() const override;

        Optional<FrameDuration> getDuration() const override { return {}; }

        FrameDuration getCurrentFrameDuration() const override;

        FrameRate getFrameRate() const override;

        FrameDuration getTimeScale() const override;

        Dimensions getDimensions() const override;

        int getGOPLength(bool& aIsFixed) const override;

        /* std::list<TrackId> getTrackReferences(const std::string& aFourCC) const override; */

        /* MP4VR::ProjectionFormatProperty getProjection() const override; */

        /* MP4VR::PodvStereoVideoConfiguration getFramePacking() const override; */

        bool getTiles(size_t& aTilesX, size_t& aTilesY) override;
        void getCtuSize(int& aCtuSize) override;

        std::vector<Streams> produce() override;

        void abort() override;

    private:
        Config mConfig;
        std::ifstream mStream; // valid only if Config.inputStream is not provided
        std::shared_ptr<H265InputStream> mH265Stream;

        // the first frame is here ready to be .produced. This is used so that the meta data
        // available from the first frame may be accessed without calling produce
        std::vector<Streams> mFirstFrameQueue;
        bool mHasAcquiredMeta = false;
        struct Meta {
            uint32_t height;
            uint32_t width;
            bool tiles;
            size_t tilesX;
            size_t tilesY;
            int ctuSize;
        } mMeta;

        bool mFirstFrameFound = false;
        bool mFinished = false;
        FrameTime mCodingTime;
        Optional<FrameDuration> mFrameDuration;
        Index mCodingIndex = 0;
        unsigned int mPrevPicOrderCntLsb = 0u;
        int mPrevPicOrderCntMsb = 0;

        // data we got in the previous call to produceDirect, but we had a frame already
        // so we need a place to stick the data to
        struct State {
            Optional<int> poc;
            std::vector<uint8_t> data;
            bool hasRAP = false;
            int largestPoc = 0; // largest seen poc; used for adjusting poc over an IDR frame
            std::map<ConfigType, std::vector<std::uint8_t>> decoderConfig;
            int pocOffset = 0; // define the poc offset when encountering IDR frames

            void reset();
        };
        State mState;

        std::list<H265::SequenceParameterSet> mSpss;
        std::list<H265::PictureParameterSet> mPpss;
        std::list<H265::SequenceParameterSet*> mSpssForH265Parser;
        std::list<H265::PictureParameterSet*> mPpssForH265Parser;
        CodedFrameMeta mCodedFrameMeta;

        Data makeFrame();
        void acquireMeta() const;
        std::vector<Streams> produceDirect();
        void produceToQueue();
        bool frameLimitEncountered() const;
    };

    class H265Loader : public MediaLoader
    {
    public:
        struct Config
        {
            std::string filename;  // If this contains the string "$Number$", it is a template for
                                   // loading multiple segments
            Optional<int> gopLength;         // required got getGopLength(); enforced
            Optional<unsigned> startNumber;  // this is the initial number; if missing, it is 1
        };

        H265Loader(Config aConfig, bool aVideoNalStartCodes = false);
        ~H265Loader();

        std::unique_ptr<MediaSource> sourceForTrack(
            TrackId aTrackId, const SourceConfig& aSourceConfig = defaultSourceConfig) override;

        std::set<TrackId> getTracksByFormat(std::set<CodedFormat> aFormats) override;

        std::set<TrackId> getTracksBy(
            std::function<bool(MediaSource& aSource)> aPredicate) override;

    private:
        Config mConfig;
        bool mVideoNalStartCodes;
    };

    bool getH265Tiles(const Data& aData, size_t& aTilesX, size_t& aTilesY);
    void getH265CtuSize(const Data& aData, int& aCtuSize);
}  // namespace VDD
