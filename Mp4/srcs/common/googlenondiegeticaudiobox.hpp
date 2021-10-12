
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
#ifndef NONDIEGETICAUDIOBOX_HPP_
#define NONDIEGETICAUDIOBOX_HPP_

#include <stdint.h>
#include <vector>
#include "bbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"

/** @brief Google Non-Diegetic Audio Box (SAND) class. Extends from Box.
 *  @details https://github.com/google/spatial-media/blob/master/docs/spatial-audio-rfc.md */

class NonDiegeticAudioBox : public Box
{
public:
    NonDiegeticAudioBox();
    virtual ~NonDiegeticAudioBox() = default;

    /** @brief Set version of box
     *  @param [in] std::uint8_t  is an 8-bit unsigned integer that specifies the version of this box.*/
    void setVersion(std::uint8_t version);

    /** @brief Get version of box
     *  @return std::uint8_t is an 8-bit unsigned integer that specifies the version of this box.*/
    std::uint8_t getVersion() const;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    uint8_t mVersion;
};

#endif /* NONDIEGETICAUDIOBOX_HPP_ */
