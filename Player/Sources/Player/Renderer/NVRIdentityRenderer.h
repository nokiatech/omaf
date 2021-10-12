
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

#include "Renderer/NVRVideoRenderer.h"

OMAF_NS_BEGIN
class IdentityRenderer : public VideoRenderer
{
public:
    IdentityRenderer();
    ~IdentityRenderer();

public:  // Implements PanoramaRenderer
    virtual void_t render(const HeadTransform& headTransform,
                          const RenderSurface& renderSurface,
                          const CoreProviderSources& sources,
                          const TextureID& renderMask = TextureID::Invalid,
                          const RenderingParameters& renderingParameters = RenderingParameters());

    /**
     * @param x Center X coordinate where will be drawn 0,0 is in the middle of viewport positive is to right range
     * -0.5,0.5
     * @param y Center Y coordinate where will be drawn 0,0 is in the middle of viewport positive is to up  range
     * -0.5,0.5
     * @param width Width of area where to draw the source 1.0 is full view
     * @param height Height of area where to draw the source 1.0 is full view
     * @param scaleX Scale texture width
     * @param scaleY Scale texture height
     * @param offsetX Offset to align texture X inside overlay area, units are relative to screen size
     *                like width. If 0 texture is drawn to middle of the overlay area.
     * @param offsetY Offset to align texture Y inside overlay area, units are relative to screen size
     *                like height. If 0 texture is drawn to middle of the overlay area.
     */
    virtual void_t renderImpl(const HeadTransform& headTransform,
                              const RenderSurface& renderSurface,
                              const CoreProviderSource* source,
                              const TextureID& renderMask,
                              const RenderingParameters& renderingParameters,
                              float32_t x,
                              float32_t y,
                              float32_t width,
                              float32_t height,
                              float32_t scaleX,
                              float32_t scaleY,
                              float32_t offsetX,
                              float32_t offsetY,
                              float32_t opacity);

private:  // Implements PanoramaRenderer
    virtual void_t createImpl();
    virtual void_t destroyImpl();

private:
    class Shader
    {
    public:
        Shader();
        ~Shader();

        void_t create();
        void_t bind();
        void_t destroy();

        bool_t isValid();

        void_t setPixelFormatConstant(VideoPixelFormat::Enum pixelFormat);
        void_t setOpacityConstant(float32_t opacity);

        void_t setSourceYConstant(uint32_t textureSampler);
        void_t setSourceUConstant(uint32_t textureSampler);
        void_t setSourceVConstant(uint32_t textureSampler);

        void_t setSourceUVConstant(uint32_t textureSampler);

        void_t setSourceConstant(uint32_t textureSampler);

        void_t setTextureRectConstant(const Vector4& textureRect);

        void_t setMVPConstant(const Matrix44& mvp);
        void_t setSTConstant(const Matrix44& st);

    private:
        ShaderID handle;

        ShaderConstantID sourceConstant;

        ShaderConstantID ySourceConstant;
        ShaderConstantID uSourceConstant;
        ShaderConstantID vSourceConstant;

        ShaderConstantID uvSourceConstant;

        ShaderConstantID textureRectConstant;

        ShaderConstantID mvpConstant;
        ShaderConstantID stConstant;

        ShaderConstantID pixelFormatConstant;
        ShaderConstantID opacityConstant;
    };

    Shader mOpaqueShader;

    bool_t mDirtyShader;

    struct Vertex
    {
        float32_t x;
        float32_t y;

        float32_t u;
        float32_t v;
    };

    VertexBufferID mVertexBuffer;
};
OMAF_NS_END
