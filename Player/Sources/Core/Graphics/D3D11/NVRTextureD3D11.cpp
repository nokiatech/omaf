
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
#include "Graphics/D3D11/NVRTextureD3D11.h"

#include "Graphics/D3D11/NVRD3D11Error.h"
#include "Graphics/D3D11/NVRD3D11Utilities.h"
#include "Math/OMAFMathFunctions.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(TextureD3D11)

TextureD3D11::TextureD3D11()
    : mHandle(NULL)
    , mDevice(NULL)
    , mFeatureLevel(D3D_FEATURE_LEVEL_9_3)
    , mShaderResourceView(NULL)
    , mUnorderedAccessView(NULL)
    , mSamplerState(NULL)
    , mType(TextureType::INVALID)
    , mComputeAccess(ComputeBufferAccess::INVALID)
    , mWidth(0)
    , mHeight(0)
    , mNumMipmaps(1)
    , mFormat(TextureFormat::INVALID)
    , mAddressModeU(TextureAddressMode::INVALID)
    , mAddressModeV(TextureAddressMode::INVALID)
    , mAddressModeW(TextureAddressMode::INVALID)
    , mFilterMode(TextureFilterMode::INVALID)
    , mRenderTarget(false)
    , mWriteOnly(false)
    , mNativeHandle(false)
{
    mNativeDesc.type = TextureType::INVALID;
}

TextureD3D11::~TextureD3D11()
{
    OMAF_ASSERT(mHandle == NULL, "Texture is not destroyed");

    OMAF_ASSERT(mShaderResourceView == NULL, "Texture is not destroyed");
    OMAF_ASSERT(mSamplerState == NULL, "Texture is not destroyed");
}

void_t TextureD3D11::setDevice(ID3D11Device* device)
{
    OMAF_ASSERT(device != NULL, "D3D Device null");
    mDevice = device;
    mFeatureLevel = mDevice->GetFeatureLevel();
}

ID3D11Resource* TextureD3D11::getHandle() const
{
    return mHandle;
}

ID3D11ShaderResourceView* TextureD3D11::getShaderResourceView() const
{
    return mShaderResourceView;
}

ID3D11SamplerState* TextureD3D11::getSamplerState() const
{
    return mSamplerState;
}

uint16_t TextureD3D11::getWidth() const
{
    return mWidth;
}

uint16_t TextureD3D11::getHeight() const
{
    return mHeight;
}

uint8_t TextureD3D11::getNumMipmaps() const
{
    return mNumMipmaps;
}

TextureFormat::Enum TextureD3D11::getFormat() const
{
    return mFormat;
}

bool_t TextureD3D11::isRenderTarget() const
{
    return mRenderTarget;
}

bool_t TextureD3D11::isWriteOnly() const
{
    return mWriteOnly;
}

TextureAddressMode::Enum TextureD3D11::getAddressModeU() const
{
    return mAddressModeU;
}

TextureAddressMode::Enum TextureD3D11::getAddressModeV() const
{
    return mAddressModeV;
}

TextureAddressMode::Enum TextureD3D11::getAddressModeW() const
{
    return mAddressModeW;
}

TextureFilterMode::Enum TextureD3D11::getFilterMode() const
{
    return mFilterMode;
}

void_t TextureD3D11::setSamplerState(TextureAddressMode::Enum addressModeU,
                                     TextureAddressMode::Enum addressModeV,
                                     TextureAddressMode::Enum addressModeW,
                                     TextureFilterMode::Enum filterMode)
{
    // Create sampler state
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
    sd.Filter = getTextureFilterModeD3D(filterMode);
    sd.AddressU = getTextureAddressModeD3D(addressModeU);
    sd.AddressV = getTextureAddressModeD3D(addressModeV);
    sd.AddressW = getTextureAddressModeD3D(addressModeW);
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;

    mSamplerState->Release();

    OMAF_DX_CHECK(mDevice->CreateSamplerState(&sd, &mSamplerState));
}

