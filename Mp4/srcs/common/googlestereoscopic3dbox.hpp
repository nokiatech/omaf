
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
#ifndef GOOGLESTEREOSCOPIC3DBOX_HPP
#define GOOGLESTEREOSCOPIC3DBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

/**
 * @brief  Google Stereoscopic 3D Video Box (st3d) box class
 * @details Stereoscopic 3D Video Box (st3d) box implementation as specified in
 *          https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.m
 */
class Stereoscopic3D : public FullBox
{
public:
    Stereoscopic3D();
    virtual ~Stereoscopic3D() = default;

    enum class V2StereoMode : uint8_t
    {
        MONOSCOPIC                = 0,
        STEREOSCOPIC_TOPBOTTOM    = 1,
        STEREOSCOPIC_LEFTRIGHT    = 2,
        STEREOSCOPIC_STEREOCUSTOM = 3
    };

    /** @brief Set stereo frame layout.
     *   @param [in] V2StereoMode enum containing stereo frame layout **/
    void setStereoMode(const V2StereoMode& stereoMode);

    /** @brief Get stereo frame layout.
     *  @return V2StereoMode enum containing stereo frame layout  **/
    V2StereoMode getStereoMode() const;

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
    V2StereoMode mStereoMode;  //  is a null-terminated string in UTF-8 characters which identifies the tool used to
                               //  create the SV3D metadata.
};

#endif /* end of include guard: GOOGLESTEREOSCOPIC3DBOX_HPP */
