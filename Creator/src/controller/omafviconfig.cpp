
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
#include <sstream>

#include "omafviconfig.h"

#include "config/config.h"

#include "jsonlib/json.hh"

namespace VDD
{
    namespace
    {
        const char* kConfig = R"#(
        {
            "video": {
            },
            "audio": {
            },
            "overlays": [
            ]
        })#";
    }

    std::shared_ptr<VDD::Config> createConvConfigJson(const ConverterConfigTemplate& aVDDConfig)
    {
        std::shared_ptr<VDD::Config> config(new VDD::Config());

        {
            std::istringstream st(kConfig);
            config->load(st, "config");
        }

        config->setKeyValue("video.filename", aVDDConfig.inputFile);

        if (aVDDConfig.h265InputConfig)
        {
            config->setKeyValue("video.gop_length", std::to_string(aVDDConfig.h265InputConfig->gopLength));
        }

        if (!aVDDConfig.disableAudio)
        {
            config->setKeyValue("audio.filename", aVDDConfig.inputFile);
        }

        config->setKeyJsonValue("enable_dummy_metadata", aVDDConfig.enableDummyMetadata);

        config->setKeyValue("video.formats", "H265");
        config->setKeyValue("video.modes", "mono, topbottom, sidebyside, nonvr");
        if (aVDDConfig.outputDASH.empty())
        {
            config->setKeyValue("mp4.filename", aVDDConfig.outputMP4);
        }
        else
        {
            config->setKeyValue("dash.mpd.filename", aVDDConfig.outputDASH);
            config->setKeyValue("dash.profile", "on_demand");
            config->setKeyValue("dash.media.segment_name.video", "$Name$.video.$Segment$.mp4");
            config->setKeyValue("dash.media.segment_name.audio", "$Name$.audio.$Segment$.mp4");
        }

        for (auto configName : aVDDConfig.jsonConfig)
        {
            std::ifstream file(configName);
            if (!file)
            {
                throw ConfigLoadError("Failed to open " + configName);
            }
            config->load(file, configName);
        }

        for (auto kv : aVDDConfig.keyValues)
        {
            config->setKeyJsonValue(kv.first, kv.second);
        }


        if (aVDDConfig.dumpGraph)
        {
            config->setKeyValue("debug.dot", *aVDDConfig.dumpGraph);
        }

        if (aVDDConfig.dumpConfig)
        {
            std::cout << *config;
        }

        return config;
    }

}  // namespace VDD
