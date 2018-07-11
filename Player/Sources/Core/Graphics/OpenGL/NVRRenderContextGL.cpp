
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
#include "Graphics/OpenGL/NVRRenderContextGL.h"

#include "Graphics/OpenGL/NVRGLContext.h"

#include "Graphics/OpenGL/NVRGLExtensions.h"
#include "Graphics/OpenGL/NVRGLUtilities.h"
#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLCompatibility.h"
#include "Math/OMAFMathFunctions.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(RenderContextGL);

#define OMAF_BUFFER_OFFSET(i) ((uint8_t*)OMAF_NULL + (i))

RenderContextGL::RenderContextGL(RendererType::Enum type, RenderBackend::Parameters& parameters)
: IRenderContext(type, parameters)
, mVAOHandle(0)
, mHostVAOHandle(0)
{
}

RenderContextGL::~RenderContextGL()
{
}

bool_t RenderContextGL::create()
{
    mVertexBuffers.resize(OMAF_MAX_VERTEX_BUFFERS);
    mIndexBuffers.resize(OMAF_MAX_INDEX_BUFFERS);
    mShaders.resize(OMAF_MAX_SHADERS);
    mShaderConstants.resize(OMAF_MAX_SHADER_CONSTANTS);
    mTextures.resize(OMAF_MAX_TEXTURES);
    mRenderTargets.resize(OMAF_MAX_RENDER_TARGETS);
    mActiveComputeResources.resize(OMAF_MAX_COMPUTE_BINDINGS);
    
#if OMAF_PLATFORM_WINDOWS
    
    InitializeGLFunctions();
    
#endif
    
    // Query OpenGL extensions
    ExtensionRegistry::load(DefaultExtensionRegistry);
    
    // Query generic capabilities
    bool_t instancedSupport = (OMAF_GRAPHICS_API_OPENGL_ES >= 30) || (OMAF_GRAPHICS_API_OPENGL >= 31);
    
#if OMAF_GRAPHICS_API_OPENGL
    
    instancedSupport |= DefaultExtensionRegistry._GL_EXT_draw_instanced.supported;
    instancedSupport |= DefaultExtensionRegistry._GL_ARB_draw_instanced.supported;
    
#elif OMAF_GRAPHICS_API_OPENGL_ES
    
    instancedSupport |= DefaultExtensionRegistry._GL_NV_draw_instanced.supported;
    
#endif
    
    bool_t computeSupport = (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43);
    
#if GL_ARB_compute_shader
    
    computeSupport |= DefaultExtensionRegistry._GL_ARB_compute_shader.supported;
    
#endif
    
    mCapabilities.rendererType = getRendererType();
    mCapabilities.apiVersionStr = (const char_t*)getStringGL(GL_VERSION);
    mCapabilities.apiShaderVersionStr = (const char_t*)getStringGL(GL_SHADING_LANGUAGE_VERSION);
    mCapabilities.vendorStr = (const char_t*)getStringGL(GL_VENDOR);
    mCapabilities.deviceStr = (const char_t*)getStringGL(GL_RENDERER);
    mCapabilities.numTextureUnits = min(getIntegerGL(GL_MAX_TEXTURE_IMAGE_UNITS), OMAF_MAX_TEXTURE_UNITS);
    mCapabilities.maxTexture2DSize = getIntegerGL(GL_MAX_TEXTURE_SIZE);
    mCapabilities.maxTexture3DSize = getIntegerGL(GL_MAX_3D_TEXTURE_SIZE);
    mCapabilities.maxTextureCubeSize = getIntegerGL(GL_MAX_CUBE_MAP_TEXTURE_SIZE);
    mCapabilities.maxRenderTargetAttachments = min(getIntegerGL(GL_MAX_DRAW_BUFFERS), OMAF_MAX_RENDER_TARGET_ATTACHMENTS);
    mCapabilities.instancedSupport = instancedSupport;
    mCapabilities.computeSupport = computeSupport;

#if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43) || GL_ARB_compute_shader
    
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &mCapabilities.maxComputeWorkGroupCount[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &mCapabilities.maxComputeWorkGroupCount[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &mCapabilities.maxComputeWorkGroupCount[2]);

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &mCapabilities.maxComputeWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &mCapabilities.maxComputeWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &mCapabilities.maxComputeWorkGroupSize[2]);
    
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &mCapabilities.maxComputeWorkGroupInvocations);
    
#endif

    // Query supported compressed texture formats
    uint32_t numCompressedTextureFormats = getIntegerGL(GL_NUM_COMPRESSED_TEXTURE_FORMATS);
    
    GLint formats[1024];
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats); OMAF_GL_CHECK_ERRORS();
    
    for (size_t f = 0; f < TextureFormat::COUNT; ++f)
    {
        TextureFormat::Enum textureFormat = (TextureFormat::Enum)f;
        const TextureFormatGL& textureFormatGL = getTextureFormatGL(textureFormat);

        for (uint32_t c = 0; c < numCompressedTextureFormats; ++c)
        {
            if (textureFormatGL.internalFormat == formats[c])
            {
                mCapabilities.textureFormats[f] = true;
            }
        }
    }

    // Query supported texture formats
    for (size_t f = 0; f < TextureFormat::COUNT; ++f)
    {
        TextureFormat::Enum textureFormat = (TextureFormat::Enum)f;
        
        if (!TextureFormat::isCompressed(textureFormat) && supportsTextureFormatGL(textureFormat))
        {
            mCapabilities.textureFormats[f] = true;
        }
    }
    
    // Query GPU memory statistics
#if GL_NVX_gpu_memory_info
    
    if (DefaultExtensionRegistry._GL_NVX_gpu_memory_info.supported)
    {
        GLint totalMemory = 0;
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalMemory); OMAF_GL_CHECK_ERRORS();
    
        GLint availableMemory = 0;
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availableMemory); OMAF_GL_CHECK_ERRORS();

        mCapabilities.memoryInfo.appendFormat("%d (Total Memory), %d (Current Available Memory) via GL_NVX_gpu_memory_info", totalMemory, availableMemory);
    }
    
#endif
    
#if WGL_AMD_gpu_association
    
    if (DefaultExtensionRegistry._WGL_AMD_gpu_association.supported)
    {
        GLint maxCount = wglGetGPUIDsAMD(0, 0);
        GLuint id[16] = { 0 };
        
        OMAF_ASSERT(maxCount > 16, "");
        
        wglGetGPUIDsAMD(maxCount, id);
        
        GLuint totalMemory = 0;
        wglGetGPUInfoAMD(id[0], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(GLuint), &totalMemory);

        mCapabilities.memoryInfo.appendFormat("%d (Total Memory via WGL_AMD_gpu_association)", totalMemory);
    }
    
