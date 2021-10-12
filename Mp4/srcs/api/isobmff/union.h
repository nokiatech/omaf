
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
\
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * self software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#ifndef ISOBMFF_UNION_HPP
#define ISOBMFF_UNION_HPP

#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include "optional.h"
#include "unionhelpers.h"

namespace ISOBMFF {
    using namespace UnionHelpers;

    /** @brief Comparison is a separate class so we can instantiate Union with types that don't do comaprison */
    template <typename Key, typename... Types>
    struct UnionCompare;

    /** Useful for adding empty elements to a Union */
    struct Empty {};

    /** @brief Union is a type-safe alternative to union; similar to std::variant (C++17) but more appropriate for
     *  passing over dll boundaries, as its structure is defined here.
     *
     *  Union has a separate Key for accessing fields instead of using the field type. This allows distinguishing
     *  between multiple values that have the same type but different meaning. It is advisable to use enum for the Key
     *  parameter.
     *
     *  Union does no dynamic memory management. Instead, the value is stored inside the Union object itself. Thus its
     *  size is similar to the largest object mentioned in Types.
     *
     *  Example:
     *
     *  enum class Key {
     *    Name,
     *    Id
     *  };
     *
     *  using UserInfo = Union<Key, std::string, int>;
     *
     *  void dump(const UserInfo& info)
     *  {
     *    switch (info.getKey()) {
     *      case Key::Name:
     *        std::cout << "Name: " << info.at<Key::Name>();
     *        break;
     *      case Key::Id:
     *        std::cout << "Id: " << info.at<Key::Id>();
     *        break;
     *    }
     *  }
     *
     *  int main()
     *  {
     *    UserInfo x; // default-initialized to empty string
     *    x.set<Key::Id>(42);
     *    assert(x.getKey() == Key::Id);
     *    assert(x.at<Key::Id>() == 42);
     *    assert(x.atPtr<Key::Name>() == nullptr);
     *  }
     *
     *  It is a fatal error (assert) to access a field other than what was most recently put in.
     */
    template <typename Key, typename... Types>
    class Union
    {
    private:
        friend struct UnionCompare<Key, Types...>;

        using Self = Union<Key, Types...>;
        using Storage = UnionStorage<Types...>;

        template <Key IndexKey>
        struct Field : UnionField<Storage, size_t(IndexKey), Types...>
        {
        };

        using FieldDynamic = UnionFieldDynamic<Storage, Types...>;

        template <typename T>
        using GetBase = UnionGetBase<Storage, T, Types...>;

    public:
        /** @brief Default-constructs Union with its first Key */
        Union();

        /** @brief Copy-constructs Union */
        Union(const Self& aOther);

        /** @brief Move-constructs Union */
        Union(Self&& aOther);

        /** @brief Destruct Union and its internal object */
        ~Union();

        /** @brief Copy another Union */
        Self& operator=(const Self& aOther);

        /** @brief Move another Union */
        Self& operator=(Self&& aOther);

        /** @brief Access the field given by the template IndexKey parameter to at.
         *
         * Example: x.at<Foo>() */
        template <Key IndexKey>
        typename Field<IndexKey>::type& at()
        {
            assert(IndexKey == mCurrentKey);
            return Field<IndexKey>::get(mStorage);
        }

        /** @brief Access the field given by the template IndexKey parameter to at.
         *
         * Example: x.at<Foo>() */
        template <Key IndexKey>
        const typename Field<IndexKey>::type& at() const
        {
            assert(IndexKey == mCurrentKey);
            return Field<IndexKey>::get(mStorage);
        }

        /** @brief Retrieve the value of a field as a pointer holding; or null if not set to that type
         *
         * Example: if (auto y = x.atPtr<Foo>()) { work with *y; } */
        template <Key IndexKey>
        typename Field<IndexKey>::type* atPtr()
        {
            return IndexKey == mCurrentKey ? &Field<IndexKey>::get(mStorage) : nullptr;
        }

