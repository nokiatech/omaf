
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

#include <memory>
#include <vector>

#include "async/future.h"
#include "processor/source.h"
#include "processor/data.h"
#include "tilefilter.h"
#include "tileconfig.h"
#include "controller/controllerconfigure.h"

namespace VDD {

    class TileProducer : public Source
    {
    public:
        struct Config
        {
            /*
            * Input filename
            */
            std::string inputFileName;
            std::uint8_t quality;
            TileFilter::OmafTileSets tileConfig;
            ExtractorMode extractorMode;
            Projection projection;
            size_t tileCount;
            size_t frameCount;
        };
        TileProducer(Config& config);

        std::vector<Views> produce();

        /** @brief Abort the Source. Next call to produce returns the
        * remaining frames and finally an EndOfStream.
        */
        void abort();

    private:
        TileFilter mTileFilter;
        TileFilter::OmafTileSets mTileConfig;
        ExtractorMode mExtractorMode;
        std::unique_ptr<MP4LoaderSource> mMp4Source;

        size_t mAUIndex;
        int mTileCount;
        uint32_t mFrameCountLimit;
    };

    class WrongTileFilterConfigurationException : public Exception
    {
    public:
        WrongTileFilterConfigurationException(std::string aMessage);

        std::string message() const override;

    private:
        std::string mMessage;
    };

}
