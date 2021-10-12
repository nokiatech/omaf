
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
#pragma once

#include <reader/mp4vrfiledatatypes.h>

#include "NVRNamespace.h"

#include "Foundation/NVRFixedArray.h"
#include "Graphics/NVRHandles.h"
#include "Math/OMAFMatrix44.h"
#include "Math/OMAFQuaternion.h"
#include "Platform/OMAFDataTypes.h"
#include "VideoDecoder/NVRVideoColorTypes.h"

OMAF_NS_BEGIN

typedef uint32_t sourceid_t;
typedef uint8_t streamid_t;
#define InvalidStreamId OMAF_UINT8_MAX

namespace SourcePosition
{
    enum Enum
    {
        INVALID = -1,

        LEFT,
        RIGHT,
        MONO,

        COUNT
    };
}

namespace SourceType
{
    enum Enum
    {
        INVALID = -1,

        EQUIRECTANGULAR_PANORAMA,
        CUBEMAP,
        IDENTITY,
        EQUIRECTANGULAR_TILES,
        EQUIRECTANGULAR_180,
        CUBEMAP_TILES,
        OVERLAY,

        RAW,

        COUNT
    };
}

namespace SourceDirection
{
    enum Enum
    {
        INVALID = -1,

        MONO,
        LEFT_RIGHT,
        RIGHT_LEFT,
        TOP_BOTTOM,
        BOTTOM_TOP,
        DUAL_TRACK,

        COUNT
    };
}

namespace SourceCategory
{
    enum Enum
    {
        INVALID = -1,

        VIDEO,
        TEXT,
        COLLECTION,

        COUNT
    };
}

namespace DepthQuantizationType
{
    enum Enum
    {
        INVALID = -1,

        LOGARITMIC,
        LINEAR,

        COUNT
    };
}

namespace CubeFaceSectionOrientation
{
    enum Enum
    {
        CUBE_FACE_ORIENTATION_NO_ROTATION = 0,
        CUBE_FACE_ORIENTATION_ROTATED_90_RIGHT = 1 << 0,
        CUBE_FACE_ORIENTATION_ROTATED_180 = 1 << 1,  // upside down
        CUBE_FACE_ORIENTATION_ROTATED_90_LEFT = 1 << 2,
        CUBE_FACE_ORIENTATION_MIRROR_NO_ROTATION = 1 << 3,
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_RIGHT = 1 << 4,
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_180 = 1 << 5,  // upside down
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_LEFT = 1 << 6,
    };
}
namespace CubeFaceSectionOrientationIndex
{
    enum Enum
    {
        CUBE_FACE_ORIENTATION_NO_ROTATION = 0,
        CUBE_FACE_ORIENTATION_ROTATED_90_RIGHT = 1,
        CUBE_FACE_ORIENTATION_ROTATED_180 = 2,  // upside down
        CUBE_FACE_ORIENTATION_ROTATED_90_LEFT = 3,
        CUBE_FACE_ORIENTATION_MIRROR_NO_ROTATION = 4,
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_RIGHT = 5,
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_180 = 6,  // upside down
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_LEFT = 7
    };
}

/**
 * Information to be able to decide in which order certain sources should be rendered.
 */
namespace SourceMediaPlacement
{
    enum Enum
    {
        INVALID = -1,
        BACKGROUND,
        SPHERE_RELATIVE_OVERLAY,
        VIEWPORT_RELATIVE_OVERLAY
    };
}

struct VRCubeMapFaceSection
{
    VRCubeMapFaceSection()
        : sourceX(OMAF_FLOAT32_NAN)
        , sourceY(OMAF_FLOAT32_NAN)
        , sourceWidth(OMAF_FLOAT32_NAN)
        , sourceHeight(OMAF_FLOAT32_NAN)
        , sourceOrientation(CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION)
        , originX(OMAF_FLOAT32_NAN)
        , originY(OMAF_FLOAT32_NAN)
        , originWidth(OMAF_FLOAT32_NAN)
        , originHeight(OMAF_FLOAT32_NAN)
    {
    }

    float32_t sourceX;  // normalized tile position in source texture (video frame)
    float32_t sourceY;
    float32_t sourceWidth;  // normalized tile size in source texture
    float32_t sourceHeight;

