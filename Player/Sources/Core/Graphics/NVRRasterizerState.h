
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

#include "NVREssentials.h"

#include "Graphics/NVRCullMode.h"
#include "Graphics/NVRFillMode.h"
#include "Graphics/NVRFrontFace.h"

OMAF_NS_BEGIN

struct RasterizerState
{
    CullMode::Enum cullMode;
    FillMode::Enum fillMode;
    FrontFace::Enum frontFace;
    bool_t scissorTestEnabled;
    
    RasterizerState()
    {
        cullMode = CullMode::BACK;
        fillMode = FillMode::SOLID;
        frontFace = FrontFace::CCW;
        scissorTestEnabled = false;
    }
};

bool_t equals(const RasterizerState& left, const RasterizerState& right);

OMAF_NS_END
