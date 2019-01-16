
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
#include "NVRIdentityRenderer.h"
#include "Graphics/NVRRenderBackend.h"

OMAF_NS_BEGIN
IdentityRenderer::IdentityRenderer()
    : mDirtyShader(true)
{
}

IdentityRenderer::~IdentityRenderer()
{
    destroy();
}

void_t IdentityRenderer::createImpl()
{
    VertexDeclaration vertexDeclaration = VertexDeclaration()
        .begin()
        .addAttribute("aPosition", VertexAttributeFormat::FLOAT32, 2, false)
        .addAttribute("aTextureCoord", VertexAttributeFormat::FLOAT32, 2, false)
        .end();

    VertexBufferDesc vertexBufferDesc;
    vertexBufferDesc.declaration = vertexDeclaration;
    vertexBufferDesc.access = BufferAccess::STATIC;

    Vertex verts[] =
    {
        { -1.f,-1.f, 0.f, 0.f },
        {  1.f,-1.f, 1.f, 0.f },
        {  1.f, 1.f, 1.f, 1.f },
        { -1.f,-1.f, 0.f, 0.f },
        {  1.f, 1.f, 1.f, 1.f },
        { -1.f, 1.f, 0.f, 1.f }
    };

    vertexBufferDesc.data = verts;
    vertexBufferDesc.size = 6 * OMAF_SIZE_OF(Vertex);

    mVertexBuffer = RenderBackend::createVertexBuffer(vertexBufferDesc);

    mDirtyShader = true;
}

void_t IdentityRenderer::destroyImpl()
{
    RenderBackend::destroyVertexBuffer(mVertexBuffer);
    mVertexBuffer = VertexBufferID::Invalid;

    mOpaqueShader.destroy();
}

ProjectionType::Enum IdentityRenderer::getType() const
{
    return ProjectionType::IDENTITY;
}

void_t IdentityRenderer::render(const HeadTransform& headTransform, const RenderSurface& renderSurface, const CoreProviderSources& sources, const TextureID& renderMask, const RenderingParameters& renderingParameters)
{
    OMAF_ASSERT(sources.getSize() == 1, "Supports only one input source");
    OMAF_ASSERT(sources.at(0)->type == SourceType::IDENTITY || sources.at(0)->type == SourceType::IDENTITY_AUXILIARY, "Source is not an identity source");

    RenderBackend::pushDebugMarker("IdentityRenderer::render");
    IdentitySource* source = (IdentitySource*)sources.at(0);

    if (mDirtyShader)
    {
        mDirtyShader = false;
        mOpaqueShader.create(source->videoFrame.format);
    }

    // Bind textures
    for (uint8_t i = 0; i < source->videoFrame.numTextures; ++i)
    {
        RenderBackend::bindTexture(source->videoFrame.textures[i], i);
    }

    // Bind shader and constants
    Shader& selectedShader = mOpaqueShader;

    selectedShader.bind();

    if (source->videoFrame.format == VideoPixelFormat::RGB ||
        source->videoFrame.format == VideoPixelFormat::RGBA ||
        source->videoFrame.format == VideoPixelFormat::RGB_422_APPLE)
    {
        selectedShader.setSourceConstant(0);
    }
    else if (source->videoFrame.format == VideoPixelFormat::NV12)
    {
        selectedShader.setSourceYConstant(0);
        selectedShader.setSourceUVConstant(1);
    }
    else if (source->videoFrame.format == VideoPixelFormat::YUV_420)
    {
        selectedShader.setSourceYConstant(0);
        selectedShader.setSourceUConstant(1);
        selectedShader.setSourceVConstant(2);
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();
    }

    selectedShader.setTextureRectConstant(makeVector4(0, 0, 1, 1));

    Matrix44 modelMatrix = makeMatrix44(source->extrinsicRotation);

    float outputAspect = (float32_t)renderSurface.viewport.height / (float32_t)renderSurface.viewport.width ;
    float videoAspect = (float32_t)source->videoFrame.height / (float32_t)source->videoFrame.width;
    modelMatrix.m11 = videoAspect / outputAspect;
    if (modelMatrix.m11 > 1.0f)
    {
        modelMatrix.m00 = 1.f / modelMatrix.m11;
        modelMatrix.m11 = 1.0f;
    }

    selectedShader.setMVPConstant(modelMatrix);
    selectedShader.setSTConstant(source->videoFrame.matrix);

    RasterizerState rasterizerState;
    rasterizerState.cullMode = CullMode::BACK;
    rasterizerState.frontFace = FrontFace::CCW;
    rasterizerState.scissorTestEnabled = true;

    RenderBackend::setRasterizerState(rasterizerState);

    // Bind geometry
    RenderBackend::bindVertexBuffer(mVertexBuffer);

    // Draw
    RenderBackend::draw(PrimitiveType::TRIANGLE_LIST, 0, 6);

    RenderBackend::popDebugMarker();
}

