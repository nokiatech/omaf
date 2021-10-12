
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

#include "Graphics/NVRHandles.h"
#include "Math/OMAFVector2.h"
#include "NVRNamespace.h"

OMAF_NS_BEGIN
class EquirectangularMesh
{
public:
    EquirectangularMesh();
    ~EquirectangularMesh();

    void_t create(uint16_t columns, uint16_t rows);
    void_t createTile(const Vector2& dir, const Vector2& size, uint16_t resolutionX, uint16_t resolutionY);

    void_t
    createSphereRelativeOmniTile(const Vector2& dir, const Vector2& size, uint16_t resolutionX, uint16_t resolutionY);

    void_t createSphereRelative2DTile(const Vector2& dir, const Vector2& size);

    void_t bind();
    void_t destroy();

    size_t getVertexCount();
    size_t getIndexCount();

private:
    /**
     * Creates mesh over sphere.
     *
     * @param rotateBeforeMeshCreate Azimuth and elevation where mesh is created in the first place. Causes mesh to be
     * more triangular near poles.
     * @param rotateAfterMeshCreate Rotation around x and y axis that is done for mesh vertices after they have been
     * created.
     * @param size Degrees how big area mesh covers.
     * @param columnCount Number of columns in generated mesh.
     * @param rowCount Number of rows in generated mesh.
     */
    void_t createTileOnSphere(const Vector2& rotateBeforeMeshCreate,
                              const Vector2& rotateAfterMeshCreate,
                              const Vector2& size,
                              uint16_t columnCount,
                              uint16_t rowCount);

    void_t create2DPlaneOnSphere(const Vector2& rotateAfterMeshCreate, const Vector2& size);

    struct Vertex
    {
        float32_t x;
        float32_t y;
        float32_t z;

        float32_t u;
        float32_t v;

        float32_t ou;
        float32_t ov;
    };

    VertexBufferID vertexBuffer;
    IndexBufferID indexBuffer;

    size_t vertexCount;
    size_t indexCount;
};
OMAF_NS_END
