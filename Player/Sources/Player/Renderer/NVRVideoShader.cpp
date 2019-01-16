
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
#include "Renderer/NVRVideoShader.h"
#include "Graphics/NVRRenderBackend.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(VideoShader);

VideoShader::VideoShader()
: handle(ShaderID::Invalid)
{
}

VideoShader::~VideoShader()
{
    OMAF_ASSERT(handle == ShaderID::Invalid, "");

    for (size_t i = 0; i < mShaderConstants.getSize(); ++i)
    {
        OMAF_ASSERT(mShaderConstants[i].second == ShaderConstantID::Invalid, "");
    }

    mShaderConstants.clear();
}

void_t VideoShader::create(const char_t *vertexShader, const char_t *fragmentShader, VideoPixelFormat::Enum videoTextureFormat, VideoPixelFormat::Enum depthTextureFormat, bool depthTextureVertexShaderFetch)
{
    OMAF_ASSERT(!isValid(), "Already initialized");

    RendererType::Enum backendType = RenderBackend::getRendererType();

    if (backendType == RendererType::OPENGL || backendType == RendererType::OPENGL_ES)
    {
        FixedString1024 version;

        if (backendType == RendererType::OPENGL)
        {
            version.append("#version 150\n");
        }
        else if (backendType == RendererType::OPENGL_ES)
        {
            if (depthTextureFormat != VideoPixelFormat::INVALID)
            {
                version.append("#version 300 es\n");
            }
            else
            {
                version.append("#version 100\n");
            }
        }

        static const char_t* precision = R"(
            #ifdef GL_ES
                #ifdef GL_FRAGMENT_PRECISION_HIGH
                    precision highp float;
                #else
                    precision mediump float;
                #endif
            #else
                #define lowp
                #define mediump
                #define highp
            #endif
        )";

        static const char_t* vsCompability = R"(
            #if (__VERSION__ < 130)
                #define in attribute
                #define out varying
            #endif
        )";

        static const char_t* fsCompability = R"(
            #if (__VERSION__ < 130)
                #define in varying
                #define fragmentColor gl_FragColor
            #else
                out vec4 fragmentColor;
            #endif
        )";

        FixedString1024 extensions;
        FixedString1024 defines;

        // Texture type defines
        if (videoTextureFormat == VideoPixelFormat::RGB_422_APPLE)
        {
            defines.append("#define TEXTURE_RGB_422_APPLE\n");
        }
        else if (videoTextureFormat == VideoPixelFormat::YUV_420)
        {
            defines.append("#define TEXTURE_YUV_420\n");
        }
        else if (videoTextureFormat == VideoPixelFormat::NV12)
        {
            defines.append("#define TEXTURE_NV12\n");
        }
        else if (videoTextureFormat == VideoPixelFormat::RGB)
        {
            defines.append("#define TEXTURE_RGB\n");
        }
        else if (videoTextureFormat == VideoPixelFormat::RGBA)
        {
            defines.append("#define TEXTURE_RGBA\n");
        }

        // Sampler type
#if OMAF_PLATFORM_ANDROID
        if (depthTextureFormat != VideoPixelFormat::INVALID)
        {
            extensions.append("#extension GL_OES_EGL_image_external_essl3 : require\n");
        }
        else
        {
            extensions.append("#extension GL_OES_EGL_image_external : require\n");
        }
        defines.append("#define SAMPLER_TYPE samplerExternalOES\n");

#else

        defines.append("#define SAMPLER_TYPE sampler2D\n");