IdentityRenderer::Shader::Shader()
    : handle(ShaderID::Invalid)
    , sourceConstant(ShaderConstantID::Invalid)
    , ySourceConstant(ShaderConstantID::Invalid)
    , uSourceConstant(ShaderConstantID::Invalid)
    , vSourceConstant(ShaderConstantID::Invalid)
    , uvSourceConstant(ShaderConstantID::Invalid)
    , textureRectConstant(ShaderConstantID::Invalid)
    , mvpConstant(ShaderConstantID::Invalid)
    , stConstant(ShaderConstantID::Invalid)
{
}

IdentityRenderer::Shader::~Shader()
{
    OMAF_ASSERT(handle == ShaderID::Invalid, "");

    OMAF_ASSERT(ySourceConstant == ShaderConstantID::Invalid, "");
    OMAF_ASSERT(uSourceConstant == ShaderConstantID::Invalid, "");
    OMAF_ASSERT(vSourceConstant == ShaderConstantID::Invalid, "");

    OMAF_ASSERT(uvSourceConstant == ShaderConstantID::Invalid, "");

    OMAF_ASSERT(sourceConstant == ShaderConstantID::Invalid, "");

    OMAF_ASSERT(textureRectConstant == ShaderConstantID::Invalid, "");

    OMAF_ASSERT(mvpConstant == ShaderConstantID::Invalid, "");
    OMAF_ASSERT(stConstant == ShaderConstantID::Invalid, "");
}

