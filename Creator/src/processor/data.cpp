
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
#include <algorithm>
#include <cstring>

#include "data.h"

namespace VDD
{
    static DataAllocations sDataAllocations;

    const DataAllocations& getGlobalDataAllocations()
    {
        return sDataAllocations;
    }

    std::mutex Data::sDataStorageMutex;

    const Meta defaultMeta {};

    InvalidFormatNameException::InvalidFormatNameException(std::string aName)
        : Exception("InvalidFormatNameException: " + aName)
    {
        // nothing
    }

    OutOfBoundsException::OutOfBoundsException()
        : Exception("OutOfBoundsException")
    {
        // nothing
    }

    WrongArgumentsException::WrongArgumentsException()
        : Exception("WrongArgumentException")
    {
        // nothing
    }

    namespace
    {
        std::vector<std::string> rawFormatNames{
            "None",
            "YUV420",
            "RGB888",
            "PCM",
            "CPUSubDataReferenceTestFormat1",
            "CPUSubDataReferenceTestFormat2",
            "RawTestFormat1",
            "RawTestFormat2",
            "RawTestFormat3" };

        // Indexed by CodedFormat
        std::vector<std::string> codedFormatNames{
            "None",
            "H264",
            "H265",
            "AAC" };

        RawFormatDescription formatDescriptions[] = {
            // None
            RawFormatDescription {},
            // YUV420
            RawFormatDescription {
                3,              // numPlanes
                12,             // bitsPerPixel
                { 8, 4, 4 },    // pixelStrideBits
                { 8, 4, 4 }     // rowStrideOf8
            },
            // RGB888,
            RawFormatDescription {
                1,              // numPlanes
                24,             // bitsPerPixel
                { 24 },         // pixelStrideBits
                { 8 }           // rowStrideOf8
            },
            // PCM
            RawFormatDescription {},
            // CPUSubDataReferenceTestFormat1
            RawFormatDescription {
                2,              // numPlanes
                8 + 16,         // bitsPerPixel
                { 8, 16 },      // pixelStrideBits
                { 8, 8 }        // rowStrideOf8
            },
            // CPUSubDataReferenceTestFormat2
            RawFormatDescription {
                2,              // numPlanes
                8 + 4,          // bitsPerPixel
                { 8, 4 },       // pixelStrideBits
                { 8, 8 }        // rowStrideOf8
            },
            // RawTestFormat1
            RawFormatDescription {
                1,              // numPlanes
                8,              // bitsPerPixel
                { 8 },          // pixelStrideBits
                { 8 }           // rowStrideOf8
            },
            // RawTestFormat2
            RawFormatDescription {
                2,              // numPlanes
                24,             // bitsPerPixel
                { 8, 16 },      // pixelStrideBits
                { 8, 8 }        // rowStrideOf8
            },
            // RawTestFormat3
            RawFormatDescription {
                2,              // numPlanes
                16,             // bitsPerPixel
                { 8, 8 },       // pixelStrideBits
                { 8, 8 }        // rowStrideOf8
            }
        };

    }  // anonymous namespace

    const RawFormatDescription getRawFormatDescription(RawFormat aFormat)
    {
        return formatDescriptions[int(aFormat)];
    }

    std::ostream& operator<<(std::ostream& aStream, RawFormat aFormat)
    {
        aStream << rawFormatNames[size_t(aFormat)];
        return aStream;
    }

    std::istream& operator>>(std::istream& aStream, RawFormat& aFormat)
    {
        std::string str;
        aStream >> str;
        for (size_t c = 0; c < rawFormatNames.size(); ++c)
        {
            if (str == rawFormatNames[c])
            {
                aFormat = RawFormat(c);
                return aStream;
            }
        }
        throw InvalidFormatNameException(str);
    }

    std::ostream& operator<<(std::ostream& aStream, CodedFormat aFormat)
    {
        aStream << codedFormatNames[size_t(aFormat)];
        return aStream;
    }

