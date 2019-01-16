
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
#include <algorithm>

#include <streamsegmenter/segmenterapi.hpp>

#include "audio.h"
#include "controllerconfigure.h"
#include "controllerops.h"
#include "dash.h"

#include "config/config.h"
#include "mp4loader/mp4loader.h"
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

        /** @brief Given an audio input configuration root object and a label, retrieve the labeled audio input.
         * Handles the backwards compatibility with the old audio configuration.
         *
         * Defaults to label "audio" if none provided. */
        ConfigValue audioInputConfigByLabel(ControllerOps& aControllerOps, const ConfigValue& aConfig, const ConfigValue& aInputLabel)
        {
            std::string inputLabel = aInputLabel.valid() ? readString(aInputLabel) : "audio";
            std::map<std::string, ConfigValue> inputs;
            // heuristics to support the old audio input format
            if (hasNonObjectChildren(aConfig))
            {
                aControllerOps.log(Info) << "Audio input is not configured underneath a labeled input, still supporting it." << std::endl;
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

        void makeAudioMP4VRPipeline(const MP4VROutputs& aMP4VROutputs,
                                    ControllerConfigure& aConfigure,
                                    AsyncNode* aAacInput,
                                    FrameDuration aTimeScale)
        {
            for (auto& output : aMP4VROutputs)
            {
                MP4VRWriter* writer = output.second.get();
                aConfigure.buildPipeline("", {},
                                         PipelineOutput::Audio,
                                         { { PipelineOutput::Audio, { aAacInput, allViews } } },
                                         writer->createSink({}, PipelineOutput::Audio, aTimeScale, FrameDuration()));

            }
        }

        std::list<TrackId> findAudioSourceTracks(MP4Loader& aMP4Loader)
        {
            std::list<TrackId> sources;

            for (auto audioTrackId : aMP4Loader.getTracksByFormat({ CodedFormat::AAC }))
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
        void hookAudioOutput(AudioInputInfo& aAudioInputInfo,
                             Dash& aDash,
                             ControllerConfigure& aConfigure)
        {

            if (aDash.isEnabled())
            {
                auto dashConfig = aDash.dashConfigFor("media");

                std::string mpdName = readString(aDash.dashConfigFor("mpd")["filename"]);
                std::string baseName = Utils::replace(mpdName, ".mpd", "");
                aConfigure.makeAudioDashPipeline(dashConfig, baseName,
                                                 aAudioInputInfo.aacInput, 
                                                 aAudioInputInfo.timeScale);
            }
        }

        void hookAudioOutputMp4(AudioInputInfo& aAudioInputInfo,
            const MP4VROutputs& aMP4VROutputs,
            ControllerConfigure& aConfigure)
        {
            makeAudioMP4VRPipeline(aMP4VROutputs,
                aConfigure,
                aAudioInputInfo.aacInput,
                aAudioInputInfo.timeScale);
        }

        AudioInputInfo makeAudioFromAAC(std::string aFilename,
                                        std::string aLoadLabel,
                                        ControllerOps& aOps)
        {
            AudioInputInfo info {};
            AACImport::Config importConfig { aFilename };
            info.aacInput = aOps.makeForGraph<AACImport>(aLoadLabel, importConfig);

            // TODO: hardcoded time scale for AAC input
            info.timeScale = { 1, 48000 };

            return info;
        }

        AudioInputInfo makeAudioFromMP4(MP4Loader& aMP4Loader,
                                        TrackId aSourceTrack,
                                        std::string aLoadLabel,
                                        ControllerOps& aOps)
        {
            auto audioSource = aMP4Loader.sourceForTrack(aSourceTrack);

            auto timeScale = audioSource->getTimeScale();
            AsyncNode* aacInput = aOps.wrapForGraph(aLoadLabel + ":" + Utils::to_string(aSourceTrack), std::move(audioSource));

            return { aacInput, timeScale };
        }

        AudioInputInfo makeAudioInput(ControllerOps& aOps, const ConfigValue& aAudioInput)
        {
            auto filename = readString(aAudioInput["filename"]);
            std::string loadLabel = "\"" + filename + "\"";

            if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".aac")
            {
                return makeAudioFromAAC(filename, loadLabel, aOps);
            }
            else if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".mp4")
            {
                MP4Loader::Config mp4LoaderConfig{ filename };
                MP4Loader mp4Loader(mp4LoaderConfig);

                auto audioSourceTracks = findAudioSourceTracks(mp4Loader);

                if (!audioSourceTracks.size())
                {
                    return {};
                }

                // If there are multiple AAC tracks, it typically means there are multiple languages. 
                // However, we take the first track only for now
                TrackId sourceTrack = audioSourceTracks.front();
                return makeAudioFromMP4(mp4Loader, sourceTrack, loadLabel, aOps);
            }
            else
            {
                throw ConfigValueInvalid("Unsupported input file name " + filename + ". Only .aac and .mp4 are supported.", aAudioInput["filename"]);
            }
            return {};
        }
    } // anonymous namespace

    void makeAudioMp4(ControllerOps& aOps,
                   ControllerConfigure& aConfigure,
                   const MP4VROutputs& aMP4VROutputs,
                   const ConfigValue& aAudioInput)
    {
        auto info = makeAudioInput(aOps, aAudioInput);
        if (info.aacInput)
        {
            hookAudioOutputMp4(info, aMP4VROutputs, aConfigure);
        }
    }

    void makeAudioDash(ControllerOps& aOps,
                   ControllerConfigure& aConfigure,
                   Dash& aDash,
                   const ConfigValue& aAudioInput)
    {
        auto info = makeAudioInput(aOps, aAudioInput);
        if (info.aacInput)
        {
            hookAudioOutput(info, aDash, aConfigure);
        }
    }
}
