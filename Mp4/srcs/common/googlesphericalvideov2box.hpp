
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
#ifndef GOOGLESPHERICALVIDEOV2BOX_HPP
#define GOOGLESPHERICALVIDEOV2BOX_HPP

#include "bbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "googleprojectionbox.hpp"
#include "googlesphericalvideoheaderbox.hpp"

/**
 * @brief  Google Spherical Video Box class
 * @details Google Spherical Video box 'sv3d' implementation as specified in
 *          https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.m
 */
class SphericalVideoV2Box : public Box
{
public:
    SphericalVideoV2Box();
    virtual ~SphericalVideoV2Box() = default;

    /** @brief Get SphericalVideoHeader Box.
     *  @returns Reference to the SphericalVideoHeader.**/
    const SphericalVideoHeader& getSphericalVideoHeaderBox() const;

    /** @brief Set SphericalVideoHeader Box.
     *  @param [in] Reference to the SphericalVideoHeader **/
    void setSphericalVideoHeaderBox(const SphericalVideoHeader& header);

    /** @brief Get Projection Box.
     *  @returns Reference to the Projection.**/
    const Projection& getProjectionBox() const;

    /** @brief Set Projection Box.
     *  @param [in] Reference to the Projection **/
    void setProjectionBox(const Projection& projectionBox);

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
    SphericalVideoHeader mSphericalVideoHeaderBox;
    Projection mProjectionBox;
};

#endif /* end of include guard: GOOGLESPHERICALVIDEOV2BOX_HPP */
