
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

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <streamsegmenter/frame.hpp>
#include <streamsegmenter/track.hpp>
#include <string>
#include <vector>
#include <map>
#include "meta.h"
#include "common/exceptions.h"
#include "common/rational.h"
#include "common/optional.h"
#include "omaf/extractor.h"

namespace VDD
{
    struct DataAllocations
    {
        // signed so negative overflow is nicer to check (and look at), though it of course never
        // happens..
        std::atomic<int64_t> count;
        std::atomic<int64_t> size;

        DataAllocations() = default;
    };

    // Retrieve the total count and size of CPUDataVector objects. They are not placed inside the
    // class as static members as gdb 7.11.1 seems to have trouble seeing it that way.
    const DataAllocations& getGlobalDataAllocations();

    typedef std::uint32_t GLuint;

    class Data;

    enum class StorageType
    {
        Empty,       //< Not stored anywhere; an empty ticket
        EndOfStream, //< Not stored anywhere; signals end of stream
        CPU,         //< Mapped in normal CPU-accessible address space
        GPU,         //< Accessed via a texture id
        FS,          //< Accessed via file system
        Fragmented,  //< Built out of multiple pieces of Data
    };

    struct Storage
    {
        Storage()
        {
        }
        virtual ~Storage()
        {
        }
        virtual StorageType getStorageType() const = 0;
        virtual Storage* clone() const = 0;
        virtual Storage* moveToClone() = 0;
        virtual bool equal(const Storage&) const = 0;
        /** This method is const because we are really using constness to track the
         * data, not this tracking information */
        virtual void setDataAllocations(DataAllocations&) const {}
    };

    /** @brief Indicates missing data. Useful for example returning data in one View slot while
     * returning nothing in another, such as may be the case for segmenter returning init segment
     * only once. */
    struct EmptyDataReference : public Storage
    {
        StorageType getStorageType() const
        {
            return StorageType::Empty;
        }
        Storage* clone() const;
        Storage* moveToClone();
        bool equal(const Storage&) const;
    };

    /** @brief Used as an argument to Data constructor to indicate end of data */
    struct EndOfStream : public Storage
    {
        StorageType getStorageType() const
        {
            return StorageType::EndOfStream;
        }
        Storage* clone() const;
        Storage* moveToClone();
        bool equal(const Storage&) const;
    };

    struct CPUDataReference : public Storage
    {
        size_t size[MaxNumPlanes]; // applicapble to compressed data

        size_t numPlanes;  //< number depends on the format of the data

        const void* address[MaxNumPlanes];
        size_t rowStride[MaxNumPlanes]; // applicable to bitmap data

        /** Bit offset to iterate within a pixel; in range [0..7].
         * Used with YUV420 to express offsets in U and V where an
         * advancement to the pixel in the image does not advance
         * to the next address in the actual data.
         *
         * Also usable expressing 10-bit data where a window to the
         * data may begin in the middle of the byte. */
        uint8_t pixelBitOffset[MaxNumPlanes];

        /** Sub-offset to iterate within a pixel; in range [0..7].
         * Used with YUV420 to express offsets in U and V where an
         * advancement to the next line does not advance point in the
         * actual data. */
        uint8_t rowSubOffset[MaxNumPlanes];

        CPUDataReference() = default;
        StorageType getStorageType() const
        {
            return StorageType::CPU;
        }
        Storage* clone() const;
        Storage* moveToClone();
        bool equal(const Storage&) const;

    protected:
        CPUDataReference(const CPUDataReference& other) = default;
    };

    struct PlaneOffset
    {
        size_t xByteOffset;
        std::uint8_t xBitOffset;
        size_t yRowOffset;
        std::uint8_t ySubOffset;
    };

    struct CPUSubDataReference : public CPUDataReference
    {
        // Applicable only for CPUDataReference-kind of Data
        CPUSubDataReference(std::shared_ptr<const CPUDataReference> storage,
                            std::vector<PlaneOffset> offsets);

        Storage* clone() const override;
        Storage* moveToClone() override;

        std::shared_ptr<const CPUDataReference> parentStorage;
        void setDataAllocations(DataAllocations& aDataAllocations) const override;

    protected:
        CPUSubDataReference(const CPUSubDataReference&);
        CPUSubDataReference(CPUSubDataReference&&);
    };

    struct CPUDataVector : public CPUDataReference
    {
        std::vector<std::vector<std::uint8_t>> holder;

        /** @brief Construct CPUDataVector from any number of pieces
         * of data with zero strides and offsets. */
        CPUDataVector(std::vector<std::vector<std::uint8_t>>&& data);

