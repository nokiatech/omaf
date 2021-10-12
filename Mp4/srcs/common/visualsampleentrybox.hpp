
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
#ifndef VISUALSAMPLEENTRYBOX_HPP
#define VISUALSAMPLEENTRYBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "sampleentrybox.hpp"

class CleanApertureBox;
class Stereoscopic3D;
class SphericalVideoV2Box;

/** @brief VisualSampleEntryBox class. Extends from SampleEntryBox.
 *  @details This box contains information related to the visual samples of the track as defined in the ISOBMFF
 * standard.
 *  @details there may be multiple visual sample entries which map to different samples in the track. **/

class VisualSampleEntryBox : public SampleEntryBox
{
public:
    VisualSampleEntryBox(FourCCInt codingName, const String& compressorName);
    VisualSampleEntryBox(const VisualSampleEntryBox& box);

    virtual ~VisualSampleEntryBox() = default;

    /** @brief Sets sample's display width as defined in ISOBMFF
     *  @param [in] width sample's display width **/
    void setWidth(std::uint16_t width);

    /** @brief Gets sample's display width as defined in ISOBMFF
     *  @returns Sample's display width **/
    std::uint32_t getWidth() const;

    /** @brief Sets sample's display height as defined in ISOBMFF
     *  @param [in] height sample's display height **/
    void setHeight(std::uint16_t height);

    /** @brief Gets sample's display height as defined in ISOBMFF
     *  @returns Sample's display height **/
    std::uint32_t getHeight() const;

    /** @brief Creates sample's clean aperture data structure as defined in ISOBMFF */
    void createClap();

    /** @brief Gets sample's clean aperture data structure as defined in ISOBMFF
     *  @returns Sample's clean aperture data structure */
    const CleanApertureBox* getClap() const;
    CleanApertureBox* getClap();

    /** @brief Gets the Stereoscopic3DBox from the derived class instance
     *  @return Pointer to Stereoscopic3DBox if present, nullptr if not. */
    virtual const Stereoscopic3D* getStereoscopic3DBox() const
    {
        return nullptr;
    }

    void setOverlayConfigBox(const ISOBMFF::Optional<OverlayConfigBox>& anOvly);
    const ISOBMFF::Optional<OverlayConfigBox> getOverlayConfigBox() const;

    /** @brief Check if Stereoscopic3DBox is present
     *  @return TRUE if Stereoscopic3DBox is present, FALSE otherwise */
    bool isStereoscopic3DBoxPresent() const
    {
        // Check if pointer to Stereoscopic3DBox is valid, doesn't modify anything.
        return (const_cast<VisualSampleEntryBox*>(this)->getStereoscopic3DBox() != nullptr);
    }

    /** @brief Gets the SphericalVideoV2BoxBox from the derived class instance
     *  @return Pointer to SphericalVideoV2BoxBox if present, nullptr if not. */
    virtual const SphericalVideoV2Box* getSphericalVideoV2BoxBox() const
    {
        return nullptr;
    }

    /** @brief Check if SphericalVideoV2BoxBox is present
     *  @return TRUE if SphericalVideoV2BoxBox is present, FALSE otherwise */
    bool isSphericalVideoV2BoxBoxPresent() const
    {
        // Check if pointer to SphericalVideoV2BoxBox is valid, doesn't modify anything.
        return (const_cast<VisualSampleEntryBox*>(this)->getSphericalVideoV2BoxBox() != nullptr);
    }

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    void writeBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Parses a VisualSampleEntryBox bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    void parseBox(ISOBMFF::BitStream& bitstr) override;

    bool isVisual() const override;

private:
    std::uint16_t mWidth;                       ///< Sample display width
    std::uint16_t mHeight;                      ///< Sample display height
    String mCompressorName;                     ///< Compressor name used, e.g. "HEVC Coding"
    std::shared_ptr<CleanApertureBox> mClap;    ///< Clean Aperture data structure
    ISOBMFF::Optional<OverlayConfigBox> mOvly;  ///< Overlay config box
};

#endif
