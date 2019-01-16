
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
#include <set>
#include <list>

#include "videoinput.h"
#include "controllerops.h"
#include "controllerconfigure.h"
#include "configreader.h"
#include "googlevrvideoinput.h"

#include "common/utils.h"
#include "mp4loader/mp4loader.h"

namespace VDD
{
    namespace
    {
        const std::map<std::string, VideoInputMode> kVideoInputNameToMode {
            { "mono", VideoInputMode::Mono },
            { "separate", VideoInputMode::Separate },
            { "framepacked", VideoInputMode::TopBottom },
            { "left-right", VideoInputMode::LeftRight },
            { "right-left", VideoInputMode::RightLeft },
            { "top-bottom", VideoInputMode::TopBottom },
            { "bottom-top", VideoInputMode::BottomTop }
        };

        const auto kVideoInputModeToName = Utils::reverseMapping(kVideoInputNameToMode);

        const auto readVideoInputMode = readMapping("video input", kVideoInputNameToMode);

        // Given a set of video track ids and a list of indices to said set converted into a list,
        // return a list of video track ids
        std::list<TrackId> pickTracks(const std::set<TrackId>& aVideoTracks, std::list<size_t> aTrackIndices, const ConfigValue& aConfig)
        {
            std::list<TrackId> tracks;
            for (auto trackIndex: aTrackIndices)
            {
                if (trackIndex >= aVideoTracks.size())
                {
                    throw ConfigValueInvalid("Insufficient number of video tracks for given input filename", aConfig);
                }
                auto it = aVideoTracks.begin();
                for (size_t n = 0; n < trackIndex; ++n)
                {
                    ++it;
                }
                tracks.push_back(*it);
            }
            return tracks;
        }

        VideoFilenameConfiguration videoFilenameConfigurationOfFilename(const ConfigValue& aFilenameConfig,
                                                                        const std::set<TrackId>& aVideoTracks)
        {
            std::string filename = readString(aFilenameConfig);
            auto matches = [&](std::string aPattern) 
            {
                return filename.find(aPattern) != std::string::npos;
            };
            auto makeConfig = [&](VideoInputMode aDirection, std::list<size_t> aTrackIndices) 
            {
                return VideoFilenameConfiguration { aDirection, pickTracks(aVideoTracks, aTrackIndices, aFilenameConfig) };
            };
            if (matches("_TB.") || matches("_TB_") || matches("_UD."))
            {
                return makeConfig(VideoInputMode::TopBottom, { 0 });
            }
            else if (matches("_BT."))
            {
                return makeConfig(VideoInputMode::BottomTop, { 0 });
            }
            else if (matches("_LR."))
            {
                return makeConfig(VideoInputMode::LeftRight, { 0 });
            }
            else if (matches("_RL."))
            {
                return makeConfig(VideoInputMode::RightLeft, { 0 });
            }
            else if (matches("_2LR."))
            {
                return makeConfig(VideoInputMode::Separate, { 0, 1 });
            }
            else if (matches("_2RL."))
            {
                return makeConfig(VideoInputMode::Separate, { 1, 0 });
            }
            else
            {
                // handles also the _M case. Note! Cubemap support is not required for OMAF HEVI profile
                return makeConfig(VideoInputMode::Mono, { 0 });
            }
        }

    } // anonymous namespace

    UnsupportedVideoInput::UnsupportedVideoInput(std::string aMessage)
        : Exception("UnsupportedVideoInput")
        , mMessage(aMessage)
    {
        // nothing
    }

    std::string UnsupportedVideoInput::message() const
    {
        return "Unsupported input video: " + mMessage;
    }

    IndeterminateVideoPacking::IndeterminateVideoPacking()
        : Exception("IndeterminateVideoPacking")
    {
        // nothing
    }

    std::ostream& operator<<(std::ostream& aStream, VideoInputMode aMode)
    {
        aStream << kVideoInputModeToName.at(aMode);
        return aStream;
    }


    VideoInput::VideoInput()
    {
    }

    VideoInput::~VideoInput() = default;

