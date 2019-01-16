
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
#include "samplegroupdescriptionbox.hpp"

#include "bitstream.hpp"
#include "log.hpp"

#include <stdexcept>

SampleGroupDescriptionBox::SampleGroupDescriptionBox()
    : FullBox("sgpd", 0, 0)
    , mGroupingType()
    , mDefaultLength(0)
    , mSampleGroupEntry()
{
}

void SampleGroupDescriptionBox::setDefaultLength(std::uint32_t defaultLength)
{
    mDefaultLength = defaultLength;
}

std::uint32_t SampleGroupDescriptionBox::getDefaultLength() const
{
    return mDefaultLength;
}

void SampleGroupDescriptionBox::addEntry(UniquePtr<SampleGroupEntry> sampleGroupEntry)
{
    mSampleGroupEntry.push_back(std::move(sampleGroupEntry));
}

const SampleGroupEntry* SampleGroupDescriptionBox::getEntry(std::uint32_t index) const
{
    return mSampleGroupEntry.at(index - 1).get();
}

void SampleGroupDescriptionBox::setGroupingType(FourCCInt groupingType)
{
    mGroupingType = groupingType;
}

FourCCInt SampleGroupDescriptionBox::getGroupingType() const
{
    return mGroupingType;
}

std::uint32_t SampleGroupDescriptionBox::getEntryIndexOfSampleId(const std::uint32_t sampleId) const
{
    uint32_t index = 1;
    for (const auto& entry : mSampleGroupEntry)
    {
        DirectReferenceSampleListEntry* drsle = dynamic_cast<DirectReferenceSampleListEntry*>(entry.get());
        if ((drsle != nullptr) && (drsle->getSampleId() == sampleId))
        {
            return index;
        }
        ++index;
    }

    throw RuntimeError("SampleGroupDescriptionBox::getEntryIndexOfSampleId: no entry for sampleId found.");
}

void SampleGroupDescriptionBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    if (mSampleGroupEntry.size() == 0)
    {
        throw RuntimeError("SampleGroupDescriptionBox::writeBox: not writing an invalid box without entries");
    }

    // Write box headers
    writeFullBoxHeader(bitstr);

    bitstr.write32Bits(mGroupingType.getUInt32());

    if (getVersion() == 1)
    {
        bitstr.write32Bits(mDefaultLength);
    }

    bitstr.write32Bits(static_cast<unsigned int>(mSampleGroupEntry.size()));

    for (auto& entry : mSampleGroupEntry)
    {
        if (getVersion() == 1 && mDefaultLength == 0)
        {
            bitstr.write32Bits(entry->getSize());
        }
        entry->writeEntry(bitstr);
    }

    // Update the size of the movie box
    updateSize(bitstr);
}

void SampleGroupDescriptionBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    //  First parse the box header
    parseFullBoxHeader(bitstr);

    mGroupingType = bitstr.read32Bits();

    if (getVersion() == 1)
    {
        mDefaultLength = bitstr.read32Bits();
    }

    const uint32_t entryCount = bitstr.read32Bits();

    for (unsigned int i = 0; i < entryCount; ++i)
    {
        uint32_t descriptionLength = mDefaultLength;
        if (getVersion() == 1 && mDefaultLength == 0)
        {
            descriptionLength = bitstr.read32Bits();
        }

        BitStream subBitstr;
        bitstr.extract(bitstr.getPos(), bitstr.getPos() + descriptionLength,
                       subBitstr);  // extract "sub-bitstream" for entry
        bitstr.skipBytes(descriptionLength);

        if (mGroupingType == "refs")
        {
            UniquePtr<SampleGroupEntry> directReferenceSampleListEntry(CUSTOM_NEW(DirectReferenceSampleListEntry, ()));
            directReferenceSampleListEntry->parseEntry(subBitstr);
            mSampleGroupEntry.push_back(std::move(directReferenceSampleListEntry));
        }
        else
        {
            logWarning() << "Skipping an entry of SampleGroupDescriptionBox of an unknown grouping type '"
                         << mGroupingType.getString() << "'.";
        }
    }
}
