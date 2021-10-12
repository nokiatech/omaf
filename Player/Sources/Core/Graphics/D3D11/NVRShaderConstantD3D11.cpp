
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
#include "Graphics/D3D11/NVRShaderConstantD3D11.h"

#include "Foundation/NVRHashFunctions.h"
#include "Foundation/NVRLogger.h"
#include "Graphics/D3D11/NVRD3D11Error.h"
#include "Graphics/D3D11/NVRD3D11Utilities.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(ShaderConstantD3D11)

ShaderConstantD3D11::ShaderConstantD3D11()
    : mHandle(0)
    , mType(ShaderConstantType::INVALID)
{
}

ShaderConstantD3D11::~ShaderConstantD3D11()
{
    OMAF_ASSERT(mHandle == 0, "Shader constant is not destroyed");
}

HashValue ShaderConstantD3D11::getHandle() const
{
    return mHandle;
}

bool_t ShaderConstantD3D11::create(const char_t* name, ShaderConstantType::Enum type)
{
    OMAF_ASSERT_NOT_NULL(name);
    OMAF_ASSERT(type != ShaderConstantType::INVALID, "");

    HashFunction<const char_t*> hashFunction;
    mHandle = hashFunction(name);

    mType = type;

    return true;
}

void_t ShaderConstantD3D11::destroy()
{
    mHandle = 0;
    mType = ShaderConstantType::INVALID;
}

ShaderConstantType::Enum ShaderConstantD3D11::getType() const
{
    return mType;
}
OMAF_NS_END