    std::istream& operator >> (std::istream& aStream, CodedFormat& aFormat)
    {
        std::string str;
        aStream >> str;
        for (size_t c = 0; c < codedFormatNames.size(); ++c)
        {
            if (str == codedFormatNames[c])
            {
                aFormat = CodedFormat(c);
                return aStream;
            }
        }
        throw InvalidFormatNameException(str);
    }

    Storage* EmptyDataReference::clone() const
    {
        return new EmptyDataReference;
    }

    Storage* EmptyDataReference::moveToClone()
    {
        return new EmptyDataReference;
    }

    bool EmptyDataReference::equal(const Storage& aStorage) const
    {
        return dynamic_cast<const EmptyDataReference*>(&aStorage) != nullptr;
    }

    Storage* EndOfStream::clone() const
    {
        return new EndOfStream;
    }

    Storage* EndOfStream::moveToClone()
    {
        return new EndOfStream;
    }

    bool EndOfStream::equal(const Storage& aStorage) const
    {
        return dynamic_cast<const EndOfStream*>(&aStorage) != nullptr;
    }

    Storage* CPUDataReference::clone() const
    {
        return new CPUDataReference(*this);
    }

    Storage* CPUDataReference::moveToClone()
    {
        return new CPUDataReference(std::move(*this));
    }

