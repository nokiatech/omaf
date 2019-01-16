
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
#ifndef RATIONAL_H
#define RATIONAL_H

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <streamsegmenter/rational.hpp>

namespace VDD
{
    template <typename T>
    using Rational = StreamSegmenter::Rational<T>;

    template <typename Type>
    Rational<Type> rationalOfDouble(double aValue)
    {
        Type m[2][2];
        double x, startx;
        const Type maxden =
            static_cast<Type>(std::min(static_cast<uint64_t>(std::numeric_limits<Type>::max() / 2),
                                       static_cast<uint64_t>(1) << 49));
        Type ai;

        startx = x = aValue;

        /* initialize matrix */
        m[0][0] = m[1][1] = 1;
        m[0][1] = m[1][0] = 0;

        /* loop finding terms until denom gets too big */
        while (m[1][0] *  ( ai = (Type)x ) + m[1][1] <= maxden) {
            Type t;
            t = m[0][0] * ai + m[0][1];
            m[0][1] = m[0][0];
            m[0][0] = t;
            t = m[1][0] * ai + m[1][1];
            m[1][1] = m[1][0];
            m[1][0] = t;
            if(x==(double)ai) break;     // AF: division by zero
            x = 1/(x - (double) ai);
            if(x>(double)maxden / 2) break;  // AF: representation failure
        }

        /* now remaining x is between 0 and 1/ai */
        /* approx as either 0 or 1/m where m is max that will fit in maxden */
        /* first try zero */
        // printf("%ld/%ld, error = %e\n", m[0][0], m[1][0],
        //        startx - ((double) m[0][0] / (double) m[1][0]));

        // /* now try other possibility */
        // ai = (maxden - m[1][1]) / m[1][0];
        // m[0][0] = m[0][0] * ai + m[0][1];
        // m[1][0] = m[1][0] * ai + m[1][1];

        return { m[0][0], m[1][0] };
    }

    template <typename T>
    std::istream& operator>>(std::istream& aStream, Rational<T>& aRational)
    {
        aStream >> aRational.num;
        if (aStream.peek() == '/')
        {
            aStream.get();
            aStream >> aRational.den;
        }
        else
        {
            aStream.setstate(std::ios::failbit);
        }
        return aStream;
    }
}

#endif  // RATIONAL_H