#endif

    // Get host VAO
    mHostVAOHandle = 0;
    OMAF_GL_CHECK(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&mHostVAOHandle));
    
    // Create and bind render backend VAO
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 30
    
    OMAF_GL_CHECK(glGenVertexArrays(1, &mVAOHandle));
    OMAF_GL_CHECK(glBindVertexArray(mVAOHandle));
    
#elif defined(GL_OES_vertex_array_object)
    
    if(DefaultExtensionRegistry._GL_OES_vertex_array_object.supported)
    {
        OMAF_GL_CHECK(glGenVertexArraysOES(1, &mVAOHandle));
        OMAF_GL_CHECK(glBindVertexArrayOES(mVAOHandle));
    }
    
#endif

    // TODO: When WGL, EGL, EAGL, CGL, XGL contexes are created for OpenGL / OpenGL ES render backend move this stuff over there!
    GLContext::createInternal();
    
    resetDefaults();

    return true;
}

void_t RenderContextGL::destroy()
{
    // Bind host VAO, destroy render backend VAO
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 30
    
    OMAF_GL_CHECK(glBindVertexArray(mHostVAOHandle));
    OMAF_GL_CHECK(glDeleteVertexArrays(1, &mVAOHandle));
    
#elif defined(GL_OES_vertex_array_object)
    
    if(DefaultExtensionRegistry._GL_OES_vertex_array_object.supported)
    {
        OMAF_GL_CHECK(glBindVertexArrayOES(mHostVAOHandle));
        OMAF_GL_CHECK(glDeleteVertexArraysOES(1, &mVAOHandle));
    }
    
#endif
    
    mHostVAOHandle = 0;
    
    mVertexBuffers.clear();
    mIndexBuffers.clear();
    mShaders.clear();
    mShaderConstants.clear();
    mTextures.clear();
    mRenderTargets.clear();
    mActiveComputeResources.clear();
}

void_t RenderContextGL::activate()
{
    // Force the OpenGL / OpenGL ES state machine to our expected state
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 30

    OMAF_GL_CHECK(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&mHostVAOHandle));
    
    OMAF_GL_CHECK(glBindVertexArray(mVAOHandle));

#elif defined(GL_OES_vertex_array_object)

    if (DefaultExtensionRegistry._GL_OES_vertex_array_object.supported)
    {
        OMAF_GL_CHECK(glBindVertexArrayOES(mVAOHandle));
    }

#endif
    
    setBlendState(mActiveBlendState, true);
    setDepthStencilState(mActiveDepthStencilState, true);
    setRasterizerState(mActiveRasterizerState, true);

    setViewport(mViewport, true);
    setScissors(mScissors, true);
}

void_t RenderContextGL::deactivate()
{
    // Unbind active resources
    bindShader(ShaderHandle::Invalid);
    bindIndexBuffer(IndexBufferHandle::Invalid);
    bindVertexBuffer(VertexBufferHandle::Invalid);
    bindRenderTarget(RenderTargetHandle::Invalid);
    
    for (uint16_t textureUnit = 0; textureUnit < mCapabilities.numTextureUnits; textureUnit++)
    {
        if (mActiveTexture[textureUnit].isValid())
        {
            TextureGL& activeTexture = mTextures[mActiveTexture[textureUnit].index];
            activeTexture.unbind(textureUnit);

            mActiveTexture[textureUnit] = TextureHandle::Invalid;
        }
    }

    // Bind host VAO
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 30

    OMAF_GL_CHECK(glBindVertexArray(mHostVAOHandle));

#elif defined(GL_OES_vertex_array_object)

    if (DefaultExtensionRegistry._GL_OES_vertex_array_object.supported)
    {
        OMAF_GL_CHECK(glBindVertexArrayOES(mHostVAOHandle));
    }

#endif
}

RenderBackend::Window RenderContextGL::getWindow() const
{
#if OMAF_PLATFORM_WINDOWS

    RECT clientRect;
    GetClientRect((HWND)mParameters.hWnd, &clientRect);

    RenderBackend::Window window;
    window.width = clientRect.right - clientRect.left;
    window.height = clientRect.bottom - clientRect.top;

    return window;

#elif (OMAF_PLATFORM_ANDROID)
    
    OMAF_ASSERT_NOT_IMPLEMENTED();
    
#else

#error Unsupported platform

#endif
}

void_t RenderContextGL::getCapabilities(RenderBackend::Capabilities& capabitilies) const
{
    capabitilies = mCapabilities;
}

void_t RenderContextGL::resetDefaults()
{
    // Bind render backend VAO
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 30

    OMAF_GL_CHECK(glBindVertexArray(mVAOHandle));

#elif defined(GL_OES_vertex_array_object)

    if (DefaultExtensionRegistry._GL_OES_vertex_array_object.supported)
    {
        OMAF_GL_CHECK(glBindVertexArrayOES(mVAOHandle));
    }

#endif

    IRenderContext::resetDefaults();

    setBlendState(mActiveBlendState, true);
    setDepthStencilState(mActiveDepthStencilState, true);
    setRasterizerState(mActiveRasterizerState, true);

    bindShader(ShaderHandle::Invalid);
    bindIndexBuffer(IndexBufferHandle::Invalid);
    bindVertexBuffer(VertexBufferHandle::Invalid);
    bindRenderTarget(RenderTargetHandle::Invalid);

    for (uint16_t textureUnit = 0; textureUnit < mCapabilities.numTextureUnits; textureUnit++)
    {
        if (mActiveTexture[textureUnit].isValid())
        {
            TextureGL& activeTexture = mTextures[mActiveTexture[textureUnit].index];
            activeTexture.unbind(textureUnit);

            mActiveTexture[textureUnit] = TextureHandle::Invalid;
        }
    }

    setViewport(Viewport(0, 0, 0, 0), true);
    setScissors(Scissors(0, 0, 0, 0), true);
}

bool_t RenderContextGL::supportsTextureFormat(TextureFormat::Enum format)
{
    return false;
}

bool_t RenderContextGL::supportsRenderTargetFormat(TextureFormat::Enum format)
{
    return false;
}

