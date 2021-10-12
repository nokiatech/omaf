
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

#include <Math/OMAFColor4.h>
#include <Math/OMAFMathTypes.h>
#include <Platform/OMAFDataTypes.h>

namespace OMAF
{
    /**
     * Version number information
     */
    struct Version
    {
        uint16_t major;     ///< Major version number, incremented when major changes are made
        uint16_t minor;     ///< Minor version number, incremented when interface changes are made
        uint16_t revision;  ///< Revision number, incremented for each new build
    };

    /**
     * Error codes and operation results
     */
    namespace Result
    {
        enum Enum
        {
            OK = 0,                    ///< Operation succeeded normally
            NO_CHANGE,                 ///< Operation didn't have an effect
            END_OF_FILE,               ///< End of file has been reached
            FILE_PARTIALLY_SUPPORTED,  ///< The file was opened but only part of the contents can be played

            // generic errors
            OUT_OF_MEMORY = 100,  ///< Operation failed due to lack of memory
            OPERATION_FAILED,     ///< Generic failure

            // state errors
            INVALID_STATE = 200,  ///< Operation was not due to invalid state
            ITEM_NOT_FOUND,       ///< Operation failed due to a missing item
            BUFFER_OVERFLOW,      ///< Buffer has overflown

            // input not accepted
            NOT_SUPPORTED = 300,  ///< Action or file is not supported
            INVALID_DATA,         ///< Data is invalid

            // operational errors
            FILE_NOT_FOUND = 400,  ///< File was not found
            FILE_OPEN_FAILED,      ///< The file was found but it can't be accessed
            FILE_NOT_SUPPORTED,    ///< The file is an MP4 but in incorrect format

            NETWORK_ACCESS_FAILED,  ///< Failed to access network

            DEPRECATED
        };
    };

    namespace SeekAccuracy
    {
        enum Enum
        {
            INVALID = -1,

            NEAREST_SYNC_FRAME,
            FRAME_ACCURATE,

            COUNT
        };
    }

    /**
     * Features which depend on the capability of the current device
     */
    namespace Feature
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            HEVC,         ///< H265 / HEVC support
            UHD_PER_EYE,  ///< 4k per eye

