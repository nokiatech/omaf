
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
#include "Renderer/NVROverlayRenderer.h"
#include "Renderer/NVRIdentityRenderer.h"

#include "Graphics/NVRRenderBackend.h"
#include "Math/OMAFMath.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRNew.h"


#ifdef _DEBUG
//#define DEBUG_DRAW_TILES
#endif  // _DEBUG

OMAF_NS_BEGIN
OMAF_LOG_ZONE(OverlayRenderer)

OverlayRenderer::OverlayRenderer()
    : mDirtyShader(true)
    , mAllocator(*MemorySystem::DefaultHeapAllocator())
    , mIdentityRenderer(OMAF_NEW(mAllocator, IdentityRenderer)())
{
}

OverlayRenderer::~OverlayRenderer()
{
    destroy();
}

void_t OverlayRenderer::createImpl()
{
    // has to be created dynamically, as there can be multiple sources
    mIdentityRenderer->create();

    mGeometry2D = OMAF_NEW(mAllocator, EquirectangularMesh)();
    mGeometryOmni = OMAF_NEW(mAllocator, EquirectangularMesh)();

    // height / width really doesn't matter at this point, they will be updated ondemand
    mGeometry2D->createSphereRelative2DTile(makeVector2(0, 0), makeVector2(170, 90));
    mGeometryOmni->createSphereRelativeOmniTile(makeVector2(0, 0), makeVector2(360, 180), 256, 128);
}


void_t OverlayRenderer::destroyImpl()
{
    mGeometry2D->destroy();
    mGeometryOmni->destroy();

    OMAF_DELETE(mAllocator, mGeometry2D);
    OMAF_DELETE(mAllocator, mGeometryOmni);

    mOpaqueShader.destroy();
    mMaskShader.destroy();

    OMAF_DELETE(mAllocator, mIdentityRenderer);
}

