
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
#include "Renderer/NVRRenderingManager.h"
#include "Graphics/NVRRenderBackend.h"

#include "Renderer/NVRCubeMapRenderer.h"
#include "Renderer/NVREquirectangularRenderer.h"
#include "Renderer/NVREquirectangularTileRenderer.h"
#include "Renderer/NVRIdentityRenderer.h"
#include "Renderer/NVROverlayRenderer.h"
#include "Renderer/NVRRawRenderer.h"

#include "API/OMAFDataTypeConversions.h"

#include "Foundation/NVRFileStream.h"
#include "Foundation/NVRFileSystem.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRPerformanceLogger.h"

#define OMAF_ENABLE_DEBUG_FRAME_DRAW 0

OMAF_NS_BEGIN

OMAF_LOG_ZONE(RenderingManager);

RenderingManager::RenderingManager()
    : mAllocator(*MemorySystem::DefaultHeapAllocator())
    , mCoreProvider(OMAF_NULL)
    , mRendererCollection(mAllocator)
    , mDebugSource(OMAF_NULL)
    , mFovHorizontal(0.f)
    , mFovVertical(0.f)
{
    mRenderMask.create();
}

RenderingManager::~RenderingManager()
{
    destroyRendererCollection();

    mRenderMask.destroy();

#if OMAF_ENABLE_DEBUG_FRAME_DRAW

    if (mDebugSource != OMAF_NULL)
    {
        RenderBackend::destroyTexture(mDebugSource->videoFrame.textures[0]);
    }

#endif
}

void_t RenderingManager::destroyRendererCollection()
{
    RendererCollection::ConstIterator it = mRendererCollection.begin();

    for (; it != mRendererCollection.end(); ++it)
    {
        VideoRenderer* renderer = *it;
        renderer->destroy();

        OMAF_DELETE(mAllocator, renderer);
        renderer = OMAF_NULL;
    }

    mRendererCollection.clear();
}

bool_t RenderingManager::initialize(CoreProvider* provider, RenderingManagerSettings& settings)
{
    OMAF_ASSERT_NOT_NULL(provider);
    OMAF_ASSERT(mCoreProvider == OMAF_NULL, "Implementation is already initialized");

    mCoreProvider = provider;

#if OMAF_ENABLE_DEBUG_FRAME_DRAW

    createDebugTexture("asset://CubeMap3x2.png");

#endif

    // this initialization copy pasted from EquirectangularMask::createRenderTarget() and from createDebugTexture
    // there are like 100 different createRenderTarget methods in this project.... hard to know what
    // to use or why so many...
    TextureDesc textureDesc;
    textureDesc.type = TextureType::TEXTURE_2D;
    textureDesc.width = (uint16_t) 720;
    textureDesc.height = (uint16_t) 720;
    textureDesc.numMipMaps = 1;
    textureDesc.generateMipMaps = false;
    textureDesc.format = TextureFormat::BGRA8;
    textureDesc.samplerState.addressModeU = TextureAddressMode::CLAMP;
    textureDesc.samplerState.addressModeV = TextureAddressMode::CLAMP;
    textureDesc.samplerState.filterMode = TextureFilterMode::BILINEAR;
    textureDesc.data = OMAF_NULL;
    textureDesc.size = (uint32_t) textureDesc.width * textureDesc.height;
    textureDesc.renderTarget = true;
    mHiddenTextureHandle = RenderBackend::createTexture(textureDesc);

    TextureID attachments[1] = {mHiddenTextureHandle};

    RenderTargetTextureDesc renderTargetDesc;
    renderTargetDesc.attachments = attachments;
    renderTargetDesc.numAttachments = 1;
    renderTargetDesc.destroyAttachments = true;
    renderTargetDesc.discardMask = DiscardMask::NONE;

    mHiddenRenderTarget = RenderBackend::createRenderTarget(renderTargetDesc);

    RenderBackend::bindRenderTarget(mHiddenRenderTarget);
    RasterizerState rasterizerState;
    rasterizerState.scissorTestEnabled = false;
    RenderBackend::setRasterizerState(rasterizerState);
    RenderBackend::setViewport(0, 0, 720, 720);
    RenderBackend::clear(ClearMask::COLOR_BUFFER | ClearMask::DEPTH_BUFFER | ClearMask::STENCIL_BUFFER, 1, 0.5, .5, 1);

    // NOTE: must be uncompressed RGBA8888 PVRv2 type of file
    // createDebugTexture("asset://debug.pvr");

    return true;
}

