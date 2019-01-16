
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
#include "NVRMediaType.h"

OMAF_NS_BEGIN

    void MediaContent::addType(Type type)
    {
        mTypeBitmask |= static_cast<uint64_t>(type);
    }

    void MediaContent::removeType(Type type)
    {
        mTypeBitmask &= ~static_cast<uint64_t>(type);
    }

    void MediaContent::clear()
    {
        mTypeBitmask = 0;
    }

    bool_t MediaContent::matches(Type type) const
    {
        return (mTypeBitmask & static_cast<uint64_t>(type)) != 0;
    }

    bool_t MediaContent::isSpecified() const
    {
        return (mTypeBitmask != static_cast<uint64_t>(Type::UNSPECIFIED));
    }
OMAF_NS_END
