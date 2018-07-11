
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
#include "userdatabox.hpp"
#include "bitstream.hpp"

using std::endl;

UserDataBox::UserDataBox()
    : Box("udta")
{
}

/** @brief Retrieve the boxes inside the box **/
bool UserDataBox::getBox(Box& box) const
{
    FourCCInt boxType = box.getType();
    auto bsIt         = mBitStreams.find(boxType);
    if (bsIt != mBitStreams.end())
    {
        // Copy as Box::parseBox takes a non-const argument
        BitStream bs = bsIt->second;
        box.parseBox(bs);
        return true;
    }
    else
    {
        return false;
    }
}

void UserDataBox::addBox(Box& box)
{
    ISOBMFF::BitStream bitstream;
    box.writeBox(bitstream);
    bitstream.reset();
    mBitStreams[box.getType()] = std::move(bitstream);
}

void UserDataBox::writeBox(BitStream& bitstr)
{
    writeBoxHeader(bitstr);

    for (auto& typeAndBitstream : mBitStreams)
    {
        bitstr.writeBitStream(typeAndBitstream.second);
    }

    updateSize(bitstr);
}

void UserDataBox::parseBox(BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    while (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        mBitStreams[boxType] = std::move(subBitstr);
    }
}
