
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
#include "./omafproperties.h"
#include "./parser/h265parser.hpp"


namespace VDD {

    namespace OMAF {
        std::uint32_t createProjectionSEI(Parser::BitStream& bitstr, int temporalIdPlus1)
        {
            H265::EquirectangularProjectionSEI projection;
            projection.erpCancel = false;
            projection.erpPersistence = true; // valid until updated
            projection.erpGuardBand = false;
            return H265Parser::writeEquiProjectionSEINal(bitstr, projection, temporalIdPlus1);
        }

        std::uint32_t createRwpkSEI(Parser::BitStream& bitstr, const RegionPacking& aRegion, int temporalIdPlus1)
        {
            H265::RegionWisePackingSEI packing;
            packing.rwpCancel = false;
            packing.rwpPersistence = true; // valid until updated
            packing.constituentPictureMatching = false; // 0 for mono

            packing.projPictureWidth = aRegion.projPictureWidth;
            packing.projPictureHeight = aRegion.projPictureHeight;
            packing.packedPictureWidth = aRegion.packedPictureWidth;
            packing.packedPictureHeight = aRegion.packedPictureHeight;

            for (auto& inputRegion : aRegion.regions)
            {
                H265::RegionStruct region;

                // projected as such without any moves
                region.projRegionTop = inputRegion.projTop;
                region.projRegionLeft = inputRegion.projLeft;
                region.projRegionWidth = inputRegion.projWidth;
                region.projRegionHeight = inputRegion.projHeight;
                region.packedRegionTop = inputRegion.packedTop;
                region.packedRegionLeft = inputRegion.packedLeft;
                region.packedRegionWidth = inputRegion.packedWidth;
                region.packedRegionHeight = inputRegion.packedHeight;

                region.rwpTransformType = inputRegion.transform;

                region.rwpGuardBand = false;

                packing.regions.push_back(std::move(region));
            }

            return H265Parser::writeRwpkSEINal(bitstr, packing, temporalIdPlus1);
        }

    }
}  // namespace VDD