bool_t TextureD3D11::create(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const TextureDesc& desc)
{
    OMAF_ASSERT_NOT_NULL(device);
    OMAF_ASSERT_NOT_NULL(deviceContext);

    setDevice(device);

    mType = desc.type;
    mComputeAccess = desc.computeAccess;
    mWidth = desc.width;
    mHeight = desc.height;
    mNumMipmaps = desc.numMipMaps;
    mFormat = desc.format;
    mAddressModeU = desc.samplerState.addressModeU;
    mAddressModeV = desc.samplerState.addressModeV;
    mAddressModeW = desc.samplerState.addressModeW;
    mFilterMode = desc.samplerState.filterMode;
    mRenderTarget = desc.renderTarget;
    mWriteOnly = desc.writeOnly;

    bool_t useMipmaps = false;

    if ((mNumMipmaps > 1) || desc.generateMipMaps)
    {
        useMipmaps = true;
    }

    // Upload data
    const TextureFormat::Description& description = TextureFormat::getDescription(desc.format);

    if (mType == TextureType::TEXTURE_2D)
    {
        const TextureFormatInfo& textureFormatInfo = getTextureFormatInfoD3D(desc.format);
        const uint8_t* ptr = (uint8_t*) desc.data;

        if (description.compressed)
        {
            OMAF_ASSERT_NOT_IMPLEMENTED();

            return false;
        }
        else
        {
            if (!desc.generateMipMaps)
            {
                bool_t needsSRV = false;
                bool_t needsDS = false;
                bool_t needsRT = false;
                bool_t needsUAV = false;
                bool_t needsIndirect = false;

                // SRV
                if (mComputeAccess == ComputeBufferAccess::NONE)
                {
                    needsSRV = true;
                }
                else if (mComputeAccess & ComputeBufferAccess::READ)
                {
                    needsSRV = true;
                }
                else if (!mWriteOnly)
                {
                    needsSRV = true;
                }

                // DS
                if (TextureFormat::hasDepthComponent(desc.format) || TextureFormat::hasStencilComponent(desc.format))
                {
                    needsDS = true;
                }

                // RT
                if (mRenderTarget)
                {
                    needsRT = true;
                }

                // UAV
                if (mComputeAccess & ComputeBufferAccess::WRITE)
                {
                    needsUAV = true;
                }


                // Create texture with manually generated mip-maps
                D3D11_TEXTURE2D_DESC textureDesc;
                ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
                textureDesc.Width = mWidth;
                textureDesc.Height = mHeight;
                textureDesc.MipLevels = mNumMipmaps;
                textureDesc.Format = textureFormatInfo.textureFormat;
                textureDesc.SampleDesc.Count = 1;
                textureDesc.SampleDesc.Quality = 0;
                textureDesc.Usage = D3D11_USAGE_DEFAULT;
                textureDesc.BindFlags = 0;
                textureDesc.BindFlags |= needsSRV ? D3D11_BIND_SHADER_RESOURCE : 0;
                textureDesc.BindFlags |= needsDS ? D3D11_BIND_DEPTH_STENCIL : 0;
                textureDesc.BindFlags |= needsRT ? D3D11_BIND_RENDER_TARGET : 0;
                textureDesc.BindFlags |= needsUAV ? D3D11_BIND_UNORDERED_ACCESS : 0;
                textureDesc.BindFlags |= needsIndirect ? D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS : 0;
                textureDesc.CPUAccessFlags = 0;
                textureDesc.ArraySize = 1;
                textureDesc.MiscFlags = 0;

                // Source data
                if (ptr != OMAF_NULL)
                {
                    D3D11_SUBRESOURCE_DATA subresourceDatas[32];  // Assumes that 32 mip-map levels are enough
                    ZeroMemory(subresourceDatas, sizeof(subresourceDatas));

                    for (uint8_t m = 0; m < mNumMipmaps; ++m)
                    {
                        uint16_t width = mWidth >> m;
                        uint16_t height = mHeight >> m;

                        width = max<uint16_t>(1, width);
                        height = max<uint16_t>(1, height);

                        subresourceDatas[m].pSysMem = ptr;
                        subresourceDatas[m].SysMemPitch = (width * description.bitsPerPixel) / 8;
                        subresourceDatas[m].SysMemSlicePitch = height * subresourceDatas[m].SysMemPitch;

                        uint32_t mipmapBytes = ((width * height) * description.bitsPerPixel) / 8;
                        ptr += mipmapBytes;
                    }

                    OMAF_DX_CHECK(mDevice->CreateTexture2D(&textureDesc, subresourceDatas, &mTexture2D));
                }
                else
                {
                    OMAF_DX_CHECK(mDevice->CreateTexture2D(&textureDesc, NULL, &mTexture2D));
                }

                // Create shader resource view
                if (needsSRV)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
                    ZeroMemory(&srvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
                    srvd.Format = textureFormatInfo.srvFormat;
                    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srvd.Texture2D.MipLevels = textureDesc.MipLevels;

                    OMAF_DX_CHECK(mDevice->CreateShaderResourceView(mTexture2D, &srvd, &mShaderResourceView));

                    // Create sampler state
                    D3D11_SAMPLER_DESC sd;
                    ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
                    sd.Filter = getTextureFilterModeD3D(mFilterMode);
                    sd.AddressU = getTextureAddressModeD3D(mAddressModeU);
                    sd.AddressV = getTextureAddressModeD3D(mAddressModeV);
                    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
                    sd.MinLOD = 0;
                    sd.MaxLOD = D3D11_FLOAT32_MAX;

                    OMAF_DX_CHECK(mDevice->CreateSamplerState(&sd, &mSamplerState));
                }

                // Create unordered access view
                if (needsUAV)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC uav;
                    ZeroMemory(&uav, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
                    uav.Format = textureDesc.Format;
                    uav.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    uav.Texture2D.MipSlice = 0;

                    OMAF_DX_CHECK(mDevice->CreateUnorderedAccessView(mTexture2D, &uav, &mUnorderedAccessView));
                }

                return true;
            }
            else
            {
                // Create texture with auto-generated mip-maps
                D3D11_TEXTURE2D_DESC textureDesc;
                ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
                textureDesc.Width = mWidth;
                textureDesc.Height = mHeight;
                textureDesc.MipLevels = 0;
                textureDesc.Format = textureFormatInfo.textureFormat;
                textureDesc.SampleDesc.Count = 1;
                textureDesc.SampleDesc.Quality = 0;
                textureDesc.Usage = D3D11_USAGE_DEFAULT;
                textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                if (TextureFormat::hasDepthComponent(desc.format) || TextureFormat::hasStencilComponent(desc.format))
                {
                    textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
                }
                else if (mRenderTarget)
                {
                    textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                }

                textureDesc.BindFlags |= desc.generateMipMaps ? D3D11_BIND_RENDER_TARGET : 0;
                textureDesc.CPUAccessFlags = 0;
                textureDesc.ArraySize = 1;
                textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

                OMAF_DX_CHECK(mDevice->CreateTexture2D(&textureDesc, NULL, &mTexture2D));

                // Create shader resource view
                D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
                ZeroMemory(&srvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
                srvd.Format = textureFormatInfo.srvFormat;
                srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvd.Texture2D.MipLevels = -1;

                OMAF_DX_CHECK(mDevice->CreateShaderResourceView(mTexture2D, &srvd, &mShaderResourceView));

                // Create sampler state
                D3D11_SAMPLER_DESC sd;
                ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
                sd.Filter = getTextureFilterModeD3D(mFilterMode);
                sd.AddressU = getTextureAddressModeD3D(mAddressModeU);
                sd.AddressV = getTextureAddressModeD3D(mAddressModeV);
                sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
                sd.MinLOD = 0;
                sd.MaxLOD = D3D11_FLOAT32_MAX;

                OMAF_DX_CHECK(mDevice->CreateSamplerState(&sd, &mSamplerState));

                // Update data and update mip-maps
                UINT rowPitch = (desc.width * description.bitsPerPixel) / 8;

                deviceContext->UpdateSubresource(mTexture2D, 0, NULL, ptr, (UINT) rowPitch, 1);
                deviceContext->GenerateMips(mShaderResourceView);

                return true;
            }
        }
    }
    else if (mType == TextureType::TEXTURE_3D)
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();
    }
    else if (mType == TextureType::TEXTURE_CUBE)
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();
    }

    return false;
}

