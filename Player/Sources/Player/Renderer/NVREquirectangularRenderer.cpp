
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
#include "Renderer/NVREquirectangularRenderer.h"
#include "Math/OMAFMath.h"
#include "Graphics/NVRRenderBackend.h"

#include "Foundation/NVRLogger.h"

namespace
{
    static const int EQUIRECT_MESH_COLS = 64;
    static const int EQUIRECT_MESH_ROWS = 32;
}

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(EquirectangularRenderer)

    EquirectangularRenderer::EquirectangularRenderer()
    : mDirtyShader(true)
    {
    }

    EquirectangularRenderer::~EquirectangularRenderer()
    {
        destroy();
    }

    void_t EquirectangularRenderer::createImpl()
    {
        mGeometry.create(EQUIRECT_MESH_COLS, EQUIRECT_MESH_ROWS);
    }

    void_t EquirectangularRenderer::destroyImpl()
    {
        mGeometry.destroy();

        mOpaqueShader.destroy();
        mMaskShader.destroy();
    }

    ProjectionType::Enum EquirectangularRenderer::getType() const
    {
        return ProjectionType::EQUIRECTANGULAR;
    }

    void_t EquirectangularRenderer::render(const HeadTransform& headTransform, const RenderSurface& renderSurface, const CoreProviderSources& sources, const TextureID& renderMask, const RenderingParameters& renderingParameters)
    {
        OMAF_ASSERT(sources.getSize() == 1, "Supports only one input source");
        OMAF_ASSERT(sources.at(0)->type == SourceType::EQUIRECTANGULAR_PANORAMA, "Source is not an equirect panorama");

        RenderBackend::pushDebugMarker("EquirectangularPanoramaRenderer::render");

        EquirectangularSource* source = (EquirectangularSource*)sources.at(0);
        bool_t useMask = renderMask != TextureID::Invalid;

        if (mDirtyShader)
        {
            mDirtyShader = false;
            mOpaqueShader.createDefaultVideoShader(source->videoFrame.format, false);
            mMaskShader.createDefaultVideoShader(source->videoFrame.format, true);
        }

        // Bind shader and constants
        VideoShader& selectedShader = useMask ? mMaskShader : mOpaqueShader;

        selectedShader.bind();
        selectedShader.bindVideoTexture(source->videoFrame);

        Matrix44 viewMatrix;
        if (source->forcedOrientation.valid)
        {
            viewMatrix = makeTranspose(makeMatrix44(source->forcedOrientation.orientation));
        }
        else
        {
            viewMatrix = makeTranspose(makeMatrix44(headTransform.orientation));
        }
        Matrix44 viewProjectionMatrix = renderSurface.projection * viewMatrix;
        Matrix44 mvp = viewProjectionMatrix * makeMatrix44(source->extrinsicRotation);

        Matrix44 vtm = source->videoFrame.matrix * makeTranslation(source->textureRect.x, source->textureRect.y, 0) * makeScale(source->textureRect.w, source->textureRect.h, 1);
        selectedShader.setDefaultVideoShaderConstants(mvp, vtm);

        if (useMask)
        {
            selectedShader.bindMaskTexture(renderMask, renderingParameters.clearColor);
        }

        // Bind geometry
        mGeometry.bind();

        BlendState blendState;
        blendState.blendEnabled = false;

        RasterizerState rasterizerState;
        rasterizerState.scissorTestEnabled = true;
        rasterizerState.cullMode = CullMode::NONE;

        RenderBackend::setBlendState(blendState);
        RenderBackend::setRasterizerState(rasterizerState);

        // Draw
        RenderBackend::drawIndexed(PrimitiveType::TRIANGLE_STRIP, 0, (uint32_t) mGeometry.getIndexCount());

        RenderBackend::popDebugMarker();
    }

OMAF_NS_END
