
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
#include "Graphics/NVRDebugTextLayer.h"

#include <Math/OMAFMath.h>

#include "Graphics/NVRDebugCharacterSet.h"

OMAF_NS_BEGIN

static const uint16_t glyphTexelWidth = 8;
static const uint16_t glyphTexelHeight = 16;

static const uint16_t glyphScreenWidth = 8;
static const uint16_t glyphScreenHeight = 16;

static const uint16_t fontAtlasWidth = 2048;
static const uint16_t fontAtlasHeight = 16;

static const uint16_t bufferGlypCount = 1024;
static const size_t scratchBufferSize = 1024 * 8;

DebugTextLayer::DebugTextLayer()
: mWidth(0)
, mHeight(0)
, mScale(0.0f)
, mVertexBuffer(VertexBufferID::Invalid)
, mTexture(TextureID::Invalid)
, mShader(ShaderID::Invalid)
{
}

DebugTextLayer::~DebugTextLayer()
{
    OMAF_ASSERT(mVertexBuffer == VertexBufferID::Invalid, "");
    OMAF_ASSERT(mTexture == TextureID::Invalid, "");
    OMAF_ASSERT(mShader == ShaderID::Invalid, "");
}

bool_t DebugTextLayer::isValid()
{
    return (mVertexBuffer != VertexBufferID::Invalid &&
            mTexture != TextureID::Invalid &&
            mShader != ShaderID::Invalid);
}

