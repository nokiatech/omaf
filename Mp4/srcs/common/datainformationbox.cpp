
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
#include "datainformationbox.hpp"
#include "log.hpp"

#include <stdexcept>

DataInformationBox::DataInformationBox()
    : Box("dinf")
    , mDataReferenceBox()
{
}

std::uint16_t DataInformationBox::addDataEntryBox(std::shared_ptr<DataEntryBox> dataEntryBox)
{
    return static_cast<std::uint16_t>(mDataReferenceBox.addEntry(dataEntryBox));
}

void DataInformationBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);
    mDataReferenceBox.writeBox(bitstr);
    updateSize(bitstr);
}

void DataInformationBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    if (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);
        mDataReferenceBox.parseBox(subBitstr);
    }
    else
    {
        logError() << "Read an empty dinf box." << std::endl;
    }
}

const DataReferenceBox& DataInformationBox::getDataReferenceBox() const
{
    return mDataReferenceBox;
}
