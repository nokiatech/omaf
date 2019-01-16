
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
#include "Graphics/OpenGL/NVRGLUtilities.h"

#include "Graphics/OpenGL/NVRGLExtensions.h"
#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLCompatibility.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(GLUtilities);

GLenum getFillModeGL(FillMode::Enum fillMode)
{
    switch (fillMode)
    {
        case FillMode::SOLID:       return GL_FILL;
        case FillMode::WIREFRAME:   return GL_LINE;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getCullModeGL(CullMode::Enum cullMode)
{
    switch (cullMode)
    {
        case CullMode::NONE:    return GL_NONE;
        case CullMode::FRONT:   return GL_FRONT;
        case CullMode::BACK:    return GL_BACK;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getFrontFaceGL(FrontFace::Enum frontFace)
{
    switch (frontFace)
    {
        case FrontFace::CW:    return GL_CW;
        case FrontFace::CCW:   return GL_CCW;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getBlendFunctionGL(BlendFunction::Enum blendFunction)
{
    switch (blendFunction)
    {
        case BlendFunction::ZERO:                   return GL_ZERO;
        case BlendFunction::ONE:                    return GL_ONE;
        case BlendFunction::SRC_COLOR:              return GL_SRC_COLOR;
        case BlendFunction::ONE_MINUS_SRC_COLOR:    return GL_ONE_MINUS_SRC_COLOR;
        case BlendFunction::DST_COLOR:              return GL_DST_COLOR;
        case BlendFunction::ONE_MINUS_DST_COLOR:    return GL_ONE_MINUS_DST_COLOR;
        case BlendFunction::SRC_ALPHA:              return GL_SRC_ALPHA;
        case BlendFunction::ONE_MINUS_SRC_ALPHA:    return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFunction::DST_ALPHA:              return GL_DST_ALPHA;
        case BlendFunction::ONE_MINUS_DST_ALPHA:    return GL_ONE_MINUS_DST_ALPHA;
        case BlendFunction::SRC_ALPHA_SATURATE:     return GL_SRC_ALPHA_SATURATE;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getBlendEquationGL(BlendEquation::Enum blendEquation)
{
    switch (blendEquation)
    {
        case BlendEquation::ADD:                return GL_FUNC_ADD;
        case BlendEquation::SUBTRACT:           return GL_FUNC_SUBTRACT;
        case BlendEquation::REVERSE_SUBTRACT:   return GL_FUNC_REVERSE_SUBTRACT;
        case BlendEquation::MINIMUM:            return GL_MIN;
        case BlendEquation::MAXIMUM:            return GL_MAX;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getBufferUsageGL(BufferAccess::Enum access)
{
    switch (access)
    {
        case BufferAccess::STATIC:  return GL_STATIC_DRAW;
        case BufferAccess::DYNAMIC: return GL_DYNAMIC_DRAW;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getComputeBufferAccessGL(ComputeBufferAccess::Enum access)
{
    switch (access)
    {
#if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43) || GL_ARB_compute_shader
            
        case ComputeBufferAccess::READ:         return GL_READ_ONLY;
        case ComputeBufferAccess::WRITE:        return GL_WRITE_ONLY;
        case ComputeBufferAccess::READ_WRITE:   return GL_READ_WRITE;

#endif
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }

    return (GLenum)-1;
}

GLboolean getDepthMaskGL(DepthMask::Enum depthMask)
{
    switch (depthMask)
    {
        case DepthMask::ZERO: return GL_FALSE;
        case DepthMask::ALL:  return GL_TRUE;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLboolean)-1;
}

GLenum getDepthFunctionGL(DepthFunction::Enum depthFunction)
{
    switch (depthFunction)
    {
        case DepthFunction::NEVER:            return GL_NEVER;
        case DepthFunction::ALWAYS:           return GL_ALWAYS;
        case DepthFunction::EQUAL:            return GL_EQUAL;
        case DepthFunction::NOT_EQUAL:        return GL_NOTEQUAL;
        case DepthFunction::LESS:             return GL_LESS;
        case DepthFunction::LESS_EQUAL:       return GL_LEQUAL;
        case DepthFunction::GREATER:          return GL_GREATER;
        case DepthFunction::GREATER_EQUAL:    return GL_GEQUAL;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getStencilOperationGL(StencilOperation::Enum stencilOperation)
{
    switch (stencilOperation)
    {
        case StencilOperation::KEEP:            return GL_KEEP;
        case StencilOperation::ZERO:            return GL_ZERO;
        case StencilOperation::REPLACE:         return GL_REPLACE;
        case StencilOperation::INCREMENT:       return GL_INCR;
        case StencilOperation::INCREMENT_WRAP:  return GL_INCR_WRAP;
        case StencilOperation::DECREMENT:       return GL_DECR;
        case StencilOperation::DECREMENT_WRAP:  return GL_DECR_WRAP;
        case StencilOperation::INVERT:          return GL_INVERT;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getStencilFunctionGL(StencilFunction::Enum stencilFunction)
{
    switch (stencilFunction)
    {
        case StencilFunction::NEVER:            return GL_NEVER;
        case StencilFunction::ALWAYS:           return GL_ALWAYS;
        case StencilFunction::EQUAL:            return GL_EQUAL;
        case StencilFunction::NOT_EQUAL:        return GL_NOTEQUAL;
        case StencilFunction::LESS:             return GL_LESS;
        case StencilFunction::LESS_EQUAL:       return GL_LEQUAL;
        case StencilFunction::GREATER:          return GL_GREATER;
        case StencilFunction::GREATER_EQUAL:    return GL_GEQUAL;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getPrimitiveTypeGL(PrimitiveType::Enum primitiveType)
{
    switch (primitiveType)
    {
        case PrimitiveType::POINT_LIST:     return GL_POINTS;
        case PrimitiveType::LINE_LIST:      return GL_LINES;
        case PrimitiveType::LINE_STRIP:     return GL_LINE_STRIP;
        case PrimitiveType::TRIANGLE_LIST:  return GL_TRIANGLES;
        case PrimitiveType::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;

        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getTextureTypeGL(TextureType::Enum textureType)
{
    switch (textureType)
    {
        case TextureType::TEXTURE_2D:   return GL_TEXTURE_2D;
        case TextureType::TEXTURE_3D:   return GL_TEXTURE_3D;
        case TextureType::TEXTURE_CUBE: return GL_TEXTURE_CUBE_MAP;
            
#if OMAF_PLATFORM_ANDROID
            
        case TextureType::TEXTURE_VIDEO_SURFACE: return GL_TEXTURE_EXTERNAL_OES;
     
#else
            
        case TextureType::TEXTURE_VIDEO_SURFACE: return GL_TEXTURE_2D;
            
#endif
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getTextureAddressModeGL(TextureAddressMode::Enum textureAddressMode)
{
    switch (textureAddressMode)
    {
        case TextureAddressMode::REPEAT:    return GL_REPEAT;
        case TextureAddressMode::MIRROR:    return GL_MIRRORED_REPEAT;
        case TextureAddressMode::CLAMP:     return GL_CLAMP_TO_EDGE;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getTextureMinFilterModeGL(TextureFilterMode::Enum textureFilterMode, bool_t useMipMaps)
{
    switch (textureFilterMode)
    {
        case TextureFilterMode::POINT:
        {
            return GL_NEAREST;
        }
            
        case TextureFilterMode::BILINEAR:
        {
            if (useMipMaps)
            {
                return GL_LINEAR_MIPMAP_NEAREST;
            }
            else
            {
                return GL_LINEAR;
            }
        }
            
        case TextureFilterMode::TRILINEAR:
        {
            return GL_LINEAR_MIPMAP_LINEAR;
        }
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getTextureMagFilterModeGL(TextureFilterMode::Enum textureFilterMode, bool_t useMipMaps)
{
    OMAF_UNUSED_VARIABLE(useMipMaps);

    switch (textureFilterMode)
    {
        case TextureFilterMode::POINT:          return GL_NEAREST;
        case TextureFilterMode::BILINEAR:       return GL_LINEAR;
        case TextureFilterMode::TRILINEAR:      return GL_LINEAR;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getShaderUniformTypeGL(ShaderConstantType::Enum shaderConstantType)
{
    switch (shaderConstantType)
    {
        case ShaderConstantType::BOOL:          return GL_BOOL;
        case ShaderConstantType::BOOL2:         return GL_BOOL_VEC2;
        case ShaderConstantType::BOOL3:         return GL_BOOL_VEC3;
        case ShaderConstantType::BOOL4:         return GL_BOOL_VEC4;
            
        case ShaderConstantType::INTEGER:       return GL_INT;
        case ShaderConstantType::INTEGER2:      return GL_INT_VEC2;
        case ShaderConstantType::INTEGER3:      return GL_INT_VEC3;
        case ShaderConstantType::INTEGER4:      return GL_INT_VEC4;
            
        case ShaderConstantType::FLOAT:         return GL_FLOAT;
        case ShaderConstantType::FLOAT2:        return GL_FLOAT_VEC2;
        case ShaderConstantType::FLOAT3:        return GL_FLOAT_VEC3;
        case ShaderConstantType::FLOAT4:        return GL_FLOAT_VEC4;
            
        case ShaderConstantType::MATRIX22:      return GL_FLOAT_MAT2;
        case ShaderConstantType::MATRIX33:      return GL_FLOAT_MAT3;
        case ShaderConstantType::MATRIX44:      return GL_FLOAT_MAT4;
            
        case ShaderConstantType::SAMPLER_2D:    return GL_SAMPLER_2D;
        case ShaderConstantType::SAMPLER_3D:    return GL_SAMPLER_3D;
        case ShaderConstantType::SAMPLER_CUBE:  return GL_SAMPLER_CUBE;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getVertexAttributeFormatGL(VertexAttributeFormat::Enum vertexAttributeFormat)
{
    switch (vertexAttributeFormat)
    {
        case VertexAttributeFormat::INT8:       return GL_BYTE;
        case VertexAttributeFormat::UINT8:      return GL_UNSIGNED_BYTE;
            
        case VertexAttributeFormat::INT16:      return GL_SHORT;
        case VertexAttributeFormat::UINT16:     return GL_UNSIGNED_SHORT;
            
        case VertexAttributeFormat::INT32:      return GL_INT;
        case VertexAttributeFormat::UINT32:     return GL_UNSIGNED_INT;
            
        case VertexAttributeFormat::FLOAT16:    return GL_HALF_FLOAT;
        case VertexAttributeFormat::FLOAT32:    return GL_FLOAT;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

GLenum getIndexBufferFormatGL(IndexBufferFormat::Enum vertexAttributeFormat)
{
    switch (vertexAttributeFormat)
    {
        case IndexBufferFormat::UINT16: return GL_UNSIGNED_SHORT;
        case IndexBufferFormat::UINT32: return GL_UNSIGNED_INT;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return (GLenum)-1;
}

const TextureFormatGL& getTextureFormatGL(TextureFormat::Enum format)
{
    // https://www.khronos.org/opengles/sdk/docs/man3/html/glTexImage2D.xhtml
    // https://www.opengl.org/sdk/docs/man/html/glTexImage2D.xhtml
    
    static TextureFormatGL textureFormats[TextureFormat::COUNT] =
    {
        /* BC1 */           { GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_COMPRESSED_RGB_S3TC_DXT1_EXT,                GL_ZERO },
        /* BC1A1 */         { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,               GL_ZERO },
        /* BC2 */           { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,             GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,               GL_ZERO },
        /* BC3 */           { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,             GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,               GL_ZERO },
        
        /* BC4 */           { GL_COMPRESSED_LUMINANCE_LATC1_EXT,            GL_COMPRESSED_LUMINANCE_LATC1_EXT,              GL_ZERO },
        /* BC5 */           { GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,      GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,        GL_ZERO },
        
        /* ETC1 */          { GL_ETC1_RGB8_OES,                             GL_ETC1_RGB8_OES,                               GL_ZERO },
        
        /* ETC2 */          { GL_COMPRESSED_RGB8_ETC2,                      GL_COMPRESSED_RGB8_ETC2,                        GL_ZERO },
        /* ETC2EAC */       { GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_COMPRESSED_RGBA8_ETC2_EAC,                   GL_ZERO },
        /* ETC2A1 */        { GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,    GL_ZERO },
        
        /* PVRTCIRGB2 */    { GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,           GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,             GL_ZERO },
        /* PVRTCIRGBA2 */   { GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,           GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,             GL_ZERO },
        /* PVRTCIRGB4 */    { GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,          GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,            GL_ZERO },
        /* PVRTCIRGBA4 */   { GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,          GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,            GL_ZERO },
        
        /* PVRTCIIRGBA2 */  { GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG,          GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG,            GL_ZERO },
        /* PVRTCIIRGBA4 */  { GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG,          GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG,            GL_ZERO },
        
        /* A8 */            { GL_ALPHA,                                     GL_ALPHA,                                       GL_UNSIGNED_BYTE },
        
        /* R8 */            { GL_R8,                                        GL_RED,                                         GL_UNSIGNED_BYTE },
        /* R8I */           { GL_R8I,                                       GL_RED_INTEGER,                                 GL_BYTE },
        /* R8U */           { GL_R8UI,                                      GL_RED_INTEGER,                                 GL_UNSIGNED_BYTE },
        /* R8S */           { GL_R8_SNORM,                                  GL_RED,                                         GL_BYTE },

        /* R16 */           { GL_R16,                                       GL_RED,                                         GL_UNSIGNED_SHORT },
        /* R16I */          { GL_R16I,                                      GL_RED_INTEGER,                                 GL_SHORT },
        /* R16U */          { GL_R16UI,                                     GL_RED_INTEGER,                                 GL_UNSIGNED_SHORT },
        /* R16F */          { GL_R16F,                                      GL_RED,                                         GL_FLOAT },
        /* R16S */          { GL_R16_SNORM,                                 GL_RED,                                         GL_SHORT },
        
        /* R32I */          { GL_R32I,                                      GL_RED_INTEGER,                                 GL_INT },
        /* R32U */          { GL_R32UI,                                     GL_RED_INTEGER,                                 GL_UNSIGNED_INT },
        /* R32F */          { GL_R32F,                                      GL_RED,                                         GL_FLOAT },
        
        /* RG8 */           { GL_RG8,                                       GL_RG,                                          GL_UNSIGNED_BYTE },
        /* RG8I */          { GL_RG8I,                                      GL_RG_INTEGER,                                  GL_BYTE },
        /* RG8U */          { GL_RG8UI,                                     GL_RG_INTEGER,                                  GL_UNSIGNED_BYTE },
        /* RG8S */          { GL_RG8_SNORM,                                 GL_RG,                                          GL_BYTE },
        
        /* RG16 */          { GL_RG16,                                      GL_RG,                                          GL_UNSIGNED_SHORT },
        /* RG16I */         { GL_RG16I,                                     GL_RG_INTEGER,                                  GL_SHORT },
        /* RG16U */         { GL_RG16UI,                                    GL_RG_INTEGER,                                  GL_UNSIGNED_SHORT },
        /* RG16F */         { GL_RG16F,                                     GL_RG,                                          GL_FLOAT },
        /* RG16S */         { GL_RG16_SNORM,                                GL_RG,                                          GL_SHORT },
        
        /* RG32I */         { GL_RG32I,                                     GL_RG_INTEGER,                                  GL_INT },
        /* RG32U */         { GL_RG32UI,                                    GL_RG_INTEGER,                                  GL_UNSIGNED_INT },
        /* RG32F */         { GL_RG32F,                                     GL_RG,                                          GL_FLOAT },
        
        /* RGB8 */          { GL_RGB8,                                      GL_RGB,                                         GL_UNSIGNED_BYTE },
        /* RGB9E5F */       { GL_RGB9_E5,                                   GL_RGB,                                         GL_FLOAT },
        
        /* BGRA8 */         { GL_BGRA8,                                     GL_BGRA,                                        GL_UNSIGNED_BYTE },
        
        /* RGBA8 */         { GL_RGBA8,                                     GL_RGBA,                                        GL_UNSIGNED_BYTE },
        /* RGBA8I */        { GL_RGBA8I,                                    GL_RGBA_INTEGER,                                GL_BYTE },
        /* RGBA8U */        { GL_RGBA8UI,                                   GL_RGBA_INTEGER,                                GL_UNSIGNED_BYTE },
        /* RGBA8S */        { GL_RGBA8_SNORM,                               GL_RGBA,                                        GL_BYTE },
        
        /* RGBA16 */        { GL_RGBA16,                                    GL_RGBA,                                        GL_UNSIGNED_SHORT },
        /* RGBA16I */       { GL_RGBA16I,                                   GL_RGBA_INTEGER,                                GL_SHORT },
        /* RGBA16U */       { GL_RGBA16UI,                                  GL_RGBA_INTEGER,                                GL_UNSIGNED_SHORT },
        /* RGBA16F */       { GL_RGBA16F,                                   GL_RGBA,                                        GL_FLOAT },
        /* RGBA16S */       { GL_RGBA16_SNORM,                              GL_RGBA,                                        GL_SHORT },
        
        /* RGBA32I */       { GL_RGBA32I,                                   GL_RGBA_INTEGER,                                GL_INT },
        /* RGBA32U */       { GL_RGBA32UI,                                  GL_RGBA_INTEGER,                                GL_UNSIGNED_INT },
        /* RGBA32F */       { GL_RGBA32F,                                   GL_RGBA,                                        GL_FLOAT },
        
        /* R5G6B5 */        { GL_RGB565,                                    GL_RGB,                                         GL_UNSIGNED_SHORT_5_6_5 },
        /* RGBA4 */         { GL_RGBA4,                                     GL_RGBA,                                        GL_UNSIGNED_SHORT_4_4_4_4 },
        /* RGB5A1 */        { GL_RGB5_A1,                                   GL_RGBA,                                        GL_UNSIGNED_SHORT_5_5_5_1 },
        
        /* RGB10A2 */       { GL_RGB10_A2,                                  GL_RGBA,                                        GL_UNSIGNED_INT_2_10_10_10_REV },
        /* R11G11B10F */    { GL_R11F_G11F_B10F,                            GL_RGB,                                         GL_UNSIGNED_INT_10F_11F_11F_REV },
        
        /* D16 */           { GL_DEPTH_COMPONENT16,                         GL_DEPTH_COMPONENT,                             GL_UNSIGNED_SHORT },
        /* D24 */           { GL_DEPTH_COMPONENT24,                         GL_DEPTH_COMPONENT,                             GL_UNSIGNED_INT },
        /* D32 */           { GL_DEPTH_COMPONENT32,                         GL_DEPTH_COMPONENT,                             GL_UNSIGNED_INT },
        
        /* D16F */          { 0, /*GL_DEPTH_COMPONENT32F,*/                 GL_DEPTH_COMPONENT,                             GL_FLOAT },
        /* D24F */          { 0, /*GL_DEPTH_COMPONENT32F,*/                 GL_DEPTH_COMPONENT,                             GL_FLOAT },
        /* D32F */          { GL_DEPTH_COMPONENT32F,                        GL_DEPTH_COMPONENT,                             GL_FLOAT },

        /* D24S8 */         { GL_DEPTH24_STENCIL8,                          GL_DEPTH_STENCIL,                               GL_UNSIGNED_INT_24_8 },
        /* D32FS8X24 */     { GL_DEPTH32F_STENCIL8,                         GL_DEPTH_STENCIL,                               GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
        
        /* S8 */            { GL_STENCIL_INDEX8,                            GL_STENCIL_INDEX,                               GL_UNSIGNED_BYTE },
        /* S16 */           { GL_STENCIL_INDEX16,                           GL_STENCIL_INDEX,                               GL_UNSIGNED_BYTE },
    };
    
    OMAF_COMPILE_ASSERT(OMAF_ARRAY_SIZE(textureFormats) == TextureFormat::COUNT);
    
    return textureFormats[format];
}

TextureFormat::Enum getTextureFormat(GLenum format)
{
    switch (format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:               return TextureFormat::BC1;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:              return TextureFormat::BC1A1;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:              return TextureFormat::BC2;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:              return TextureFormat::BC3;
        
        case GL_COMPRESSED_LUMINANCE_LATC1_EXT:             return TextureFormat::BC4;
        case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:       return TextureFormat::BC5;
        
        case GL_ETC1_RGB8_OES:                              return TextureFormat::ETC1;
        
        case GL_COMPRESSED_RGB8_ETC2:                       return TextureFormat::ETC2;
        case GL_COMPRESSED_RGBA8_ETC2_EAC:                  return TextureFormat::ETC2EAC;
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:   return TextureFormat::ETC2A1;
        
        case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:            return TextureFormat::PVRTCIRGB2;
        case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:            return TextureFormat::PVRTCIRGBA2;
        case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:           return TextureFormat::PVRTCIRGB4;
        case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:           return TextureFormat::PVRTCIRGBA4;
        
        case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG:           return TextureFormat::PVRTCIIRGBA2;
        case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG:           return TextureFormat::PVRTCIIRGBA4;
        
        case GL_ALPHA:                                      return TextureFormat::A8;
        
        case GL_R8:                                         return TextureFormat::R8;
        case GL_R8I:                                        return TextureFormat::R8I;
        case GL_R8UI:                                       return TextureFormat::R8U;
        case GL_R8_SNORM:                                   return TextureFormat::R8S;
        
        case GL_R16:                                        return TextureFormat::R16;
        case GL_R16I:                                       return TextureFormat::R16I;
        case GL_R16UI:                                      return TextureFormat::R16U;
        case GL_R16F:                                       return TextureFormat::R16F;
        case GL_R16_SNORM:                                  return TextureFormat::R16S;
        
        case GL_R32I:                                       return TextureFormat::R32I;
        case GL_R32UI:                                      return TextureFormat::R32U;
        case GL_R32F:                                       return TextureFormat::R32F;
        
        case GL_RG8:                                        return TextureFormat::RG8;
        case GL_RG8I:                                       return TextureFormat::RG8I;
        case GL_RG8UI:                                      return TextureFormat::RG8U;
        case GL_RG8_SNORM:                                  return TextureFormat::RG8S;
        
        case GL_RG16:                                       return TextureFormat::RG16;
        case GL_RG16I:                                      return TextureFormat::RG16I;
        case GL_RG16UI:                                     return TextureFormat::RG16U;
        case GL_RG16F:                                      return TextureFormat::RG16F;
        case GL_RG16_SNORM:                                 return TextureFormat::RG16S;
        
        case GL_RG32I:                                      return TextureFormat::RG32I;
        case GL_RG32UI:                                     return TextureFormat::RG32U;
        case GL_RG32F:                                      return TextureFormat::RG32F;
        
        case GL_RGB8:                                       return TextureFormat::RGB8;
        case GL_RGB9_E5:                                    return TextureFormat::RGB9E5F;
        
        case GL_BGRA8:                                      return TextureFormat::BGRA8;
        
        case GL_RGBA8:                                      return TextureFormat::RGBA8;
        case GL_RGBA8I:                                     return TextureFormat::RGBA8I;
        case GL_RGBA8UI:                                    return TextureFormat::RGBA8U;
        case GL_RGBA8_SNORM:                                return TextureFormat::RGBA8S;
        
        case GL_RGBA16:                                     return TextureFormat::RGBA16;
        case GL_RGBA16I:                                    return TextureFormat::RGBA16I;
        case GL_RGBA16UI:                                   return TextureFormat::RGBA16U;
        case GL_RGBA16F:                                    return TextureFormat::RGBA16F;
        case GL_RGBA16_SNORM:                               return TextureFormat::RGBA16S;
        
        case GL_RGBA32I:                                    return TextureFormat::RGBA32I;
        case GL_RGBA32UI:                                   return TextureFormat::RGBA32U;
        case GL_RGBA32F:                                    return TextureFormat::RGBA32F;
        
        case GL_RGB565:                                     return TextureFormat::R5G6B5;
        case GL_RGBA4:                                      return TextureFormat::RGBA4;
        case GL_RGB5_A1:                                    return TextureFormat::RGB5A1;
        
        case GL_RGB10_A2:                                   return TextureFormat::RGB10A2;
        case GL_R11F_G11F_B10F:                             return TextureFormat::R11G11B10F;
        
        case GL_DEPTH_COMPONENT16:                          return TextureFormat::D16;
        case GL_DEPTH_COMPONENT24:                          return TextureFormat::D24;
        case GL_DEPTH_COMPONENT32:                          return TextureFormat::D32;
        
        case GL_DEPTH_COMPONENT32F:                         return TextureFormat::D32F;
        
        case GL_DEPTH24_STENCIL8:                           return TextureFormat::D24S8;
        case GL_DEPTH32F_STENCIL8:                          return TextureFormat::D32FS8X24;
        
        case GL_STENCIL_INDEX8:                             return TextureFormat::S8;
        case GL_STENCIL_INDEX16:                            return TextureFormat::S16;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    };
    
    return TextureFormat::INVALID;
}

const DepthStencilBufferAttachmentGL& getDepthStencilBufferAttachmentGL(TextureFormat::Enum format)
{
    static const DepthStencilBufferAttachmentGL depthStencilBufferAttachments[] =
    {
        /* D16 */   { GL_DEPTH_ATTACHMENT },
        /* D24 */   { GL_DEPTH_ATTACHMENT },
        /* D32 */   { GL_DEPTH_ATTACHMENT },
        
        /* D16F */  { GL_DEPTH_ATTACHMENT },
        /* D24F */  { GL_DEPTH_ATTACHMENT },
        /* D32F */  { GL_DEPTH_ATTACHMENT },
        
        /* D24S8 */ { GL_DEPTH_STENCIL_ATTACHMENT },
        /* D32FS8X24*/ { GL_DEPTH_STENCIL_ATTACHMENT },

        /* S8 */    { GL_STENCIL_ATTACHMENT },
        /* S16 */   { GL_STENCIL_ATTACHMENT },
    };
    
    return depthStencilBufferAttachments[format];
}

bool_t supportsTextureFormatGL(TextureFormat::Enum format)
{
    // Only OpenGL 4.3+ has possibility to query if texture format is supported or not.
    // Due to this, we need to make workaround and create dummy textures with each format to know if it fails or not.
    
    static const size_t width = 1;
    static const size_t height = 1;
    static const size_t bytesPerPixel = (128 / 8); // 128 bits per pixel should be enough even for block-compression cases
    
    static const size_t maxDataSize = width * height * bytesPerPixel;
    static const uint8_t data[maxDataSize] = { 0 };
    
    bool_t isSupported = false;
    
    const TextureFormatGL& textureFormat = getTextureFormatGL(format);
    const TextureFormat::Description& description = TextureFormat::getDescription(format);
    
    GLuint handle = 0;
    
    OMAF_GL_CHECK(glGenTextures(1, &handle));
    OMAF_GL_CHECK(glBindTexture(GL_TEXTURE_2D, handle));
    
    if (TextureFormat::isCompressed(format))
    {
        const TextureFormat::CompressionDescription& compressionDescription = TextureFormat::getCompressionDescription(format);

        uint16_t alignedWidth = width;
        uint16_t alignedHeight = height;
        
        size_t size = TextureFormat::calculateImageSize(compressionDescription, alignedWidth, alignedHeight);
        
        glCompressedTexImage2D(GL_TEXTURE_2D,
                               0,
                               textureFormat.internalFormat,
                               alignedWidth,
                               alignedHeight,
                               0,
                               (GLsizei)size,
                               data);
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     textureFormat.internalFormat,
                     width,
                     height,
                     0,
                     textureFormat.format,
                     textureFormat.type,
                     data);
    }
    
    isSupported = (glGetError() == 0);
    
    if (handle != 0)
    {
        OMAF_GL_CHECK(glDeleteTextures(1, &handle));
    }
    
    return isSupported;
}

bool_t supportsRenderTargetTextureFormatGL(TextureFormat::Enum format)
{
    return false;
}

#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 30

const char_t* getStringGL(GLenum key)
{
    const char_t* string = (const char_t*)glGetString(key);
    OMAF_GL_CHECK_ERRORS();
    
    if(string != OMAF_NULL)
    {
        return string;
    }
    
    return "OMAF_NULL";
}

const char_t* getStringGL(GLenum key, GLuint index)
{
    const char_t* string = (const char_t*)glGetStringi(key, index);
    OMAF_GL_CHECK_ERRORS();
    
    if(string != OMAF_NULL)
    {
        return string;
    }
    
    return "OMAF_NULL";
}

#else

const char_t* getStringGL(GLenum key)
{
    const char_t* string = (const char_t*)glGetString(key);
    OMAF_GL_CHECK_ERRORS();
    
    if(string != OMAF_NULL)
    {
        return string;
    }
    
    return "OMAF_NULL";
}

#endif

GLint getIntegerGL(GLenum name)
{
    GLint result = 0;
    OMAF_GL_CHECK(glGetIntegerv(name, &result));
    
    return result;
}

OMAF_NS_END