        /** @brief Construct CPUDataVector from any number of pieces
         * of data. */
        CPUDataVector(std::vector<size_t> rowStrides,
                      std::vector<uint8_t> rowSubOffsets,
                      std::vector<uint8_t> pixelBitOffsets,
                      std::vector<std::vector<std::uint8_t>>&& data);

        /** @brief Construct CPUDataVector from one piece of data;
         * a convenience constructor in addition to the general one. */
        CPUDataVector(std::vector<size_t> rowStrides,
                      std::vector<uint8_t> rowSubOffsets,
                      std::vector<uint8_t> pixelBitOffsets,
                      std::vector<std::uint8_t>&& data);

        /** @brief Construct CPUDataVector from two pieces of data;
         * a convenience constructor in addition to the general one. */
        CPUDataVector(std::vector<size_t> rowStrides,
                      std::vector<uint8_t> rowSubOffsets,
                      std::vector<uint8_t> pixelBitOffsets,
                      std::vector<std::uint8_t>&& data1,
                      std::vector<std::uint8_t>&& data2);

        /** @brief Construct CPUDataVector from three pieces of data;
         * a convenience constructor in addition to the general one. */
        CPUDataVector(std::vector<size_t> rowStrides,
                      std::vector<uint8_t> rowSubOffsets,
                      std::vector<uint8_t> pixelBitOffsets,
                      std::vector<std::uint8_t>&& data1,
                      std::vector<std::uint8_t>&& data2,
                      std::vector<std::uint8_t>&& data3);

        CPUDataVector(CPUDataVector&& aOther);

        ~CPUDataVector();
        Storage* clone() const override;
        Storage* moveToClone() override;

        void setDataAllocations(DataAllocations& aDataAllocations) const override;

    protected:
        CPUDataVector(const CPUDataVector& aOther);

    private:
        mutable std::reference_wrapper<DataAllocations> dataAllocations;
    };

    struct GPUDataReference : public Storage
    {
        int gpuIndex;        //< which GPU is this texture in?
        size_t numTextures;  //< number depends on the format of the data
        GLuint textureId[MaxNumPlanes];
        StorageType getStorageType() const
        {
            return StorageType::GPU;
        }
        Storage* clone() const;
        Storage* moveToClone();
        bool equal(const Storage&) const;
    };

    struct FSDataReference : public Storage
    {
        std::string path;
        off_t offset;
        size_t size;
        StorageType getStorageType() const
        {
            return StorageType::FS;
        }
        Storage* clone() const;
        Storage* moveToClone();
        bool equal(const Storage&) const;
    };

    struct FragmentedDataReference : public Storage
    {
        std::vector<Storage*> data;
        FragmentedDataReference(std::vector<Storage*>&&);
        void operator=(const FragmentedDataReference) = delete;
        ~FragmentedDataReference();
        StorageType getStorageType() const
        {
            return StorageType::Fragmented;
        }
        Storage* clone() const;
        Storage* moveToClone();
        bool equal(const Storage&) const;
        void setDataAllocations(DataAllocations& aDataAllocations) const;

    protected:
        FragmentedDataReference(const FragmentedDataReference&);
        FragmentedDataReference(FragmentedDataReference&&);
    };

    class InvalidFormatNameException : public Exception
    {
    public:
        InvalidFormatNameException(std::string);
    };

    class OutOfBoundsException : public Exception
    {
    public:
        OutOfBoundsException();
    };

    class WrongArgumentsException : public Exception
    {
    public:
         WrongArgumentsException();
    };

    class Exception;

    class MismatchingStorageTypeException;

#ifdef FUTURE
    /** @brief Processor-specific way to indicate what to do with the frame */

    class Instructions
    {
        Instructions();
        virtual ~Instructions();
    };

    class NoInstructions : public Instructions
    {
    };

    class Ticket
    {
    public:
        /** @brief Node-specific processing instructions for this packet */
        Instructions& getInstructions() const;
    };
#endif

    extern const Meta defaultMeta;

    /** @brief Data is a reference-counted cheap-to-copy way to keep around
        reference to a piece of storage, in the CPU, the GPU or the file
        system */
    class Data
    {
    public:
        /** @brief creates an empty data */
        Data();
        /** @brief creates an end-of-data Data */
        Data(const EndOfStream&, const Meta& aMeta = defaultMeta);
        Data(const EndOfStream&, const StreamId aStreamId, const Meta& aMeta = defaultMeta);
        Data(const Storage&, const Meta&, const StreamId aStreamId = StreamIdUninitialized);
        Data(Storage&& storage, const Meta&, const StreamId aStreamId = StreamIdUninitialized);

