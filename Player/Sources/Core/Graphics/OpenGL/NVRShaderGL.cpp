
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
#include "Graphics/OpenGL/NVRShaderGL.h"

#include "Foundation/NVRLogger.h"
#include "Graphics/OpenGL/NVRGLCompatibility.h"
#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLExtensions.h"
#include "Graphics/OpenGL/NVRGLUtilities.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(ShaderGL);

static bool_t compileShader(GLuint* handle, GLenum type, const char_t* shader)
{
    const GLchar* shaderString = (const GLchar*) shader;
    GLint shaderLength = (GLint) StringByteLength(shaderString);

    GLuint shaderHandle = glCreateShader(type);
    OMAF_GL_CHECK_ERRORS();

    OMAF_GL_CHECK(glShaderSource(shaderHandle, 1, &shaderString, &shaderLength));
    OMAF_GL_CHECK(glCompileShader(shaderHandle));

    GLint compileSuccess;
    OMAF_GL_CHECK(glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess));

    if (compileSuccess == GL_FALSE)
    {
        GLchar messages[256];
        OMAF_GL_CHECK(glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]));

        OMAF_LOG_E("Shader failed to compile! (%s)", messages);

        OMAF_GL_CHECK(glDeleteShader(shaderHandle));

        return false;
    }

    *handle = shaderHandle;

    return true;
}

OMAF_INLINE bool_t linkProgram(GLuint handle)
{
    GLint status = GL_FALSE;

    OMAF_GL_CHECK(glLinkProgram(handle));
    OMAF_GL_CHECK(glGetProgramiv(handle, GL_LINK_STATUS, &status));

    if (status == GL_FALSE)
    {
        OMAF_LOG_E("Shader linking failed");

        return false;
    }

    return true;
}

OMAF_INLINE bool_t validateProgram(GLuint handle)
{
    GLint status = GL_FALSE;

    OMAF_GL_CHECK(glValidateProgram(handle));
    OMAF_GL_CHECK(glGetProgramiv(handle, GL_VALIDATE_STATUS, &status));

    if (status == GL_FALSE)
    {
        GLint logLength = 0;
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLength);
        OMAF_GL_CHECK_ERRORS();

        char_t buffer[2048];
        glGetProgramInfoLog(handle, logLength, &logLength, buffer);
        OMAF_GL_CHECK_ERRORS();

        OMAF_LOG_E("Shader validation failed: %s", buffer);

        return false;
    }

    return true;
}


ShaderGL::ShaderGL()
    : mHandle(0)
{
}

ShaderGL::~ShaderGL()
{
    OMAF_ASSERT(mHandle == 0, "Shader is not destroyed");
    OMAF_ASSERT(mAttributes.getSize() == 0, "Shader is not destroyed");
    OMAF_ASSERT(mUniforms.getSize() == 0, "Shader is not destroyed");
}

GLuint ShaderGL::getHandle() const
{
    return mHandle;
}

bool_t ShaderGL::create(const char_t* computeShader)
{
#if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43) || GL_ARB_compute_shader

    mHandle = glCreateProgram();
    OMAF_GL_CHECK_ERRORS();

    if (mHandle != 0)
    {
        // Create program
        GLuint computeShaderHandle = 0;

        bool_t compileResult = true;

        compileResult &= compileShader(&computeShaderHandle, GL_COMPUTE_SHADER, computeShader);

        if (compileResult)
        {
            OMAF_GL_CHECK(glAttachShader(mHandle, computeShaderHandle));

            bool_t linkResult = linkProgram(mHandle);

            if (linkResult)
            {
                queryAttributes(mHandle, mAttributes);
                queryUniforms(mHandle, mUniforms);

                // Destroy individual shaders
                OMAF_GL_CHECK(glDeleteShader(computeShaderHandle));
                computeShaderHandle = 0;

                return true;
            }
        }

        // Destroy individual shaders
        if (computeShaderHandle != 0)
        {
            OMAF_GL_CHECK(glDeleteShader(computeShaderHandle));
            computeShaderHandle = 0;
        }

        return false;
    }

#endif

    return false;
}

bool_t ShaderGL::create(const char_t* vertexShader, const char_t* fragmentShader)
{
    mHandle = glCreateProgram();
    OMAF_GL_CHECK_ERRORS();

    if (mHandle != 0)
    {
        // Create program
        GLuint vertexShaderHandle = 0;
        GLuint fragmentShaderHandle = 0;

        bool_t compileResult = true;

        compileResult &= compileShader(&vertexShaderHandle, GL_VERTEX_SHADER, vertexShader);
        compileResult &= compileShader(&fragmentShaderHandle, GL_FRAGMENT_SHADER, fragmentShader);

        if (compileResult)
        {
            OMAF_GL_CHECK(glAttachShader(mHandle, vertexShaderHandle));
            OMAF_GL_CHECK(glAttachShader(mHandle, fragmentShaderHandle));

            bool_t linkResult = linkProgram(mHandle);

            if (linkResult)
            {
                queryAttributes(mHandle, mAttributes);
                queryUniforms(mHandle, mUniforms);

                // Destroy individual shaders
                OMAF_GL_CHECK(glDeleteShader(vertexShaderHandle));
                vertexShaderHandle = 0;

                OMAF_GL_CHECK(glDeleteShader(fragmentShaderHandle));
                fragmentShaderHandle = 0;

                return true;
            }
        }

        // Destroy individual shaders
        if (vertexShaderHandle != 0)
        {
            OMAF_GL_CHECK(glDeleteShader(vertexShaderHandle));
            vertexShaderHandle = 0;
        }

        if (fragmentShaderHandle != 0)
        {
            OMAF_GL_CHECK(glDeleteShader(fragmentShaderHandle));
            fragmentShaderHandle = 0;
        }

        return false;
    }

    return false;
}

