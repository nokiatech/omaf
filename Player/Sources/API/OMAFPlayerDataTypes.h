
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

#include <Platform/OMAFDataTypes.h>
#include <Math/OMAFMathTypes.h>
#include <Math/OMAFColor4.h>

namespace OMAF
{

    /**
     * Version number information
     */
    struct Version
    {
        uint16_t major;       ///< Major version number, incremented when major changes are made
        uint16_t minor;       ///< Minor version number, incremented when interface changes are made
        uint16_t revision;    ///< Revision number, incremented for each new build
    };

    /**
     * Error codes and operation results
     */
    namespace Result
    {
        enum Enum
        {
            OK = 0,                      ///< Operation succeeded normally
            NO_CHANGE,                   ///< Operation didn't have an effect
            END_OF_FILE,                 ///< End of file has been reached
            FILE_PARTIALLY_SUPPORTED,    ///< The file was opened but only part of the contents can be played

            // generic errors
            OUT_OF_MEMORY = 100,         ///< Operation failed due to lack of memory
            OPERATION_FAILED,            ///< Generic failure

            // state errors
            INVALID_STATE = 200,         ///< Operation was not due to invalid state
            ITEM_NOT_FOUND,              ///< Operation failed due to a missing item
            BUFFER_OVERFLOW,             ///< Buffer has overflown

            // input not accepted
            NOT_SUPPORTED = 300,         ///< Action or file is not supported
            INVALID_DATA,                ///< Data is invalid

            // operational errors
            FILE_NOT_FOUND = 400,        ///< File was not found
            FILE_OPEN_FAILED,            ///< The file was found but it can't be accessed
            FILE_NOT_SUPPORTED,          ///< The file is an MP4 but in incorrect format

            NETWORK_ACCESS_FAILED,       ///< Failed to access network

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
            INVALID = -1,   ///< Invalid enum

            HEVC,           ///< H265 / HEVC support
            UHD_PER_EYE,    ///< 4k per eye

            COUNT, ///< Enum max value
        };
    }

    /**
     * Video playback states
     */
    namespace VideoPlaybackState
    {
        enum Enum
        {
            INVALID = -1,      ///< Invalid enum

            IDLE,              ///< Video playback is idle, no file has been loaded
            LOADING,           ///< Loading a video is in progress
            PAUSED,            ///< Video playback is paused
            BUFFERING,         ///< Video playback is buffering, not used currently
            PLAYING,           ///< Video playback is in progress
            END_OF_FILE,       ///< Video has reached the end of the file or stream
            STREAM_NOT_FOUND,  ///< Cannot find the stream on the server
            CONNECTION_ERROR,  ///< Connection error, HTTP operation failed due to an unknown reason
            STREAM_ERROR,      ///< Stream error, stream not compatible or some other similiar error

            COUNT              ///< Enum max value
        };
    };

    /**
     * Return values for the audio interface
     */
    namespace AudioReturnValue
    {
        enum Enum
        {
            INVALID = -1,     ///< Invalid enum

            OK,               ///< Operation succeeded
            END_OF_FILE,      ///< This source has now given all the samples it has, there will be no more.
            OUT_OF_SAMPLES,   ///< Audio is currently out of samples, but it might get more later
            FAIL,             ///< There was unrecoverable error while processing the request.
            NOT_INITIALIZED,  ///< Audio has not been initialized. Do that first, then try again.

            COUNT             ///< Enum max value
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

            FLOAT_1 = 0,   ///< Output samples are floats, from -1 to +1
            FLOAT_16,      ///< Output samples are floats, from -32768f to +32767f   (input is anyway int16 so max for floats is the same)
            INT16,         ///< Output samples are integers, from -32768 to 32767

            COUNT          ///< Enum max value
        };
    };

    /**
     * Playback is enabled for headphones or headphones with head orientation tracking support
     */
    namespace AudioPlaybackMode
    {
        enum Enum
        {
            INVALID = -1,            ///< Invalid enum

            HEADPHONES,              ///< Output is processed for headphones without tracking
            HEADTRACKED_HEADPHONES,  ///< Output is processed for headphones with tracking