void_t RenderContextGL::setRasterizerState(const RasterizerState& rasterizerState, bool_t forced)
{
    if(!equals(mActiveRasterizerState, rasterizerState) || forced)
    {
        if(mActiveRasterizerState.fillMode != rasterizerState.fillMode || forced)
        {
#if !OMAF_GRAPHICS_API_OPENGL_ES
            
            GLenum mode = getFillModeGL(rasterizerState.fillMode);
            
            OMAF_GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, mode));
            
#endif
        }
        
        {
            if(mActiveRasterizerState.cullMode != rasterizerState.cullMode || forced)
            {
                if(rasterizerState.cullMode != CullMode::NONE)
                {
                    GLenum mode = getCullModeGL(rasterizerState.cullMode);
                    
                    OMAF_GL_CHECK(glEnable(GL_CULL_FACE));
                    OMAF_GL_CHECK(glCullFace(mode));
                }
                else
                {
                    OMAF_GL_CHECK(glDisable(GL_CULL_FACE));
                }
            }
        }
        
        {
            if(mActiveRasterizerState.frontFace != rasterizerState.frontFace || forced)
            {
                GLenum mode = getFrontFaceGL(rasterizerState.frontFace);
                
                OMAF_GL_CHECK(glFrontFace(mode));
            }
        }
        
        {
            if(mActiveRasterizerState.scissorTestEnabled != rasterizerState.scissorTestEnabled || forced)
            {
                if(rasterizerState.scissorTestEnabled)
                {
                    OMAF_GL_CHECK(glEnable(GL_SCISSOR_TEST));
                }
                else
                {
                    OMAF_GL_CHECK(glDisable(GL_SCISSOR_TEST));
                }
            }
        }
        
        mActiveRasterizerState = rasterizerState;
    }
}

void_t RenderContextGL::setBlendState(const BlendState& blendState, bool_t forced)
{
    if(!equals(mActiveBlendState, blendState) || forced)
    {
        {
            if(mActiveBlendState.blendEnabled != blendState.blendEnabled || forced)
            {
                if(blendState.blendEnabled)
                {
                    OMAF_GL_CHECK(glEnable(GL_BLEND));
                }
                else
                {
                    OMAF_GL_CHECK(glDisable(GL_BLEND));
                }
            }
        }
        
        {
            bool_t change = false;
            
            if(mActiveBlendState.blendFunctionSrcRgb != blendState.blendFunctionSrcRgb || forced)
            {
                change = true;
            }
            
            if(mActiveBlendState.blendFunctionDstRgb != blendState.blendFunctionDstRgb || forced)
            {
                change = true;
            }
            
            if(mActiveBlendState.blendFunctionSrcAlpha != blendState.blendFunctionSrcAlpha || forced)
            {
                change = true;
            }
            
            if(mActiveBlendState.blendFunctionDstAlpha != blendState.blendFunctionDstAlpha || forced)
            {
                change = true;
            }
            
            if(change)
            {
                GLenum srcRgb = getBlendFunctionGL(blendState.blendFunctionSrcRgb);
                GLenum dstRgb = getBlendFunctionGL(blendState.blendFunctionDstRgb);
                GLenum srcAlpha = getBlendFunctionGL(blendState.blendFunctionSrcAlpha);
                GLenum dstAlpha = getBlendFunctionGL(blendState.blendFunctionDstAlpha);
                
                OMAF_GL_CHECK(glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha));
            }
        }
        
        {
            bool_t change = false;
            
            if(mActiveBlendState.blendEquationRgb != blendState.blendEquationRgb || forced)
            {
                change = true;
            }
            
            if(mActiveBlendState.blendEquationAlpha != blendState.blendEquationAlpha || forced)
            {
                change = true;
            }
            
            if(change)
            {
                GLenum rgb = getBlendEquationGL(blendState.blendEquationRgb);
                GLenum alpha = getBlendEquationGL(blendState.blendEquationAlpha);
                
                OMAF_GL_CHECK(glBlendEquationSeparate(rgb, alpha));
            }
        }
        
        {
            if(mActiveBlendState.blendWriteMask != blendState.blendWriteMask || forced)
            {
                GLboolean red = (blendState.blendWriteMask & ColorMask::RED) & 1;
                GLboolean green = ((blendState.blendWriteMask & ColorMask::GREEN) >> 1) & 1;
                GLboolean blue = ((blendState.blendWriteMask & ColorMask::BLUE) >> 2) & 1;
                GLboolean alpha = ((blendState.blendWriteMask & ColorMask::ALPHA) >> 3) & 1;
                
                OMAF_GL_CHECK(glColorMask(red, green, blue, alpha));
            }
        }
        
        {
            if((mActiveBlendState.blendFactor[0] != blendState.blendFactor[0]) ||
               (mActiveBlendState.blendFactor[1] != blendState.blendFactor[1]) ||
               (mActiveBlendState.blendFactor[2] != blendState.blendFactor[2]) ||
               (mActiveBlendState.blendFactor[3] != blendState.blendFactor[3]) || forced)
            {
                mActiveBlendState.blendFactor[0] = blendState.blendFactor[0];
                mActiveBlendState.blendFactor[1] = blendState.blendFactor[1];
                mActiveBlendState.blendFactor[2] = blendState.blendFactor[2];
                mActiveBlendState.blendFactor[3] = blendState.blendFactor[3];
                
                GLfloat blendFactor[4];
                blendFactor[0] = (blendState.blendFactor[0]) / 255.0f;
                blendFactor[1] = (blendState.blendFactor[1]) / 255.0f;
                blendFactor[2] = (blendState.blendFactor[2]) / 255.0f;
                blendFactor[3] = (blendState.blendFactor[3]) / 255.0f;
                
                OMAF_GL_CHECK(glBlendColor(blendFactor[0], blendFactor[1], blendFactor[2], blendFactor[3]));
            }
        }
        
        {
            if(mActiveBlendState.alphaToCoverageEnabled != blendState.alphaToCoverageEnabled || forced)
            {
                if(blendState.alphaToCoverageEnabled)
                {
                    OMAF_GL_CHECK(glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE));
                }
                else
                {
                    OMAF_GL_CHECK(glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE));
                }
            }
        }
        
        mActiveBlendState = blendState;
    }
}

