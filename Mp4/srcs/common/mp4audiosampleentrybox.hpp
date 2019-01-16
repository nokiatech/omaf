
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#ifndef MP4AUDIOSAMPLEENTRYBOX_HPP
#define MP4AUDIOSAMPLEENTRYBOX_HPP

#include "audiosampleentrybox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "elementarystreamdescriptorbox.hpp"
#include "googlenondiegeticaudiobox.hpp"
#include "googlespatialaudiobox.hpp"
#include "mp4audiodecoderconfigrecord.hpp"

/** @brief AudioSampleEntryBox class. Extends from SampleEntryBox.
 *  @details This box contains information related to the mp4 audio samples of the track
 *  @details as defined in the ISO/IEC FDIS 14496-14 standard. **/

class MP4AudioSampleEntryBox : public AudioSampleEntryBox
{
public:
    MP4AudioSampleEntryBox();
    virtual ~MP4AudioSampleEntryBox() = default;

    /** @brief Gets sample's ElementaryStreamDescritorBox as defined in ISO/IEC FDIS 14496-14 standard
     *  @returns Sample's ElementaryStreamDescritorBox **/
    ElementaryStreamDescriptorBox& getESDBox();

    /** @brief Gets sample's ElementaryStreamDescritorBox as defined in ISO/IEC FDIS 14496-14 standard
     *  @returns Sample's ElementaryStreamDescritorBox **/
    const ElementaryStreamDescriptorBox& getESDBox() const;

    /** @brief Gets whether MP4AudioSampleEntryBox contains Google Spatial Audio Box (SA3D)
     *  @returns true if present **/
    bool hasSpatialAudioBox() const;

    /** @brief Gets sample's SpatialAudioBox
     *  @returns Sample's SpatialAudioBox **/
    const SpatialAudioBox& getSpatialAudioBox() const;

    /** @brief Gets sample's SpatialAudioBox
     *  @returns Sample's SpatialAudioBox **/
    void setSpatialAudioBox(const SpatialAudioBox&);

    /** @brief Gets whether MP4AudioSampleEntryBox contains Google Non-Diegetic Audio Box (SAND)
     *  @returns true if present **/
    bool hasNonDiegeticAudioBox() const;

    /** @brief Gets sample's NonDiegeticAudioBox
     *  @returns Sample's NonDiegeticAudioBox **/
    const NonDiegeticAudioBox& getNonDiegeticAudioBox() const;

    /** @brief Gets sample's NonDiegeticAudioBox
     *  @returns Sample's NonDiegeticAudioBox **/
    void setNonDiegeticAudioBox(const NonDiegeticAudioBox&);

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a AudioSampleEntryBox bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

    virtual MP4AudioSampleEntryBox* clone() const;

    /* @brief Returns the configuration record for this sample */
    virtual const DecoderConfigurationRecord* getConfigurationRecord() const override;

    /* @brief Returns the configuration box for this sample */
    virtual const Box* getConfigurationBox() const override;

private:
    ElementaryStreamDescriptorBox mESDBox;
    bool mHasSpatialAudioBox;
    SpatialAudioBox mSpatialAudioBox;  // Google Spatial Audio Box (SA3D)
    bool mHasNonDiegeticAudioBox;
    NonDiegeticAudioBox mNonDiegeticAudioBox;  // Google Non-Diegetic Audio Box (SAND)

    MP4AudioDecoderConfigurationRecord mRecord;
};

#endif  // MP4AUDIOSAMPLEENTRYBOX_HPP
