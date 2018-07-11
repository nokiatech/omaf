
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

#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRTextureFilterMode.h"

OMAF_NS_BEGIN

struct SamplerState
{
    TextureAddressMode::Enum addressModeU;
    TextureAddressMode::Enum addressModeV;
    TextureAddressMode::Enum addressModeW;
    
    TextureFilterMode::Enum filterMode;
    
    // uint8_t maxAnisotropy;
    // uint8_t maxMipMapLevel;
    // float32_t mipMapLODBias;

    SamplerState()
    {
        addressModeU = TextureAddressMode::CLAMP;
        addressModeV = TextureAddressMode::CLAMP;
        addressModeW = TextureAddressMode::CLAMP;
        
        filterMode = TextureFilterMode::BILINEAR;
        
        // maxAnisotropy = 1;
        // maxMipMapLevel = 1000;
        // mipMapLODBias = 0.0f;
    }
};

bool_t equals(const SamplerState& left, const SamplerState& right);

OMAF_NS_END