void_t ShaderGL::bind()
{
    OMAF_ASSERT(mHandle != 0, "");

    OMAF_GL_CHECK(glUseProgram(mHandle));
}

void_t ShaderGL::unbind()
{
    OMAF_GL_CHECK(glUseProgram(0));
}

void_t ShaderGL::destroy()
{
    OMAF_ASSERT(mHandle != 0, "");

    OMAF_GL_CHECK(glDeleteProgram(mHandle));
    mHandle = 0;

    mAttributes.clear();
    mUniforms.clear();
}

bool_t ShaderGL::setUniform(ShaderConstantType::Enum type, HashValue name, const void_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        switch (type)
        {
        case ShaderConstantType::BOOL:
            return setUniformBool(name, (int32_t*) values, numValues);
        case ShaderConstantType::BOOL2:
            return setUniformBool2(name, (int32_t*) values, numValues);
        case ShaderConstantType::BOOL3:
            return setUniformBool3(name, (int32_t*) values, numValues);
        case ShaderConstantType::BOOL4:
            return setUniformBool4(name, (int32_t*) values, numValues);

        case ShaderConstantType::INTEGER:
            return setUniformInteger(name, (int32_t*) values, numValues);
        case ShaderConstantType::INTEGER2:
            return setUniformInteger2(name, (int32_t*) values, numValues);
        case ShaderConstantType::INTEGER3:
            return setUniformInteger3(name, (int32_t*) values, numValues);
        case ShaderConstantType::INTEGER4:
            return setUniformInteger4(name, (int32_t*) values, numValues);

        case ShaderConstantType::FLOAT:
            return setUniformFloat(name, (float32_t*) values, numValues);
        case ShaderConstantType::FLOAT2:
            return setUniformFloat2(name, (float32_t*) values, numValues);
        case ShaderConstantType::FLOAT3:
            return setUniformFloat3(name, (float32_t*) values, numValues);
        case ShaderConstantType::FLOAT4:
            return setUniformFloat4(name, (float32_t*) values, numValues);

        case ShaderConstantType::MATRIX22:
            return setUniformMatrix22(name, (float32_t*) values, numValues);
        case ShaderConstantType::MATRIX33:
            return setUniformMatrix33(name, (float32_t*) values, numValues);
        case ShaderConstantType::MATRIX44:
            return setUniformMatrix44(name, (float32_t*) values, numValues);

        case ShaderConstantType::SAMPLER_2D:
            return setUniformSampler2D(name, (int32_t*) values, numValues);
            // case ShaderConstantType::SAMPLER_3D:
        case ShaderConstantType::SAMPLER_CUBE:
            return setUniformSamplerCube(name, (int32_t*) values, numValues);

        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
        }
    }

    return false;
}

