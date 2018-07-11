
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
#ifndef PROJECTIONDATABOX_HPP
#define PROJECTIONDATABOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

/**
 * @brief Google Spatial Media Projection Data Box class
 * @details ProjectionDataBox box implementation as specified in the
 * https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.md
 */
class ProjectionDataBox : public FullBox
{
public:
    ProjectionDataBox(FourCCInt proj_type, uint8_t version, uint32_t flags);
    virtual ~ProjectionDataBox() = default;

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
};

#endif /* end of include guard: PROJECTIONDATABOX_HPP */