#endif

        static const char_t* vsCommon = "";

        static const char_t* fsCommon = R"(
            #if defined(ENABLE_SAMPLER_2D_RECT)
                #if (__VERSION__ < 150)
                    #define texture texture2DRect
                #endif
            #else
                #if (__VERSION__ < 150)
                    #define texture texture2D
                #endif
            #endif

            uniform lowp sampler2D uSourceMask;

            #if defined(TEXTURE_YUV_420)
                uniform lowp SAMPLER_TYPE uSourceY;
                uniform lowp SAMPLER_TYPE uSourceU;
                uniform lowp SAMPLER_TYPE uSourceV;
            #elif defined(TEXTURE_NV12)
                uniform lowp SAMPLER_TYPE uSourceY;
                uniform lowp SAMPLER_TYPE uSourceUV;
            #elif defined(TEXTURE_RGB) || defined(TEXTURE_RGBA) || defined(TEXTURE_RGB_422_APPLE)
                uniform lowp SAMPLER_TYPE uSource;
            #endif

            #if defined(TEXTURE_RGB) || defined(TEXTURE_RGBA) || defined(TEXTURE_RGB_422_APPLE)
                lowp vec4 SampleTextureRGBA(vec2 uv)
                {
                    #if defined(ENABLE_SAMPLER_2D_RECT)
                        uv = vec2(uv.x * textureSize(uSource).x, uv.y * textureSize(uSource).y);
                    #endif
                    return texture(uSource, uv);
                }
            #endif

            #if defined(TEXTURE_YUV_420)
                lowp vec4 SampleTextureY(vec2 uv)
                {
                    #if defined(ENABLE_SAMPLER_2D_RECT)
                        uv = vec2(uv.x * textureSize(uSourceY).x, uv.y * textureSize(uSourceY).y);
                    #endif

                    return texture(uSourceY, uv);
                }

                lowp vec4 SampleTextureU(vec2 uv)
                {
                    #if defined(ENABLE_SAMPLER_2D_RECT)
                        uv = vec2(uv.x * textureSize(uSourceU).x, uv.y * textureSize(uSourceU).y);
                    #endif

                    return texture(uSourceU, uv);
                }

                lowp vec4 SampleTextureV(vec2 uv)
                {
                    #if defined(ENABLE_SAMPLER_2D_RECT)
                        uv = vec2(uv.x * textureSize(uSourceV).x, uv.y * textureSize(uSourceV).y);
                    #endif

                    return texture(uSourceV, uv);
                }
            #endif

            #if defined(TEXTURE_NV12)
                lowp vec4 SampleTextureY(vec2 uv)
                {
                    #if defined(ENABLE_SAMPLER_2D_RECT)
                        uv = vec2(uv.x * textureSize(uSourceY).x, uv.y * textureSize(uSourceY).y);
                    #endif

                    return texture(uSourceY, uv);
                }

                lowp vec4 SampleTextureUV(vec2 uv)
                {
                    #if defined(ENABLE_SAMPLER_2D_RECT)
                        uv = vec2(uv.x * textureSize(uSourceUV).x, uv.y * textureSize(uSourceUV).y);
                    #endif
                    return texture(uSourceUV, uv);
                }
            #endif

            lowp vec4 SampleVideoTexture(vec2 textureCoord)
            {
                lowp vec4 c;
                #if defined(TEXTURE_RGB) || defined(TEXTURE_RGBA)
                    c = vec4(SampleTextureRGBA(textureCoord).rgb, 1.0);
                #else
                    #if defined(TEXTURE_RGB_422_APPLE)
                        lowp vec4 color = SampleTextureRGBA(textureCoord);
                        lowp vec3 convertedColor = vec3(-0.87075, 0.52975, -1.08175);
                        convertedColor += 1.164 * color.g; // Y
                        convertedColor += vec3(0.0, -0.391, 2.018) * color.b; // U
                        convertedColor += vec3(1.596, -0.813, 0.0) * color.r; // V
                        c = vec4(convertedColor, 1.0);
                    #elif defined(TEXTURE_YUV_420)
                        mediump float v = SampleTextureV(textureCoord).r - 128.0 / 255.0;
                        mediump float u = SampleTextureU(textureCoord).r - 128.0 / 255.0;
                        mediump float y = (SampleTextureY(textureCoord).r - 16.0 / 255.0) * (255.0 / (235.0 - 16.0));
                        c = vec4(y + 1.403 * v, y - 0.344 * u - 0.714 * v, y + 1.770 * u, 1.0);
                    #elif defined(TEXTURE_NV12)
                        mat3 kColorConversion601 = mat3(1.164,  1.164, 1.164,
                                                        0.000, -0.392, 2.017,
                                                        1.596, -0.813, 0.000);
                        mediump vec3 yuv;
                        lowp vec3 rgb;

                        // Subtract constants to map the video range start at 0
                        yuv.x = (SampleTextureY(textureCoord).r - (16.0/255.0));
                        yuv.yz = (SampleTextureUV(textureCoord).rg - vec2(0.5, 0.5));
                        rgb = kColorConversion601 * yuv;
                        c = vec4(rgb, 1.0);
                    #endif
                #endif
                return c;
            }

            lowp float SampleMaskTexture(vec2 textureCoord)
            {
                return texture(uSourceMask, textureCoord).r;
            }
        )";

        static const char_t* depthCommon = R"(
            uniform SAMPLER_TYPE uDepthSource;

            uniform float uDepthNear;
            uniform float uDepthFar;
            uniform float uDepthScale;

            ivec2 DepthTextureSize()
            {
#if defined(ENABLE_SAMPLER_2D_RECT)
                return textureSize(uDepthSource);
#else
                return textureSize(uDepthSource, 0);
#endif
            }

            float depthFromLog(float zLog)
            {
                return pow(2.0, zLog * uDepthScale) * uDepthNear;
            }

            float depthSample(float linearDepth)
            {
                float d = depthFromLog(linearDepth);
                float nonLinearDepth = (uDepthFar + uDepthNear - 2.0 * uDepthNear * uDepthFar / d) / (uDepthFar - uDepthNear);
                nonLinearDepth = (nonLinearDepth + 1.0) / 2.0;
                return nonLinearDepth;
            }

            #if defined(TEXTURE_RGB) || defined(TEXTURE_RGBA)
            float sampleDepthRaw(vec2 uv)
            {
                #if defined(ENABLE_SAMPLER_2D_RECT)
                    uv = vec2(uv.x * textureSize(uDepthSource).x, uv.y * textureSize(uDepthSource).y);
                #endif
                return texture(uDepthSource, uv).r;
            }
            #elif defined(TEXTURE_NV12) || defined(TEXTURE_YUV_420)
            float sampleDepthRaw(vec2 uv)
            {
                // extend range from [16..235] -> [0..255]
#if defined(ENABLE_SAMPLER_2D_RECT)
                uv = vec2(uv.x * textureSize(uDepthSource).x, uv.y * textureSize(uDepthSource).y);
#endif
                return (texture(uDepthSource, uv).r - (16.0 / 255.0)) * (255.0 / 219.0);
            }
            #endif

            float SampleDepthTextureZBuffer(vec2 uv)
            {
                return depthSample(sampleDepthRaw(uv));
            }

            float SampleDepthTexture(vec2 uv)
            {
                return depthFromLog(sampleDepthRaw(uv));
            }
        )";
        FixedString16384 preprocessedVS;
        preprocessedVS.append(version.getData());
        preprocessedVS.append(extensions.getData());
        preprocessedVS.append(precision);
        preprocessedVS.append(vsCompability);
        preprocessedVS.append(defines.getData());
        preprocessedVS.append(vsCommon);

        FixedString16384 preprocessedFS;
        preprocessedFS.append(version.getData());
        preprocessedFS.append(extensions.getData());
        preprocessedFS.append(precision);
        preprocessedFS.append(fsCompability);
        preprocessedFS.append(defines.getData());
        preprocessedFS.append(fsCommon);

        if (depthTextureFormat != VideoPixelFormat::INVALID)
        {
            if (depthTextureFormat == VideoPixelFormat::NV12
                || depthTextureFormat == VideoPixelFormat::RGB
                || depthTextureFormat == VideoPixelFormat::RGBA
                || depthTextureFormat == VideoPixelFormat::YUV_420
                )
            {
                if (depthTextureVertexShaderFetch)
                {
                    preprocessedVS.append(depthCommon);
                }
                else
                {
                    preprocessedFS.append(depthCommon);
                }
            }
            else
            {
                OMAF_ASSERT_UNREACHABLE();
            }
        }

        preprocessedVS.append(vertexShader);
        preprocessedFS.append(fragmentShader);

        handle = RenderBackend::createShader(preprocessedVS.getData(), preprocessedFS.getData());
        OMAF_ASSERT(handle != ShaderID::Invalid, "");
    }
    else if (backendType == RendererType::D3D11 || backendType == RendererType::D3D12)
    {
        FixedString8192 preprocessed;

        static const char_t* fsCommonDX = R"(
            Texture2D uSourceY : register(t0);
            SamplerState sSourceY : register(s0);

            Texture2D uSourceUV : register(t1);
            SamplerState sSourceUV : register(s1);

            Texture2D uSourceMask : register(t4);
            SamplerState sSourceMask : register(s4);

            float3 yuv2rgb(float3 yuv)
            {
                float3x3 kColorConversion601 = {1.164,  1.164, 1.164,
                                                0.000, -0.392, 2.017,
                                                1.596, -0.813, 0.000};
                return mul(yuv - float3(16.0/255.0, 0.5, 0.5), kColorConversion601);
            }

            float4 SampleVideoTexture(float2 uv)
            {
                float3 yuv;
                yuv.r = uSourceY.Sample(sSourceY, uv).r;
                yuv.gb = uSourceUV.Sample(sSourceUV, uv).rg;
                return float4(yuv2rgb(yuv), 1.0);
            }

            float SampleMaskTexture(float2 uv)
            {
                return uSourceMask.Sample(sSourceMask, float2(uv.x, 1.0 - uv.y)).r;
            }
        )";

        static const char_t* depthCommonDX = R"(
            Texture2D uDepthSource : register(t3);
            SamplerState sDepthSource : register(s3);

            float uDepthNear;
            float uDepthFar;
            float uDepthScale;

            uint2 DepthTextureSize()
            {
                uint w,h;
                uDepthSource.GetDimensions(w, h);
                return uint2(w, h);
            }

            float depthFromLog(float zLog)
            {
                return pow(2.0, zLog * uDepthScale) * uDepthNear;
            }

            float depthSample(float linearDepth)
            {
                float d = depthFromLog(linearDepth);
                float nonLinearDepth = (uDepthFar + uDepthNear - 2.0 * uDepthNear * uDepthFar / d) / (uDepthFar - uDepthNear);
                nonLinearDepth = (nonLinearDepth + 1.0) / 2.0;
                return nonLinearDepth;
            }

            float sampleDepthRaw(float2 uv)
            {
                float d = (uDepthSource.SampleLevel(sDepthSource, uv, 0).r - (16.0 / 255.0)) * (255.0 / 219.0);
                return d;
            }

            float SampleDepthTextureZBuffer(float2 uv)
            {
                return depthSample(sampleDepthRaw(uv));
            }

            float SampleDepthTexture(float2 uv)
            {
                return depthFromLog(sampleDepthRaw(uv));
            }
        )";

        preprocessed.append(fsCommonDX);

        if (depthTextureFormat != VideoPixelFormat::INVALID)
        {
            if (depthTextureFormat == VideoPixelFormat::NV12)
            {
                preprocessed.append(depthCommonDX);
            }
            else
            {
                OMAF_ASSERT_UNREACHABLE();
            }
        }

        preprocessed.append(vertexShader);

        handle = RenderBackend::createShader(preprocessed.getData(), preprocessed.getData());
        OMAF_ASSERT(handle != ShaderID::Invalid, "");
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();
    }
}

