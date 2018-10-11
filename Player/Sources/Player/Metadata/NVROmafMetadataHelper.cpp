
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
#include "Metadata/NVROmafMetadataHelper.h"
#include "Provider/NVRCoreProviderSources.h"
#include "Metadata/NVRCubemapDefinitions.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(OmafMetadataHelper)

namespace OmafMetadata
{

    static Region parseRegion(MP4VR::DynArray<MP4VR::RegionWisePackingRegion>::const_iterator it, uint32_t aProjWidth, uint32_t aProjHeight, uint16_t aPackedWidth, uint16_t aPackedHeight, float64_t aOffsetX, float64_t aOffsetY, 
        float64_t aProjWidthLon, float64_t aProjHeightLat, float64_t aProjOriginLon, float64_t aProjOriginLat)
    {
        Region region;
        // packed coordinates - note that the player has origin in bottom-left corner of the input rectangle; that is handled when setting the source, here we just map the structs
        region.inputRect.x = (float32_t)(*it).region.rectangular.packedRegLeft / aPackedWidth + aOffsetX;
        region.inputRect.y = (float32_t)(*it).region.rectangular.packedRegTop / aPackedHeight + aOffsetY;
        region.inputRect.w = (float32_t)(*it).region.rectangular.packedRegWidth / aPackedWidth;
        region.inputRect.h = (float32_t)(*it).region.rectangular.packedRegHeight / aPackedHeight;
        // projected coordinates
        region.centerLongitude = ((float64_t)((*it).region.rectangular.projRegLeft + (*it).region.rectangular.projRegWidth / 2) / aProjWidth) * aProjWidthLon - aProjOriginLon;
        region.centerLatitude = aProjOriginLat - ((float64_t)((*it).region.rectangular.projRegTop + (*it).region.rectangular.projRegHeight / 2) / aProjHeight) * aProjHeightLat;
        region.spanLongitude = (float64_t)((*it).region.rectangular.projRegWidth) / aProjWidth * aProjWidthLon;
        region.spanLatitude = (float64_t)((*it).region.rectangular.projRegHeight) / aProjHeight * aProjHeightLat;
        return region;
    }

    Error::Enum parseOmafEquirectRegions(const MP4VR::RegionWisePackingProperty& aRwpk, RegionPacking& aRegionPacking, SourceDirection::Enum aSourceDirection)
    {
        // note! for rectangular, we currently ignore transformation, as there is no support in renderer for transforming sources
        if (aRwpk.constituentPictureMatchingFlag)
        {
            float64_t offsetX = 0.f;
            float64_t offsetY = 0.f;
            float64_t projAreaWidthLon = 360.f;
            float64_t projAreaHeightLat = 180.f;
            float64_t projOriginLon = 180.f;
            float64_t projOriginLat = 90.f;
            uint32_t width = aRwpk.projPictureWidth;
            uint32_t height = aRwpk.projPictureHeight;
            if (aSourceDirection == SourceDirection::TOP_BOTTOM)
            {
                height = aRwpk.projPictureHeight / 2;
                projAreaHeightLat = 90.f;
            }
            else if (aSourceDirection == SourceDirection::LEFT_RIGHT)
            {
                width = aRwpk.projPictureWidth / 2;
                projAreaWidthLon = 180.f;
                projOriginLon = 0.f;
            }
            for (MP4VR::DynArray<MP4VR::RegionWisePackingRegion>::const_iterator it = aRwpk.regions.begin(); it != aRwpk.regions.end(); ++it)
            {
                if ((*it).region.rectangular.transformType != 0)
                {
                    // transformations for equirect not support atm
                    //return Error::FILE_NOT_SUPPORTED;
                }
                aRegionPacking.add(parseRegion(it, width, height, aRwpk.packedPictureWidth, aRwpk.packedPictureHeight, offsetX, offsetY, projAreaWidthLon, projAreaHeightLat, projOriginLon, projOriginLat));
            }
            if (aSourceDirection == SourceDirection::TOP_BOTTOM)
            {
                offsetY = 0.5;
                projOriginLat = 0.f;
            }
            else if (aSourceDirection == SourceDirection::LEFT_RIGHT)
            {
                offsetX = 0.5;
                projOriginLon = 180.f;
            }
            for (MP4VR::DynArray<MP4VR::RegionWisePackingRegion>::const_iterator it = aRwpk.regions.begin(); it != aRwpk.regions.end(); ++it)
            {
                if ((*it).region.rectangular.transformType != 0)
                {
                    // transformations for equirect not support atm
                    return Error::FILE_NOT_SUPPORTED;
                }
                aRegionPacking.add(parseRegion(it, width, height, aRwpk.packedPictureWidth, aRwpk.packedPictureHeight, offsetX, offsetY, projAreaWidthLon, projAreaHeightLat, projOriginLon, projOriginLat));
            }
        }
        else
        {
            for (MP4VR::DynArray<MP4VR::RegionWisePackingRegion>::const_iterator it = aRwpk.regions.begin(); it != aRwpk.regions.end(); ++it)
            {
                if ((*it).region.rectangular.transformType != 0)
                {
                    // transformations for equirect not support atm
                    //return Error::FILE_NOT_SUPPORTED;
                }
                aRegionPacking.add(parseRegion(it, aRwpk.projPictureWidth, aRwpk.projPictureHeight, aRwpk.packedPictureWidth, aRwpk.packedPictureHeight, 0.f, 0.f, 360.f, 180.f, 180.f, 90.f));
            }
        }
        return Error::OK;
    }

