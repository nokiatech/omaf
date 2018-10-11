
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
#include <sstream>

#include "omafviconfig.h"

#include "config/config.h"

#include "jsonlib/json.hh"

namespace VDD
{
    namespace {
        const char* kConfig = R"#(
        {
            "video": {
            },
            "audio" : {
            }
        })#";
    }

    std::shared_ptr<VDD::Config> createConvConfigJson(const ConverterConfigTemplate& aVDDConfig)
    {
        std::shared_ptr<VDD::Config> config(new VDD::Config());

        {
            std::istringstream st(kConfig);
            config->load(st, "config");
        }

        config->setKeyValue("video.filename", aVDDConfig.inputMP4);
        config->setKeyValue("audio.filename", aVDDConfig.inputMP4);
        config->setKeyValue("video.formats", "H265");
        if (aVDDConfig.outputDASH.empty())
        {
            config->setKeyValue("mp4.filename", aVDDConfig.outputMP4);
            config->setKeyValue("video.modes", "mono, topbottom, sidebyside");
        }
        else
        {
            config->setKeyValue("dash.mpd.filename", aVDDConfig.outputDASH);
            config->setKeyValue("dash.media.segment_name.video", "$Name$.video.$Segment$.mp4");
            config->setKeyValue("dash.media.segment_name.audio", "$Name$.audio.$Segment$.mp4");
            config->setKeyValue("video.modes", "mono, topbottom, sidebyside");
        }


        for (auto kv: aVDDConfig.keyValues)
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

}
