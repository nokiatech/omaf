
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "Foundation/NVRLogger.h"
#include "VAS/NVRVASTilePicker.h"

#if 0 // enable only when needed as can cause frame drops
OMAF_NS_BEGIN
    namespace VDD_LOG
    {
        static FileLogger sFileLog("vaslog.txt");
        inline void_t logDebug(const char_t* zone, const char_t* format, ...)
        {
            va_list args;
            va_start(args, format);

            sFileLog.logDebug(zone, format, args);

            va_end(args);
        }

        inline void_t logTilesDebug(const char_t* title, const VASTileSelection& tiles)
        {
            if (tiles.isEmpty())
            {
                return;
            }
            logDebug("VAS", title);
            logDebug("VAS", "Count: %zd", tiles.getSize());
            for (size_t i = 0; i < tiles.getSize(); i++)
            {
                const VASTileViewport& tile = tiles[i]->getCoveredViewport();
                float64_t fovH;
                float64_t fovV;
                tile.getSpan(fovH, fovV);
                logDebug("VAS", "center: (%f, %f), span (%f, %f)", tile.getCenterLongitude(), tile.getCenterLatitude(), fovH, fovV);
            }

        }

    }

#define VAS_LOG_D(...) VDD_LOG::logDebug("VDD", ## __VA_ARGS__)
#define VAS_LOG_TILES_D(title, tiles) VDD_LOG::logTilesDebug(title, tiles)



OMAF_NS_END
#else
#define VAS_LOG_D(...)
#define VAS_LOG_TILES_D(title, tiles)

#endif