    namespace OmafCubeFaceOrientation
    {
        enum Enum
        {
            CUBE_FACE_ORIENTATION_NO_TRANSFORM = 0,
            CUBE_FACE_ORIENTATION_MIRROR = 1,
            CUBE_FACE_ORIENTATION_ROTATED_180 = 2,
            CUBE_FACE_ORIENTATION_MIRROR_ROTATED_180 = 3,
            CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_LEFT = 4,
            CUBE_FACE_ORIENTATION_ROTATED_90_LEFT = 5,
            CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_RIGHT = 6,
            CUBE_FACE_ORIENTATION_ROTATED_90_RIGHT = 7,
        };
    }

    static int mapOmafTransformToInternalIndex(int8_t aOmafTransform)
    {
        switch (aOmafTransform)
        {
        case OmafCubeFaceOrientation::CUBE_FACE_ORIENTATION_MIRROR:
            return CubeFaceSectionOrientationIndex::CUBE_FACE_ORIENTATION_MIRROR_NO_ROTATION;
        case OmafCubeFaceOrientation::CUBE_FACE_ORIENTATION_ROTATED_180:
            return CubeFaceSectionOrientationIndex::CUBE_FACE_ORIENTATION_ROTATED_180;
        case OmafCubeFaceOrientation::CUBE_FACE_ORIENTATION_MIRROR_ROTATED_180:
            return CubeFaceSectionOrientationIndex::CUBE_FACE_ORIENTATION_MIRROR_ROTATED_180;
        case OmafCubeFaceOrientation::CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_LEFT:
            return CubeFaceSectionOrientationIndex::CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_LEFT;
        case OmafCubeFaceOrientation::CUBE_FACE_ORIENTATION_ROTATED_90_LEFT:
            return CubeFaceSectionOrientationIndex::CUBE_FACE_ORIENTATION_ROTATED_90_LEFT;
        case OmafCubeFaceOrientation::CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_RIGHT:
            return CubeFaceSectionOrientationIndex::CUBE_FACE_ORIENTATION_MIRROR_ROTATED_90_RIGHT;
        case OmafCubeFaceOrientation::CUBE_FACE_ORIENTATION_ROTATED_90_RIGHT:
            return CubeFaceSectionOrientationIndex::CUBE_FACE_ORIENTATION_ROTATED_90_RIGHT;
        default: //CUBE_FACE_ORIENTATION_NO_TRANSFORM
            return CubeFaceSectionOrientationIndex::CUBE_FACE_ORIENTATION_NO_ROTATION;
        }
    }

