
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
#include "uribox.hpp"

UriBox::UriBox()
    : FullBox("uri ", 0, 0)
    , mUri()
{
}

void UriBox::setUri(const String& uri)
{
    mUri = uri;
}

const String& UriBox::getUri() const
{
    return mUri;
}

void UriBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    bitstr.writeZeroTerminatedString(mUri);
    updateSize(bitstr);
}

void UriBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    bitstr.readZeroTerminatedString(mUri);
}