    CubeFaceSectionOrientation::Enum sourceOrientation;

    float32_t originX;  // normalized tile position inside target face
    float32_t originY;
    float32_t originWidth;  // normalized tile size inside target face
    float32_t originHeight;
};

struct VRCubeMapFace
{
    static const uint8_t MAX_FACE_SECTIONS = 4;

    VRCubeMapFace()
        : faceIndex(OMAF_UINT8_MAX)
        , numCoordinates(0)
    {
    }

    uint8_t faceIndex;  // front, left, back, right, up, down
    uint8_t numCoordinates;
    VRCubeMapFaceSection sections[MAX_FACE_SECTIONS];  // numCoordinates, i.e. how many parts a single face is divided
};

struct VRCubeMap
{
    static const uint8_t MAX_FACES = 6;

    VRCubeMap()
        : cubeYaw(OMAF_FLOAT32_NAN)
        , cubePitch(OMAF_FLOAT32_NAN)
        , cubeRoll(OMAF_FLOAT32_NAN)
        , cubeNumFaces(0)
    {
    }

    float32_t cubeYaw;  // of the front face
    float32_t cubePitch;
    float32_t cubeRoll;
    uint8_t cubeNumFaces;  // number of faces, always 6?? As views are separated anyway?
    VRCubeMapFace cubeFaces[MAX_FACES];
};

struct ForcedOrientation
{
    bool_t valid;
    Quaternion orientation;

    ForcedOrientation()
        : valid(false)
    {
    }
};

class VideoRenderer;  // should not be stored here or may cause problems in future... cyclic reference stuff
struct CoreProviderSource
{
    CoreProviderSource()
        : type(SourceType::INVALID)
        , category(SourceCategory::INVALID)
        , sourceId(OMAF_UINT32_MAX)
        , extrinsicRotation(QuaternionIdentity)
        , sourcePosition(SourcePosition::INVALID)
        , omitRender(false)
        , mediaPlacement(SourceMediaPlacement::INVALID)
        , sourceGroup(-1)
        , renderer(OMAF_NULL)
        , tempTexture(0)
        , tempTarget(0)

    {
    }

    SourceType::Enum type;
    SourceCategory::Enum category;
    sourceid_t sourceId;
    Quaternion extrinsicRotation;
    ForcedOrientation forcedOrientation;
    ForcedOrientation initialViewingOrientation;
    SourcePosition::Enum sourcePosition;
    bool omitRender;  // if source should be decoded, but not necessarily rendered (video sources used by overlays)
    SourceMediaPlacement::Enum mediaPlacement;

    // NOTE: not sure how in future real entity groups will be represented, but this works for now
    int32_t sourceGroup;

    //
    // this is temporary solution to pass information needed by overlay rendering to
    // renderers
    VideoRenderer* renderer;
    TextureID tempTexture;
    RenderTargetID tempTarget;
};

namespace VideoPixelFormat
{
    enum Enum
    {
        INVALID = -1,

        RGB,
        RGBA,
        RGB_422_APPLE,  // https://www.khronos.org/registry/OpenGL/extensions/APPLE/APPLE_rgb_422.txt
        YUV_420,
        NV12,

        COUNT
    };
}

struct VideoFrame
{
    static const uint8_t MAX_TEXTURES = 8;

    VideoFrame()
        : format(VideoPixelFormat::INVALID)
        , width(0)
        , height(0)
        , pts(OMAF_UINT64_MAX)
        , duration(OMAF_UINT64_MAX)
        , matrix(Matrix44Identity)
        , numTextures(0)
    {
        for (uint8_t i = 0; i < MAX_TEXTURES; ++i)
        {
            textures[i] = TextureID::Invalid;
        }
    }

    VideoPixelFormat::Enum format;

    uint32_t width;
    uint32_t height;

    uint64_t pts;
    uint64_t duration;

    Matrix44 matrix;

    TextureID textures[MAX_TEXTURES];
    uint8_t numTextures;

    VideoColorInfo colorInfo;
};

struct VideoFrameSource : public CoreProviderSource
{
    VideoFrameSource()
        : streamIndex(InvalidStreamId)
        , widthInPixels(0)
        , heightInPixels(0)
    {
        type = SourceType::INVALID;
        category = SourceCategory::VIDEO;
        mediaPlacement = SourceMediaPlacement::BACKGROUND;

        textureRect.x = 0;
        textureRect.y = 0;
        textureRect.w = 0;
        textureRect.h = 0;
    }

