
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
#include "Graphics/NVRBufferAccess.h"
#include "Graphics/NVRIndexBufferFormat.h"
#include "Graphics/NVRComputeBufferAccess.h"

OMAF_NS_BEGIN

class IndexBufferGL
{
    public:
        
        IndexBufferGL();
        ~IndexBufferGL();
        
        GLuint getHandle() const;
        
        bool_t create(BufferAccess::Enum access, IndexBufferFormat::Enum format, const void_t* data = OMAF_NULL, size_t bytes = 0);
        void_t bufferData(size_t offset, const void_t* data, size_t bytes);
        void_t destroy();
        
        void_t bind();
        void_t unbind();

        void_t bindCompute(uint16_t stage, ComputeBufferAccess::Enum computeAccess);
        void_t unbindCompute(uint16_t stage);
        
        GLsizei getNumIndices() const;
        GLenum getFormat() const;
        
    private:
        
        OMAF_NO_ASSIGN(IndexBufferGL);
        OMAF_NO_COPY(IndexBufferGL);
        
    private:
        
        GLuint mHandle;
        GLsizei mBytes;

        ComputeBufferAccess::Enum mComputeAccess;
        
        IndexBufferFormat::Enum mFormat;
};

OMAF_NS_END
