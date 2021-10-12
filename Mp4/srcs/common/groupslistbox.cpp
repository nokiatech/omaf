
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
#include "groupslistbox.hpp"
#include "alternateitemsgroupbox.hpp"
#include "overlayandbackgroundgroupingbox.hpp"
#include "overlayswitchalternativesbox.hpp"
#include "viewpointentitygroupbox.hpp"

GroupsListBox::GroupsListBox()
    : Box("grpl")
{
}


GroupsListBox::GroupsListBox(const GroupsListBox& box)
    : Box(box)
{
    BitStream serialize;
    const_cast<GroupsListBox&>(box).writeBox(serialize);
    parseBox(serialize);
}

std::uint32_t GroupsListBox::getGroupCount() const
{
    return mEntityToGoups.size();
}

void GroupsListBox::addGroup(UniquePtr<EntityToGroupBox> group)
{
    mEntityToGoups.push_back(std::move(group));
}

EntityToGroupBox& GroupsListBox::getGroup(std::uint32_t groupIndex) const
{
    return *mEntityToGoups.at(groupIndex);
}

void GroupsListBox::writeBox(BitStream& bitstream)
{
    writeBoxHeader(bitstream);
    for (auto& entityToGroupBox : mEntityToGoups)
    {
        entityToGroupBox->writeBox(bitstream);
    }
    updateSize(bitstream);
}

void GroupsListBox::parseBox(BitStream& bitstream)
{
    uint64_t startPos = bitstream.getPos();
    parseBoxHeader(bitstream);
    uint64_t headerBytes = bitstream.getPos() - startPos;
    uint32_t bytesLeft  = getSize() - headerBytes;

    // read entitytogroups until box if fully read...
    while (bytesLeft)
    {
        FourCCInt boxType;
        BitStream subStream = bitstream.readSubBoxBitStream(boxType);
        bytesLeft -= subStream.getSize();

        if (boxType == "altr")
        {
            UniquePtr<AlternateItemsGroupBox, EntityToGroupBox> entityToGroupBox(
                CUSTOM_NEW(AlternateItemsGroupBox, ()));
            entityToGroupBox->parseBox(subStream);
            addGroup(std::move(entityToGroupBox));
        }
        else if (boxType == "ovbg")
        {
            UniquePtr<OverlayAndBackgroundGroupingBox, EntityToGroupBox> entityToGroupBox(
                CUSTOM_NEW(OverlayAndBackgroundGroupingBox, ()));
            entityToGroupBox->parseBox(subStream);
            addGroup(std::move(entityToGroupBox));
        }
        else if (boxType == "oval")
        {
            UniquePtr<OverlaySwitchAlternativesBox, EntityToGroupBox> entityToGroupBox(
                CUSTOM_NEW(OverlaySwitchAlternativesBox, ()));
            entityToGroupBox->parseBox(subStream);
            addGroup(std::move(entityToGroupBox));
        }
        else if (boxType == "vipo")
        {
            UniquePtr<ViewpointEntityGroupBox, EntityToGroupBox> entityToGroupBox(
                CUSTOM_NEW(ViewpointEntityGroupBox, ()));
            entityToGroupBox->parseBox(subStream);
            addGroup(std::move(entityToGroupBox));
        }
    }
}
