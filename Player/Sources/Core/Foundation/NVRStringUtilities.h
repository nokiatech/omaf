
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
#pragma once

#include "Foundation/NVRCompatibility.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
static const size_t Npos = (size_t) -1;
static const utf8_t Eos = (utf8_t)('\0');

size_t StringLength(const utf8_t* string);

size_t StringByteLength(const utf8_t* string);
size_t StringByteLength(const utf8_t* string, size_t index);

size_t StringFormatVar(char_t* buffer, size_t bufferSize, const char_t* format, va_list args);

size_t StringFormatLengthVar(const char_t* format, va_list args);

size_t StringFormatLength(const char_t* format, ...);

size_t CharacterPosition(const utf8_t* string, size_t index);
size_t CharacterPositionPrevious(const utf8_t* string, const utf8_t* beginning);

size_t StringFindFirst(const utf8_t* source, const utf8_t* string, size_t startIndex = 0);
size_t StringFindLast(const utf8_t* source, const utf8_t* string, size_t startIndex = Npos);

ComparisonResult::Enum StringCompare(const utf8_t* l, const utf8_t* r);
ComparisonResult::Enum StringCompare(const utf8_t* l, const utf8_t* r, size_t length);
OMAF_NS_END
