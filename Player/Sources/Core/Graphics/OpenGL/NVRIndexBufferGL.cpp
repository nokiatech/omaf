
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
#include "Graphics/OpenGL/NVRIndexBufferGL.h"

#include "Graphics/OpenGL/NVRGLCompatibility.h"
#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLExtensions.h"
#include "Graphics/OpenGL/NVRGLUtilities.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(IndexBufferGL);

IndexBufferGL::IndexBufferGL()
    : mHandle(0)
    , mBytes(0)
    , mComputeAccess(ComputeBufferAccess::INVALID)
    , mFormat(IndexBufferFormat::INVALID)
{
}

IndexBufferGL::~IndexBufferGL()
{
    OMAF_ASSERT(mHandle == 0, "Index buffer is not destroyed");
}

GLuint IndexBufferGL::getHandle() const
{
    return mHandle;
}

bool_t
IndexBufferGL::create(BufferAccess::Enum access, IndexBufferFormat::Enum format, const void_t* data, size_t bytes)
{
    OMAF_ASSERT(access != BufferAccess::INVALID, "");

    OMAF_GL_CHECK(glGenBuffers(1, &mHandle));

    if (mHandle != 0)
    {
        mBytes = (GLsizei) bytes;
        mFormat = format;

        GLint currentIndexBuffer = 0;
        OMAF_GL_CHECK(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &currentIndexBuffer));

        OMAF_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mHandle));
        OMAF_GL_CHECK(
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) mBytes, (const GLvoid*) data, getBufferUsageGL(access)));

        OMAF_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint) currentIndexBuffer));

        return true;
    }

    return false;
}

void_t IndexBufferGL::bufferData(size_t offset, const void_t* data, size_t bytes)
{
    OMAF_ASSERT(mHandle != 0, "");

    GLint currentIndexBuffer = 0;
    OMAF_GL_CHECK(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &currentIndexBuffer));

    OMAF_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mHandle));
    OMAF_GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, bytes, data));

    OMAF_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currentIndexBuffer));

    mBytes = (GLsizei)(offset + bytes);
}

void_t IndexBufferGL::destroy()
{
    OMAF_ASSERT(mHandle != 0, "");

    OMAF_GL_CHECK(glDeleteBuffers(1, &mHandle));
    mHandle = 0;

    mBytes = 0;
    mFormat = IndexBufferFormat::INVALID;
}

void_t IndexBufferGL::bind()
{
    OMAF_ASSERT(mHandle != 0, "");

    OMAF_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mHandle));
}

void_t IndexBufferGL::unbind()
{
    OMAF_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void_t IndexBufferGL::bindCompute(uint16_t stage, ComputeBufferAccess::Enum computeAccess)
{
#if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43) || GL_ARB_compute_shader

    OMAF_ASSERT(mHandle != 0, "");

    OMAF_ASSERT(mComputeAccess == computeAccess, "Compute buffer access doesn't match to resource description");

    OMAF_GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, stage, mHandle));

#else

    OMAF_ASSERT(false, "Render backend doesn't support compute shaders");

#endif
}

void_t IndexBufferGL::unbindCompute(uint16_t stage)
{
#if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43) || GL_ARB_compute_shader

    OMAF_GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0));

#else

    OMAF_ASSERT(false, "Render backend doesn't support compute shaders");

#endif
}

GLsizei IndexBufferGL::getNumIndices() const
{
    size_t size = IndexBufferFormat::getSize(mFormat);

    return (GLsizei)(mBytes / size);
}

GLenum IndexBufferGL::getFormat() const
{
    return getIndexBufferFormatGL(mFormat);
}

OMAF_NS_END