    streamid_t streamIndex;

    VideoFrame videoFrame;
    Rect textureRect;

    uint16_t widthInPixels;
    uint16_t heightInPixels;
};

/**
 * Overlay source wraps up underlying visual media source and
 * has an additional info how it should be rendered.
 */
struct OverlaySource : public VideoFrameSource
{
    OverlaySource()
    {
        type = SourceType::OVERLAY;
        category = SourceCategory::VIDEO;
        mediaPlacement = SourceMediaPlacement::INVALID;  // need to be set later

        widthInPixels = 720;
        heightInPixels = 720;
        distance = 1.0;  // distance adjustment
    }

    //       or something like that, becuase sources represents
    //       visible media which is already assigned tobe be
    //       rendered in certains eye etc.
    VideoFrameSource* mediaSource;

    bool_t userActivatable = false;
    bool_t active = true;

    float32_t distance;

    // parameters from user interacitions while still maintaining the latest values
    // from timed metadata / ovly boxes

    // uptodate overlay parameters
    MP4VR::SingleOverlayProperty overlayParameters;
};

struct PanoramaSource : public VideoFrameSource
{
    PanoramaSource()
    {
    }
};

struct EquirectangularSource : public PanoramaSource
{
    EquirectangularSource()
    {
        type = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
};

struct CubeMapSource : public PanoramaSource
{
    CubeMapSource()
    {
        type = SourceType::CUBEMAP;
    }

    VRCubeMap cubeMap;
};

struct IdentitySource : public PanoramaSource
{
    IdentitySource()
    {
        type = SourceType::IDENTITY;
    }
};

struct EquirectangularTileSource : public PanoramaSource
{
    EquirectangularTileSource()
        : centerLatitude(OMAF_FLOAT32_NAN)
        , centerLongitude(OMAF_FLOAT32_NAN)
        , spanLatitude(OMAF_FLOAT32_NAN)
        , spanLongitude(OMAF_FLOAT32_NAN)
        , relativeQuality(OMAF_UINT8_MAX)
    {
        type = SourceType::EQUIRECTANGULAR_TILES;
    }

    float32_t centerLatitude;   // degrees, -90..90
    float32_t centerLongitude;  // degrees, -180...180
    float32_t spanLatitude;     // degrees, 0...180
    float32_t spanLongitude;    // degrees, 0...360
    uint8_t relativeQuality;
};

struct Equirectangular180Source : public EquirectangularTileSource
{
    Equirectangular180Source()
    {
        type = SourceType::EQUIRECTANGULAR_180;
        centerLatitude = 0.0f;
        centerLongitude = 0.0f;
        spanLatitude = 180.0f;
        spanLongitude = 180.0f;
    }
};

struct Region
{
    Rect inputRect;
    // we use tile source for regions, which is suboptimal since it is based on latitude/longitude output coordinates,
    // which can cause rounding errors.
    float32_t centerLatitude;   // degrees, -90..90
    float32_t centerLongitude;  // degrees, -180...180
    float32_t spanLatitude;     // degrees, 0...180
    float32_t spanLongitude;    // degrees, 0...360
};

typedef FixedArray<Region, 128> RegionPacking;

struct Rotation
{
    bool_t valid;
    float32_t yaw;
    float32_t pitch;
    float32_t roll;
};

struct BasicSourceInfo
{
    BasicSourceInfo()
        : sourceType(SourceType::INVALID)
        , sourceDirection(SourceDirection::INVALID)
        , faceOrder(0)
        , faceOrientation(0)
        , rotation({false, 0.f, 0.f, 0.f})
    {
    }
    SourceDirection::Enum sourceDirection;
    SourceType::Enum sourceType;
    uint32_t faceOrder;
    uint32_t faceOrientation;
    RegionPacking erpRegions;
    VRCubeMap tiledCubeMap;
    Rotation rotation;
};


typedef FixedArray<streamid_t, 256> Streams;
typedef FixedArray<CoreProviderSource*, 512> CoreProviderSources;
typedef FixedArray<SourceType::Enum, 64> CoreProviderSourceTypes;

OMAF_NS_END
