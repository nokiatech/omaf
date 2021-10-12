
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
#include "audio.h"

#include <algorithm>
#include <streamsegmenter/segmenterapi.hpp>

#include "config/config.h"
#include "controllerconfigure.h"
#include "controllerops.h"
#include "controllerparsers.h"
#include "dash.h"
#include "medialoader/mp4loader.h"
#include "raw/aacimport.h"

namespace VDD
{
    namespace
    {
        /** @brief Does this object have children that are not objects? */
        bool hasNonObjectChildren(const ConfigValue& aConfig)
        {
            if (aConfig->isObject())
            {
                bool ret = false;
                for (const auto& child : aConfig.childValues())
                {
                    if (!child->isObject())
                    {
                        ret = true;
                        break;
                    }
                }
                return ret;
            }
            else
            {
                return false;
            }
        }

        /** @brief Given an audio input configuration root object and a label, retrieve the
         * labeled audio input. Handles the backwards compatibility with the old audio
         * configuration.
         *
         * Defaults to label "audio" if none provided. */
        ConfigValue audioInputConfigByLabel(ControllerOps& aControllerOps,
                                            const ConfigValue& aConfig,
                                            const ConfigValue& aInputLabel)
        {
            std::string inputLabel = aInputLabel ? readString(aInputLabel) : "audio";
            std::map<std::string, ConfigValue> inputs;
            // heuristics to support the old audio input format
            if (hasNonObjectChildren(aConfig))
            {
                aControllerOps.log(Info) << "Audio input is not configured underneath a labeled "
                                            "input, still supporting it."
                                         << std::endl;
                if (inputLabel == "audio")
                {
                    return aConfig;
                }
                else
                {
                    return aConfig[inputLabel];
                }
            }
            else
            {
                return aConfig[inputLabel];
            }
        }

        Optional<AudioMP4Info> makeAudioMP4VRPipeline(const MP4VROutputs& aMP4VROutputs,
                                                      ControllerConfigure& aConfigure,
                                                      AsyncNode* aAacInput,
                                                      FrameDuration aTimeScale,
                                                      AudioIsOverlay aAudioIsOverlay,
                                                      Optional<PipelineBuildInfo> aPipelineBuildInfo)
        {
            AudioMP4Info info;
            for (auto& output : aMP4VROutputs)
            {
                MP4VRWriter* writer = output.second.get();
                PipelineOutput pipelineOutput{PipelineOutputAudio::Audio,
                                              aAudioIsOverlay.isOverlay};
                MP4VRWriter::Sink sink =
                    writer->createSink({}, pipelineOutput, aTimeScale);
                aConfigure.buildPipeline({}, pipelineOutput,
                                         {{pipelineOutput, {aAacInput, allStreams}}}, sink.sink,
                                         aPipelineBuildInfo);
                for (auto trackId : sink.trackIds)
                {
                    info.output[output.first].trackIds.insert(trackId);
                }
            }
            return info;
        }

        std::list<TrackId> findAudioSourceTracks(MP4Loader& aMP4Loader)
        {
            std::list<TrackId> sources;

            for (auto audioTrackId : aMP4Loader.getTracksByFormat({CodedFormat::AAC}))
            {
                sources.push_back(audioTrackId);
            }

            return sources;
        }

        struct AudioInputInfo
        {
            AsyncNode* aacInput;
            FrameDuration timeScale;
        };

        /** Given the input data from AudioInputInfo (returned ie. by makeAudioFromMP4), configure
         * DASH and MP4VR appropriately. */
        Optional<AudioDashInfo> hookAudioOutput(AudioInputInfo& aAudioInputInfo,
                                                Dash& aDash,
                                                ControllerConfigure& aConfigure,
                                                Optional<PipelineBuildInfo> aPipelineBuildInfo)
        {
            auto dashConfig = aDash.dashConfigFor("media");

            return aConfigure.makeAudioDashPipeline(dashConfig, aDash.getBaseName(),
                                                    aAudioInputInfo.aacInput,
                                                    aAudioInputInfo.timeScale,
                                                    aPipelineBuildInfo);
        }

        Optional<AudioMP4Info> hookAudioOutputMp4(AudioInputInfo& aAudioInputInfo,
                                                  const MP4VROutputs& aMP4VROutputs,
                                                  ControllerConfigure& aConfigure,
                                                  AudioIsOverlay aAudioIsOverlay,
                                                  Optional<PipelineBuildInfo> aPipelineBuildInfo)
        {
            return makeAudioMP4VRPipeline(aMP4VROutputs, aConfigure, aAudioInputInfo.aacInput,
                                          aAudioInputInfo.timeScale, aAudioIsOverlay, aPipelineBuildInfo);
        }

