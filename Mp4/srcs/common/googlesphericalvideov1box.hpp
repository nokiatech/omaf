
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
#ifndef GOOGLESPHERICALVIDEOV1BOX_HPP
#define GOOGLESPHERICALVIDEOV1BOX_HPP

#include "bbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "googleprojectionbox.hpp"
#include "googlesphericalvideoheaderbox.hpp"

// From https://github.com/google/spatial-media/blob/master/docs/spherical-video-rfc.md :
// UUID value ffcc8263-f855-4a93-8814-587a02521fdd
#define GOOGLE_SPHERICAL_VIDEO_V1_GLOBAL_UUID                                                          \
    {                                                                                                  \
        0xff, 0xcc, 0x82, 0x63, 0xf8, 0x55, 0x4a, 0x93, 0x88, 0x14, 0x58, 0x7a, 0x02, 0x52, 0x1f, 0xdd \
    }

/**
* @brief  Google Spherical Video V1 Box class
* @details Google Spherical Video box 'sv3d' implementation as specified in
           https://github.com/google/spatial-media/blob/master/docs/spherical-video-rfc.md
*/
class SphericalVideoV1Box : public Box
{
public:
    SphericalVideoV1Box();
    virtual ~SphericalVideoV1Box() = default;

    enum class V1StereoMode : uint8_t
    {
        UNDEFINED = 0,

        MONO                    = 1,
        STEREOSCOPIC_TOP_BOTTOM = 2,
        STEREOSCOPIC_LEFT_RIGHT = 3
    };

    enum class V1Projection : uint8_t
    {
        UNKNOWN         = 0,
        EQUIRECTANGULAR = 1  // Only one defined so far.
    };

    struct GlobalMetadata
    {
        bool spherical;
        bool stitched;
        String stitchingSoftware;
        V1Projection projectionType;
        V1StereoMode stereoMode;
        uint32_t sourceCount;
        int32_t initialViewHeadingDegrees;
        int32_t initialViewPitchDegrees;
        int32_t initialViewRollDegrees;
        uint64_t timestamp;
        uint32_t fullPanoWidthPixels;
        uint32_t fullPanoHeightPixels;
        uint32_t croppedAreaImageWidthPixels;
        uint32_t croppedAreaImageHeightPixels;
        uint32_t croppedAreaLeftPixels;
        uint32_t croppedAreaTopPixels;
    };

    /** @brief Sets GlobalMetadata.
     *   @param [in] globalMetadata GlobalMetadata reference **/
    void setGlobalMetadata(const GlobalMetadata& globalMetadata);

    /** @brief Gets GlobalMetadata.
     *   @return GlobalMetadata reference **/
    const GlobalMetadata& getGlobalMetadata() const;

    /**
     * @brief Serialize box data to the ISOBMFF::BitStream.
     * @see Box::writeBox()
     */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /**
     * @brief Deserialize box data from the ISOBMFF::BitStream.
     * @see Box::parseBox()
     */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    template <typename T>
    void readTag(const String& tag, T& value);
    template <typename T>
    void writeTag(ISOBMFF::BitStream& bitstr, const String& tag, const T value);

private:
    String mXMLMetadata;
    GlobalMetadata mGlobalMetadata;
};

#endif /* end of include guard: GOOGLESPHERICALVIDEOV1BOX_HPP */
