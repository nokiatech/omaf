
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
#include "processor/processor.h"
#include "processor/data.h"
#include "common/rational.h"

namespace VDD {
    struct Center
    {
        Rational<int32_t> latitude;  // It ranges from -90 to 90 degree
        Rational<int32_t> longitude;  // It ranges from -180 to 180
    };

    struct Span
    {
        Rational<uint32_t> horizontal;  // Horizontal degree coverage of the tile, value between 0 to 360
        Rational<uint32_t> vertical;  // Vertical degree coverage of the tile, value between 0 to 180
    };

    struct TileConfiguration
    {
        Center center;
        Span span;
    };

    class Tiler : public Processor
    {
    public:
        struct Config
        {
            std::vector<TileConfiguration> tileConfig;
        };
        Tiler(Config config);
        StorageType getPreferredStorageType() const override;
        std::vector<Streams> process(const Streams& data) override;

        /** Retrieve the configuration once it has been updated to match the input data */
        Future<Config> getAdjustedConfig() const;

    private:
        struct TileROI
        {
            uint32_t width;
            uint32_t height;
            int32_t offsetX;
            int32_t offsetY;
        };

        void Initialize(RawFrameMeta rawFrameMeta);

        std::vector<TileConfiguration> mTileConfig;
        std::vector<TileROI> mTilesRoi;

        Promise<Config> mAdjustedConfig;
    };

    class WrongTileConfigurationException : public Exception
    {
    public:
        WrongTileConfigurationException(std::string aMessage);

        std::string message() const override;

    private:
        std::string mMessage;
    };

}
