
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
#include "Graphics/OpenGL/NVRTextureGL.h"

#include "Graphics/OpenGL/NVRGLExtensions.h"
#include "Graphics/OpenGL/NVRGLUtilities.h"
#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLCompatibility.h"
#include "Math/OMAFMathFunctions.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(TextureGL);

TextureGL::TextureGL()
: mHandle(0)
, mTarget(0)
, mType(TextureType::INVALID)
, mComputeAccess(ComputeBufferAccess::INVALID)
, mWidth(0)
, mHeight(0)
, mNumMipmaps(0)
, mFormat(TextureFormat::INVALID)
, mAddressModeU(TextureAddressMode::INVALID)
, mAddressModeV(TextureAddressMode::INVALID)
, mAddressModeW(TextureAddressMode::INVALID)
, mFilterMode(TextureFilterMode::INVALID)
, mNativeHandle(false)
{
}

TextureGL::~TextureGL()
{
    OMAF_ASSERT(mHandle == 0, "Texture is not destroyed");
}

GLuint TextureGL::getHandle() const
{
    return mHandle;
}

uint16_t TextureGL::getWidth() const
{
    return mWidth;
}

uint16_t TextureGL::getHeight() const
{
    return mHeight;
}

uint8_t TextureGL::getNumMipmaps() const
{
    return mNumMipmaps;
}

TextureFormat::Enum TextureGL::getFormat() const
{
    return mFormat;
}

TextureAddressMode::Enum TextureGL::getAddressModeU() const
{
    return mAddressModeU;
}

TextureAddressMode::Enum TextureGL::getAddressModeV() const
{
    return mAddressModeV;
}

TextureAddressMode::Enum TextureGL::getAddressModeW() const
{
    return mAddressModeW;
}

TextureFilterMode::Enum TextureGL::getFilterMode() const
{
    return mFilterMode;
}

void_t TextureGL::setSamplerState(TextureAddressMode::Enum addressModeU,
                                  TextureAddressMode::Enum addressModeV,
                                  TextureAddressMode::Enum addressModeW,
                                  TextureFilterMode::Enum filterMode)
{
    GLenum bindingType = 0;

    if (mTarget == GL_TEXTURE_2D)
    {
        bindingType = GL_TEXTURE_BINDING_2D;
    }
    else if (mTarget == GL_TEXTURE_3D)
    {
        bindingType = GL_TEXTURE_BINDING_3D;
    }
    else if (mTarget == GL_TEXTURE_CUBE_MAP)
    {
        bindingType = GL_TEXTURE_BINDING_CUBE_MAP;
    }
#if OMAF_GRAPHICS_API_OPENGL_ES
    else if (mTarget == GL_TEXTURE_EXTERNAL_OES)
    {
        // EGLImage limitation: https://www.khronos.org/registry/OpenGL/extensions/OES/OES_EGL_image_external.txt
        bindingType = GL_TEXTURE_BINDING_EXTERNAL_OES;
        addressModeU = TextureAddressMode::CLAMP;
        addressModeV = TextureAddressMode::CLAMP;
        if (filterMode == TextureFilterMode::TRILINEAR)
        {
            filterMode = TextureFilterMode::BILINEAR;
        }
    }
#endif
#if OMAF_GRAPHICS_API_OPENGL
    else if (mTarget == GL_TEXTURE_RECTANGLE)
    {
        // TextureRectangle limitation: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_rectangle.txt
        bindingType = GL_TEXTURE_BINDING_RECTANGLE;
        addressModeU = TextureAddressMode::CLAMP;
        addressModeV = TextureAddressMode::CLAMP;
        if (filterMode == TextureFilterMode::TRILINEAR)
        {
            filterMode = TextureFilterMode::BILINEAR;
        }
    }
#endif
    else
    {
        OMAF_ASSERT_UNREACHABLE();
    }

    // Backup state
    GLint currentTexture = 0;
    OMAF_GL_CHECK(glGetIntegerv(bindingType, &currentTexture));

    OMAF_GL_CHECK(glBindTexture(mTarget, mHandle));

    // Set address mode
    GLenum addressModeS = getTextureAddressModeGL(addressModeU);
    GLenum addressModeT = getTextureAddressModeGL(addressModeV);

    OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, addressModeS));
    OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, addressModeT));

    if (mTarget == GL_TEXTURE_3D)
    {
        GLenum addressModeR = getTextureAddressModeGL(addressModeW);

        OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_WRAP_R, addressModeR));
    }

    // Set min/mag filters
    bool_t useMipmaps = false;

    if (mNumMipmaps > 1)
    {
        useMipmaps = true;
    }

    GLenum min = getTextureMinFilterModeGL(filterMode, useMipmaps);
    GLenum mag = getTextureMagFilterModeGL(filterMode, useMipmaps);

    OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, min));
    OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, mag));

    // Restore state
    OMAF_GL_CHECK(glBindTexture(mTarget, currentTexture));
}

