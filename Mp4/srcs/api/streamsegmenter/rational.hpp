
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
#ifndef RATIONAL_HPP
#define RATIONAL_HPP

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace StreamSegmenter
{
    template <typename T>
    T gcd(T a, T b)
    {
        T c;
        while (a)
        {
            c = a;
            a = b % a;
            b = c;
        }
        return b;
    }

    template <typename T>
    T lcm(T a, T b)
    {
        return a / gcd(a, b) * b;
    }

    struct InvalidRational
    {
    };

    template <typename T>
    struct Rational
    {
    public:
        typedef T value;

        Rational();
        Rational(T aNum, T aDen);
        Rational(InvalidRational);
        ~Rational();

        Rational<T> minimize() const;
        Rational<T> per1() const;
        Rational<T> floor() const;
        Rational<T> ceil() const;

        template <typename U>
        U cast() const;

        double asDouble() const;
        T asBase() const;

        T num, den;
    };

    typedef Rational<uint64_t> RatU64;
    typedef Rational<int64_t> RatS64;

    template <typename T>
    Rational<T>::Rational()
        : num(0)
        , den(1)
    {
        // nothing
    }

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4146)
#endif

    template <typename T>
    Rational<T>::Rational(T aNum, T aDen)
        : num(aNum)
        , den(aDen)
    {
        if (den < 0)
        {
            den = -den;
            num = -num;
        }
        assert(den != 0);
    }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

    template <typename T>
    template <typename U>
    U Rational<T>::cast() const
    {
        return U(static_cast<typename U::value>(num), static_cast<typename U::value>(den));
    }

    template <typename T>
    Rational<T>::Rational(InvalidRational)
        : num(0)
        , den(0)
    {
        // nothing
    }

    template <typename T>
    Rational<T>::~Rational()
    {
        // nothing
    }

    template <typename T>
    double Rational<T>::asDouble() const
    {
        return double(num) / den;
    }

    template <typename T>
    T Rational<T>::asBase() const
    {
        return num / den;
    }

    template <typename T>
    void shareDenominator(Rational<T>& x, Rational<T>& y)
    {
        if (x.den != y.den)
        {
            T cm   = lcm(x.den, y.den);
            T mulX = cm / x.den;
            T mulY = cm / y.den;

            x.num *= mulX;
            x.den *= mulX;

            y.num *= mulY;
            y.den *= mulY;
            assert(x.den == y.den);
        }
    }

    template <typename Iterator>
    void shareDenominators(Iterator begin, Iterator end)
    {
        if (std::distance(begin, end) >= 2)
        {
            Iterator it = begin;
            auto cm     = (**it).den;
            ++it;
            for (; it != end; ++it)
            {
                cm = lcm(cm, (**it).den);
            }

            for (it = begin; it != end; ++it)
            {
                auto mul = cm / (**it).den;
                (**it).num *= mul;
                (**it).den *= mul;
            }
        }
    }

    template <typename T>
    Rational<T>& operator+=(Rational<T>& self, Rational<T> other)
    {
        shareDenominator(self, other);
        self.num += other.num;
        return self;
    }

    template <typename T>
    Rational<T>& operator-=(Rational<T>& self, Rational<T> other)
    {
        shareDenominator(self, other);
        self.num -= other.num;
        return self;
    }

    template <typename T>
    bool operator==(Rational<T> x, Rational<T> y)
    {
        if (x.den == y.den)
        {
            return x.num == y.num;
        }
        else
        {
            shareDenominator(x, y);
            return x.num == y.num;
        }
    }

    template <typename T>
    bool operator<=(Rational<T> x, Rational<T> y)
    {
        if (x.den == y.den)
        {
            return x.num <= y.num;
        }
        else
        {
            shareDenominator(x, y);
            return x.num <= y.num;
        }
    }

    template <typename T>
    bool operator>=(Rational<T> x, Rational<T> y)
    {
        if (x.den == y.den)
        {
            return x.num >= y.num;
        }
        else
        {
            shareDenominator(x, y);
            return x.num >= y.num;
        }
    }

    template <typename T>
    bool operator!=(Rational<T> x, Rational<T> y)
    {
        if (x.den == y.den)
        {
            return x.num != y.num;
        }
        else
        {
            shareDenominator(x, y);
            return x.num != y.num;
        }
    }

    template <typename T>
    bool operator<(Rational<T> x, Rational<T> y)
    {
        if (x.den == y.den)
        {
            return x.num < y.num;
        }
        else
        {
            shareDenominator(x, y);
            return x.num < y.num;
        }
    }

    template <typename T>
    bool operator>(Rational<T> x, Rational<T> y)
    {
        if (x.den == y.den)
        {
            return x.num > y.num;
        }
        else
        {
            shareDenominator(x, y);
            return x.num > y.num;
        }
    }

    template <typename T>
    Rational<T> operator*(Rational<T> x, Rational<T> y)
    {
        return Rational<T>(x.num * y.num, x.den * y.den).minimize();
    }

    template <typename T>
    Rational<T> operator/(Rational<T> x, Rational<T> y)
    {
        return x * y.per1().minimize();
    }

    template <typename T>
    Rational<T> operator+(Rational<T> x, Rational<T> y)
    {
        shareDenominator(x, y);
        return Rational<T>(x.num + y.num, x.den).minimize();
    }

    template <typename T>
    Rational<T> operator-(Rational<T> x, Rational<T> y)
    {
        shareDenominator(x, y);
        return Rational<T>(x.num - y.num, x.den).minimize();
    }

    template <typename T>
    Rational<T> operator-(Rational<T> x)
    {
        return Rational<T>(-x.num, x.den);
    }

    template <typename T>
    Rational<T> Rational<T>::minimize() const
    {
        Rational r(*this);
        if (r.num == 0)
        {
            return Rational<T>(0, 1);
        }
        else
        {
            T cd = gcd(num, den);
            return Rational<T>(num / cd, den / cd);
        }
    }

    template <typename T>
    Rational<T> Rational<T>::per1() const
    {
        return Rational<T>(den, num);
    }

    template <typename T>
    Rational<T> Rational<T>::floor() const
    {
        if (num >= 0 || num % den == 0)
        {
            return {(num - num % den) / den, 1};
        }
        else
        {
            return {(num - den - num % den) / den, 1};
        }
    }

    template <typename T>
    Rational<T> Rational<T>::ceil() const
    {
        if (num % den == 0 || num < 0)
        {
            return {(den + num - (den + num % den)) / den, 1};
        }
        else if (num >= 0)
        {
            return {(num + (den - num % den)) / den, 1};
        }
    }

    template <typename T>
    std::ostream& operator<<(std::ostream& stream, Rational<T> x)
    {
        return stream << x.num << "/" << x.den;
    }
}

#endif  // RATIONAL_HPP
