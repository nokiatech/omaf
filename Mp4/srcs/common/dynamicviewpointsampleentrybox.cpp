
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
#include <cassert>

#include "dynamicviewpointsampleentrybox.hpp"

::DynamicViewpointSampleEntryBox::DynamicViewpointSampleEntryBox() :
    MetaDataSampleEntryBox("dyvp")
{
    // nothing
}

::DynamicViewpointSampleEntryBox::~DynamicViewpointSampleEntryBox() = default;

::DynamicViewpointSampleEntryBox* ::DynamicViewpointSampleEntryBox::clone() const
{
    return CUSTOM_NEW(::DynamicViewpointSampleEntryBox,(*this));
}

void ::DynamicViewpointSampleEntryBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    MetaDataSampleEntryBox::writeBox(bitstr);
    mSampleEntry.write(bitstr);

    // Update the size of the movie box
    updateSize(bitstr);
}

void ::DynamicViewpointSampleEntryBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    MetaDataSampleEntryBox::parseBox(bitstr);
    mSampleEntry.parse(bitstr);
}

/** @brief Set the sample entry */
void DynamicViewpointSampleEntryBox::setSampleEntry(const ISOBMFF::DynamicViewpointSampleEntry& aSampleEntry)
{
    mSampleEntry = aSampleEntry;
}

/** @brief Get the sample entry */
const ISOBMFF::DynamicViewpointSampleEntry& DynamicViewpointSampleEntryBox::getSampleEntry() const
{
    return mSampleEntry;
}
