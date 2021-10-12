
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
#ifndef STREAMSEGMENTER_OPTIONAL_HPP
#define STREAMSEGMENTER_OPTIONAL_HPP

#include <memory>
#include "../isobmff/optional.h"

namespace StreamSegmenter
{
    namespace Utils
    {
        using ISOBMFF::Optional;
        using ISOBMFF::NoneType;

        using ISOBMFF::coalesce;
        using ISOBMFF::makeOptional;
        using ISOBMFF::none;
    }
}  // namespace StreamSegmenter::Utils

#endif