void_t RenderContextGL::setDepthStencilState(const DepthStencilState& depthStencilState, bool_t forced)
{
    if(!equals(mActiveDepthStencilState, depthStencilState) || forced)
    {
        {
            if(mActiveDepthStencilState.depthTestEnabled != depthStencilState.depthTestEnabled || forced)
            {
                if(depthStencilState.depthTestEnabled)
                {
                    OMAF_GL_CHECK(glEnable(GL_DEPTH_TEST));
                }
                else
                {
                    OMAF_GL_CHECK(glDisable(GL_DEPTH_TEST));
                }
            }
        }
        
        {
            if(mActiveDepthStencilState.depthWriteMask != depthStencilState.depthWriteMask || forced)
            {
                OMAF_GL_CHECK(glDepthMask(depthStencilState.depthWriteMask));
            }
        }
        
        {
            if(mActiveDepthStencilState.depthFunction != depthStencilState.depthFunction || forced)
            {
                GLenum func = getDepthFunctionGL(depthStencilState.depthFunction);
                
                OMAF_GL_CHECK(glDepthFunc(func));
            }
        }
        
        {
            if(mActiveDepthStencilState.stencilTestEnabled != depthStencilState.stencilTestEnabled || forced)
            {
                if(depthStencilState.stencilTestEnabled)
                {
                    OMAF_GL_CHECK(glEnable(GL_STENCIL_TEST));
                }
                else
                {
                    OMAF_GL_CHECK(glDisable(GL_STENCIL_TEST));
                }
            }
        }
        
        // Front
        {
            bool_t change = false;
            
            if(mActiveDepthStencilState.frontStencilFailOperation != depthStencilState.frontStencilFailOperation || forced)
            {
                change = true;
            }
            
            if(mActiveDepthStencilState.frontStencilZFailOperation != depthStencilState.frontStencilZFailOperation || forced)
            {
                change = true;
            }
            
            if(mActiveDepthStencilState.frontStencilZPassOperation != depthStencilState.frontStencilZPassOperation || forced)
            {
                change = true;
            }
            
            if(change)
            {
                GLenum fail = getStencilOperationGL(depthStencilState.frontStencilFailOperation);
                GLenum zfail = getStencilOperationGL(depthStencilState.frontStencilZFailOperation);
                GLenum zpass = getStencilOperationGL(depthStencilState.frontStencilZPassOperation);

                OMAF_GL_CHECK(glStencilOpSeparate(GL_FRONT, fail, zfail, zpass));
            }
        }
        
        {
            bool_t change = false;
            
            if(mActiveDepthStencilState.frontStencilFunction != depthStencilState.frontStencilFunction || forced)
            {
                change = true;
            }
            
            if(mActiveDepthStencilState.stencilReference != depthStencilState.stencilReference || forced)
            {
                change = true;
            }
            
            if(mActiveDepthStencilState.stencilReadMask != depthStencilState.stencilReadMask || forced)
            {
                change = true;
            }
            
            if(change)
            {
                GLenum func = getStencilFunctionGL(depthStencilState.frontStencilFunction);
                GLint ref = depthStencilState.stencilReference;
                GLuint mask = depthStencilState.stencilReadMask;
                
                OMAF_GL_CHECK(glStencilFuncSeparate(GL_FRONT, func, ref, mask));
            }
        }
        
        // Back
        {
            bool_t change = false;
            
            if(mActiveDepthStencilState.backStencilFailOperation != depthStencilState.backStencilFailOperation || forced)
            {
                change = true;
            }
            
            if(mActiveDepthStencilState.backStencilZFailOperation != depthStencilState.backStencilZFailOperation || forced)
            {
                change = true;
            }
            
            if(mActiveDepthStencilState.backStencilZPassOperation != depthStencilState.backStencilZPassOperation || forced)
            {
                change = true;
            }
            
            if(change)
            {
                GLenum fail = getStencilOperationGL(depthStencilState.backStencilFailOperation);
                GLenum zfail = getStencilOperationGL(depthStencilState.backStencilZFailOperation);
                GLenum zpass = getStencilOperationGL(depthStencilState.backStencilZPassOperation);
                
                OMAF_GL_CHECK(glStencilOpSeparate(GL_BACK, fail, zfail, zpass));
            }
        }
        
        {
            bool_t change = false;
            
            if(mActiveDepthStencilState.backStencilFunction != depthStencilState.backStencilFunction || forced)
            {
                change = true;
            }
            
            if(mActiveDepthStencilState.stencilReference != depthStencilState.stencilReference || forced)
            {
                change = true;
            }
            
            if(mActiveDepthStencilState.stencilReadMask != depthStencilState.stencilReadMask || forced)
            {
                change = true;
            }
            
            if(change)
            {
                GLenum func = getStencilFunctionGL(depthStencilState.backStencilFunction);
                GLint ref = depthStencilState.stencilReference;
                GLuint mask = depthStencilState.stencilReadMask;
                
                OMAF_GL_CHECK(glStencilFuncSeparate(GL_BACK, func, ref, mask));
            }
        }
        
        mActiveDepthStencilState = depthStencilState;
    }
}

void_t RenderContextGL::setScissors(const Scissors& scissors, bool_t forced)
{
    if(!equals(mScissors, scissors) || forced)
    {
        mScissors = scissors;

        OMAF_GL_CHECK(glScissor((GLint)scissors.x, (GLint)scissors.y, (GLsizei)scissors.width, (GLsizei)scissors.height));
    }
}

void_t RenderContextGL::setViewport(const Viewport& viewport, bool_t forced)
{
    if(!equals(mViewport, viewport) || forced)
    {
        mViewport = viewport;

        OMAF_GL_CHECK(glViewport((GLint)viewport.x, (GLint)viewport.y, (GLsizei)viewport.width, (GLsizei)viewport.height));
    }
}

void_t RenderContextGL::clearColor(uint32_t color)
{
    float32_t r, g, b, a;
    unpackColor(color, r, g, b, a);
        
    OMAF_GL_CHECK(glClearColor(r, g, b, a));
    OMAF_GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
}

void_t RenderContextGL::clearDepth(float32_t value)
{
#if OMAF_GRAPHICS_API_OPENGL_ES
        
    OMAF_GL_CHECK(glClearDepthf(value));
        
#else
        
    OMAF_GL_CHECK(glClearDepth(value));
        
#endif
    
    OMAF_GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));
}

void_t RenderContextGL::clearStencil(int32_t value)
{
    OMAF_GL_CHECK(glClearStencil(value));
    OMAF_GL_CHECK(glClear(GL_STENCIL_BUFFER_BIT));
}

