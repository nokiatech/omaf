
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
#ifndef GOOGLESPHERICALVIDEOHEADERBOX_HPP
#define GOOGLESPHERICALVIDEOHEADERBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

/**
 * @brief  Google Spherical Video Header box class
 * @details Spherical Video Header box 'svhd' box implementation as specified in
 *          https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.m
 */
class SphericalVideoHeader : public FullBox
{
public:
    SphericalVideoHeader();
    virtual ~SphericalVideoHeader() = default;

    /** @brief Sets the null-terminated string in UTF-8 characters which identifies the tool used to create the SV3D
     * metadata.
     *  @param [in] URI field as a string value  **/
    void setMetadataSource(const String& source);

    /** @brief Get the null-terminated string in UTF-8 characters which identifies the tool used to create the SV3D
     * metadata.
     *  @return name field as a string value  **/
    const String& getMetadataSource() const;

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
    String mMetadataSource;  //  is a null-terminated string in UTF-8 characters which identifies the tool used to
                             //  create the SV3D metadata.
};

#endif /* end of include guard: GOOGLESPHERICALVIDEOHEADERBOX_HPP */