        /** @brief Retrieve the value of a field as a pointer holding; or null if not set to that type
         *
         * Example: if (auto y = x.atPtr<Foo>()) { work with *y; } */
        template <Key IndexKey>
        const typename Field<IndexKey>::type* atPtr() const
        {
            return IndexKey == mCurrentKey ? &Field<IndexKey>::get(mStorage) : nullptr;
        }

        /** @brief Retrieve the value of a field as an optional value holding a std::reference_wrapper
         *
         * Example: if (auto y = x.getOpt<Foo>()) { work with *y; } */
        template <Key IndexKey>
        Optional<typename Field<IndexKey>::type> getOpt() const
        {
            if (IndexKey == mCurrentKey)
            {
                return {Field<IndexKey>::get(mStorage)};
            }
            else
            {
                return {};
            }
        }

        /** @brief Cast the value to Base, regardless of which value is set
         *
         * Will fail at compile time if the cast is not possible
         *
         * Example: auto& base = x.cast<Base>(); */
        template <typename Base>
        Base& cast();

        /** @brief Cast the value to Base, regardless of which value is set
         *
         * Will fail at compile time if the cast is not possible
         *
         * Example: auto& base = x.cast<Base>(); */
        template <typename Base>
        const Base& cast() const;

        /** @brief Set the field given by the template IndexKey parameter to set.
         *
         * Example: x.set<Foo>(42) */
        template <Key IndexKey>
        typename Field<IndexKey>::type& set(const typename Field<IndexKey>::type& aValue)
        {
            mValid = false;
            destructKey(mCurrentKey);
            mCurrentKey = IndexKey;
            auto value  = new (Field<IndexKey>::storage(mStorage)) typename Field<IndexKey>::type(aValue);
            mValid      = true;
            return *value;
        }

        /** @brief Set the field given by the template IndexKey parameter to emplace, avoiding an intermediate object.
         *
         * Example: x.emplace<Sequence>({42, 44}) */
        template <Key IndexKey, typename... Args>
        typename Field<IndexKey>::type& emplace(Args&&... aArgs)
        {
            mValid = false;
            destructKey(mCurrentKey);
            mCurrentKey = IndexKey;
            auto value =
                new (Field<IndexKey>::storage(mStorage)) typename Field<IndexKey>::type(std::forward<Args>(aArgs)...);
            mValid = true;
            return *value;
        }

        /** @brief Retrieve the most previously set key (defaulting to key with value 0 in default constructor)
         *
         * Example: assert(x.getKey() == Sequence); */
        Key getKey() const;
    private:
        void destructKey(Key aKey);

    protected:
        Storage mStorage; // Aligned storage
        Key mCurrentKey; // Most previously set key

        // Object can become invalid if the destructor of the contained object throws or if the constructor or copy/move
        // operator of a new object to be assigned to this Union throws
        bool mValid;
    };

    template <typename Key, typename Base, typename... Types>
    class UnionCommonBase : public Union<Key, Types...>
    {
    public:
        // Access the common base of the value, regardless which value is currently actually set
        Base& base();
        const Base& base() const;
    };

    template <typename Key, typename... Types>
    bool operator==(const Union<Key, Types...>& self, const Union<Key, Types...>& other);
    template <typename Key, typename... Types>
    bool operator!=(const Union<Key, Types...>& self, const Union<Key, Types...>& other);
    template <typename Key, typename... Types>
    bool operator<=(const Union<Key, Types...>& self, const Union<Key, Types...>& other);
    template <typename Key, typename... Types>
    bool operator>=(const Union<Key, Types...>& self, const Union<Key, Types...>& other);
    template <typename Key, typename... Types>
    bool operator<(const Union<Key, Types...>& self, const Union<Key, Types...>& other);
    template <typename Key, typename... Types>
    bool operator>(const Union<Key, Types...>& self, const Union<Key, Types...>& other);
}

#include "union.icpp"

#endif