bool_t RenderingManager::initialize(CoreProvider* provider)
{
    OMAF_ASSERT_NOT_NULL(provider);

    RenderingManagerSettings settings = DefaultPresenceRendererSettings;

    return initialize(provider, settings);
}

static TextureFormat::Enum getTextureFormat(RenderTextureFormat::Enum format)
{
    switch (format)
    {
    case RenderTextureFormat::R8G8B8A8:
        return TextureFormat::RGBA8;
        // case RenderTextureFormat::R8G8B8A8_SRGB:    return TextureFormat::RGBA8_SRGB;

    case RenderTextureFormat::D16:
        return TextureFormat::D16;
    case RenderTextureFormat::D32F:
        return TextureFormat::D32F;
    case RenderTextureFormat::D24S8:
        return TextureFormat::D24S8;
    case RenderTextureFormat::D32FS8X24:
        return TextureFormat::D32FS8X24;

    default:
        break;
    }

    return TextureFormat::INVALID;
}

static bool_t isValidTextureDescription(const RenderTextureDesc& desc)
{
    if (desc.width == 0)
    {
        return false;
    }

    if (desc.height == 0)
    {
        return false;
    }

    if (getTextureFormat(desc.format) == TextureFormat::INVALID)
    {
        return false;
    }

    if (desc.ptr == OMAF_NULL)
    {
        return false;
    }

    return true;
}

static TextureType::Enum convertTextureType(OMAF::TextureType::Enum aEnum)
{
    switch (RenderBackend::getRendererType())
    {
    case RendererType::OPENGL:
    case RendererType::OPENGL_ES:
    {
        switch (aEnum)
        {
        case OMAF::TextureType::TEXTURE_2D:
        {
            return TextureType::TEXTURE_2D;
        }
        default:
        {
            return TextureType::Enum::INVALID;
        }
        }
        break;
    }
    case RendererType::D3D11:
    {
        switch (aEnum)

        {
        case OMAF::TextureType::TEXTURE_2D:
        {
            return TextureType::TEXTURE_2D;
        }
        case OMAF::TextureType::TEXTURE_ARRAY:
        {
            return TextureType::TEXTURE_ARRAY;
        }
        case OMAF::TextureType::TEXTURE_CUBE:
        {
            return TextureType::TEXTURE_CUBE;
        }
        default:
        {
            return TextureType::Enum::INVALID;
        }
        }
        break;
    }
    default:
    {
        break;
    }
    }
    return TextureType::Enum::INVALID;
}

