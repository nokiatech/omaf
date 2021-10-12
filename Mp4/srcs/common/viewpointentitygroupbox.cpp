
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
#include "viewpointentitygroupbox.hpp"

ViewpointEntityGroupBox::ViewpointEntityGroupBox()
    : EntityToGroupBox("vipo", 0, 0)
{
}

uint32_t ViewpointEntityGroupBox::getViewpointId() const
{
    return mViewpointId;
}

void ViewpointEntityGroupBox::setViewpointId(uint32_t anId)
{
    mViewpointId = anId;
}

String ViewpointEntityGroupBox::getLabel() const
{
    return mLabel;
}

void ViewpointEntityGroupBox::setLabel(const String& label)
{
    mLabel = label;
}

const ISOBMFF::ViewpointPosStruct& ViewpointEntityGroupBox::getViewpointPos() const
{
    return mViewpointPos;
}

void ViewpointEntityGroupBox::setViewpointPos(const ISOBMFF::ViewpointPosStruct& pos)
{
    mViewpointPos = pos;
}

const ISOBMFF::ViewpointGroupStruct<true>& ViewpointEntityGroupBox::getViewpointGroup() const
{
    return mViewpointGroup;
}

void ViewpointEntityGroupBox::setViewpointGroup(const ISOBMFF::ViewpointGroupStruct<true>& vpGroup)
{
    mViewpointGroup = vpGroup;
}

const ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct&
ViewpointEntityGroupBox::getViewpointGlobalCoordinateSysRotation() const
{
    return mViewpointGlobalCoordinateSysRotation;
}

void ViewpointEntityGroupBox::setViewpointGlobalCoordinateSysRotation(
    const ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct& globCoordRot)
{
    mViewpointGlobalCoordinateSysRotation = globCoordRot;
}

ISOBMFF::Optional<ISOBMFF::ViewpointGpsPositionStruct> ViewpointEntityGroupBox::getViewpointGpsPosition() const
{
    return mViewpointGpsPosition;
}

void ViewpointEntityGroupBox::setViewpointGpsPosition(
    const ISOBMFF::Optional<ISOBMFF::ViewpointGpsPositionStruct>& gpsPos)
{
    mViewpointGpsPosition = gpsPos;
}

ISOBMFF::Optional<ISOBMFF::ViewpointGeomagneticInfoStruct> ViewpointEntityGroupBox::getViewpointGeomagneticInfo() const
{
    return mViewpointGeomagneticInfo;
}

void ViewpointEntityGroupBox::setViewpointGeomagneticInfo(
    const ISOBMFF::Optional<ISOBMFF::ViewpointGeomagneticInfoStruct>& geomagInfo)
{
    mViewpointGeomagneticInfo = geomagInfo;
}

ISOBMFF::Optional<ISOBMFF::ViewpointSwitchingListStruct> ViewpointEntityGroupBox::getViewpointSwitchingList() const
{
    return mViewpointSwitchingList;
}

void ViewpointEntityGroupBox::setViewpointSwitchingList(
    const ISOBMFF::Optional<ISOBMFF::ViewpointSwitchingListStruct>& switchList)
{
    mViewpointSwitchingList = switchList;
}

ISOBMFF::Optional<ISOBMFF::ViewpointLoopingStruct> ViewpointEntityGroupBox::getViewpointLooping() const
{
    return mViewpointLooping;
}

void ViewpointEntityGroupBox::setViewpointLooping(const ISOBMFF::Optional<ISOBMFF::ViewpointLoopingStruct>& looping)
{
    mViewpointLooping = looping;
}

void ViewpointEntityGroupBox::writeBox(ISOBMFF::BitStream& bitstream)
{
    EntityToGroupBox::writeBox(bitstream);

    bitstream.write32Bits(mViewpointId);
    bitstream.writeZeroTerminatedString(mLabel);
    bitstream.writeBit(!!mViewpointGpsPosition);
    bitstream.writeBit(!!mViewpointGeomagneticInfo);
    bitstream.writeBit(!!mViewpointSwitchingList);
    bitstream.writeBit(!!mViewpointLooping);
    bitstream.writeBits(0, 4);  // reserved

    mViewpointPos.write(bitstream);
    mViewpointGroup.write(bitstream);
    mViewpointGlobalCoordinateSysRotation.write(bitstream);

    if (!!mViewpointGpsPosition)
    {
        mViewpointGpsPosition->write(bitstream);
    }

    if (!!mViewpointGeomagneticInfo)
    {
        mViewpointGeomagneticInfo->write(bitstream);
    }

    if (!!mViewpointSwitchingList)
    {
        mViewpointSwitchingList->write(bitstream);
    }

    if (!!mViewpointLooping)
    {
        mViewpointLooping->write(bitstream);
    }

    updateSize(bitstream);
}

void ViewpointEntityGroupBox::parseBox(ISOBMFF::BitStream& bitstream)
{
    EntityToGroupBox::parseBox(bitstream);
    mViewpointId = bitstream.read32Bits();
	bitstream.readZeroTerminatedString(mLabel);

    bool hasViewpointGpsPosition     = bitstream.readBit();
    bool hasViewpointGeomagneticInfo = bitstream.readBit();
    bool hasViewpointSwitchingList   = bitstream.readBit();
    bool hasViewpointLooping         = bitstream.readBit();
    bitstream.readBits(4);  // reserved bits

    mViewpointPos.parse(bitstream);
    mViewpointGroup.parse(bitstream);
    mViewpointGlobalCoordinateSysRotation.parse(bitstream);

    if (hasViewpointGpsPosition)
    {
        mViewpointGpsPosition = ISOBMFF::ViewpointGpsPositionStruct();
        mViewpointGpsPosition->parse(bitstream);
    }

    if (hasViewpointGeomagneticInfo)
    {
        mViewpointGeomagneticInfo = ISOBMFF::ViewpointGeomagneticInfoStruct();
        mViewpointGeomagneticInfo->parse(bitstream);
    }

    if (hasViewpointSwitchingList)
    {
        mViewpointSwitchingList = ISOBMFF::ViewpointSwitchingListStruct();
        mViewpointSwitchingList->parse(bitstream);
    }

    if (hasViewpointLooping)
    {
        mViewpointLooping = ISOBMFF::ViewpointLoopingStruct();
        mViewpointLooping->parse(bitstream);
    }
}