void_t IdentityRenderer::Shader::create(VideoPixelFormat::Enum textureFormat)
{
    OMAF_ASSERT(!isValid(), "Already initialized");

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
            "in vec2 aPosition;\n"
            "in vec2 aTextureCoord;\n"
            "\n"
            "out vec2 vTextureCoord;\n"
            "\n"
            "uniform highp vec4 uTextureRect;\n"
            "\n"
            "uniform mat4 uMVP;\n"
            "\n"
            "void main()\n"
            "{\n"
            "	vTextureCoord = aTextureCoord * uTextureRect.zw + uTextureRect.xy;\n"
            "\n"
            "	gl_Position = vec4(aPosition.xy, 0.0, 1.0)*uMVP;\n"
            "}\n";

        FixedString1024 version;
        FixedString1024 extensions;
        FixedString1024 defines;

        // GLSL / GLSL ES versions
#if OMAF_GRAPHICS_API_OPENGL
        version.append("#version 150\n");
#elif OMAF_GRAPHICS_API_OPENGL_ES
        version.append("#version 100\n");
#endif

        // Texture type defines
        if (textureFormat == VideoPixelFormat::RGB_422_APPLE)
        {
            defines.append("#define TEXTURE_RGB_422_APPLE\n");
        }
        else if (textureFormat == VideoPixelFormat::YUV_420)
        {
            defines.append("#define TEXTURE_YUV_420\n");
        }
        else if (textureFormat == VideoPixelFormat::NV12)
        {
            defines.append("#define TEXTURE_NV12\n");
        }
        else if (textureFormat == VideoPixelFormat::RGB)
        {
            defines.append("#define TEXTURE_RGB\n");
        }
        else if (textureFormat == VideoPixelFormat::RGBA)
        {
            defines.append("#define TEXTURE_RGBA\n");
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }

        // Enable st matrix
        defines.append("#define ENABLE_ST_MATRIX\n");

        // Sampler type
#if OMAF_PLATFORM_ANDROID

        extensions.append("#extension GL_OES_EGL_image_external : require\n");
        defines.append("#define SAMPLER_TYPE samplerExternalOES\n");

#else

        defines.append("#define SAMPLER_TYPE sampler2D\n");

#endif

        static const char_t* fs =
            "#if (__VERSION__ < 130)\n"
            "   #define in varying\n"
            "#else\n"
            "   out vec4 fragmentColor;\n"
            "#endif\n"
            "\n"
            "#if defined(ENABLE_SAMPLER_2D_RECT)\n"
            "   #if (__VERSION__ < 150)\n"
            "       #define texture texture2DRect\n"
            "   #endif\n"
            "#else\n"
            "   #if (__VERSION__ < 150)\n"
            "       #define texture texture2D\n"
            "   #endif\n"
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
            "in vec2 vTextureCoord;\n"
            "\n"
            "#if defined(TEXTURE_YUV_420)\n"
            "   uniform lowp SAMPLER_TYPE uSourceY;\n"
            "   uniform lowp SAMPLER_TYPE uSourceU;\n"
            "   uniform lowp SAMPLER_TYPE uSourceV;\n"
            "#elif defined(TEXTURE_NV12)\n"
            "   uniform lowp SAMPLER_TYPE uSourceY;\n"
            "   uniform lowp SAMPLER_TYPE uSourceUV;\n"
            "#elif defined(TEXTURE_RGB) || defined(TEXTURE_RGBA) || defined(TEXTURE_RGB_422_APPLE)\n"
            "   uniform lowp SAMPLER_TYPE uSource;\n"
            "#endif\n"
            "\n"
            "uniform mat4 uST;\n"
            "\n"
            "#if defined(TEXTURE_RGB) || defined(TEXTURE_RGBA) || defined(TEXTURE_RGB_422_APPLE)\n"
            "\n"
            "lowp vec4 SampleTextureRGBA(vec2 uv)\n"
            "{\n"
            "   #if defined(ENABLE_SAMPLER_2D_RECT)\n"
            "       uv = vec2(uv.x * textureSize(uSource).x, uv.y * textureSize(uSource).y);\n"
            "   #endif\n"
            "\n"
            "   return texture(uSource, uv);\n"
            "}\n"
            "\n"
            "#endif\n"
            "\n"
            "#if defined(TEXTURE_YUV_420)\n"
            "\n"
            "lowp vec4 SampleTextureY(vec2 uv)\n"
            "{\n"
            "   #if defined(ENABLE_SAMPLER_2D_RECT)\n"
            "       uv = vec2(uv.x * textureSize(uSourceY).x, uv.y * textureSize(uSourceY).y);\n"
            "   #endif\n"
            "\n"
            "   return texture(uSourceY, uv);\n"
            "}\n"
            "\n"
            "lowp vec4 SampleTextureU(vec2 uv)\n"
            "{\n"
            "   #if defined(ENABLE_SAMPLER_2D_RECT)\n"
            "       uv = vec2(uv.x * textureSize(uSourceU).x, uv.y * textureSize(uSourceU).y);\n"
            "   #endif\n"
            "\n"
            "   return texture(uSourceU, uv);\n"
            "}\n"
            "\n"
            "lowp vec4 SampleTextureV(vec2 uv)\n"
            "{\n"
            "   #if defined(ENABLE_SAMPLER_2D_RECT)\n"
            "       uv = vec2(uv.x * textureSize(uSourceV).x, uv.y * textureSize(uSourceV).y);\n"
            "   #endif\n"
            "\n"
            "   return texture(uSourceV, uv);\n"
            "}\n"
            "\n"
            "#endif\n"
            "\n"
            "#if defined(TEXTURE_NV12)\n"
            "\n"
            "lowp vec4 SampleTextureY(vec2 uv)\n"
            "{\n"
            "   #if defined(ENABLE_SAMPLER_2D_RECT)\n"
            "       uv = vec2(uv.x * textureSize(uSourceY).x, uv.y * textureSize(uSourceY).y);\n"
            "   #endif\n"
            "\n"
            "   return texture(uSourceY, uv);\n"
            "}\n"
            "\n"
            "\n"
            "lowp vec4 SampleTextureUV(vec2 uv)\n"
            "{\n"
            "   #if defined(ENABLE_SAMPLER_2D_RECT)\n"
            "       uv = vec2(uv.x * textureSize(uSourceUV).x, uv.y * textureSize(uSourceUV).y);\n"
            "   #endif\n"
            "\n"
            "   return texture(uSourceUV, uv);\n"
            "}\n"
            "\n"
            "#endif\n"
            "\n"
            "void main()"
            "{\n"
            "#if defined(ENABLE_ST_MATRIX)\n"
            "   vec2 textureCoord = (uST * vec4(vTextureCoord, 0.0, 1.0)).xy;\n"
            "#else\n"
            "   vec2 textureCoord = vTextureCoord;\n"
            "#endif\n"
            "\n"
            "#if defined(TEXTURE_RGB) || defined(TEXTURE_RGBA)\n"
            "   lowp vec4 c = vec4(SampleTextureRGBA(textureCoord).rgb, 1.0);\n"
            "#else\n"
            "   #if defined(TEXTURE_RGB_422_APPLE)\n"
            "       lowp vec4 color = SampleTextureRGBA(textureCoord);\n"
            "       lowp vec3 convertedColor = vec3(-0.87075, 0.52975, -1.08175);\n"
            "       convertedColor += 1.164 * color.g; // Y\n"
            "       convertedColor += vec3(0.0, -0.391, 2.018) * color.b; // U\n"
            "       convertedColor += vec3(1.596, -0.813, 0.0) * color.r; // V\n"
            "       lowp vec4 c = vec4(convertedColor, 1.0);\n"
            "   #elif defined(TEXTURE_YUV_420)\n"
            "       mediump float v = SampleTextureV(textureCoord).r - 128.0 / 255.0;\n"
            "       mediump float u = SampleTextureU(textureCoord).r - 128.0 / 255.0;\n"
            "       mediump float y = (SampleTextureY(textureCoord).r - 16.0 / 255.0) * (255.0 / (235.0 - 16.0));\n"
            "       lowp vec4 c = vec4(y + 1.403 * v, y - 0.344 * u - 0.714 * v, y + 1.770 * u, 1.0);\n"
            "   #elif defined(TEXTURE_NV12)\n"
            "       mat3 kColorConversion601 = mat3(1.164,  1.164, 1.164,\n"
            "                                         0.0, -0.392, 2.017,\n"
            "                                       1.596, -0.813,  0.0);\n"
            "       mediump vec3 yuv;\n"
            "       lowp vec3 rgb;\n"
            "\n"
            "       // Subtract constants to map the video range start at 0\n"
            "       yuv.x = (SampleTextureY(textureCoord).r - (16.0/255.0));\n"
            "       yuv.yz = (SampleTextureUV(textureCoord).rg - vec2(0.5, 0.5));\n"
            "       rgb = kColorConversion601 * yuv;\n"
            "       lowp vec4 c = vec4(rgb, 1);\n"
            "   #endif\n"
            "#endif\n"
            "\n"
            "#if (__VERSION__ < 130)\n"
            "	gl_FragColor = c;\n"
            "#else\n"
            "   fragmentColor = c;\n"
            "#endif\n"
            "}\n";

        FixedString8192 preprocessedFS;
        preprocessedFS.append(version.getData());
        preprocessedFS.append(extensions.getData());
        preprocessedFS.append(defines.getData());
        preprocessedFS.append(fs);

        handle = RenderBackend::createShader(vs, preprocessedFS.getData());
        OMAF_ASSERT(handle != ShaderID::Invalid, "");
    }
    else if (backendType == RendererType::D3D11 || backendType == RendererType::D3D12)
    {
        const char_t* shader =
            "float4 uTextureRect;\n"
            "matrix uMVP;\n"
            "matrix uST;\n"
            "\n"
            "Texture2D Texture0;\n"
            "Texture2D Texture1;\n"
            "SamplerState Sampler0;\n"
            "\n"
            "struct VS_INPUT\n"
            "{\n"
            "	float2 Position : POSITION;\n"
            "	float2 TexCoord0 : TEXCOORD0;\n"
            "};\n"
            "\n"
            "struct PS_INPUT\n"
            "{\n"
            "	float4 Position : SV_POSITION;\n"
            "	float2 TexCoord0 : TEXCOORD0;\n"
            "};\n"
            "\n"
            "PS_INPUT mainVS(VS_INPUT input)\n"
            "{\n"
            "	float4 position = float4(input.Position, 0.f, 1.0f);\n"
            "\n"
            "	PS_INPUT output = (PS_INPUT)0;\n"
            "	output.Position = mul(uMVP, position);\n"
            "	output.TexCoord0 = input.TexCoord0 * uTextureRect.zw + uTextureRect.xy;\n"
            "	output.TexCoord0 = float2(output.TexCoord0.x, 1.0f - output.TexCoord0.y);\n"
            "\n"
            "	return output;\n"
            "}"
            "\n"
            "float4 mainPS(PS_INPUT input) : SV_TARGET\n"
            "{\n"
            "	float4 yuv;\n"
            "	yuv.r = Texture0.Sample(Sampler0, input.TexCoord0).r;\n"
            "	yuv.gb = Texture1.Sample(Sampler0, input.TexCoord0).rg;\n"
            "\n"
            "	float4 rgb = float4(0.0f, 0.0f, 0.0f, 1.0f);\n"
            "	rgb.r = yuv.r + 1.140f * (yuv.b - (128.0f / 255.0f));\n"
            "	rgb.g = yuv.r - 0.395f * (yuv.g - (128.0f / 255.0f)) - 0.581f * (yuv.b - (128.0f / 255.0f));\n"
            "	rgb.b = yuv.r + 2.028f * (yuv.g - (128.0f / 255.0f));\n"
            "\n"
            "	return rgb;\n"
            "}\n";

        handle = RenderBackend::createShader(shader, shader);
        OMAF_ASSERT(handle != ShaderID::Invalid, "");
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();
    }

    sourceConstant = RenderBackend::createShaderConstant(handle, "uSource", ShaderConstantType::SAMPLER_2D);
    OMAF_ASSERT(sourceConstant != ShaderConstantID::Invalid, "");

    ySourceConstant = RenderBackend::createShaderConstant(handle, "uSourceY", ShaderConstantType::SAMPLER_2D);
    OMAF_ASSERT(ySourceConstant != ShaderConstantID::Invalid, "");

    uSourceConstant = RenderBackend::createShaderConstant(handle, "uSourceU", ShaderConstantType::SAMPLER_2D);
    OMAF_ASSERT(uSourceConstant != ShaderConstantID::Invalid, "");

    vSourceConstant = RenderBackend::createShaderConstant(handle, "uSourceV", ShaderConstantType::SAMPLER_2D);
    OMAF_ASSERT(vSourceConstant != ShaderConstantID::Invalid, "");

    uvSourceConstant = RenderBackend::createShaderConstant(handle, "uSourceUV", ShaderConstantType::SAMPLER_2D);
    OMAF_ASSERT(uvSourceConstant != ShaderConstantID::Invalid, "");

    textureRectConstant = RenderBackend::createShaderConstant(handle, "uTextureRect", ShaderConstantType::FLOAT4);
    OMAF_ASSERT(textureRectConstant != ShaderConstantID::Invalid, "");

    mvpConstant = RenderBackend::createShaderConstant(handle, "uMVP", ShaderConstantType::MATRIX44);
    OMAF_ASSERT(mvpConstant != ShaderConstantID::Invalid, "");

    stConstant = RenderBackend::createShaderConstant(handle, "uST", ShaderConstantType::MATRIX44);
    OMAF_ASSERT(stConstant != ShaderConstantID::Invalid, "");
}

