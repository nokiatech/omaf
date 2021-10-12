
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
#include "Foundation/NVRAssert.h"
#include "NVRNamespace.h"
#include "Platform/OMAFCompiler.h"

#include "Graphics/NVRPrimitiveType.h"

OMAF_NS_BEGIN

// https://www.khronos.org/opengles/
// https://developer.apple.com/opengl-es/

// https://www.khronos.org/opengl/
// https://developer.apple.com/opengl/

// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476080(v=vs.85).aspx
// https://msdn.microsoft.com/en-us/library/windows/desktop/dn903821(v=vs.85).aspx

// https://developer.apple.com/metal/

// https://www.khronos.org/vulkan/

namespace RendererType
{
    enum Enum
    {
        INVALID = -1,

        /*
         Null test backend
        */

        NIL,

        /*
        OpenGL ES 2.0
        OpenGL ES 3.0
        */

        OPENGL_ES,

        /*
         OpenGL 3.0
         OpenGL 3.3

         OpenGL 4.0
         OpenGL 4.1, Full API compatibility with OpenGL ES 2.0 via ARB_ES2_compatibility
         OpenGL 4.2
         OpenGL 4.3, Full compatibility with OpenGL ES 3.0 APIs
         OpenGL 4.4
         OpenGL 4.5, OpenGL ES 3.1 API and shader compatibility
         */

        OPENGL,

        /*
         D3D11 backend with following feature levels:
          - 11_1
          - 11_0
          - 10_1
          - 10_0
          - 9_3
          - 9_2
          - 9_1
        */

        D3D11,

        /*
         D3D12 backend with following feature levels:
         - 12_2
         - 12_1
         - 12_0
         */

        D3D12,

        METAL,

        VULKAN,

        COUNT
    };

    OMAF_INLINE const char_t* toString(Enum value)
    {
        switch (value)
        {
        case INVALID:
            return "Invalid";
        case NIL:
            return "NULL / NIL";
        case OPENGL_ES:
            return "OpenGL ES";
        case OPENGL:
            return "OpenGL";
        case D3D11:
            return "D3D11";
        case D3D12:
            return "D3D12";
        case METAL:
            return "Metal";
        case VULKAN:
            return "Vulkan";

        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
        }

        return OMAF_NULL;
    }
}  // namespace RendererType

OMAF_NS_END
