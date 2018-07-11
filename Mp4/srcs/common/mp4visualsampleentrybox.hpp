
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
#ifndef MP4VISUALSAMPLEENTRYBOX_HPP
#define MP4VISUALSAMPLEENTRYBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "elementarystreamdescriptorbox.hpp"
#include "googlesphericalvideov2box.hpp"
#include "googlestereoscopic3dbox.hpp"
#include "visualsampleentrybox.hpp"

/** @brief MP4VIsualSampleEntryBox class. Extends from VisualSampleEntryBox.
 *  @details This box contains information related to the mp4 visual samples of the track
 *  @details as defined in the ISO/IEC FDIS 14496-14 standard. **/

class MP4VisualSampleEntryBox : public VisualSampleEntryBox
{
public:
    MP4VisualSampleEntryBox();
    MP4VisualSampleEntryBox(const MP4VisualSampleEntryBox& box);
    virtual ~MP4VisualSampleEntryBox() = default;

    /** @brief Create Stereoscopic3D box */
    void createStereoscopic3DBox();

    /** @brief Create SphericalVideoV2Box */
    void createSphericalVideoV2Box();

    /** @brief Gets sample's ElementaryStreamDescritorBox as defined in ISO/IEC FDIS 14496-14 standard
     *  @returns Sample's ElementaryStreamDescritorBox **/
    ElementaryStreamDescriptorBox& getESDBox();

    /** @brief Gets the Stereoscopic3DBox
     *  @return Pointer to Stereoscopic3DBox, if present. */
    virtual const Stereoscopic3D* getStereoscopic3DBox() const override;

    /** @brief Gets the SphericalVideoV2BoxBox
     *  @return Pointer to SphericalVideoV2BoxBox, if present. */
    virtual const SphericalVideoV2Box* getSphericalVideoV2BoxBox() const override;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Parses a AudioSampleEntryBox bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr) override;

    virtual MP4VisualSampleEntryBox* clone() const override;

    /* @brief Returns the configuration record for this sample */
    virtual const DecoderConfigurationRecord* getConfigurationRecord() const override;

    /* @brief Returns the configuration box for this sample */
    virtual const Box* getConfigurationBox() const override;

private:
    ElementaryStreamDescriptorBox mESDBox;

    // Google Spatial Media boxes:
    bool mIsStereoscopic3DPresent;
    Stereoscopic3D mStereoscopic3DBox;
    bool mIsSphericalVideoV2BoxPresent;
    SphericalVideoV2Box mSphericalVideoV2Box;
};

#endif  // MP4VISUALSAMPLEENTRYBOX_HPP
