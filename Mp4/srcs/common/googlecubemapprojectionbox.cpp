
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
#include "googlecubemapprojectionbox.hpp"

CubemapProjection::CubemapProjection()
    : ProjectionDataBox("cbmp", 0, 0)
    , mLayout(0)
    , mPadding(0)
{
}

void CubemapProjection::setLayout(uint32_t layout)
{
    mLayout = layout;
}

uint32_t CubemapProjection::getLayout() const
{
    return mLayout;
}

void CubemapProjection::setPadding(uint32_t padding)
{
    mPadding = padding;
}

uint32_t CubemapProjection::getPadding() const
{
    return mPadding;
}

void CubemapProjection::writeBox(BitStream& bitstr)
{
    ProjectionDataBox::writeBox(bitstr);
    bitstr.write32Bits(mLayout);
    bitstr.write32Bits(mPadding);
    updateSize(bitstr);
}

void CubemapProjection::parseBox(BitStream& bitstr)
{
    ProjectionDataBox::parseBox(bitstr);
    mLayout  = bitstr.read32Bits();
    mPadding = bitstr.read32Bits();
}