            COUNT,  ///< Enum max value
        };
    }

    /**
     * Video playback states
     */
    namespace VideoPlaybackState
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            IDLE,              ///< Video playback is idle, no file has been loaded
            LOADING,           ///< Loading a video is in progress
            PAUSED,            ///< Video playback is paused
            BUFFERING,         ///< Video playback is buffering, not used currently
            PLAYING,           ///< Video playback is in progress
            SWITCHING_VIEWPOINT,///< Switching to another viewpoint (camera) in progress
            END_OF_FILE,       ///< Video has reached the end of the file or stream
            STREAM_NOT_FOUND,  ///< Cannot find the stream on the server
            CONNECTION_ERROR,  ///< Connection error, HTTP operation failed due to an unknown reason
            STREAM_ERROR,      ///< Stream error, stream not compatible or some other similiar error

            COUNT  ///< Enum max value
        };
    };

    /**
     * Return values for the audio interface
     */
    namespace AudioReturnValue
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            OK,               ///< Operation succeeded
            END_OF_FILE,      ///< This source has now given all the samples it has, there will be no more.
            OUT_OF_SAMPLES,   ///< Audio is currently out of samples, but it might get more later
            FAIL,             ///< There was unrecoverable error while processing the request.
            NOT_INITIALIZED,  ///< Audio has not been initialized. Do that first, then try again.

            COUNT  ///< Enum max value
        };
    };

    /**
     * AudioRenderer supports int16 and float32 output. This enum indicates the value range of
     * audio sample amplitude. Use also the correct renderSamples-variant
     */
    namespace AudioOutputRange
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            FLOAT_1 = 0,  ///< Output samples are floats, from -1 to +1
            FLOAT_16,     ///< Output samples are floats, from -32768f to +32767f   (input is anyway int16 so max for
                          ///< floats is the same)
            INT16,        ///< Output samples are integers, from -32768 to 32767

            COUNT  ///< Enum max value
        };
    };

    /**
     * Playback is enabled for headphones or headphones with head orientation tracking support
     */
    namespace AudioPlaybackMode
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            HEADPHONES,              ///< Output is processed for headphones without tracking
            HEADTRACKED_HEADPHONES,  ///< Output is processed for headphones with tracking

            COUNT  ///< Enum max value
        };
    };

    /**
     * Graphics APIs for the SDK
     */
    namespace GraphicsAPI
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid

            OPENGL = 0x01,     ///< OpenGL 3.0
            OPENGL_ES = 0x02,  ///< OpenGL ES 2.0 for mobile devices

            VULKAN = 0x04,  ///< Vulkan, not currently supported

            METAL = 0x08,  ///< Metal, not currently supported

            D3D11 = 0x10,  ///< DirectX 11
            D3D12 = 0x20,  ///< DirectX 12, not currently supported

            COUNT = 6  ///< Enum max value
        };
    };

    /**
     * Eye position. Tells whether to render content from the left, the right eye view.
     */
    namespace EyePosition
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            LEFT,   ///< Left eye. Can also be used to draw mono content
            RIGHT,  ///< Right eye.

            COUNT  ///< Enum max value
        };
    };

    /**
     * Type of the currently loaded media
     */
    namespace StreamType
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            LOCAL_FILE,       ///< Local file
            LIVE_STREAM,      ///< Live stream
            LIVE_ON_DEMAND,   ///< Live profile with static type == pre-recorded
            VIDEO_ON_DEMAND,  ///< Video-on-demand stream

            COUNT  ///< Enum max value
        };
    }

    /**
     * Struct containing information about the currently loaded video
     */
    struct MediaInformation
    {
        uint32_t width;   ///< Width of the source video per eye, note that with live streaming this can change during
                          ///< playback
        uint32_t height;  ///< Height of the source video per eye, note that with live streaming this can change during
                          ///< playback
        bool_t isStereoscopic;        ///< Whether or not this video is stereoscopic
        uint64_t durationUs;            ///< Duration of the video, 0 if unknown
        StreamType::Enum streamType;  ///< Stream type
    };

    namespace HeadTransformSource
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            SENSOR,  ///< HMD or a handheld phone with sensor
            MANUAL,  ///< desktop player

            COUNT  ///< Enum max value
        };
    }
    /**
     * Head tranform
     */
    struct HeadTransform
    {
        Quaternion orientation;            ///< Orientation
        Vector3 position;                  ///< Position
        Vector3 positionOffset;            ///< Position offset for recentering etc.
        HeadTransformSource::Enum source;  ///< source of head transform: sensor or handheld
    };

    /**
     * Render surface for rendering
     */
    struct RenderSurface
    {
        uint32_t handle;  ///< Handle to the render target

        struct Viewport
        {
            uint16_t x;
            uint16_t y;
            uint16_t width;
            uint16_t height;
        };

        Viewport viewport;

        EyePosition::Enum eyePosition;

        Matrix44 eyeTransform;
        Matrix44 projection;
    };

    namespace RenderTextureFormat
    {
        enum Enum
        {
            INVALID = -1,

            // Color formats
            R8G8B8A8,

            // Depth / stencil formats
            D16,
            D32F,
            D24S8,
            D32FS8X24,

            COUNT
        };
    };

    namespace TextureType
    {
        enum Enum
        {
            INVALID = -1,

            TEXTURE_2D,  ///< Regular texture
            RESERVED_1,
            TEXTURE_CUBE,   ///< Cubemap texture, only supported with D3D11
            TEXTURE_ARRAY,  ///< Array texture, only supported with D3D11

            RESERVED_2,

            COUNT
        };
    };

    struct RenderTextureDesc
    {
        RenderTextureDesc()
            : type(TextureType::TEXTURE_2D)
            , arrayIndex(0)
        {
        }
        TextureType::Enum type;
        RenderTextureFormat::Enum format;

        uint16_t width;
        uint16_t height;

        uint8_t arrayIndex;  ///< Cube face [+x,-x,+y,-y,+z,-z] / Array index [0 .. n] to render into.
        const void_t* ptr;   ///< Pointer to native texture object. (ID3DTexture2D* or GLuint)
    };

    struct ClipArea
    {
        float32_t centerLatitude;   // degrees, [-90, 90]
        float32_t centerLongitude;  // degrees, [-180, 180]
        float32_t spanLatitude;     // degrees, [0, 180]
        float32_t spanLongitude;    // degrees, [0, 360]
        float32_t opacity;          // [0, 1]
    };


    namespace ActionRegionType
    {
        enum Enum
        {
            INVALID = -1,  ///< Invalid enum

            SPHERE_RELATIVE_OMNI,
            SPHERE_RELATIVE_2D,
            VIEWPORT_RELATIVE,

            COUNT  ///< Enum max value
        };
    }

    struct ActionRegion
    {
        ActionRegionType::Enum type;
        // for viewport relative...
        uint64_t x;
        uint64_t y;
        uint64_t h;
        uint64_t w;
        // for omni and 2d
        float32_t centerLatitude;   // degrees, [-90, 90]
        float32_t centerLongitude;  // degrees, [-180, 180]
        float32_t spanLatitude;     // degrees, [0, 180]
        float32_t spanLongitude;    // degrees, [0, 360]
        float32_t yaw;              // degrees
        float32_t pitch;            // degrees
        float32_t roll;             // degrees
    };

    /**
     * State of overlay source
     */
    struct OverlayState
    {
        bool enabled;
        float32_t distance = 1.0;
    };

    namespace OverlayUserInteraction
    {
        enum Enum
        {
            INVALID = -1,

            MOVE,
            DEPTH,
            SWITCH_ON_OFF,
            OPACITY,
            RESIZE,
            ROTATE,
            SOURCE_SWITCH,

            CHANGE_VIEWPOINT,   // not part of OverlayInteraction

            COUNT
        };
    }

    /**
     * Object for sending control action
     */
    struct OverlayControl
    {
        // overlay id
        uint32_t id = 0;

        OverlayUserInteraction::Enum command;
        union {
            // show / hide overlay (omit value also for this)
            bool enable = true;

            // adjust overlay distance...
            float32_t distance;

            // switch viewpoint
            uint32_t destinationViewpointId;
        };

        // seek stream position to (or should this be offset) -1 means to not to seek
        int64_t seekTo = -1;
    };

    /**
     * Action description of thing that can be done.
     */
    struct UserAction
    {
        ActionRegion actionRegion;
        uint32_t overlayId;

        bool canActivate = false;
        bool canUpdateDistance = false;

        bool canSwitchViewpoint = false;
        uint32_t destinationViewpointId;

        OMAF::OverlayControl createActivateCommand(bool aState) const
        {
            OverlayControl cmd;
            cmd.command = OverlayUserInteraction::SWITCH_ON_OFF;
            cmd.enable = aState;
            cmd.id = overlayId;
            return cmd;
        }
        OMAF::OverlayControl createDistanceCommand(float32_t aDistance) const
        {
            OverlayControl cmd;
            cmd.command = OverlayUserInteraction::DEPTH;
            cmd.distance = aDistance;
            cmd.id = overlayId;
            return cmd;
        }
        OMAF::OverlayControl createViewpointCommand(uint32_t aViewpointId) const
        {
            OverlayControl cmd;
            cmd.command = OverlayUserInteraction::CHANGE_VIEWPOINT;
            cmd.destinationViewpointId = aViewpointId;
            cmd.id = overlayId;
            return cmd;
        }
    };

    /**
     * Additional rendering parameters
     */
    struct RenderingParameters
    {
        RenderingParameters()
            : displayWaterMark(false)
            , clearColor(Color4BlackColor)
            , clipAreas(OMAF_NULL)
            , clipAreaCount(0)
            , renderMonoscopic(false)
            , rawRender(false)
        {
        }

        bool displayWaterMark;    ///< Display watermark
        Color4 clearColor;        ///< Rendering clear color
        ClipArea* clipAreas;      ///< Rendering clipping area data
        int16_t clipAreaCount;    ///< Rendering clipping area count
        bool_t renderMonoscopic;  ///< Render in monoscopic
        bool_t rawRender;         ///< Render 360 video as "raw" decoded output, instead of projecting it to sphere
        uint32_t dofStyle;        ///< 0: 3-dof, 1: 6-dof for overlays, 2: 6-dof for all
        void* extra;              ///< for passing extra data to renderer for debugging purposes
    };


}  // namespace OMAF
