
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
#include "Graphics/NVRRasterizerState.h"
#include "Graphics/NVRBlendState.h"
#include "Graphics/NVRDepthStencilState.h"
#include "Graphics/NVRBufferAccess.h"
#include "Graphics/NVRPrimitiveType.h"
#include "Graphics/NVRTextureType.h"
#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRTextureFilterMode.h"
#include "Graphics/NVRTextureFormat.h"
#include "Graphics/NVRShaderConstantType.h"
#include "Graphics/NVRVertexAttributeFormat.h"
#include "Graphics/NVRViewport.h"
#include "Graphics/NVRScissors.h"
#include "Graphics/NVRIndexBufferFormat.h"
#include "Graphics/NVRComputeBufferAccess.h"

#include "Graphics/OpenGL/NVRGLError.h"

OMAF_NS_BEGIN

// Convert types to OpenGL / OpenGL ES types
GLenum getFillModeGL(FillMode::Enum fillMode);
GLenum getCullModeGL(CullMode::Enum cullMode);
GLenum getFrontFaceGL(FrontFace::Enum frontFace);

GLenum getBlendFunctionGL(BlendFunction::Enum blendFunction);
GLenum getBlendEquationGL(BlendEquation::Enum blendEquation);

GLenum getBufferUsageGL(BufferAccess::Enum access);
GLenum getComputeBufferAccessGL(ComputeBufferAccess::Enum access);

GLboolean getDepthMaskGL(DepthMask::Enum depthMask);
GLenum getDepthFunctionGL(DepthFunction::Enum depthFunction);

GLenum getStencilOperationGL(StencilOperation::Enum stencilOperation);
GLenum getStencilFunctionGL(StencilFunction::Enum stencilFunction);

GLenum getPrimitiveTypeGL(PrimitiveType::Enum primitiveType);

GLenum getTextureTypeGL(TextureType::Enum textureType);
GLenum getTextureAddressModeGL(TextureAddressMode::Enum textureAddressMode);
GLenum getTextureMinFilterModeGL(TextureFilterMode::Enum textureFilterMode, bool_t useMipMaps);
GLenum getTextureMagFilterModeGL(TextureFilterMode::Enum textureFilterMode, bool_t useMipMaps);

GLenum getShaderUniformTypeGL(ShaderConstantType::Enum shaderConstantType);
GLenum getVertexAttributeFormatGL(VertexAttributeFormat::Enum vertexAttributeFormat);
GLenum getIndexBufferFormatGL(IndexBufferFormat::Enum vertexAttributeFormat);

// Helpers
struct TextureFormatGL
{
    GLenum internalFormat;
    GLenum format;
    GLenum type;
};

struct DepthStencilBufferAttachmentGL
{
    GLenum attachment;
};

const TextureFormatGL& getTextureFormatGL(TextureFormat::Enum format);
TextureFormat::Enum getTextureFormat(GLenum format);
const DepthStencilBufferAttachmentGL& getDepthStencilBufferAttachmentGL(TextureFormat::Enum format);

bool_t supportsTextureFormatGL(TextureFormat::Enum format);
bool_t supportsRenderTargetTextureFormatGL(TextureFormat::Enum format);

#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 30

    const char_t* getStringGL(GLenum key);
    const char_t* getStringGL(GLenum key, GLuint index);

#else

    const char_t* getStringGL(GLenum key);

#endif

GLint getIntegerGL(GLenum name);

OMAF_NS_END