void_t VideoShader::bind()
{
    RenderBackend::bindShader(handle);
}

void_t VideoShader::bindVideoTexture(VideoFrame videoFrame)
{
    SamplerState samplerState;
    samplerState.addressModeU = TextureAddressMode::CLAMP;
    samplerState.addressModeV = TextureAddressMode::CLAMP;
    samplerState.filterMode = TextureFilterMode::BILINEAR;

    // Bind textures
    for (uint8_t i = 0; i < videoFrame.numTextures; ++i)
    {
        RenderBackend::setSamplerState(videoFrame.textures[i], samplerState);
        RenderBackend::bindTexture(videoFrame.textures[i], i);
    }

    uint32_t textureUnit0 = 0;
    uint32_t textureUnit1 = 1;
    uint32_t textureUnit2 = 2;

    if (videoFrame.format == VideoPixelFormat::RGB ||
        videoFrame.format == VideoPixelFormat::RGBA ||
        videoFrame.format == VideoPixelFormat::RGB_422_APPLE)
    {
        setConstant("uSource", ShaderConstantType::SAMPLER_2D, &textureUnit0);
    }
    else if (videoFrame.format == VideoPixelFormat::NV12)
    {
        setConstant("uSourceY", ShaderConstantType::SAMPLER_2D, &textureUnit0);
        setConstant("uSourceUV", ShaderConstantType::SAMPLER_2D, &textureUnit1);
    }
    else if (videoFrame.format == VideoPixelFormat::YUV_420)
    {
        setConstant("uSourceY", ShaderConstantType::SAMPLER_2D, &textureUnit0);
        setConstant("uSourceU", ShaderConstantType::SAMPLER_2D, &textureUnit1);
        setConstant("uSourceV", ShaderConstantType::SAMPLER_2D, &textureUnit2);
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();
    }
}

