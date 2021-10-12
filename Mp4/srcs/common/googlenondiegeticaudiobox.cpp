
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
#include "googlenondiegeticaudiobox.hpp"

NonDiegeticAudioBox::NonDiegeticAudioBox()
    : Box("SAND")
    , mVersion(0)
{
}

void NonDiegeticAudioBox::setVersion(std::uint8_t version)
{
    mVersion = version;
}

std::uint8_t NonDiegeticAudioBox::getVersion() const
{
    return mVersion;
}

void NonDiegeticAudioBox::writeBox(BitStream& bitstr)
{
    writeBoxHeader(bitstr);

    bitstr.write8Bits(mVersion);

    updateSize(bitstr);
}

void NonDiegeticAudioBox::parseBox(BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    mVersion = bitstr.read8Bits();
}
