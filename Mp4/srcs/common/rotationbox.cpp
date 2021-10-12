
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
#include "rotationbox.hpp"
#include "log.hpp"

RotationBox::RotationBox()
    : FullBox("rotn", 0, 0)
    , mRotation({})
{
}

RotationBox::RotationBox(const RotationBox& box)
    : FullBox(box)
    , mRotation(box.mRotation)
{
}

Rotation RotationBox::getRotation() const
{
    return mRotation;
}

void RotationBox::setRotation(Rotation rotation)
{
    mRotation = rotation;
}

void RotationBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);

    bitstr.write32BitsSigned(mRotation.yaw);
    bitstr.write32BitsSigned(mRotation.pitch);
    bitstr.write32BitsSigned(mRotation.roll);

    updateSize(bitstr);
}

void RotationBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);

    mRotation.yaw   = bitstr.read32BitsSigned();
    mRotation.pitch = bitstr.read32BitsSigned();
    mRotation.roll  = bitstr.read32BitsSigned();
}
