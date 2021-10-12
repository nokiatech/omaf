
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
#ifndef GROUPSLISTBOX_HPP
#define GROUPSLISTBOX_HPP

#include "api/isobmff/commontypes.h"

#include "bbox.hpp"
#include "customallocator.hpp"
#include "entitytogroupbox.hpp"

/** GroupsListBox class
 * @details box implementation as specified in the ISOBMFF specification. */
class GroupsListBox : public Box
{
public:
    GroupsListBox();
    GroupsListBox(const GroupsListBox& box);  // uniqueptr copying is not trivial
    virtual ~GroupsListBox() = default;

    std::uint32_t getGroupCount() const;
    void addGroup(UniquePtr<EntityToGroupBox> group);
    EntityToGroupBox& getGroup(std::uint32_t groupIndex) const;

    /** Write box to ISOBMFF::BitStream.
     *  @see Box::writeBox() */
    virtual void writeBox(ISOBMFF::BitStream& bitstream);

    /** Read box from ISOBMFF::BitStream.
     *  @see Box::parseBox() */
    virtual void parseBox(ISOBMFF::BitStream& bitstream);

private:
    Vector<UniquePtr<EntityToGroupBox>> mEntityToGoups;  ///< Vector of sample entries
};

template class MP4VR_DLL_PUBLIC ISOBMFF::Optional<GroupsListBox>;

#endif /* GROUPSLISTBOX_HPP */
