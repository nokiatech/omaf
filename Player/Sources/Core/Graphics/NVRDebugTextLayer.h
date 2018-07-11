
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
#include "Graphics/NVRRenderBackend.h"
#include "Graphics/NVRStreamBuffer.h"

#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRNew.h"

OMAF_NS_BEGIN

class DebugTextLayer
{
    public:

        DebugTextLayer();
        ~DebugTextLayer();

        bool_t create(MemoryAllocator& allocator, float32_t scale = 1.0f);
        void_t destroy(MemoryAllocator& allocator);

        bool_t isValid();
        void_t clear();
    
        void_t setWindow(uint32_t width, uint32_t height);

        void_t debugPrintFormatVar(uint16_t x, uint16_t y, uint32_t textColor, uint32_t backgroundColor, const char_t* format, va_list args);

    private:

        uint32_t mWidth;
        uint32_t mHeight;

        float32_t mScale;

        struct Buffer
        {
            char_t* data;
            size_t capacity;

            Buffer()
            : data(OMAF_NULL)
            , capacity(0)
            {
            }

            ~Buffer()
            {
                OMAF_ASSERT(data == OMAF_NULL, "");
                OMAF_ASSERT(capacity == OMAF_NULL, "");
            }

            static bool_t allocate(MemoryAllocator& allocator, Buffer& buffer, size_t size)
            {
                OMAF_ASSERT(buffer.data == OMAF_NULL, "");
                OMAF_ASSERT(buffer.capacity == 0, "");

                buffer.data = (char_t*)OMAF_ALLOC(allocator, size);

                if (buffer.data == OMAF_NULL)
                {
                    return false;
                }

                buffer.capacity = size;

                return true;
            }

            static void_t free(MemoryAllocator& allocator, Buffer& buffer)
            {
                if (buffer.data != OMAF_NULL && buffer.capacity != 0)
                {
                    OMAF_FREE(allocator, buffer.data);
                    buffer.data = OMAF_NULL;

                    buffer.capacity = 0;
                }
            }
        };

        Buffer mTextBuffer;

        struct GlyphVertex
        {
            float32_t x;
            float32_t y;
            float32_t u;
            float32_t v;
            uint32_t textColor;
            uint32_t backgroundColor;
        };

        StreamBuffer<GlyphVertex> mVertexStream;

        VertexBufferID mVertexBuffer;
        TextureID mTexture;
        ShaderID mShader;

        ShaderConstantID mModelViewProjectionConstant;
        ShaderConstantID mTextureSamplerConstant;
};

OMAF_NS_END
