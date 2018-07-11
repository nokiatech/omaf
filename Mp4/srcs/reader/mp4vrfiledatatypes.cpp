
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
#define _SCL_SECURE_NO_WARNINGS

#include <algorithm>

#include "api/reader/mp4vrfiledatatypes.h"
#include "bitstream.hpp"
#include "customallocator.hpp"


namespace MP4VR
{
    InitialViewingOrientationSample::InitialViewingOrientationSample(char* frameData, uint32_t frameLen)
    {
        Vector<uint8_t> vec;
        vec.assign(frameData, frameData + frameLen);

        BitStream bitstr;
        bitstr.write8BitsArray(vec);

        region.centreAzimuth   = bitstr.read32Bits();
        region.centreElevation = bitstr.read32Bits();
        region.centreTilt      = bitstr.read32Bits();
        region.interpolate     = bitstr.read8Bits();
        refreshFlag            = bitstr.read8Bits() & 0x1;
    }

    template <typename T>
    DynArray<T>::~DynArray()
    {
        CUSTOM_DELETE_ARRAY(elements, T);
    }

    template <typename T>
    DynArray<T>::DynArray()
        : size(0)
        , elements(nullptr)
    {
    }

    template <typename T>
    DynArray<T>::DynArray(size_t n)
        : size(n)
        , elements(CUSTOM_NEW_ARRAY(T, n))
    {
    }

    template <typename T>
    DynArray<T>::DynArray(const DynArray& other)
        : size(other.size)
        , elements(CUSTOM_NEW_ARRAY(T, other.size))
    {
        std::copy(other.elements, other.elements + other.size, elements);
    }

    template <typename T>
    DynArray<T>& DynArray<T>::operator=(const DynArray<T>& other)
    {
        if (this != &other)
        {
            CUSTOM_DELETE_ARRAY(elements, T);
            size     = other.size;
            elements = CUSTOM_NEW_ARRAY(T, size);
            std::copy(other.elements, other.elements + other.size, elements);
        }
        return *this;
    }

    template struct DynArray<uint8_t>;
    template struct DynArray<char>;
    template struct DynArray<uint16_t>;
    template struct DynArray<uint32_t>;
    template struct DynArray<uint64_t>;
    template struct DynArray<DecoderSpecificInfo>;
    template struct DynArray<ChannelLayout>;
    template struct DynArray<FourCC>;
    template struct DynArray<TimestampIDPair>;
    template struct DynArray<TypeToTrackIDs>;
    template struct DynArray<SampleInformation>;
    template struct DynArray<TrackInformation>;
    template struct DynArray<SegmentInformation>;
    template struct DynArray<RegionWisePackingRegion>;
    template struct DynArray<CoverageSphereRegion>;
    template struct DynArray<SchemeType>;

}  // namespace MP4VR
