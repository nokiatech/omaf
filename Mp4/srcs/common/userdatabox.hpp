
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
#ifndef USERDATABOX_HPP
#define USERDATABOX_HPP

#include <cstdint>
#include <functional>
#include "bbox.hpp"
#include "customallocator.hpp"

/** @brief Sample Entry Box class. Extends from Box.
 *  @details Contains Sample Entry data structure as defined in the ISOBMFF standard. **/
class UserDataBox : public Box
{
public:
    UserDataBox();
    virtual ~UserDataBox() = default;

    /** @brief Retrieve a box from inside
        @param [in out] box The box to read, where the result is returned
        @returns true if the box was found **/
    bool getBox(Box& box) const;

    /** @brief Add a box inside this box
     *  @param [in] box The box to put in **/
    void addBox(Box& box);

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a UserDataBox bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    Map<FourCCInt, ISOBMFF::BitStream> mBitStreams;
};

#endif
