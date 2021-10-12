
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
#ifndef MEDIATYPEDEFS_HPP
#define MEDIATYPEDEFS_HPP

#include <stdexcept>
#include <string>
#include "customallocator.hpp"

/** @brief Supported bitstream media types */
enum class MediaType
{
    AVC,
    HEVC,
    INVALID
};

/** Helpers for media type - code type - bitstream type conversions */
namespace MediaTypeTool
{
    /** Get MediaType for code type string */
    inline MediaType getMediaTypeByCodeType(const String codeType,
                                            const String fileNameForErrorMsg)
    {
        if (codeType == "avc1" || codeType == "avc3")
        {
            return MediaType::AVC;
        }
        else if (codeType == "hvc1" || codeType == "hev1")
        {
            return MediaType::HEVC;
        }
        else
        {
            // Unsupported code type
            String fileInfo = (fileNameForErrorMsg.empty()) ? "" : " (" + fileNameForErrorMsg + ")";

            if (codeType.empty())
            {
                throw RuntimeError("Failed to define media type, code_type not set" + fileInfo);
            }
            else
            {
                throw RuntimeError("Failed to define media type for unsupported code_type '" +
                                   codeType + "'" + fileInfo);
            }
        }
    }

    /** Get "friendly" bitstream type name for MediaType, used only for logging (can be changed!) */
    inline const String getMP4VRImpl::BitStreamTypeName(MediaType mediaType)
    {
        switch (mediaType)
        {
        case MediaType::AVC:
        {
            return "AVC";
            break;
        }
        case MediaType::HEVC:
        {
            return "HEVC";
            break;
        }
        default:
        {
            // Invalid media type
            return "INVALID";
            break;
        }
        }
    }
}

#endif
