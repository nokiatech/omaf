
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
#include "Renderer/NVRCubeMapMesh.h"
#include "Graphics/NVRRenderBackend.h"

OMAF_NS_BEGIN
    CubeMapMesh::CubeMapMesh()
    : vertexBuffer(VertexBufferID::Invalid)
    , vertexCount(0)
    {
    }

    CubeMapMesh::~CubeMapMesh()
    {
        OMAF_ASSERT(vertexBuffer == VertexBufferID::Invalid, "");
    }

    void_t CubeMapMesh::create(float32_t width, float32_t height)
    {
        const uint32_t numVertices = 6 * 2 * 3;

        float32_t half_texel_x = (1.0f / width)* 0.5f;
        float32_t half_texel_y = (1.0f / height)* 0.5f;
        
        Vertex vertices[] =
        {
            // +x, right
            { 0.5f, 0.5f, -0.5f, 0.0f + half_texel_x, 1.0f - half_texel_y },
            { 0.5f, -0.5f, -0.5f, 0.0f + half_texel_x, 0.5f + half_texel_y },
            { 0.5f, 0.5f, 0.5f, 1.0f / 3.0f - half_texel_x, 1.0f - half_texel_y },
            
            { 0.5f, 0.5f, 0.5f, 1.0f / 3.0f - half_texel_x, 1.0f - half_texel_y },
            { 0.5f, -0.5f, -0.5f, 0.0f + half_texel_x, 0.5f + half_texel_y },
            { 0.5f, -0.5f, 0.5f, 1.0f / 3.0f - half_texel_x, 0.5f + half_texel_y },
            
            // -x, left
            { -0.5f, 0.5f, 0.5f, 1.0f / 3.0f + half_texel_x, 1.0f - half_texel_y },
            { -0.5f, -0.5f, 0.5f, 1.0f / 3.0f + half_texel_x, 0.5f + half_texel_y},
            { -0.5f, -0.5f, -0.5f, 2.0f / 3.0f - half_texel_x, 0.5f + half_texel_y},
            
            { -0.5f, 0.5f, -0.5f, 2.0f / 3.0f + half_texel_x, 1.0f - half_texel_y },
            { -0.5f, 0.5f, 0.5f, 1.0f / 3.0f + half_texel_x, 1.0f - half_texel_y },
            { -0.5f, -0.5f, -0.5f, 2.0f / 3.0f - half_texel_x, 0.5f + half_texel_y },
            
            // +y, front
            { -0.5f, 0.5f, -0.5f, 1.0f / 3.0f + half_texel_x, 0.5f - half_texel_y },
            { -0.5f, -0.5f, -0.5f, 1.0f / 3.0f + half_texel_x, 0.0f + half_texel_y},
            { 0.5f, -0.5f, -0.5f, 2.0f / 3.0f - half_texel_x, 0.0f + half_texel_y},
            
            { 0.5f, 0.5f, -0.5f, 2.0f / 3.0f - half_texel_x, 0.5f - half_texel_y },
            { -0.5f, 0.5f, -0.5f, 1.0f / 3.0f + half_texel_x, 0.5f - half_texel_y},
            { 0.5f, -0.5f, -0.5f, 2.0f / 3.0f - half_texel_x, 0.0f + half_texel_y},
            
            // -y, back
            { 0.5f, 0.5f, 0.5f, 2.0f / 3.0f + half_texel_x, 0.5f - half_texel_y },
            { 0.5f, -0.5f, 0.5f, 2.0f / 3.0f + half_texel_x, 0.0f + half_texel_y },
            { -0.5f, -0.5f, 0.5f, 1.0f - half_texel_x, 0.0f + half_texel_y },
            
            { -0.5f, 0.5f, 0.5f, 1.0f - half_texel_x, 0.5f - half_texel_y},
            { 0.5f, 0.5f, 0.5f, 2.0f / 3.0f + half_texel_x, 0.5f - half_texel_y },
            { -0.5f, -0.5f, 0.5f, 1.0f - half_texel_x, 0.0f + half_texel_y},
            
            // +z, up
            { -0.5f, 0.5f, 0.5f, 2.0f / 3.0f + half_texel_x, 1.0f - half_texel_y },
            { -0.5f, 0.5f, -0.5f, 2.0f / 3.0f + half_texel_x, 0.5f + half_texel_y },
            { 0.5f, 0.5f, -0.5f, 1.0f - half_texel_x, 0.5f + half_texel_y },
            
            { 0.5f, 0.5f, 0.5f, 1.0f - half_texel_x, 1.0f - half_texel_y },
            { -0.5f, 0.5f, 0.5f, 2.0f / 3.0f + half_texel_x, 1.0f - half_texel_y },
            { 0.5f, 0.5f, -0.5f, 1.0f - half_texel_x, 0.5f + half_texel_y },
            
            // -z, bottom
            { -0.5f, -0.5f, -0.5f, 0.0f + half_texel_x, 0.5f - half_texel_y},
            { -0.5f, -0.5f, 0.5f, 0.0f + half_texel_x, 0.0f + half_texel_y },
            { 0.5f, -0.5f, 0.5f, 1.0f / 3.0f - half_texel_x, 0.0f + half_texel_y },
            
            { 0.5f, -0.5f, -0.5f, 1.0f / 3.0f - half_texel_x, 0.5f - half_texel_y},
            { -0.5f, -0.5f, -0.5f, 0.0f + half_texel_x, 0.5f - half_texel_y },
            { 0.5f, -0.5f, 0.5f, 1.0f / 3.0f - half_texel_x, 0.0f + half_texel_y },
        };
        
        VertexDeclaration vertexDeclaration = VertexDeclaration()
        .begin()
            .addAttribute("aPosition", VertexAttributeFormat::FLOAT32, 3, false)
            .addAttribute("aTextureCoord", VertexAttributeFormat::FLOAT32, 2, false)
        .end();
        
        VertexBufferDesc vertexBufferDesc;
        vertexBufferDesc.declaration = vertexDeclaration;
        vertexBufferDesc.access = BufferAccess::STATIC;
        vertexBufferDesc.data = vertices;
        vertexBufferDesc.size = numVertices * OMAF_SIZE_OF(Vertex);
        
        vertexBuffer = RenderBackend::createVertexBuffer(vertexBufferDesc);
        vertexCount = numVertices;
    }

    void_t CubeMapMesh::bind()
    {
        RenderBackend::bindVertexBuffer(vertexBuffer);
    }

    void_t CubeMapMesh::destroy()
    {
        RenderBackend::destroyVertexBuffer(vertexBuffer);
        vertexBuffer = VertexBufferID::Invalid;
    }

    size_t CubeMapMesh::getVertexCount()
    {
        return vertexCount;
    }
OMAF_NS_END