    VideoFileProperties VideoInput::getVideoFileProperties(std::string aFilename)
    {
        VideoFileProperties props{};

        Json::Value root(Json::objectValue);
        MP4Loader::Config mp4LoaderConfig{ aFilename };
        MP4Loader mp4Loader(mp4LoaderConfig);
        auto videoTracks = mp4Loader.getTracksByFormat({ CodedFormat::H264, CodedFormat::H265 });
        if (videoTracks.size())
        {
            auto firstVideoTrack = mp4Loader.sourceForTrack(*videoTracks.begin());
            props.fps = firstVideoTrack->getFrameRate();
            auto dims = firstVideoTrack->getDimensions();
            props.width = dims.width;
            props.height = dims.height;
            props.timeScale = firstVideoTrack->getTimeScale();
            
            props.gopDuration.duration = FrameDuration(firstVideoTrack->getGOPLength(props.gopDuration.fixed), props.fps.asDouble());

            bool omaf = false;
            try
            {
                MP4VR::ProjectionFormatProperty omafProjection = firstVideoTrack->getProjection();
                omaf = true;
            }
            catch (MP4LoaderError& exn)
            {
                // ignore
            }

            Optional<GoogleVRVideoMetadata> googleVrMetadata = {};
            try
            {
                googleVrMetadata = loadGoogleVRVideoMetadata(mp4Loader);
            }
            catch (VDD::UnsupportedStereoscopicLayout& exn) {
                std::cerr << exn.what() << " " << exn.message() << std::endl;
                throw UnsupportedStereoscopicLayout(exn.message());
            }

            if (googleVrMetadata)
            {
                props.stereo = googleVrMetadata->mode != VideoInputMode::Mono;
                props.mode = googleVrMetadata->mode;
            }
            else if (omaf)
            {
                MP4VR::PodvStereoVideoConfiguration stereo = firstVideoTrack->getFramePacking();
                switch (stereo)
                {
                case MP4VR::PodvStereoVideoConfiguration::MONOSCOPIC:
                {
                    props.mode = VideoInputMode::Mono;
                }
                break;
                case MP4VR::PodvStereoVideoConfiguration::TOP_BOTTOM_PACKING:
                {
                    props.mode = VideoInputMode::TopBottom;
                }
                break;
                case MP4VR::PodvStereoVideoConfiguration::SIDE_BY_SIDE_PACKING:
                {
                    props.mode = VideoInputMode::LeftRight;
                }
                break;
                default:
                    throw UnsupportedStereoscopicLayout("OMAF framepacking mode not supported");
                }
            }
            else
            {
                Config config;
                config.setKeyValue("filename", aFilename);
                VideoFilenameConfiguration nameConfig = videoFilenameConfigurationOfFilename(config["filename"], videoTracks);
                props.stereo = nameConfig.videoInputMode != VideoInputMode::Mono;
                props.mode = nameConfig.videoInputMode;
            }

            // adjust width/height for frame packed videos to retrieve the size of a single image
            switch (props.mode)
            {
            case VideoInputMode::Mono:
            case VideoInputMode::Separate:
            {
                break;
            }
            case VideoInputMode::LeftRight:
            case VideoInputMode::RightLeft:
            {
                props.width /= 2;
                break;
            }
            case VideoInputMode::TopBottom:
            case VideoInputMode::BottomTop:
            {
                props.height /= 2;
                break;
            }
            }

            props.tiles = firstVideoTrack->getTiles(props.tilesX, props.tilesY);
            firstVideoTrack->getCtuSize(props.ctuSize);
        }
        return props;
    }

