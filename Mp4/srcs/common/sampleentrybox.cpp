
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
#include "sampleentrybox.hpp"
#include "bitstream.hpp"
#include "log.hpp"

using std::endl;

static const int RESERVED_BYTES = 6;

SampleEntryBox::SampleEntryBox(FourCCInt codingname)
    : Box(codingname)
    , mDataReferenceIndex(0)
    , mRestrictedSchemeInfoBox(nullptr)
{
}

SampleEntryBox::SampleEntryBox(const SampleEntryBox& box)
    : Box(box.getType())
    , mDataReferenceIndex(box.mDataReferenceIndex)
{
    if (box.mRestrictedSchemeInfoBox)
    {
        mRestrictedSchemeInfoBox =
            makeCustomUnique<RestrictedSchemeInfoBox, RestrictedSchemeInfoBox>(*box.mRestrictedSchemeInfoBox);
    }
}

std::uint16_t SampleEntryBox::getDataReferenceIndex() const
{
    return mDataReferenceIndex;
}

void SampleEntryBox::setDataReferenceIndex(std::uint16_t dataReferenceIndex)
{
    mDataReferenceIndex = dataReferenceIndex;
}

void SampleEntryBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);

    for (int i = 0; i < RESERVED_BYTES; ++i)
    {
        bitstr.write8Bits(0);  // reserved = 0
    }

    bitstr.write16Bits(mDataReferenceIndex);

    // Update the size of the movie box
    updateSize(bitstr);
}

void SampleEntryBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    for (int i = 0; i < RESERVED_BYTES; ++i)
    {
        bitstr.read8Bits();  // reserved
    }

    mDataReferenceIndex = bitstr.read16Bits();
}

void SampleEntryBox::addRestrictedSchemeInfoBox(UniquePtr<RestrictedSchemeInfoBox> restrictedSchemeInfoBox)
{
    mRestrictedSchemeInfoBox = std::move(restrictedSchemeInfoBox);
}

RestrictedSchemeInfoBox* SampleEntryBox::getRestrictedSchemeInfoBox() const
{
    return mRestrictedSchemeInfoBox.get();
}

bool SampleEntryBox::isVisual() const
{
    return false;
}
