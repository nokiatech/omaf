
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

#include "Graphics/NVRConfig.h"
#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRShaderConstantType.h"
#include "Foundation/NVRHashFunctions.h"
#include "Foundation/NVRFixedArray.h"

#include "Graphics/D3D11/NVRD3D11Error.h"

OMAF_NS_BEGIN
    // https://blogs.msdn.microsoft.com/chuckw/2012/05/07/hlsl-fxc-and-d3dcompile/
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476590(v=vs.85).aspx
    class MemoryAllocator;
    class ShaderD3D11
    {
        public:

            ShaderD3D11();
            ~ShaderD3D11();

            bool_t create(ID3D11Device* device, const char_t* vertexShader, const char_t* pixelShader);
            bool_t create(ID3D11Device* device, const char_t* computeShader);

            void_t destroy();

            void_t bind(ID3D11DeviceContext* deviceContext);
            void_t unbind(ID3D11DeviceContext* deviceContext);

            bool_t setUniform(ID3D11DeviceContext* deviceContext, ShaderConstantType::Enum type, HashValue name, const void_t* values, int numValues = 1);

            void_t flushUniforms(ID3D11DeviceContext* deviceContext);
        private:

            OMAF_NO_ASSIGN(ShaderD3D11);
            OMAF_NO_COPY(ShaderD3D11);

        private:

            struct ShaderUniform
            {
                D3D_SHADER_VARIABLE_CLASS variableClass;
                D3D_SHADER_VARIABLE_TYPE variableType;
                size_t offset;
                uint16_t size;
                size_t numElements;
                HashValue name;

                #if OMAF_ENABLE_DX_DEBUGGING
                
                FixedString64 debugName;

                #endif
            };

        private:

            const ShaderUniform* getUniform(HashValue name) const;

            ID3DBlob* compileShader(const char_t* shader, const char_t* entry, const char_t* target);

        private:

            ID3D11VertexShader* mVertexShader;
            ID3D11PixelShader* mPixelShader;
            ID3D11ComputeShader* mComputeShader;

            ID3DBlob* mVertexShaderByteCode;
            ID3DBlob* mPixelShaderByteCode;
            ID3DBlob* mComputeShaderByteCode;

            ID3D11InputLayout* mInputLayout;

            ID3D11Buffer* mConstantBuffer;
            bool_t mConstantBufferDirty;
            uint32_t mConstantCacheSize;
            uint8_t *mConstantCache;

            FixedArray<ShaderUniform, OMAF_MAX_SHADER_UNIFORMS> mUniforms;
            MemoryAllocator* mAllocator;
    };
OMAF_NS_END
