
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
#include <math.h>
#include <sstream>
#include "./tiler.h"
#include "processor/data.h"
#include "log/logstream.h"

namespace VDD {

    WrongTileConfigurationException::WrongTileConfigurationException(std::string aMessage)
        : Exception("WrongTileConfigurationException")
        , mMessage(aMessage)
    {
        // nothing
    }

    std::string WrongTileConfigurationException::message() const
    {
        return mMessage;
    }

    Tiler::Tiler(Config config)
        : mTileConfig(config.tileConfig)
    {
    }

    Future<Tiler::Config> Tiler::getAdjustedConfig() const
    {
        return mAdjustedConfig;
    }

    void Tiler::Initialize(RawFrameMeta rawFrameMeta)
    {
        uint32_t width = rawFrameMeta.width;
        uint32_t height = rawFrameMeta.height;
        double tileWidth, tileHeight;
        double tileCenterX, tileCenterY;
        for (size_t i = 0; i < mTileConfig.size(); i++)
        {
            TileROI tile;
            if ((mTileConfig[i].span.horizontal.num / mTileConfig[i].span.horizontal.den > 360)
                || (mTileConfig[i].span.vertical.num / mTileConfig[i].span.vertical.den > 180))
            {
                std::ostringstream buf;
                buf << "Error initializing tiler, the span of tile " << i << " is out of range";
                throw WrongTileConfigurationException(buf.str());
            }
            if ((mTileConfig[i].center.longitude.num / mTileConfig[i].center.longitude.den  > 180 || mTileConfig[i].center.longitude.num / mTileConfig[i].center.longitude.den  < -180)
                || (mTileConfig[i].center.latitude.num / mTileConfig[i].center.latitude.den > 90 || mTileConfig[i].center.latitude.num / mTileConfig[i].center.latitude.den < -90))
            {
                std::ostringstream buf;
                buf << "Error initializing tiler, the center of tile " << i << " is out of range";
                throw WrongTileConfigurationException(buf.str());
            }

            tileWidth = (double)(width * mTileConfig[i].span.horizontal.num) / (double)(360 * mTileConfig[i].span.horizontal.den);
            tileCenterX = (double)(((int32_t)width * mTileConfig[i].center.longitude.num) / (double)(360 * mTileConfig[i].center.longitude.den)) + (width / 2);

            if (tileWidth != floor(tileWidth) || tileCenterX != floor(tileCenterX) || ((uint32_t)tileWidth % 2) != 0)
            {
                // round upwards => creates overlap. Wrap around is allowed
                double left = floor(tileCenterX - tileWidth / 2);
                double right = ceil(tileCenterX + tileWidth / 2);

                tileWidth = right - left;
#if (VAAPI)
                if (((uint32_t)tileWidth % 8) != 0)
                {
                    int tileWidthOrg = tileWidth;
                    tileWidth = ((uint32_t)tileWidth / 8) * 8 + 8;
                    int deltaWidth = tileWidth - tileWidthOrg;
                    if (deltaWidth % 2 == 0)
                    {
                        right += deltaWidth/2;
                    }
                    else
                    {
                        right += deltaWidth/2 + 1;
                    }
                }
#else
                if (((uint32_t)tileWidth % 2) != 0)
                {
                    // encoder requires even pixel dimensions
                    tileWidth++;
                    right++;
                }
#endif
                tileCenterX = right - tileWidth/2;

                mTileConfig[i].center.longitude.num = (int32_t)((tileCenterX - (width/2)) * 360);
                if (mTileConfig[i].center.longitude.num == 0)
                {
                    mTileConfig[i].center.longitude.den = 1;
                }
                else
                {
                    mTileConfig[i].center.longitude.den = static_cast<int32_t>(width);
                }

                mTileConfig[i].span.horizontal.num = static_cast<uint32_t>(tileWidth * 360);
                mTileConfig[i].span.horizontal.den = width;
            }

            tileHeight = (double)(height * mTileConfig[i].span.vertical.num) / (180 * mTileConfig[i].span.vertical.den);
            // image coordinate (0,0) presents top-left corner, but in equirect coordinates latitude increases from bottom to top (-90 is bottom, +90 top)
            tileCenterY = (height / 2) - (double)(((int32_t)height * mTileConfig[i].center.latitude.num) / (double)(180 * mTileConfig[i].center.latitude.den));
            if (tileHeight != floor(tileHeight) || tileCenterY != floor(tileCenterY) || ((uint32_t)tileHeight % 2) != 0)
            {
                // round upwards => creates overlap
                double top = floor(tileCenterY - tileHeight / 2);
                double bottom = ceil(tileCenterY + tileHeight / 2);

                tileHeight = bottom - top;
                if (((uint32_t)tileHeight % 2) != 0)
                {
                    // encoder requires even pixel dimensions
                    tileHeight++;
                    bottom++;
                }
                // we don't support wrap around in vertical, so shift the tile
                while (top < 0)
                {
                    top++;
                    bottom++;
                }
                // we don't support wrap around in vertical, so shift the tile
                while (bottom > height)
                {
                    // tiles don't wrap around vertically, so shift the tile 1 pixel up
                    bottom--;
                    top--;
                }
                tileCenterY = bottom - tileHeight / 2;

                mTileConfig[i].center.latitude.num = (int32_t)(((height / 2) - tileCenterY) * 180);
                if (mTileConfig[i].center.latitude.num == 0)
                {
                    mTileConfig[i].center.latitude.den = 1;
                }
                else
                {
                    mTileConfig[i].center.latitude.den = static_cast<int32_t>(height);
                }

                mTileConfig[i].span.vertical.num = static_cast<uint32_t>(tileHeight * 180);
                mTileConfig[i].span.vertical.den = height;
            }

            tile.width = tileWidth;

            tile.height = tileHeight;

            tile.offsetX = (int32_t)(tileCenterX - tile.width / 2);
            if (tile.offsetX < 0)
            {
                tile.offsetX = (int32_t)width + tile.offsetX;
            }
            tile.offsetY = (int32_t)(tileCenterY - tile.height / 2);
            if (tile.offsetY < 0)
            {
                tile.offsetY = (int32_t)height + tile.offsetY;
            }
            mTilesRoi.push_back(tile);
        }
        log(Info) << "number of tiles per view is " << mTilesRoi.size() << std::endl;
        for (size_t i = 0 ; i < mTilesRoi.size(); i++)
        {
            log(Info) << "Tile " << i << " width = " << mTilesRoi[i].width << " height = " << mTilesRoi[i].height << std::endl;
            log(Info) << "Tile " << i << " top left coordinates x = " << mTilesRoi[i].offsetX << " y = " << mTilesRoi[i].offsetY << std::endl;
        }
        mAdjustedConfig.set({ mTileConfig });
        return;
    }

