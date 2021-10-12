
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
#ifndef OVERLAYANDBACKGROUNDGROUPINGBOX_HPP
#define OVERLAYANDBACKGROUNDGROUPINGBOX_HPP

#include "customallocator.hpp"
#include "entitytogroupbox.hpp"
#include "api/isobmff/optional.h"

/**
 * OverlayAndBackgroundGroupingBox 'ovbg' from OMAFv2 specs.
 */
class OverlayAndBackgroundGroupingBox : public EntityToGroupBox
{
public:
    using OverlaySubset = std::vector<uint32_t>;

    struct GroupingFlags
    {
        GroupingFlags()
            : overlayFlag(false)
            , backgroundFlag(false)
        {
        }

        GroupingFlags(uint8_t aData)
            : GroupingFlags()
        {
            overlayFlag    = aData & 0b00000100;
            overlaySubset  =
                (aData & 0b00000010) ? ISOBMFF::makeOptional(OverlaySubset()) : ISOBMFF::Optional<OverlaySubset>();
            backgroundFlag = aData & 0b00000001;
        }

        uint8_t flagsAsByte() const
        {
            // clang-format off
            return 0
                | (overlayFlag    ? 0b00000100 : 0)
                | (overlaySubset  ? 0b00000010 : 0)
                | (backgroundFlag ? 0b00000001 : 0);
            // clang-format on
        }

        GroupingFlags withOverlaySubset(OverlaySubset aSubset) const
        {
            auto created = *this;
            created.overlaySubset = aSubset;
            return created;
        }

        bool overlayFlag;
        ISOBMFF::Optional<OverlaySubset> overlaySubset; // allow empty subset vs no subset with Optional
        bool backgroundFlag;
    };


    OverlayAndBackgroundGroupingBox();
    virtual ~OverlayAndBackgroundGroupingBox() = default;

    uint32_t getSphereDistanceInMm() const;
    void setSphereDistanceInMm(uint32_t mm);

    void addGroupingFlags(GroupingFlags flags);
    GroupingFlags getGroupingFlags(uint32_t index) const;

    virtual void writeBox(ISOBMFF::BitStream& bitstream);
    virtual void parseBox(ISOBMFF::BitStream& bitstream);

private:
    uint32_t mSphereDistanceInMm;
    // grouping flags for each entity id
    Vector<GroupingFlags> mEntityFlags;
};

#endif /* OVERLAYANDBACKGROUNDGROUPINGBOX_HPP */