bool_t ShaderGL::setUniformBool(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_BOOL, "");

        OMAF_GL_CHECK(glUniform1iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformBool2(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_BOOL_VEC2, "");

        OMAF_GL_CHECK(glUniform2iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformBool3(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_BOOL_VEC3, "");

        OMAF_GL_CHECK(glUniform3iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformBool4(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_BOOL_VEC4, "");

        OMAF_GL_CHECK(glUniform4iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformFloat(HashValue name, const float32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_FLOAT, "");

        OMAF_GL_CHECK(glUniform1fv((GLint) uniform->handle, (GLsizei) numValues, (const GLfloat*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformFloat2(HashValue name, const float32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_FLOAT_VEC2, "");

        OMAF_GL_CHECK(glUniform2fv((GLint) uniform->handle, (GLsizei) numValues, (const GLfloat*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformFloat3(HashValue name, const float32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_FLOAT_VEC3, "");

        OMAF_GL_CHECK(glUniform3fv((GLint) uniform->handle, (GLsizei) numValues, (const GLfloat*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformFloat4(HashValue name, const float32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_FLOAT_VEC4, "");

        OMAF_GL_CHECK(glUniform4fv((GLint) uniform->handle, (GLsizei) numValues, (const GLfloat*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformInteger(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_INT, "");

        OMAF_GL_CHECK(glUniform1iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformInteger2(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_INT_VEC2, "");

        OMAF_GL_CHECK(glUniform2iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformInteger3(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_INT_VEC3, "");

        OMAF_GL_CHECK(glUniform3iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformInteger4(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_INT_VEC4, "");

        OMAF_GL_CHECK(glUniform4iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformMatrix22(HashValue name, const float32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_FLOAT_MAT2, "");

        OMAF_GL_CHECK(
            glUniformMatrix2fv((GLint) uniform->handle, (GLsizei) numValues, GL_FALSE, (const GLfloat*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformMatrix33(HashValue name, const float32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_FLOAT_MAT3, "");

        OMAF_GL_CHECK(
            glUniformMatrix3fv((GLint) uniform->handle, (GLsizei) numValues, GL_FALSE, (const GLfloat*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformMatrix44(HashValue name, const float32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_FLOAT_MAT4, "");

        OMAF_GL_CHECK(
            glUniformMatrix4fv((GLint) uniform->handle, (GLsizei) numValues, GL_FALSE, (const GLfloat*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformSampler2D(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
#if OMAF_GRAPHICS_API_OPENGL_ES

        OMAF_ASSERT((uniform->type == GL_SAMPLER_2D || uniform->type == GL_SAMPLER_EXTERNAL_OES), "");

#else

        OMAF_ASSERT((uniform->type == GL_SAMPLER_2D || uniform->type == GL_SAMPLER_2D_RECT), "");

#endif

        OMAF_GL_CHECK(glUniform1iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

bool_t ShaderGL::setUniformSamplerCube(HashValue name, const int32_t* values, uint32_t numValues)
{
    const ShaderUniformGL* uniform = getUniform(name);

    if (uniform && uniform->size >= numValues)
    {
        OMAF_ASSERT(uniform->type == GL_SAMPLER_CUBE, "");

        OMAF_GL_CHECK(glUniform1iv((GLint) uniform->handle, (GLsizei) numValues, (const GLint*) values));

        return true;
    }

    return false;
}

const ShaderAttributeGL* ShaderGL::getAttribute(HashValue name) const
{
    for (size_t i = 0; i < mAttributes.getSize(); ++i)
    {
        const ShaderAttributeGL& attribute = mAttributes[i];

        if (attribute.name == name)
        {
            return &attribute;
        }
    }

    return OMAF_NULL;
}

const ShaderUniformGL* ShaderGL::getUniform(HashValue name) const
{
    for (size_t i = 0; i < mUniforms.getSize(); ++i)
    {
        const ShaderUniformGL& uniform = mUniforms[i];

        if (uniform.name == name)
        {
            return &uniform;
        }
    }

    return OMAF_NULL;
}

void_t ShaderGL::queryAttributes(GLuint handle, Attributes& attributes)
{
    GLint activeAttributes = 0;
    OMAF_GL_CHECK(glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTES, &activeAttributes));

    for (GLint i = 0; i < activeAttributes; ++i)
    {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;

        const GLsizei nameBufferSize = 64;
        GLchar nameBuffer[nameBufferSize];
        MemorySet(nameBuffer, 0, OMAF_SIZE_OF(nameBuffer));

        OMAF_GL_CHECK(glGetActiveAttrib(handle, i, nameBufferSize, &length, &size, &type, nameBuffer));

        // Skip internal attributes such as gl_VertexID, gl_InstanceID etc.
        if (nameBuffer[0] == 'g' && nameBuffer[1] == 'l')
        {
            continue;
        }

        GLint location = glGetAttribLocation(handle, nameBuffer);
        OMAF_GL_CHECK_ERRORS();
        OMAF_ASSERT(location != -1, "");

        HashFunction<const char_t*> hashFunction;

        ShaderAttributeGL attribute;
        attribute.handle = location;
        attribute.type = type;
        attribute.size = size;
        attribute.name = hashFunction(nameBuffer);

#if OMAF_ENABLE_GL_DEBUGGING

        attribute.debugName = nameBuffer;

#endif

        attributes.add(attribute);
    }
}

void_t ShaderGL::queryUniforms(GLuint handle, Uniforms& uniforms)
{
    GLint activeUniforms = 0;
    OMAF_GL_CHECK(glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &activeUniforms));

    for (GLint i = 0; i < activeUniforms; ++i)
    {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;

        const GLsizei nameBufferSize = 64;
        GLchar nameBuffer[nameBufferSize];
        MemorySet(nameBuffer, 0, OMAF_SIZE_OF(nameBuffer));

        OMAF_GL_CHECK(glGetActiveUniform(handle, i, nameBufferSize, &length, &size, &type, nameBuffer));

        GLint location = glGetUniformLocation(handle, nameBuffer);
        OMAF_GL_CHECK_ERRORS();
        OMAF_ASSERT(location != -1, "");

        char_t* array = strchr(nameBuffer, '[');

        if (array)
        {
            *array = '\0';
        }

        HashFunction<const char_t*> hashFunction;

        ShaderUniformGL uniform;
        uniform.handle = location;
        uniform.type = type;
        uniform.size = size;
        uniform.name = hashFunction(nameBuffer);

#if OMAF_ENABLE_GL_DEBUGGING

        uniform.debugName = nameBuffer;

#endif

        uniforms.add(uniform);
    }
}

OMAF_NS_END