            COUNT                    ///< Enum max value
        };
    };

    /**
     * Graphics APIs for the SDK
     */
    namespace GraphicsAPI
    {
        enum Enum
        {
            INVALID     = -1,       ///< Invalid

            OPENGL      = 0x01,     ///< OpenGL 3.0
            OPENGL_ES   = 0x02,     ///< OpenGL ES 2.0 for mobile devices

            VULKAN      = 0x04,     ///< Vulkan, not currently supported

            METAL       = 0x08,     ///< Metal, not currently supported

            D3D11       = 0x10,     ///< DirectX 11
            D3D12       = 0x20,     ///< DirectX 12, not currently supported

            COUNT       = 6         ///< Enum max value
        };
    };

    /**
     * Eye position. Tells whether to render content from the left, the right eye view or the auxiliary view.
     */
    namespace EyePosition
    {
        enum Enum
        {
            INVALID = -1,   ///< Invalid enum

            LEFT,           ///< Left eye. Can also be used to draw mono content
            RIGHT,          ///< Right eye.

            AUXILIARY,       ///< Auxiliary track

            COUNT           ///< Enum max value
        };
    };

    /**
    * Type of the currently loaded media
    */
    namespace StreamType
    {
        enum Enum
        {
            INVALID = -1,           ///< Invalid enum

            LOCAL_FILE,             ///< Local file
            LIVE_STREAM,            ///< Live stream
            VIDEO_ON_DEMAND,        ///< Video-on-demand stream

            COUNT                   ///< Enum max value
        };
    }

    /**
    * Struct containing information about the currently loaded video
    */
    struct MediaInformation
    {
        uint32_t width;                 ///< Width of the source video per eye, note that with live streaming this can change during playback
        uint32_t height;                ///< Height of the source video per eye, note that with live streaming this can change during playback
        bool_t isStereoscopic;          ///< Whether or not this video is stereoscopic
        uint64_t duration;              ///< Duration of the video, 0 if unknown
        StreamType::Enum streamType;    ///< Stream type
    };

    namespace HeadTransformSource
    {
        enum Enum
        {
            INVALID = -1,           ///< Invalid enum
            
            SENSOR,                 ///< HMD or a handheld phone with sensor
            MANUAL,                 ///< desktop player

            COUNT                   ///< Enum max value
        };
    }
    /**
     * Head tranform
     */
    struct HeadTransform
    {
        Quaternion orientation;     ///< Orientation
        HeadTransformSource::Enum source; ///< source of head transform: sensor or handheld
    };

    /**
     * Render surface for rendering
     */
    struct RenderSurface
    {
        uint32_t handle;    ///< Handle to the render target

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

            TEXTURE_2D,     ///< Regular texture
            RESERVED_1,
            TEXTURE_CUBE,   ///< Cubemap texture, only supported with D3D11
            TEXTURE_ARRAY,  ///< Array texture, only supported with D3D11

            RESERVED_2,

            COUNT
        };
    };

    struct RenderTextureDesc
    {
        RenderTextureDesc() : type(TextureType::TEXTURE_2D),arrayIndex(0) {}
        TextureType::Enum type;
        RenderTextureFormat::Enum format;

        uint16_t width;
        uint16_t height;

        uint8_t arrayIndex; ///< Cube face [+x,-x,+y,-y,+z,-z] / Array index [0 .. n] to render into. 
        const void_t* ptr;  ///< Pointer to native texture object. (ID3DTexture2D* or GLuint)
    };

    struct ClipArea
    {
        float32_t centerLatitude;       // degrees, [-90, 90]
        float32_t centerLongitude;      // degrees, [-180, 180]
        float32_t spanLatitude;         // degrees, [0, 180]
        float32_t spanLongitude;        // degrees, [0, 360]
        float32_t opacity;              // [0, 1]
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
        {
        }

        bool displayWaterMark;      ///< Display watermark
        Color4 clearColor;          ///< Rendering clear color
        ClipArea* clipAreas;        ///< Rendering clipping area data
        int16_t clipAreaCount;      ///< Rendering clipping area count
        bool_t renderMonoscopic;    ///< Render in monoscopic
    };
}
