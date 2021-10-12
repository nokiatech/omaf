
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

#include "NVRNamespace.h"

OMAF_NS_BEGIN
struct RenderingManagerSettings
{
    float32_t crossBlendDurationInMilliSeconds;
    float32_t orientationVelocityCoEffA;
    float32_t orientationVelocityCoEffB;
};

static const RenderingManagerSettings DefaultPresenceRendererSettings = {500.0f, 0.8f, 0.2f};
OMAF_NS_END
