
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
#ifndef GOOGLEPROJECTIONBOX_HPP
#define GOOGLEPROJECTIONBOX_HPP

#include "bbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "googlecubemapprojectionbox.hpp"
#include "googleequirectangularprojectionbox.hpp"
#include "googleprojectionheaderbox.hpp"

/**
 * @brief  Google Projection box class
 * @details Google Projection box 'proj' implementation as specified in
 *          https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.m
 */
class Projection : public Box
{
public:
    Projection();
    virtual ~Projection() = default;

    enum class ProjectionType : uint8_t
    {
        UNKNOWN = 0,

        CUBEMAP         = 1,
        EQUIRECTANGULAR = 2,
        MESH            = 3
    };

    /** @brief Get ProjectionHeader Box.
     *  @returns Reference to the ProjectionHeader.**/
    const ProjectionHeader& getProjectionHeaderBox() const;

    /** @brief Set ProjectionHeader Box.
     *  @param [in] Reference to the ProjectionHeader **/
    void setProjectionHeaderBox(const ProjectionHeader& header);

    /** @brief Gets the type of projection.
     *  @return ProjectionType enumeration**/
    ProjectionType getProjectionType() const;

    /** @brief Get CubemapProjection Box.
     *  @returns Reference to the CubemapProjection.**/
    const CubemapProjection& getCubemapProjectionBox() const;

    /** @brief Set CubemapProjection Box.
     *  @param [in] Reference to the CubemapProjection **/
    void setCubemapProjectionBox(const CubemapProjection& projection);

    /** @brief Get EquirectangularProjection Box.
     *  @returns Reference to the EquirectangularProjection.**/
    const EquirectangularProjection& getEquirectangularProjectionBox() const;

    /** @brief Set EquirectangularProjection Box.
     *  @param [in] Reference to the EquirectangularProjection **/
    void setEquirectangularProjectionBox(const EquirectangularProjection& projection);

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
    ProjectionHeader mProjectionHeaderBox;
    ProjectionType mProjectionType;
    CubemapProjection mCubemapProjectionBox;                  // contains either this
    EquirectangularProjection mEquirectangularProjectionBox;  // or this
};

#endif /* end of include guard: GOOGLEPROJECTIONBOX_HPP */
