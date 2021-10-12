
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
#ifndef ISOBMFF_OPTIONAL_HPP
#define ISOBMFF_OPTIONAL_HPP

#include <memory>
#include <type_traits>

namespace ISOBMFF
{
    class NoneType
    {
    };

    // instantiated in commontypes.cpp
    extern NoneType none;

    template <typename T>
    class Optional
    {
    public:
        Optional();
        Optional(NoneType);
        Optional(const Optional<T>&);

        Optional(const T&);

        Optional& operator=(const Optional<T>&);

        ~Optional();

        explicit operator bool() const;

        T& operator*();
        const T& operator*() const;
        T& get();
        const T& get() const;
        T* operator->();
        const T* operator->() const;
        T* ptr();
        const T* ptr() const;
        void clear();

        // If the value is set, return makeOptional(aMap(**this)); if not, return None
        template <typename MapFunc> auto map(const MapFunc& aMap) const -> Optional<decltype(aMap(**this))>;

        // If the value is set, return the return type given by the fiven function; if not, return the
        // default-instantiated value of the same type
        template <typename MapFunc> auto then_do(const MapFunc& aMap) const -> decltype(aMap(**this));

        // If the value is set, return the return type given by the fiven function; if not, return the
        // default-instantiated value of the same type
        template <typename MakeFunc> T else_do(const MakeFunc& aMakeValue) const;

        /** @brief If it is set, return **this. If no value is, return aDefault.
         *
         * Named such per C++17 */
        T value_or(const T& aDefault) const;
    private:
        typename std::aligned_storage<sizeof(T), alignof(T)>::type mStorage;
        bool mIsSet = false;
    };

    template <typename T>
    Optional<T> makeOptional(const T& value);

    template <typename First, typename... Rest>
    First coalesce(First value, Rest... rest);

    template <typename T>
    bool operator==(const Optional<T>& self, const Optional<T>& other);
    template <typename T>
    bool operator!=(const Optional<T>& self, const Optional<T>& other);
    template <typename T>
    bool operator<=(const Optional<T>& self, const Optional<T>& other);
    template <typename T>
    bool operator>=(const Optional<T>& self, const Optional<T>& other);
    template <typename T>
    bool operator<(const Optional<T>& self, const Optional<T>& other);
    template <typename T>
    bool operator>(const Optional<T>& self, const Optional<T>& other);
}  // namespace ISOBMFF

#include "optional.icpp"

#endif