void_t RenderContextGL::clear(uint16_t clearMask, uint32_t color, float32_t depth, int32_t stencil)
{
    GLbitfield mask = 0;
    mask |= (clearMask & ClearMask::COLOR_BUFFER) ? GL_COLOR_BUFFER_BIT : 0;
    mask |= (clearMask & ClearMask::DEPTH_BUFFER) ? GL_DEPTH_BUFFER_BIT : 0;
    mask |= (clearMask & ClearMask::STENCIL_BUFFER) ? GL_STENCIL_BUFFER_BIT : 0;
    
    if(clearMask & ClearMask::COLOR_BUFFER)
    {
        float32_t r, g, b, a;
        unpackColor(color, r, g, b, a);
        
        OMAF_GL_CHECK(glClearColor(r, g, b, a));
    }
    
    if(clearMask & ClearMask::DEPTH_BUFFER)
    {
#if OMAF_GRAPHICS_API_OPENGL_ES
        
        OMAF_GL_CHECK(glClearDepthf(depth));
        
#else
        
        OMAF_GL_CHECK(glClearDepth(depth));
        
#endif
    }
    
    if(clearMask & ClearMask::STENCIL_BUFFER)
    {
        OMAF_GL_CHECK(glClearStencil(stencil));
    }
    
    OMAF_GL_CHECK(glClear(mask));
}

void_t RenderContextGL::bindVertexBuffer(VertexBufferHandle handle)
{
    if (mActiveVertexBuffer != handle)
    {
        if (handle != VertexBufferHandle::Invalid)
        {
            VertexBufferGL& vertexBuffer = mVertexBuffers[handle.index];
            vertexBuffer.bind();
        }
        else
        {
            if (mActiveVertexBuffer.isValid())
            {
                VertexBufferGL& activeVertexBuffer = mVertexBuffers[mActiveVertexBuffer.index];
                activeVertexBuffer.unbind(); // Set no active vertex buffer with unbind
            }
        }
        
        mActiveVertexBuffer = handle;
    }
}

void_t RenderContextGL::bindIndexBuffer(IndexBufferHandle handle)
{
    if (mActiveIndexBuffer != handle)
    {
        if (handle != IndexBufferHandle::Invalid)
        {
            IndexBufferGL& indexBuffer = mIndexBuffers[handle.index];
            indexBuffer.bind();
        }
        else
        {
            if (mActiveIndexBuffer.isValid())
            {
                IndexBufferGL& activeIndexBuffer = mIndexBuffers[mActiveIndexBuffer.index];
                activeIndexBuffer.unbind(); // Set no active index buffer with unbind
            }
        }
    
        mActiveIndexBuffer = handle;
    }
}

void_t RenderContextGL::bindShader(ShaderHandle handle)
{
    if (mActiveShader != handle)
    {
        if (handle != ShaderHandle::Invalid)
        {
            ShaderGL& shader = mShaders[handle.index];
            shader.bind();
        }
        else
        {
            if (mActiveShader.isValid())
            {
                ShaderGL& activeShader = mShaders[mActiveShader.index];
                activeShader.unbind(); // Set no active shader with unbind
            }
        }
        
        mActiveShader = handle;
    }
}

void_t RenderContextGL::bindTexture(TextureHandle handle, uint16_t textureUnit)
{
    if (mActiveTexture[textureUnit] != handle)
    {
        if (handle != TextureHandle::Invalid)
        {
            TextureGL& texture = mTextures[handle.index];
            texture.bind(textureUnit);
        }
        else
        {
            if (mActiveTexture[textureUnit].isValid())
            {
                TextureGL& activeTexture = mTextures[mActiveTexture[textureUnit].index];
                activeTexture.unbind(textureUnit); // Set no active texture with unbind
            }
        }
            
        mActiveTexture[textureUnit] = handle;
    }
}

void_t RenderContextGL::bindRenderTarget(RenderTargetHandle handle)
{
    if (handle == RenderTargetHandle::Invalid) // Settings default framebuffer is special case
    {
        if (mActiveRenderTarget.isValid())
        {
            RenderTargetGL& activeRenderTarget = mRenderTargets[mActiveRenderTarget.index];
            activeRenderTarget.discard();
        }

        mActiveRenderTarget = RenderTargetHandle::Invalid;
        
        setViewport(Viewport(0, 0, 0, 0), true);

        // TODO: OMAF_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }
    else if (mActiveRenderTarget != handle) // Normal render target switching...
    {
        if (mActiveRenderTarget.isValid())
        {
            RenderTargetGL& activeRenderTarget = mRenderTargets[mActiveRenderTarget.index];
            activeRenderTarget.discard();
        }
        
        RenderTargetGL& renderTarget = mRenderTargets[handle.index];
        renderTarget.bind();
        
        setViewport(Viewport(0, 0, renderTarget.getWidth(), renderTarget.getHeight()), true);
        
        mActiveRenderTarget = handle;
    }
}

void_t RenderContextGL::setShaderConstant(ShaderConstantHandle handle, const void_t* values, uint32_t numValues)
{
    OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");

    if (mActiveShader.isValid())
    {
        ShaderConstantGL& shaderConstant = mShaderConstants[handle.index];
    
        ShaderGL& shaderProgram = mShaders[mActiveShader.index];
        shaderProgram.setUniform(shaderConstant.getType(), shaderConstant.getHandle(), values, numValues);
    }
}

void_t RenderContextGL::setShaderConstant(ShaderHandle shaderHandle, ShaderConstantHandle constantHandle, const void_t* values, uint32_t numValues)
{
    ShaderHandle activeShader = mActiveShader;

    bindShader(shaderHandle);

    ShaderConstantGL& shaderConstant = mShaderConstants[constantHandle.index];
    ShaderGL& shaderProgram = mShaders[shaderHandle.index];
    shaderProgram.setUniform(shaderConstant.getType(), shaderConstant.getHandle(), values, numValues);

    bindShader(activeShader);
}

void_t RenderContextGL::setSamplerState(TextureHandle handle, const SamplerState& samplerState)
{
    TextureGL& texture = mTextures[handle.index];
    texture.setSamplerState(samplerState.addressModeU,
                            samplerState.addressModeV,
                            samplerState.addressModeW,
                            samplerState.filterMode);
}

void_t RenderContextGL::draw(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count)
{
    OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
    OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");

    // Draw
    GLenum mode = getPrimitiveTypeGL(primitiveType);
    GLint first = (GLint)offset;
    GLsizei numVertices = (GLsizei)((count == OMAF_UINT32_MAX) ? 0 : count);
    
    if (mActiveVertexBuffer.isValid())
    {
        enableVertexAttributes();
        
        VertexBufferGL& activeVertexBuffer = mVertexBuffers[mActiveVertexBuffer.index];
        numVertices = (GLsizei)((count == OMAF_UINT32_MAX) ? activeVertexBuffer.getNumVertices() : count);
        
        OMAF_ASSERT(((GLsizei)offset + numVertices) <= activeVertexBuffer.getNumVertices(), "Vertex buffer: Out of bounds");
    }
    
    OMAF_GL_CHECK(glDrawArrays(mode, first, numVertices));
    
    if (mActiveVertexBuffer.isValid())
    {
        disableVertexAttributes();
    }
}

void_t RenderContextGL::drawIndexed(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count)
{
    OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
    OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");

    // Draw indexed
    GLenum mode = getPrimitiveTypeGL(primitiveType);
    GLenum type = GL_UNSIGNED_INT;
    GLsizei numIndices = (GLsizei)((count == OMAF_UINT32_MAX) ? 0 : count);
    
    if (mActiveIndexBuffer.isValid())
    {
        enableVertexAttributes();
        
        IndexBufferGL& activeIndexBuffer = mIndexBuffers[mActiveIndexBuffer.index];
        numIndices = (GLsizei)((count == OMAF_UINT32_MAX) ? activeIndexBuffer.getNumIndices() : count);
        type = activeIndexBuffer.getFormat();
        
        OMAF_ASSERT(((GLsizei)offset + numIndices) <= activeIndexBuffer.getNumIndices(), "Index buffer: Out of bounds");
    }
    
    OMAF_GL_CHECK(glDrawElements(mode, numIndices, type, OMAF_BUFFER_OFFSET(offset)));
    
    if (mActiveIndexBuffer.isValid())
    {
        disableVertexAttributes();
    }
}

void_t RenderContextGL::drawInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t vertexCount, uint32_t instanceCount)
{
    OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
    OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");
    
    GLenum mode = getPrimitiveTypeGL(primitiveType);
    GLsizei numInstances = (GLsizei)instanceCount;
    GLsizei numPrimitives = (GLsizei)vertexCount;
    
    if (mActiveVertexBuffer.isValid())
    {
        enableVertexAttributes();
    }
    
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 31
    
    OMAF_GL_CHECK(glDrawArraysInstanced(mode, (GLint)offset, numPrimitives, numInstances));
    
#elif GL_EXT_draw_instanced
    
    if(DefaultExtensionRegistry._GL_EXT_draw_instanced.supported)
    {
        OMAF_GL_CHECK(glDrawArraysInstancedEXT(mode, (GLint)offset, numPrimitives, numInstances));
    }
    
#elif GL_ARB_draw_instanced
  
    if(DefaultExtensionRegistry._GL_ARB_draw_instanced.supported)
    {
        OMAF_GL_CHECK(glDrawArraysInstancedARB(mode, (GLint)offset, numPrimitives, numInstances));
    }
    
#elif GL_NV_draw_instanced
    
    if(DefaultExtensionRegistry._GL_NV_draw_instanced.supported)
    {
        OMAF_GL_CHECK(glDrawArraysInstancedNV(mode, (GLint)offset, numPrimitives, numInstances));
    }
    
#else
    
    OMAF_ASSERT(false, "Render backend doesn't support instancing");
    
#endif
    
    if (mActiveVertexBuffer.isValid())
    {
        disableVertexAttributes();
    }
}

