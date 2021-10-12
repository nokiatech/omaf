
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

#include "NVREssentials.h"

#include "Foundation/NVRHashFunctions.h"
#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRShaderConstantType.h"

OMAF_NS_BEGIN
class ShaderConstantD3D11
{
public:
    ShaderConstantD3D11();
    ~ShaderConstantD3D11();

    HashValue getHandle() const;

    bool_t create(const char_t* name, ShaderConstantType::Enum type);
    void_t destroy();

    ShaderConstantType::Enum getType() const;

private:
    OMAF_NO_ASSIGN(ShaderConstantD3D11);
    OMAF_NO_COPY(ShaderConstantD3D11);

private:
    HashValue mHandle;
    ShaderConstantType::Enum mType;
};
OMAF_NS_END
