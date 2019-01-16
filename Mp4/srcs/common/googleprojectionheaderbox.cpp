
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
#include "googleprojectionheaderbox.hpp"

ProjectionHeader::ProjectionHeader()
    : FullBox("prhd", 0, 0)
    , mPose{}
{
}

void ProjectionHeader::setPose(const PoseInDegrees_16_16_FP& pose)
{
    mPose = pose;
}

const ProjectionHeader::PoseInDegrees_16_16_FP& ProjectionHeader::getPose() const
{
    return mPose;
}


void ProjectionHeader::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    bitstr.write32Bits(static_cast<uint32_t>(mPose.yaw));
    bitstr.write32Bits(static_cast<uint32_t>(mPose.pitch));
    bitstr.write32Bits(static_cast<uint32_t>(mPose.roll));
    updateSize(bitstr);
}

void ProjectionHeader::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    mPose.yaw   = static_cast<int32_t>(bitstr.read32Bits());
    mPose.pitch = static_cast<int32_t>(bitstr.read32Bits());
    mPose.roll  = static_cast<int32_t>(bitstr.read32Bits());
}
