
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
#ifndef LAZY_H
#define LAZY_H

#include "optional.h"

namespace VDD
{
    // not thread safe; does not use locking
    template <typename T>
    struct Lazy {
        Lazy(const T& aInit);  // initialization with concrete value
        Lazy(T&& aInit);
        ~Lazy() = default;

        Lazy(const Lazy<T>& aOther) = default;
        Lazy(Lazy<T>&& aOther) = default;

        Lazy<T>& operator=(const Lazy<T>& aOther) = default;
        Lazy<T>& operator=(Lazy<T>&& aOther) = default;

        Lazy(std::function<T()> aMake);

        T& operator*();
        const T& operator*() const;

        T* operator->();
        const T* operator->() const;

        T* ptr();
        const T* ptr() const;
    private:
        mutable Optional<std::function<T()>> mMake;
        mutable Optional<T> mValue;

        void resolve() const;
    };
}

#include "lazy.icpp"

#endif // LAZY_H