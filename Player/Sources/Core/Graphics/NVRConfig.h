
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

OMAF_NS_BEGIN

#ifndef OMAF_MAX_VERTEX_BUFFERS
    
    #define OMAF_MAX_VERTEX_BUFFERS 4096
    
#endif
    
#ifndef OMAF_MAX_INDEX_BUFFERS
    
    #define OMAF_MAX_INDEX_BUFFERS 4096
    
#endif
    
#ifndef OMAF_MAX_VERTEX_DECLARATIONS
    
    #define OMAF_MAX_VERTEX_DECLARATIONS 512
    
#endif
    
#ifndef OMAF_MAX_SHADERS
    
    #define OMAF_MAX_SHADERS 512
    
#endif
    
#ifndef OMAF_MAX_SHADER_CONSTANTS
    
    #define OMAF_MAX_SHADER_CONSTANTS 1024
    
#endif

#ifndef OMAF_MAX_TEXTURES
    
    #define OMAF_MAX_TEXTURES 4096
    
#endif
    
#ifndef OMAF_MAX_RENDER_TARGETS
    
    #define OMAF_MAX_RENDER_TARGETS 512
    
#endif
    
#ifndef OMAF_MAX_RENDER_TARGET_ATTACHMENTS
    
    #define OMAF_MAX_RENDER_TARGET_ATTACHMENTS 8
    
#endif
    
#ifndef OMAF_MAX_TEXTURE_UNITS
    
    #define OMAF_MAX_TEXTURE_UNITS 32
    
#endif

#ifndef OMAF_MAX_SHADER_ATTRIBUTES

    #define OMAF_MAX_SHADER_ATTRIBUTES 16

#endif

#ifndef OMAF_MAX_SHADER_UNIFORMS

    #define OMAF_MAX_SHADER_UNIFORMS 32

#endif

#ifndef OMAF_MAX_DEBUG_MARKER_NAME

    #define OMAF_MAX_DEBUG_MARKER_NAME 256

#endif

#ifndef OMAF_MAX_COMPUTE_BINDINGS

    #define OMAF_MAX_COMPUTE_BINDINGS 8

#endif

OMAF_NS_END
