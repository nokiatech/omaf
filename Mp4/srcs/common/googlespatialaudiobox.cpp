
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
#include "googlespatialaudiobox.hpp"

/** @brief Google Spatial Audio Box (SA3D) class. Extends from Box.
 *  @details https://github.com/google/spatial-media/blob/master/docs/spatial-audio-rfc.md */

SpatialAudioBox::SpatialAudioBox()
    : Box("SA3D")
    , mVersion(0)
    , mAmbisonicType(0)
    , mAmbisonicOrder(1)
    , mAmbisonicChannelOrdering(0)
    , mAmbisonicNormalization(0)
    , mChannelMap()
{
}

void SpatialAudioBox::setVersion(std::uint8_t version)
{
    mVersion = version;
}

std::uint8_t SpatialAudioBox::getVersion() const
{
    return mVersion;
}

void SpatialAudioBox::setAmbisonicType(std::uint8_t ambisonicType)
{
    mAmbisonicType = ambisonicType;
}

std::uint8_t SpatialAudioBox::getAmbisonicType() const
{
    return mAmbisonicType;
}

void SpatialAudioBox::setAmbisonicOrder(std::uint32_t ambisonicOrder)
{
    mAmbisonicOrder = ambisonicOrder;
}

std::uint32_t SpatialAudioBox::getAmbisonicOrder() const
{
    return mAmbisonicOrder;
}

void SpatialAudioBox::setAmbisonicChannelOrdering(std::uint8_t ambisonicChannelOrdering)
{
    mAmbisonicChannelOrdering = ambisonicChannelOrdering;
}

std::uint8_t SpatialAudioBox::getAmbisonicChannelOrdering() const
{
    return mAmbisonicChannelOrdering;
}

void SpatialAudioBox::setAmbisonicNormalization(std::uint8_t ambisonicNormalization)
{
    mAmbisonicNormalization = ambisonicNormalization;
}

std::uint8_t SpatialAudioBox::getAmbisonicNormalization() const
{
    return mAmbisonicNormalization;
}

void SpatialAudioBox::setChannelMap(const Vector<uint32_t>& channelMap)
{
    mChannelMap.clear();
    mChannelMap = channelMap;
}

Vector<uint32_t> SpatialAudioBox::getChannelMap() const
{
    return mChannelMap;
}

void SpatialAudioBox::writeBox(BitStream& bitstr)
{
    writeBoxHeader(bitstr);

    bitstr.write8Bits(mVersion);
    bitstr.write8Bits(mAmbisonicType);
    bitstr.write32Bits(mAmbisonicOrder);
    bitstr.write8Bits(mAmbisonicChannelOrdering);
    bitstr.write8Bits(mAmbisonicNormalization);
    bitstr.write32Bits(static_cast<uint32_t>(mChannelMap.size()));
    for (uint32_t i = 0; i < mChannelMap.size(); i++)
    {
        bitstr.write32Bits(mChannelMap.at(i));
    }

    updateSize(bitstr);
}

void SpatialAudioBox::parseBox(BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    mVersion                  = bitstr.read8Bits();
    mAmbisonicType            = bitstr.read8Bits();
    mAmbisonicOrder           = bitstr.read32Bits();
    mAmbisonicChannelOrdering = bitstr.read8Bits();
    mAmbisonicNormalization   = bitstr.read8Bits();
    uint32_t numberOfChannels = bitstr.read32Bits();
    mChannelMap.clear();
    for (uint32_t i = 0; i < numberOfChannels; i++)
    {
        uint32_t value = bitstr.read32Bits();
        mChannelMap.push_back(value);
    }
}