void_t OverlayRenderer::render(const HeadTransform& headTransform,
                               const RenderSurface& renderSurface,
                               const CoreProviderSources& sources,
                               const TextureID& renderMask,
                               const RenderingParameters& renderingParameters)
{
    OMAF_ASSERT(sources.getSize() == 1, "Supports only one input source");
    OMAF_ASSERT(sources.at(0)->type == SourceType::OVERLAY, "Source is not an overlay");

    RenderBackend::pushDebugMarker("EquirectangularPanoramaRenderer::render");

    bool_t useMask = renderMask != TextureID::Invalid;

    for (size_t i = 0, endi = sources.getSize(); i < endi; ++i)
    {
        OverlaySource* source = (OverlaySource*) sources.at(i);

        // skip deactivated overlays
        if (!source->active)
        {
            continue;
        }

        // render original media to virtual surface in case if source is VR media
        OMAF::Private::VideoFrameSource* overlayFrameSource = source->mediaSource;

        source->videoFrame.format = VideoPixelFormat::RGBA;
        source->videoFrame.textures[0] = source->tempTexture;
        source->videoFrame.numTextures = 1;
        source->videoFrame.matrix = source->mediaSource->videoFrame.matrix;
        source->videoFrame.height = 720;
        source->videoFrame.width = 720;
        source->videoFrame.pts = source->mediaSource->videoFrame.pts;

        if (source->mediaSource->type == SourceType::EQUIRECTANGULAR_PANORAMA)
        {
            overlayFrameSource = source;

            RenderSurface surf;
            static OMAF::Matrix44 projection =
                renderSurface.projection;
                                           // with correct projection matrix for left eye
            surf.projection = renderSurface.projection;
            surf.projection = projection;
            surf.eyeTransform = Matrix44Identity;
            surf.eyePosition = EyePosition::LEFT;
            surf.viewport.height = 720;
            surf.viewport.width = 720;
            surf.viewport.x = 0;
            surf.viewport.y = 0;
            surf.handle = (uint32_t) source->tempTarget._handle;

            source->textureRect.h = 1;
            source->textureRect.w = 1;
            source->textureRect.x = 0;
            source->textureRect.y = 0;

            RenderBackend::bindRenderTarget(source->tempTarget);
            CoreProviderSources sources;
            sources.add(source->mediaSource);

            HeadTransform overlayHead;

            if (source->initialViewingOrientation.valid)
            {
                overlayHead.orientation = source->initialViewingOrientation.orientation;
            }
            else
            {
                overlayHead.orientation = makeQuaternion(0, 0, 0, EulerAxisOrder::YXZ);
            }

            overlayHead.source = HeadTransformSource::MANUAL;
            source->mediaSource->forcedOrientation.valid = false;

            source->mediaSource->renderer->render(overlayHead, surf, sources, renderMask, renderingParameters);
            RenderBackend::bindRenderTarget((RenderTargetID) renderSurface.handle);
        }

        if (mDirtyShader)
        {
            mDirtyShader = false;
#ifdef DEBUG_DRAW_TILES
            mOpaqueShader.createTintVideoShader(source->videoFrame.format);
#else
            // videoformat is just ignored during shader creation and read from shader constant
            mOpaqueShader.createDefaultVideoShader(VideoPixelFormat::INVALID);
            mMaskShader.createDefaultVideoShader(VideoPixelFormat::INVALID, true);

#endif  // DEBUG_DRAW_TILES
        }

        float32_t opacity = source->overlayParameters.overlayOpacity.doesExist
            ? (float(source->overlayParameters.overlayOpacity.opacity) / 256)
            : 1.0f;

        // use identity renderer for drawing viewport relative overlays
        if (source->overlayParameters.viewportRelativeOverlay.doesExist)
        {
            float32_t width = (float) source->overlayParameters.viewportRelativeOverlay.rectWidthtPercent / 65536;
            float32_t height = (float) source->overlayParameters.viewportRelativeOverlay.rectHeightPercent / 65536;

            float32_t ovlyAreaLeft = (float) source->overlayParameters.viewportRelativeOverlay.rectLeftPercent / 65536;
            float32_t ovlyAreaTop = (float) source->overlayParameters.viewportRelativeOverlay.rectTopPercent / 65536;

            float32_t ovlyCenterX = (-0.5f + (ovlyAreaLeft + width / 2));
            float32_t ovlyCenterY = (0.5f - (ovlyAreaTop + height / 2));

            // set aspect ratio of temporary source to match the overlay area
            float32_t aspect = (float32_t)width / height;

            // NOTE: these might be necessary for overlay with VR content... but looks like they might
            //       not be used for anything...
            if (aspect > 1)
            {
                source->textureRect.h = 1.f / aspect;
                source->textureRect.w = 1.f;
                source->textureRect.x = 0;
                source->textureRect.y = (1.f - source->textureRect.h) / 2;
            }
            else
            {
                source->textureRect.h = 1.f;
                source->textureRect.w = aspect;
                source->textureRect.x = (1.f - aspect) / 2;
                source->textureRect.y = 0;
            }

            // Calculate scaling and offset according to alignment and fill style
            // 100% is actually ~1024x1024 rect in the middle of screen (double check)

            auto sourceHeightInPix = overlayFrameSource->heightInPixels;
            auto sourceWidthInPix = overlayFrameSource->widthInPixels;
            auto sourceAspect = (float32_t) sourceWidthInPix / sourceHeightInPix;

            auto offsetX = 0.f;
            auto offsetY = 0.f;
            auto scaleX = 1.f;
            auto scaleY = 1.f;

            // NOTE: By default all the frameSource textures are the same size filling up
            // the center (around 945x1010 pix) of the render surface's viewport if not scaled down..
            // easier and not dependent on HMD device

            // calculations for overlay ascpect ratio.... we really should fix
            // drawing 100%x100% overlay to fill entire drawing area so that we could
            // use dawing surface size as a reference.

            // scaled dimensions to correct aspect ratio
            // for scaling select width x aspectCorrectScaleY or aspectCorrectScaleX x height
            auto aspectCorrectScaleX = (float32_t) width / aspect * sourceAspect;
            auto aspectCorrectScaleY = (float32_t) height * aspect / sourceAspect;

            // NOTE: probably generated files are old
            // (update WWS omafvi etc. and try to regenerate... first fix o4c)
            // files generated with old omaf creator:
            // works with  sh update_from_artifactory.sh OMAFv2 OMAF_1.0.0-181-gcc9c732
            // fails with  sh update_from_artifactory.sh OMAFv2 OMAF_1.0.0-184-gb24a18d

            if (source->overlayParameters.viewportRelativeOverlay.mediaAlignment >= ISOBMFF::MediaAlignmentType::HC_VC_CROP) {
                // -------------------- crop to fill --------------------
                if (aspectCorrectScaleY < height) {
                    scaleX = aspectCorrectScaleX;
                    scaleY = height;
                }

                if (aspectCorrectScaleX < width) {
                    scaleX = width;
                    scaleY = aspectCorrectScaleY;
                }
            } else if (source->overlayParameters.viewportRelativeOverlay.mediaAlignment >= ISOBMFF::MediaAlignmentType::HC_VC_SCALE) {
                // -------------------- scale to fit --------------------
                if (aspectCorrectScaleY < height) {
                    scaleX = width;
                    scaleY = aspectCorrectScaleY;
                }

                if (aspectCorrectScaleX < width) {
                    scaleX = aspectCorrectScaleX;
                    scaleY = height;
                }
            } else {
                // -------------------- stretch to fill -------------------
                scaleX = width;
                scaleY = height;
            }

        /*
            switch (source->overlayParameters.viewportRelativeOverlay.mediaAlignment) {
                case ISOBMFF::MediaAlignmentType::HC_VC_SCALE:
                case ISOBMFF::MediaAlignmentType::HC_VC_CROP:
                    break;
                case ISOBMFF::MediaAlignmentType::HC_VT_SCALE:
                case ISOBMFF::MediaAlignmentType::HC_VT_CROP:
                    break;
                case ISOBMFF::MediaAlignmentType::HC_VB_SCALE:
                case ISOBMFF::MediaAlignmentType::HC_VB_CROP:
                    break;
                case ISOBMFF::MediaAlignmentType::HL_VC_SCALE:
                case ISOBMFF::MediaAlignmentType::HL_VC_CROP:
                    break;
                case ISOBMFF::MediaAlignmentType::HR_VC_SCALE:
                case ISOBMFF::MediaAlignmentType::HR_VC_CROP:
                    break;
                case ISOBMFF::MediaAlignmentType::HL_VT_SCALE:
                case ISOBMFF::MediaAlignmentType::HL_VT_CROP:
                    break;
                case ISOBMFF::MediaAlignmentType::HR_VT_SCALE:
                case ISOBMFF::MediaAlignmentType::HR_VT_CROP:
                    break;
                case ISOBMFF::MediaAlignmentType::HL_VB_SCALE:
                case ISOBMFF::MediaAlignmentType::HL_VB_CROP:
                    break;
                case ISOBMFF::MediaAlignmentType::HR_VB_SCALE:
                case ISOBMFF::MediaAlignmentType::HR_VB_CROP:
                    break;
                default:
                    break;
            }
*/
            mIdentityRenderer->renderImpl(headTransform, renderSurface, overlayFrameSource,
                                      renderMask,
                                      renderingParameters, ovlyCenterX, ovlyCenterY, width,
                                      height,
                                      scaleX, scaleY, offsetX, offsetY,
                                      opacity);

            continue;
        }

        float32_t widthDegrees;
        float32_t heightDegrees;
        float32_t distance = 1.0;

        if (source->overlayParameters.sphereRelative2DOverlay.doesExist)
        {
            widthDegrees = (float) source->overlayParameters.sphereRelative2DOverlay.sphereRegion.azimuthRange / 65536;
            heightDegrees =
                (float) source->overlayParameters.sphereRelative2DOverlay.sphereRegion.elevationRange / 65536;
            distance = ((float) source->overlayParameters.sphereRelative2DOverlay.regionDepthMinus1 + 1) / 65536;
        }
        else if (source->overlayParameters.sphereRelativeOmniOverlay.doesExist)
        {
            widthDegrees =
                (float) source->overlayParameters.sphereRelativeOmniOverlay.sphereRegion.azimuthRange / 65536;
            heightDegrees =
                (float) source->overlayParameters.sphereRelativeOmniOverlay.sphereRegion.elevationRange / 65536;
            distance = ((float) source->overlayParameters.sphereRelativeOmniOverlay.regionDepthMinus1 + 1) / 65536;
        }

        EquirectangularMesh* mesh;
        if (source->overlayParameters.sphereRelative2DOverlay.doesExist)
        {
            mGeometry2D->destroy();
            mGeometry2D->createSphereRelative2DTile(makeVector2(0, 0), makeVector2(widthDegrees, heightDegrees));
            mesh = mGeometry2D;
        }

        else if (source->overlayParameters.sphereRelativeOmniOverlay.doesExist)
        {
            // different manner)

            // instead of trying to cache this for every source and adding dirty checks
            // to generate if geometry is actually changed just make this generation code to work fast
            // enough, so that even in worst case when these geometries are changed on every frame
            // by timed metadata, updating will be fast enough
            mGeometryOmni->destroy();
            mGeometryOmni->createSphereRelativeOmniTile(makeVector2(0, 0), makeVector2(widthDegrees, heightDegrees),
                                                        256, 128);


            mesh = mGeometryOmni;
        }

        else
        {
            OMAF_LOG_E("Invalid overlay configuration, must be either sphere or viewport relative");
        }

        // set aspect ratio of temporary source to match the overlay area degrees
        float32_t aspect = widthDegrees / heightDegrees;
        if (aspect > 1)
        {
            source->textureRect.h = 1.f / aspect;
            source->textureRect.w = 1.f;
            source->textureRect.x = 0;
            source->textureRect.y = (1.f - source->textureRect.h) / 2;
        }
        else
        {
            source->textureRect.h = 1.f;
            source->textureRect.w = aspect;
            source->textureRect.x = (1.f - aspect) / 2;
            source->textureRect.y = 0;
        }

        // Bind shader and constants
        VideoShader& selectedShader = useMask ? mMaskShader : mOpaqueShader;

        selectedShader.bind();

// #define USE_RANDOM_TEXTURE
#ifdef USE_RANDOM_TEXTURE
        VideoFrameSource* extSource = (VideoFrameSource*) (renderingParameters.extra);
        selectedShader.bindVideoTexture(extSource->videoFrame);
#else
        selectedShader.bindVideoTexture(overlayFrameSource->videoFrame);
#endif
        OMAF::Quaternion orientation = headTransform.orientation;

        if (source->forcedOrientation.valid)
        {
            orientation = source->forcedOrientation.orientation;
        }

        float32_t rotAroundYRadians = 0;
        float32_t rotAroundXRadians = 0;
        float32_t rotAroundZRadians = 0;

        // by default overlay is not rotated around its axis
        OMAF::Matrix44 overlayRotate = makeTranslation(0, 0, 0);
        OMAF::Matrix44 overlayMoveToCorrectDistance = makeTranslation(0, 0, 0);

        // combine overlay source distance (from overlay structs) with user adjusted distance
        distance = source->distance * distance;

        if (source->overlayParameters.overlaySourceRegion.doesExist)
        {

            // Use textureRect to implement source region, since this is applied after transformation matrix
            // we need to convert these values to match rotated and mirrored texture. 
            // 
            // If this is not suitable for example if this texture rect is used for something else, then this 
            // could be also implemented directly to transformation matrix by first going to center position 
            // of selected source, scale texture so that the source area fills (0,0 ... 1,1) completely and 
            // then move back to new 0,0 coordinate (in scaled world). Or it could be also done after rotate
            // and mirror with same kind of tricks that was done wiht textureRect implementation.
            
            auto& srcRegion = source->overlayParameters.overlaySourceRegion;

            // NOTE: test code for debugging region transformations (dont delete)
            //srcRegion.region.pictureWidth = 100;          
            //srcRegion.region.pictureHeight = 100;
            //srcRegion.region.regionWidth = 40;
            //srcRegion.region.regionHeight = 40;
            //srcRegion.region.regionLeft = 50;
            //srcRegion.region.regionTop = 50;
            //srcRegion.transformType = ISOBMFF::TransformType::ROTATE_CCW_180_BEFORE_MIRRORING;

            auto initialX = (float32_t) srcRegion.region.regionLeft / (float32_t) srcRegion.region.pictureWidth;
            auto initialY = (float32_t) srcRegion.region.regionTop / (float32_t) srcRegion.region.pictureHeight;
            auto initialW = (float32_t) srcRegion.region.regionWidth / (float32_t) srcRegion.region.pictureWidth;
            auto initialH = (float32_t) srcRegion.region.regionHeight / (float32_t) srcRegion.region.pictureHeight;

            // texture rect 0,0 is bottom-left
            overlayFrameSource->textureRect.h = initialH;
            overlayFrameSource->textureRect.y = 1.0 - initialY - initialH;
            overlayFrameSource->textureRect.w = initialW;
            overlayFrameSource->textureRect.x = initialX;
            
            // for some reason normal orientation is not 0 radians
            auto rotate = OMAF::makeRotationZ(OMAF_PI);
            auto mirror = OMAF::makeRotationY(OMAF_PI); 
 
            // normal rotate around Z
            switch (srcRegion.transformType)
            {
            case ISOBMFF::TransformType::ROTATE_CCW_180:
            case ISOBMFF::TransformType::ROTATE_CCW_180_BEFORE_MIRRORING:
                rotate = OMAF::makeRotationZ(0);

                // translate coordinates to rotated texture
                overlayFrameSource->textureRect.x = 1.0 - initialX - initialW;
                overlayFrameSource->textureRect.y = initialY;
                break;

            case ISOBMFF::TransformType::ROTATE_CCW_90_BEFORE_MIRRORING:
            case ISOBMFF::TransformType::ROTATE_CCW_90:
                rotate = OMAF::makeRotationZ(-OMAF_PI / 2);

                // translate coordinates to rotated texture                
                overlayFrameSource->textureRect.x = initialY;
                overlayFrameSource->textureRect.y = initialX;
                overlayFrameSource->textureRect.w = initialH;
                overlayFrameSource->textureRect.h = initialW;
                break;

            case ISOBMFF::TransformType::ROTATE_CCW_270_BEFORE_MIRRORING:
            case ISOBMFF::TransformType::ROTATE_CCW_270:
                rotate = OMAF::makeRotationZ(OMAF_PI / 2);

                // translate coordinates to rotated texture
                overlayFrameSource->textureRect.x = 1.0 - initialY - initialH;
                overlayFrameSource->textureRect.y = 1.0 - initialX - initialW;
                overlayFrameSource->textureRect.w = initialH;
                overlayFrameSource->textureRect.h = initialW;
                break;

            default:
                break;
            }

            // horizontal mirroring is done by rotating texture around Y axis
            switch (srcRegion.transformType)
            {
            case ISOBMFF::TransformType::MIRROR_HORIZONTAL:
            case ISOBMFF::TransformType::ROTATE_CCW_180_BEFORE_MIRRORING:
            case ISOBMFF::TransformType::ROTATE_CCW_90_BEFORE_MIRRORING:
            case ISOBMFF::TransformType::ROTATE_CCW_270_BEFORE_MIRRORING:
                mirror = OMAF::makeRotationY(0);

                // mirror also source region coordinates
                overlayFrameSource->textureRect.x =
                    1.0 - overlayFrameSource->textureRect.x - overlayFrameSource->textureRect.w;
                break;

            default:
                break;
            }

            overlayFrameSource->videoFrame.matrix = OMAF::Matrix44Identity *
                OMAF::makeTranslation(0.5, 0.5, 0) *   // move to center point of textureRect
                rotate *
                mirror *
                OMAF::makeTranslation(-0.5, -0.5, 0);  // move back to top-left coordinate
        }

        if (source->overlayParameters.sphereRelative2DOverlay.doesExist)
        {
            MP4VR::SphereRelative2DOverlay& ovly2d = source->overlayParameters.sphereRelative2DOverlay;

            static float deltaYaw = 0, deltaPitch = 0, deltaRoll = 0, deltaAzimuth = 0, deltaElevation = 0;

            // -- uncomment one for debug to rotate around single axis
            // deltaAzimuth += 0.0005;
            // deltaElevation += 0.0005;
            // deltaYaw += 0.0005;
            // deltaPitch += 0.0005;
            // deltaRoll += 0.0005;

            rotAroundYRadians = OMAF::toRadians(-float(ovly2d.sphereRegion.centreAzimuth) / 65536) + deltaAzimuth;
            rotAroundXRadians = OMAF::toRadians(float(ovly2d.sphereRegion.centreElevation) / 65536) + deltaElevation;

            float32_t yawRad = OMAF::toRadians(-float(ovly2d.overlayRotation.yaw) / 65536);
            float32_t pitchRad = OMAF::toRadians(float(ovly2d.overlayRotation.pitch) / 65536);
            float32_t rollRad = OMAF::toRadians(float(ovly2d.overlayRotation.roll) / 65536);

            OMAF::Matrix44 rotate = makeTranspose(makeMatrix44(
                makeQuaternion(pitchRad + deltaPitch, yawRad + deltaYaw, rollRad + deltaRoll, EulerAxisOrder::ZXY)));

            overlayRotate = rotate;
            overlayMoveToCorrectDistance = makeTranslation(0, 0, -distance);
        }
        else if (source->overlayParameters.sphereRelativeOmniOverlay.doesExist)
        {
            static float deltaTilt = 0, deltaAzimuth = 0, deltaElevation = 0;

            // -- uncomment one for debug to rotate around single axis
            // deltaAzimuth += 0.0005;
            // deltaElevation += 0.0005;
            // deltaTilt += 0.0005;

            rotAroundYRadians = deltaAzimuth +
                -source->overlayParameters.sphereRelativeOmniOverlay.sphereRegion.centreAzimuth * OMAF_PI * 2 / 65536 /
                    360;
            rotAroundXRadians = deltaElevation +
                source->overlayParameters.sphereRelativeOmniOverlay.sphereRegion.centreElevation * OMAF_PI * 2 / 65536 /
                    360;
            rotAroundZRadians = deltaTilt +
                -source->overlayParameters.sphereRelativeOmniOverlay.sphereRegion.centreTilt * OMAF_PI * 2 / 65536 /
                    360;

            // compensate the fact that omni overlays are created on unit sphere
            // (because the same function is used to create background sphere)
            // NOTE: 2020-01-24 regionDepthMinus1 field was still not written to mp4 so omni overlays
            //       get always rendered in origo
            overlayMoveToCorrectDistance = makeTranslation(0, 0, 1 - distance);
        }

        Matrix44 viewMatrix;
        viewMatrix = makeTranspose(makeMatrix44(orientation));


        // make overlays which are the same level with background or further away to stick in background
        // (emulates the effect that background is in infinity)
        float32_t overlayDistanceBackground = 1.0f - distance;
        if (overlayDistanceBackground < 0)
        {
            overlayDistanceBackground = 0;
        }

        if (renderingParameters.dofStyle >= 1)
        {
            // dampen effect of head position so that more near to background the overlay is
            // less head position affects to it (now distance for overlay needs to be set correctly 0..1)

            viewMatrix = viewMatrix *
                makeTranslation(
                             (headTransform.positionOffset.x - headTransform.position.x) * overlayDistanceBackground,
                             (headTransform.positionOffset.y - headTransform.position.y) * overlayDistanceBackground,
                             (headTransform.positionOffset.z - headTransform.position.z) * overlayDistanceBackground);
        }

        Matrix44 viewProjectionMatrix = renderSurface.projection * viewMatrix;
        Matrix44 mvp = viewProjectionMatrix * makeRotationY(rotAroundYRadians) * makeRotationX(rotAroundXRadians) *
            makeRotationZ(rotAroundZRadians) * overlayMoveToCorrectDistance * overlayRotate;

        // pass the texture rectangle information to the shader
        Matrix44 vtm = overlayFrameSource->videoFrame.matrix *
            makeTranslation(overlayFrameSource->textureRect.x, overlayFrameSource->textureRect.y, 0) *
            makeScale(overlayFrameSource->textureRect.w, overlayFrameSource->textureRect.h, 1);

        selectedShader.setDefaultVideoShaderConstants(mvp, vtm, opacity);

        if (useMask)
        {
            selectedShader.bindMaskTexture(renderMask, renderingParameters.clearColor);
        }

        // Bind geometry
        mesh->bind();
        BlendState blendState;

        blendState.blendEnabled = true;
        blendState.blendFunctionSrcRgb = BlendFunction::SRC_ALPHA;
        blendState.blendFunctionDstRgb = BlendFunction::ONE_MINUS_SRC_ALPHA;

        RasterizerState rasterizerState;
        rasterizerState.scissorTestEnabled = true;
        rasterizerState.cullMode = CullMode::NONE;

        RenderBackend::setBlendState(blendState);
        RenderBackend::setRasterizerState(rasterizerState);
        RenderBackend::setScissors(renderSurface.viewport.x, renderSurface.viewport.y, renderSurface.viewport.width,
                                   renderSurface.viewport.height);

// Draw
#ifdef DEBUG_DRAW_TILES
        selectedShader.setTintVideoShaderConstants(makeColor4(1.5f, 1.f, 1.5f, 1.f));
#endif  // DEBUG_DRAW_TILES
        RenderBackend::drawIndexed(PrimitiveType::TRIANGLE_STRIP, 0, (uint32_t) mesh->getIndexCount());

#ifdef DEBUG_DRAW_TILES
        rasterizerState.fillMode = FillMode::WIREFRAME;
        selectedShader.setTintVideoShaderConstants(makeColor4(2.f, 2.f, 2.f, 1.f));
        RenderBackend::setRasterizerState(rasterizerState);
        RenderBackend::drawIndexed(PrimitiveType::TRIANGLE_STRIP, 0, (uint32_t) mesh->getIndexCount());
#endif  // DEBUG_DRAW_TILES
    }

    RenderBackend::popDebugMarker();
}

OMAF_NS_END
