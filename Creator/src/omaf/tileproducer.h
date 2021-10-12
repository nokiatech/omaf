
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

#include <memory>
#include <vector>

#include "async/future.h"
#include "processor/data.h"
#include "processor/processor.h"
#include "tilefilter.h"
#include "tileconfig.h"
#include "controller/controllerconfigure.h"

namespace VDD {

    class TileProducer : public Processor
    {
    public:
        struct Config
        {
            /*
            * Input filename
            */
            std::uint8_t quality;
            OmafTileSets tileConfig;
            ExtractorMode extractorMode;
            Projection projection;
            VideoInputMode videoMode;
            size_t tileCount;
            bool resetExtractorLevelIDCTo51;
        };
        TileProducer(Config& config);

        std::vector<Streams> process(const Streams& aStreams) override;

        StorageType getPreferredStorageType() const override { return StorageType::CPU; }


    private:
        TileFilter mTileFilter;
        OmafTileSets mTileConfig;
        ExtractorMode mExtractorMode;

        size_t mAUIndex;
        int mTileCount;
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