        /** special variants of Data constructor that use the storage of given data, but replace with
         * custom metadata. This doesn't directly make use of the shared pointer for storage (nor
         * does the interface provide it) to ensure that the mutex is properly used
         */
        Data(const Data& aData, const Meta& aMeta, const StreamId aStreamId = StreamIdUninitialized);

        Data(const Storage& aStorage, const Meta& aMeta, const Extractors& aExtractor, const StreamId aStreamId);
        Data(const Meta& aMeta, const Extractors& aExtractor, const StreamId aStreamId);

        Data(const Data&);
        Data(Data&&) = default;
        virtual ~Data();

        Data& operator=(const Data&);
        Data& operator=(Data&&) = default;

        /** @brief Return true if this Data signals end of stream */
        bool isEndOfStream() const;

        /** @brief Which host this information is located at? Typically
            only the controller needs to care about this, processors only
            receive data that is already local and in their desired
            storage.
        */
        std::string getHost() const;

        /** @brief Is this piece of data stored in CPU-accessible memory,
            the GPU or the Filesystem? */
        StorageType getStorageType() const;

        /** @brief Set the data allocations object for the data referenced.  The data allocations
         * related to this Data will now be tracked with this object.
         *
         * The method is 'const' because it affects the storage referenced by Data, not itself. */
        void setDataAllocations(DataAllocations& aDataAllocations) const;

        /** @brief If the data is stored in CPU memory, retrieve its address */
        const CPUDataReference& getCPUDataReference() const;

        /** @brief If the data is stored in GPU memory, retrieve its texture
            information */
        const GPUDataReference& getGPUDataReference() const;

        /** @brief If the data is stored in a file system (SSD, HDD), retrieve
            the file path, offset and size */
        const FSDataReference& getFSDataReference() const;

        /** @brief If the data is stored in multiple parts, retrieve the
         * parts. This is used for passing segments out from Segmenter for
         * uploading. */
        const FragmentedDataReference& getFragmentedDataReference() const;

        ContentType getContentType() const;

        /** @brief Get access to the master meta containing other meta */
        const Meta& getMeta() const;

        /** @brief Get metadata common for both raw and coded frames,
            such as its time stamps and video dimensions */
        CommonFrameMeta getCommonFrameMeta() const;

        /** @brief Get metadata of the frame, such as its time stamps,
            pixel format and index */
        RawFrameMeta getFrameMeta() const;

        /** @brief Get metadata of the frame, such as its time stamps,
            pixel format and index */
        CodedFrameMeta getCodedFrameMeta() const;

        /** @brief Construct a subdata. Currently works only if the original
            data is stored in CPU.
         */
        Data subData(size_t aLeft, size_t aTop, size_t aWidth, size_t aHeight) const;

        /** @brief Construct a subdata. Currently works only if the original
            data is stored in CPU. It should only be called if the subdata wraps around.
            the data is copied, it is not a reference like subdata.
        */
        Data subDataWrapAround(size_t aLeft, size_t aTop, size_t aWidth, size_t aHeight) const;

        Extractors& getExtractors();
        const Extractors& getExtractors() const;

        StreamId getStreamId() const;
        StreamId setStreamId(StreamId aStreamId);

        /* Return a version of Data with this tag attached */
        template <typename T> Data withTag(const T& aTag) const;

    private:
        std::shared_ptr<const Storage> mStorage; // protected by sDataStorageMutex
        Meta mMeta;
        friend bool operator==(const Data& aA, const Data& aB);

        Extractors mExtractors;

        StreamId mStreamId;

        // Shared by all instances of Data to protect the shared pointers to mStorage, that could be
        // shared by any instance of Data
        static std::mutex sDataStorageMutex;
    };

    /** @brief Are these two pieces of metadata bitwise equal? */
    bool operator==(const RawFrameMeta& aA, const RawFrameMeta& aB);

    bool operator==(const CodedFrameMeta& aA, const CodedFrameMeta& aB);

    bool operator==(const Meta& aA, const Meta& aB);

    /** @brief Are these two pieces of storage bitwise equal? */
    bool operator==(const Storage& aA, const Storage& aB);

    /** @brief Are these two pieces of data bitwise equal? */
    bool operator==(const Data& aA, const Data& aB);

    /** @brief Move data to given storage, if it's not already
        there. Used by the controller. */
    Data copyToStorage(StorageType storage, Data data);
}

#include "data.icpp"
