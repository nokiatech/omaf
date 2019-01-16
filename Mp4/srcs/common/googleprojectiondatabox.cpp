
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
#include "googleprojectiondatabox.hpp"

ProjectionDataBox::ProjectionDataBox(FourCCInt proj_type, uint8_t version, uint32_t flags)
    : FullBox(proj_type, version, flags)
{
}

void ProjectionDataBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    updateSize(bitstr);
}

void ProjectionDataBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
}
