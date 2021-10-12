
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
#include "recommendedviewportinfobox.hpp"
#include "bitstream.hpp"

RecommendedViewportInfoBox::RecommendedViewportInfoBox()
    : FullBox("rvif", 0, 0)
    , mRecommendedViewportInfo()
{
}

void RecommendedViewportInfoBox::setFrom(const ISOBMFF::RecommendedViewportInfoStruct& rcvpInfo)
{
    mRecommendedViewportInfo = rcvpInfo;
}

ISOBMFF::RecommendedViewportInfoStruct RecommendedViewportInfoBox::get() const
{
    return mRecommendedViewportInfo;
}

std::uint8_t RecommendedViewportInfoBox::getViewportType() const
{
    return mRecommendedViewportInfo.type;
}

const String RecommendedViewportInfoBox::getDescription() const
{
    return String(mRecommendedViewportInfo.description.elements, mRecommendedViewportInfo.description.numElements);
}

void RecommendedViewportInfoBox::setViewportType(std::uint8_t type)
{
    mRecommendedViewportInfo.type = type;
}

void RecommendedViewportInfoBox::setDescription(const String& description)
{
    mRecommendedViewportInfo.description =
        ISOBMFF::DynArray<char>(description.data(), description.data() + description.size());
}

void RecommendedViewportInfoBox::writeBox(BitStream& bitstream)
{
    writeFullBoxHeader(bitstream);
    bitstream.write8Bits(mRecommendedViewportInfo.type);
    bitstream.writeZeroTerminatedString(
        String(mRecommendedViewportInfo.description.elements, mRecommendedViewportInfo.description.numElements));
    updateSize(bitstream);
}

void RecommendedViewportInfoBox::parseBox(BitStream& bitstream)
{
    parseFullBoxHeader(bitstream);
    mRecommendedViewportInfo.type = bitstream.read8Bits();
    String description;
    bitstream.readZeroTerminatedString(description);
    mRecommendedViewportInfo.description =
        ISOBMFF::DynArray<char>(description.data(), description.data() + description.size());
}