    bool VideoInput::setupMP4VideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure, const ConfigValue& aConfig, VideoProcessingMode aProcessingMode)
    {
        auto filename = aConfig["filename"];
        MP4Loader::Config mp4LoaderConfig{ readString(filename) };
        std::set<VDD::CodedFormat> setOfFormats;
        std::string formats;
        if (aConfig["formats"].valid())
        {
            formats = readString(aConfig["formats"]);
            if (formats.find("H264") != std::string::npos)
            {
                setOfFormats.insert(setOfFormats.end(), CodedFormat::H264);
            }
            if (formats.find("H265") != std::string::npos)
            {
                setOfFormats.insert(setOfFormats.end(), CodedFormat::H265);
                setOfFormats.insert(setOfFormats.end(), CodedFormat::H265Extractor);
            }
        }
        else
        {
            // allow H264 and H265
            setOfFormats = { CodedFormat::H264, CodedFormat::H265, CodedFormat::H265Extractor };
            formats = "H264 or H265";
        }
        try
        {
            std::string inputLabel = "MP4 input:\n" + readString(filename);
            MP4Loader mp4Loader(mp4LoaderConfig, (aProcessingMode == VideoProcessingMode::Transcode));
            auto videoTracks = mp4Loader.getTracksByFormat(setOfFormats);
            if (!videoTracks.size())
            {
                throw UnsupportedVideoInput(readString(filename) + " does not contain " + formats + " video.");
            }

            std::set<VideoInputMode> allowedInputModes;
            if (aConfig["modes"].valid())
            {
                allowedInputModes.insert(VideoInputMode::Mono);
                std::string modes = readString(aConfig["modes"]);
                if (modes.find("framepacked") != std::string::npos)
                {
                    allowedInputModes.insert(VideoInputMode::TopBottom);
                    allowedInputModes.insert(VideoInputMode::BottomTop);
                    allowedInputModes.insert(VideoInputMode::LeftRight);
                    allowedInputModes.insert(VideoInputMode::RightLeft);
                }
                if (modes.find("topbottom") != std::string::npos)
                {
                    // OMAF specifies that PackedContentInterpretationType is inferred to be 1, i.e. left is the first part of the frame
                    allowedInputModes.insert(VideoInputMode::TopBottom);
                }
                if (modes.find("sidebyside") != std::string::npos)
                {
                    // OMAF specifies that PackedContentInterpretationType is inferred to be 1, i.e. left is the first part of the frame
                    allowedInputModes.insert(VideoInputMode::LeftRight);
                }
                if (modes.find("separate") != std::string::npos)
                {
                    allowedInputModes.insert(VideoInputMode::Separate);
                }
            }

            if (Optional<GoogleVRVideoMetadata> googleVrMetadata = loadGoogleVRVideoMetadata(mp4Loader))
            {
                if (aProcessingMode == VideoProcessingMode::Passthrough && allowedInputModes.find(googleVrMetadata.get().mode) == allowedInputModes.end())
                {
                    throw UnsupportedVideoInput("Framepacked stereo video is not supported");
                }

                setupGoogleVRVideoInput(aOps, aConfigure, *googleVrMetadata, mp4Loader, inputLabel, aProcessingMode);
                return true;
            }
            else
            {
                GenericVRVideo genericVRVideo{};
                genericVRVideo.config = aConfig;
                genericVRVideo.filename = filename;
                genericVRVideo.videoTracks = videoTracks;
                genericVRVideo.allowedInputModes = std::move(allowedInputModes);
                try
                {
                    return setupOMAFVRVideoInput(aOps, aConfigure, genericVRVideo, mp4Loader, inputLabel, aProcessingMode);
                }
                catch (MP4LoaderError& exn)
                {
                    // ignore
                }
                return setupGenericVRVideoInput(aOps, aConfigure, genericVRVideo, mp4Loader, inputLabel, aProcessingMode);
            }
        }
        catch (MP4LoaderError& exn)
        {
            throw ConfigValueInvalid("Input file " + readString(filename) + " cannot be opened: " + exn.message(), filename);
        }

        return false;
    }


    VideoInputMode VideoInput::validateMp4VideoInputConfig(VideoInput::GenericVRVideo aGenericVRVideo, MP4Loader& aMP4Loader, std::list<std::unique_ptr<MP4LoaderSource>>& aSources)
    {
        VideoFilenameConfiguration filenameConfig = videoFilenameConfigurationOfFilename(aGenericVRVideo.filename, aGenericVRVideo.videoTracks);
        auto trackNumbers = optionWithDefault(aGenericVRVideo.config, "tracks", "tracks to load", readList("tracks", readTrackId), filenameConfig.videoTracks);

        if (trackNumbers.size() < 1)
        {
            throw ConfigValueInvalid("No tracks provided", aGenericVRVideo.config["tracks"]);
        }

        if (trackNumbers.size() > 2)
        {
            throw ConfigValueInvalid("Too many tracks provided", aGenericVRVideo.config["tracks"]);
        }

        VideoInputMode mode = optionWithDefault(aGenericVRVideo.config, "mode", "video input mode", readVideoInputMode, filenameConfig.videoInputMode);
        if (!aGenericVRVideo.allowedInputModes.empty())
        {
            if (aGenericVRVideo.allowedInputModes.find(mode) == aGenericVRVideo.allowedInputModes.end())
            {
                throw UnsupportedVideoInput("Stereo video is not supported");
            }
        }
        // else allow all

        for (auto trackId : trackNumbers)
        {
            if (aGenericVRVideo.videoTracks.count(trackId) == 0)
            {
                throw ConfigValueInvalid("Track " + Utils::to_string(trackId) + " does not exist or isn't  a video track", aGenericVRVideo.config["tracks"]);
            }

            MP4Loader::SourceConfig sourceConfig{};
            aSources.push_back(aMP4Loader.sourceForTrack(trackId, sourceConfig));
        }
        assert(aSources.size() >= 1); // because trackNumbers.size() == [1, 2]

        if (mode == VideoInputMode::Separate && aSources.size() < 2)
        {
            throw ConfigValueInvalid("Video input mode " + mappingToString(kVideoInputNameToMode, mode) + " requires two tracks", aGenericVRVideo.config["mode"]);
        }

        return mode;
    }

    bool VideoInput::setupGenericVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
                                  VideoInput::GenericVRVideo aGenericVRVideo,
                                  MP4Loader& aMP4Loader,
                                  std::string aInputLabel,
                                  VideoProcessingMode aProcessingMode)
    {

        std::list<std::unique_ptr<MP4LoaderSource>> sources;

        VideoInputMode mode = validateMp4VideoInputConfig(aGenericVRVideo, aMP4Loader, sources);

        AsyncNode* inputLeft {};
        ViewMask inputLeftMask {};
        AsyncNode* inputRight {};
        ViewMask inputRightMask {};

        // Needs to be captured before calling buildVideoDecoders, that takes ownership of the pointers
        auto frameDuration = (*sources.begin())->getFrameRate().per1();
        auto timeScale = (*sources.begin())->getTimeScale();
        VideoGOP gopInfo;
        gopInfo.duration = FrameDuration((*sources.begin())->getGOPLength(gopInfo.fixed), (*sources.begin())->getFrameRate().asDouble());

        if (aProcessingMode == VideoProcessingMode::Passthrough)
        {
            if (buildVideoPassThrough(aOps, aInputLabel, mode, sources,
                inputLeft, inputLeftMask,
                inputRight, inputRightMask))
            {
                aConfigure.setInput(inputLeft, inputLeftMask,
                    inputRight, inputRightMask,
                    mode,
                    frameDuration,
                    timeScale,
                    gopInfo,
                    mode != VideoInputMode::Mono);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    bool VideoInput::setupOMAFVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
        VideoInput::GenericVRVideo aGenericVRVideo,
        MP4Loader& aMP4Loader,
        std::string aInputLabel,
        VideoProcessingMode aProcessingMode)
    {

        std::list<std::unique_ptr<MP4LoaderSource>> sources;
        for (auto trackId : aGenericVRVideo.videoTracks)
        {
            if (aGenericVRVideo.videoTracks.count(trackId) == 0)
            {
                throw ConfigValueInvalid("Track " + Utils::to_string(trackId) + " does not exist or isn't  a video track", aGenericVRVideo.config["tracks"]);
            }

            MP4Loader::SourceConfig sourceConfig{};
            sources.push_back(aMP4Loader.sourceForTrack(trackId, sourceConfig));
        }

        VideoInputMode mode = VideoInputMode::Mono;
        // If input has OMAF extractor track(s), we take only one of them. For other OMAF cases there should not be more than 1 video track.
        MP4VR::PodvStereoVideoConfiguration stereo = (*sources.begin())->getFramePacking();
        switch (stereo)
        {
        case MP4VR::PodvStereoVideoConfiguration::MONOSCOPIC:
        {
            mode = VideoInputMode::Mono;
        }
        break;
        case MP4VR::PodvStereoVideoConfiguration::TOP_BOTTOM_PACKING:
        {
            mode = VideoInputMode::TopBottom;
        }
        break;
        case MP4VR::PodvStereoVideoConfiguration::SIDE_BY_SIDE_PACKING:
        {
            mode = VideoInputMode::LeftRight;
        }
        break;
        default:
            throw UnsupportedStereoscopicLayout("OMAF framepacking mode not supported");
        }


        AsyncNode* inputLeft{};
        ViewMask inputLeftMask{};
        AsyncNode* inputRight{};
        ViewMask inputRightMask{};

        // Needs to be captured before calling buildVideoDecoders, that takes ownership of the pointers
        auto frameDuration = (*sources.begin())->getFrameRate().per1();
        auto timeScale = (*sources.begin())->getTimeScale();
        VideoGOP gopInfo;
        gopInfo.duration = FrameDuration((*sources.begin())->getGOPLength(gopInfo.fixed), (*sources.begin())->getFrameRate().asDouble());

        if (aProcessingMode == VideoProcessingMode::Passthrough)
        {
            if (buildVideoPassThrough(aOps, aInputLabel, mode, sources,
                inputLeft, inputLeftMask,
                inputRight, inputRightMask))
            {
                aConfigure.setInput(inputLeft, inputLeftMask,
                    inputRight, inputRightMask,
                    mode,
                    frameDuration,
                    timeScale,
                    gopInfo,
                    mode != VideoInputMode::Mono);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    /** @brief Setup the input pipeline according to the Google VR Video metadata */
    void VideoInput::setupGoogleVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
        GoogleVRVideoMetadata aGoogleVRVideoMetadata,
        MP4Loader& aMP4Loader,
        std::string aInputLabel, VideoProcessingMode aProcessingMode)
    {
        assert(aProcessingMode == VideoProcessingMode::Passthrough);

        auto trackId = aGoogleVRVideoMetadata.vrTrackId;
        auto loader = aMP4Loader.sourceForTrack(trackId);
        auto frameDuration = loader->getFrameRate().per1();
        auto timeScale = loader->getTimeScale();
        VideoGOP gopInfo;
        gopInfo.duration = FrameDuration(loader->getGOPLength(gopInfo.fixed), loader->getFrameRate().asDouble());
        auto source = aOps.wrapForGraph(aInputLabel, std::move(loader));

        // Google VR doesn't support multi-track, so mono and frame-packed can be handled together for passthru mode
        aConfigure.setInput(source, allViews,
            nullptr, 0,
            aGoogleVRVideoMetadata.mode,
            frameDuration,
            timeScale,
            gopInfo,
            aGoogleVRVideoMetadata.mode != VideoInputMode::Mono);
    }

    bool VideoInput::buildVideoPassThrough(ControllerOps& aOps, std::string aInputLabel, VideoInputMode aMode,
        std::list<std::unique_ptr<MP4LoaderSource>>& aSources,
        AsyncNode*& aInputLeft, ViewMask& aInputLeftMask,
        AsyncNode*& aInputRight, ViewMask& aInputRightMask)
    {
        switch (aMode)
        {
        case VideoInputMode::Separate:
        {
            auto sourceIt = aSources.begin();
            auto sourceLeft = aOps.wrapForGraph(aInputLabel, std::move(*sourceIt));
            aInputLeft = sourceLeft;
            aInputLeftMask = allViews;

            ++sourceIt;
            auto sourceRight = aOps.wrapForGraph(aInputLabel, std::move(*sourceIt));
            aInputRight = sourceRight;
            aInputRightMask = allViews;
            break;
        }
        default: // mono / framepacked
        {
            auto source = aOps.wrapForGraph(aInputLabel, std::move(*aSources.begin()));
            aInputLeft = source;
            aInputLeftMask = allViews;
            break;
        }
        }
        return true;
    }
}
