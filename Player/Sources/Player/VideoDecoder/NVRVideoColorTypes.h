
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

#include "NVRNamespace.h"

OMAF_NS_BEGIN

// https://www.itu.int/rec/T-REC-H.264
// https://www.itu.int/rec/T-REC-H.265

// Video matrix coefficients
struct VideoMatrixCoefficients
{
    enum Enum
    {
        INVALID = -1,

        RGB = 0,           // The identity matrix
        XYZ = 0,           // The identity matrix
        IEC_SRGB = 0,      // IEC 61966-2-1 (sRGB)
        SMPTE_ST_428 = 0,  // Society of Motion Picture and Television Engineers ST 428-1

        ITU_R_BT_709 = 1,   // Rec. ITU-R BT.709-6
        ITU_R_BT_1361 = 1,  // Rec. ITU-R BT.1361
        IEC_SYCC = 1,       // IEC 61966-2-1 (sYCC)
        IEC_XVYCC_709 = 1,  // IEC 61966-2-4 xvYCC 709
        SMPTE_RP_177 = 1,   // Society of Motion Picture and Television Engineers RP 177

        UNSPECIFIED = 2,  // Image characteristics are unknown or are determined by the application

        // 3, For future use by ITU-T | ISO/IEC

        // 4, United States Federal Communications Commission Title 47 Code of Federal Regulations (2003) 73.682 (a)
        // (20)

        ITU_R_BT_470_BG = 5,    // Rec. ITU-R BT.470-6 System B, G
        ITU_R_BT_601_625 = 5,   // Rec. ITU-R BT.601-6 625
        ITU_R_BT_1358_625 = 5,  // Rec. ITU-R BT.1358 625
        ITU_R_BT_1700_625 = 5,  // ITU-R Rec. BT.1700-0 625 PAL and 625 SECAM
        IEC_XVYCC_601 = 5,      // IEC 61966-2-4 xvYCC 601

        ITU_R_BT_601_525 = 6,   // Rec. ITU-R BT.601-6 525
        ITU_R_BT_1358_525 = 6,  // Rec. ITU-R BT.1358 525
        ITU_R_BT_1700 = 6,      // ITU-R Rec. BT.1700-0 NTSC
        SMPTE_170M = 6,         // Society of Motion Picture and Television Engineers 170M

        SMPTE_240M = 7,  // Society of Motion Picture and Television Engineers 240M

        YCGCO = 8,  // YCgCo

        ITU_R_BT_2020_NCL = 9,  // Rec. ITU-R BT.2020-2 non-constant luminance system

        ITU_R_BT_2020_CL = 10,  // Rec. ITU-R BT.2020-2 constant luminance system

        SMPTE_ST_2085 = 11,  // Society of Motion Picture and Television Engineers ST 2085

        CHROMATICITY_DERIVED_NLC = 12,  // Chromaticity-derived non-constant luminance system
        CHROMATICITY_DERIVED_CL = 13,   // Chromaticity-derived constant luminance system

        ITU_R_BT_2100 = 14,  // Rec. ITU-R BT.2100-0 ICTCP

        // 15..255, For future use by ITU-T | ISO/IEC

        COUNT
    };
};

// Video color ranges
struct VideoColorRange
{
    enum Enum
    {
        INVALID = -1,

        UNSPECIFIED,

        FULL,     // PC, full range (0-255 for both luma and chroma)
        LIMITED,  // TV, limited range (16-235 for luma, 16-240 for chroma)

        COUNT
    };
};

// Video color primaries / chromacities
struct VideoColorPrimaries
{
    enum Enum
    {
        INVALID = -1,

        // 0, For future use by ITU-T | ISO/IEC

        ITU_R_BT_709 = 1,   // Rec. ITU-R BT.709-5
        ITU_R_BT_1361 = 1,  // Rec. ITU-R BT.1361-0
        IEC_SRGB = 1,       // IEC 61966-2-1 sRGB
        IEC_SYCC = 1,       // IEC 61966-2-1 sYCC
        IEC_XVYCC = 1,      // IEC 61966-2-4
        SMPTE_RP_177 = 1,   // Society of Motion Picture and Television Engineers RP 177

        UNSPECIFIED = 2,  // Image characteristics are unknown or are determined by the application.

        // 3, For future use by ITU-T | ISO/IEC

        ITU_R_BT_470_M = 4,  // Rec. ITU-R BT.470-6 System M

        ITU_R_BT_470_BG = 5,    // Rec. ITU-R BT.470-6 System B, G
        ITU_R_BT_601_625 = 5,   // Rec. ITU-R BT.601-6 625
        ITU_R_BT_1358_625 = 5,  // Rec. ITU-R BT.1358-1 625
        ITU_R_BT_1700_625 = 5,  // Rec. ITU-R BT.1700-0 625 PAL and 625 SECAM

        ITU_R_BT_601_525 = 6,   // Rec. ITU-R BT.601-6 525
        ITU_R_BT_1358_525 = 6,  // Rec. ITU-R BT.1358-1 525
        ITU_R_BT_1700 = 6,      // Rec. ITU-R BT.1700-0 NTSC
        SMPTE_170M = 6,         // Society of Motion Picture and Television Engineers 170M