    StorageType Tiler::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    std::vector<Streams> Tiler::process(const Streams& data)
    {
        std::vector<Streams> tiledFrames;
        if (data.isEndOfStream())
        {
            // nothing to flush; just return the empty data for each tile
            Streams tiles_per_eye;
            for (size_t i = 0; i < mTileConfig.size(); i++)
            {
                for ( size_t k = 0; k < data.size(); k++)
                {
                    tiles_per_eye.push_back(data[0]);
                }
            }
            tiledFrames.push_back({tiles_per_eye});
        }
        else
        {
            if (!mTilesRoi.size())
            {
                Initialize(data[0].getFrameMeta());
            }
            Streams tiles_per_eye;
            for (size_t i = 0; i < mTileConfig.size(); i++)
            {
                for ( size_t k = 0; k < data.size(); k++)
                {
                    try
                    {
                        VDD::Data subdata = data[k].subData(static_cast<size_t>(mTilesRoi[i].offsetX),
                                                            static_cast<size_t>(mTilesRoi[i].offsetY),
                                                            static_cast<size_t>(mTilesRoi[i].width),
                                                            static_cast<size_t>(mTilesRoi[i].height));
                        tiles_per_eye.push_back(subdata);
                    }
                    catch (OutOfBoundsException& e)
                    {
                        VDD::Data subdata = data[k].subDataWrapAround(static_cast<size_t>(mTilesRoi[i].offsetX),
                                                                      static_cast<size_t>(mTilesRoi[i].offsetY),
                                                                      static_cast<size_t>(mTilesRoi[i].width),
                                                                      static_cast<size_t>(mTilesRoi[i].height));
                        tiles_per_eye.push_back(subdata);
                    }
                }
            }
            tiledFrames.push_back({tiles_per_eye});
        }
        return tiledFrames;
    }

}  // namespace VDD