const NativeTextureDesc& TextureD3D11::getNativeDesc()
{
    return mNativeDesc;
}

bool_t TextureD3D11::createNative(ID3D11Device* device,
                                  ID3D11DeviceContext* deviceContext,
                                  const NativeTextureDesc& desc)
{
    OMAF_ASSERT_NOT_NULL(device);
    OMAF_ASSERT_NOT_NULL(deviceContext);

    setDevice(device);

    D3D11_TEXTURE2D_DESC textureDesc;

    ID3D11Texture2D* texture = (ID3D11Texture2D*) desc.nativeHandle;
    texture->GetDesc(&textureDesc);

    mType = desc.type;

    if (desc.type == TextureType::TEXTURE_VIDEO_SURFACE)
    {
        mFormat = desc.format;
    }
    else
    {
        mFormat = getTextureFormat(textureDesc.Format);
    }

    mWidth = textureDesc.Width;
    mHeight = textureDesc.Height;
    mNumMipmaps = textureDesc.MipLevels;

    mNativeHandle = true;
    mNativeDesc = desc;
    mTexture2D = texture;


    // Create shader resources if needed. (texture might just be a write only surface which cant be bound to a shader)
    if (textureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        const TextureFormatInfo& info = getTextureFormatInfoD3D(mFormat);

        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
        ZeroMemory(&srvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
        srvd.Format = info.srvFormat;
        switch (desc.type)
        {
        case TextureType::TEXTURE_CUBE:
        {
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srvd.TextureCube.MipLevels = mNumMipmaps;
            break;
        }
        case TextureType::TEXTURE_ARRAY:
        {
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            srvd.Texture2DArray.ArraySize = 1;
            srvd.Texture2DArray.FirstArraySlice = desc.arrayIndex;
            srvd.Texture2DArray.MipLevels = mNumMipmaps;
            break;
        }
        case TextureType::TEXTURE_2D:
        default:
        {
            if (textureDesc.SampleDesc.Count > 1)
            {
                srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvd.Texture2D.MipLevels = mNumMipmaps;
            }
            break;
        }
        }

        OMAF_DX_CHECK(mDevice->CreateShaderResourceView(mTexture2D, &srvd, &mShaderResourceView));

        // Create sampler state
        D3D11_SAMPLER_DESC sd;
        ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
        sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MinLOD = 0;
        sd.MaxLOD = D3D11_FLOAT32_MAX;

        OMAF_DX_CHECK(mDevice->CreateSamplerState(&sd, &mSamplerState));
    }
    else
    {
        // This texture can not be bound as a ... texture.
        // ie. it's write only, most likely to be used only as a render target.
        mShaderResourceView = OMAF_NULL;
        mSamplerState = OMAF_NULL;
    }

    return true;
}

void_t TextureD3D11::bind(ID3D11DeviceContext* deviceContext, uint16_t textureUnit)
{
    OMAF_ASSERT_NOT_NULL(deviceContext);

    OMAF_ASSERT_NOT_NULL(mShaderResourceView);
    OMAF_ASSERT_NOT_NULL(mSamplerState);

    if (mFeatureLevel > D3D_FEATURE_LEVEL_9_3)
    {
        deviceContext->VSSetShaderResources(textureUnit, 1, &mShaderResourceView);
        deviceContext->VSSetSamplers(textureUnit, 1, &mSamplerState);
    }

    deviceContext->PSSetShaderResources(textureUnit, 1, &mShaderResourceView);
    deviceContext->PSSetSamplers(textureUnit, 1, &mSamplerState);
}

void_t TextureD3D11::unbind(ID3D11DeviceContext* deviceContext, uint16_t textureUnit)
{
    if (mFeatureLevel > D3D_FEATURE_LEVEL_9_3)
    {
        ID3D11ShaderResourceView* nullSRV = NULL;
        deviceContext->VSSetShaderResources(textureUnit, 1, &nullSRV);

        ID3D11SamplerState* nullSampler = NULL;
        deviceContext->VSSetSamplers(textureUnit, 1, &nullSampler);
    }

    ID3D11ShaderResourceView* nullSRV = NULL;
    deviceContext->PSSetShaderResources(textureUnit, 1, &nullSRV);

    ID3D11SamplerState* nullSampler = NULL;
    deviceContext->PSSetSamplers(textureUnit, 1, &nullSampler);
}

void_t TextureD3D11::bindCompute(ID3D11DeviceContext* deviceContext,
                                 uint16_t stage,
                                 ComputeBufferAccess::Enum computeAccess)
{
    bool_t needsSRV = false;
    bool_t needsUAV = false;

    // SRV
    if (mComputeAccess == ComputeBufferAccess::NONE)
    {
        needsSRV = true;
    }
    else if (mComputeAccess & ComputeBufferAccess::READ)
    {
        needsSRV = true;
    }

    // UAV
    if (mComputeAccess & ComputeBufferAccess::WRITE)
    {
        needsUAV = true;
    }

    if (needsSRV)
    {
        OMAF_ASSERT_NOT_NULL(mShaderResourceView);

        deviceContext->CSSetShaderResources(stage, 1, &mShaderResourceView);
        deviceContext->CSSetSamplers(stage, 1, &mSamplerState);
    }

    if (needsUAV)
    {
        OMAF_ASSERT_NOT_NULL(mUnorderedAccessView);

        deviceContext->CSSetUnorderedAccessViews(stage, 1, &mUnorderedAccessView, NULL);
    }
}

void_t TextureD3D11::unbindCompute(ID3D11DeviceContext* deviceContext, uint16_t stage)
{
    bool_t needsSRV = false;
    bool_t needsUAV = false;

    // SRV
    if (mComputeAccess == ComputeBufferAccess::NONE)
    {
        needsSRV = true;
    }
    else if (mComputeAccess & ComputeBufferAccess::READ)
    {
        needsSRV = true;
    }

    // UAV
    if (mComputeAccess & ComputeBufferAccess::WRITE)
    {
        needsUAV = true;
    }

    if (needsSRV)
    {
        ID3D11ShaderResourceView* nullSRV = NULL;
        deviceContext->CSSetShaderResources(stage, 1, &nullSRV);

        ID3D11SamplerState* nullSampler = NULL;
        deviceContext->CSSetSamplers(stage, 1, &nullSampler);
    }

    if (needsUAV)
    {
        ID3D11UnorderedAccessView* nullUAV = NULL;
        deviceContext->CSSetUnorderedAccessViews(stage, 1, &nullUAV, NULL);
    }
}

void_t TextureD3D11::destroy()
{
    OMAF_ASSERT(mHandle != NULL, "");

    if (!mNativeHandle)
    {
        if (mType == TextureType::TEXTURE_2D)
        {
            OMAF_DX_RELEASE(mTexture2D, 0);
        }
        else if (mType == TextureType::TEXTURE_3D)
        {
            OMAF_DX_RELEASE(mTexture3D, 0);
        }
        else if (mType == TextureType::TEXTURE_CUBE)
        {
            OMAF_ASSERT_NOT_IMPLEMENTED();
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }
    }

    OMAF_DX_RELEASE(mShaderResourceView, 0);
    OMAF_DX_RELEASE(mSamplerState, 0);

    mWidth = 0;
    mHeight = 0;
    mNumMipmaps = 0;
    mFormat = TextureFormat::INVALID;
    mAddressModeU = TextureAddressMode::INVALID;
    mAddressModeV = TextureAddressMode::INVALID;
    mAddressModeW = TextureAddressMode::INVALID;
    mFilterMode = TextureFilterMode::INVALID;
    mRenderTarget = false;
    mWriteOnly = false;
    mNativeHandle = false;
    mHandle = NULL;
}
OMAF_NS_END
