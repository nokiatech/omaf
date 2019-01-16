
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
#include "mp4access.hpp"

namespace StreamSegmenter
{
    namespace
    {
        struct BoxInfo
        {
            FourCCInt fourcc;
            bool hasSubBoxes;
        };


        std::map<FourCCInt, BoxInfo> makeBoxInfos()
        {
            const std::vector<BoxInfo> boxInfos{{FourCCInt("ftyp"), false}};
            std::map<FourCCInt, BoxInfo> map;
            for (auto& boxInfo : boxInfos)
            {
                map.insert(std::make_pair(boxInfo.fourcc, boxInfo));
            }
            return map;
        }

        const std::map<FourCCInt, BoxInfo> boxInfos = makeBoxInfos();

        uint32_t readUInt32(std::istream& aStream)
        {
            uint32_t v = 0;
            for (int c = 0; c < 4; ++c)
            {
                v = (v << 8) | static_cast<uint8_t>(aStream.get());
            }
            if (!aStream.good())
            {
                throw PrematureEndOfFile();
            }
            return v;
        }
    }  // anonymous namespace

    Box::Box(FourCCInt aFourcc, size_t aBlockOffset, size_t aBlockSize)
        : Block(aBlockOffset, aBlockSize)
        , fourcc(aFourcc)
        , indexBuilt(false)
    {
        // nothing
    }

    BoxIndex MP4Access::makeIndex(std::istream& aStream, const Box* aParent)
    {
        BoxIndex index;

        std::streamoff endLimit = 0;
        if (aParent)
        {
            endLimit = std::streamoff(aParent->blockOffset + aParent->blockSize);
        }

        size_t offset = 0;
        aStream.get();
        if (aStream)
        {
            aStream.unget();
        }
        while (aStream && (!aParent || aStream.tellg() < endLimit))
        {
            size_t size      = size_t(readUInt32(aStream));
            FourCCInt fourcc = FourCCInt(readUInt32(aStream));
            aStream.seekg(aStream.tellg() + std::streamoff(size - 8));

            index[fourcc].push_back(Box(fourcc, offset, size));
            offset += size;
            aStream.get();
            if (aStream)
            {
                aStream.unget();
            }
        }

        aStream.clear();

        return index;
    }

    MP4Access::MP4Access(std::istream& aStream)
        : mStream(aStream)
        , mIndex(makeIndex(aStream, nullptr))
    {
    }

    MP4Access::~MP4Access()
    {
    }

    Box MP4Access::getBox(FourCCInt aFourcc) const
    {
        auto at = mIndex.find(aFourcc);
        if (at == mIndex.end())
        {
            throw BoxNotFound();
        }
        else
        {
            auto& els = at->second;
            if (els.size() == 0)
            {
                throw BoxNotFound();
            }
            else if (els.size() > 1)
            {
                throw AmbiguousBox();
            }
            else
            {
                return *at->second.begin();
            }
        }
    }

    Vector<std::uint8_t> MP4Access::getData(const Block& aBlock) const
    {
        return aBlock.getData(mStream);
    }

    BitStream MP4Access::getBitStream(const Block& aBlock) const
    {
        return aBlock.getBitStream(mStream);
    }

    BitStream MP4Access::getBitStream(FourCCInt aFourcc) const
    {
        return getBox(aFourcc).getBitStream(mStream);
    }
}
