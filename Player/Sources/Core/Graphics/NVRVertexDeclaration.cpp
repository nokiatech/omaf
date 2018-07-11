
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
#include "Graphics/NVRVertexDeclaration.h"
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRStringUtilities.h"

OMAF_NS_BEGIN

VertexDeclaration::VertexDeclaration()
: mStride(0)
{
}

VertexDeclaration::VertexDeclaration(const VertexDeclaration& other)
: mStride(other.mStride)
, mAttributes(other.mAttributes)
{
    
}

VertexDeclaration::~VertexDeclaration()
{
    mStride = 0;
    mAttributes.clear();
}

VertexDeclaration& VertexDeclaration::operator = (const VertexDeclaration& other)
{
    mStride = other.mStride;
    mAttributes.clear();
    mAttributes.add(other.mAttributes);
    
    return *this;
}

VertexDeclaration& VertexDeclaration::begin()
{
    return *this;
}

VertexDeclaration& VertexDeclaration::addAttribute(const char_t* name,
                                                   VertexAttributeFormat::Enum format,
                                                   uint32_t count,
                                                   bool_t normalized)
{
    OMAF_ASSERT_NOT_NULL(name);
    OMAF_ASSERT(StringByteLength(name) > 0, "");
    
    // Create attribute
    HashFunction<const char_t*> hashFunction;
    
    VertexAttribute attribute;
    attribute.name = hashFunction(name);
    attribute.format = format;
    attribute.offset = getStride();
    attribute.count = count;
    attribute.normalized = normalized;
    
    mAttributes.add(attribute);
    
    // Update stride
    uint32_t stride = 0;
    
    for(size_t i = 0; i < mAttributes.getSize(); ++i)
    {
        VertexAttribute& attribute = mAttributes[i];
        uint8_t bytes = VertexAttributeFormat::getSize(attribute.format);
        bytes *= attribute.count;
        
        stride += bytes;
    }
    
    mStride = stride;
    
    return *this;
}

VertexDeclaration& VertexDeclaration::end()
{
    return *this;
}

uint16_t VertexDeclaration::getOffset(const char_t* name) const
{
    HashFunction<const char_t*> hashFunction;
    HashValue nameHash = hashFunction(name);
    
    for (size_t i = 0; i < mAttributes.getSize(); ++i)
    {
        const VertexAttribute& attribute = mAttributes[i];
        
        if (attribute.name == nameHash)
        {
            return attribute.offset;
        }
    }
    
    return 0;
}

uint32_t VertexDeclaration::getAttributeCount() const
{
    return (uint32_t)mAttributes.getSize();
}

uint32_t VertexDeclaration::getStride() const
{
    return mStride;
}

const VertexDeclaration::VertexAttributeList& VertexDeclaration::getAttributes() const
{
    return mAttributes;
}

OMAF_NS_END
