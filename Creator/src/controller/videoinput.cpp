
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
#include <set>
#include <list>

#include "videoinput.h"
#include "controllerops.h"
#include "controllerconfigure.h"
#include "controllerparsers.h"
#include "configreader.h"
#include "googlevrvideoinput.h"

#include "common/utils.h"
#include "medialoader/mp4loader.h"

namespace VDD
{
    namespace
    {
        const std::map<std::string, VideoInputMode> kVideoInputNameToMode{
            {"mono", VideoInputMode::Mono},
            {"separate", VideoInputMode::Separate},
            {"framepacked", VideoInputMode::TopBottom},
            {"leftright", VideoInputMode::LeftRight},
            {"rightleft", VideoInputMode::RightLeft},
            {"topbottom", VideoInputMode::TopBottom},
            {"bottomtop", VideoInputMode::BottomTop},
            {"temporalinterleaving", VideoInputMode::TemporalInterleaving},
            {"nonvr", VideoInputMode::NonVR},
            {"left-right", VideoInputMode::LeftRight},
            {"right-left", VideoInputMode::RightLeft},
            {"top-bottom", VideoInputMode::TopBottom},
            {"bottom-top", VideoInputMode::BottomTop},
            {"temporal-interleaving", VideoInputMode::TemporalInterleaving},
            {"non-vr", VideoInputMode::NonVR}};

        const std::map<std::string, std::set<VideoInputMode>> kVideoInputNameToModes{
            {"mono", {VideoInputMode::Mono}},
            {"separate", {VideoInputMode::Separate}},
            {"framepacked",
             {VideoInputMode::TopBottom, VideoInputMode::BottomTop, VideoInputMode::LeftRight,
              VideoInputMode::RightLeft}},
            {"sidebyside", {VideoInputMode::LeftRight}},
            {"side-by-side", {VideoInputMode::LeftRight}},
            {"topbottom", {VideoInputMode::TopBottom}},
            {"top-bottom", {VideoInputMode::TopBottom}},
            {"temporal-interleaving", {VideoInputMode::TemporalInterleaving}},
            {"nonvr", {VideoInputMode::NonVR}}};

        const auto kVideoInputModeToName = Utils::reverseMapping(kVideoInputNameToMode);

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

        VideoFilenameConfiguration videoFilenameConfigurationOfFilename(const ParsedValue<MediaInputConfig>& aMP4LoaderConfig,
                                                                        const std::set<TrackId>& aVideoTracks)
        {
            MediaInputConfig mediaInputConfig = *aMP4LoaderConfig;
            auto matches = [&](std::string aPattern) 
            {
                return mediaInputConfig.filename.find(aPattern) != std::string::npos;
            };
            auto makeConfig = [&](VideoInputMode aDirection, std::list<size_t> aTrackIndices) 
            {
                return VideoFilenameConfiguration { aDirection, pickTracks(aVideoTracks, aTrackIndices, aMP4LoaderConfig.getConfigValue()) };
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
            else if (matches("_M."))
            {
                return makeConfig(VideoInputMode::Mono, { 0 });
            }
            else
            {
                // Note! Cubemap support is not required for OMAF HEVI profile
                return makeConfig(VideoInputMode::NonVR, { 0 });
            }
        }

        std::set<VideoInputMode> readVideoInputModesSimplified(const ConfigValue& aValue)
        {
            std::set<VideoInputMode> modes;
            if (aValue->isArray())
            {
                // also support format like: ["framepacked", "sidebyside"]
                for (auto input : readVector(
                         "video inputs",
                         readMapping("video input", kVideoInputNameToModes))(aValue))
                {
                    for (auto mode : input)
                    {
                        modes.insert(mode);
                    }
                }
            }
            else
            {
                std::string modesStr = readString(aValue);
                if (modesStr.find("framepacked") != std::string::npos)
                {
                    modes.insert(VideoInputMode::TopBottom);
                    modes.insert(VideoInputMode::BottomTop);
                    modes.insert(VideoInputMode::LeftRight);
                    modes.insert(VideoInputMode::RightLeft);
                }
                if (modesStr.find("topbottom") != std::string::npos)
                {
                    // OMAF specifies that PackedContentInterpretationType is inferred to be 1, i.e.
                    // left is the first part of the frame
                    modes.insert(VideoInputMode::TopBottom);
                }
                if (modesStr.find("sidebyside") != std::string::npos)
                {
                    // OMAF specifies that PackedContentInterpretationType is inferred to be 1, i.e.
                    // left is the first part of the frame
                    modes.insert(VideoInputMode::LeftRight);
                }
                if (modesStr.find("separate") != std::string::npos)
                {
                    modes.insert(VideoInputMode::Separate);
                }
                if (modesStr.find("nonvr") != std::string::npos)
                {
                    modes.insert(VideoInputMode::NonVR);
                }
            }
            modes.insert(VideoInputMode::Mono);

            return modes;
        }

    } // anonymous namespace