void_t RenderContextGL::drawIndexedInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t indexCount, uint32_t instanceCount)
{
    OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
    OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");

    GLenum mode = getPrimitiveTypeGL(primitiveType);
    GLenum type = GL_UNSIGNED_INT;
    GLsizei numInstances = (GLsizei)instanceCount;
    GLsizei numPrimitives = (GLsizei)indexCount;
    
    if (mActiveIndexBuffer.isValid())
    {
        enableVertexAttributes();
    }
    
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 31
    
    OMAF_GL_CHECK(glDrawElementsInstanced(mode, numPrimitives, type, OMAF_BUFFER_OFFSET(offset), numInstances));
    
#elif GL_EXT_draw_instanced
    
    if(DefaultExtensionRegistry._GL_EXT_draw_instanced.supported)
    {
        OMAF_GL_CHECK(glDrawElementsInstancedEXT(mode, numPrimitives, type, OMAF_BUFFER_OFFSET(offset), numInstances));
    }
    
#elif GL_ARB_draw_instanced
    
    if(DefaultExtensionRegistry._GL_ARB_draw_instanced.supported)
    {
        OMAF_GL_CHECK(glDrawElementsInstancedARB(mode, numPrimitives, type, OMAF_BUFFER_OFFSET(offset), numInstances));
    }
    
#elif GL_NV_draw_instanced
    
    if(DefaultExtensionRegistry._GL_NV_draw_instanced.supported)
    {
        OMAF_GL_CHECK(glDrawElementsInstancedNV(mode, numPrimitives, type, OMAF_BUFFER_OFFSET(offset), numInstances));
    }
    
#else
    
    OMAF_ASSERT(false, "Render backend doesn't support instancing");
    
#endif
    
    if (mActiveIndexBuffer.isValid())
    {
        disableVertexAttributes();
    }
}

void_t RenderContextGL::bindComputeImage(uint16_t stage, TextureHandle handle, ComputeBufferAccess::Enum access)
{
    TextureHandle activeStageHandle = mActiveComputeResources[stage].handle;

    if (activeStageHandle != handle)
    {
        if (handle != TextureHandle::Invalid)
        {
            TextureGL& texture = mTextures[handle.index];
            texture.bindCompute(stage, access);
        }
        else
        {
            if (activeStageHandle.isValid())
            {
                TextureGL& activeTexture = mTextures[activeStageHandle.index];
                activeTexture.unbindCompute(stage);
            }
        }

        ComputeResourceBinding binding;
        binding.type = ComputeResourceType::IMAGE_BUFFER;
        binding.handle = handle;

        mActiveComputeResources[stage] = binding;
    }
}

void_t RenderContextGL::bindComputeVertexBuffer(uint16_t stage, VertexBufferHandle handle, ComputeBufferAccess::Enum access)
{
    VertexBufferHandle activeStageHandle = mActiveComputeResources[stage].handle;

    if (activeStageHandle != handle)
    {
        if (handle != VertexBufferHandle::Invalid)
        {
            VertexBufferGL& vertexBuffer = mVertexBuffers[handle.index];
            vertexBuffer.bindCompute(stage, access);
        }
        else
        {
            if (activeStageHandle.isValid())
            {
                VertexBufferGL& activeVertexBuffer = mVertexBuffers[activeStageHandle.index];
                activeVertexBuffer.unbindCompute(stage);
            }
        }

        ComputeResourceBinding binding;
        binding.type = ComputeResourceType::VERTEX_BUFFER;
        binding.handle = handle;

        mActiveComputeResources[stage] = binding;
    }
}

