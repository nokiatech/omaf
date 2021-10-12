
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
#include "tileconfigurations.h"
#include "config/config.h"
#include "controller/videoinput.h"
#include "common/utils.h"

namespace VDD {
    VideoRole readRole(const ConfigValue& aConfig)
    {
        auto str = Utils::lowercaseString(readString(aConfig));
        if (str == "fullres")
        {
            return VideoRole::FullRes;
        }
        else if (str == "mediumres")
        {
            return VideoRole::MediumRes;
        }
        else if (str == "mediumrespolar")
        {
            return VideoRole::MediumResPolar;
        }
        else if (str == "lowres")
        {
            return VideoRole::LowRes;
        }
        else if (str == "lowrespolar")
        {
            return VideoRole::LowResPolar;
        }
        else
        {
            throw ConfigValueInvalid("Invalid video role", aConfig);
        }
    }

    std::string stringOfVideoRole(VideoRole aVideoRole)
    {
        switch (aVideoRole)
        {
        case VideoRole::FullRes:
            return "fullres";
        case VideoRole::MediumRes:
            return "mediumres";
        case VideoRole::MediumResPolar:
            return "mediumrespolar";
        case VideoRole::LowRes:
            return "lowres";
        case VideoRole::LowResPolar:
            return "lowrespolar";
        default:
            assert(false);
            return "UNINITIALIZED";
        }
    }

    Optional<VideoRole> detectVideoRole(const VideoFileProperties& aVideo, VDMode aVDMode)
    {
        switch (aVDMode)
        {
        case VDMode::MultiRes6K:
        {
            if (aVideo.width == 6144 || aVideo.width == 5760)
            {
                return VideoRole::FullRes;
            }
            else if (aVideo.width == 3072 || aVideo.width == 2880)
            {
                if (aVideo.tilesX * aVideo.tilesY == 24)
                {
                    return VideoRole::MediumRes;
                }
                else
                {
                    return VideoRole::MediumResPolar;
                }
            }
            else
            {
                return VideoRole::LowResPolar;
            }
            break;
        }
        default:
            return {};
        }
    }

    namespace TileConfigurations {
        static TileInGrid makeGridTile(uint32_t aProjTop, uint32_t aProjLeft, uint16_t aPackedTop, uint16_t aPackedLeft, uint32_t aProjTileWidth, uint32_t aProjTileHeight, uint16_t aPackedColumnWidth, uint16_t aPackedRowHeight)
        {
            TileInGrid gridTile;

            Region& region = gridTile.regionPacking;
            region.projTop = aProjTop;
            region.projLeft = aProjLeft;
            region.packedTop = aPackedTop;
            region.packedLeft = aPackedLeft;
            region.transform = 0;

            region.projWidth = aProjTileWidth;
            region.projHeight = aProjTileHeight;
            region.packedWidth = aPackedColumnWidth;
            region.packedHeight = aPackedRowHeight;

            return gridTile;
        }

        static TileSingle makeVideoTile(StreamAndTrackGroup& aIds, uint16_t aPackedTop, uint16_t aPackedLeft, uint16_t aPackedWidth, int ctuSize)
        {
            TileSingle videoTile;

            videoTile.ids = aIds;
            // for slice header rewrite
            videoTile.ctuIndex = (aPackedTop / ctuSize) * (aPackedWidth / ctuSize) + aPackedLeft / ctuSize;
            return videoTile;
        }

        static TileRow makeERP5KRow(std::vector<StreamAndTrackGroup> aIds, uint16_t aPackedWidth, std::vector<uint16_t> aPackedColumnWidths, uint16_t aPackedRowHeight, uint16_t aPackedColumnTop, uint32_t aHighResProjColumnTop,
            std::vector<uint32_t> aHighResColumnProjLeft, uint32_t aProjTileWidth, uint32_t aProjTileHeight, uint32_t aLowResProjLeft, uint32_t aLowResProjTop, int aCtuSize)
        {
            TileRow row;

            TileInGrid gridTile;

            // pack full res tiles to the left of the packed picture

            // first full res tile
            gridTile = makeGridTile(aHighResProjColumnTop, aHighResColumnProjLeft.at(0), aPackedColumnTop, 0, aProjTileWidth, aProjTileHeight, aPackedColumnWidths.at(0), aPackedRowHeight);
            gridTile.tile.push_back(makeVideoTile(aIds.at(0), gridTile.regionPacking.packedTop, gridTile.regionPacking.packedLeft, aPackedWidth, aCtuSize));

            row.push_back(std::move(gridTile));

            // 2nd full res tile
            gridTile = makeGridTile(aHighResProjColumnTop, aHighResColumnProjLeft.at(1), aPackedColumnTop, aPackedColumnWidths.at(0), aProjTileWidth, aProjTileHeight, aPackedColumnWidths.at(1), aPackedRowHeight);
            gridTile.tile.push_back(makeVideoTile(aIds.at(1), gridTile.regionPacking.packedTop, gridTile.regionPacking.packedLeft, aPackedWidth, aCtuSize));

            row.push_back(std::move(gridTile));

            // then add low res tiles as a single tile (2 slices)
            gridTile = makeGridTile(aLowResProjTop, aLowResProjLeft, aPackedColumnTop, aPackedColumnWidths.at(0) + aPackedColumnWidths.at(1), aProjTileWidth, aProjTileHeight * 2, aPackedColumnWidths.at(2), aPackedRowHeight);
            // two slices per tile case; no separate rwpk, just slice header rewrite
            gridTile.tile.push_back(makeVideoTile(aIds.at(2), gridTile.regionPacking.packedTop, gridTile.regionPacking.packedLeft, aPackedWidth, aCtuSize));
            gridTile.tile.push_back(makeVideoTile(aIds.at(3), (gridTile.regionPacking.packedTop + gridTile.regionPacking.packedHeight / 2), gridTile.regionPacking.packedLeft, aPackedWidth, aCtuSize));

            row.push_back(std::move(gridTile));

            return row;
        }