    bool CPUDataReference::equal(const Storage& aStorage) const
    {
        if (auto other = dynamic_cast<const CPUDataReference*>(&aStorage))
        {
            if (numPlanes == other->numPlanes)
            {
                for (size_t c = 0; c < numPlanes; ++c)
                {
                    if (size[c] != other->size[c] ||
                        std::memcmp(address[c], other->address[c], size[c]) != 0)
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    void CPUDataVector::setDataAllocations(DataAllocations& aDataAllocations) const
    {
        if (numPlanes)
        {
            --dataAllocations.get().count;
        }

        for (size_t c = 0; c < numPlanes; ++c)
        {
            dataAllocations.get().size -= int64_t(size[c]);
        }

        dataAllocations = aDataAllocations;

        if (numPlanes)
        {
            ++dataAllocations.get().count;
        }

        for (size_t c = 0; c < numPlanes; ++c)
        {
            dataAllocations.get().size += int64_t(size[c]);
        }
    }

    CPUDataVector::CPUDataVector(const CPUDataVector& aOther)
        : CPUDataReference()
        , holder(aOther.holder)
        , dataAllocations(aOther.dataAllocations)
    {
        ++dataAllocations.get().count;
        numPlanes = holder.size();
        for (size_t c = 0; c < numPlanes; ++c)
        {
            address[c] = &holder[c][0] + (static_cast<const unsigned char*>(aOther.address[c]) -
                                          static_cast<const unsigned char*>(&aOther.holder[c][0]));
            assert(address[c] >= &holder[c][0]);
            assert(address[c] < &holder[c][0] + holder[c].size());
            size[c] = aOther.size[c];
            rowStride[c] = aOther.rowStride[c];
            pixelBitOffset[c] = aOther.pixelBitOffset[c];
            rowSubOffset[c] = aOther.rowSubOffset[c];
            dataAllocations.get().size += int64_t(size[c]);
        }
    }

    CPUDataVector::CPUDataVector(CPUDataVector&& aOther)
        : CPUDataReference()
        , holder(std::move(aOther.holder))
        , dataAllocations(sDataAllocations)
    {
        numPlanes = holder.size();
        for (size_t c = 0; c < numPlanes; ++c)
        {
            address[c] = aOther.address[c];
            assert(address[c] >= &holder[c][0]);
            assert(address[c] < &holder[c][0] + holder[c].size());
            size[c] = aOther.size[c];
            aOther.size[c] = 0;
            rowStride[c] = aOther.rowStride[c];
            pixelBitOffset[c] = aOther.pixelBitOffset[c];
            rowSubOffset[c] = aOther.rowSubOffset[c];
        }
        aOther.numPlanes = 0;
    }

    CPUDataVector::CPUDataVector(std::vector<std::vector<std::uint8_t>>&& aData)
        : holder(std::move(aData))
        , dataAllocations(sDataAllocations)
    {
        ++dataAllocations.get().count;
        numPlanes = holder.size();
        for (size_t c = 0; c < numPlanes; ++c)
        {
            address[c] = &holder[c][0];
            size[c] = holder[c].size();
            rowStride[c] = 0;
            pixelBitOffset[c] = 0;
            rowSubOffset[c] = 0;
            dataAllocations.get().size += int64_t(size[c]);
        }
    }

    CPUDataVector::CPUDataVector(std::vector<size_t> aRowStrides,
                                 std::vector<uint8_t> aRowSubOffsets,
                                 std::vector<uint8_t> aPixelBitOffsets,
                                 std::vector<std::vector<std::uint8_t>>&& aData)
        : holder(std::move(aData))
        , dataAllocations(sDataAllocations)
    {
        ++dataAllocations.get().count;
        numPlanes = holder.size();
        for (size_t c = 0; c < numPlanes; ++c)
        {
            address[c] = &holder[c][0];
            size[c] = holder[c].size();
            rowStride[c] = c < aRowStrides.size() ? aRowStrides[c] : 0;
            rowSubOffset[c] = c < aRowSubOffsets.size() ? aRowSubOffsets[c] : 0;
            pixelBitOffset[c] = c < aPixelBitOffsets.size() ? aPixelBitOffsets[c] : 0;
            dataAllocations.get().size += int64_t(size[c]);
        }
    }

    CPUDataVector::CPUDataVector(std::vector<size_t> aRowStrides,
                                 std::vector<uint8_t> aRowSubOffsets,
                                 std::vector<uint8_t> aPixelBitOffsets,
                                 std::vector<std::uint8_t>&& aData)
        : CPUDataVector(aRowStrides, aRowSubOffsets, aPixelBitOffsets,
                        std::vector<std::vector<std::uint8_t>> { std::move(aData) })
    {
        // nothing
    }

    CPUDataVector::CPUDataVector(std::vector<size_t> aRowStrides,
                                 std::vector<uint8_t> aRowSubOffsets,
                                 std::vector<uint8_t> aPixelBitOffsets,
                                 std::vector<std::uint8_t>&& aData1,
                                 std::vector<std::uint8_t>&& aData2)
        : CPUDataVector(aRowStrides, aRowSubOffsets, aPixelBitOffsets,
                        std::vector<std::vector<std::uint8_t>> { std::move(aData1), std::move(aData2) })
    {
        // nothing
    }

    CPUDataVector::CPUDataVector(std::vector<size_t> aRowStrides,
                                 std::vector<uint8_t> aRowSubOffsets,
                                 std::vector<uint8_t> aPixelBitOffsets,
                                 std::vector<std::uint8_t>&& aData1,
                                 std::vector<std::uint8_t>&& aData2,
                                 std::vector<std::uint8_t>&& aData3)
        : CPUDataVector(aRowStrides, aRowSubOffsets, aPixelBitOffsets,
                        std::vector<std::vector<std::uint8_t>> { std::move(aData1), std::move(aData2), std::move(aData3) })
    {
        // nothing
    }

    CPUDataVector::~CPUDataVector()
    {
        if (numPlanes)
        {
            --dataAllocations.get().count;
        }

        for (size_t c = 0; c < numPlanes; ++c)
        {
            dataAllocations.get().size -= int64_t(size[c]);
        }
    }

    Storage* CPUDataVector::clone() const
    {
        return new CPUDataVector(*this);
    }

    Storage* CPUDataVector::moveToClone()
    {
        return new CPUDataVector(std::move(*this));
    }

    CPUSubDataReference::CPUSubDataReference(CPUSubDataReference&& aOther)
        : CPUDataReference(aOther)
        , parentStorage(std::move(aOther.parentStorage))
    {
        // nothing
    }

    CPUSubDataReference::CPUSubDataReference(const CPUSubDataReference& aOther)
        : CPUDataReference(aOther)
        , parentStorage(aOther.parentStorage)
    {
        numPlanes = aOther.numPlanes;
        for (size_t c = 0; c < numPlanes; ++c)
        {
            address[c] = aOther.address[c];
            size[c] = aOther.size[c];
            rowStride[c] = aOther.rowStride[c];
            rowSubOffset[c] = aOther.rowSubOffset[c];
            pixelBitOffset[c] = aOther.pixelBitOffset[c];
        }
    }

    CPUSubDataReference::CPUSubDataReference(std::shared_ptr<const CPUDataReference> aStorage,
                                             std::vector<PlaneOffset> aOffsets)
        : parentStorage(aStorage)
    {
        numPlanes = parentStorage->numPlanes;
        assert(aOffsets.size() == numPlanes);
        for (size_t plane = 0; plane < numPlanes; ++plane)
        {
            const PlaneOffset& planeOffset = aOffsets[plane];
            size_t carryXOffset = 0;
            pixelBitOffset[plane] = parentStorage->pixelBitOffset[plane] + planeOffset.xBitOffset;
            if (pixelBitOffset[plane] >= 8)
            {
                carryXOffset = 1;
                pixelBitOffset[plane] -= 8;
            }

            size_t carryYOffset = 0;
            rowSubOffset[plane] = parentStorage->rowSubOffset[plane] + planeOffset.ySubOffset;
            if (rowSubOffset[plane] >= 8)
            {
                carryYOffset = 1;
                rowSubOffset[plane] -= 8;
            }

            size_t byteOffset =
                planeOffset.xByteOffset + carryXOffset +
                (planeOffset.yRowOffset + carryYOffset) * parentStorage->rowStride[plane];
            address[plane] =
                static_cast<const char*>(parentStorage->address[plane]) +
                byteOffset;
            rowStride[plane] = parentStorage->rowStride[plane];
            // size not updated as it is applicable to bitmaps
        }
    }

    Storage* CPUSubDataReference::clone() const
    {
        return new CPUSubDataReference(*this);
    }

    Storage* CPUSubDataReference::moveToClone()
    {
        return new CPUSubDataReference(std::move(*this));
    }

    void CPUSubDataReference::setDataAllocations(DataAllocations& aDataAllocations) const
    {
        parentStorage->setDataAllocations(aDataAllocations);
    }

    Storage* GPUDataReference::clone() const
    {
        return new GPUDataReference(*this);
    }

    Storage* GPUDataReference::moveToClone()
    {
        return new GPUDataReference(std::move(*this));
    }

    bool GPUDataReference::equal(const Storage& aStorage) const
    {
        if (auto other = dynamic_cast<const GPUDataReference*>(&aStorage))
        {
            if (gpuIndex == other->gpuIndex && numTextures == other->numTextures)
            {
                for (size_t c = 0; c < numTextures; ++c)
                {
                    if (textureId[c] != other->textureId[c])
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    Storage* FSDataReference::clone() const
    {
        return new FSDataReference(*this);
    }

    Storage* FSDataReference::moveToClone()
    {
        return new FSDataReference(std::move(*this));
    }

    bool FSDataReference::equal(const Storage& aStorage) const
    {
        if (auto other = dynamic_cast<const FSDataReference*>(&aStorage))
        {
            return path == other->path && offset == other->offset && size == other->size;
        }
        else
        {
            return false;
        }
    }

    FragmentedDataReference::FragmentedDataReference(const FragmentedDataReference& aOther)
        : data(aOther.data.size())
    {
        for (size_t c = 0; c < aOther.data.size(); ++c)
        {
            data[c] = aOther.data[c]->clone();
        }
    }

    FragmentedDataReference::FragmentedDataReference(FragmentedDataReference&& aOther)
        : data(std::move(aOther.data))
    {
        aOther.data.clear();
    }

    FragmentedDataReference::FragmentedDataReference(std::vector<Storage*>&& aFragments)
        : data(std::move(aFragments))
    {
        aFragments.clear();
    }

    FragmentedDataReference::~FragmentedDataReference()
    {
        for (auto ptr : data)
        {
            delete ptr;
        }
    }

    Storage* FragmentedDataReference::clone() const
    {
        return new FragmentedDataReference(*this);
    }

    Storage* FragmentedDataReference::moveToClone()
    {
        return new FragmentedDataReference(std::move(*this));
    }

    bool FragmentedDataReference::equal(const Storage& aStorage) const
    {
        if (auto other = dynamic_cast<const FragmentedDataReference*>(&aStorage))
        {
            return data == other->data;
        }
        else
        {
            return false;
        }
    }

    void FragmentedDataReference::setDataAllocations(DataAllocations& aDataAllocations) const
    {
        for (auto fragment : data)
        {
            fragment->setDataAllocations(aDataAllocations);
        }
    }

    Data::Data()
        : mStorage(new EmptyDataReference)
        , mStreamId(StreamIdUninitialized)
    {
        // nothing
    }

    Data::Data(const EndOfStream&, const Meta& aMeta)
        : mStorage(new EndOfStream)
        , mMeta(aMeta)
        , mStreamId(StreamIdUninitialized)
    {
        // nothing
    }

    Data::Data(const EndOfStream&, const StreamId aStreamId, const Meta& aMeta)
        : mStorage(new EndOfStream)
        , mMeta(aMeta)
        , mStreamId(aStreamId)
    {
        // nothing
    }

    Data::Data(const Storage& aStorage, const Meta& aFrameMeta, const StreamId aStreamId)
        : mStorage(aStorage.clone())
        , mMeta(aFrameMeta)
        , mStreamId(aStreamId)
    {
        // nothing
    }

    Data::Data(Storage&& aStorage, const Meta& aFrameMeta, const StreamId aStreamId)
        : mStorage(aStorage.moveToClone())
        , mMeta(aFrameMeta)
        , mStreamId(aStreamId)
    {
        // nothing
    }

    Data::Data(const Data& aData, const Meta& aFrameMeta, const StreamId aStreamId)
        : mMeta(aFrameMeta)
        , mExtractors(std::move(aData.mExtractors))
        , mStreamId(aStreamId)
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        mStorage = aData.mStorage;
    }

    Data::Data(const Storage& aStorage, const Meta& aFrameMeta, const Extractors& aExtractor, const StreamId aStreamId)
        : mStorage(aStorage.clone())
        , mMeta(aFrameMeta)
        , mExtractors(std::move(aExtractor))
        , mStreamId(aStreamId)
    {
    }

    Data::Data(const Meta& aFrameMeta, const Extractors& aExtractor, const StreamId aStreamId)
        : mStorage(new EmptyDataReference)
        , mMeta(aFrameMeta)
        , mExtractors(std::move(aExtractor))
        , mStreamId(aStreamId)
    {

    }

    Data::Data(const Data& aOther)
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        mStorage = aOther.mStorage;
        lock.unlock();
        mMeta = aOther.mMeta;
        mExtractors = std::move(aOther.mExtractors);
        mStreamId = aOther.mStreamId;
    }

    Data& Data::operator=(const Data& aOther)
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        mStorage = aOther.mStorage;
        lock.unlock();
        mMeta = aOther.mMeta;
        mExtractors = std::move(aOther.mExtractors);
        mStreamId = aOther.mStreamId;
        return *this;
    }

    Data::~Data()
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        mStorage.reset();
    }

    bool Data::isEndOfStream() const
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        if (mStorage)
        {
            return mStorage->getStorageType() == StorageType::EndOfStream;
        }
        else
        {
            return false;
        }
    }

    std::string Data::getHost() const
    {
        return "";
    }

    StorageType Data::getStorageType() const
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        return mStorage->getStorageType();
    }

    void Data::setDataAllocations(DataAllocations& aDataAllocations) const
    {
        mStorage->setDataAllocations(aDataAllocations);
    }

    const CPUDataReference& Data::getCPUDataReference() const
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        assert(mStorage->getStorageType() == StorageType::CPU);
        return static_cast<const CPUDataReference&>(*mStorage);
    }

    const GPUDataReference& Data::getGPUDataReference() const
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        assert(mStorage->getStorageType() == StorageType::GPU);
        return static_cast<const GPUDataReference&>(*mStorage);
    }