void_t VideoShader::bindDepthTexture(VideoFrame videoFrame)
{
    // Bind textures
    uint32_t textureUnit3 = 3;

    if (videoFrame.format == VideoPixelFormat::NV12 ||
        videoFrame.format == VideoPixelFormat::YUV_420 ||
        videoFrame.format == VideoPixelFormat::RGBA ||
        videoFrame.format == VideoPixelFormat::RGB)
    {
        SamplerState samplerState;
        samplerState.addressModeU = TextureAddressMode::CLAMP;
        samplerState.addressModeV = TextureAddressMode::CLAMP;
        samplerState.filterMode = TextureFilterMode::POINT;

        RenderBackend::setSamplerState(videoFrame.textures[0], samplerState);
        RenderBackend::bindTexture(videoFrame.textures[0], textureUnit3);
        setConstant("uDepthSource", ShaderConstantType::SAMPLER_2D, &textureUnit3);
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();
    }
}

void_t VideoShader::bindMaskTexture(TextureID maskTexture, const Color4& clearColor)
{
    setConstant("uClearColor", ShaderConstantType::FLOAT4, &clearColor);

    // Bind textures
    uint32_t textureUnit4 = 4;

    SamplerState samplerState;
    samplerState.addressModeU = TextureAddressMode::CLAMP;
    samplerState.addressModeV = TextureAddressMode::CLAMP;
    samplerState.filterMode = TextureFilterMode::POINT;

    RenderBackend::setSamplerState(maskTexture, samplerState);
    RenderBackend::bindTexture(maskTexture, textureUnit4);
    setConstant("uSourceMask", ShaderConstantType::SAMPLER_2D, &textureUnit4);
}