bool_t DebugTextLayer::create(MemoryAllocator& allocator, float32_t scale)
{
    OMAF_ASSERT(!isValid(), "Already initialized");

    mScale = scale;

    // Allocate scratch buffer
    if (!Buffer::allocate(allocator, mTextBuffer, scratchBufferSize))
    {
        return false;
    }

    // Create glyph vertex buffer
    VertexDeclaration vertexDeclaration = VertexDeclaration()
    .begin()
        .addAttribute("aPosition", VertexAttributeFormat::FLOAT32, 2, false)
        .addAttribute("aTextureCoord", VertexAttributeFormat::FLOAT32, 2, false)
        .addAttribute("aTextColor", VertexAttributeFormat::UINT8, 4, true)
        .addAttribute("aBackgroundColor", VertexAttributeFormat::UINT8, 4, true)
    .end();

    size_t numVertices = bufferGlypCount * 6;

    if (!mVertexStream.allocate(numVertices))
    {
        destroy(allocator);

        return false;
    }

    VertexBufferDesc vertexBufferDesc;
    vertexBufferDesc.declaration = vertexDeclaration;
    vertexBufferDesc.access = BufferAccess::DYNAMIC;
    vertexBufferDesc.data = OMAF_NULL;
    vertexBufferDesc.size = OMAF_SIZE_OF(GlyphVertex) * numVertices;

    mVertexBuffer = RenderBackend::createVertexBuffer(vertexBufferDesc);

    if (mVertexBuffer == VertexBufferID::Invalid)
    {
        destroy(allocator);

        return false;
    }

    // Create font atlas texture
    TextureDesc textureDesc;
    textureDesc.type = TextureType::TEXTURE_2D;
    textureDesc.width = fontAtlasWidth;
    textureDesc.height = fontAtlasHeight;
    textureDesc.numMipMaps = 1;
    textureDesc.generateMipMaps = false;
    textureDesc.format = TextureFormat::R8;
    textureDesc.samplerState.addressModeU = TextureAddressMode::CLAMP;
    textureDesc.samplerState.addressModeV = TextureAddressMode::CLAMP;
    textureDesc.samplerState.filterMode = TextureFilterMode::POINT;
    textureDesc.data = vga8x16;
    textureDesc.size = OMAF_SIZE_OF(vga8x16);

    mTexture = RenderBackend::createTexture(textureDesc);

    if (mTexture == TextureID::Invalid)
    {
        destroy(allocator);

        return false;
    }

    // Create shader
    RendererType::Enum backendType = RenderBackend::getRendererType();

    if (backendType == RendererType::OPENGL || backendType == RendererType::OPENGL_ES)
    {
        static const char_t* vs =
#if OMAF_GRAPHICS_API_OPENGL
        "#version 150\n"
#elif OMAF_GRAPHICS_API_OPENGL_ES
        "#version 100\n"
#endif
        "#if (__VERSION__ < 130)\n"
        "   #define in attribute\n"
        "   #define out varying\n"
        "#endif\n"
        "\n"
        "#ifdef GL_ES\n"
        "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
        "    precision highp float;\n"
        "#else\n"
        "    precision mediump float;\n"
        "#endif\n"
        "#else\n"
        "   #define lowp\n"
        "   #define mediump\n"
        "   #define highp\n"
        "#endif\n"
        "\n"
        "in vec4 aPosition;\n"
        "in vec2 aTextureCoord;\n"
        "in vec4 aTextColor;\n"
        "in vec4 aBackgroundColor;\n"
        "\n"
        "out vec2 vTextureCoord;\n"
        "out vec4 vTextColor;\n"
        "out vec4 vBackgroundColor;\n"
        "\n"
        "uniform mat4 uModelViewProjection;\n"
        "\n"
        "void main()\n"
        "{\n"
        "	vTextureCoord = aTextureCoord;\n"
        "\n"
        "	vTextColor = aTextColor;\n"
        "	vBackgroundColor = aBackgroundColor;\n"
        "\n"
        "	gl_Position = uModelViewProjection * aPosition;\n"
        "}\n";

        static const char_t* fs =
#if OMAF_GRAPHICS_API_OPENGL
        "#version 150\n"
#elif OMAF_GRAPHICS_API_OPENGL_ES
        "#version 100\n"
#endif
        "\n"
        "#ifdef GL_ES\n"
        "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
        "    precision highp float;\n"
        "#else\n"
        "    precision mediump float;\n"
        "#endif\n"
        "#else\n"
        "   #define lowp\n"
        "   #define mediump\n"
        "   #define highp\n"
        "#endif\n"
        "\n"
        "#if (__VERSION__ < 130)\n"
        "   #define in varying\n"
        "#else\n"
        "   out lowp vec4 fragmentColor;\n"
        "#endif\n"
        "\n"
        "#if (__VERSION__ < 150)\n"
        "   #define texture texture2D\n"
        "#endif\n"
        "\n"
        "in vec2 vTextureCoord;\n"
        "\n"
        "in vec4 vTextColor;\n"
        "in vec4 vBackgroundColor;\n"
        "\n"
        "uniform sampler2D uTextureSampler;\n"
        "\n"
        "void main()\n"
        "{\n"
        "#if (__VERSION__ < 130)\n"
        "	gl_FragColor = mix(vBackgroundColor, vTextColor, texture(uTextureSampler, vTextureCoord).r);\n"
        "#else\n"
        "   fragmentColor = mix(vBackgroundColor, vTextColor, texture(uTextureSampler, vTextureCoord).r);\n"
        "#endif\n"
        "}\n";

        mShader = RenderBackend::createShader(vs, fs);
        OMAF_ASSERT(mShader != ShaderID::Invalid, "");
    }
    else if (backendType == RendererType::D3D11 || backendType == RendererType::D3D12)
    {
        static const char_t* shader =
        "matrix uModelViewProjection;\n"
        "\n"
        "Texture2D Texture0;\n"
        "SamplerState Sampler0;\n"
        "\n"
        "struct VS_INPUT\n"
        "{\n"
        "	float2 Position : POSITION;\n"
        "	float2 TexCoord0 : TEXCOORD0;\n"
        "	uint TextColor : COLOR0;\n"
        "	uint BackgroundColor : COLOR1;\n"
        "};\n"
        "\n"
        "struct PS_INPUT\n"
        "{\n"
        "	float4 Position : SV_POSITION;\n"
        "	float4 TextColor : COLOR0;\n"
        "	float4 BackgroundColor : COLOR1;\n"
        "	float2 TexCoord0 : TEXCOORD0;\n"
        "};\n"
        "\n"
        "float4 unpackColorUintToFloat4(uint color)\n"
        "{\n"
        "    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
        "    result.x = ((color >> 24) & 0xFF) / 255.0f;\n"
        "    result.y = ((color >> 16) & 0xFF) / 255.0f;\n"
        "    result.z = ((color >> 8) & 0xFF) / 255.0f;\n"
        "    result.w = (color & 0xFF) / 255.0f;\n"
        "\n"
        "    return result;\n"
        "}\n"
        "\n"
        "PS_INPUT mainVS(VS_INPUT input)\n"
        "{\n"
        "	float4 position = float4(input.Position, 0.0f, 1.0f);\n"
        ""
        "	PS_INPUT output = (PS_INPUT)0;\n"
        "	output.Position = mul(uModelViewProjection, position);\n"
        "	output.TextColor = unpackColorUintToFloat4(input.TextColor); \n"
        "	output.BackgroundColor = unpackColorUintToFloat4(input.BackgroundColor);\n"
        "	output.TexCoord0 = float2(input.TexCoord0.x, input.TexCoord0.y);\n"
        "\n"
        "	return output;\n"
        "}"
        "\n"
        "float4 mainPS(PS_INPUT input) : SV_TARGET\n"
        "{\n"
        "	float4 color = lerp(input.BackgroundColor, input.TextColor, Texture0.Sample(Sampler0, input.TexCoord0).r); \n"
        "\n"
        "	return color;"
        "}\n";

        mShader = RenderBackend::createShader(shader, shader);
        OMAF_ASSERT(mShader != ShaderID::Invalid, "");
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();
    }

    mModelViewProjectionConstant = RenderBackend::createShaderConstant(mShader, "uModelViewProjection", ShaderConstantType::MATRIX44);
    OMAF_ASSERT(mModelViewProjectionConstant != ShaderConstantID::Invalid, "");

    mTextureSamplerConstant = RenderBackend::createShaderConstant(mShader, "uTextureSampler", ShaderConstantType::SAMPLER_2D);
    OMAF_ASSERT(mTextureSamplerConstant != ShaderConstantID::Invalid, "");

    return true;
}

