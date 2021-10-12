
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
#include "Renderer/NVREquirectangularMesh.h"
#include "Graphics/NVRRenderBackend.h"
#include "Math/OMAFMathConstants.h"
#include "Math/OMAFMatrix44.h"
#include "Math/OMAFQuaternion.h"
#include "Math/OMAFVector2.h"

#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRNew.h"

OMAF_NS_BEGIN
EquirectangularMesh::EquirectangularMesh()
    : vertexBuffer(VertexBufferID::Invalid)
    , indexBuffer(IndexBufferID::Invalid)
    , vertexCount(0)
    , indexCount(0)
{
}

EquirectangularMesh::~EquirectangularMesh()
{
    OMAF_ASSERT(vertexBuffer == VertexBufferID::Invalid, "");
    OMAF_ASSERT(indexBuffer == IndexBufferID::Invalid, "");
}

void_t EquirectangularMesh::create(uint16_t columns, uint16_t rows)
{
    OMAF_ASSERT(columns > 0, "");
    OMAF_ASSERT(rows > 0, "");

    uint32_t meshColumns = columns;
    uint32_t meshRows = rows;
    uint32_t vertexColumns = columns + 1;
    uint32_t vertexRows = rows + 1;

    float32_t width = 1.0f;
    float32_t height = 1.0f;

    float32_t bottom = 0.0f;
    float32_t top = 1.0f;
    float32_t left = 0.0f;
    float32_t right = 1.0f;

    // Create sphere vertices and vertex buffer
    MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

    uint32_t numVertices = vertexColumns * vertexRows;
    Vertex* vertices = (Vertex*) OMAF_ALLOC(allocator, OMAF_SIZE_OF(Vertex) * numVertices);
    Vertex* vertexPtr = vertices;

    float32_t longitudeStep = width / columns;
    float32_t latitudeStep = height / rows;

    float32_t u = 0.0f;
    float32_t v = top;

    for (uint32_t j = 0; j < vertexRows; ++j, v -= latitudeStep)
    {
        u = left;

        for (uint32_t i = 0; i < vertexColumns; ++i, u += longitudeStep)
        {
            float32_t r = sinf(OMAF_PI * j / rows);

            float32_t x = -r * sinf(2.0f * OMAF_PI * i / columns);
            float32_t y = cosf(OMAF_PI * j / rows);
            float32_t z = r * cosf(2.0f * OMAF_PI * i / columns);

            vertexPtr->x = x;
            vertexPtr->y = y;
            vertexPtr->z = z;

            if (i < columns)
            {
                vertexPtr->u = u;
                vertexPtr->ou = u;
            }
            else
            {
                vertexPtr->u = right;
                vertexPtr->ou = right;
            }

            if (j < rows)
            {
                vertexPtr->v = v;
                vertexPtr->ov = v;
            }
            else
            {
                vertexPtr->v = bottom;
                vertexPtr->ov = bottom;
            }

            ++vertexPtr;
        }
    }

    VertexDeclaration vertexDeclaration =
        VertexDeclaration()
            .begin()
            .addAttribute("aPosition", VertexAttributeFormat::FLOAT32, 3, false)
            .addAttribute("aTextureCoord", VertexAttributeFormat::FLOAT32, 2, false)
            .addAttribute("aTextureOrigCoord", VertexAttributeFormat::FLOAT32, 2, false)
            .end();

    VertexBufferDesc vertexBufferDesc;
    vertexBufferDesc.declaration = vertexDeclaration;
    vertexBufferDesc.access = BufferAccess::STATIC;
    vertexBufferDesc.data = vertices;
    vertexBufferDesc.size = numVertices * OMAF_SIZE_OF(Vertex);

    vertexBuffer = RenderBackend::createVertexBuffer(vertexBufferDesc);
    vertexCount = numVertices;

    OMAF_FREE(allocator, vertices);
    vertices = OMAF_NULL;

    // Create sphere indices and index buffer
    uint32_t numIndices = 2 * meshColumns * meshRows - 2 + 4 * meshRows;
    uint32_t* indices = (uint32_t*) OMAF_ALLOC(allocator, OMAF_SIZE_OF(uint32_t) * numIndices);
    uint32_t* indexPtr = indices;

    uint32_t n = 0;

    for (uint32_t j = 0; j < meshRows; ++j)
    {
        // Repeat first index
        if (j != 0)
        {
            *(indexPtr++) = n;
        }

        // Insert one row's worth of vertex index numbers in top, bottom, top, bottom... order
        for (uint32_t i = 0; i < vertexColumns; ++i, ++n)
        {
            *(indexPtr++) = n;
            *(indexPtr++) = n + vertexColumns;
        }

        // Repeat last index
        if (j < meshRows - 1)
        {
            *(indexPtr++) = n + vertexColumns - 1;
        }
    }

    IndexBufferDesc indexBufferDesc;
    indexBufferDesc.access = BufferAccess::STATIC;
    indexBufferDesc.format = IndexBufferFormat::UINT32;
    indexBufferDesc.data = indices;
    indexBufferDesc.size = OMAF_SIZE_OF(uint32_t) * numIndices;

    indexBuffer = RenderBackend::createIndexBuffer(indexBufferDesc);
    indexCount = numIndices;

    OMAF_FREE(allocator, indices);
    indices = OMAF_NULL;
}