void_t VideoShader::destroy()
{
    if (isValid())
    {
        for (ShaderConstantList::Iterator it = mShaderConstants.begin(); it != mShaderConstants.end(); ++it)
        {
            RenderBackend::destroyShaderConstant(it->second);
        }

        mShaderConstants.clear();

        RenderBackend::destroyShader(handle);
        handle = ShaderID::Invalid;
    }
}

bool_t VideoShader::isValid()
{
    return (handle != ShaderID::Invalid);
}

void_t VideoShader::setConstant(const char_t* name, ShaderConstantType::Enum type, const void_t* values, uint32_t numValues)
{
    for (ShaderConstantList::Iterator it = mShaderConstants.begin(); it != mShaderConstants.end(); ++it)
    {
        FixedString128 value = it->first;

        if (FixedString128(name).compare(value) == ComparisonResult::EQUAL)
        {
            RenderBackend::setShaderConstant(handle, it->second, values, numValues);
            return;
        }
    }

    ShaderConstantID constant = RenderBackend::createShaderConstant(handle, name, type);
    OMAF_ASSERT(constant != ShaderConstantID::Invalid, "Shader constant creation failed");

    RenderBackend::setShaderConstant(handle, constant, values, numValues);

    mShaderConstants.add(ShaderConstantPair(name, constant));
}

