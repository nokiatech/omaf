
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

#include "NVREssentials.h"

#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRHashFunctions.h"
#include "Graphics/NVRConfig.h"
#include "Graphics/NVRVertexAttributeFormat.h"

OMAF_NS_BEGIN

struct VertexAttribute
{
    HashValue name;
    VertexAttributeFormat::Enum format;
    uint32_t offset;
    uint32_t count;
    bool_t normalized;
};

class VertexDeclaration
{
public:
    typedef FixedArray<VertexAttribute, OMAF_MAX_SHADER_ATTRIBUTES> VertexAttributeList;

public:
    VertexDeclaration();
    VertexDeclaration(const VertexDeclaration& other);

    ~VertexDeclaration();

    VertexDeclaration& operator=(const VertexDeclaration& other);

    VertexDeclaration& begin();
    VertexDeclaration&
    addAttribute(const char_t* name, VertexAttributeFormat::Enum format, uint32_t count, bool_t normalized);
    VertexDeclaration& end();

    uint16_t getOffset(const char_t* name) const;
    uint32_t getAttributeCount() const;
    uint32_t getStride() const;

    const VertexAttributeList& getAttributes() const;

private:
    uint16_t mStride;

    VertexAttributeList mAttributes;
};

OMAF_NS_END