uint32_t RenderingManager::createRenderTarget(const RenderTextureDesc* colorAttachment,
                                              const RenderTextureDesc* depthStencilAttachment)
{
    // Validate texture types
    if ((colorAttachment) && (convertTextureType(colorAttachment->type) == TextureType::Enum::INVALID))
    {
        return 0;
    }
    if ((depthStencilAttachment) && (convertTextureType(depthStencilAttachment->type) == TextureType::Enum::INVALID))
    {
        return 0;
    }

    TextureID attachments[2] = {TextureID::Invalid, TextureID::Invalid};
    uint8_t numAttachments = 0;

    if (colorAttachment != OMAF_NULL && isValidTextureDescription(*colorAttachment))
    {
        NativeTextureDesc colorAttachmentDesc;
        colorAttachmentDesc.type = convertTextureType(colorAttachment->type);
        colorAttachmentDesc.width = colorAttachment->width;
        colorAttachmentDesc.height = colorAttachment->height;
        colorAttachmentDesc.arrayIndex = colorAttachment->arrayIndex;
        colorAttachmentDesc.numMipMaps = 1;
        colorAttachmentDesc.format = getTextureFormat(colorAttachment->format);
        colorAttachmentDesc.nativeHandle = colorAttachment->ptr;

        TextureID colorAttachmentId = RenderBackend::createNativeTexture(colorAttachmentDesc);

        if (colorAttachmentId != TextureID::Invalid)
        {
            attachments[numAttachments] = colorAttachmentId;
            numAttachments++;
        }
    }

    if (depthStencilAttachment != OMAF_NULL && isValidTextureDescription(*depthStencilAttachment))
    {
        NativeTextureDesc depthStencilAttachmentDesc;
        depthStencilAttachmentDesc.type = convertTextureType(depthStencilAttachment->type);
        depthStencilAttachmentDesc.width = depthStencilAttachment->width;
        depthStencilAttachmentDesc.height = depthStencilAttachment->height;
        depthStencilAttachmentDesc.arrayIndex = depthStencilAttachment->arrayIndex;
        depthStencilAttachmentDesc.numMipMaps = 1;
        depthStencilAttachmentDesc.format = getTextureFormat(depthStencilAttachment->format);
        depthStencilAttachmentDesc.nativeHandle = depthStencilAttachment->ptr;

        TextureID depthStencilAttachmentId = RenderBackend::createNativeTexture(depthStencilAttachmentDesc);

        if (depthStencilAttachmentId != TextureID::Invalid)
        {
            attachments[numAttachments] = depthStencilAttachmentId;
            numAttachments++;
        }
    }

    if (numAttachments > 0)
    {
        RenderTargetTextureDesc renderTargetDesc;
        renderTargetDesc.attachments = attachments;
        renderTargetDesc.numAttachments = numAttachments;
        renderTargetDesc.destroyAttachments = true;
        renderTargetDesc.discardMask = DiscardMask::NONE;

        RenderTargetID handle = RenderBackend::createRenderTarget(renderTargetDesc);

        if (handle != RenderTargetID::Invalid)
        {
            return *(uint32_t*) &handle;
        }
    }

    return 0;
}

void RenderingManager::destroyRenderTarget(uint32_t handle)
{
    if (handle != 0)
    {
        RenderBackend::destroyRenderTarget((RenderTargetID) handle);
    }
}

void_t RenderingManager::prepareRender(const HeadTransform& headTransform,
                                       const RenderingParameters& renderingParamaters)
{
    PerformanceLogger logger("RenderingManager::prepareRender", 5);
    if (mCoreProvider == OMAF_NULL)
    {
        return;
    }
    mCoreProvider->prepareSources(headTransform, mFovHorizontal, mFovVertical);
    logger.printIntervalTime("prepareSources");
    if (!mCoreProvider->getSources().isEmpty() && mRendererCollection.isEmpty())
    {
        createRenderers();
        logger.printIntervalTime("createRenderers");
    }

    mRenderMask.updateClippingAreas(renderingParamaters.clipAreas, renderingParamaters.clipAreaCount);
}

void_t RenderingManager::render(const HeadTransform& headTransform,
                                const RenderSurface& renderSurface,
                                const RenderingParameters& renderingParamaters)
{
    PerformanceLogger logger("RenderingManager::render", 5);
    if (mCoreProvider == OMAF_NULL)
    {
        return;
    }

    // Always clear render surface if nothing else is not drawn
    clearFrame(renderSurface, renderingParamaters.clearColor);
    logger.printIntervalTime("clearFrame");
    if (mFovHorizontal == 0.f)
    {
        mFovVertical = toDegrees(atanf(1.0f / renderSurface.projection.m11)) * 2.0f;
        mFovHorizontal = toDegrees(atanf(1.0f / renderSurface.projection.m00)) * 2.0f;
    }

    renderFrame(headTransform, renderSurface, renderingParamaters);
    logger.printIntervalTime("renderFrame");
}

void_t RenderingManager::finishRender()
{
}

void_t RenderingManager::deinitialize()
{
    mCoreProvider = OMAF_NULL;

    // Throw away old renderers
    destroyRendererCollection();

    RenderBackend::destroyRenderTarget(mHiddenRenderTarget);
    if (mDebugSource)
    {
        RenderBackend::destroyTexture(mDebugSource->videoFrame.textures[0]);
        OMAF_DELETE(mAllocator, mDebugSource);
        mDebugSource = OMAF_NULL;
    }
}

