
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "googlestereoscopic3dbox.hpp"

Stereoscopic3D::Stereoscopic3D()
    : FullBox("st3d", 0, 0)
    , mStereoMode(V2StereoMode::MONOSCOPIC)
{
}

void Stereoscopic3D::setStereoMode(const V2StereoMode& stereoMode)
{
    mStereoMode = stereoMode;
}

Stereoscopic3D::V2StereoMode Stereoscopic3D::getStereoMode() const
{
    return mStereoMode;
}

void Stereoscopic3D::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    bitstr.write8Bits(static_cast<uint8_t>(mStereoMode));
    updateSize(bitstr);
}

void Stereoscopic3D::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    mStereoMode = static_cast<V2StereoMode>(bitstr.read8Bits());
}
