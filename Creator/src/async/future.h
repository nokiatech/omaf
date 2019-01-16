
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

#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "async/asyncnode.h"
#include "processor/data.h"
#include "common/exceptions.h"
#include "common/optional.h"

namespace VDD
{
    class PromiseAlreadySet : public Exception
    {
    public:
        PromiseAlreadySet();
    };

    template <typename T>
    using FutureCallback = std::function<void(const T&)>;

    template <typename T> class Promise;

    // 0 is a special "not set" value; may be returned by "then" if the callback is invoked
    // immediately. Calling removeCallback with such a value is fine.
    typedef size_t FutureCallbackKey;

    template <typename T>
    struct PromiseState
    {
        std::mutex mutex;

        std::map<FutureCallbackKey, FutureCallback<T>> callbacks;

        size_t callbackKeyCounter;

        Optional<T> value;
    };

    template <typename T> using SharedPromiseState = std::shared_ptr<PromiseState<T>>;

    /** A value that will be accessible some time in the future (or maybe now); use "then" to get
     * get the value */
    template <typename T>
    class Future
    {
    public:
        typedef T type;

        Future();

        // shared copy
        Future(const Future& aOther);

        ~Future();

        /** Call this function once the value has been set; may be called immediately if the future
         * already has a value. We use constness to track only the mutability of the actual value,
         * so this function is const as it only manipulates the list of callbacks.
         *
         * Called so to match the C++ Techical Report's futures, that at some point probably end up
         * in C++ */
        FutureCallbackKey then(const FutureCallback<T>&) const;

        /** Remove a callback with a key; or the key is zero, does remove anything. */
        void removeCallback(FutureCallbackKey key) const;

        /** Returns *this. Useful for converting Promises to Future in certain contexts. */
        const Future<T>& getFuture() const;

    protected:
        friend class Promise<T>;

        Future(SharedPromiseState<T> aState);

        /** Constness is handled in the interface, not in the internals. */
        PromiseState<T>& state() const;

    private:
        SharedPromiseState<T> mState;
    };

    /** Same as future, but this part can actually be set. Use SharedPromise locally, give
     * Future externally */
    template <typename T>
    class Promise : public Future<T>
    {
    public:
        /** No value */
        Promise();

        /** Immediately has a value */
        Promise(const T&);

        /** Set a value to a future that doesn't have its value already set. */
        void set(const T&);
    };
}

#include "future.icpp"
