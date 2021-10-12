
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

#include "initialviewpointsampleentrybox.hpp"

::InitialViewpointSampleEntryBox::InitialViewpointSampleEntryBox() :
    MetaDataSampleEntryBox("invp")
{
    // nothing
}

::InitialViewpointSampleEntryBox::~InitialViewpointSampleEntryBox() = default;

::InitialViewpointSampleEntryBox* ::InitialViewpointSampleEntryBox::clone() const
{
    return CUSTOM_NEW(::InitialViewpointSampleEntryBox,(*this));
}

void ::InitialViewpointSampleEntryBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    MetaDataSampleEntryBox::writeBox(bitstr);
    mSampleEntry.write(bitstr);

    // Update the size of the movie box
    updateSize(bitstr);
}

void ::InitialViewpointSampleEntryBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    MetaDataSampleEntryBox::parseBox(bitstr);
    mSampleEntry.parse(bitstr);
}

/** @brief Set the sample entry */
void InitialViewpointSampleEntryBox::setSampleEntry(const ISOBMFF::InitialViewpointSampleEntry& aSampleEntry)
{
    mSampleEntry = aSampleEntry;
}

/** @brief Get the sample entry */
const ISOBMFF::InitialViewpointSampleEntry& InitialViewpointSampleEntryBox::getSampleEntry() const
{
    return mSampleEntry;
}