bool_t TextureGL::create(const TextureDesc& desc)
{
    OMAF_GL_CHECK(glGenTextures(1, &mHandle));

    if (mHandle != 0)
    {
        mTarget = getTextureTypeGL(desc.type);

        mType = desc.type;
        mComputeAccess = desc.computeAccess;

        mWidth = desc.width;
        mHeight = desc.height;

        mNumMipmaps = desc.numMipMaps;
        mFormat = desc.format;
        mAddressModeU = desc.samplerState.addressModeU;
        mAddressModeV = desc.samplerState.addressModeV;
        mAddressModeW = desc.samplerState.addressModeW;
        mFilterMode = desc.samplerState.filterMode;

        GLenum bindingType = 0;

        if (mTarget == GL_TEXTURE_2D)
        {
            bindingType = GL_TEXTURE_BINDING_2D;
        }
        else if (mTarget == GL_TEXTURE_3D)
        {
            bindingType = GL_TEXTURE_BINDING_3D;
        }
        else if (mTarget == GL_TEXTURE_CUBE_MAP)
        {
            bindingType = GL_TEXTURE_BINDING_CUBE_MAP;
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }

        GLint currentTexture = 0;
        OMAF_GL_CHECK(glGetIntegerv(bindingType, &currentTexture));

        OMAF_GL_CHECK(glBindTexture(mTarget, mHandle));

        // Set address mode
        GLenum addressModeS = getTextureAddressModeGL(desc.samplerState.addressModeU);
        GLenum addressModeT = getTextureAddressModeGL(desc.samplerState.addressModeV);

        OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, addressModeS));
        OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, addressModeT));

        if (mTarget == GL_TEXTURE_3D)
        {
            GLenum addressModeR = getTextureAddressModeGL(desc.samplerState.addressModeW);

            OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_WRAP_R, addressModeR));
        }

        // Set min/mag filters
        bool_t useMipmaps = false;

        if ((mNumMipmaps > 1) || desc.generateMipMaps)
        {
            useMipmaps = true;
        }

        GLenum min = getTextureMinFilterModeGL(desc.samplerState.filterMode, useMipmaps);
        GLenum mag = getTextureMagFilterModeGL(desc.samplerState.filterMode, useMipmaps);

        OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, min));
        OMAF_GL_CHECK(glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, mag));

        // Upload data
        const TextureFormat::Description& description = TextureFormat::getDescription(desc.format);

        if (desc.type == TextureType::TEXTURE_2D)
        {
            if (description.compressed)
            {
                OMAF_ASSERT_NOT_IMPLEMENTED();
            }
            else
            {
                TextureFormatGL formatGL = getTextureFormatGL(desc.format);

                const uint8_t* ptr = (uint8_t*)desc.data;

                if (!desc.generateMipMaps)
                {
                    for(uint16_t i = 0; i < mNumMipmaps; ++i)
                    {
                        uint16_t mipMapLevelWidth = mWidth >> i;
                        uint16_t mipMapLevelHeight = mHeight >> i;

                        uint32_t mipMapLevelSize = mipMapLevelWidth * mipMapLevelHeight * (description.bitsPerPixel / 8);

                        OMAF_GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, mipMapLevelWidth));
                        OMAF_GL_CHECK(glTexImage2D(mTarget,
                                                  i,
                                                  formatGL.internalFormat,
                                                  mipMapLevelWidth,
                                                  mipMapLevelHeight,
                                                  0,
                                                  formatGL.format,
                                                  formatGL.type,
                                                  (GLubyte*)ptr));
                        OMAF_GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));

                        ptr += mipMapLevelSize;
                    }
                }
                else
                {
                    mNumMipmaps = (uint8_t)(floor(log2(max(desc.width, desc.height))) + 1);

                    OMAF_GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, desc.width));
                    OMAF_GL_CHECK(glTexImage2D(mTarget,
                                              0,
                                              formatGL.internalFormat,
                                              mWidth,
                                              mHeight,
                                              0,
                                              formatGL.format,
                                              formatGL.type,
                                              (GLubyte*)ptr));
                    OMAF_GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));

                    OMAF_GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
                }
            }
        }
        else if (desc.type == TextureType::TEXTURE_3D)
        {
            OMAF_ASSERT_NOT_IMPLEMENTED();
        }
        else if (desc.type == TextureType::TEXTURE_CUBE)
        {
            OMAF_ASSERT_NOT_IMPLEMENTED();
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }

        OMAF_GL_CHECK(glBindTexture(mTarget, currentTexture));

        return true;
    }

    return false;
}

