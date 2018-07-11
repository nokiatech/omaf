
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "future.h"

/**
 * Operations one may perform on Futures. Because these are always template-based and most always
 * used only from implementation code, they are in their own header to minimize the compile time
 * impact.
 */

namespace VDD
{
    /** Given a promise of type T and a function to go from T to U, return a promise of type U */
    template <typename T, typename Map>
    auto mapFuture(const Future<T>& aFuture, Map aMap) -> Future<decltype(aMap(T()))>;

    // A namespace for containing the template meta programming classes/functions for Future
    namespace FutureTMP
    {
        template <typename Tuple, size_t N> struct ConvertTuple;
    }

    /** Given a variadic list of promises of type T1, T2, .., return a single promise of type
     * std::tuple<T1, T2, ..>
     *
     * Example:
     * Future<int> a;
     * Future<std::string> b;
     * Future<std::tuple<int, std::string>> ab = whenAll(a, b);
     * auto xs = waitFuture(ab);
     * std::cout << std::get<0>(xs) << " " << std::get<1>(xs) << std::endl;
     * // or maybe:
     * int x;
     * std::string y;
     * std::tie(x, y) = waitFuture(ab);
     *
     * now "ab" is activated once both a and b have been.
     *
     * whenAll is so named as the C++ Technical Report has function when_all for the same purpose.
     */
    template <class ... Types>
    auto whenAll(Types ... args)
        -> Future<typename FutureTMP::ConvertTuple<std::tuple<Types...>, std::tuple_size<std::tuple<Types...>>::value>::return_type>;

    /** @brief Same as whenAll, but instead of inputting variable number of arguments and outputting
     * a tuple with the appropriate types, require that all inputs have the same type, and produce
     * a future that is a vector of the future results. */
    template <typename Container>
    Future<std::vector<typename Container::value_type::type>> whenAllOfContainer(const Container&);

    /** Given a promise, wait it synchronously. Mostly useful for tests. */
    template <typename T>
    T waitFuture(const Future<T>& aFuture);
}

#include "futureops.icpp"
