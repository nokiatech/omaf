
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
#include "Graphics/NVRStreamBuffer.h"
#include "Math/OMAFVector2.h"
#include "Renderer/NVRVideoShader.h"

OMAF_NS_BEGIN
    class EquirectangularMask
    {
        public:

            EquirectangularMask();
            ~EquirectangularMask();

            void_t create();
            void_t destroy();
            void_t updateClippingAreas(const ClipArea* clippingAreas, const int16_t clipAreaCount);

            TextureID getMaskTexture();

        private:
            void_t createRenderTarget();
            void_t createShader();

            TextureID colorAttachment;
            RenderTargetID mRenderTarget;

            VideoShader mShader;

            TextureID mTextureHandle;
            bool_t mValidMask;

            struct Vertex
            {
                float32_t x;
                float32_t y;
                float32_t opacity;
            };

            StreamBuffer<Vertex> mVertexStream;

            VertexBufferID mVertexBuffer;
    };
OMAF_NS_END