bool_t TextureGL::createNative(const NativeTextureDesc& desc)
{
    GLuint handle = *(GLuint*)desc.nativeHandle;
    GLenum target = getTextureTypeGL(desc.type);

    mType = desc.type;
    mFormat = desc.format;
    mWidth = desc.width;
    mHeight = desc.height;
    mNumMipmaps = desc.numMipMaps;
    mNativeHandle = true;

    mHandle = handle;
    mTarget = target;

    return true;
}

void_t TextureGL::bind(uint16_t textureUnit)
{
    OMAF_ASSERT(mHandle != 0, "");

    OMAF_GL_CHECK(glActiveTexture(GL_TEXTURE0 + textureUnit));
    OMAF_GL_CHECK(glBindTexture(mTarget, mHandle));
}

void_t TextureGL::unbind(uint16_t textureUnit)
{
    OMAF_GL_CHECK(glActiveTexture(GL_TEXTURE0 + textureUnit));
    OMAF_GL_CHECK(glBindTexture(mTarget, 0));
}

void_t TextureGL::bindCompute(uint16_t stage, ComputeBufferAccess::Enum computeAccess)
{
#if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43) || GL_ARB_compute_shader

    OMAF_ASSERT(mHandle != 0, "");

    OMAF_ASSERT(mComputeAccess == computeAccess, "Compute buffer access doesn't match to resource description");

    GLenum access = getComputeBufferAccessGL(mComputeAccess);
    TextureFormatGL formatGL = getTextureFormatGL(mFormat);

    OMAF_GL_CHECK(glBindImageTexture((GLuint)stage, mHandle, 0, GL_FALSE, 0, access, formatGL.internalFormat));

#else

    OMAF_ASSERT(false, "Render backend doesn't support compute shaders");

#endif
}

void_t TextureGL::unbindCompute(uint16_t stage)
{
#if (OMAF_GRAPHICS_API_OPENGL_ES >= 31) || (OMAF_GRAPHICS_API_OPENGL >= 43) || GL_ARB_compute_shader

    GLenum access = getComputeBufferAccessGL(mComputeAccess);
    TextureFormatGL formatGL = getTextureFormatGL(mFormat);

    OMAF_GL_CHECK(glBindImageTexture((GLuint)stage, 0, 0, GL_FALSE, 0, access, formatGL.internalFormat));

#else

    OMAF_ASSERT(false, "Render backend doesn't support compute shaders");

#endif
}

void_t TextureGL::destroy()
{
    OMAF_ASSERT(mHandle != 0, "");

    if (!mNativeHandle)
    {
        OMAF_GL_CHECK(glDeleteTextures(1, &mHandle));
    }

    mHandle = 0;
    mTarget = 0;

    mComputeAccess = ComputeBufferAccess::INVALID;

    mWidth = 0;
    mHeight = 0;

    mNumMipmaps = 0;

    mFormat = TextureFormat::INVALID;
    mAddressModeU = TextureAddressMode::INVALID;
    mAddressModeV = TextureAddressMode::INVALID;
    mAddressModeW = TextureAddressMode::INVALID;
    mFilterMode = TextureFilterMode::INVALID;

    mNativeHandle = false;
}

OMAF_NS_END
