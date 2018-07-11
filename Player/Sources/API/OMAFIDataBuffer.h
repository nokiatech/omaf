
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

#include "Platform/OMAFDataTypes.h"

namespace OMAF
{
    template <typename T>
    class IDataBuffer
    {
    public:
        virtual ~IDataBuffer() {};

        /*
        * Returns the pointer to access the data
        */
        virtual T* getDataPtr() = 0;

        virtual const T* getDataPtr() const = 0;

        /*
        * Return the amount of data written in this data buffer
        */
        virtual size_t getSize() const = 0;
        /*
        * Resize the underlying buffer and keep existing data.
        */
        virtual void_t reserve(size_t capacity) = 0;
        /*
        * Return the size reserved for this data buffer.
        */
        virtual size_t getCapacity() const = 0;
        /*
        * Set the amount of data written in this data buffer
        */
        virtual void_t setSize(size_t capacity) = 0;
    };

    typedef IDataBuffer<uint8_t> IUint8DataBuffer;
}