
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

#include "Graphics/NVRDepthMask.h"
#include "Graphics/NVRDepthFunction.h"
#include "Graphics/NVRStencilFunction.h"
#include "Graphics/NVRStencilOperation.h"

OMAF_NS_BEGIN

struct DepthStencilState
{
    bool_t depthTestEnabled;
    
    DepthMask::Enum depthWriteMask;
    DepthFunction::Enum depthFunction;
    
    bool_t stencilTestEnabled;
    
    uint8_t stencilReference;
    uint8_t stencilReadMask;
    
    StencilFunction::Enum frontStencilFunction;
    StencilOperation::Enum frontStencilFailOperation;
    StencilOperation::Enum frontStencilZFailOperation;
    StencilOperation::Enum frontStencilZPassOperation;
    
    StencilFunction::Enum backStencilFunction;
    StencilOperation::Enum backStencilFailOperation;
    StencilOperation::Enum backStencilZFailOperation;
    StencilOperation::Enum backStencilZPassOperation;
    
    DepthStencilState()
    {
        depthTestEnabled = false;
        
        depthWriteMask = DepthMask::ZERO;
        depthFunction = DepthFunction::LESS;
        
        stencilTestEnabled = false;
        
        stencilReference = 0;
        stencilReadMask = 0xff;
        
        frontStencilFunction = StencilFunction::ALWAYS;
        frontStencilFailOperation = StencilOperation::KEEP;
        frontStencilZFailOperation = StencilOperation::KEEP;
        frontStencilZPassOperation = StencilOperation::KEEP;
        
        backStencilFunction = StencilFunction::ALWAYS;
        backStencilFailOperation = StencilOperation::KEEP;
        backStencilZFailOperation = StencilOperation::KEEP;
        backStencilZPassOperation = StencilOperation::KEEP;
    }
};

bool_t equals(const DepthStencilState& left, const DepthStencilState& right);

OMAF_NS_END
