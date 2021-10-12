
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
#ifndef STREAMSEGMENTER_ID_HHP
#define STREAMSEGMENTER_ID_HHP

#include <iostream>

namespace StreamSegmenter
{
    template <typename T, typename Tag>
    class ExplicitIdBase
    {
    public:
        constexpr ExplicitIdBase()
            : mId()
        {
            // nothing
        }
        // explicit in, but explicit out
        constexpr explicit ExplicitIdBase(T id)
            : mId(id)
        {
            // nothing
        }

        T get() const
        {
            return mId;
        }

        // These are too useful for generation purposes, leave them in
        ExplicitIdBase<T, Tag>& operator++();
        ExplicitIdBase<T, Tag> operator++(int);
    protected:
        T mId;
    };

    template <typename T, typename Tag>
    class IdBase: public ExplicitIdBase<T, Tag>
    {
    public:
        constexpr IdBase()
            : ExplicitIdBase<T, Tag>()
        {
            // nothing
        }
        // implicit in, but explicit out
        constexpr IdBase(T id)
            : ExplicitIdBase<T, Tag>(id)
        {
            // nothing
        }

        // These are too useful for generation purposes, leave them in
        IdBase<T, Tag>& operator++();
        IdBase<T, Tag> operator++(int);
    };

    // IdBaseWithAdditions is like IdBase but provides + - += -= as well
    template <typename T, typename Tag>
    class IdBaseWithAdditions : public IdBase<T, Tag>
    {
    public:
        constexpr IdBaseWithAdditions()
            : IdBase<T, Tag>()
        {
            // nothing
        }
        // implicit in, but explicit out
        constexpr IdBaseWithAdditions(T id)
            : IdBase<T, Tag>(id)
        {
            // nothing
        }

        IdBaseWithAdditions<T, Tag> operator++(int);
        IdBaseWithAdditions<T, Tag>& operator++();
        IdBaseWithAdditions<T, Tag> operator--(int);
        IdBaseWithAdditions<T, Tag>& operator--();
    };

    template <typename T, typename Tag>
    ExplicitIdBase<T, Tag>& ExplicitIdBase<T, Tag>::operator++()
    {
        ++ExplicitIdBase<T, Tag>::mId;
        return *this;
    }

    template <typename T, typename Tag>
    IdBase<T, Tag>& IdBase<T, Tag>::operator++()
    {
        ++ExplicitIdBase<T, Tag>::mId;
        return *this;
    }

    template <typename T, typename Tag>
    ExplicitIdBase<T, Tag> ExplicitIdBase<T, Tag>::operator++(int)
    {
        return ExplicitIdBase<T, Tag>(ExplicitIdBase<T, Tag>::mId++);
    }

    template <typename T, typename Tag>
    IdBase<T, Tag> IdBase<T, Tag>::operator++(int)
    {
        return IdBase<T, Tag>(ExplicitIdBase<T, Tag>::mId++);
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag> IdBaseWithAdditions<T, Tag>::operator++(int)
    {
        return IdBaseWithAdditions<T, Tag>(ExplicitIdBase<T, Tag>::mId++);
    }

    template <typename T, typename Tag>
    bool operator<(ExplicitIdBase<T, Tag> a, ExplicitIdBase<T, Tag> b)
    {
        return a.get() < b.get();
    }

    template <typename T, typename Tag>
    bool operator==(ExplicitIdBase<T, Tag> a, ExplicitIdBase<T, Tag> b)
    {
        return a.get() == b.get();
    }

    template <typename T, typename Tag>
    bool operator!=(ExplicitIdBase<T, Tag> a, ExplicitIdBase<T, Tag> b)
    {
        return a.get() != b.get();
    }

    template <typename T, typename Tag>
    bool operator>(ExplicitIdBase<T, Tag> a, ExplicitIdBase<T, Tag> b)
    {
        return a.get() > b.get();
    }

    template <typename T, typename Tag>
    bool operator<=(ExplicitIdBase<T, Tag> a, ExplicitIdBase<T, Tag> b)
    {
        return a.get() <= b.get();
    }

    template <typename T, typename Tag>
    bool operator>=(ExplicitIdBase<T, Tag> a, ExplicitIdBase<T, Tag> b)
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
        ++ExplicitIdBase<T, Tag>::mId;
        return *this;
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag>& IdBaseWithAdditions<T, Tag>::operator--()
    {
        --ExplicitIdBase<T, Tag>::mId;
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
    std::ostream& operator<<(std::ostream& stream, ExplicitIdBase<T, Tag> value)
    {
        stream << value.get();
        return stream;
    }
}


#endif  // STREAMSEGMENTER_ID_HHP
