
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
#ifndef STREAMSEGMENTER_ID_HHP
#define STREAMSEGMENTER_ID_HHP

#include <iostream>

namespace StreamSegmenter
{
    template <typename T, typename Tag>
    class IdBase
    {
    public:
        IdBase()
            : mId()
        {
            // nothing
        }
        // implicit in, but explicit out
        IdBase(T id)
            : mId(id)
        {
            // nothing
        }

        T get() const
        {
            return mId;
        }

    protected:
        T mId;
    };

    // IdBaseWithAdditions is like IdBase but provides + - += -= as well
    template <typename T, typename Tag>
    class IdBaseWithAdditions : public IdBase<T, Tag>
    {
    public:
        IdBaseWithAdditions()
            : IdBase<T, Tag>()
        {
            // nothing
        }
        // implicit in, but explicit out
        IdBaseWithAdditions(T id)
            : IdBase<T, Tag>(id)
        {
            // nothing
        }

        IdBaseWithAdditions<T, Tag>& operator++();
        ;
        IdBaseWithAdditions<T, Tag>& operator--();
    };

    template <typename T, typename Tag>
    bool operator<(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() < b.get();
    }

    template <typename T, typename Tag>
    bool operator==(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() == b.get();
    }

    template <typename T, typename Tag>
    bool operator!=(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() != b.get();
    }

    template <typename T, typename Tag>
    bool operator>(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() > b.get();
    }

    template <typename T, typename Tag>
    bool operator<=(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() <= b.get();
    }

    template <typename T, typename Tag>
    bool operator>=(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() >= b.get();
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag> operator+(IdBaseWithAdditions<T, Tag> a, IdBaseWithAdditions<T, Tag> b)
    {
        return a.get() + b.get();
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag>& IdBaseWithAdditions<T, Tag>::operator++()
    {
        ++IdBase<T, Tag>::mId;
        return *this;
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag>& IdBaseWithAdditions<T, Tag>::operator--()
    {
        --IdBase<T, Tag>::mId;
        return *this;
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag> operator++(IdBaseWithAdditions<T, Tag>& a, int)
    {
        auto orig = a;
        ++a;
        return orig;
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag> operator--(IdBaseWithAdditions<T, Tag>& a, int)
    {
        auto orig = a;
        --a;
        return orig;
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag> operator-(IdBaseWithAdditions<T, Tag> a, IdBaseWithAdditions<T, Tag> b)
    {
        return a.get() - b.get();
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag>& operator+=(IdBaseWithAdditions<T, Tag>& a, IdBaseWithAdditions<T, Tag> b)
    {
        a = a.get() + b.get();
        return a;
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag>& operator-=(IdBaseWithAdditions<T, Tag>& a, IdBaseWithAdditions<T, Tag> b)
    {
        a = a.get() - b.get();
        return a;
    }

    template <typename T, typename Tag>
    std::ostream& operator<<(std::ostream& stream, IdBase<T, Tag> value)
    {
        stream << value.get();
        return stream;
    }
}


#endif  // STREAMSEGMENTER_ID_HHP
