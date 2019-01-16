
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
#include "googlesphericalvideoheaderbox.hpp"
#include "buildinfo.hpp"

SphericalVideoHeader::SphericalVideoHeader()
    : FullBox("svhd", 0, 0)
    , mMetadataSource("Nokia MP4VR Tool " + String(MP4VR_BUILD_VERSION) + "\0")
{
}

void SphericalVideoHeader::setMetadataSource(const String& source)
{
    mMetadataSource = source;
}

const String& SphericalVideoHeader::getMetadataSource() const
{
    return mMetadataSource;
}


void SphericalVideoHeader::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    bitstr.writeZeroTerminatedString(mMetadataSource);
    updateSize(bitstr);
}

void SphericalVideoHeader::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    bitstr.readZeroTerminatedString(mMetadataSource);
}
