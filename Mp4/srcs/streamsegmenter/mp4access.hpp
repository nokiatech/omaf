
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
#ifndef MP4ACCES_HPP
#define MP4ACCES_HPP

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include "api/streamsegmenter/exceptions.hpp"
#include "bitstream.hpp"
#include "block.hpp"
#include "fourccint.hpp"

namespace StreamSegmenter
{
    class BoxNotFound : public std::runtime_error
    {
    public:
        BoxNotFound()
            : std::runtime_error("BoxNotFound")
        {
        }
    };

    class AmbiguousBox : public std::runtime_error
    {
    public:
        AmbiguousBox()
            : std::runtime_error("AmbiguousBox")
        {
        }
    };

    struct Box;
    typedef std::map<FourCCInt, std::list<Box>> BoxIndex;

    struct Box : public Block
    {
        Box(FourCCInt aFourcc, size_t aBlockOffset, size_t aBlockSize);

        FourCCInt fourcc;

        bool operator<(const Box& other)
        {
            return fourcc < other.fourcc
                       ? true
                       : fourcc > other.fourcc
                             ? false
                             : blockOffset < other.blockOffset
                                   ? true
                                   : blockOffset > other.blockOffset
                                         ? false
                                         : blockSize < other.blockSize ? true
                                                                       : blockSize > other.blockSize ? false : false;
        }

        bool indexBuilt;
        BoxIndex index;
    };

    class MP4Access
    {
    public:
        MP4Access(std::istream& aStream);

        Box getBox(FourCCInt aFourcc) const;
        Vector<std::uint8_t> getData(const Block& aBlock) const;
        BitStream getBitStream(const Block& aBlock) const;
        BitStream getBitStream(FourCCInt aFourcc) const;
        template <typename BoxType>
        void parseBox(BoxType& aBox) const;

        virtual ~MP4Access();

    private:
        static BoxIndex makeIndex(std::istream& aStream, const Box* aParent);

        std::istream& mStream;
        BoxIndex mIndex;
    };

    template <typename BoxType>
    void MP4Access::parseBox(BoxType& aBox) const
    {
        BitStream bs = getBitStream(aBox.getType());
        aBox.parseBox(bs);
    }
}

#endif  // MP4ACCESS_HPP