        AudioInputInfo makeAudioFromAAC(std::string aFilename, std::string aLoadLabel,
                                        ControllerOps& aOps)
        {
            AudioInputInfo info{};
            AACImport::Config importConfig{aFilename};
            info.aacInput = aOps.makeForGraph<AACImport>(aLoadLabel, importConfig);

            info.timeScale = {1, 48000};

            return info;
        }

        AudioInputInfo makeAudioFromMP4(MP4Loader& aMP4Loader, TrackId aSourceTrack,
                                        std::string aLoadLabel, ControllerOps& aOps)
        {
            auto audioSource = aMP4Loader.sourceForTrack(aSourceTrack);

            auto timeScale = audioSource->getTimeScale();
            AsyncNode* aacInput = aOps.wrapForGraph(
                aLoadLabel + ":" + Utils::to_string(aSourceTrack), std::move(audioSource));

            return {aacInput, timeScale};
        }

        AudioInputInfo makeAudioInput(ControllerOps& aOps, const ConfigValue& aAudioInput)
        {
            Optional<AudioInputInfo> audioInputInfo;
            // Special handling to support filename-based autodetection of format for
            // audio files
            auto filenameConfig = aAudioInput["filename"];
            if (filenameConfig)
            {
                auto filename = readFilename(filenameConfig);
                std::string loadLabel = "\"" + filename + "\"";

                if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".aac")
                {
                    audioInputInfo = makeAudioFromAAC(filename, loadLabel, aOps);
                }
            }

            if (!audioInputInfo)
            {
                Optional<MediaInputConfig> mediaInputConfig;
                if (filenameConfig)
                {
                    auto filename = readFilename(filenameConfig);
                    if (filename.size() >= 4 && (filename.substr(filename.size() - 4) == ".mp4" ||
                                                 filename.substr(filename.size() - 4) == ".m4a"))
                    {
                        mediaInputConfig = MediaInputConfig{};
                        mediaInputConfig->filename = filename;
                    }
                }
                else
                {
                    // fall back to the general config parser to deal with
                    // segmented input
                    mediaInputConfig = readMediaInputConfig(aAudioInput);
                }

                if (mediaInputConfig)
                {
                    MP4Loader mp4Loader(mediaInputConfig->getMP4LoaderConfig());

                    auto audioSourceTracks = findAudioSourceTracks(mp4Loader);

                    if (audioSourceTracks.size())
                    {
                        // If there are multiple AAC tracks, it typically
                        // means there are multiple languages. However, we
                        // take the first track only for now
                        TrackId sourceTrack = audioSourceTracks.front();
                        audioInputInfo = makeAudioFromMP4(mp4Loader, sourceTrack,
                                                          mediaInputConfig->filename, aOps);
                    }
                }
            }

            if (!audioInputInfo)
            {
                if (filenameConfig)
                {
                    throw ConfigValueInvalid("Unsupported input file name " +
                                                 readString(filenameConfig) +
                                                 ". Only .aac, .mp4 and .m4a are supported.",
                                             filenameConfig);
                }
                else
                {
                    throw ConfigValueInvalid(
                        "Requires valid \"filename\" or \"segmented\" field "
                        "for audio.",
                        filenameConfig);
                }
            }
            return *audioInputInfo;
        }
    }  // anonymous namespace

    Optional<AudioMP4Info> makeAudioMp4(ControllerOps& aOps, ControllerConfigure& aConfigure,
                                        const MP4VROutputs& aMP4VROutputs, const ConfigValue& aAudioInput,
                                        AudioIsOverlay aAudioIsOverlay,
                                        Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        auto info = makeAudioInput(aOps, aAudioInput);
        if (info.aacInput)
        {
            return hookAudioOutputMp4(info, aMP4VROutputs, aConfigure, aAudioIsOverlay, aPipelineBuildInfo);
        }
        else
        {
            return {};
        }
    }

    Optional<AudioDashInfo> makeAudioDash(ControllerOps& aOps, ControllerConfigure& aConfigure,
                                          Dash& aDash, const ConfigValue& aAudioInput,
                                          Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        auto info = makeAudioInput(aOps, aAudioInput);
        if (info.aacInput)
        {
            return hookAudioOutput(info, aDash, aConfigure, aPipelineBuildInfo);
        }
        else
        {
            return {};
        }
    }
}  // namespace VDD