void_t IdentityRenderer::Shader::bind()
{
    RenderBackend::bindShader(handle);
}

void_t IdentityRenderer::Shader::destroy()
{
    if (isValid())
    {
        RenderBackend::destroyShaderConstant(stConstant);
        stConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShaderConstant(mvpConstant);
        mvpConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShaderConstant(textureRectConstant);
        textureRectConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShaderConstant(ySourceConstant);
        ySourceConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShaderConstant(uSourceConstant);
        uSourceConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShaderConstant(vSourceConstant);
        vSourceConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShaderConstant(uvSourceConstant);
        uvSourceConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShaderConstant(sourceConstant);
        sourceConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShader(handle);
        handle = ShaderID::Invalid;
    }
}

bool_t IdentityRenderer::Shader::isValid()
{
    return (handle != ShaderID::Invalid);
}

void_t IdentityRenderer::Shader::setSourceYConstant(uint32_t textureSampler)
{
    RenderBackend::setShaderConstant(handle, ySourceConstant, &textureSampler);
}

void_t IdentityRenderer::Shader::setSourceUConstant(uint32_t textureSampler)
{
    RenderBackend::setShaderConstant(handle, uSourceConstant, &textureSampler);
}

void_t IdentityRenderer::Shader::setSourceVConstant(uint32_t textureSampler)
{
    RenderBackend::setShaderConstant(handle, vSourceConstant, &textureSampler);
}

void_t IdentityRenderer::Shader::setSourceUVConstant(uint32_t textureSampler)
{
    RenderBackend::setShaderConstant(handle, uvSourceConstant, &textureSampler);
}

void_t IdentityRenderer::Shader::setSourceConstant(uint32_t textureSampler)
{
    RenderBackend::setShaderConstant(handle, sourceConstant, &textureSampler);
}

void_t IdentityRenderer::Shader::setTextureRectConstant(const Vector4& textureRect)
{
    RenderBackend::setShaderConstant(handle, textureRectConstant, &textureRect);
}

void_t IdentityRenderer::Shader::setMVPConstant(const Matrix44& mvp)
{
    RenderBackend::setShaderConstant(handle, mvpConstant, &mvp);
}

void_t IdentityRenderer::Shader::setSTConstant(const Matrix44& st)
{
    RenderBackend::setShaderConstant(handle, stConstant, &st);
}
OMAF_NS_END
