
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

#include <list>
#include <memory>
#include <string>

#include "videoinput.h"

#include "common/optional.h"

namespace VDD
{
    class Config;

    struct H265InputConfig
    {
        int gopLength;
    };

    /** Simple configuration that for generating a VDD::Config (basically JSON) */
    struct ConverterConfigTemplate
    {
        // inputFile is MP4 if there is no h265Config; otherwise it's H265
        std::string inputFile;
        Optional<H265InputConfig> h265InputConfig;
        std::string outputMP4;
        std::string outputDASH;
        std::list<std::pair<std::string, std::string>> keyValues;
        Optional<std::string> dumpGraph;
        std::list<std::string> jsonConfig;
        bool dumpConfig = false;
        bool disableAudio = false;
        bool enableDummyMetadata = false;
    };

    std::shared_ptr<VDD::Config> createConvConfigJson(const ConverterConfigTemplate& aConvConfig);

}  // namespace VDD