void_t VideoShader::createDefaultVideoShader(VideoPixelFormat::Enum videoTextureFormat, const bool_t maskEnabled)
{
    RendererType::Enum backendType = RenderBackend::getRendererType();

    char_t* vs = OMAF_NULL;
    char_t* ps = OMAF_NULL;

    if (backendType == RendererType::OPENGL || backendType == RendererType::OPENGL_ES)
    {
        static const char_t* vsGL = R"(
            in vec3 aPosition;
            in vec2 aTextureCoord;
            out vec2 vTextureCoord;

            uniform mat4 uMVP;
            uniform mat4 uVideoTextureMatrix;

            void main()
            {
                vTextureCoord = (uVideoTextureMatrix * vec4(aTextureCoord, 0.0, 1.0)).xy;
                gl_Position = uMVP * vec4(aPosition, 1.0);
            }
        )";

        static const char_t* psGL = R"(
            in vec2 vTextureCoord;

            void main()
            {
                vec4 c = SampleVideoTexture(vTextureCoord);
                fragmentColor = c;
            }
        )";


        static const char_t* vsGLmask = R"(
            in vec3 aPosition;
            in vec2 aTextureCoord;
            in vec2 aTextureOrigCoord;
            out vec2 vTextureCoord;
            out vec2 vTextureOrigCoord;

            uniform mat4 uMVP;
            uniform mat4 uVideoTextureMatrix;

            void main()
            {
                vTextureOrigCoord = aTextureOrigCoord;
                vTextureCoord = (uVideoTextureMatrix * vec4(aTextureCoord, 0.0, 1.0)).xy;
                gl_Position = uMVP * vec4(aPosition, 1.0);
            }
        )";

        static const char_t* psGLmask = R"(
            in vec2 vTextureCoord;
            in vec2 vTextureOrigCoord;
            uniform vec4 uClearColor;

            void main()
            {
                vec4 c = SampleVideoTexture(vTextureCoord);
                float a = SampleMaskTexture(vTextureOrigCoord);
                fragmentColor = mix(uClearColor, c, a);
            }
        )";

        if (maskEnabled)
        {
            vs = (char_t*)vsGLmask;
            ps = (char_t*)psGLmask;
        }
        else
        {
            vs = (char_t*)vsGL;
            ps = (char_t*)psGL;
        }
    }
    else if (backendType == RendererType::D3D11 || backendType == RendererType::D3D12)
    {
        static const char_t* shader = R"(
            matrix uMVP;
            matrix uVideoTextureMatrix;

            struct VS_INPUT
            {
                float3 aPosition : POSITION;
                float2 aTextureCoord : TEXCOORD0;
            };

            struct PS_INPUT
            {
                float4 Position : SV_POSITION;
                float2 vTextureCoord : TEXCOORD0;
            };

            PS_INPUT mainVS(VS_INPUT input)
            {
                PS_INPUT output = (PS_INPUT)0;
                output.vTextureCoord =  mul(uVideoTextureMatrix, float4(input.aTextureCoord, 0.0, 1.0)).xy;
                output.Position = mul(uMVP, float4(input.aPosition, 1.0));
                return output;
            }

            float4 mainPS(PS_INPUT input) : SV_TARGET
            {
                return SampleVideoTexture(input.vTextureCoord);
            }
        )";

        static const char_t* maskShader = R"(
            matrix uMVP;
            matrix uVideoTextureMatrix;
            uniform float4 uClearColor;

            struct VS_INPUT
            {
                float3 aPosition : POSITION;
                float2 aTextureCoord : TEXCOORD0;
                float2 aTextureOrigCoord : TEXCOORD1;
            };

            struct PS_INPUT
            {
                float4 Position : SV_POSITION;
                float2 vTextureCoord : TEXCOORD0;
                float2 vTextureOrigCoord : TEXCOORD1;
            };

            PS_INPUT mainVS(VS_INPUT input)
            {
                PS_INPUT output = (PS_INPUT)0;
                output.vTextureOrigCoord =  input.aTextureOrigCoord;
                output.vTextureCoord =  mul(uVideoTextureMatrix, float4(input.aTextureCoord, 0.0, 1.0)).xy;
                output.Position = mul(uMVP, float4(input.aPosition, 1.0));
                return output;
            }

            float4 mainPS(PS_INPUT input) : SV_TARGET
            {
                float4 c = SampleVideoTexture(input.vTextureCoord);
                float a = SampleMaskTexture(input.vTextureOrigCoord);
                return lerp(uClearColor, c, a);
            }
        )";

        if (maskEnabled)
        {
            vs = (char_t*)maskShader;
        }
        else
        {
            vs = (char_t*)shader;
        }
        ps = vs;
    }
    create(vs, ps, videoTextureFormat);
}

