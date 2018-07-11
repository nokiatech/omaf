
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "googleequirectangularprojectionbox.hpp"

EquirectangularProjection::EquirectangularProjection()
    : ProjectionDataBox("equi", 0, 0)
    , mProjectionBounds{}
{
}

void EquirectangularProjection::setProjectionBounds(ProjectionBounds& bounds)
{
    mProjectionBounds = bounds;
}

const EquirectangularProjection::ProjectionBounds& EquirectangularProjection::getProjectionBounds() const
{
    return mProjectionBounds;
}

void EquirectangularProjection::writeBox(BitStream& bitstr)
{
    ProjectionDataBox::writeBox(bitstr);
    bitstr.write32Bits(mProjectionBounds.top_0_32_FP);
    bitstr.write32Bits(mProjectionBounds.bottom_0_32_FP);
    bitstr.write32Bits(mProjectionBounds.left_0_32_FP);
    bitstr.write32Bits(mProjectionBounds.right_0_32_FP);
    updateSize(bitstr);
}

void EquirectangularProjection::parseBox(BitStream& bitstr)
{
    ProjectionDataBox::parseBox(bitstr);
    mProjectionBounds.top_0_32_FP    = bitstr.read32Bits();
    mProjectionBounds.bottom_0_32_FP = bitstr.read32Bits();
    mProjectionBounds.left_0_32_FP   = bitstr.read32Bits();
    mProjectionBounds.right_0_32_FP  = bitstr.read32Bits();
}
