
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
#include "recommendedviewportsampleentry.hpp"

RecommendedViewportSampleEntryBox::RecommendedViewportSampleEntryBox()
    : SphereRegionSampleEntryBox("rcvp")
    , mRecommendedViewportInfoBox()
{
}

RecommendedViewportSampleEntryBox* RecommendedViewportSampleEntryBox::clone() const
{
    return CUSTOM_NEW(RecommendedViewportSampleEntryBox, (*this));
}

RecommendedViewportInfoBox& RecommendedViewportSampleEntryBox::getRecommendedViewportInfo()
{
    return mRecommendedViewportInfoBox;
}

void RecommendedViewportSampleEntryBox::writeBox(BitStream& bitstr)
{
    SphereRegionSampleEntryBox::writeBox(bitstr);

    BitStream subStream;
    mRecommendedViewportInfoBox.writeBox(subStream);
    bitstr.writeBitStream(subStream);

    updateSize(bitstr);
}

void RecommendedViewportSampleEntryBox::parseBox(BitStream& bitstr)
{
    SphereRegionSampleEntryBox::parseBox(bitstr);
    mRecommendedViewportInfoBox.parseBox(bitstr);
}
