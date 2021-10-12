
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
#ifndef MOVIEEXTENDSHEADERBOX_HPP
#define MOVIEEXTENDSHEADERBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

/**
 * @brief  Movie Extends Header Box class
 * @details 'mehd' box implementation as specified in the ISOBMFF specification.
 */
class MovieExtendsHeaderBox : public FullBox
{
public:
    MovieExtendsHeaderBox(uint8_t version = 0);
    virtual ~MovieExtendsHeaderBox() = default;

    /** @brief Set Fragment Duration of the MovieExtendsHeaderBox.
     *  @param [in] uint64_t fragmentDuration**/
    void setFragmentDuration(const uint64_t fragmentDuration);

    /** @brief Get Fragment Duration of the MovieExtendsHeaderBox.
     *  @return uint64_t fragmentDuration as specified in 8.8.2.1 of ISO/IEC 14496-12:2015(E)**/
    uint64_t getFragmentDuration() const;

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
    uint64_t mFragmentDuration;
};

#endif /* end of include guard: MOVIEEXTENDSHEADERBOX_HPP */