void_t RenderContextGL::bindComputeIndexBuffer(uint16_t stage, IndexBufferHandle handle, ComputeBufferAccess::Enum access)
{
    IndexBufferHandle activeStageHandle = mActiveComputeResources[stage].handle;

    if (activeStageHandle != handle)
    {
        if (handle != IndexBufferHandle::Invalid)
        {
            IndexBufferGL& indexBuffer = mIndexBuffers[handle.index];
            indexBuffer.bindCompute(stage, access);
        }
        else
        {
            if (activeStageHandle.isValid())
            {
                IndexBufferGL& activeIndexBuffer = mIndexBuffers[activeStageHandle.index];
                activeIndexBuffer.unbindCompute(stage);
            }
        }

        ComputeResourceBinding binding;
        binding.type = ComputeResourceType::INDEX_BUFFER;
        binding.handle = handle;

        mActiveComputeResources[stage] = binding;
    }
}

void_t RenderContextGL::dispatchCompute(ShaderHandle handle, uint16_t numGroupsX, uint16_t numGroupsY, uint16_t numGroupsZ)
{
#if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43) || GL_ARB_compute_shader
    
    bindShader(handle);

    #if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43)
        
        OMAF_GL_CHECK(glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ));
        
    #elif GL_ARB_compute_shader
        
        if(DefaultExtensionRegistry._GL_ARB_compute_shader.supported)
        {
            OMAF_GL_CHECK(glDispatchComputeARB(numGroupsX, numGroupsY, numGroupsZ));
        }
        
    #endif

    // Set barriers based on bound compute resources
    GLbitfield barrier = 0;

    for (uint8_t i = 0; i < mActiveComputeResources.getSize(); ++i)
    {
        ComputeResourceBinding& binding = mActiveComputeResources[i];

        if (binding.type == ComputeResourceType::IMAGE_BUFFER)
        {
            barrier |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        }
        else if (binding.type == ComputeResourceType::INDEX_BUFFER)
        {
            barrier |= GL_SHADER_STORAGE_BARRIER_BIT;
        }
        else if (binding.type == ComputeResourceType::VERTEX_BUFFER)
        {
            barrier |= GL_SHADER_STORAGE_BARRIER_BIT;
        }
    }

    OMAF_GL_CHECK(glMemoryBarrier(barrier));

    // Reset all active compute resources
    for (uint8_t i = 0; i < mActiveComputeResources.getSize(); ++i)
    {
        ComputeResourceBinding& binding = mActiveComputeResources[i];

        if (binding.type == ComputeResourceType::IMAGE_BUFFER)
        {
            TextureHandle handle = binding.handle;

            if (handle.isValid())
            {
                TextureGL& activeTexture = mTextures[handle.index];
                activeTexture.unbindCompute(i);

                ComputeResourceBinding binding;
                binding.type = ComputeResourceType::INVALID;
                binding.handle = TextureHandle::Invalid;

                mActiveComputeResources[i] = binding;
            }
        }
        else if (binding.type == ComputeResourceType::INDEX_BUFFER)
        {
            IndexBufferHandle handle = binding.handle;

            if (handle.isValid())
            {
                IndexBufferGL& activeIndexBuffer = mIndexBuffers[handle.index];
                activeIndexBuffer.unbindCompute(i);

                ComputeResourceBinding binding;
                binding.type = ComputeResourceType::INVALID;
                binding.handle = IndexBufferHandle::Invalid;

                mActiveComputeResources[i] = binding;
            }
        }
        else if (binding.type == ComputeResourceType::VERTEX_BUFFER)
        {
            VertexBufferHandle handle = binding.handle;

            if (handle.isValid())
            {
                VertexBufferGL& activeVertexBuffer = mVertexBuffers[handle.index];
                activeVertexBuffer.unbindCompute(i);

                ComputeResourceBinding binding;
                binding.type = ComputeResourceType::INVALID;
                binding.handle = VertexBufferHandle::Invalid;

                mActiveComputeResources[i] = binding;
            }
        }
    }
    
#else

    OMAF_ASSERT(false, "Render backend doesn't support compute shaders");
    
#endif
}

void_t RenderContextGL::submitFrame()
{
}

bool_t RenderContextGL::createVertexBuffer(VertexBufferHandle handle, const VertexBufferDesc& desc)
{
    VertexBufferGL& vertexBuffer = mVertexBuffers[handle.index];
    
    return vertexBuffer.create(desc.declaration,
                               desc.access,
                               desc.data,
                               desc.size);
}

bool_t RenderContextGL::createIndexBuffer(IndexBufferHandle handle, const IndexBufferDesc& desc)
{
    IndexBufferGL& indexBuffer = mIndexBuffers[handle.index];
    
    return indexBuffer.create(desc.access,
                              desc.format,
                              desc.data,
                              desc.size);
}

bool_t RenderContextGL::createShader(ShaderHandle handle, const char_t* vertexShader, const char_t* fragmentShader)
{
    ShaderGL& shader = mShaders[handle.index];
    
    return shader.create(vertexShader, fragmentShader);
}

bool_t RenderContextGL::createShader(ShaderHandle handle, const char_t* computeShader)
{
    ShaderGL& shader = mShaders[handle.index];
    
    return shader.create(computeShader);
}

bool_t RenderContextGL::createShaderConstant(ShaderHandle shaderHandle, ShaderConstantHandle constantHandle, const char_t* name, ShaderConstantType::Enum type)
{
    ShaderConstantGL& shaderConstant = mShaderConstants[constantHandle.index];
    
    return shaderConstant.create(name, type);
}

bool_t RenderContextGL::createTexture(TextureHandle handle, const TextureDesc& desc)
{
    TextureGL& texture = mTextures[handle.index];

    return texture.create(desc);
}

bool_t RenderContextGL::createNativeTexture(TextureHandle handle, const NativeTextureDesc& desc)
{
    TextureGL& texture = mTextures[handle.index];
    
    return texture.createNative(desc);
}

bool_t RenderContextGL::createRenderTarget(RenderTargetHandle handle,
                                           TextureHandle* attachments,
                                           uint8_t numAttachments,
                                           int16_t discardMask)
{
    RenderTargetGL& renderTarget = mRenderTargets[handle.index];
    
    TextureGL* textures[OMAF_MAX_RENDER_TARGET_ATTACHMENTS];
    
    for (uint8_t i = 0; i < numAttachments; ++i)
    {
        TextureHandle attachmentHandle = attachments[i];
        textures[i] = &mTextures[attachmentHandle.index];
    }

    return renderTarget.create(textures, numAttachments, discardMask);
}

