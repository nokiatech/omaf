
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

#include "NVREssentials.h"

#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRTextureType.h"
#include "Graphics/NVRTextureFormat.h"
#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRTextureFilterMode.h"
#include "Graphics/NVRComputeBufferAccess.h"
#include "Graphics/NVRResourceDescriptors.h"

OMAF_NS_BEGIN

class TextureGL
{
    public:
        
        TextureGL();
        ~TextureGL();
    
        GLuint getHandle() const;
    
        uint16_t getWidth() const;
        uint16_t getHeight() const;
    
        uint8_t getNumMipmaps() const;
    
        TextureFormat::Enum getFormat() const;
    
        TextureAddressMode::Enum getAddressModeU() const;
        TextureAddressMode::Enum getAddressModeV() const;
        TextureAddressMode::Enum getAddressModeW() const;
    
        TextureFilterMode::Enum getFilterMode() const;
    
        void_t setSamplerState(TextureAddressMode::Enum addressModeU,
                               TextureAddressMode::Enum addressModeV,
                               TextureAddressMode::Enum addressModeW,
                               TextureFilterMode::Enum filterMode);
    
        bool_t create(const TextureDesc& desc);
        bool_t createNative(const NativeTextureDesc& descs);
    
        void_t destroy();

        void_t bind(uint16_t textureUnit);
        void_t unbind(uint16_t textureUnit);

        void_t bindCompute(uint16_t stage, ComputeBufferAccess::Enum computeAccess);
        void_t unbindCompute(uint16_t stage);
    
    private:
    
        OMAF_NO_ASSIGN(TextureGL);
        OMAF_NO_COPY(TextureGL);
    
    private:

        GLuint mHandle;
        GLenum mTarget;

        TextureType::Enum mType;
        ComputeBufferAccess::Enum mComputeAccess;
    
        uint16_t mWidth;
        uint16_t mHeight;
    
        uint8_t mNumMipmaps;

        TextureFormat::Enum mFormat;
        TextureAddressMode::Enum mAddressModeU;
        TextureAddressMode::Enum mAddressModeV;
        TextureAddressMode::Enum mAddressModeW;
        TextureFilterMode::Enum mFilterMode;
    
        bool_t mNativeHandle;
};

OMAF_NS_END
