
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

    Vertex verts[] = {{-1.f, -1.f, 0.f, 0.f}, {1.f, -1.f, 1.f, 0.f}, {1.f, 1.f, 1.f, 1.f},
                      {-1.f, -1.f, 0.f, 0.f}, {1.f, 1.f, 1.f, 1.f},  {-1.f, 1.f, 0.f, 1.f}};

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

void_t IdentityRenderer::render(const HeadTransform& headTransform,
                                const RenderSurface& renderSurface,
                                const CoreProviderSources& sources,
                                const TextureID& renderMask,
                                const RenderingParameters& renderingParameters)
{
    OMAF_ASSERT(sources.getSize() == 1, "Supports only one input source");
    OMAF_ASSERT(sources.at(0)->type == SourceType::IDENTITY, "Source is not an identity source");
    auto source = sources.at(0);


    renderImpl(headTransform, renderSurface, source, renderMask, renderingParameters, 0, 0, 1, 1, 1, 1, 0, 0, 1);
}

void_t IdentityRenderer::renderImpl(const HeadTransform& headTransform,
                                    const RenderSurface& renderSurface,
                                    const CoreProviderSource* aSource,
                                    const TextureID& renderMask,
                                    const RenderingParameters& renderingParameters,
                                    float32_t x,
                                    float32_t y,
                                    float32_t width,
                                    float32_t height,
                                    float32_t scaleX,
                                    float32_t scaleY,
                                    float32_t offsetX,
                                    float32_t offsetY,
                                    float32_t opacity)
{
    x = x * 2;  // scale range to -1,1
    y = y * 2;  // scale range to -1,1

    //scaleX*=20;
    //scaleY*=20;

    // scale full screen (is actually not full screen... maybe 1024x1024)
    // should we make default scaling to be actaully the whole screen?
    //scaleX=1;
    //scaleY=1;
    //y = 0;
    //x = 0;

    VideoFrameSource* source = (VideoFrameSource*) aSource;

    if (mDirtyShader)
    {
        mDirtyShader = false;
        mOpaqueShader.create();
    }

    // Bind textures
    for (uint8_t i = 0; i < source->videoFrame.numTextures; ++i)
    {
        RenderBackend::bindTexture(source->videoFrame.textures[i], i);
    }

    // Bind shader and constants
    Shader& selectedShader = mOpaqueShader;

    selectedShader.bind();
    selectedShader.setPixelFormatConstant(source->videoFrame.format);
    selectedShader.setOpacityConstant(opacity);

    if (source->videoFrame.format == VideoPixelFormat::RGB || source->videoFrame.format == VideoPixelFormat::RGBA ||
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

    auto scale = makeScale(scaleX, scaleY, 1);

    auto translate = makeTranslation((x+offsetX)/scaleX, (y+offsetY)/scaleY, -1);
    auto mvp = renderSurface.projection * scale * translate;

    selectedShader.setMVPConstant(mvp);
    selectedShader.setSTConstant(source->videoFrame.matrix);

    BlendState blendState;
    blendState.blendEnabled = true;
    blendState.blendFunctionSrcRgb = BlendFunction::SRC_ALPHA;
    blendState.blendFunctionDstRgb = BlendFunction::ONE_MINUS_SRC_ALPHA;


    RasterizerState rasterizerState;
    rasterizerState.cullMode = CullMode::BACK;
    rasterizerState.frontFace = FrontFace::CCW;
    rasterizerState.scissorTestEnabled = true;

    RenderBackend::setBlendState(blendState);
    RenderBackend::setRasterizerState(rasterizerState);

    // scissors take screen pixels as a parameter 0,0 is bottom-left

    // figure out where this virtual screen size is defined... might not work on rift...
    // scale / offset and scissors calculations match on every (something about orthogonal camera fov maybe)
    auto virtualScreenWidth = 945.f;
    auto virtualScreenHeight = 1010.f;

    auto overlayWidthInPixels = width * virtualScreenWidth;
    auto overlayHeightInPixels = height * virtualScreenHeight;
    auto overlayCenterXInPixels = (x + 1.f)/2 * virtualScreenWidth;
    auto overlayCenterYInPixels = (y + 1.f)/2 * virtualScreenHeight;

    auto clippingXStart = overlayCenterXInPixels - overlayWidthInPixels/2;
    auto clippingYStart = overlayCenterYInPixels - overlayHeightInPixels/2;

    auto areaStartX = (float32_t)(renderSurface.viewport.width - virtualScreenWidth) / 2;
    auto areaStartY = (float32_t)(renderSurface.viewport.height - virtualScreenHeight) / 2;

    RenderBackend::setScissors(clippingXStart+areaStartX, clippingYStart+areaStartY, overlayWidthInPixels, overlayHeightInPixels);
    // NOTE: for debugging fulls screen scissors...
    // RenderBackend::setScissors(0, 0, renderSurface.viewport.width, renderSurface.viewport.height);

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
    , pixelFormatConstant(ShaderConstantID::Invalid)
    , opacityConstant(ShaderConstantID::Invalid)
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

void_t IdentityRenderer::Shader::create()
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
            R"(
            #if (__VERSION__ < 130)
                #define in attribute
                #define out varying
            #endif
            
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
            
            in vec2 aPosition;
            in vec2 aTextureCoord;           
            out vec2 vTextureCoord;            
            uniform highp vec4 uTextureRect;            
            uniform mat4 uMVP;
            
            void main()
            {
            	vTextureCoord = aTextureCoord * uTextureRect.zw + uTextureRect.xy;            
            	gl_Position = uMVP*vec4(aPosition.xy, 0.0, 1.0);
            }
        )";

        FixedString1024 version;
        FixedString1024 extensions;
        FixedString1024 defines;

// GLSL / GLSL ES versions
#if OMAF_GRAPHICS_API_OPENGL
        version.append("#version 150\n");
#elif OMAF_GRAPHICS_API_OPENGL_ES
        version.append("#version 100\n");
#endif
        // Enable st matrix
        defines.append("#define ENABLE_ST_MATRIX\n");

// Sampler type
#if OMAF_PLATFORM_ANDROID

        extensions.append("#extension GL_OES_EGL_image_external : require\n");
        defines.append("#define SAMPLER_TYPE samplerExternalOES\n");

#else

        defines.append("#define SAMPLER_TYPE sampler2D\n");

#endif

        static const char_t* fs = R"(

            #if (__VERSION__ < 130)
               #define in varying
            #else
               out vec4 fragmentColor;
            #endif
            
            #if defined(ENABLE_SAMPLER_2D_RECT)
               #if (__VERSION__ < 150)
                   #define texture texture2DRect
               #endif
            #else
               #if (__VERSION__ < 150)
                   #define texture texture2D
               #endif
            #endif
            
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
            
            #define TEXTURE_RGB 0
            #define TEXTURE_RGBA 1
            #define TEXTURE_RGB_422_APPLE 2
            #define TEXTURE_YUV_420 3
            #define TEXTURE_NV12 4

            uniform int uPixelFormat;
            uniform float uSrcOpacity;

            in vec2 vTextureCoord;

            //       also refactor opengl / opengl version differences so that           
            uniform lowp SAMPLER_TYPE uSourceY;
            uniform lowp SAMPLER_TYPE uSourceU;
            uniform lowp SAMPLER_TYPE uSourceV;
            uniform lowp SAMPLER_TYPE uSourceUV;
            uniform lowp SAMPLER_TYPE uSource;
            
            uniform mat4 uST;
                        
            lowp vec4 SampleTextureRGBA(vec2 uv)
            {
                #if defined(ENABLE_SAMPLER_2D_RECT)
                    uv = vec2(uv.x * textureSize(uSource).x, uv.y * textureSize(uSource).y);
                #endif            
                return texture(uSource, uv);
            }
            
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
                                             
            lowp vec4 SampleTextureUV(vec2 uv)
            {
               #if defined(ENABLE_SAMPLER_2D_RECT)
                   uv = vec2(uv.x * textureSize(uSourceUV).x, uv.y * textureSize(uSourceUV).y);
               #endif
            
               return texture(uSourceUV, uv);
            }
            
            void main()
            {
            #if defined(ENABLE_ST_MATRIX)
               vec2 textureCoord = (uST * vec4(vTextureCoord, 0.0, 1.0)).xy;
            #else
               vec2 textureCoord = vTextureCoord;
            #endif
            
            lowp vec4 c = vec4(0.0, 0.0, 0.0, uSrcOpacity);

            if (uPixelFormat == TEXTURE_RGB || uPixelFormat == TEXTURE_RGBA) {
                c.rgb = SampleTextureRGBA(textureCoord).rgb;
            } else if(uPixelFormat == TEXTURE_RGB_422_APPLE) {
                lowp vec4 color = SampleTextureRGBA(textureCoord);
                lowp vec3 convertedColor = vec3(-0.87075, 0.52975, -1.08175);
                convertedColor += 1.164 * color.g; // Y
                convertedColor += vec3(0.0, -0.391, 2.018) * color.b; // U
                convertedColor += vec3(1.596, -0.813, 0.0) * color.r; // V
                c.rgb = convertedColor;
            } else if(uPixelFormat == TEXTURE_YUV_420) {
                mediump float v = SampleTextureV(textureCoord).r - 128.0 / 255.0;
                mediump float u = SampleTextureU(textureCoord).r - 128.0 / 255.0;
                mediump float y = (SampleTextureY(textureCoord).r - 16.0 / 255.0) * (255.0 / (235.0 - 16.0));
                c.rgb = vec3(y + 1.403 * v, y - 0.344 * u - 0.714 * v, y + 1.770 * u);
            } else if(uPixelFormat == TEXTURE_NV12) {
                mat3 kColorConversion601 = mat3(1.164,  1.164, 1.164,
                                                0.0, -0.392, 2.017,
                                                1.596, -0.813,  0.0);
                mediump vec3 yuv;
                lowp vec3 rgb;
            
                // Subtract constants to map the video range start at 0
                yuv.x = (SampleTextureY(textureCoord).r - (16.0/255.0));
                yuv.yz = (SampleTextureUV(textureCoord).rg - vec2(0.5, 0.5));
                rgb = kColorConversion601 * yuv;
                c.rgb = rgb;
            }
            
            #if (__VERSION__ < 130)
            	gl_FragColor = c;
            #else
               fragmentColor = c;
            #endif
            }
        )";

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
            R"(
                #define TEXTURE_RGB 0
                #define TEXTURE_RGBA 1
                #define TEXTURE_RGB_422_APPLE 2
                #define TEXTURE_YUV_420 3
                #define TEXTURE_NV12 4

                float4 uTextureRect;
                matrix uMVP;
                matrix uST;
            
                Texture2D Texture0;
                Texture2D Texture1;
                SamplerState Sampler0;

                uniform float uSrcOpacity;
                uniform int uPixelFormat;

            
                struct VS_INPUT
                {
            	    float2 Position : POSITION;
            	    float2 TexCoord0 : TEXCOORD0;
                };
            
                struct PS_INPUT
                {
            	    float4 Position : SV_POSITION;
            	    float2 TexCoord0 : TEXCOORD0;
                };
            
                PS_INPUT mainVS(VS_INPUT input)
                {
            	    float4 position = float4(input.Position, 0.f, 1.0f);
            
            	    PS_INPUT output = (PS_INPUT)0;
            	    output.Position = mul(uMVP, position);
            	    output.TexCoord0 = input.TexCoord0 * uTextureRect.zw + uTextureRect.xy;
            	    output.TexCoord0 = float2(output.TexCoord0.x, 1.0f - output.TexCoord0.y);
            
            	    return output;
                }
            
                float4 mainPS(PS_INPUT input) : SV_TARGET
                {
                    float4 output = float4(0.0f, 0.0f, 0.0f, uSrcOpacity);

                    if (uPixelFormat == TEXTURE_NV12) {
            	        float4 yuv;
            	        yuv.r = Texture0.Sample(Sampler0, input.TexCoord0).r;
            	        yuv.gb = Texture1.Sample(Sampler0, input.TexCoord0).rg;
            
            	        output.r = yuv.r + 1.140f * (yuv.b - (128.0f / 255.0f));
            	        output.g = yuv.r - 0.395f * (yuv.g - (128.0f / 255.0f)) - 0.581f * (yuv.b - (128.0f / 255.0f));
            	        output.b = yuv.r + 2.028f * (yuv.g - (128.0f / 255.0f));
                    } else {
                        // expect RGB or RGBA input
                        output.rgb = Texture0.Sample(Sampler0, input.TexCoord0).rgb;
                    }

                    return output;
                 }
            )";

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

    opacityConstant = RenderBackend::createShaderConstant(handle, "uSrcOpacity", ShaderConstantType::FLOAT);
    OMAF_ASSERT(opacityConstant != ShaderConstantID::Invalid, "");

    pixelFormatConstant = RenderBackend::createShaderConstant(handle, "uPixelFormat", ShaderConstantType::INTEGER);
    OMAF_ASSERT(pixelFormatConstant != ShaderConstantID::Invalid, "");
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

        RenderBackend::destroyShaderConstant(pixelFormatConstant);
        pixelFormatConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShaderConstant(opacityConstant);
        opacityConstant = ShaderConstantID::Invalid;

        RenderBackend::destroyShader(handle);
        handle = ShaderID::Invalid;
    }
}

bool_t IdentityRenderer::Shader::isValid()
{
    return (handle != ShaderID::Invalid);
}

void_t IdentityRenderer::Shader::setPixelFormatConstant(VideoPixelFormat::Enum pixelFormat)
{
    RenderBackend::setShaderConstant(handle, pixelFormatConstant, &pixelFormat);
}

void_t IdentityRenderer::Shader::setOpacityConstant(float32_t opacity)
{
    RenderBackend::setShaderConstant(handle, opacityConstant, &opacity);
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
