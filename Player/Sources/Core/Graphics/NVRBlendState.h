
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

#include "Graphics/NVRBlendFunction.h"
#include "Graphics/NVRBlendEquation.h"
#include "Graphics/NVRColorMask.h"

OMAF_NS_BEGIN

struct BlendState
{
    bool_t blendEnabled;
    bool_t alphaToCoverageEnabled;
    
    BlendFunction::Enum blendFunctionSrcRgb;
    BlendFunction::Enum blendFunctionDstRgb;
    BlendFunction::Enum blendFunctionSrcAlpha;
    BlendFunction::Enum blendFunctionDstAlpha;
    
    BlendEquation::Enum blendEquationRgb;
    BlendEquation::Enum blendEquationAlpha;
    
    ColorMask::Enum blendWriteMask;
    uint8_t blendFactor[4];
    
    BlendState()
    {
        blendEnabled = false;
        alphaToCoverageEnabled = false;
        
        blendFunctionSrcRgb = BlendFunction::ONE;
        blendFunctionDstRgb = BlendFunction::ZERO;
        blendFunctionSrcAlpha = BlendFunction::ONE;
        blendFunctionDstAlpha = BlendFunction::ZERO;
        
        blendEquationRgb = BlendEquation::ADD;
        blendEquationAlpha = BlendEquation::ADD;
        
        blendWriteMask = ColorMask::ALL;
        
        blendFactor[0] = 255;
        blendFactor[1] = 255;
        blendFactor[2] = 255;
        blendFactor[3] = 255;
    }
};

bool_t equals(const BlendState& left, const BlendState& right);

OMAF_NS_END
