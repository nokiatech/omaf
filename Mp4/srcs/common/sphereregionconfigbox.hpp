
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
#ifndef SPHEREREGIONCONFIGBOX_HPP
#define SPHEREREGIONCONFIGBOX_HPP

#include "api/isobmff/commontypes.h"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

/** @brief OMAF SphereRegionConfigBox class.
 */

class SphereRegionConfigBox : public FullBox
{
public:
    enum class ShapeType : std::uint8_t
    {
        FourGreatCircles                 = 0,
        TwoAzimuthAndTwoElevationCircles = 1
    };

    SphereRegionConfigBox();
    virtual ~SphereRegionConfigBox() = default;

    void setFrom(const ISOBMFF::SphereRegionConfigStruct& rosc);
    ISOBMFF::SphereRegionConfigStruct get() const;

    void setShapeType(ShapeType shapeType);
    ShapeType getShapeType();

    void setDynamicRangeFlag(bool shapeType);
    bool getDynamicRangeFlag();

    void setStaticAzimuthRange(std::uint32_t range);
    std::uint32_t getStaticAzimuthRange();

    void setStaticElevationRange(std::uint32_t range);
    std::uint32_t getStaticElevationRange();

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);


private:
    ISOBMFF::SphereRegionConfigStruct mSphereRegionConfig;
};

#endif  // SPHEREREGIONCONFIGBOX_HPP
