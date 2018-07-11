
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
#ifndef CUBEMAPPROJECTIONBOX_HPP
#define CUBEMAPPROJECTIONBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"
#include "googleprojectiondatabox.hpp"

/**
 * @brief Google Spatial Media Cubemap Projection Box class
 * @details CubemapProjection box implementation as specified in the
 *          https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.md
 */
class CubemapProjection : public ProjectionDataBox
{
public:
    CubemapProjection();
    virtual ~CubemapProjection() = default;

    /** @brief Set Layout of cube faces. The values 0 to 255 are reserved for current and future layouts.
     *  @detail A value of 0 corresponds to a grid with 3 columns and 2 rows.
     *          Faces are oriented upwards for the front, left, right, and back faces.
     *          The up face is oriented so the top of the face is forwards and the down face is oriented so the top of
     * the face is to the back.
     *
     *          right face	left face	up face
     *          down face	front face	back face
     *
     *  @param [in] uint32_t layout of cube faces */
    void setLayout(uint32_t layout);

    /** @brief Get Layout of cube faces.
     *  @detail A value of 0 corresponds to a grid with 3 columns and 2 rows.
     *          The values 0 to 255 are reserved for current and future layouts.
     *          Faces are oriented upwards for the front, left, right, and back faces.
     *          The up face is oriented so the top of the face is forwards and the down face is oriented so the top of
     * the face is to the back.
     *
     *          right face	left face	up face
     *          down face	front face	back face
     *
     *  @return uint32_t layout of cube faces */
    uint32_t getLayout() const;

    /** @brief Set padding integer measuring the number of pixels to pad from the edge of each cube face
     *  @param [in] uint32_t padding integer */
    void setPadding(uint32_t padding);

    /** @brief Get padding integer measuring the number of pixels to pad from the edge of each cube face
     *  @return uint32_t padding integer */
    uint32_t getPadding() const;

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
    uint32_t mLayout;
    uint32_t mPadding;
};

#endif /* end of include guard: CUBEMAPPROJECTIONBOX_HPP */