void_t
EquirectangularMesh::createTile(const Vector2& dir, const Vector2& size, uint16_t resolutionX, uint16_t resolutionY)
{
    float32_t diffPerDegreeX = resolutionX / 360.f;
    float32_t diffPerDegreeY = resolutionY / 180.f;

    uint32_t meshColumns = (uint32_t) ceilf(size.x * diffPerDegreeX);
    uint32_t meshRows = (uint32_t) ceilf(size.y * diffPerDegreeY);

    createTileOnSphere(dir, makeVector2(0, 0), size, meshColumns, meshRows);
}

void_t EquirectangularMesh::createSphereRelativeOmniTile(const Vector2& dir,
                                                         const Vector2& size,
                                                         uint16_t resolutionX,
                                                         uint16_t resolutionY)
{
    float32_t diffPerDegreeX = resolutionX / 360.f;
    float32_t diffPerDegreeY = resolutionY / 180.f;

    uint32_t meshColumns = (uint32_t) ceilf(size.x * diffPerDegreeX);
    uint32_t meshRows = (uint32_t) ceilf(size.y * diffPerDegreeY);

    createTileOnSphere(makeVector2(0, 0), dir, size, meshColumns, meshRows);
}

void_t EquirectangularMesh::createSphereRelative2DTile(const Vector2& dir, const Vector2& size)
{
    create2DPlaneOnSphere(dir, size);
}

void_t EquirectangularMesh::create2DPlaneOnSphere(const Vector2& rotateAfterMeshCreate, const Vector2& size)
{
    float32_t planeHalfHeight = tanf(size.y / 2 * OMAF_PI / 180);
    float32_t planeHalfWidth = tanf(size.x / 2 * OMAF_PI / 180);

    // Create sphere vertices and vertex buffer
    MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

    uint32_t numVertices = 4;
    Vertex* vertices = (Vertex*) OMAF_ALLOC(allocator, OMAF_SIZE_OF(Vertex) * numVertices);
    Vertex* vertexPtr = vertices;

    float32_t pointX[4] = {-planeHalfWidth, planeHalfWidth, planeHalfWidth, -planeHalfWidth};
    float32_t pointY[4] = {-planeHalfHeight, -planeHalfHeight, planeHalfHeight, planeHalfHeight};

    float32_t u[4] = {0, 1, 1, 0};
    float32_t v[4] = {0, 0, 1, 1};

    for (uint32_t i = 0; i < numVertices; i++)
    {
        float32_t x = pointX[i];
        float32_t y = pointY[i];
        float32_t z = 0;

        // rotate x,y,z vertex to correct direction
        auto rotX = makeQuaternion(makeVector3(1, 0, 0), rotateAfterMeshCreate.y * OMAF_PI / 180);
        auto rotY = makeQuaternion(makeVector3(0, 1, 0), -rotateAfterMeshCreate.x * OMAF_PI / 180);
        Vector3 point = makeVector3(x, y, z);

        auto pointRotX = rotate(rotX, point);
        auto pointRotXY = rotate(rotY, pointRotX);

        vertexPtr->x = pointRotXY.x;
        vertexPtr->y = pointRotXY.y;
        vertexPtr->z = pointRotXY.z;

        vertexPtr->u = u[i];
        vertexPtr->v = v[i];
        vertexPtr->ou = pointX[i];
        vertexPtr->ov = pointY[i];

        ++vertexPtr;
    }

    VertexDeclaration vertexDeclaration =
        VertexDeclaration()
            .begin()
            .addAttribute("aPosition", VertexAttributeFormat::FLOAT32, 3, false)
            .addAttribute("aTextureCoord", VertexAttributeFormat::FLOAT32, 2, false)
            .addAttribute("aTextureOrigCoord", VertexAttributeFormat::FLOAT32, 2, false)
            .end();

    VertexBufferDesc vertexBufferDesc;
    vertexBufferDesc.declaration = vertexDeclaration;
    vertexBufferDesc.access = BufferAccess::STATIC;
    vertexBufferDesc.data = vertices;
    vertexBufferDesc.size = numVertices * OMAF_SIZE_OF(Vertex);

    vertexBuffer = RenderBackend::createVertexBuffer(vertexBufferDesc);
    vertexCount = numVertices;

    OMAF_FREE(allocator, vertices);
    vertices = OMAF_NULL;

    // Create face indices and index buffer
    uint32_t numIndices = numVertices + 1;
    uint32_t* indices = (uint32_t*) OMAF_ALLOC(allocator, OMAF_SIZE_OF(uint32_t) * numIndices);

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 3;
    indices[4] = 0;

    IndexBufferDesc indexBufferDesc;
    indexBufferDesc.access = BufferAccess::STATIC;
    indexBufferDesc.format = IndexBufferFormat::UINT32;
    indexBufferDesc.data = indices;
    indexBufferDesc.size = OMAF_SIZE_OF(uint32_t) * numIndices;

    indexBuffer = RenderBackend::createIndexBuffer(indexBufferDesc);
    indexCount = numIndices;

    OMAF_FREE(allocator, indices);
    indices = OMAF_NULL;
}


