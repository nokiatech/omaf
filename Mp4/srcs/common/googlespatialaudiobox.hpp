
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
#ifndef SPATIALAUDIOBOX_HPP_
#define SPATIALAUDIOBOX_HPP_

#include <stdint.h>
#include <vector>
#include "bbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"

/** @brief Google Spatial Audio Box (SA3D) class. Extends from Box.
 *  @details https://github.com/google/spatial-media/blob/master/docs/spatial-audio-rfc.md */

class SpatialAudioBox : public Box
{
public:
    SpatialAudioBox();
    virtual ~SpatialAudioBox() = default;

    /** @brief Set version of box
     *  @param [in] std::uint8_t  is an 8-bit unsigned integer that specifies the version of this box.*/
    void setVersion(std::uint8_t version);

    /** @brief Get version of box
     *  @return std::uint8_t is an 8-bit unsigned integer that specifies the version of this box.*/
    std::uint8_t getVersion() const;

    /** @brief Set ambisonic type
     *  @param [in] std::uint8_t  is an 8-bit unsigned integer that specifies the type of ambisonic audio represented.
     *              0 = Periphonic: Indicates that the audio stored is a periphonic ambisonic sound field (i.e., full
     * 3D). */
    void setAmbisonicType(std::uint8_t ambisonicType);

    /** @brief Get ambisonic type
     *  @return std::uint8_t is an 8-bit unsigned integer that specifies the type of ambisonic audio represented. */
    std::uint8_t getAmbisonicType() const;

    /** @brief Set Ambisonic Order
     *  @param [in] std::uint32_t  is a 32-bit unsigned integer that specifies the order of the ambisonic sound field.*/
    void setAmbisonicOrder(std::uint32_t ambisonicOrder);

    /** @brief Get Ambisonic Order
     *  @return std::uint32_t is a 32-bit unsigned integer that specifies the order of the ambisonic sound field.*/
    std::uint32_t getAmbisonicOrder() const;

    /** @brief Set the channel ordering
     *  @param [in] std::uint8_t is an 8-bit integer specifying the channel ordering (i.e., spherical harmonics
     *                           component ordering) used in the represented ambisonic audio data.*/
    void setAmbisonicChannelOrdering(std::uint8_t ambisonicChannelOrdering);

    /** @brief Get the channel ordering
     *  @return std::uint8_t is an 8-bit unsigned integer that specifies the version of this box.*/
    std::uint8_t getAmbisonicChannelOrdering() const;

    /** @brief Set normalization
     * @param [in] std::uint8_t is an 8-bit unsigned integer specifying the normalization (i.e., spherical
     *                          harmonics normalization) used in the represented ambisonic audio data. */
    void setAmbisonicNormalization(std::uint8_t ambisonicNormalization);

    /** @brief Get normalization
     *  @return std::uint8_t is an 8-bit unsigned integer specifying the normalization (i.e., spherical
     *                          harmonics normalization) used in the represented ambisonic audio data. */
    std::uint8_t getAmbisonicNormalization() const;

    /** @brief set Channel Map
     *  @param [in] Vector<uint32_t>& is a sequence of 32-bit unsigned integers that maps audio channels in a given
     *                                audio track to ambisonic components */
    void setChannelMap(const Vector<uint32_t>& channelMap);

    /** @brief Get Channel Map
     *  @return Vector<uint32_t> is a sequence of 32-bit unsigned integers that maps audio channels in a given
     *                           audio track to ambisonic components **/
    Vector<uint32_t> getChannelMap() const;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    uint8_t mVersion;
    uint8_t mAmbisonicType;
    uint32_t mAmbisonicOrder;
    uint8_t mAmbisonicChannelOrdering;
    uint8_t mAmbisonicNormalization;
    Vector<uint32_t> mChannelMap;  // size = number of channels
};

#endif
