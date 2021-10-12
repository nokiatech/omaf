
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

#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRGraphicsAPIDetection.h"

// OpenGL / OpenGL ES API import
#if OMAF_GRAPHICS_API_OPENGL_ES || OMAF_GRAPHICS_API_OPENGL

#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_ANDROID

#define OMAF_OPENGL_API_IMPORT 1

#endif

#endif

#if OMAF_PLATFORM_WINDOWS

#include "Graphics/OpenGL/Windows/NVRGLFunctions.h"

#endif

// ------------------------------------------------------------
// S3TC extensions
// ------------------------------------------------------------

#ifndef GL_EXT_texture_compression_s3tc
#ifndef GL_OES_texture_compression_S3TC
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#endif

#ifndef GL_EXT_texture_compression_dxt1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#ifndef GL_EXT_texture_compression_dxt3
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_EXT_texture_compression_dxt5
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

// ------------------------------------------------------------
// ATI / AMD extensions
// ------------------------------------------------------------

#ifndef GL_AMD_compressed_3DC_texture
#define GL_3DC_X_AMD 0x87F9
#define GL_3DC_XY_AMD 0x87FA
#endif

#ifndef GL_AMD_compressed_ATC_texture
#ifndef GL_ATI_texture_compression_atitc
#define GL_ATC_RGB_AMD 0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD 0x8C93
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD 0x87EE
#endif
#endif

#ifndef WGL_AMD_gpu_association
#define WGL_GPU_VENDOR_AMD 0x1F00
#define WGL_GPU_RENDERER_STRING_AMD 0x1F01
#define WGL_GPU_OPENGL_VERSION_STRING_AMD 0x1F02
#define WGL_GPU_FASTEST_TARGET_GPUS_AMD 0x21A2
#define WGL_GPU_RAM_AMD 0x21A3
#define WGL_GPU_CLOCK_AMD 0x21A4
#define WGL_GPU_NUM_PIPES_AMD 0x21A5
#define WGL_GPU_NUM_SIMD_AMD 0x21A6
#define WGL_GPU_NUM_RB_AMD 0x21A7
#define WGL_GPU_NUM_SPI_AMD 0x21A8
#endif

// ------------------------------------------------------------
// NVidia extensions
// ------------------------------------------------------------

#ifndef GL_EXT_texture_compression_latc
#ifndef GL_NV_texture_compression_latc
#define GL_COMPRESSED_LUMINANCE_LATC1_EXT 0x8C70
#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT 0x8C71
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT 0x8C72
#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT 0x8C73
#endif
#endif

#ifndef GL_NVX_gpu_memory_info
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX 0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX 0x904B
#endif

// ------------------------------------------------------------
// Imagination Technologies extensions
// ------------------------------------------------------------

#ifndef GL_IMG_texture_compression_pvrtc
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif

#ifndef GL_IMG_texture_compression_pvrtc2
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG 0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG 0x9138
#endif

// ------------------------------------------------------------
// OES extensions
// ------------------------------------------------------------

#ifndef GL_OES_compressed_ETC1_RGB8_texture
#define GL_ETC1_RGB8_OES 0x8D64
#endif

#ifndef GL_OES_EGL_image_external
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#define GL_SAMPLER_EXTERNAL_OES 0x8D66
#define GL_TEXTURE_BINDING_EXTERNAL_OES 0x8D67
#define GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES 0x8D68
#endif

// ------------------------------------------------------------
// KHR extensions
// ------------------------------------------------------------
#ifndef GL_KHR_debug
#define GL_SAMPLER 0x82E6
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_KHR 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION_KHR 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM_KHR 0x8245
#define GL_DEBUG_SOURCE_API_KHR 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER_KHR 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY_KHR 0x8249
#define GL_DEBUG_SOURCE_APPLICATION_KHR 0x824A
#define GL_DEBUG_SOURCE_OTHER_KHR 0x824B
#define GL_DEBUG_TYPE_ERROR_KHR 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY_KHR 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE_KHR 0x8250
#define GL_DEBUG_TYPE_OTHER_KHR 0x8251
#define GL_DEBUG_TYPE_MARKER_KHR 0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP_KHR 0x8269
#define GL_DEBUG_TYPE_POP_GROUP_KHR 0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION_KHR 0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH_KHR 0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH_KHR 0x826D
#define GL_BUFFER_KHR 0x82E0
#define GL_SHADER_KHR 0x82E1
#define GL_PROGRAM_KHR 0x82E2
#define GL_VERTEX_ARRAY_KHR 0x8074
#define GL_QUERY_KHR 0x82E3
#define GL_SAMPLER_KHR 0x82E6
#define GL_MAX_LABEL_LENGTH_KHR 0x82E8
#define GL_MAX_DEBUG_MESSAGE_LENGTH_KHR 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES_KHR 0x9144
#define GL_DEBUG_LOGGED_MESSAGES_KHR 0x9145
#define GL_DEBUG_SEVERITY_HIGH_KHR 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_KHR 0x9147
#define GL_DEBUG_SEVERITY_LOW_KHR 0x9148
#define GL_DEBUG_OUTPUT_KHR 0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT_KHR 0x00000002
#define GL_STACK_OVERFLOW_KHR 0x0503
#define GL_STACK_UNDERFLOW_KHR 0x0504
#endif

