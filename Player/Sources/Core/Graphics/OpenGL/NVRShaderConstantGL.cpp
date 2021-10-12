
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
#include "Graphics/OpenGL/NVRShaderConstantGL.h"

#include "Graphics/OpenGL/NVRGLCompatibility.h"
#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLExtensions.h"
#include "Graphics/OpenGL/NVRGLUtilities.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(ShaderConstantGL);

ShaderConstantGL::ShaderConstantGL()
    : mHandle(0)
    , mType(ShaderConstantType::INVALID)
{
}

ShaderConstantGL::~ShaderConstantGL()
{
    OMAF_ASSERT(mHandle == 0, "Shader constant is not destroyed");
}

HashValue ShaderConstantGL::getHandle() const
{
    return mHandle;
}

bool_t ShaderConstantGL::create(const char_t* name, ShaderConstantType::Enum type)
{
    OMAF_ASSERT_NOT_NULL(name);
    OMAF_ASSERT(type != ShaderConstantType::INVALID, "");

    HashFunction<const char_t*> hashFunction;
    mHandle = hashFunction(name);

    mType = type;

    return true;
}

void_t ShaderConstantGL::destroy()
{
    mHandle = 0;
    mType = ShaderConstantType::INVALID;
}

ShaderConstantType::Enum ShaderConstantGL::getType() const
{
    return mType;
}

OMAF_NS_END
