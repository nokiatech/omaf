
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
#include "hevcconfigurationbox.hpp"
#include "bitstream.hpp"

HevcConfigurationBox::HevcConfigurationBox()
    : Box("hvcC")
    , mHevcConfig()
{
}

HevcConfigurationBox::HevcConfigurationBox(const HevcConfigurationBox& box)
    : Box(box.getType())
    , mHevcConfig(box.mHevcConfig)
{
}

const HevcDecoderConfigurationRecord& HevcConfigurationBox::getConfiguration() const
{
    return mHevcConfig;
}

void HevcConfigurationBox::setConfiguration(const HevcDecoderConfigurationRecord& config)
{
    mHevcConfig = config;
}

void HevcConfigurationBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);
    mHevcConfig.writeDecConfigRecord(bitstr);
    updateSize(bitstr);
}

void HevcConfigurationBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);
    mHevcConfig.parseConfig(bitstr);
}
