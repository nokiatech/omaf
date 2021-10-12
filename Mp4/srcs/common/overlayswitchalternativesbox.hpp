
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
#ifndef OVERLAYSWITCHALTERNATIVESBOX_HPP
#define OVERLAYSWITCHALTERNATIVESBOX_HPP

#include "customallocator.hpp"
#include "entitytogroupbox.hpp"

/**
 * Overlay swiching alternatives 'oval' from OMAFv2 specification.
 */
class OverlaySwitchAlternativesBox : public EntityToGroupBox
{
public:
    OverlaySwitchAlternativesBox();
    virtual ~OverlaySwitchAlternativesBox() = default;

    void addRefOverlayId(uint16_t refOvlyId);
    uint16_t getRefOverlayId(uint32_t index) const;

    virtual void writeBox(ISOBMFF::BitStream& bitstream);
    virtual void parseBox(ISOBMFF::BitStream& bitstream);

private:
    // references overlay id for each entity
    Vector<uint16_t> mRefOverlayIds;
};

#endif /* OVERLAYSWITCHALTERNATIVESBOX_HPP */