void_t DebugTextLayer::destroy(MemoryAllocator& allocator)
{
    Buffer::free(allocator, mTextBuffer);

    mVertexStream.free();

    RenderBackend::destroyVertexBuffer(mVertexBuffer);
    mVertexBuffer = VertexBufferID::Invalid;

    RenderBackend::destroyTexture(mTexture);
    mTexture = TextureID::Invalid;

    RenderBackend::destroyShader(mShader);
    mShader = ShaderID::Invalid;

    RenderBackend::destroyShaderConstant(mModelViewProjectionConstant);
    mModelViewProjectionConstant = ShaderConstantID::Invalid;

    RenderBackend::destroyShaderConstant(mTextureSamplerConstant);
    mTextureSamplerConstant = ShaderConstantID::Invalid;
}

void_t DebugTextLayer::clear()
{
    mVertexStream.clear();
}

void_t DebugTextLayer::setWindow(uint32_t width, uint32_t height)
{
    mWidth = width;
    mHeight = height;
}

void_t DebugTextLayer::debugPrintFormatVar(uint16_t x, uint16_t y, uint32_t textColor, uint32_t backgroundColor, const char_t* format, va_list args)
{
    OMAF_ASSERT(isValid(), "Not initialized");

    // Calculate glyph positions
    size_t stringLength = StringFormatVar(mTextBuffer.data, mTextBuffer.capacity, format, args);

    const float32_t startPositionX = (float32_t)(x * glyphScreenWidth);
    const float32_t startPositionY = (float32_t)(y * glyphScreenHeight);

    float32_t positionX = startPositionX;
    float32_t positionY = startPositionY;

    const float32_t horizontalTabSize = (float32_t)(glyphScreenWidth * 4);
    const float32_t verticalTabSize = (float32_t)(glyphScreenHeight * 4);

    const float32_t lineHeight = (float32_t)glyphScreenHeight;

    // Bind resources
    RenderBackend::bindVertexBuffer(mVertexBuffer);
    RenderBackend::bindTexture(mTexture, 0);
    RenderBackend::bindShader(mShader);
    
    // Set shader constants
    Matrix44 scale = makeScale(mScale, mScale, 1.0f);
    Matrix44 projection = makeOrthographic(0.0f, (float32_t)mWidth, (float32_t)mHeight, 0.0f, -1.0f, 1.0f);
   
    Matrix44 mvp = projection * scale;
    
    RenderBackend::setShaderConstant(mShader, mModelViewProjectionConstant, &mvp);
    
    uint32_t textureUnit = 0;
    RenderBackend::setShaderConstant(mShader, mTextureSamplerConstant, &textureUnit);
    
    // Set PSOs
    RasterizerState rasterizerState;
    rasterizerState.cullMode = CullMode::NONE;
    rasterizerState.frontFace = FrontFace::CCW;
    
    BlendState blendState;
    blendState.blendEnabled = true;
    blendState.blendFunctionSrcRgb = BlendFunction::ONE;
    blendState.blendFunctionDstRgb = BlendFunction::ONE_MINUS_SRC_ALPHA;
    blendState.blendFunctionSrcAlpha = BlendFunction::ONE;
    blendState.blendFunctionDstAlpha = BlendFunction::ONE_MINUS_SRC_ALPHA;
    
    DepthStencilState depthStencilState;
    depthStencilState.depthTestEnabled = false;
    depthStencilState.stencilTestEnabled = false;
    
    RenderBackend::setRasterizerState(rasterizerState);
    RenderBackend::setBlendState(blendState);
    RenderBackend::setDepthStencilState(depthStencilState);
    
    for (size_t i = 0; i < stringLength; ++i)
    {
        char_t character = mTextBuffer.data[i];

        // Handle control characters
        if (character == '\t')
        {
            positionX += horizontalTabSize;

            continue;
        }

        if (character == '\n')
        {
            positionY += lineHeight;
            positionX = startPositionX;

            continue;
        }

        if (character == '\v')
        {
            positionY += verticalTabSize;
            positionX = startPositionX;

            continue;
        }

        uint16_t glyphIndex = mTextBuffer.data[i];

        // Skip control codes
        if (glyphIndex < 32)
        {
            continue;
        }

        float32_t texelWidth = 1.0f / (float32_t)fontAtlasWidth;
        float32_t texelHeight = 1.0f / (float32_t)fontAtlasHeight;

        bool_t enableTexelOffset = true;

        float32_t halfTexelWidth = enableTexelOffset ? 1.0f * texelWidth : 0.0f;
        float32_t halfTexelHeight = enableTexelOffset ? 1.0f * texelHeight : 0.0f;

        float32_t left = ((float32_t)glyphIndex * glyphTexelWidth * texelWidth) - halfTexelWidth;
        float32_t right = ((float32_t)(glyphIndex + 1) * glyphTexelWidth * texelWidth) - halfTexelWidth;
        float32_t bottom = 0.0f - halfTexelHeight;
        float32_t top = 1.0f - halfTexelHeight;

        // a c
        // b d
        GlyphVertex glyph[6] = 
        {
            (positionX + 0.0f),                        (positionY + 0.0f),                         left, top,     textColor, backgroundColor, // a
            (positionX + 0.0f),                        (positionY + (float32_t)glyphScreenHeight), left, bottom,  textColor, backgroundColor, // b
            (positionX + (float32_t)glyphScreenWidth), (positionY + 0.0f),                         right, top,    textColor, backgroundColor, // c

            (positionX + (float32_t)glyphScreenWidth), (positionY + 0.0f),                         right, top,    textColor, backgroundColor, // c
            (positionX + 0.0f),                        (positionY + (float32_t)glyphScreenHeight), left, bottom,  textColor, backgroundColor, // b
            (positionX + (float32_t)glyphScreenWidth), (positionY + (float32_t)glyphScreenHeight), right, bottom, textColor, backgroundColor, // d
        };

        mVertexStream.add(glyph, 6);

        positionX += (float32_t)glyphScreenWidth;
        
        // Check if stream cannot handle next glyph and submit draw call
        if (mVertexStream.getCapacity() - mVertexStream.getCount() < 6)
        {
            RenderBackend::updateVertexBuffer(mVertexBuffer, 0, mVertexStream.buffer);
            RenderBackend::draw(PrimitiveType::TRIANGLE_LIST, 0, (uint32_t)mVertexStream.getCount());
            
            mVertexStream.clear();
        }
    }

    if (mVertexStream.getCount() > 0)
    {
        RenderBackend::updateVertexBuffer(mVertexBuffer, 0, mVertexStream.buffer);
        RenderBackend::draw(PrimitiveType::TRIANGLE_LIST, 0, (uint32_t)mVertexStream.getCount());
        
        mVertexStream.clear();
    }
}

OMAF_NS_END
