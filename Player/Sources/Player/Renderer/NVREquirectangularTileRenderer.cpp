
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
#include "Renderer/NVREquirectangularTileRenderer.h"
#include "Math/OMAFMath.h"
#include "Graphics/NVRRenderBackend.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRNew.h"

namespace
{
    static const int EQUIRECT_MESH_COLS = 64;
    static const int EQUIRECT_MESH_ROWS = 32;
}

#ifdef _DEBUG
//#define DEBUG_DRAW_TILES
#endif // _DEBUG

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(EquirectangularTileRenderer)

    EquirectangularTileRenderer::EquirectangularTileRenderer()
    : mDirtyShader(true)
    , mAllocator(*MemorySystem::DefaultHeapAllocator())
    , mGeometryMap(*MemorySystem::DefaultHeapAllocator())
    {
    }

    EquirectangularTileRenderer::~EquirectangularTileRenderer()
    {
        destroy();
    }

    void_t EquirectangularTileRenderer::createImpl()
    {
        //has to be created dynamically, as there can be multiple sources
    }

    void_t EquirectangularTileRenderer::destroyImpl()
    {
        for (EquirectTileMeshMap::Iterator it = mGeometryMap.begin(); it != mGeometryMap.end(); ++it)
        {
            (*it)->destroy();
            OMAF_DELETE(mAllocator, *it);
        }

        mOpaqueShader.destroy();
        mMaskShader.destroy();
    }

    ProjectionType::Enum EquirectangularTileRenderer::getType() const
    {
        return ProjectionType::EQUIRECTANGULAR_TILE;
    }

    void_t EquirectangularTileRenderer::render(const HeadTransform& headTransform, const RenderSurface& renderSurface, const CoreProviderSources& sources, const TextureID& renderMask, const RenderingParameters& renderingParameters)
    {
        OMAF_ASSERT(sources.getSize() == 1, "Supports only one input source");
        OMAF_ASSERT(sources.at(0)->type == SourceType::EQUIRECTANGULAR_TILES || sources.at(0)->type == SourceType::EQUIRECTANGULAR_180, "Source is not an equirect panorama with tiles");

        RenderBackend::pushDebugMarker("EquirectangularPanoramaRenderer::render");

        bool_t useMask = renderMask != TextureID::Invalid;

        for (size_t i = 0, endi = sources.getSize(); i < endi; ++i)
        {
            EquirectangularTileSource* source = (EquirectangularTileSource*)sources.at(i);

            if (mDirtyShader)
            {
                mDirtyShader = false;
#ifdef DEBUG_DRAW_TILES
                mOpaqueShader.createTintVideoShader(source->videoFrame.format);
#else
                mOpaqueShader.createDefaultVideoShader(source->videoFrame.format);
                mMaskShader.createDefaultVideoShader(source->videoFrame.format, true);
#endif // DEBUG_DRAW_TILES
            }

            //get a correct mesh for the tile. TODO the map becomes relatively large in multi-resolution RWPK case, where a frame can consist of e.g 16-24 tiles, and there can be as many directions, so 256-576 elements in the map
            EquirectangularMesh* mesh;
            auto viewElement = mGeometryMap.find(source->sourceId);

            //if no geometry for this tile yet, create one
            if (viewElement == EquirectTileMeshMap::InvalidIterator)
            {
                EquirectangularMesh* geometry = OMAF_NEW(mAllocator, EquirectangularMesh)();

                geometry->createTile(makeVector2(source->centerLongitude, source->centerLatitude), makeVector2(source->spanLongitude, source->spanLatitude), EQUIRECT_MESH_COLS, EQUIRECT_MESH_ROWS);

                mGeometryMap.insert(source->sourceId, geometry);

                mesh = geometry;
            }
            else
            {
                mesh = *viewElement;
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


            // crop borders to mitigate flickering lines in case the texture is not full width or height
            float borderCropX = 0.f;
            float borderCropY = 0.f;
            if (source->textureRect.w < 1.f)
            {
                borderCropX = (1.f / source->videoFrame.width) * 0.5f;
            }
            if (source->textureRect.h < 1.f)
            {
                borderCropY = (1.f / source->videoFrame.height) * 0.5f; //half a pixel is enough
            }

            //pass the texture rectangle information to the shader
            Matrix44 vtm = source->videoFrame.matrix *
                makeTranslation(source->textureRect.x + borderCropX, source->textureRect.y + borderCropY, 0) *
                makeScale(source->textureRect.w - (borderCropX*2.f), source->textureRect.h - (borderCropY*2.f), 1);

            selectedShader.setDefaultVideoShaderConstants(mvp, vtm);
			
            if (useMask)
            {
                selectedShader.bindMaskTexture(renderMask, renderingParameters.clearColor);
            }

            // Bind geometry
            mesh->bind();

            BlendState blendState;
            blendState.blendEnabled = false;
            blendState.blendFunctionSrcRgb = BlendFunction::ONE;
            blendState.blendFunctionDstRgb = BlendFunction::ONE_MINUS_SRC_ALPHA;
            blendState.blendFunctionSrcAlpha = BlendFunction::ONE;
            blendState.blendFunctionDstAlpha = BlendFunction::ONE_MINUS_SRC_ALPHA;

            RasterizerState rasterizerState;
            rasterizerState.scissorTestEnabled = true;
            rasterizerState.cullMode = CullMode::NONE;

            RenderBackend::setBlendState(blendState);
            RenderBackend::setRasterizerState(rasterizerState);

            // Draw
#ifdef DEBUG_DRAW_TILES
            selectedShader.setTintVideoShaderConstants(makeColor4(1.5f, 1.f, 1.5f, 1.f));
#endif // DEBUG_DRAW_TILES
            RenderBackend::drawIndexed(PrimitiveType::TRIANGLE_STRIP, 0, (uint32_t) mesh->getIndexCount());

#ifdef DEBUG_DRAW_TILES
            rasterizerState.fillMode = FillMode::WIREFRAME;
            selectedShader.setTintVideoShaderConstants(makeColor4(2.f, 2.f, 2.f, 1.f));
            RenderBackend::setRasterizerState(rasterizerState);
            RenderBackend::drawIndexed(PrimitiveType::TRIANGLE_STRIP, 0, (uint32_t)mesh->getIndexCount());
#endif // DEBUG_DRAW_TILES
        }

        RenderBackend::popDebugMarker();
    }

OMAF_NS_END