void_t RenderingManager::createRenderers()
{
    OMAF_ASSERT_NOT_NULL(mCoreProvider);
    OMAF_ASSERT(mRendererCollection.isEmpty(), "");

    const CoreProviderSourceTypes& providerSourceTypes = mCoreProvider->getSourceTypes();
    OMAF_ASSERT(!providerSourceTypes.isEmpty(), "No source types yet");

    for (CoreProviderSourceTypes::ConstIterator it = providerSourceTypes.begin(); it != providerSourceTypes.end(); ++it)
    {
        VideoRenderer* renderer = createRenderer(*it);

        if (renderer != OMAF_NULL)
        {
            mRendererCollection.insert((*it), renderer);
        }
        else
        {
            OMAF_LOG_E("Could not create renderer for source type = %d", *it);

            OMAF_ASSERT_UNREACHABLE();
        }
    }
}

VideoRenderer* RenderingManager::createRenderer(SourceType::Enum sourceType) const
{
    VideoRenderer* renderer = OMAF_NULL;

    switch (sourceType)
    {
    case SourceType::EQUIRECTANGULAR_PANORAMA:
    {
        renderer = OMAF_NEW(mAllocator, EquirectangularRenderer)();

        break;
    }

    case SourceType::EQUIRECTANGULAR_TILES:
    case SourceType::EQUIRECTANGULAR_180:
    {
        renderer = OMAF_NEW(mAllocator, EquirectangularTileRenderer)();

        break;
    }

    case SourceType::CUBEMAP:
    {
        renderer = OMAF_NEW(mAllocator, CubeMapRenderer)();

        break;
    }

    case SourceType::IDENTITY:
    {
        renderer = OMAF_NEW(mAllocator, IdentityRenderer)();  // just flat regular video.

        break;
    }

    case SourceType::OVERLAY:
    {
        renderer = OMAF_NEW(mAllocator, OverlayRenderer)();

        break;
    }

    case SourceType::RAW:
    {
        renderer = OMAF_NEW(mAllocator, RawRenderer)();  // just flat regular video.

        break;
    }

    default:
        return OMAF_NULL;
    }

    renderer->create();

    return renderer;
}

void_t RenderingManager::clearFrame(const RenderSurface& renderSurface, const Color4& clearColor)
{
    RenderBackend::pushDebugMarker("RenderingManager::clearFrame");

    // NOTE: this method seems to do some pretty important stuff for rendering as a side effect, like setting scissors
    // and binding render target
    RenderBackend::bindRenderTarget((RenderTargetID) renderSurface.handle);

    RasterizerState rasterizerState;
    rasterizerState.scissorTestEnabled = true;

    RenderBackend::setRasterizerState(rasterizerState);

    RenderBackend::setViewport(renderSurface.viewport.x, renderSurface.viewport.y, renderSurface.viewport.width,
                               renderSurface.viewport.height);
    RenderBackend::setScissors(renderSurface.viewport.x, renderSurface.viewport.y, renderSurface.viewport.width,
                               renderSurface.viewport.height);

    RenderBackend::clear(ClearMask::COLOR_BUFFER | ClearMask::DEPTH_BUFFER | ClearMask::STENCIL_BUFFER, clearColor.r,
                         clearColor.g, clearColor.b, clearColor.a);

    RenderBackend::popDebugMarker();
}

