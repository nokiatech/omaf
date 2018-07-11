
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

#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRTextureType.h"
#include "Graphics/NVRTextureFormat.h"
#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRTextureFilterMode.h"
#include "Graphics/NVRComputeBufferAccess.h"
#include "Graphics/NVRResourceDescriptors.h"

OMAF_NS_BEGIN
    class TextureD3D11
    {
        public:

            TextureD3D11();
            ~TextureD3D11();

            ID3D11Resource* getHandle() const;

            ID3D11ShaderResourceView* getShaderResourceView() const;
            ID3D11SamplerState* getSamplerState() const;

            uint16_t getWidth() const;
            uint16_t getHeight() const;

            uint8_t getNumMipmaps() const;

            TextureFormat::Enum getFormat() const;

            bool_t isRenderTarget() const;
            bool_t isWriteOnly() const;

            TextureAddressMode::Enum getAddressModeU() const;
            TextureAddressMode::Enum getAddressModeV() const;
            TextureAddressMode::Enum getAddressModeW() const;

            TextureFilterMode::Enum getFilterMode() const;

            void_t setSamplerState(TextureAddressMode::Enum addressModeU,
                                   TextureAddressMode::Enum addressModeV,
                                   TextureAddressMode::Enum addressModeW,
                                   TextureFilterMode::Enum filterMode);

            bool_t create(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const TextureDesc& desc);
            bool_t createNative(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const NativeTextureDesc& desc);

            void_t destroy();

            void_t bind(ID3D11DeviceContext* deviceContext, uint16_t textureUnit);
            void_t unbind(ID3D11DeviceContext* deviceContext, uint16_t textureUnit);

            void_t bindCompute(ID3D11DeviceContext* deviceContext, uint16_t stage, ComputeBufferAccess::Enum computeAccess);
            void_t unbindCompute(ID3D11DeviceContext* deviceContext, uint16_t stage);

            const NativeTextureDesc& getNativeDesc();
        private:

            OMAF_NO_ASSIGN(TextureD3D11);
            OMAF_NO_COPY(TextureD3D11);

        private:

            union
            {
                ID3D11Resource* mHandle;
                ID3D11Texture2D* mTexture2D;
                ID3D11Texture3D* mTexture3D;
            };

            ID3D11Device* mDevice;
            D3D_FEATURE_LEVEL mFeatureLevel;
            void_t setDevice(ID3D11Device* device);

            ID3D11ShaderResourceView* mShaderResourceView;
            ID3D11UnorderedAccessView* mUnorderedAccessView;
            ID3D11SamplerState* mSamplerState;

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

            bool_t mRenderTarget;
            bool_t mWriteOnly;

            bool_t mNativeHandle;

            NativeTextureDesc mNativeDesc;
    };
OMAF_NS_END
