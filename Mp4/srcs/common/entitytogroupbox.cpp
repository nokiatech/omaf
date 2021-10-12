
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
#include "entitytogroupbox.hpp"

EntityToGroupBox::EntityToGroupBox(FourCCInt boxType, std::uint8_t version, std::uint32_t flags)
    : FullBox(boxType, version, flags)
{
}

std::uint32_t EntityToGroupBox::getGroupId() const
{
    return mGroupId;
}

void EntityToGroupBox::setGroupId(std::uint32_t id)
{
    mGroupId = id;
}

std::uint32_t EntityToGroupBox::getEntityCount() const
{
    return mEntityIds.size();
}

void EntityToGroupBox::addEntityId(std::uint32_t entityId)
{
    mEntityIds.push_back(entityId);
}

std::uint32_t EntityToGroupBox::getEntityId(std::uint32_t index) const
{
    return mEntityIds.at(index);
}

void EntityToGroupBox::writeBox(BitStream& bitstream)
{
    writeFullBoxHeader(bitstream);
    bitstream.write32Bits(mGroupId);
    bitstream.write32Bits(getEntityCount());
    for (auto& eId : mEntityIds)
    {
        bitstream.write32Bits(eId);
    }
    updateSize(bitstream);
}

void EntityToGroupBox::parseBox(BitStream& bitstream)
{
    parseFullBoxHeader(bitstream);
    mGroupId = bitstream.read32Bits();

    const unsigned int entityIdCount = bitstream.read32Bits();
    for (unsigned int i = 0; i < entityIdCount; ++i)
    {
        mEntityIds.push_back(bitstream.read32Bits());
    }
}
