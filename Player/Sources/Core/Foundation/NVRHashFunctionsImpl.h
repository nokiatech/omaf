
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
OMAF_NS_BEGIN
template <typename T>
HashValue HashFunction<T>::operator()(const T& key) const
{
    return (HashValue) key;
}

template <typename T>
HashValue HashFunction<T*>::operator()(const T* key) const
{
    return (HashValue) key;
}

template <typename T>
HashValue HashFunction<const T*>::operator()(const T* key) const
{
    return (HashValue) key;
}

HashValue HashFunction<float32_t>::operator()(float32_t key) const
{
    union CastUnion {
        float32_t key;
        uint32_t hash;
    };

    const CastUnion cast = {key};

    return (HashValue) cast.hash;
}

HashValue HashFunction<float64_t>::operator()(float64_t key) const
{
    union CastUnion {
        float64_t key;
        uint64_t hash;
    };

    const CastUnion cast = {key};

    return (HashValue) cast.hash;
}
OMAF_NS_END
