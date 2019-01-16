
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
#ifndef LOCALCOPY_OPTIONAL_HPP
#define LOCALCOPY_OPTIONAL_HPP

#include <memory>

// NOTE: This file will be removed once the symbol conflict issue between streamsegmenter and heif is corrected

namespace LocalCopy
{
        class NoneType
        {
        };

        extern NoneType none;

        template <typename T>
        class Optional
        {
        public:
            Optional();
            Optional(NoneType);
            Optional(const Optional<T>&);
            Optional(Optional<T>&&);

            Optional(const T&);
            Optional(T&&);

            Optional& operator=(const Optional<T>&);
            Optional& operator=(Optional<T>&&);

            ~Optional();

            bool operator==(const Optional<T>& other) const;
            bool operator!=(const Optional<T>& other) const;
            bool operator<=(const Optional<T>& other) const;
            bool operator>=(const Optional<T>& other) const;
            bool operator<(const Optional<T>& other) const;
            bool operator>(const Optional<T>& other) const;

            explicit operator bool() const;

            T& operator*();
            const T& operator*() const;
            T& get();
            const T& get() const;
            T* operator->();
            const T* operator->() const;

        private:
            std::unique_ptr<T> mValue;
        };

        template <typename T>
        Optional<T> makeOptional(const T& value);

        template <typename T>
        Optional<T> makeOptional(T&& value);

        template <typename First, typename... Rest>
        First coalesce(First value, Rest... rest);
} // namespace LocalCopy

#include "localoptional.icpp"

#endif
