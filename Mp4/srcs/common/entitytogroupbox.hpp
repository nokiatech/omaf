
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
#ifndef ENTITYTOGROUPBOX_HPP
#define ENTITYTOGROUPBOX_HPP

#include "customallocator.hpp"
#include "fullbox.hpp"

/** Entity To Group Box class
 * @details Entity group box implementation as specified in the ISOBMFF specification. */
class EntityToGroupBox : public FullBox
{
public:
    EntityToGroupBox(FourCCInt boxType, std::uint8_t version, std::uint32_t flags = 0);
    virtual ~EntityToGroupBox() = default;

    std::uint32_t getGroupId() const;
    void setGroupId(std::uint32_t id);

    /** @return Number of contained entity ids */
    std::uint32_t getEntityCount() const;

    void addEntityId(std::uint32_t entityId);
    std::uint32_t getEntityId(std::uint32_t index) const;

    /** Write box to ISOBMFF::BitStream.
     *  @see Box::writeBox() */
    virtual void writeBox(ISOBMFF::BitStream& bitstream);

    /** Read box from ISOBMFF::BitStream.
     *  @see Box::parseBox() */
    virtual void parseBox(ISOBMFF::BitStream& bitstream);

private:
    std::uint32_t mGroupId;
    Vector<std::uint32_t> mEntityIds;
};

#endif /* ENTITYTOGROUPBOX_HPP */