void_t RenderingManager::renderFrame(const HeadTransform& headTransform,
                                     const RenderSurface& renderSurface,
                                     const RenderingParameters& renderingParameters)
{
    RenderBackend::pushDebugMarker("RenderingManager::renderFrame");
    SourcePosition::Enum eyePosition;
    if (renderSurface.eyePosition == EyePosition::RIGHT)
    {
        eyePosition = SourcePosition::RIGHT;
    }
    else if (renderSurface.eyePosition == EyePosition::LEFT)
    {
        eyePosition = SourcePosition::LEFT;
    }
    else
    {
        eyePosition = SourcePosition::LEFT;
    }

    FixedArray<VideoRenderer*, 32> preparedRenderers;

    // signal provider that it must not be modified
    mCoreProvider->enter();
    const CoreProviderSources& sources = mCoreProvider->getSources();

    for (CoreProviderSources::ConstIterator sit = sources.begin(); sit != sources.end(); ++sit)
    {
        CoreProviderSource* source = *sit;

        if (renderingParameters.renderMonoscopic && sources.getSize() > 1 &&
            source->sourcePosition == SourcePosition::LEFT)
        {
            // Don't draw the left sources when using monoscopic rendering
            continue;
        }

        if (eyePosition == source->sourcePosition || source->sourcePosition == SourcePosition::MONO ||
            sources.getSize() == 1 || renderingParameters.renderMonoscopic)
        {
            SourceType::Enum sourceType;
            if (!renderingParameters.rawRender)
            {
                sourceType = (*sit)->type;
            }
            else
            {
                sourceType = SourceType::RAW;
            }

            RendererCollection::Iterator rit = mRendererCollection.find(sourceType);
            VideoRenderer* renderer = OMAF_NULL;
            if (rit == RendererCollection::InvalidIterator)
            {
                renderer = createRenderer(sourceType);

                if (renderer != OMAF_NULL)
                {
                    mRendererCollection.insert(sourceType, renderer);
                }
                else
                {
                    OMAF_LOG_E("Could not create renderer for source type = %d", sourceType);

                    OMAF_ASSERT_UNREACHABLE();
                }
            }
            else
            {
                renderer = *rit;
            }
            // source should be done just once and passing temp render target like this is really bad
            source->renderer = renderer;
            source->tempTexture = mHiddenTextureHandle;
            source->tempTarget = mHiddenRenderTarget;

            // DEBUG help: pass random source to renderer in addition to actual one allowing to
            // see if the problem is in rendering or video decoding ()
            RenderingParameters* overrideConst = const_cast<RenderingParameters*>(&renderingParameters);
            overrideConst->extra = (void*) sources[rand() % sources.getSize()];

            CoreProviderSources selectedSources;
            selectedSources.add(source);
            if (renderer != OMAF_NULL && !source->omitRender)
            {
                renderer->render(headTransform, renderSurface, selectedSources, mRenderMask.getMaskTexture(),
                                 renderingParameters);
            }
        }
    }

    // free provider for modifications
    mCoreProvider->leave();
    RenderBackend::popDebugMarker();
}

void_t RenderingManager::createDebugTexture(const char_t* filename)
{
    struct PVRLegacyTextureHeader
    {
        uint32_t headerLength;
        uint32_t height;
        uint32_t width;
        uint32_t numMipmaps;
        uint32_t flags;
        uint32_t dataLength;
        uint32_t bpp;
        uint32_t bitmaskRed;
        uint32_t bitmaskGreen;
        uint32_t bitmaskBlue;
        uint32_t bitmaskAlpha;
        uint32_t pvrTag;
        uint32_t numSurfs;
    };

    FileStream* fileStream = FileSystem::open(filename, FileSystem::AccessMode::READ);
    OMAF_ASSERT_NOT_NULL(fileStream);
    OMAF_ASSERT(fileStream->isOpen(), "");

    PVRLegacyTextureHeader header;
    fileStream->read(&header, OMAF_SIZE_OF(header));

    uint8_t* buffer = (uint8_t*) malloc(header.dataLength);
    OMAF_ASSERT_NOT_NULL(buffer);

    fileStream->read(buffer, header.dataLength);

    TextureDesc textureDesc;
    textureDesc.type = TextureType::TEXTURE_2D;
    textureDesc.width = header.width;
    textureDesc.height = header.height;
    // numMipMaps must be at least 1 or texture generation will be setup wihtout memory pointer set
    textureDesc.numMipMaps = header.numMipmaps > 0 ? header.numMipmaps : 1;
    textureDesc.generateMipMaps = false;
    textureDesc.format = TextureFormat::RGBA8;
    textureDesc.samplerState.addressModeU = TextureAddressMode::CLAMP;
    textureDesc.samplerState.addressModeV = TextureAddressMode::CLAMP;
    textureDesc.samplerState.filterMode = TextureFilterMode::BILINEAR;
    textureDesc.data = buffer;
    textureDesc.size = header.dataLength;

    mDebugSource = OMAF_NEW_HEAP(EquirectangularSource);
    mDebugSource->videoFrame.format = VideoPixelFormat::RGBA;
    mDebugSource->videoFrame.textures[0] = RenderBackend::createTexture(textureDesc);
    mDebugSource->videoFrame.numTextures = 1;
    mDebugSource->videoFrame.matrix = Matrix44Identity;
}
OMAF_NS_END