    static int convertCustomOmafTransformToOmaf(int8_t aOmafTransform, char_t aFace)
    {
        int transform = mapOmafTransformToInternalIndex(aOmafTransform);
        switch (aFace)
        {
            case 'B':
            {
                // OMAF default orientation for back face is rotate 90 degree right, no mirror
                // Our internal orientation for back face is no rotate, no mirror
                // So to get OMAF orientation (custom or default) to internal, need to rotate it to left by 90
                // As the enum CubeFaceSectionOrientation runs clockwise with 90 degree steps, we can apply rotate left by subtracting 1 step 
                // Further, mirroring must not change, and the enum has values 0...3 for non-mirror, 4...7 for mirror
                int base = (transform > 3 ? 4 : 0);
                transform = base + (transform + 3) % 4; // +3 == -1 in the used domain; sounds better than modulo(-1) which is generally not defined
                break;
            }
            case 'D':
            case 'U':
            {
                // OMAF default orientation for top and bottom faces is rotate 90 degree right, no mirror, but aligned with back face
                // Our internal is no rotate, no mirror, but top and bottom faces are aligned with front face => they get additional 180 when aligned with back face
                // So to get OMAF orientation (custom or default) to internal, need to rotate it to left by 270 == right by 90
                // As the enum CubeFaceSectionOrientation runs clockwise with 90 degree steps, we can apply rotate right by adding 1 step 
                // Further, mirroring must not change, and the enum has values 0...3 for non-mirror, 4...7 for mirror
                int base = (transform > 3 ? 4 : 0);
                transform = base + (transform + 1) % 4;
                break;
            }
            default:    // 'L', 'F', 'R'
            {
                // no rotation or mirror in OMAF or internal => any custom transform need to be applied as such
            }
        }
        return transform;
    }

    /*
     *  maps the given coordinates to faces in 3x2 packed cubemap layout
     */
    static int32_t findPositionInCube(uint32_t aTop, uint32_t aLeft, int32_t aProjectedHeight, int32_t aProjectedWidth)
    {
        int32_t row = aTop / (aProjectedHeight / 2);
        int32_t col = aLeft / (aProjectedWidth / 3);

        return row * 3 + col;
    }

