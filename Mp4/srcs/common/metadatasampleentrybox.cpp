
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
#include "metadatasampleentrybox.hpp"
#include "log.hpp"

MetaDataSampleEntryBox::MetaDataSampleEntryBox(FourCCInt codingname)
    : SampleEntryBox(codingname)
{
}

void MetaDataSampleEntryBox::writeBox(BitStream& bitstr)
{
    SampleEntryBox::writeBox(bitstr);

    // Update the size of the movie box
    updateSize(bitstr);
}

void MetaDataSampleEntryBox::parseBox(BitStream& bitstr)
{
    SampleEntryBox::parseBox(bitstr);
}

const Box* MetaDataSampleEntryBox::getConfigurationBox() const
{
    logError() << "MetaDataSampleEntryBox::getConfigurationBox() not impelmented " << std::endl;
    return nullptr;
}

const DecoderConfigurationRecord* MetaDataSampleEntryBox::getConfigurationRecord() const
{
    logError() << "MetaDataSampleEntryBox::getConfigurationRecord() not impelmented" << std::endl;
    return nullptr;
}