    const FSDataReference& Data::getFSDataReference() const
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        assert(mStorage->getStorageType() == StorageType::FS);
        return static_cast<const FSDataReference&>(*mStorage);
    }

    const FragmentedDataReference& Data::getFragmentedDataReference() const
    {
        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        assert(mStorage->getStorageType() == StorageType::Fragmented);
        return static_cast<const FragmentedDataReference&>(*mStorage);
    }

    const Meta& Data::getMeta() const
    {
        return mMeta;
    }

    ContentType Data::getContentType() const
    {
        return mMeta.getContentType();
    }

    RawFrameMeta Data::getFrameMeta() const
    {
        return mMeta.getRawFrameMeta();
    }

    CodedFrameMeta Data::getCodedFrameMeta() const
    {
        return mMeta.getCodedFrameMeta();
    }

    CommonFrameMeta Data::getCommonFrameMeta() const
    {
        return mMeta.getCommonFrameMeta();
    }

    bool operator==(const RawFrameMeta& aA, const RawFrameMeta& aB)
    {
        return aA.presIndex == aB.presIndex &&
            aA.presTime == aB.presTime &&
            aA.duration == aB.duration &&
            aA.format == aB.format &&
            aA.width == aB.width &&
            aA.height == aB.height;
    }

    bool operator==(const CodedFrameMeta& aA, const CodedFrameMeta& aB)
    {
        return aA.presIndex == aB.presIndex &&
            aA.codingIndex == aB.codingIndex &&
            aA.codingTime == aB.codingTime &&
            aA.presTime == aB.presTime &&
            aA.duration == aB.duration &&
            aA.format == aB.format &&
            aA.type == aB.type;
    }

    bool operator==(const Meta& aA, const Meta& aB)
    {
        return aA.getContentType() == aB.getContentType() &&
            (aA.getContentType() == ContentType::RAW ? aA.getRawFrameMeta() == aB.getRawFrameMeta() :
             aA.getContentType() == ContentType::CODED ? aA.getCodedFrameMeta() == aB.getCodedFrameMeta() :
             true);
    }

    Data Data::subData(size_t aLeft, size_t aTop, size_t aWidth, size_t aHeight) const
    {
        assert(mStorage->getStorageType() == StorageType::CPU);
        assert(mMeta.getContentType() == ContentType::RAW);
        const auto& rawMeta = mMeta.getRawFrameMeta();
        if (aLeft + aWidth > rawMeta.width || aTop + aHeight > rawMeta.height)
        {
            throw OutOfBoundsException();
        }

        const RawFormatDescription& formatDescr = getRawFormatDescription(rawMeta.format);
        std::vector<PlaneOffset> offsets(formatDescr.numPlanes);
        for (size_t plane = 0; plane < formatDescr.numPlanes; ++plane)
        {
            PlaneOffset& offset = offsets[plane];
            offset.xByteOffset = aLeft * formatDescr.pixelStrideBits[plane] / 8;
            offset.xBitOffset = aLeft * formatDescr.pixelStrideBits[plane] % 8;
            offset.yRowOffset = aTop * formatDescr.rowStrideOf8[plane] / 8;
            offset.ySubOffset = aTop * formatDescr.rowStrideOf8[plane] % 8;
        }

        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        CPUSubDataReference sub(std::static_pointer_cast<const CPUDataReference, const Storage>(mStorage),
                                offsets);
        lock.unlock();
        RawFrameMeta meta = rawMeta;
        meta.width = uint32_t(aWidth);
        meta.height = uint32_t(aHeight);
        return Data(std::move(sub), std::move(meta));
    }

    Data Data::subDataWrapAround(size_t aLeft, size_t aTop, size_t aWidth, size_t aHeight) const
    {
        assert(mStorage->getStorageType() == StorageType::CPU);
        assert(mMeta.getContentType() == ContentType::RAW);
        // The tile probably warps around, so the data needs to be copied. Only supports tiles that warp around horizontally for now
        // We don't support subdata that has width and height larger than original data
        const auto& rawMeta = mMeta.getRawFrameMeta();
        if (aLeft > rawMeta.width || aHeight > rawMeta.height || aTop + aHeight > rawMeta.height)
        {
            throw OutOfBoundsException();
        }

        const RawFormatDescription& formatDescr = getRawFormatDescription(rawMeta.format);
        std::vector<PlaneOffset> offsets(formatDescr.numPlanes);
        for (size_t plane = 0; plane < formatDescr.numPlanes; ++plane)
        {
            PlaneOffset& offset = offsets[plane];
            offset.xByteOffset = aLeft * formatDescr.pixelStrideBits[plane] / 8;
            offset.xBitOffset = aLeft * formatDescr.pixelStrideBits[plane] % 8;
            offset.yRowOffset = aTop * formatDescr.rowStrideOf8[plane] / 8;
            offset.ySubOffset = aTop * formatDescr.rowStrideOf8[plane] % 8;
         }

        std::unique_lock<std::mutex> lock(sDataStorageMutex);
        CPUSubDataReference sub(std::static_pointer_cast<const CPUDataReference, const Storage>(mStorage), offsets);
        lock.unlock();

         std::vector<std::vector<uint8_t>> contents;
         std::vector<std::size_t> mRowStrides;
         for (size_t plane = 0; plane < sub.numPlanes; plane++)
         {
             mRowStrides.push_back(aWidth * formatDescr.rowStrideOf8[plane] / 8);
             size_t planeWidth = (size_t) aWidth * formatDescr.pixelStrideBits[plane] / 8;
             size_t planeHeight = (size_t) aHeight * formatDescr.rowStrideOf8[plane] / 8;
             std::vector<uint8_t> planeContents(planeWidth * planeHeight);
             size_t noneWrapAroundSize = (size_t) (std::min(rawMeta.width - aLeft, aWidth) * formatDescr.pixelStrideBits[plane] / 8);
             size_t wrapAroundSize = (size_t) (planeWidth - noneWrapAroundSize);
             for (size_t i = 0; i < planeHeight; i++)
             {
                 std::memcpy(&planeContents[i * planeWidth], ((static_cast<const uint8_t*>(sub.address[plane])) + (i * sub.rowStride[plane])), noneWrapAroundSize);
                 std::memcpy(&planeContents[i * planeWidth + noneWrapAroundSize], (static_cast<const uint8_t*>(getCPUDataReference().address[plane]) + (i + aTop * formatDescr.rowStrideOf8[plane] / 8) * sub.rowStride[plane]), wrapAroundSize);
             }
             contents.push_back(planeContents);
         }

         CPUDataVector data(mRowStrides, { }, { }, std::move(contents));
         RawFrameMeta meta = rawMeta;
         meta.width = uint32_t(aWidth);
         meta.height = uint32_t(aHeight);
         return Data(std::move(data), meta);
    }

    bool operator==(const Storage& aA, const Storage& aB)
    {
        return aA.equal(aB);
    }

    bool operator==(const Data& aA, const Data& aB)
    {
        return *aA.mStorage == *aB.mStorage && aA.mMeta == aB.mMeta;
    }

    Extractors& Data::getExtractors()
    {
        return mExtractors;
    }

    const Extractors& Data::getExtractors() const
    {
        return mExtractors;
    }

    StreamId Data::getStreamId() const
    {
        return mStreamId;
    }
    StreamId Data::setStreamId(StreamId aStreamId)
    {
        mStreamId = aStreamId;
        return mStreamId;
    }
}
