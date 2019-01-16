
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
#pragma once

#include <memory>

#include "mp4vrwriter.h"

#include "config/config.h"

namespace VDD
{
    struct AuxConfig;
    class ControllerOps;
    class ControllerConfigure;
    class ConfigValue;
    class Dash;

    /** @brief This sets up the audio outputs. The same audio could be copied to multiple DASH
     * outputs and that's a bit pointless.
     */

    void makeAudioMp4(ControllerOps& aOps,
                   ControllerConfigure& aConfigure,
                   const MP4VROutputs& aMP4VROutputs,
                   const ConfigValue& aAudioInput);

    void makeAudioDash(ControllerOps& aOps,
        ControllerConfigure& aConfigure,
        Dash& aDash,
        const ConfigValue& aAudioInput);
}
