
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
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <gl/gl.h>
#include <gl/glext.h>
#include <wgl/wglext.h>
#include <stdio.h>

#pragma comment(lib,"opengl32.lib")
#include "NVREssentials.h"
#include "Foundation/NVRLogger.h"
OMAF_NS_BEGIN
OMAF_LOG_ZONE("NVRGLFunctions")
#define declare(a,b) a b;
#include "Graphics/OpenGL/Windows/NVRGLFunctions.inc"
#undef declare


void InitializeGLFunctions()
{    
    #define declare(a,b) if (b==OMAF_NULL) {*((void**)&b)=(void*)wglGetProcAddress(#b);}
    #include "Graphics/OpenGL/Windows/NVRGLFunctions.inc"
    #undef declare

    //Log out missing funcs..
    bool fail = false;

    #define declare(a,b) if (b==OMAF_NULL) {OMAF_LOG_E("OpenGL extension "#b" is missing\n");fail=true;}
    #include "Graphics/OpenGL/Windows/NVRGLFunctions.inc"
    #undef declare

}
OMAF_NS_END