
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
#ifndef GOOGLEPROJECTIONHEADERBOX_HPP
#define GOOGLEPROJECTIONHEADERBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

/**
 * @brief  Google Projection Header box class
 * @details Projection Header box 'prhd' box implementation as specified in
 *          https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.m
 */
class ProjectionHeader : public FullBox
{
public:
    ProjectionHeader();
    virtual ~ProjectionHeader() = default;

    struct PoseInDegrees_16_16_FP
    {
        int32_t yaw;
        int32_t pitch;
        int32_t roll;
    };

    /** @brief Sets Pose for ProjectionHeader box
     *  @param [in] PoseInDegrees_16_16_FP Pose values are 16.16 fixed point values measuring rotation in degrees. **/
    void setPose(const PoseInDegrees_16_16_FP& pose);

    /** @brief Get Pose of the ProjectionHeader.
     *  @return PoseInDegrees_16_16_FP Pose values are 16.16 fixed point values measuring rotation in degrees. **/
    const PoseInDegrees_16_16_FP& getPose() const;

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
    PoseInDegrees_16_16_FP mPose;  // Pose values are 16.16 fixed point values measuring rotation in degrees.
};

#endif /* end of include guard: GOOGLEPROJECTIONHEADERBOX_HPP */
