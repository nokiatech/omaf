
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
#include "avcconfigurationbox.hpp"
#include "avccommondefs.hpp"
#include "bitstream.hpp"

AvcConfigurationBox::AvcConfigurationBox()
    : Box("avcC")
    , mAvcConfig()
{
}

AvcConfigurationBox::AvcConfigurationBox(const AvcConfigurationBox& box)
    : Box(box.getType())
    , mAvcConfig(box.mAvcConfig)
{
}

const AvcDecoderConfigurationRecord& AvcConfigurationBox::getConfiguration() const
{
    return mAvcConfig;
}

void AvcConfigurationBox::setConfiguration(const AvcDecoderConfigurationRecord& config)
{
    mAvcConfig = config;
}

void AvcConfigurationBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);
    mAvcConfig.writeDecConfigRecord(bitstr);
    updateSize(bitstr);
}

void AvcConfigurationBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);
    mAvcConfig.parseConfig(bitstr);
}
