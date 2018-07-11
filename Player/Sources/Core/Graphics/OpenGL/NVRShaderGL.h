
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

#include "NVREssentials.h"

#include "Graphics/NVRConfig.h"
#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRShaderConstantType.h"
#include "Foundation/NVRHashFunctions.h"
#include "Foundation/NVRFixedArray.h"
#include "Graphics/OpenGL/NVRGLError.h"

#if OMAF_ENABLE_GL_DEBUGGING
#include "Foundation/NVRFixedString.h"
#endif

OMAF_NS_BEGIN

// GLSL Versions
// -------------
//
// OpenGL Version   GLSL Version
// 2.0              110
// 2.1              120
// 3.0              130
// 3.1              140
// 3.2              150
// 3.3              330
// 4.0              400
// 4.1              410
// 4.2              420
// 4.3              430
//
// GLSL ES Versions (Android, iOS, WebGL)
//
// OpenGL ES Version    GLSL ES Version
// 2.0                  100
// 3.0                  300

struct ShaderAttributeGL
{
    GLuint handle;
    GLuint type;
    GLuint size;
    HashValue name;
    
#if OMAF_ENABLE_GL_DEBUGGING
    FixedString64 debugName;
#endif
};

struct ShaderUniformGL
{
    GLuint handle;
    GLuint type;
    GLuint size;
    HashValue name;
    
#if OMAF_ENABLE_GL_DEBUGGING
    FixedString64 debugName;
#endif
};

class ShaderGL
{
    public:
        
        ShaderGL();
        ~ShaderGL();
    
        GLuint getHandle() const;
        
        bool_t create(const char_t* vertexShader, const char_t* fragmentShader);
        bool_t create(const char_t* computeShader);
    
        void_t destroy();

        void_t bind();
        void_t unbind();
    
        bool_t setUniform(ShaderConstantType::Enum type, HashValue name, const void_t* values, uint32_t numValues = 1);
    
        bool_t setUniformBool(HashValue name, const int32_t* values, uint32_t numValues);
        bool_t setUniformBool2(HashValue name, const int32_t* values, uint32_t numValues);
        bool_t setUniformBool3(HashValue name, const int32_t* values, uint32_t numValues);
        bool_t setUniformBool4(HashValue name, const int32_t* values, uint32_t numValues);
    
        bool_t setUniformInteger(HashValue name, const int32_t* values, uint32_t numValues);
        bool_t setUniformInteger2(HashValue name, const int32_t* values, uint32_t numValues);
        bool_t setUniformInteger3(HashValue name, const int32_t* values, uint32_t numValues);
        bool_t setUniformInteger4(HashValue name, const int32_t* values, uint32_t numValues);
    
        bool_t setUniformFloat(HashValue name, const float32_t* values, uint32_t numValues);
        bool_t setUniformFloat2(HashValue name, const float32_t* values, uint32_t numValues);
        bool_t setUniformFloat3(HashValue name, const float32_t* values, uint32_t numValues);
        bool_t setUniformFloat4(HashValue name, const float32_t* values, uint32_t numValues);
    
        bool_t setUniformMatrix22(HashValue name, const float32_t* values, uint32_t numValues);
        bool_t setUniformMatrix33(HashValue name, const float32_t* values, uint32_t numValues);
        bool_t setUniformMatrix44(HashValue name, const float32_t* values, uint32_t numValues);

        bool_t setUniformSampler2D(HashValue name, const int32_t* values, uint32_t numValues);
        bool_t setUniformSamplerCube(HashValue name, const int32_t* values, uint32_t numValues);
    
        const ShaderAttributeGL* getAttribute(HashValue name) const;
        const ShaderUniformGL* getUniform(HashValue name) const;
    
    private:
    
        OMAF_NO_ASSIGN(ShaderGL);
        OMAF_NO_COPY(ShaderGL);
    
    private:
    
        typedef FixedArray<ShaderAttributeGL, OMAF_MAX_SHADER_ATTRIBUTES> Attributes;
        typedef FixedArray<ShaderUniformGL, OMAF_MAX_SHADER_UNIFORMS> Uniforms;
    
        void_t queryAttributes(GLuint handle, Attributes& attributes);
        void_t queryUniforms(GLuint handle, Uniforms& uniforms);
    
    private:
        
        GLuint mHandle;
        
        Attributes mAttributes;
        Uniforms mUniforms;
};

OMAF_NS_END