        SMPTE_240M = 7,  // Society of Motion Picture and Television Engineers 240M

        GENERIC_FILM = 8,  // Generic film (colour filters using Illuminant C)

        ITU_R_BT_2020 = 9,  // Rec. ITU-R BT.2020-2
        ITU_R_BT_2100 = 9,  // Rec. ITU-R BT.2100

        SMPTE_ST_428 = 10,  // Society of Motion Picture and Television Engineers ST 428-1

        SMPTE_RP_431 = 11,  // Society of Motion Picture and Television Engineers RP 431-2

        SMPTE_EG_432 = 12,  // Society of Motion Picture and Television Engineers EG 432-1

        // 13..21, For future use by ITU-T | ISO/IEC

        EBU_TECH_3213 = 22,  // EBU Tech. 3213-E

        // 23..255, For future use by ITU-T | ISO/IEC

        COUNT,
    };
};

// Video transfer characteristics
struct VideoTransferCharacteristics
{
    enum Enum
    {
        INVALID = -1,

        // 0, For future use by ITU-T | ISO/IEC

        ITU_R_BT_709 = 1,        // Rec. ITU-R BT.709-5
        ITU_R_BT_1361_CCGS = 1,  // Rec. ITU-R BT.1361-0 conventional colour gamut system

        UNSPECIFIED = 2,  // Image characteristics are unknown or are determined by the application.

        // 3, For future use by ITU-T | ISO/IEC

        GAMMA_22 = 4,           // Assumed display gamma 2.2
        ITU_R_BT_470_M = 4,     // Rec. ITU-R BT.470-6 System M
        ITU_R_BT_1700_625 = 4,  // Rec. ITU-R BT.1700-0 625 PAL and 625 SECAM

        GAMMA_28 = 5,         // Assumed display gamma 2.8
        ITU_R_BT_470_BG = 5,  // Rec. ITU-R BT.470-6 System B, G

        ITU_R_BT_601_525 = 6,   // Rec. ITU-R BT.601-6 525
        ITU_R_BT_601_625 = 6,   // Rec. ITU-R BT.601-6 625
        ITU_R_BT_1358_525 = 6,  // Rec. ITU-R BT.1358-1 525
        ITU_R_BT_1358_625 = 6,  // Rec. ITU-R BT.1358-1 625
        ITU_R_BT_1700 = 6,      // Rec. ITU-R BT.1700-0 NTSC
        SMPTE_170M = 6,         // Society of Motion Picture and Television Engineers 170M

        SMPTE_240M = 7,  // Society of Motion Picture and Television Engineers 240M

        LINEAR = 8,  // Linear transfer characteristics

        LOG_100 = 9,  // Logarithmic transfer characteristic (100:1 range)

        LOG_SQRT = 10,  // Logarithmic transfer characteristic (100 * Sqrt( 10 ) : 1 range)

        IEC_XVYCC = 11,  // IEC 61966-2-4

        ITU_R_BT_1361_ECGS = 12,  // Rec. ITU-R BT.1361-0 extended colour gamut system

        IEC_SRGB = 13,  // IEC 61966-2-1 sRGB
        IEC_SYCC = 13,  // IEC 61966-2-1 sYCC

        ITU_R_BT_2020_10 = 14,  // Rec. ITU-R BT.2020-2 (10 bit system) (functionally the same as the values 1, 6, and
                                // 15; the value 1 is preferred)

        ITU_R_BT_2020_12 = 15,  // Rec. ITU-R BT.2020-2 (12 bit system) (functionally the same as the values 1, 6, and
                                // 15; the value 1 is preferred)

        SMPTE_ST_2048 =
            16,  // Society of Motion Picture and Television Engineers ST 2084 for 10, 12, 14, and 16-bit systems
        ITU_R_BT_2100_PQ = 16,  // Rec. ITU-R BT.2100 perceptual quantization (PQ) system

        SMPTE_ST_428 = 17,  // Society of Motion Picture and Television Engineers ST 428-1

        ARIB_STD_B67 = 18,      // Association of Radio Industries and Businesses (ARIB) STD-B67
        ITU_R_BT2100_HLG = 18,  // Rec. ITU-R BT.2100-0 hybrid log-gamma (HLG) system

        // 19..255, For future use by ITU-T | ISO/IEC

        COUNT
    };
};

struct VideoColorInfo
{
    VideoColorInfo()
        : matrixCoefficients(VideoMatrixCoefficients::INVALID)
        , colorRange(VideoColorRange::INVALID)
        , colorPrimaries(VideoColorPrimaries::INVALID)
        , transferCharacteristics(VideoTransferCharacteristics::INVALID)
    {
    }

    VideoMatrixCoefficients::Enum matrixCoefficients;
    VideoColorRange::Enum colorRange;
    VideoColorPrimaries::Enum colorPrimaries;
    VideoTransferCharacteristics::Enum transferCharacteristics;
};

OMAF_NS_END
