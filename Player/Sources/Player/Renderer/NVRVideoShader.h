
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

#include "NVRNamespace.h"
#include "Graphics/NVRHandles.h"
#include "Graphics/NVRShaderConstantType.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRPair.h"
#include "Foundation/NVRFixedString.h"

#include "Renderer/NVRVideoRenderer.h"

OMAF_NS_BEGIN

class VideoShader
{
public:

    VideoShader();
    ~VideoShader();

    void_t create(const char_t *vertexShader, const char_t *fragmentShader, VideoPixelFormat::Enum videoTextureFormat = VideoPixelFormat::INVALID, VideoPixelFormat::Enum depthTextureFormat = VideoPixelFormat::INVALID, bool depthTextureVertexShaderFetch = false);

    void_t createDefaultVideoShader(VideoPixelFormat::Enum videoTextureFormat, const bool_t maskEnabled = false);
    void_t setDefaultVideoShaderConstants(const Matrix44 mvp, const Matrix44 vtm);

    void_t createTintVideoShader(VideoPixelFormat::Enum videoTextureFormat);
    void_t setTintVideoShaderConstants(const Color4& tintColor);


    void_t bind();

    void_t bindVideoTexture(VideoFrame videoFrame);
    void_t bindDepthTexture(VideoFrame videoFrame);
    void_t bindMaskTexture(TextureID maskTexture, const Color4& clearColor);

    void_t destroy();

    bool_t isValid();

    void_t setConstant(const char_t* name, ShaderConstantType::Enum type, const void_t* values, uint32_t numValues = 1);

private:

    ShaderID handle;

    typedef Pair<FixedString128, ShaderConstantID> ShaderConstantPair;
    typedef FixedArray<ShaderConstantPair, 128> ShaderConstantList;

    ShaderConstantList mShaderConstants;
};

OMAF_NS_END
