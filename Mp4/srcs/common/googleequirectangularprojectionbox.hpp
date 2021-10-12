
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
#ifndef EQUIRECTANGULARPROJECTION_HPP
#define EQUIRECTANGULARPROJECTION_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"
#include "googleprojectiondatabox.hpp"

/**
 * @brief Google Spatial Media Equirectangular Projection class
 * @details EquirectangularProjection box implementation as specified in the
 *          https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.m
 *
 *          The equirectangular projection should be arranged such that the default pose has the forward vector in the
 * center of the frame, the up vector at top of the frame, and the right vector towards the right of the frame.
 */
class EquirectangularProjection : public ProjectionDataBox
{
public:
    EquirectangularProjection();
    virtual ~EquirectangularProjection() = default;

    struct ProjectionBounds
    {
        uint32_t top_0_32_FP;     // is the amount from the top of the frame to crop
        uint32_t bottom_0_32_FP;  // is the amount from the bottom of the frame to crop; must be less than 0xFFFFFFFF -
                                  // projection_bounds_top
        uint32_t left_0_32_FP;    // is the amount from the left of the frame to crop
        uint32_t right_0_32_FP;   // is the amount from the right of the frame to crop; must be less than 0xFFFFFFFF -
                                  // projection_bounds_left
    };

    /** @brief Set projection bounds for projection.
     *  @param [in] struct ProjectionBounds projection bounds */
    void setProjectionBounds(ProjectionBounds& bounds);

    /** @brief Get projection bounds for projection.
     *  @return struct ProjectionBounds projection bounds */
    const ProjectionBounds& getProjectionBounds() const;

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
    ProjectionBounds mProjectionBounds;
};

#endif /* end of include guard: EQUIRECTANGULARPROJECTION_HPP */