void_t RenderContextGL::updateVertexBuffer(VertexBufferHandle handle, uint32_t offset, const MemoryBuffer* buffer)
{
    VertexBufferGL& vertexBuffer = mVertexBuffers[handle.index];
    vertexBuffer.bufferData(offset, buffer->data + buffer->offset, buffer->size);
}

void_t RenderContextGL::updateIndexBuffer(IndexBufferHandle handle, uint32_t offset, const MemoryBuffer* buffer)
{
    IndexBufferGL& indexBuffer = mIndexBuffers[handle.index];
    indexBuffer.bufferData(offset, buffer->data + buffer->offset, buffer->size);
}

void_t RenderContextGL::destroyVertexBuffer(VertexBufferHandle handle)
{
    VertexBufferGL& vertexBuffer = mVertexBuffers[handle.index];
    vertexBuffer.destroy();
    
    if (mActiveVertexBuffer == handle)
    {
        mActiveVertexBuffer = VertexBufferHandle::Invalid;
    }
}

void_t RenderContextGL::destroyIndexBuffer(IndexBufferHandle handle)
{
    IndexBufferGL& indexBuffer = mIndexBuffers[handle.index];
    indexBuffer.destroy();
    
    if (mActiveIndexBuffer == handle)
    {
        mActiveIndexBuffer = IndexBufferHandle::Invalid;
    }
}

void_t RenderContextGL::destroyShader(ShaderHandle handle)
{
    ShaderGL& shader = mShaders[handle.index];
    shader.destroy();
    
    if (mActiveShader == handle)
    {
        mActiveShader = ShaderHandle::Invalid;
    }
}

void_t RenderContextGL::destroyShaderConstant(ShaderConstantHandle handle)
{
    ShaderConstantGL& shaderConstant = mShaderConstants[handle.index];
    shaderConstant.destroy();
}

void_t RenderContextGL::destroyTexture(TextureHandle handle)
{
    TextureGL& texture = mTextures[handle.index];
    texture.destroy();
    
    for (size_t textureIndex = 0; textureIndex < OMAF_MAX_TEXTURE_UNITS; ++textureIndex)
    {
        if (mActiveTexture[textureIndex] == handle)
        {
            mActiveTexture[textureIndex] = TextureHandle::Invalid;
        }
    }
}

void_t RenderContextGL::destroyRenderTarget(RenderTargetHandle handle)
{
    RenderTargetGL& renderTarget = mRenderTargets[handle.index];
    renderTarget.destroy();
    
    if (mActiveRenderTarget == handle)
    {
        mActiveRenderTarget = RenderTargetHandle::Invalid;
    }
}

void_t RenderContextGL::pushDebugMarker(const char_t* name)
{
#if OMAF_DEBUG_BUILD

#if GL_KHR_debug

    if(DefaultExtensionRegistry._GL_KHR_debug.supported)
    {
        OMAF_GL_CHECK(glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION_KHR, 1, -1, name));
    }

#elif GL_EXT_debug_marker

    if(DefaultExtensionRegistry._GL_EXT_debug_marker.supported)
    {
        OMAF_GL_CHECK(glPushGroupMarkerEXT(0, name));
    }

#endif

#endif
}

void_t RenderContextGL::popDebugMarker()
{
#if OMAF_DEBUG_BUILD

#if GL_KHR_debug

    if(DefaultExtensionRegistry._GL_KHR_debug.supported)
    {
        OMAF_GL_CHECK(glPopDebugGroupKHR());
    }

#elif GL_EXT_debug_marker

    if(DefaultExtensionRegistry._GL_EXT_debug_marker.supported)
    {
        OMAF_GL_CHECK(glPopGroupMarkerEXT());
    }

#endif

#endif
}

void_t RenderContextGL::enableVertexAttributes()
{
    OMAF_ASSERT(mActiveVertexBuffer.isValid(), "");
    OMAF_ASSERT(mActiveShader.isValid(), "");
    
    const VertexBufferGL& activeVertexBuffer = mVertexBuffers[mActiveVertexBuffer.index];
    const VertexDeclaration& vertexDeclaration = activeVertexBuffer.getVertexDeclaration();
    const VertexDeclaration::VertexAttributeList& vertexAttributes = vertexDeclaration.getAttributes();
    const ShaderGL& shader = mShaders[mActiveShader.index];
    
    for (size_t a = 0; a < vertexAttributes.getSize(); ++a)
    {
        const VertexAttribute& vertexAttribute = vertexAttributes[a];
        const ShaderAttributeGL* shaderAttribute = shader.getAttribute(vertexAttribute.name);
        
        if (shaderAttribute != OMAF_NULL)
        {
            GLuint location = shaderAttribute->handle;
            GLenum format = getVertexAttributeFormatGL(vertexAttribute.format);
            GLboolean normalized = (GLboolean)vertexAttribute.normalized;
            GLint count = (GLint)vertexAttribute.count;
            GLint stride = (GLint)vertexDeclaration.getStride();
            size_t offset = (size_t)vertexAttribute.offset;
            
            OMAF_GL_CHECK(glEnableVertexAttribArray(location));
            OMAF_GL_CHECK(glVertexAttribPointer(location, count, format, normalized, stride, OMAF_BUFFER_OFFSET(offset)));
        }
    }
}

void_t RenderContextGL::disableVertexAttributes()
{
    OMAF_ASSERT(mActiveVertexBuffer.isValid(), "");
    OMAF_ASSERT(mActiveShader.isValid(), "");
    
    const VertexBufferGL& activeVertexBuffer = mVertexBuffers[mActiveVertexBuffer.index];
    const VertexDeclaration& vertexDeclaration = activeVertexBuffer.getVertexDeclaration();
    const VertexDeclaration::VertexAttributeList& vertexAttributes = vertexDeclaration.getAttributes();
    const ShaderGL& shader = mShaders[mActiveShader.index];
    
    for (size_t a = 0; a < vertexAttributes.getSize(); ++a)
    {
        const VertexAttribute& vertexAttribute = vertexAttributes[a];
        const ShaderAttributeGL* shaderAttribute = shader.getAttribute(vertexAttribute.name);
        
        if (shaderAttribute != OMAF_NULL)
        {
            GLuint location = shaderAttribute->handle;

            OMAF_GL_CHECK(glDisableVertexAttribArray(location));
        }
    }
}

OMAF_NS_END
