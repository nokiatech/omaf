
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

#include <string>
#include "common/optional.h"
#include "common/utils.h"
#include "config/config.h"
#include "medialoader/mp4loader.h"
#include "medialoader/h265loader.h"

namespace VDD
{
    struct MediaInputConfig
    {
        // if reading segmented input, this is the template
        std::string filename;

        // if reading segmented input, this is the initialization segment
        Optional<std::string> initFilename;

        // if reading segmented input, this is the initial number;
        // if missing, it is 1
        Optional<unsigned> startNumber;

        // Set the GOP length explicitly; used with H265
        Optional<int> gopLength;

        MP4Loader::Config getMP4LoaderConfig() const
        {
            return MP4Loader::Config{filename, initFilename, startNumber};
        }

        H265Loader::Config getH265LoaderConfig() const
        {
            if (!gopLength)
            {
                throw ConfigValueReadError("H265 input requires gop length to be set");
            }
            if (initFilename)
            {
                throw ConfigValueReadError("H265 does not use init file name");
            }
            return H265Loader::Config{filename, gopLength, startNumber};
        }

        std::unique_ptr<MediaLoader> makeMediaLoader(bool aVideoNalStartCodes) const;
    };

    // this may consume the source completely
    Optional<FrameDuration> getMediaDurationConsumingSource(MediaSource& aMediaSource);
}  // namespace VDD
