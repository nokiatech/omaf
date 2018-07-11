
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
#pragma once

#include <memory>
#include <string>
#include <list>

#include "videoinput.h"

#include "common/optional.h"

namespace VDD
{
    class Config;

    /** Simple configuration that for generating a VDD::Config (basically JSON) */
    struct ConverterConfigTemplate
    {
        std::string inputMP4;
        std::string outputMP4;
        std::string outputDASH;
        std::list<std::pair<std::string, std::string>> keyValues;
        Optional<std::string> dumpGraph;
        bool dumpConfig;
    };

    std::shared_ptr<VDD::Config> createConvConfigJson(const ConverterConfigTemplate& aConvConfig);

}
