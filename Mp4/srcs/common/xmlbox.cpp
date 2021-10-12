
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
#include "xmlbox.hpp"

XmlBox::XmlBox()
    : FullBox("uri ", 0, 0)
    , mContents()
{
}

void XmlBox::setContents(const String& uri)
{
    mContents = uri;
}

const String& XmlBox::getContents() const
{
    return mContents;
}

void XmlBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    bitstr.writeZeroTerminatedString(mContents);
    updateSize(bitstr);
}

void XmlBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    bitstr.readZeroTerminatedString(mContents);
}
