
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

#include "NVREssentials.h"

#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRBufferAccess.h"
#include "Graphics/NVRVertexDeclaration.h"
#include "Graphics/NVRComputeBufferAccess.h"

OMAF_NS_BEGIN
    class VertexBufferD3D11
    {
        public:

            VertexBufferD3D11();
            ~VertexBufferD3D11();

            const ID3D11Buffer* getHandle() const;

            bool_t create(ID3D11Device* device, const VertexDeclaration& declaration, BufferAccess::Enum access, const void_t* data = OMAF_NULL, size_t bytes = 0);
            void_t bufferData(ID3D11DeviceContext* deviceContext, size_t offset, const void_t* data, size_t bytes);
            void_t destroy();

            void_t bind(ID3D11DeviceContext* deviceContext);
            void_t unbind(ID3D11DeviceContext* deviceContext);

            void_t bindCompute(ID3D11DeviceContext* deviceContext, uint16_t stage, ComputeBufferAccess::Enum computeAccess);
            void_t unbindCompute(ID3D11DeviceContext* deviceContext, uint16_t stage);

            size_t getNumVertices();
            const VertexDeclaration& getVertexDeclaration() const;

        private:

            OMAF_NO_ASSIGN(VertexBufferD3D11);
            OMAF_NO_COPY(VertexBufferD3D11);

        private:

            ID3D11Buffer* mHandle;

            size_t mBytes;
            BufferAccess::Enum mBufferAccess;
            
            VertexDeclaration mVertexDeclaration;
    };
OMAF_NS_END
