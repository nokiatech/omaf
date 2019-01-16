
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

#include "Renderer/NVRVideoRenderer.h"
#include "Renderer/NVRRenderingManagerSettings.h"
#include "Renderer/NVREquirectangularMask.h"

#include "Foundation/NVRHashMap.h"

OMAF_NS_BEGIN

    class RenderingManager
    {
    public:

        RenderingManager();
        virtual ~RenderingManager();

    public:

        virtual bool_t initialize(CoreProvider* provider);
        virtual bool_t initialize(CoreProvider* provider, RenderingManagerSettings& settings);

        virtual void_t deinitialize();

        virtual uint32_t createRenderTarget(const RenderTextureDesc* colorAttachment, const RenderTextureDesc* depthStencilAttachment);
        virtual void_t destroyRenderTarget(uint32_t handle);

        virtual void_t prepareRender(const HeadTransform& headTransform, const RenderingParameters& renderingParameters);
        virtual void_t render(const HeadTransform& headTransform, const RenderSurface& renderSurface, const RenderingParameters& renderingParameters);
        virtual void_t finishRender();

    private:

        void_t createRenderers();
        void_t destroyRendererCollection();

        VideoRenderer* createRenderer(SourceType::Enum sourceType) const;

        void_t clearFrame(const RenderSurface& surface, const Color4& clearColor);

        void_t renderFrame(const HeadTransform& headTransform, const RenderSurface& renderSurface, const RenderingParameters& renderingParameters);

        ProjectionType::Enum getProjectionType(const CoreProviderSource* videoSource) const;
        const CoreProviderSource* findMetadata(uint8_t sourceId);

        void_t createDebugTexture(const char_t* filename);

    private:

        MemoryAllocator& mAllocator;

        CoreProvider* mCoreProvider;

        typedef HashMap<SourceType::Enum, VideoRenderer*> RendererCollection;
        RendererCollection mRendererCollection;

        VideoFrameSource* mDebugSource;
        float32_t mFovHorizontal;
        float32_t mFovVertical;

        EquirectangularMask mRenderMask;
    };
OMAF_NS_END