void_t EquirectangularMesh::createTileOnSphere(const Vector2& rotateBeforeMeshCreate,
                                               const Vector2& rotateAfterMeshCreate,
                                               const Vector2& size,
                                               uint16_t columnCount,
                                               uint16_t rowCount)
{
    OMAF_ASSERT(columnCount > 0, "");
    OMAF_ASSERT(rowCount > 0, "");

    //[-1 , 1]
    Vector2 viewMin = (rotateBeforeMeshCreate - size * 0.5f) / makeVector2(180.0f, 90.f);
    Vector2 viewMax = (rotateBeforeMeshCreate + size * 0.5f) / makeVector2(180.0f, 90.f);

    Vector2 viewMinN = (viewMin + makeVector2(1.f, 1.f)) * 0.5f;
    Vector2 viewMaxN = (viewMax + makeVector2(1.f, 1.f)) * 0.5f;

    uint32_t vertexColumns = columnCount + 1;
    uint32_t vertexRows = rowCount + 1;

    float32_t width = viewMaxN.x - viewMinN.x;
    float32_t height = viewMaxN.y - viewMinN.y;

    float32_t bottom = viewMinN.y;
    float32_t top = viewMaxN.y;
    float32_t left = viewMinN.x;
    float32_t right = viewMaxN.x;

    // Create sphere vertices and vertex buffer
    MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

    uint32_t numVertices = vertexColumns * vertexRows;
    Vertex* vertices = (Vertex*) OMAF_ALLOC(allocator, OMAF_SIZE_OF(Vertex) * numVertices);
    Vertex* vertexPtr = vertices;

    float32_t longitudeStep = width / columnCount;
    float32_t latitudeStep = height / rowCount;

    float32_t startU = 0.0f;
    float32_t startV = 0.0f;
    float32_t dU = 1.0f / (vertexColumns - 1);
    float32_t dV = 1.0f / (vertexRows - 1);

    float32_t u = startU;
    float32_t v = startV;

    // full sphere UV coordinates
    float32_t ou = left;
    float32_t ov = bottom;
    float32_t dOU = (right - left) / (vertexColumns - 1);
    float32_t dOV = (top - bottom) / (vertexRows - 1);

    for (uint32_t j = 0; j < vertexRows; ++j, v += dV, ov += dOV)
    {
        u = startU;
        ou = left;

        for (uint32_t i = 0; i < vertexColumns; ++i, u += dU, ou += dOU)
        {
            u = (u < dU) ? 0.f : u;

            float32_t pointX = left + i * (width / columnCount);
            float32_t pointY = bottom + j * (height / rowCount);

            float32_t r = sinf(OMAF_PI * pointY);

            float32_t x = -r * sinf(2.0f * OMAF_PI * pointX);
            float32_t y = -cosf(OMAF_PI * pointY);
            float32_t z = r * cosf(2.0f * OMAF_PI * pointX);

            // rotate x,y,z vertex to correct direction
            auto rotX = makeQuaternion(makeVector3(1, 0, 0), rotateAfterMeshCreate.y * OMAF_PI / 180);
            auto rotY = makeQuaternion(makeVector3(0, 1, 0), -rotateAfterMeshCreate.x * OMAF_PI / 180);
            Vector3 point = makeVector3(x, y, z);

            auto pointRotX = rotate(rotX, point);
            auto pointRotXY = rotate(rotY, pointRotX);

            vertexPtr->x = pointRotXY.x;
            vertexPtr->y = pointRotXY.y;
            vertexPtr->z = pointRotXY.z;

            vertexPtr->u = u;
            vertexPtr->v = v;
            vertexPtr->ou = ou;
            vertexPtr->ov = ov;

            ++vertexPtr;
        }
    }

    VertexDeclaration vertexDeclaration =
        VertexDeclaration()
            .begin()
            .addAttribute("aPosition", VertexAttributeFormat::FLOAT32, 3, false)
            .addAttribute("aTextureCoord", VertexAttributeFormat::FLOAT32, 2, false)
            .addAttribute("aTextureOrigCoord", VertexAttributeFormat::FLOAT32, 2, false)
            .end();

    VertexBufferDesc vertexBufferDesc;
    vertexBufferDesc.declaration = vertexDeclaration;
    vertexBufferDesc.access = BufferAccess::STATIC;
    vertexBufferDesc.data = vertices;
    vertexBufferDesc.size = numVertices * OMAF_SIZE_OF(Vertex);

    vertexBuffer = RenderBackend::createVertexBuffer(vertexBufferDesc);
    vertexCount = numVertices;

    OMAF_FREE(allocator, vertices);
    vertices = OMAF_NULL;

    // Create sphere indices and index buffer
    uint32_t numIndices = 2 * columnCount * rowCount - 2 + 4 * rowCount;
    uint32_t* indices = (uint32_t*) OMAF_ALLOC(allocator, OMAF_SIZE_OF(uint32_t) * numIndices);
    uint32_t* indexPtr = indices;

    uint32_t n = 0;

    for (uint32_t j = 0; j < rowCount; ++j)
    {
        // Repeat first index
        if (j != 0)
        {
            *(indexPtr++) = n;
        }

        // Insert one row's worth of vertex index numbers in top, bottom, top, bottom... order
        for (uint32_t i = 0; i < vertexColumns; ++i, ++n)
        {
            *(indexPtr++) = n;
            *(indexPtr++) = n + vertexColumns;
        }

        // Repeat last index
        if (j < rowCount - 1)
        {
            *(indexPtr++) = n + vertexColumns - 1;
        }
    }

    IndexBufferDesc indexBufferDesc;
    indexBufferDesc.access = BufferAccess::STATIC;
    indexBufferDesc.format = IndexBufferFormat::UINT32;
    indexBufferDesc.data = indices;
    indexBufferDesc.size = OMAF_SIZE_OF(uint32_t) * numIndices;

    indexBuffer = RenderBackend::createIndexBuffer(indexBufferDesc);
    indexCount = numIndices;

    OMAF_FREE(allocator, indices);
    indices = OMAF_NULL;
}

void_t EquirectangularMesh::bind()
{
    RenderBackend::bindVertexBuffer(vertexBuffer);
    RenderBackend::bindIndexBuffer(indexBuffer);
}

void_t EquirectangularMesh::destroy()
{
    RenderBackend::destroyVertexBuffer(vertexBuffer);
    vertexBuffer = VertexBufferID::Invalid;

    RenderBackend::destroyIndexBuffer(indexBuffer);
    indexBuffer = IndexBufferID::Invalid;
}

size_t EquirectangularMesh::getVertexCount()
{
    return vertexCount;
}

size_t EquirectangularMesh::getIndexCount()
{
    return indexCount;
}
OMAF_NS_END
