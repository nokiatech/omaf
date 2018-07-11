
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

#include "NVRNamespace.h"

#include "VideoDecoder/NVRVideoColorTypes.h"
#include "Foundation/NVRFixedArray.h"
#include "Platform/OMAFDataTypes.h"
#include "Math/OMAFQuaternion.h"
#include "Math/OMAFMatrix44.h"
#include "Graphics/NVRHandles.h"

OMAF_NS_BEGIN

typedef uint8_t sourceid_t;
typedef uint8_t streamid_t;

namespace SourcePosition
{
    enum Enum
    {
        INVALID = -1,

        LEFT,
        RIGHT,
        MONO,

        AUXILIARY,

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
        IDENTITY_AUXILIARY,
        EQUIRECTANGULAR_TILES,
        EQUIRECTANGULAR_180,
        CUBEMAP_TILES,

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
        CUBE_FACE_ORIENTATION_NO_ROTATION               = 0,
        CUBE_FACE_ORIENTATION_ROTATED_90_RIGHT          = 1 << 0,
        CUBE_FACE_ORIENTATION_ROTATED_180               = 1 << 1, //upside down
        CUBE_FACE_ORIENTATION_ROTATED_90_LEFT           = 1 << 2,
        CUBE_FACE_ORIENTATION_MIRROR_NO_ROTATION        = 1 << 3,
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_RIGHT   = 1 << 4,
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_180        = 1 << 5, //upside down
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_LEFT    = 1 << 6,
    };
}
namespace CubeFaceSectionOrientationIndex
{
    enum Enum
    {
        CUBE_FACE_ORIENTATION_NO_ROTATION = 0,
        CUBE_FACE_ORIENTATION_ROTATED_90_RIGHT = 1,
        CUBE_FACE_ORIENTATION_ROTATED_180 = 2, //upside down
        CUBE_FACE_ORIENTATION_ROTATED_90_LEFT = 3,
        CUBE_FACE_ORIENTATION_MIRROR_NO_ROTATION = 4,
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_RIGHT = 5,
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_180 = 6, //upside down
        CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_LEFT = 7
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
    
    float32_t sourceX;      //normalized tile position in source texture (video frame)
    float32_t sourceY;
    float32_t sourceWidth;  //normalized tile size in source texture
    float32_t sourceHeight;

    CubeFaceSectionOrientation::Enum sourceOrientation;

    float32_t originX;      //normalized tile position inside target face
    float32_t originY;
    float32_t originWidth;  //normalized tile size inside target face
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
    
    uint8_t faceIndex; //front, left, back, right, up, down
    uint8_t numCoordinates;
    VRCubeMapFaceSection sections[MAX_FACE_SECTIONS];   //numCoordinates, i.e. how many parts a single face is divided to; TODO is 4 enough as a max? 1 and 2 are typical values
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
    
    float32_t cubeYaw;      // of the front face
    float32_t cubePitch;
    float32_t cubeRoll;
    uint8_t cubeNumFaces; // number of faces, always 6?? As views are separated anyway?
    VRCubeMapFace cubeFaces[MAX_FACES];
};

struct ForcedOrientation
{
    bool_t valid;
    Quaternion orientation;

    ForcedOrientation() : valid(false) {}
};

struct CoreProviderSource
{
    CoreProviderSource()
    : type(SourceType::INVALID)
    , category(SourceCategory::INVALID)
    , sourceId(OMAF_UINT8_MAX)
    , extrinsicRotation(QuaternionIdentity)
    , sourcePosition(SourcePosition::INVALID)
    {
    }

    SourceType::Enum type;
    SourceCategory::Enum category;
    sourceid_t sourceId;
    Quaternion extrinsicRotation;
    ForcedOrientation forcedOrientation;
    SourcePosition::Enum sourcePosition;
};

namespace VideoPixelFormat
{
    enum Enum
    {
        INVALID = -1,
        
        RGB,
        RGBA,
        RGB_422_APPLE, // https://www.khronos.org/registry/OpenGL/extensions/APPLE/APPLE_rgb_422.txt
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

struct VideoFrameSource
: public CoreProviderSource
{
    VideoFrameSource()
    : streamIndex(OMAF_UINT8_MAX)
    , widthInPixels(0)
    , heightInPixels(0)
    {
        type = SourceType::INVALID;
        category = SourceCategory::VIDEO;
        
        textureRect.x = 0;
        textureRect.y = 0;
        textureRect.w = 0;
        textureRect.h = 0;
    }

    uint8_t streamIndex;

    VideoFrame videoFrame;
    Rect textureRect;

    uint16_t widthInPixels;
    uint16_t heightInPixels;
};

struct PanoramaSource
: public VideoFrameSource
{
    PanoramaSource()
    {
    }
};

struct EquirectangularSource
: public PanoramaSource
{
    EquirectangularSource()
    {
        type = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
};

struct CubeMapSource
: public PanoramaSource
{
    CubeMapSource()
    {
        type = SourceType::CUBEMAP;
    }
    
    VRCubeMap cubeMap;
};

struct IdentitySource
: public PanoramaSource
{
    IdentitySource()
    {
        type = SourceType::IDENTITY;
    }
};

struct IdentityAuxiliarySource
    : public IdentitySource
{
    IdentityAuxiliarySource()
    {
        type = SourceType::IDENTITY_AUXILIARY;
    }
};

struct EquirectangularTileSource
: public PanoramaSource
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

    float32_t centerLatitude;       // degrees, -90..90
    float32_t centerLongitude;      // degrees, -180...180
    float32_t spanLatitude;         // degrees, 0...180
    float32_t spanLongitude;        // degrees, 0...360
    uint8_t relativeQuality;        // 0..100 - TODO is this relevant for the renderer?
};

struct Equirectangular180Source
    : public EquirectangularTileSource
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
    // we use tile source for regions, which is suboptimal since it is based on latitude/longitude output coordinates, which can cause rounding errors. 
    float32_t centerLatitude;       // degrees, -90..90
    float32_t centerLongitude;      // degrees, -180...180
    float32_t spanLatitude;         // degrees, 0...180
    float32_t spanLongitude;        // degrees, 0...360
};

typedef FixedArray<Region,128> RegionPacking;

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
    {}
    SourceDirection::Enum   sourceDirection;
    SourceType::Enum        sourceType;
    uint32_t faceOrder;
    uint32_t faceOrientation;
    RegionPacking erpRegions;
    Rotation rotation;
};

typedef FixedArray<streamid_t, 64> Streams;
typedef FixedArray<CoreProviderSource*, 512> CoreProviderSources;
typedef FixedArray<SourceType::Enum, 64> CoreProviderSourceTypes;

OMAF_NS_END
