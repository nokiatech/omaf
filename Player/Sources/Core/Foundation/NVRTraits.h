
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
#pragma once

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

// https://gcc.gnu.org/onlinedocs/gcc/Type-Traits.html
// https://msdn.microsoft.com/en-us/library/ms177194.aspx
// http://libcxx.llvm.org/type_traits_design.html

namespace Traits
{
    // Determines if given data type is a POD-type during compile-time.
    // With POD-type information it's possible to implement low-level optimization.
    // E.g. construct / destruct calls can be skipped for POD-types in memory and container classes.
    
    // Note: Template specialization can be used to declare user defined non-POD data types as POD-type.
    //       This kind of optimizations can improve performance significantly, but should be handled with care.
    template <typename T>
    struct isPOD
    {
        static const bool Value = __is_pod(T);
    };
}

OMAF_NS_END