        static void fill5KQuality(Quality3d& aQuality, int32_t aAzimuthCenter, int32_t aElevationCenter, uint32_t aAzimuthRange, uint32_t aElevationRange, uint8_t aQualityHigh, uint8_t aQualityLow)
        {
            QualityInfo info;
            info.origWidth = 5120;
            info.origHeight = 2560;
            info.qualityRank = aQualityHigh;
            Spherical sphere;
            sphere.cAzimuth = aAzimuthCenter * 65536;
            sphere.cElevation = aElevationCenter * 65536;
            sphere.cTilt = 0;
            sphere.rAzimuth = aAzimuthRange * 65536;
            sphere.rElevation = aElevationRange * 65536;
            info.sphere = sphere;
            aQuality.qualityInfo.push_back(info);
            // remaining area
            aQuality.qualityInfo.push_back(info);
            info.origWidth = 2560;
            info.origHeight = 1280;
            info.qualityRank = aQualityLow;
            aQuality.qualityInfo.push_back(info);
            aQuality.remainingArea = true;
        }

        // VD scheme from OMAF Annex D.6.2 (figure D.7)
        size_t createERP5KConfig(TileMergeConfig& aMergedConfig, const TiledInputVideo& aFullResVideo, const TiledInputVideo& aLowResVideo, StreamAndTrackIds& aExtractorIds)
        {
            if (!(aFullResVideo.width == 5120 && aFullResVideo.height == 2560 && aLowResVideo.width == 2560 && aLowResVideo.height == 1280))
            {
                //throw not supported
                throw UnsupportedVideoInput("Input video resolutions are not suitable for 5K output");
            }

            if (aFullResVideo.tiles.size() != 8 || aLowResVideo.tiles.size() != 8)
            {
                // throw not supported
                throw UnsupportedVideoInput("Input video tile counts are not suitable for 5K output");
            }

            if (aFullResVideo.ctuSize != aLowResVideo.ctuSize)
            {
                throw UnsupportedVideoInput("Input video CTU sizes are not matching");
            }

            size_t totalTileCount = 0;

            aMergedConfig.grid.columnWidths.push_back(1280);
            aMergedConfig.grid.columnWidths.push_back(1280);
            aMergedConfig.grid.columnWidths.push_back(640);
            aMergedConfig.grid.rowHeights.push_back(1280);
            aMergedConfig.grid.rowHeights.push_back(1280);
            aMergedConfig.packedWidth = 3200;
            aMergedConfig.packedHeight = 2560;
            aMergedConfig.projectedWidth = 5120;
            aMergedConfig.projectedHeight = 2560;

            // look at 90 (left)
            TileDirectionConfig cfg;

            cfg.qualityRank.shapeType = 0;
            cfg.qualityRank.qualityType = 1;    // resolution differences
            fill5KQuality(cfg.qualityRank, 90, 0, 180, 180, aFullResVideo.qualityRank, aLowResVideo.qualityRank);
            cfg.label = "5k_left";

            std::vector<StreamAndTrackGroup> ids;
            ids.push_back({ aFullResVideo.tiles.at(0).streamId, aFullResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(1).streamId, aFullResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(2).streamId, aLowResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(6).streamId, aLowResVideo.tiles.at(6).trackGroupId });

            cfg.tiles.push_back(makeERP5KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, 1280, 0, 0, { 0, 1280 }, 1280, 1280, 2560, 0, aFullResVideo.ctuSize));
            ids.clear();
            ids.push_back({ aFullResVideo.tiles.at(4).streamId, aFullResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(5).streamId, aFullResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(3).streamId, aLowResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(7).streamId, aLowResVideo.tiles.at(7).trackGroupId });
            cfg.tiles.push_back(makeERP5KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, 1280, 1280, 1280, { 0, 1280 }, 1280, 1280, 3840, 0, aFullResVideo.ctuSize));
            cfg.extractorId = StreamAndTrack(100, 100);
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            // look at 0 (center)
            cfg.qualityRank.shapeType = 0;
            cfg.qualityRank.qualityType = 1;    // resolution differences
            fill5KQuality(cfg.qualityRank, 0, 0, 180, 180, aFullResVideo.qualityRank, aLowResVideo.qualityRank);
            cfg.label = "5k_center";

