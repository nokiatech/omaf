
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
#ifndef AVCSAMPLEENTRY_HPP
#define AVCSAMPLEENTRY_HPP

#include "avcconfigurationbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "googlesphericalvideov2box.hpp"
#include "googlestereoscopic3dbox.hpp"
#include "visualsampleentrybox.hpp"

/** @brief AVC Sample Entry class. Extends from VisualSampleEntry.
 *  @details 'avc1' box implementation as specified in the ISO/IEC 14496-15 specification
 *  @details AvcConfigurationBox is mandatory to be present
 *  @todo Implement other AvcSampleEntry internal boxes or extra boxes if needed:
 *  @todo MPEG4BitRateBox, MPEG4ExtensionDescriptorsBox, extra_boxes **/

class AvcSampleEntry : public VisualSampleEntryBox
{
public:
    AvcSampleEntry();
    AvcSampleEntry(const AvcSampleEntry& box);
    virtual ~AvcSampleEntry() = default;

    /** @brief Gets the AvcConfigurationBox
     *  @return Reference to the AvcConfigurationBox **/
    AvcConfigurationBox& getAvcConfigurationBox();

    /** @brief Create Stereoscopic3D */
    void createStereoscopic3DBox();

    /** @brief Create SphericalVideoV2Box */
    void createSphericalVideoV2Box();

    /** @brief Gets the Stereoscopic3DBox
     *  @return Pointer to Stereoscopic3DBox, if present. */
    virtual const Stereoscopic3D* getStereoscopic3DBox() const override;

    /** @brief Gets the SphericalVideoV2BoxBox
     *  @return Pointer to SphericalVideoV2BoxBox, if present. */
    virtual const SphericalVideoV2Box* getSphericalVideoV2BoxBox() const override;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Parses a AvcSampleEntry Box bitstream and fills in the necessary member variables
     *  @details If there is an unknown box present then a warning is logged
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr) override;

    /* @brief Make a copy of this box that has dynamically the same type as this */
    virtual AvcSampleEntry* clone() const override;

    /* @brief Returns the configuration record for this sample */
    virtual const DecoderConfigurationRecord* getConfigurationRecord() const override;

    /* @brief Returns the configuration box for this sample */
    virtual const Box* getConfigurationBox() const override;

private:
    AvcConfigurationBox mAvcConfigurationBox;

    // Google Spatial Media boxes:
    bool mIsStereoscopic3DPresent;
    Stereoscopic3D mStereoscopic3DBox;
    bool mIsSphericalVideoV2BoxPresent;
    SphericalVideoV2Box mSphericalVideoV2Box;
};

#endif
