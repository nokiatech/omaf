
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
#ifndef ROTATIONBOX_HPP
#define ROTATIONBOX_HPP

#include <cstdint>
#include <vector>
#include "api/isobmff/commontypes.h"
#include "fullbox.hpp"

using namespace ISOBMFF;

/** @brief his box provides information on the rotation of this track
 *  @details Defined in the OMAF standard. **/
class RotationBox : public FullBox
{
public:
    RotationBox();
    RotationBox(const RotationBox&);
    virtual ~RotationBox() = default;

    Rotation getRotation() const;
    void setRotation(Rotation rot);

    /** @see Box::writeBox()
     */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @see Box::parseBox()
     */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    Rotation mRotation;
};

#endif