//////////////////// Definitions for compatibility ////////////////////

// OpenGL
#ifndef GL_POINT
#define GL_POINT 0x1B00
#endif

#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif

#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#ifndef GL_RGBA16
#define GL_RGBA16 0x805B
#endif

#ifndef GL_R16
#define GL_R16 0x822A
#endif

#ifndef GL_R16_SNORM
#define GL_R16_SNORM 0x8F98
#endif

#ifndef GL_RG16
#define GL_RG16 0x822C
#endif

#ifndef GL_RG16_SNORM
#define GL_RG16_SNORM 0x8F99
#endif

#ifndef GL_RGBA16_SNORM
#define GL_RGBA16_SNORM 0x8F9B
#endif

#ifndef GL_DEPTH_COMPONENT32
#define GL_DEPTH_COMPONENT32 0x81A7
#endif

#ifndef GL_STENCIL_INDEX1
#define GL_STENCIL_INDEX1 0x8D46
#endif

#ifndef GL_STENCIL_INDEX4
#define GL_STENCIL_INDEX4 0x8D47
#endif

#ifndef GL_STENCIL_INDEX8
#define GL_STENCIL_INDEX8 0x8D48
#endif

#ifndef GL_STENCIL_INDEX16
#define GL_STENCIL_INDEX16 0x8D49
#endif

#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x1901
#endif

// OpenGL ES 3
#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif

#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif

#ifndef GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#endif

// General
#ifndef GL_BGRA8
#define GL_BGRA8 GL_BGRA
#endif

#ifndef GL_BGR8
#define GL_BGR8 GL_BGR
#endif

#ifndef GL_RED
#define GL_RED 0x1903
#endif

#ifndef GL_R16F
#define GL_R16F 0x822D
#endif

#ifndef GL_R32F
#define GL_R32F 0x822E
#endif

#ifndef GL_RG
#define GL_RG 0x8227
#endif

//////////////////// Extension mapping ////////////////////

#ifdef GL_OES_texture_half_float
#undef GL_HALF_FLOAT

#define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#endif

#ifdef GL_EXT_blend_minmax
#undef GL_MIN
#undef GL_MAX

#define GL_MIN GL_MIN_EXT
#define GL_MAX GL_MAX_EXT
#endif

#if GL_EXT_texture_rg
#undef GL_RED
#undef GL_RG
#undef GL_R8
#undef GL_RG8

#define GL_RED GL_RED_EXT
#define GL_RG GL_RG_EXT
#define GL_R8 GL_R8_EXT
#define GL_RG8 GL_RG8_EXT
#endif

#if GL_EXT_texture_storage
#undef GL_TEXTURE_IMMUTABLE_FORMAT
#undef GL_ALPHA8
#undef GL_LUMINANCE8
#undef GL_LUMINANCE8_ALPHA8
#undef GL_BGRA8
#undef GL_RGBA32F
#undef GL_RGB32F
#undef GL_RG32F
#undef GL_R32F
#undef GL_ALPHA32F
#undef GL_LUMINANCE32F
#undef GL_LUMINANCE_ALPHA32F
#undef GL_ALPHA16F
#undef GL_LUMINANCE16F
#undef GL_LUMINANCE_ALPHA16F
#undef GL_DEPTH_COMPONENT32
#undef GL_RGB_RAW_422

#define GL_TEXTURE_IMMUTABLE_FORMAT GL_TEXTURE_IMMUTABLE_FORMAT_EXT
#define GL_ALPHA8 GL_ALPHA8_EXT
#define GL_LUMINANCE8 GL_LUMINANCE8_EXT
#define GL_LUMINANCE8_ALPHA8 GL_LUMINANCE8_ALPHA8_EXT
#define GL_BGRA8 GL_BGRA8_EXT
#define GL_RGBA32F GL_RGBA32F_EXT
#define GL_RGB32F GL_RGB32F_EXT
#define GL_RG32F GL_RG32F_EXT
#define GL_R32F GL_R32F_EXT
#define GL_ALPHA32F GL_ALPHA32F_EXT
#define GL_LUMINANCE32F GL_LUMINANCE32F_EXT
#define GL_LUMINANCE_ALPHA32F GL_LUMINANCE_ALPHA32F_EXT
#define GL_ALPHA16F GL_ALPHA16F_EXT
#define GL_LUMINANCE16F GL_LUMINANCE16F_EXT
#define GL_LUMINANCE_ALPHA16F GL_LUMINANCE_ALPHA16F_EXT
#define GL_DEPTH_COMPONENT32 GL_DEPTH_COMPONENT32_OES
#define GL_RGB_RAW_422 GL_RGB_RAW_422_APPLE
#endif

#ifdef GL_OES_packed_depth_stencil
#undef GL_DEPTH_STENCIL
#undef GL_UNSIGNED_INT_24_8
#undef GL_DEPTH24_STENCIL8

#define GL_DEPTH_STENCIL GL_DEPTH_STENCIL_OES
#define GL_UNSIGNED_INT_24_8 GL_UNSIGNED_INT_24_8_OES
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#endif

#ifdef GL_OES_depth24
#undef GL_DEPTH_COMPONENT24

#define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
#endif

#ifdef GL_OES_depth32
#undef GL_DEPTH_COMPONENT32

#define GL_DEPTH_COMPONENT32 GL_DEPTH_COMPONENT32_OES
#endif