    /*
    *  Map cubemap regions from packed to projected domain.
    *  The projected domain should match with OMAF default cubemap layout.
    *  Packed domain is the actual video content from decoder's output.
    *  The output from this function is then our internal cubemap layout.
    *  Note: Framepacked stereo must have the same packing for both channels, with or without constituent picture matching flag set
    */
    Error::Enum parseOmafCubemapRegions(const MP4VR::RegionWisePackingProperty& aRwpk, SourceDirection::Enum aSourceDirection, VRCubeMap& aTiledCubeMap)
    {
        uint32_t projectedPictureWidth = aRwpk.projPictureWidth;
        uint32_t projectedPictureHeight = aRwpk.projPictureHeight;
        if (aSourceDirection == SourceDirection::TOP_BOTTOM)
        {
            // we support only region packings, where both channels have the same packing. So limit the projected area based on the indicated framepacking, and ignore regions outside of it
            if (aRwpk.constituentPictureMatchingFlag)
            {
                // ok
                projectedPictureHeight /= 2;
            }
            else
            {
                // read still only regions for the other channel, as we currently don't support any more advanced packings
                projectedPictureHeight /= 2;
            }
        }
        else if (aSourceDirection == SourceDirection::LEFT_RIGHT)
        {
            // we support only region packings, where both channels have the same packing. So limit the projected area based on the indicated framepacking, and ignore regions outside of it
            if (aRwpk.constituentPictureMatchingFlag)
            {
                // ok
                projectedPictureWidth /= 2;
            }
            else
            {
                // read still only regions for the other channel, as we currently don't support any more advanced packings
                projectedPictureWidth /= 2;
            }
        }

        float32_t faceWidth = aRwpk.projPictureWidth / 3;
        float32_t faceHeight = aRwpk.projPictureHeight / 2;
        // OMAF face order:
        // L F R
        // D B U (all rotated)

        // First collect tiles for each face. One tile per face could be enough, but here all the regions are processed and the last one per face remains effective. 
        // Assumption is that if they are rotated, the rotation is in the same direction in all of them
        for (MP4VR::DynArray<MP4VR::RegionWisePackingRegion>::const_iterator it = aRwpk.regions.begin(); it != aRwpk.regions.end(); ++it)
        {
            if ((*it).region.rectangular.projRegTop >= projectedPictureHeight ||
                (*it).region.rectangular.projRegLeft >= projectedPictureWidth)
            {
                // ignore, is probably stereo region for the other channel, but as noted above we don't support such case
                continue;
            }
            VRCubeMapFaceSection tile;

            // packed face coordinates
            tile.sourceX = (float32_t)(*it).region.rectangular.packedRegLeft / aRwpk.packedPictureWidth;
            tile.sourceY = (float32_t)(*it).region.rectangular.packedRegTop / aRwpk.packedPictureHeight;
            tile.sourceWidth = (float32_t)(*it).region.rectangular.packedRegWidth / aRwpk.packedPictureWidth;
            tile.sourceHeight = (float32_t)(*it).region.rectangular.packedRegHeight / aRwpk.packedPictureHeight;

            int32_t facesTop = 0;
            int32_t facesLeft = 0;
            char_t face;
            int32_t position = findPositionInCube((*it).region.rectangular.projRegTop, (*it).region.rectangular.projRegLeft, projectedPictureHeight, projectedPictureWidth);
            int32_t faceIndex = 0;
            // position follows the OMAF order L F R D B U
            // faceIndex, i.e. output, follows internal order: 0 - front 1 - left 2 - back 3 - right 4 - top 5 - bottom
            switch (position)
            {
            case 0:
            {
                // left
                face = 'L';
                facesTop = 0;
                facesLeft = 0;
                faceIndex = 1;
                break;
            }
            case 1:
            {
                // front
                face = 'F';
                facesTop = 0;
                facesLeft = 1;
                faceIndex = 0;
                break;
            }
            case 2:
            {
                // right
                face = 'R';
                facesTop = 0;
                facesLeft = 2;
                faceIndex = 3;
                break;
            }
            case 3:
            {
                // down
                face = 'D';
                facesTop = 1;
                facesLeft = 0;
                faceIndex = 5;
                break;
            }
            case 4:
            {
                // back
                face = 'B';
                facesTop = 1;
                facesLeft = 1;
                faceIndex = 2;
                break;
            }
            default:
            {
                // up
                face = 'U';
                facesTop = 1;
                facesLeft = 2;
                faceIndex = 4;
                break;
            }
            }

            // target coordinates within a face - Note: this maps to OMAF cubemap, which is still different than our internal cubemap, as OMAF rotates DBU, but we don't
            tile.originX = (float32_t)(*it).region.rectangular.projRegLeft / faceWidth - facesLeft;
            tile.originY = (float32_t)(*it).region.rectangular.projRegTop / faceHeight - facesTop;
            if (position == 3 || position == 5)
            {
                if (tile.originX == 0.f)
                {
                    if (tile.originY == 0.f)
                    {
                        tile.originX = 0.5f;
                    }
                    else
                    {
                        tile.originY = 0.f;
                    }
                }
                else if (tile.originY == 0.f)
                {
                    tile.originY = 0.5f;
                }
                else
                {
                    tile.originX = 0.f;
                }
            }
            else if (position == 4)
            {
                if (tile.originX == 0.f)
                {
                    if (tile.originY == 0.f)
                    {
                        tile.originY = 0.5f;
                    }
                    else
                    {
                        tile.originX = 0.5f;
                    }
                }
                else if (tile.originY == 0.f)
                {
                    tile.originX = 0.f;
                }
                else
                {
                    tile.originY = 0.f;
                }
            }
            tile.originWidth = (float32_t)(*it).region.rectangular.projRegWidth / faceWidth;
            tile.originHeight = (float32_t)(*it).region.rectangular.projRegHeight / faceHeight;

            //TODO is this correct mapping?
            tile.sourceOrientation = (CubeFaceSectionOrientation::Enum)convertCustomOmafTransformToOmaf((*it).region.rectangular.transformType, face);

            aTiledCubeMap.cubeFaces[position].faceIndex = faceIndex;
            aTiledCubeMap.cubeFaces[position].sections[aTiledCubeMap.cubeFaces[position].numCoordinates++] = tile;
        }
        aTiledCubeMap.cubeNumFaces = 6;

        return Error::OK;
    }

}
OMAF_NS_END
