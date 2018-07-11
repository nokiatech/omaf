
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
#include "Platform/OMAFDataTypes.h"
#include "Graphics/NVRHandles.h"

OMAF_NS_BEGIN
    class CubeMapMesh
    {
        public:
            
            CubeMapMesh();
            ~CubeMapMesh();
            
            void_t create(float32_t width, float32_t height);
            void_t bind();
            void_t destroy();
            
            size_t getVertexCount();
            
        private:
            
            struct Vertex
            {
                float32_t x;
                float32_t y;
                float32_t z;
                
                float32_t u;
                float32_t v;
            };
            
            VertexBufferID vertexBuffer;
            
            size_t vertexCount;
    };
OMAF_NS_END
