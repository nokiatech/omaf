
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
#include "sphereregionsampleentrybox.hpp"

SphereRegionSampleEntryBox::SphereRegionSampleEntryBox(FourCCInt codingname)
    : MetaDataSampleEntryBox(codingname)
{
}

SphereRegionConfigBox& SphereRegionSampleEntryBox::getSphereRegionConfig()
{
    return mSphereRegionConfig;
}

void SphereRegionSampleEntryBox::writeBox(BitStream& bitstr)
{
    MetaDataSampleEntryBox::writeBox(bitstr);

    BitStream subStream;
    mSphereRegionConfig.writeBox(subStream);
    bitstr.writeBitStream(subStream);

    updateSize(bitstr);
}

void SphereRegionSampleEntryBox::parseBox(BitStream& bitstr)
{
    MetaDataSampleEntryBox::parseBox(bitstr);

    FourCCInt boxType;
    auto sphrereRegionConfigBoxStream = bitstr.readSubBoxBitStream(boxType);
    mSphereRegionConfig.parseBox(sphrereRegionConfigBoxStream);
}

SphereRegionSampleEntryBox::SphereRegionSample::SphereRegionSample()
{
    // there must always be single region in sample
    regions.push_back(SphereRegion());
}