void_t VideoShader::createTintVideoShader(VideoPixelFormat::Enum videoTextureFormat)
{
    RendererType::Enum backendType = RenderBackend::getRendererType();

    char_t* vs = OMAF_NULL;
    char_t* ps = OMAF_NULL;

    if (backendType == RendererType::OPENGL || backendType == RendererType::OPENGL_ES)
    {
        static const char_t* vsGL = R"(
            in vec3 aPosition;
            in vec2 aTextureCoord;
            out vec2 vTextureCoord;

            uniform mat4 uMVP;
            uniform mat4 uVideoTextureMatrix;

            void main()
            {
                vTextureCoord = (uVideoTextureMatrix * vec4(aTextureCoord, 0.0, 1.0)).xy;
                gl_Position = uMVP * vec4(aPosition, 1.0);
            }
        )";

        static const char_t* psGL = R"(
            in vec2 vTextureCoord;
            uniform vec4 uTint;

            void main()
            {
                vec4 c = SampleVideoTexture(vTextureCoord);
                fragmentColor = c * uTint;
            }
        )";
        vs = (char_t*)vsGL;
        ps = (char_t*)psGL;
    }
    else if (backendType == RendererType::D3D11 || backendType == RendererType::D3D12)
    {
        static const char_t* shader = R"(
            matrix uMVP;
            matrix uVideoTextureMatrix;
            float4 uTint;

            struct VS_INPUT
            {
                float3 Position : POSITION;
                float2 TexCoord0 : TEXCOORD0;
            };

            struct PS_INPUT
            {
                float4 Position : SV_POSITION;
                float2 vTextureCoord : TEXCOORD0;
            };

            PS_INPUT mainVS(VS_INPUT input)
            {
                PS_INPUT output = (PS_INPUT)0;
                output.vTextureCoord =  mul(uVideoTextureMatrix, float4(input.TexCoord0, 0.0, 1.0)).xy;
                output.Position = mul(uMVP, float4(input.Position, 1.0));
                return output;
            }

            float4 mainPS(PS_INPUT input) : SV_TARGET
            {
                return SampleVideoTexture(input.vTextureCoord) * uTint;
            }
        )";
        vs = (char_t*)shader;
        ps = vs;
    }
    create(vs, ps, videoTextureFormat);
}

void_t VideoShader::setTintVideoShaderConstants(const Color4& tintColor)
{
    setConstant("uTint", ShaderConstantType::FLOAT4, &tintColor);
}

void_t VideoShader::setDefaultVideoShaderConstants(const Matrix44 mvp, const Matrix44 vtm)
{
    setConstant("uMVP", ShaderConstantType::MATRIX44, &mvp);
    setConstant("uVideoTextureMatrix", ShaderConstantType::MATRIX44, &vtm);
}

OMAF_NS_END
