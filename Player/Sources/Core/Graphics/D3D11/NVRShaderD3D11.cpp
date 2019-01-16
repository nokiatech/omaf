
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
#include "Foundation/NVRNew.h"
#include "Graphics/D3D11/NVRShaderD3D11.h"

#include "Graphics/D3D11/NVRD3D11Error.h"
#include "Graphics/D3D11/NVRD3D11Utilities.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(ShaderD3D11)

    static DXGI_FORMAT getShaderParameterFormat(D3D11_SIGNATURE_PARAMETER_DESC spd)
    {
        // Determine DXGI format
        if (spd.Mask == 1)
        {
            switch (spd.ComponentType)
            {
                case D3D_REGISTER_COMPONENT_UINT32:     return DXGI_FORMAT_R32_UINT;
                case D3D_REGISTER_COMPONENT_SINT32:     return DXGI_FORMAT_R32_SINT;
                case D3D_REGISTER_COMPONENT_FLOAT32:    return DXGI_FORMAT_R32_FLOAT;

                default:
                    OMAF_ASSERT(false, "Invalid component type");
                    break;
            }
        }
        else if (spd.Mask <= 3)
        {
            switch (spd.ComponentType)
            {
                case D3D_REGISTER_COMPONENT_UINT32:     return DXGI_FORMAT_R32G32_UINT;
                case D3D_REGISTER_COMPONENT_SINT32:     return DXGI_FORMAT_R32G32_SINT;
                case D3D_REGISTER_COMPONENT_FLOAT32:    return DXGI_FORMAT_R32G32_FLOAT;

                default:
                    OMAF_ASSERT(false, "Invalid component type");
                    break;
            }
        }
        else if (spd.Mask <= 7)
        {
            switch (spd.ComponentType)
            {
                case D3D_REGISTER_COMPONENT_UINT32:     return DXGI_FORMAT_R32G32B32_UINT;
                case D3D_REGISTER_COMPONENT_SINT32:     return DXGI_FORMAT_R32G32B32_SINT;
                case D3D_REGISTER_COMPONENT_FLOAT32:    return DXGI_FORMAT_R32G32B32_FLOAT;

                default:
                    OMAF_ASSERT(false, "Invalid component type");
                    break;
            }
        }
        else if (spd.Mask <= 15)
        {
            switch (spd.ComponentType)
            {
                case D3D_REGISTER_COMPONENT_UINT32:     return DXGI_FORMAT_R32G32B32A32_UINT;
                case D3D_REGISTER_COMPONENT_SINT32:     return DXGI_FORMAT_R32G32B32A32_SINT;
                case D3D_REGISTER_COMPONENT_FLOAT32:    return DXGI_FORMAT_R32G32B32A32_FLOAT;

                default:
                    OMAF_ASSERT(false, "Invalid component type");
                    break;
            }
        }

        return DXGI_FORMAT(0);
    }

    struct ShaderConstantTypeInfo
    {
        D3D_SHADER_VARIABLE_CLASS variableClass;
        D3D_SHADER_VARIABLE_TYPE variableType;
        uint8_t sizeInBytes;
        uint8_t paddedSizeInBytes;
    };

    static ShaderConstantTypeInfo getShaderConstantTypeInfo(ShaderConstantType::Enum constantType)
    {
        OMAF_ASSERT(constantType != ShaderConstantType::INVALID, "");
        OMAF_ASSERT(constantType < ShaderConstantType::COUNT, "");

        ShaderConstantTypeInfo infos[ShaderConstantType::COUNT] =
        {
            /* BOOL */          { D3D_SVC_SCALAR, D3D_SVT_BOOL, 4, 16 },
            /* BOOL2 */         { D3D_SVC_VECTOR, D3D_SVT_BOOL, 8, 16 },
            /* BOOL3 */         { D3D_SVC_VECTOR, D3D_SVT_BOOL, 12, 16 },
            /* BOOL4 */         { D3D_SVC_VECTOR, D3D_SVT_BOOL, 16, 16 },

            /* INTEGER */       { D3D_SVC_SCALAR, D3D_SVT_INT, 4, 16 },
            /* INTEGER2 */      { D3D_SVC_VECTOR, D3D_SVT_INT, 8, 16 },
            /* INTEGER3 */      { D3D_SVC_VECTOR, D3D_SVT_INT, 12, 16 },
            /* INTEGER4 */      { D3D_SVC_VECTOR, D3D_SVT_INT, 16, 16 },

            /* FLOAT */         { D3D_SVC_SCALAR, D3D_SVT_FLOAT, 4, 16 },
            /* FLOAT2 */        { D3D_SVC_VECTOR, D3D_SVT_FLOAT, 8, 16 },
            /* FLOAT3 */        { D3D_SVC_VECTOR, D3D_SVT_FLOAT, 12, 16 },
            /* FLOAT4 */        { D3D_SVC_VECTOR, D3D_SVT_FLOAT, 16, 16 },

            /* MATRIX22 */      { D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT, 16, 16 },
            /* MATRIX33 */      { D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT, 36, 16 },
            /* MATRIX44 */      { D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT, 64, 16 },

            // Note: Samplers are handled differently compared to normal shader constants
            /* SAMPLER_2D */    { D3D_SVC_OBJECT, D3D_SVT_SAMPLER2D, 0, 0 },
            /* SAMPLER_3D */    { D3D_SVC_OBJECT, D3D_SVT_SAMPLER3D, 0, 0 },
            /* SAMPLER_CUBE */  { D3D_SVC_OBJECT, D3D_SVT_SAMPLERCUBE, 0, 0 },
        };

        OMAF_COMPILE_ASSERT(OMAF_ARRAY_SIZE(infos) == ShaderConstantType::COUNT);

        return infos[constantType];
    }

    ShaderD3D11::ShaderD3D11()
    : mVertexShader(NULL)
    , mPixelShader(NULL)
    , mComputeShader(NULL)
    , mVertexShaderByteCode(NULL)
    , mPixelShaderByteCode(NULL)
    , mComputeShaderByteCode(NULL)
    , mInputLayout(NULL)
    , mConstantBuffer(NULL)
    , mConstantBufferDirty(true)
    , mConstantCacheSize(0)
    , mConstantCache(NULL)
    , mAllocator(MemorySystem::DefaultHeapAllocator())
    {
    }

    ShaderD3D11::~ShaderD3D11()
    {
        OMAF_ASSERT(mVertexShader == NULL, "Shader is not destroyed");
        OMAF_ASSERT(mPixelShader == NULL, "Shader is not destroyed");
        OMAF_ASSERT(mComputeShader == NULL, "Shader is not destroyed");

        OMAF_ASSERT(mVertexShaderByteCode == NULL, "Shader is not destroyed");
        OMAF_ASSERT(mPixelShaderByteCode == NULL, "Shader is not destroyed");
        OMAF_ASSERT(mComputeShaderByteCode == NULL, "Shader is not destroyed");

        OMAF_ASSERT(mInputLayout == NULL, "Shader is not destroyed");
        OMAF_ASSERT(mConstantBuffer == NULL, "Shader is not destroyed");
        OMAF_ASSERT(mConstantCache == NULL, "Shader is not destroyed");
    }

    bool_t ShaderD3D11::create(ID3D11Device* device, const char_t* vertexShader, const char_t* pixelShader)
    {
        // Create vertex shader
        HRESULT result = 0;

        mVertexShaderByteCode = compileShader(vertexShader, "mainVS", "vs_5_0");

        if (mVertexShaderByteCode == NULL)
        {
            return false;
        }

        result = device->CreateVertexShader(mVertexShaderByteCode->GetBufferPointer(),
                                            mVertexShaderByteCode->GetBufferSize(),
                                            NULL,
                                            &mVertexShader);

        if (FAILED(result))
        {
            OMAF_DX_RELEASE(mVertexShaderByteCode, 0);

            return false;
        }

        // Create pixel shader
        mPixelShaderByteCode = compileShader(pixelShader, "mainPS", "ps_5_0");

        if (mPixelShaderByteCode == NULL)
        {
            return false;
        }

        result = device->CreatePixelShader(mPixelShaderByteCode->GetBufferPointer(),
                                           mPixelShaderByteCode->GetBufferSize(),
                                           NULL,
                                           &mPixelShader);

        if (FAILED(result))
        {
            OMAF_DX_RELEASE(mPixelShaderByteCode, 0);

            return false;
        }

        {
            // Reflect shader info
            ID3D11ShaderReflection* shaderReflection = NULL;

            HRESULT result = D3DReflect(mVertexShaderByteCode->GetBufferPointer(),
                                        mVertexShaderByteCode->GetBufferSize(),
                                        IID_ID3D11ShaderReflection,
                                        (void**)&shaderReflection);

            if (FAILED(result))
            {
                return false;
            }

            // Get shader info
            D3D11_SHADER_DESC shaderDesc;
            shaderReflection->GetDesc(&shaderDesc);

            // Create input elements
            OMAF_ASSERT(shaderDesc.InputParameters < OMAF_MAX_SHADER_ATTRIBUTES, "Too many shader attributes");

            D3D11_INPUT_ELEMENT_DESC inputElementDescs[OMAF_MAX_SHADER_ATTRIBUTES];
            ZeroMemory(inputElementDescs, sizeof(inputElementDescs));

            for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
            {
                D3D11_SIGNATURE_PARAMETER_DESC signatureParameterDesc;
                shaderReflection->GetInputParameterDesc(i, &signatureParameterDesc);

                D3D11_INPUT_ELEMENT_DESC inputElementDesc;
                inputElementDesc.SemanticName = signatureParameterDesc.SemanticName;
                inputElementDesc.SemanticIndex = signatureParameterDesc.SemanticIndex;
                inputElementDesc.InputSlot = 0;
                inputElementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                inputElementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                inputElementDesc.InstanceDataStepRate = 0;
                inputElementDesc.Format = getShaderParameterFormat(signatureParameterDesc);

                inputElementDescs[i] = inputElementDesc;
            }

            // Create input layout
            result = device->CreateInputLayout(inputElementDescs,
                                               shaderDesc.InputParameters,
                                               mVertexShaderByteCode->GetBufferPointer(),
                                               mVertexShaderByteCode->GetBufferSize(),
                                               &mInputLayout);

            if (FAILED(result))
            {
                return false;
            }

            // Get constant buffer info
            OMAF_ASSERT(shaderDesc.ConstantBuffers <= 1, "Only supports $Globals constant buffer");

            for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i)
            {
                ID3D11ShaderReflectionConstantBuffer* constantBuffer = shaderReflection->GetConstantBufferByIndex(i);

                D3D11_SHADER_BUFFER_DESC shaderBufferDesc;
                constantBuffer->GetDesc(&shaderBufferDesc);

                D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
                shaderReflection->GetResourceBindingDescByName(shaderBufferDesc.Name, &shaderInputBindDesc);

                if (shaderBufferDesc.Type == D3D_CT_CBUFFER)
                {
                    for (UINT j = 0; j < shaderBufferDesc.Variables; ++j)
                    {
                        ID3D11ShaderReflectionVariable* variable = constantBuffer->GetVariableByIndex(j);
                        ID3D11ShaderReflectionType* type = variable->GetType();

                        D3D11_SHADER_VARIABLE_DESC variableDesc;
                        variable->GetDesc(&variableDesc);

                        D3D11_SHADER_TYPE_DESC variableTypeDesc;
                        type->GetDesc(&variableTypeDesc);

                        HashFunction<const char_t*> hashFunction;

                        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632(v=vs.85).aspx
                        // https://geidav.wordpress.com/2013/03/05/hidden-hlsl-performance-hit-accessing-unpadded-arrays-in-constant-buffers/

                        ShaderUniform uniform;
                        uniform.variableClass = variableTypeDesc.Class;
                        uniform.variableType = variableTypeDesc.Type;
                        uniform.offset = variableDesc.StartOffset;
                        uniform.size = variableDesc.Size;
                        uniform.numElements = variableTypeDesc.Elements;
                        uniform.name = hashFunction(variableDesc.Name);

                        #if OMAF_ENABLE_DX_DEBUGGING

                        uniform.debugName = variableDesc.Name;

                        #endif

                        mUniforms.add(uniform);
                    }
                }
                else if (shaderBufferDesc.Type == D3D_CT_TBUFFER)
                {
                    OMAF_ASSERT_NOT_IMPLEMENTED();
                }
                else if (shaderBufferDesc.Type == D3D_CT_INTERFACE_POINTERS)
                {
                    OMAF_ASSERT_NOT_IMPLEMENTED();
                }
                else if (shaderBufferDesc.Type == D3D_CT_RESOURCE_BIND_INFO)
                {
                    OMAF_ASSERT_NOT_IMPLEMENTED();
                }
                else
                {
                    OMAF_ASSERT_UNREACHABLE();
                }

                // Create constant buffer
                D3D11_BUFFER_DESC constantBufferDesc;
                mConstantCacheSize = constantBufferDesc.ByteWidth = shaderBufferDesc.Size;
                constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
                constantBufferDesc.CPUAccessFlags = 0;
                constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                constantBufferDesc.MiscFlags = 0;
                constantBufferDesc.StructureByteStride = 0;

                OMAF_DX_CHECK(device->CreateBuffer(&constantBufferDesc, NULL, &mConstantBuffer));
                mConstantCache = OMAF_NEW_ARRAY(*mAllocator, uint8_t, mConstantCacheSize);
                ZeroMemory(mConstantCache, mConstantCacheSize);
            }

            OMAF_DX_RELEASE(shaderReflection, 0);
        }

        return true;
    }

    bool_t ShaderD3D11::create(ID3D11Device* device, const char_t* computeShader)
    {
        mComputeShaderByteCode = compileShader(computeShader, "mainCS", "cs_5_0");

        if (mComputeShaderByteCode == NULL)
        {
            return false;
        }

        HRESULT result = device->CreateComputeShader(mComputeShaderByteCode->GetBufferPointer(),
                                                     mComputeShaderByteCode->GetBufferSize(),
                                                     NULL,
                                                     &mComputeShader);

        if (FAILED(result))
        {
            return false;
        }

        return true;
    }

    void_t ShaderD3D11::bind(ID3D11DeviceContext* deviceContext)
    {
        OMAF_ASSERT_NOT_NULL(deviceContext);

        // Set shaders and input layout
        if (mVertexShader != NULL && mPixelShader != NULL)
        {
            deviceContext->VSSetShader(mVertexShader, NULL, 0);
            deviceContext->PSSetShader(mPixelShader, NULL, 0);
        }
        else if (mComputeShader != NULL)
        {
            deviceContext->CSSetShader(mComputeShader, NULL, 0);
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }

        deviceContext->IASetInputLayout(mInputLayout);

        // Set constant buffers
        if (mConstantBuffer != NULL)
        {
            if (mVertexShader != NULL && mPixelShader != NULL)
            {
                deviceContext->VSSetConstantBuffers(0, 1, &mConstantBuffer);
                deviceContext->PSSetConstantBuffers(0, 1, &mConstantBuffer);
            }
            else if (mComputeShader != NULL)
            {
                deviceContext->CSSetConstantBuffers(0, 1, &mConstantBuffer);
            }
            else
            {
                OMAF_ASSERT_UNREACHABLE();
            }
        }
    }

    void_t ShaderD3D11::unbind(ID3D11DeviceContext* deviceContext)
    {
        if (mVertexShader != NULL && mPixelShader != NULL)
        {
            deviceContext->VSSetShader(NULL, NULL, 0);
            deviceContext->PSSetShader(NULL, NULL, 0);
        }
        else if (mComputeShader != NULL)
        {
            deviceContext->CSSetShader(NULL, NULL, 0);
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }

        deviceContext->IASetInputLayout(NULL);

        if (mConstantBuffer != NULL)
        {
            ID3D11Buffer* nullBuffer = NULL;

            if (mVertexShader != NULL && mPixelShader != NULL)
            {
                deviceContext->VSSetConstantBuffers(0, 1, &nullBuffer);
                deviceContext->PSSetConstantBuffers(0, 1, &nullBuffer);
            }
            else if (mComputeShader != NULL)
            {
                deviceContext->CSSetConstantBuffers(0, 1, &nullBuffer);
            }
            else
            {
                OMAF_ASSERT_UNREACHABLE();
            }
        }
    }

    void_t ShaderD3D11::destroy()
    {
        OMAF_DX_RELEASE(mVertexShader, 0);
        OMAF_DX_RELEASE(mPixelShader, 0);
        OMAF_DX_RELEASE(mComputeShader, 0);

        OMAF_DX_RELEASE(mVertexShaderByteCode, 0);
        OMAF_DX_RELEASE(mPixelShaderByteCode, 0);
        OMAF_DX_RELEASE(mComputeShaderByteCode, 0);

        OMAF_DX_RELEASE(mInputLayout, 0);

        OMAF_DX_RELEASE(mConstantBuffer, 0);
        OMAF_DELETE_ARRAY(*mAllocator,mConstantCache);
        mConstantCache = NULL;
        mConstantCacheSize = 0;

        mUniforms.clear();
    }

    bool_t ShaderD3D11::setUniform(ID3D11DeviceContext* deviceContext, ShaderConstantType::Enum type, HashValue name, const void_t* values, int numValues)
    {
        // Textures and SamplerStates are bound differently compared to normal shader constants, 
        // SRV and SamplerState is bound in TextureD3D11
        if (type == ShaderConstantType::SAMPLER_2D ||
            type == ShaderConstantType::SAMPLER_3D ||
            type == ShaderConstantType::SAMPLER_CUBE)
        {
            return true; 
        }

        const ShaderUniform* uniform = getUniform(name);

        if (uniform)
        {
            const ShaderConstantTypeInfo& typeInfo = getShaderConstantTypeInfo(type);

            D3D_SHADER_VARIABLE_CLASS variableClass = typeInfo.variableClass;
            D3D_SHADER_VARIABLE_TYPE variableType = typeInfo.variableType;

            if (uniform->variableClass == variableClass && uniform->variableType == variableType)
            {
                // Note: This size maybe be different than uniform size if not 
                // all values are not updated in case of an array
                size_t writeBytes = typeInfo.paddedSizeInBytes * numValues;

                if (mConstantCacheSize >= (uniform->offset + writeBytes))
                {                    
                    if (typeInfo.paddedSizeInBytes == typeInfo.sizeInBytes)
                    {
                        MemoryCopy(&mConstantCache[uniform->offset], values, writeBytes);
                    }
                    else
                    {                        
                        char* dst = (char*)&mConstantCache[uniform->offset];
                        char* src = (char*)values;

                        for (int i = 0; i < numValues; i++)
                        {
                            MemoryCopy(dst, src, typeInfo.sizeInBytes);
                            dst += typeInfo.paddedSizeInBytes;
                            src += typeInfo.sizeInBytes;
                        }
                    }

                    mConstantBufferDirty = true;

                    return true;
                }
            }
        }

        return false;
    }

    void ShaderD3D11::flushUniforms(ID3D11DeviceContext* deviceContext)
    {
        if (mConstantBufferDirty)
        {
            mConstantBufferDirty = false;
            deviceContext->UpdateSubresource(mConstantBuffer, 0, 0, mConstantCache, 0, 0);
        }
    }
    const ShaderD3D11::ShaderUniform* ShaderD3D11::getUniform(HashValue name) const
    {
        for (size_t i = 0; i < mUniforms.getSize(); ++i)
        {
            const ShaderUniform& uniform = mUniforms[i];

            if (uniform.name == name)
            {
                return &uniform;
            }
        }

        return OMAF_NULL;
    }

    ID3DBlob* ShaderD3D11::compileShader(const char_t* shader, const char_t* entry, const char_t* target)
    {
        DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

#ifdef OMAF_DEBUG_BUILD

        // Override shader compilations flags to make debugging easier
        shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
        shaderFlags |= D3DCOMPILE_DEBUG;
        shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

#endif

        ID3DBlob* byteCode = NULL;
        ID3DBlob* blobError = NULL;

        HRESULT result = D3DCompile(shader,
                                    StringByteLength(shader),
                                    NULL,
                                    NULL,
                                    NULL,
                                    entry,
                                    target, // https://msdn.microsoft.com/en-us/library/windows/desktop/jj215820(v=vs.85).aspx
                                    shaderFlags, // https://msdn.microsoft.com/en-us/library/windows/desktop/gg615083(v=vs.85).aspx
                                    NULL,
                                    &byteCode,
                                    &blobError);

        if (FAILED(result))
        {
            if (blobError)
            {
                OMAF_LOG_E("Shader compilation failed: %s", (char_t*)blobError->GetBufferPointer());
            }

            OMAF_DX_RELEASE(blobError, 0);
            OMAF_DX_RELEASE(byteCode, 0);

            return NULL;
        }

        return byteCode;
    }

OMAF_NS_END
