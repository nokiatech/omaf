
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

#include "Graphics/NVRBlendEquation.h"
#include "Graphics/NVRBlendFunction.h"
#include "Graphics/NVRBlendState.h"
#include "Graphics/NVRBufferAccess.h"
#include "Graphics/NVRColor.h"
#include "Graphics/NVRColorMask.h"
#include "Graphics/NVRColorSpace.h"
#include "Graphics/NVRComputeBufferAccess.h"
#include "Graphics/NVRCullMode.h"
#include "Graphics/NVRDebugMode.h"
#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRDepthFunction.h"
#include "Graphics/NVRDepthMask.h"
#include "Graphics/NVRDepthStencilState.h"
#include "Graphics/NVRDiscardMask.h"
#include "Graphics/NVRFillMode.h"
#include "Graphics/NVRFrontFace.h"
#include "Graphics/NVRGraphicsAPIDetection.h"
#include "Graphics/NVRHandles.h"
#include "Graphics/NVRIRenderContext.h"
#include "Graphics/NVRPrimitiveType.h"
#include "Graphics/NVRRasterizerState.h"
#include "Graphics/NVRRenderBackend.h"
#include "Graphics/NVRRendererType.h"
#include "Graphics/NVRResourceDescriptors.h"
#include "Graphics/NVRScissors.h"
#include "Graphics/NVRShaderConstantType.h"
#include "Graphics/NVRStencilFunction.h"
#include "Graphics/NVRStencilOperation.h"
#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRTextureFilterMode.h"
#include "Graphics/NVRTextureFormat.h"
#include "Graphics/NVRTextureType.h"
#include "Graphics/NVRVertexAttributeFormat.h"
#include "Graphics/NVRVertexDeclaration.h"
#include "Graphics/NVRViewport.h"

#if OMAF_GRAPHICS_API_OPENGL_ES || OMAF_GRAPHICS_API_OPENGL

#include "Graphics/OpenGL/NVRGLContext.h"

#endif

/*
 *
 *  NVR SDK uses normalized texture coordinate space, where origin (0, 0) exists in bottom-left corner of the space.
 *
 *     0.0    ->    1.0
 * 1.0 ----------------
 *     |              |
 *     |              |
 *     |              |
 *     |              |
 *     |              |
 * 0.0 ----------------
 *
 */
