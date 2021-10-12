
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
#include "Renderer/NVREquirectangularRenderer.h"
#include "Graphics/NVRRenderBackend.h"
#include "Math/OMAFMath.h"

#include "Foundation/NVRLogger.h"

namespace
{
    static const int EQUIRECT_MESH_COLS = 64;
    static const int EQUIRECT_MESH_ROWS = 32;
}  // namespace

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

void_t EquirectangularRenderer::render(const HeadTransform& headTransform,
                                       const RenderSurface& renderSurface,
                                       const CoreProviderSources& sources,
                                       const TextureID& renderMask,
                                       const RenderingParameters& renderingParameters)
{
    OMAF_ASSERT(sources.getSize() == 1, "Supports only one input source");
    OMAF_ASSERT(sources.at(0)->type == SourceType::EQUIRECTANGULAR_PANORAMA, "Source is not an equirect panorama");

    RenderBackend::pushDebugMarker("EquirectangularPanoramaRenderer::render");

    EquirectangularSource* source = (EquirectangularSource*) sources.at(0);
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


    /**
     * Fade background if 6-dof position is too far from center
     * (constants are selected for Quest, would require tuning for each device)
     */

    float32_t opacity = 1.0f;
    if (renderingParameters.dofStyle >= 1)
    {
        Vector3 currentPos = headTransform.positionOffset - headTransform.position;
        currentPos.y = 0;  // don't care about y-axis when calculating distance
        float32_t distanceFromCenter = length(currentPos);

        // if HMD movement is scaled before sending to player it can be set here
        // should match value in HMD device position scaling
        const float32_t sixDofScale = 0.1f;

        // in meters
        const float32_t FADE_TO_BLACK_THRESHOLD = 0.25f * sixDofScale;
        const float32_t FADE_TO_BLACK_WIDTH = 0.05f * sixDofScale;  // fade starts / end radius

        // for converting fade steps to range of 0..1
        const float32_t FADE_TO_BLACK_SCALE = 1 / (FADE_TO_BLACK_WIDTH * 2);

        if (distanceFromCenter > (FADE_TO_BLACK_THRESHOLD - FADE_TO_BLACK_WIDTH))
        {
            if (renderingParameters.dofStyle >= 2)
            {
                float32_t fadeStep =
                    ((FADE_TO_BLACK_THRESHOLD + FADE_TO_BLACK_WIDTH) - distanceFromCenter) * FADE_TO_BLACK_SCALE;
                opacity = fadeStep > 0 ? fadeStep : 0;
            }
            else
            {
                if (distanceFromCenter > FADE_TO_BLACK_THRESHOLD)
                {
                    opacity = 0;
                }
            }
        }
    }

    Matrix44 viewProjectionMatrix = renderSurface.projection * viewMatrix;
    Matrix44 mvp = viewProjectionMatrix * makeMatrix44(source->extrinsicRotation);

    Matrix44 vtm = source->videoFrame.matrix * makeTranslation(source->textureRect.x, source->textureRect.y, 0) *
        makeScale(source->textureRect.w, source->textureRect.h, 1);

    selectedShader.setDefaultVideoShaderConstants(mvp, vtm, opacity);

    if (useMask)
    {
        selectedShader.bindMaskTexture(renderMask, renderingParameters.clearColor);
    }

    // Bind geometry
    mGeometry.bind();

    BlendState blendState;

    blendState.blendEnabled = true;
    blendState.blendFunctionSrcRgb = BlendFunction::SRC_ALPHA;
    blendState.blendFunctionDstRgb = BlendFunction::ONE_MINUS_SRC_ALPHA;


    RasterizerState rasterizerState;
    rasterizerState.cullMode = CullMode::NONE;

    rasterizerState.scissorTestEnabled = true;
    RenderBackend::setScissors(renderSurface.viewport.x, renderSurface.viewport.y, renderSurface.viewport.width,
                               renderSurface.viewport.height);
    RenderBackend::setBlendState(blendState);
    RenderBackend::setRasterizerState(rasterizerState);

    // Draw
    RenderBackend::drawIndexed(PrimitiveType::TRIANGLE_STRIP, 0, (uint32_t) mGeometry.getIndexCount());

    RenderBackend::popDebugMarker();
}

OMAF_NS_END
