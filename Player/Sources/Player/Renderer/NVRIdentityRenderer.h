
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

#include "Renderer/NVRVideoRenderer.h"

OMAF_NS_BEGIN
    class IdentityRenderer
    : public VideoRenderer
    {
        public:

            IdentityRenderer();
            ~IdentityRenderer();

        public: // Implements PanoramaRenderer

            virtual ProjectionType::Enum getType() const;

            virtual void_t render(const HeadTransform& headTransform, const RenderSurface& renderSurface, const CoreProviderSources& sources, const TextureID& renderMask = TextureID::Invalid, const RenderingParameters& renderingParameters = RenderingParameters());

        private: // Implements PanoramaRenderer

            virtual void_t createImpl();
            virtual void_t destroyImpl();

        private:

            class Shader
            {
                public:

                    Shader();
                    ~Shader();

                    void_t create(VideoPixelFormat::Enum textureFormat);
                    void_t bind();
                    void_t destroy();

                    bool_t isValid();

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