    namespace
    {
        std::map<VideoInputMode, PipelineOutput> kVideoInputModeToPipelineOutput{
            {VideoInputMode::Mono, PipelineOutputVideo::Mono},
            {VideoInputMode::LeftRight, PipelineOutputVideo::SideBySide},
            {VideoInputMode::RightLeft, PipelineOutputVideo::SideBySide},
            {VideoInputMode::TopBottom, PipelineOutputVideo::TopBottom},
            {VideoInputMode::BottomTop, PipelineOutputVideo::TopBottom},
            {VideoInputMode::NonVR, PipelineOutputVideo::NonVR}
            // not supported: VideoInputMode::Separate
        };
    }

    PipelineOutput pipelineOutputOfVideoInputMode(VideoInputMode aInputMode)
    {
        assert(kVideoInputModeToPipelineOutput.count(
            aInputMode));  // bug; VideoInputMode::Separate not supported
        return kVideoInputModeToPipelineOutput.at(aInputMode);
    }

    const std::function<VDD::VideoInputMode(const VDD::ConfigValue&)> readVideoInputMode =
        readMapping("video input", kVideoInputNameToMode,
                    mapConfigValue<std::string, std::string>(readString, Utils::lowercaseString));

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

    VideoFileProperties VideoInput::getVideoFileProperties(const ParsedValue<MediaInputConfig>& aMP4LoaderConfig,
                                                           Optional<FrameDuration> aOverrideFrameDuration)
    {
        VideoFileProperties props{};

        Json::Value root(Json::objectValue);
        auto mediaLoader = aMP4LoaderConfig->makeMediaLoader(false);
        auto videoTracks = mediaLoader->getTracksByFormat({ CodedFormat::H264, CodedFormat::H265 });
        if (videoTracks.size())
        {
            MediaLoader::SourceConfig sourceConfig = MediaLoader::defaultSourceConfig;
            sourceConfig.overrideFrameDuration = aOverrideFrameDuration;
            auto firstVideoTrack = mediaLoader->sourceForTrack(*videoTracks.begin(), sourceConfig);
            MP4Loader* mp4Loader = dynamic_cast<MP4Loader*>(mediaLoader.get());
            props.fps = firstVideoTrack->getFrameRate();
            auto dims = firstVideoTrack->getDimensions();
            props.width = dims.width;
            props.height = dims.height;
            props.timeScale = firstVideoTrack->getTimeScale();
            props.gopDuration.duration = FrameDuration(static_cast<uint64_t>(firstVideoTrack->getGOPLength(props.gopDuration.fixed)), props.fps.asDouble());

            bool omaf = false;
            try
            {
                /* MP4VR::ProjectionFormatProperty omafProjection = */ firstVideoTrack->getProjection();
                omaf = true;
            }
            catch (MP4LoaderError& exn)
            {
                // ignore
            }

            Optional<GoogleVRVideoMetadata> googleVrMetadata = {};
            if (mp4Loader)
            {
                try
                {
                    googleVrMetadata = loadGoogleVRVideoMetadata(*mp4Loader);
                }
                catch (VDD::UnsupportedStereoscopicLayout& exn)
                {
                    std::cerr << exn.what() << " " << exn.message() << std::endl;
                    throw UnsupportedStereoscopicLayout(exn.message());
                }
            }

            if (googleVrMetadata)
            {
                props.stereo = googleVrMetadata->mode != VideoInputMode::Mono;
                props.mode = googleVrMetadata->mode;
            }
            else if (omaf)
            {
                props.mode = videoInputOfProperty(firstVideoTrack->getFramePacking());
            }
            else
            {
                VideoFilenameConfiguration nameConfig = videoFilenameConfigurationOfFilename(aMP4LoaderConfig, videoTracks);
                props.stereo = nameConfig.videoInputMode != VideoInputMode::Mono;
                props.mode = nameConfig.videoInputMode;
            }

            // adjust width/height for frame packed videos to retrieve the size of a single image
            switch (props.mode)
            {
            case VideoInputMode::NonVR:
            case VideoInputMode::Mono:
            case VideoInputMode::Separate:
            case VideoInputMode::TemporalInterleaving:
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

    bool VideoInput::setupMP4VideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
                                        const ConfigValue& aConfig,
                                        VideoProcessingMode aProcessingMode,
                                        Optional<ParsedValue<RefIdLabel>>& aRefIdLabel)
    {
        ParsedValue<MediaInputConfig> mediaInputConfig(aConfig, readMediaInputConfig);
        std::set<VDD::CodedFormat> setOfFormats;
        std::string formats;

        aRefIdLabel = readOptional(readRefIdLabel)(aConfig["ref_id"]);

        if (aConfig["formats"])
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
            std::string inputLabel = "MP4 input:\n" + mediaInputConfig->filename;
            auto mediaLoader(mediaInputConfig->makeMediaLoader((aProcessingMode == VideoProcessingMode::Transcode)));
            MP4Loader* mp4Loader = dynamic_cast<MP4Loader*>(mediaLoader.get());
            auto videoTracks = mediaLoader->getTracksByFormat(setOfFormats);
            if (!videoTracks.size())
            {
                throw UnsupportedVideoInput(mediaInputConfig->filename + " does not contain " + formats + " video.");
            }

            std::set<VideoInputMode> allowedInputModes;
            if (aConfig["modes"])
            {
                allowedInputModes = readVideoInputModesSimplified(aConfig["modes"]);
            }

            if (Optional<GoogleVRVideoMetadata> googleVrMetadata =
                    mp4Loader ? loadGoogleVRVideoMetadata(*mp4Loader)
                              : Optional<GoogleVRVideoMetadata>())
            {
                if (aProcessingMode == VideoProcessingMode::Passthrough &&
                    allowedInputModes.find(googleVrMetadata.get().mode) == allowedInputModes.end())
                {
                    throw UnsupportedVideoInput("Framepacked stereo video is not supported");
                }

                setupGoogleVRVideoInput(aOps, aConfigure, *googleVrMetadata, *mp4Loader, inputLabel,
                                        aProcessingMode);
                return true;
            }
            else
            {
                GenericVRVideo genericVRVideo{};
                genericVRVideo.config = aConfig;
                genericVRVideo.mediaInputConfig = mediaInputConfig;
                genericVRVideo.videoTracks = videoTracks;
                genericVRVideo.allowedInputModes = std::move(allowedInputModes);
                try
                {
                    return setupOMAFVRVideoInput(aOps, aConfigure, genericVRVideo, *mediaLoader,
                                                 inputLabel, aProcessingMode);
                }
                catch (MP4LoaderError& exn)
                {
                    // ignore
                }
                return setupGenericVRVideoInput(aOps, aConfigure, genericVRVideo, *mediaLoader,
                                                inputLabel, aProcessingMode);
            }
        }
        catch (MP4LoaderError& exn)
        {
            throw ConfigValueInvalid("Input file " + mediaInputConfig->filename + " cannot be opened: " + exn.message(), aConfig);
        }

        return false;
    }


    VideoInputMode VideoInput::validateMp4VideoInputConfig(VideoInput::GenericVRVideo aGenericVRVideo, MediaLoader& aMediaLoader, std::list<std::unique_ptr<MediaSource>>& aSources)
    {
        VideoFilenameConfiguration filenameConfig = videoFilenameConfigurationOfFilename(aGenericVRVideo.mediaInputConfig, aGenericVRVideo.videoTracks);
        auto trackNumbers = optionWithDefault(aGenericVRVideo.config, "tracks", readList("tracks", readTrackId), filenameConfig.videoTracks);

        if (trackNumbers.size() < 1)
        {
            throw ConfigValueInvalid("No tracks provided", aGenericVRVideo.config["tracks"]);
        }

        if (trackNumbers.size() > 2)
        {
            throw ConfigValueInvalid("Too many tracks provided", aGenericVRVideo.config["tracks"]);
        }

        VideoInputMode mode = optionWithDefault(aGenericVRVideo.config, "mode", readVideoInputMode, filenameConfig.videoInputMode);
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

            MediaLoader::SourceConfig sourceConfig{};
            aSources.push_back(aMediaLoader.sourceForTrack(trackId, sourceConfig));
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
                                  MediaLoader& aMediaLoader,
                                  std::string aInputLabel,
                                  VideoProcessingMode aProcessingMode)
    {

        std::list<std::unique_ptr<MediaSource>> sources;

        VideoInputMode mode = validateMp4VideoInputConfig(aGenericVRVideo, aMediaLoader, sources);

        AsyncNode* inputLeft {};
        StreamFilter inputLeftMask {};
        AsyncNode* inputRight {};
        StreamFilter inputRightMask {};

        // Needs to be captured before calling buildVideoDecoders, that takes ownership of the pointers
        auto frameDuration = (*sources.begin())->getFrameRate().per1();
        auto timeScale = (*sources.begin())->getTimeScale();
        VideoGOP gopInfo;
        gopInfo.duration = FrameDuration(static_cast<uint64_t>((*sources.begin())->getGOPLength(gopInfo.fixed)), (*sources.begin())->getFrameRate().asDouble());

        if (aProcessingMode == VideoProcessingMode::Passthrough)
        {
            if (buildVideoPassThrough(aOps, aInputLabel, mode, sources,
                inputLeft, inputLeftMask,
                inputRight, inputRightMask))
            {
                aConfigure.setInput(inputLeft, inputRight, mode, frameDuration, timeScale, gopInfo);
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
        MediaLoader& aMediaLoader,
        std::string aInputLabel,
        VideoProcessingMode aProcessingMode)
    {

        std::list<std::unique_ptr<MediaSource>> sources;
        for (auto trackId : aGenericVRVideo.videoTracks)
        {
            if (aGenericVRVideo.videoTracks.count(trackId) == 0)
            {
                throw ConfigValueInvalid("Track " + Utils::to_string(trackId) + " does not exist or isn't  a video track", aGenericVRVideo.config["tracks"]);
            }

            MediaLoader::SourceConfig sourceConfig{};
            sources.push_back(aMediaLoader.sourceForTrack(trackId, sourceConfig));
        }

        VideoInputMode mode;
        // If input has OMAF extractor track(s), we take only one of them. For other OMAF cases there should not be more than 1 video track.
        mode = videoInputOfProperty((*sources.begin())->getFramePacking());

        AsyncNode* inputLeft{};
        StreamFilter inputLeftMask{};
        AsyncNode* inputRight{};
        StreamFilter inputRightMask{};

        // Needs to be captured before calling buildVideoDecoders, that takes ownership of the pointers
        auto frameDuration = (*sources.begin())->getFrameRate().per1();
        auto timeScale = (*sources.begin())->getTimeScale();
        VideoGOP gopInfo;
        gopInfo.duration = FrameDuration(static_cast<uint64_t>((*sources.begin())->getGOPLength(gopInfo.fixed)), (*sources.begin())->getFrameRate().asDouble());

        if (aProcessingMode == VideoProcessingMode::Passthrough)
        {
            if (buildVideoPassThrough(aOps, aInputLabel, mode, sources,
                inputLeft, inputLeftMask,
                inputRight, inputRightMask))
            {
                aConfigure.setInput(inputLeft, inputRight, mode, frameDuration, timeScale, gopInfo);
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
        MediaLoader& aMediaLoader,
        std::string aInputLabel, VideoProcessingMode aProcessingMode)
    {
        assert(aProcessingMode == VideoProcessingMode::Passthrough);

        auto trackId = aGoogleVRVideoMetadata.vrTrackId;
        auto loader = aMediaLoader.sourceForTrack(trackId);
        auto frameDuration = loader->getFrameRate().per1();
        auto timeScale = loader->getTimeScale();
        VideoGOP gopInfo;
        gopInfo.duration = FrameDuration(static_cast<uint64_t>(loader->getGOPLength(gopInfo.fixed)), loader->getFrameRate().asDouble());
        auto source = aOps.wrapForGraph(aInputLabel, std::move(loader));

        // Google VR doesn't support multi-track, so mono and frame-packed can be handled together for passthru mode
        aConfigure.setInput(source, nullptr, aGoogleVRVideoMetadata.mode, frameDuration, timeScale,
                            gopInfo);
    }

    bool VideoInput::buildVideoPassThrough(ControllerOps& aOps, std::string aInputLabel, VideoInputMode aMode,
        std::list<std::unique_ptr<MediaSource>>& aSources,
        AsyncNode*& aInputLeft, StreamFilter& aInputLeftMask,
        AsyncNode*& aInputRight, StreamFilter& aInputRightMask)
    {
        switch (aMode)
        {
        case VideoInputMode::Separate:
        {
            auto sourceIt = aSources.begin();
            auto sourceLeft = aOps.wrapForGraph(aInputLabel, std::move(*sourceIt));
            aInputLeft = sourceLeft;
            aInputLeftMask = allStreams;

            ++sourceIt;
            auto sourceRight = aOps.wrapForGraph(aInputLabel, std::move(*sourceIt));
            aInputRight = sourceRight;
            aInputRightMask = allStreams;
            break;
        }
        default: // mono / framepacked
        {
            auto source = aOps.wrapForGraph(aInputLabel, std::move(*aSources.begin()));
            aInputLeft = source;
            aInputLeftMask = allStreams;
            break;
        }
        }
        return true;
    }

    VideoInputMode videoInputOfProperty(MP4VR::StereoScopic3DProperty aStereoscopic)
    {
        switch (aStereoscopic)
        {
        case MP4VR::StereoScopic3DProperty::MONOSCOPIC:
        {
            return VideoInputMode::Mono;
        }
        case MP4VR::StereoScopic3DProperty::STEREOSCOPIC_TOP_BOTTOM:
        {
            return VideoInputMode::TopBottom;
        }
        case MP4VR::StereoScopic3DProperty::STEREOSCOPIC_LEFT_RIGHT:
        {
            return VideoInputMode::LeftRight;
        }
        default:  // MP4VR::StereoScopic3DProperty::STEREOSCOPIC_STEREOMESH:
            throw UnsupportedStereoscopicLayout("Stereo mesh not supported");
        }
    }

    VideoInputMode videoInputOfProperty(MP4VR::PodvStereoVideoConfiguration aStereoscopic)
    {
        switch (aStereoscopic)
        {
        case MP4VR::MONOSCOPIC:
        {
            return VideoInputMode::Mono;
        }
        case MP4VR::TOP_BOTTOM_PACKING:
        {
            return VideoInputMode::TopBottom;
        }
        case MP4VR::SIDE_BY_SIDE_PACKING:
        {
            return VideoInputMode::LeftRight;
        }
        case MP4VR::TEMPORAL_INTERLEAVING:
        {
            return VideoInputMode::TemporalInterleaving;
        }
        default:
            throw UnsupportedStereoscopicLayout("OMAF framepacking mode not supported");
        }
    }
}