            ids.clear();
            ids.push_back({ aFullResVideo.tiles.at(1).streamId, aFullResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(2).streamId, aFullResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(0).streamId, aLowResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(4).streamId, aLowResVideo.tiles.at(4).trackGroupId });

            cfg.tiles.clear();
            cfg.tiles.push_back(makeERP5KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, 1280, 0, 0, { 1280, 2560 }, 1280, 1280, 0, 0, aFullResVideo.ctuSize));
            ids.clear();
            ids.push_back({ aFullResVideo.tiles.at(5).streamId, aFullResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(6).streamId, aFullResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(3).streamId, aLowResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(7).streamId, aLowResVideo.tiles.at(7).trackGroupId });
            cfg.tiles.push_back(makeERP5KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, 1280, 1280, 1280, { 1280, 2560 }, 1280, 1280, 3840, 0, aFullResVideo.ctuSize));

            cfg.extractorId = StreamAndTrack(101, 101);
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            // look at -90 (right)
            cfg.qualityRank.shapeType = 0;
            cfg.qualityRank.qualityType = 1;    // resolution differences
            fill5KQuality(cfg.qualityRank, -90, 0, 180, 180, aFullResVideo.qualityRank, aLowResVideo.qualityRank);
            cfg.label = "5k_right";

            ids.clear();
            ids.push_back({ aFullResVideo.tiles.at(2).streamId, aFullResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(3).streamId, aFullResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(0).streamId, aLowResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(4).streamId, aLowResVideo.tiles.at(4).trackGroupId });

            cfg.tiles.clear();
            cfg.tiles.push_back(makeERP5KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, 1280, 0, 0, { 2560, 3840 }, 1280, 1280, 0, 0, aFullResVideo.ctuSize));
            ids.clear();
            ids.push_back({ aFullResVideo.tiles.at(6).streamId, aFullResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(7).streamId, aFullResVideo.tiles.at(7).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(1).streamId, aLowResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(5).streamId, aLowResVideo.tiles.at(5).trackGroupId });
            cfg.tiles.push_back(makeERP5KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, 1280, 1280, 1280, { 2560, 3840 }, 1280, 1280, 1280, 0, aFullResVideo.ctuSize));

            cfg.extractorId = StreamAndTrack(102, 102);
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));


            // look at +-180 (back)
            cfg.qualityRank.shapeType = 0;
            cfg.qualityRank.qualityType = 1;    // resolution differences
            fill5KQuality(cfg.qualityRank, 180, 0, 180, 180, aFullResVideo.qualityRank, aLowResVideo.qualityRank);
            cfg.label = "5k_back";

            ids.clear();
            ids.push_back({ aFullResVideo.tiles.at(0).streamId, aFullResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(3).streamId, aFullResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(1).streamId, aLowResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(5).streamId, aLowResVideo.tiles.at(5).trackGroupId });

            cfg.tiles.clear();
            cfg.tiles.push_back(makeERP5KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, 1280, 0, 0, { 0, 3840 }, 1280, 1280, 1280, 0, aFullResVideo.ctuSize));
            ids.clear();
            ids.push_back({ aFullResVideo.tiles.at(4).streamId, aFullResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(7).streamId, aFullResVideo.tiles.at(7).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(2).streamId, aLowResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aLowResVideo.tiles.at(6).streamId, aLowResVideo.tiles.at(6).trackGroupId });
            cfg.tiles.push_back(makeERP5KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, 1280, 1280, 1280, { 0, 3840 }, 1280, 1280, 2560, 0, aFullResVideo.ctuSize));

            cfg.extractorId = StreamAndTrack(103, 103);
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            totalTileCount = 16;
            return totalTileCount;
        }

        static TileRow makeERP6KRow(std::vector<StreamAndTrackGroup> aIds, uint16_t aPackedWidth, std::vector<uint16_t> aPackedColumnWidths, uint16_t aPackedRowHeight, uint16_t aPackedColumnTop, uint32_t aHighResProjColumnTop,
            std::vector<uint32_t> aHighResColumnProjLeft, uint32_t aProjTileWidth, uint32_t aProjTileHeight, std::vector<uint32_t> aLowResProjLeft, uint32_t aLowResProjTop, int aCtuSize)
        {
            TileRow row;

            TileInGrid gridTile;

            // pack full res tiles to the left of the packed picture

            // first 4xfull res tiles
            uint16_t packedLeft = 0;
            for (size_t i = 0; i < 4; i++)
            {
                gridTile = makeGridTile(aHighResProjColumnTop, aHighResColumnProjLeft.at(i), aPackedColumnTop, packedLeft, aProjTileWidth, aProjTileHeight, aPackedColumnWidths.at(i), aPackedRowHeight);
                gridTile.tile.push_back(makeVideoTile(aIds.at(i), gridTile.regionPacking.packedTop, gridTile.regionPacking.packedLeft, aPackedWidth, aCtuSize));
                row.push_back(std::move(gridTile));
                packedLeft += aPackedColumnWidths.at(i);
            }

            // then 2 lower res tiles with 2 slices per tile
            for (size_t i = 4, j = 4; i < 6; i++, j += 2)
            {
                gridTile = makeGridTile(aLowResProjTop, aLowResProjLeft[j - 4], aPackedColumnTop, packedLeft, aProjTileWidth, aProjTileHeight, aPackedColumnWidths.at(i), aPackedRowHeight / 2);
                // two slices per tile case; no separate rwpk, just slice header rewrite
                gridTile.tile.push_back(makeVideoTile(aIds.at(j), gridTile.regionPacking.packedTop, gridTile.regionPacking.packedLeft, aPackedWidth, aCtuSize));
                row.push_back(std::move(gridTile));

                gridTile = makeGridTile(aLowResProjTop, aLowResProjLeft[j - 3], aPackedColumnTop + aPackedRowHeight / 2, packedLeft, aProjTileWidth, aProjTileHeight, aPackedColumnWidths.at(i), aPackedRowHeight / 2);
                gridTile.tile.push_back(makeVideoTile(aIds.at(j + 1), (gridTile.regionPacking.packedTop), gridTile.regionPacking.packedLeft, aPackedWidth, aCtuSize));
                row.push_back(std::move(gridTile));

                packedLeft += aPackedColumnWidths.at(i);
            }

            return row;
        }

        static void fill6KQuality(Quality3d& aQuality, int32_t aCenterAzimuthMain, int32_t aPolarElevation, std::vector<uint8_t> aQualityRanks, const TiledInputVideo& aFullResVideo)
        {
            QualityInfo info;
            info.origWidth = aFullResVideo.width;
            info.origHeight = aFullResVideo.height;
            info.qualityRank = aQualityRanks.at(0);
            Spherical sphere;
            sphere.cAzimuth = aCenterAzimuthMain * 65536;
            sphere.cElevation = 0;
            sphere.cTilt = 0;
            sphere.rAzimuth = 4 * 45 * 65536;
            sphere.rElevation = 120 * 65536;
            info.sphere = sphere;
            aQuality.qualityInfo.push_back(info);
            // polar high Q
            info.origWidth = aFullResVideo.width/2;
            info.origHeight = aFullResVideo.height/2;
            info.qualityRank = aQualityRanks.at(1);
            sphere.cAzimuth = 0;
            sphere.cElevation = aPolarElevation * 65536;
            sphere.cTilt = 0;
            sphere.rAzimuth = 360 * 65536;
            sphere.rElevation = 30 * 65536;
            info.sphere = sphere;
            aQuality.qualityInfo.push_back(info);
            // polar low Q
            info.origWidth = aFullResVideo.width / 4;
            info.origHeight = aFullResVideo.height / 4;
            info.qualityRank = aQualityRanks.at(2);
            sphere.cAzimuth = 0;
            sphere.cElevation = (-1)*aPolarElevation * 65536;
            sphere.cTilt = 0;
            sphere.rAzimuth = 360 * 65536;
            sphere.rElevation = 30 * 65536;
            info.sphere = sphere;
            aQuality.qualityInfo.push_back(info);
            // remaining area == center tiles in low Q
            aQuality.remainingArea = true;
            QualityInfo infoRemaining;
            infoRemaining.origWidth = aFullResVideo.width / 2;
            infoRemaining.origHeight = aFullResVideo.height / 2;
            infoRemaining.qualityRank = aQualityRanks.at(3);
            aQuality.qualityInfo.push_back(infoRemaining);
        }

        static void createERP6KRows(TileMergeConfig& aMergedConfig,
                                    const TiledInputVideo& aFullResVideo,
                                    const TiledInputVideo& aMediumResVideo,
                                    StreamAndTrackIds& aExtractorIds, TileRow& polarRow,
                                    int32_t aPolarElevation, TrackIdProvider aTrackIdProvider,
                                    StreamIdProvider aStreamIdProvider, std::string& aLabel,
                                    std::vector<uint8_t> aQualityRanks)
        {
            std::vector<StreamAndTrackGroup> ids;

            uint32_t columnWidth = aFullResVideo.width / 8;
            uint32_t centerHeight = 2 * aFullResVideo.height / 3; // 120 degree high
            uint32_t polarHeight = (aFullResVideo.height - centerHeight ) / 2; // 30 degree high
            std::vector<uint32_t> columns = { 0, columnWidth, 2u * columnWidth, 3u * columnWidth, 4u * columnWidth, 5u * columnWidth, 6u * columnWidth, 7u * columnWidth };
            // look at 90 (left)
            TileDirectionConfig cfg;
            cfg.qualityRank.shapeType = 0;
            cfg.qualityRank.qualityType = 1;    // resolution differences

            fill6KQuality(cfg.qualityRank, 90, aPolarElevation, aQualityRanks, aFullResVideo);
            cfg.label = "6k_1_" + aLabel;

            ids.push_back({ aFullResVideo.tiles.at(0).streamId, aFullResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(1).streamId, aFullResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(2).streamId, aFullResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(3).streamId, aFullResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(4).streamId, aMediumResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(5).streamId, aMediumResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(6).streamId, aMediumResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(7).streamId, aMediumResVideo.tiles.at(7).trackGroupId });

            cfg.tiles.push_back(makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, centerHeight, 0, polarHeight, { columns[0], columns[1], columns[2], columns[3] }, columnWidth, centerHeight,
                { columns[4], columns[5], columns[6], columns[7] }, polarHeight, aFullResVideo.ctuSize));

            ids.clear();
            cfg.tiles.push_back(polarRow);
            cfg.extractorId = StreamAndTrack(aStreamIdProvider(), aTrackIdProvider());
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            // look at 45 (center-left)
            fill6KQuality(cfg.qualityRank, 45, aPolarElevation, aQualityRanks, aFullResVideo);
            cfg.label = "6k_2_" + aLabel;

            ids.push_back({ aFullResVideo.tiles.at(1).streamId, aFullResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(2).streamId, aFullResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(3).streamId, aFullResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(4).streamId, aFullResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(0).streamId, aMediumResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(5).streamId, aMediumResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(6).streamId, aMediumResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(7).streamId, aMediumResVideo.tiles.at(7).trackGroupId });

            cfg.tiles.push_back(makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, centerHeight, 0, polarHeight, { columns[1], columns[2], columns[3], columns[4] }, columnWidth, centerHeight, 
                { columns[0], columns[5], columns[6], columns[7] }, polarHeight, aFullResVideo.ctuSize));
            ids.clear();

            cfg.tiles.push_back(polarRow);
            cfg.extractorId = StreamAndTrack(aStreamIdProvider(), aTrackIdProvider());
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            // look at 0 (center)
            fill6KQuality(cfg.qualityRank, 0, aPolarElevation, aQualityRanks, aFullResVideo);
            cfg.label = "6k_3_" + aLabel;

            ids.push_back({ aFullResVideo.tiles.at(2).streamId, aFullResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(3).streamId, aFullResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(4).streamId, aFullResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(5).streamId, aFullResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(0).streamId, aMediumResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(1).streamId, aMediumResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(6).streamId, aMediumResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(7).streamId, aMediumResVideo.tiles.at(7).trackGroupId });

            cfg.tiles.push_back(makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, centerHeight, 0, polarHeight, { columns[2], columns[3], columns[4], columns[5] }, columnWidth, centerHeight, 
                { columns[0], columns[1], columns[6], columns[7] }, polarHeight, aFullResVideo.ctuSize));
            ids.clear();

            cfg.tiles.push_back(polarRow);
            cfg.extractorId = StreamAndTrack(aStreamIdProvider(), aTrackIdProvider());
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            // look at -45 (center-right)
            fill6KQuality(cfg.qualityRank, -45, aPolarElevation, aQualityRanks, aFullResVideo);
            cfg.label = "6k_4_" + aLabel;

            ids.push_back({ aFullResVideo.tiles.at(3).streamId, aFullResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(4).streamId, aFullResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(5).streamId, aFullResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(6).streamId, aFullResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(0).streamId, aMediumResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(1).streamId, aMediumResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(2).streamId, aMediumResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(7).streamId, aMediumResVideo.tiles.at(7).trackGroupId });

            cfg.tiles.push_back(makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, centerHeight, 0, polarHeight, { columns[3], columns[4], columns[5], columns[6] }, columnWidth, centerHeight, 
            { columns[0], columns[1], columns[2], columns[7] }, polarHeight, aFullResVideo.ctuSize));
            ids.clear();

            cfg.tiles.push_back(polarRow);
            cfg.extractorId = StreamAndTrack(aStreamIdProvider(), aTrackIdProvider());
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));


            // look at -90 (right)
            fill6KQuality(cfg.qualityRank, -90, aPolarElevation, aQualityRanks, aFullResVideo);
            cfg.label = "6k_5_" + aLabel;

            ids.push_back({ aFullResVideo.tiles.at(4).streamId, aFullResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(5).streamId, aFullResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(6).streamId, aFullResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(7).streamId, aFullResVideo.tiles.at(7).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(0).streamId, aMediumResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(1).streamId, aMediumResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(2).streamId, aMediumResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(3).streamId, aMediumResVideo.tiles.at(3).trackGroupId });

            cfg.tiles.push_back(makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, centerHeight, 0, polarHeight, { columns[4], columns[5], columns[6], columns[7] }, columnWidth, centerHeight, 
                { columns[0], columns[1], columns[2], columns[3] }, polarHeight, aFullResVideo.ctuSize));
            ids.clear();

            cfg.tiles.push_back(polarRow);
            cfg.extractorId = StreamAndTrack(aStreamIdProvider(), aTrackIdProvider());
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            // look at -135 (back-right)
            fill6KQuality(cfg.qualityRank, -135, aPolarElevation, aQualityRanks, aFullResVideo);
            cfg.label = "6k_6_" + aLabel;

            ids.push_back({ aFullResVideo.tiles.at(0).streamId, aFullResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(5).streamId, aFullResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(6).streamId, aFullResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(7).streamId, aFullResVideo.tiles.at(7).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(1).streamId, aMediumResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(2).streamId, aMediumResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(3).streamId, aMediumResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(4).streamId, aMediumResVideo.tiles.at(4).trackGroupId });

            cfg.tiles.push_back(makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, centerHeight, 0, polarHeight, { columns[0], columns[5], columns[6], columns[7] }, columnWidth, centerHeight, 
                { columns[1], columns[2], columns[3], columns[4]}, polarHeight, aFullResVideo.ctuSize));
            ids.clear();

            cfg.tiles.push_back(polarRow);
            cfg.extractorId = StreamAndTrack(aStreamIdProvider(), aTrackIdProvider());
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            // look at -180 (back)
            fill6KQuality(cfg.qualityRank, -180, aPolarElevation, aQualityRanks, aFullResVideo);
            cfg.label = "6k_7_" + aLabel;

            ids.push_back({ aFullResVideo.tiles.at(0).streamId, aFullResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(1).streamId, aFullResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(6).streamId, aFullResVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(7).streamId, aFullResVideo.tiles.at(7).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(2).streamId, aMediumResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(3).streamId, aMediumResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(4).streamId, aMediumResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(5).streamId, aMediumResVideo.tiles.at(5).trackGroupId });

            cfg.tiles.push_back(makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, centerHeight, 0, polarHeight, { columns[0], columns[1], columns[6], columns[7] }, columnWidth, centerHeight, 
                { columns[2], columns[3], columns[4], columns[5] }, polarHeight, aFullResVideo.ctuSize));
            ids.clear();

            cfg.tiles.push_back(polarRow);
            cfg.extractorId = StreamAndTrack(aStreamIdProvider(), aTrackIdProvider());
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

            // look at 135 (back-left)
            fill6KQuality(cfg.qualityRank, 135, aPolarElevation, aQualityRanks, aFullResVideo);
            cfg.label = "6k_8_" + aLabel;

            ids.push_back({ aFullResVideo.tiles.at(0).streamId, aFullResVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(1).streamId, aFullResVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(2).streamId, aFullResVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aFullResVideo.tiles.at(7).streamId, aFullResVideo.tiles.at(7).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(3).streamId, aMediumResVideo.tiles.at(3).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(4).streamId, aMediumResVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(5).streamId, aMediumResVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aMediumResVideo.tiles.at(6).streamId, aMediumResVideo.tiles.at(6).trackGroupId });

            cfg.tiles.push_back(makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, centerHeight, 0, polarHeight, { columns[0], columns[1], columns[2], columns[7] }, columnWidth, centerHeight, 
                { columns[3], columns[4], columns[5], columns[6] }, polarHeight, aFullResVideo.ctuSize));
            ids.clear();

            cfg.tiles.push_back(polarRow);
            cfg.extractorId = StreamAndTrack(aStreamIdProvider(), aTrackIdProvider());
            aExtractorIds.push_back(cfg.extractorId);
            aMergedConfig.directions.push_back(std::move(cfg));

        }

        // returns the removed tiles
        static OmafTileSets cropByTiles(OmafTileSets& aTileConfig, std::vector<int> aUseRows, int aTilesOnRow, int aTileRows)
        {
            OmafTileSets removedTiles;
            for (int y = 0, r = 0; y < aTileRows; y++)
            {
                if (static_cast<size_t>(r) < aUseRows.size() && y == aUseRows.at(static_cast<size_t>(r)))
                {
                    r++;
                }
                else
                {
                    std::move(aTileConfig.begin() + r*aTilesOnRow, aTileConfig.begin() + (r+1)*aTilesOnRow,
                              std::back_inserter(removedTiles));
                    aTileConfig.erase(aTileConfig.begin() + r*aTilesOnRow, aTileConfig.begin() + (r+1)*aTilesOnRow);
                }
            }
            return removedTiles;
        }

        OmafTileSets crop6KMainVideo(OmafTileSets& aTileConfig, VideoInputMode aInputVideoMode)
        {
            OmafTileSets removedTiles;
            std::vector<int> rows;
            rows.push_back(1);
            if (aInputVideoMode == VideoInputMode::TopBottom)
            {
                if (aTileConfig.size() != 48)
                {
                    throw UnsupportedVideoInput("Input video tile counts are not suitable for 6K output");
                }
                rows.push_back(4);
                removedTiles = cropByTiles(aTileConfig, rows, 8, 6);
            }
            else
            {
                if (aTileConfig.size() != 24)
                {
                    throw UnsupportedVideoInput("Input video tile counts are not suitable for 6K output");
                }
                removedTiles = cropByTiles(aTileConfig, rows, 8, 3);
            }
            return removedTiles;
        }

        OmafTileSets crop6KPolarVideo(OmafTileSets& aTileConfig, VideoInputMode aInputVideoMode)
        {
            OmafTileSets removedTiles;
            std::vector<int> rows;
            rows.push_back(0);
            rows.push_back(2);
            if (aInputVideoMode == VideoInputMode::TopBottom)
            {
                if (aTileConfig.size() != 24)
                {
                    throw UnsupportedVideoInput("Input video tile counts are not suitable for 6K output");
                }
                rows.push_back(3);
                rows.push_back(5);
                removedTiles = cropByTiles(aTileConfig, rows, 4, 6);
            }
            else
            {
                if (aTileConfig.size() != 12)
                {
                    throw UnsupportedVideoInput("Input video tile counts are not suitable for 6K output");
                }
                removedTiles = cropByTiles(aTileConfig, rows, 4, 3);
            }
            return removedTiles;
        }

        size_t createERP6KConfig(TileMergeConfig& aMergedConfig,
                                 const TiledInputVideo& aFullResVideo,
                                 const TiledInputVideo& aMediumResVideo,
                                 const TiledInputVideo& aMediumResPolarVideo,
                                 const TiledInputVideo& aLowResPolarVideo,
                                 StreamAndTrackIds& aExtractorIds, TrackIdProvider aTrackIdProvider,
                                 StreamIdProvider aStreamIdProvider)
        {
            if (aFullResVideo.width == 6144 && aFullResVideo.height == 3072)
            {
                if (!(aMediumResVideo.width == 3072 && aMediumResVideo.height == 1536 &&
                    aMediumResPolarVideo.width == 3072 && aMediumResPolarVideo.height == 1536 &&
                    aLowResPolarVideo.width == 1536 && aLowResPolarVideo.height == 768))
                {
                    throw UnsupportedVideoInput("Input video resolutions are not suitable for 6K output");
                }
            }
            else if (aFullResVideo.width == 5760 && aFullResVideo.height == 2880)
            {
                if (!(aMediumResVideo.width == 2880 && aMediumResVideo.height == 1440 &&
                    aMediumResPolarVideo.width == 2880 && aMediumResPolarVideo.height == 1440 &&
                    aLowResPolarVideo.width == 1440 && aLowResPolarVideo.height == 720))
                {
                    throw UnsupportedVideoInput("Input video resolutions are not suitable for 6K output");
                }
            }
            else
            {
                throw UnsupportedVideoInput("Input video resolutions are not suitable for 6K output");
            }

            if (aFullResVideo.tiles.size() != 8 || aMediumResVideo.tiles.size() != 8 || aMediumResPolarVideo.tiles.size() != 8 || aLowResPolarVideo.tiles.size() != 8)
            {
                throw UnsupportedVideoInput("Input video tile counts are not suitable for 6K output");
            }

            if (aFullResVideo.ctuSize != aMediumResVideo.ctuSize || aMediumResVideo.ctuSize != aMediumResPolarVideo.ctuSize || aMediumResPolarVideo.ctuSize != aLowResPolarVideo.ctuSize)
            {
                throw UnsupportedVideoInput("Input video CTU sizes are not matching");
            }

            std::vector<uint8_t> qualityRanks;
            qualityRanks.push_back(aFullResVideo.qualityRank);
            qualityRanks.push_back(aMediumResPolarVideo.qualityRank);
            qualityRanks.push_back(aLowResPolarVideo.qualityRank);
            qualityRanks.push_back(aMediumResVideo.qualityRank);

            size_t totalTileCount = 0;

            uint32_t columnWidth = aFullResVideo.width / 8;
            uint32_t centerHeight = 2 * aFullResVideo.height / 3;	// 120 degree high
            uint32_t fullResVideoPolarHeight = aFullResVideo.height / 6; // 30 degree high
            uint32_t polarHeight = aMediumResPolarVideo.height / 6; // 30 degree high

            aMergedConfig.grid.columnWidths.push_back(columnWidth);
            aMergedConfig.grid.columnWidths.push_back(columnWidth);
            aMergedConfig.grid.columnWidths.push_back(columnWidth);
            aMergedConfig.grid.columnWidths.push_back(columnWidth);
            aMergedConfig.grid.columnWidths.push_back(columnWidth/2);
            aMergedConfig.grid.columnWidths.push_back(columnWidth/2);
            aMergedConfig.grid.rowHeights.push_back(centerHeight);
            aMergedConfig.grid.rowHeights.push_back(polarHeight);
            aMergedConfig.packedWidth = 5*columnWidth;
            aMergedConfig.packedHeight = centerHeight + polarHeight;
            aMergedConfig.projectedWidth = aFullResVideo.width;
            aMergedConfig.projectedHeight = aFullResVideo.height;

            std::vector<StreamAndTrackGroup> ids;

            // top tiles from medium
            ids.push_back({ aMediumResPolarVideo.tiles.at(0).streamId, aMediumResPolarVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aMediumResPolarVideo.tiles.at(1).streamId, aMediumResPolarVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aMediumResPolarVideo.tiles.at(2).streamId, aMediumResPolarVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aMediumResPolarVideo.tiles.at(3).streamId, aMediumResPolarVideo.tiles.at(3).trackGroupId });
            // bottom tiles from low
            ids.push_back({ aLowResPolarVideo.tiles.at(4).streamId, aLowResPolarVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aLowResPolarVideo.tiles.at(5).streamId, aLowResPolarVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aLowResPolarVideo.tiles.at(6).streamId, aLowResPolarVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aLowResPolarVideo.tiles.at(7).streamId, aLowResPolarVideo.tiles.at(7).trackGroupId });
 
            TileRow polarRow = makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, polarHeight, centerHeight, 0, { 0, 2u * columnWidth, 4u * columnWidth, 6u * columnWidth }, 
                2*columnWidth, 2*polarHeight, { 0, 2u * columnWidth, 4u * columnWidth, 6u * columnWidth }, aFullResVideo.height - fullResVideoPolarHeight, aFullResVideo.ctuSize);
            ids.clear();

            std::string label = "top";
            createERP6KRows(aMergedConfig, aFullResVideo, aMediumResVideo, aExtractorIds, polarRow, 75, aTrackIdProvider, aStreamIdProvider, label, qualityRanks);

            // bottom tiles from medium
            ids.push_back({ aMediumResPolarVideo.tiles.at(4).streamId, aMediumResPolarVideo.tiles.at(4).trackGroupId });
            ids.push_back({ aMediumResPolarVideo.tiles.at(5).streamId, aMediumResPolarVideo.tiles.at(5).trackGroupId });
            ids.push_back({ aMediumResPolarVideo.tiles.at(6).streamId, aMediumResPolarVideo.tiles.at(6).trackGroupId });
            ids.push_back({ aMediumResPolarVideo.tiles.at(7).streamId, aMediumResPolarVideo.tiles.at(7).trackGroupId });
            // top tiles from low
            ids.push_back({ aLowResPolarVideo.tiles.at(0).streamId, aLowResPolarVideo.tiles.at(0).trackGroupId });
            ids.push_back({ aLowResPolarVideo.tiles.at(1).streamId, aLowResPolarVideo.tiles.at(1).trackGroupId });
            ids.push_back({ aLowResPolarVideo.tiles.at(2).streamId, aLowResPolarVideo.tiles.at(2).trackGroupId });
            ids.push_back({ aLowResPolarVideo.tiles.at(3).streamId, aLowResPolarVideo.tiles.at(3).trackGroupId });
            polarRow = makeERP6KRow(ids, aMergedConfig.packedWidth, aMergedConfig.grid.columnWidths, polarHeight, centerHeight, aFullResVideo.height - fullResVideoPolarHeight, { 0, 2u * columnWidth, 4u * columnWidth, 6u * columnWidth },
                2 * columnWidth, 2 * polarHeight, { 0, 2u * columnWidth, 4u * columnWidth, 6u * columnWidth }, 0, aFullResVideo.ctuSize);

            label = "bottom";
            createERP6KRows(aMergedConfig, aFullResVideo, aMediumResVideo, aExtractorIds, polarRow, -75, aTrackIdProvider, aStreamIdProvider, label, qualityRanks);

            totalTileCount = 32;
            return totalTileCount;
        }

    }
}