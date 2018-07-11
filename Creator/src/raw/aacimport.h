
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
#include <fstream>

#include "processor/source.h"

namespace VDD
{
    /** @brief Thrown when ADTS header parsing fails or when the frequency is not indicated in the
     * header (frequency index 15 or one of the reserved ones is used) */
    class UnsupportedFormat : public Exception
    {
    public:
        UnsupportedFormat();
    };

    /** @brief AACImport reads an AAC with ADTS headers, producing one raw AAC-encoded frame without
     * ADTS header at a time. Possible CRC in the ADTS headers is ignored. */
    class AACImport : public Source
    {
    public:
        struct Config
        {
            std::string dataFilename;
        };

        AACImport(Config aConfig);
        ~AACImport() override;

        std::vector<Views> produce() override;

    private:
        Config mConfig;
        std::ifstream mInput;
        Index mIndex = 0;
        FrameDuration mFrameDuration;
        std::vector<uint8_t> mDecoderConfig;
        Bitrate mBitrate;
    };
}
