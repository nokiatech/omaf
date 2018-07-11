
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
#include <array>
#include <fstream>
#include <iostream>

#include "singlefilesave.h"

#include "save.h" // for CannotWriteException
#include "segmenter.h" // for SegmentRoleTag

namespace VDD
{
    SingleFileSave::SingleFileSave(Config aConfig) : mConfig(aConfig)
    {
        mRolesPending.insert(SegmentRole::InitSegment);
        mRolesPending.insert(SegmentRole::SegmentIndex);
    }

    SingleFileSave::~SingleFileSave() = default;

    void SingleFileSave::writeData(std::ostream& aStream, const Data& aData, SegmentRole aRole)
    {
        std::ostream::pos_type originalPosition = aStream.tellp();

        // Write segment indices always in the same offset. Of course, we don't receive separate
        // segment indices unless the Segmenter has been specifically configured to produce them
        // (and not, they come embedded in TrackRunSegments)
        if (aRole == SegmentRole::SegmentIndex)
        {
            if (!mSidxPosition)
            {
                mSidxPosition = aStream.tellp();
            }
            else
            {
                aStream.seekp(*mSidxPosition);
            }
        }

        switch (aData.getStorageType())
        {
        case StorageType::Fragmented:
        {
            const FragmentedDataReference& frags = aData.getFragmentedDataReference();

            for (size_t n = 0; n < frags.data.size(); ++n)
            {
                auto& data = dynamic_cast<const CPUDataReference&>(*frags.data[n]);
                aStream.write(reinterpret_cast<const char*>(data.address[0]),
                              std::streamsize(data.size[0]));
            }
        }
        break;

        case StorageType::CPU:
        {
            const CPUDataReference& data = aData.getCPUDataReference();

            aStream.write(reinterpret_cast<const char*>(data.address[0]),
                          std::streamsize(data.size[0]));
        }
        break;

        default:
            assert(0);  // not supported
        }

        if (aRole == SegmentRole::SegmentIndex && originalPosition != *mSidxPosition)
        {
            aStream.seekp(originalPosition);
        }
    }

    std::vector<Views> SingleFileSave::process(const Views& aStreams)
    {
        auto tag = aStreams[0].getMeta().findTag<SegmentRoleTag>();

        // the SegmentRoleTag is required for SingleFileSave
        assert(tag);
        std::map<SegmentRole, bool> accept;
        mRolesPending.erase(tag->get());
        accept[SegmentRole::InitSegment] = true;
        accept[SegmentRole::SegmentIndex] = mRolesPending.count(SegmentRole::InitSegment) == 0;
        accept[SegmentRole::TrackRunSegment] = accept[SegmentRole::SegmentIndex] && mRolesPending.count(SegmentRole::SegmentIndex) == 0;

        if (aStreams[0].isEndOfStream())
        {
            if (mRolesPending.size() == 0)
            {
                return { aStreams };
            }
            else
            {
                return {};
            }
        }
        else if (mConfig.disable)
        {
            return { { Data() } };
        }
        else
        {
            std::ofstream stream;

            mDataQueue[tag->get()].push_back(aStreams[0]);

            for (auto role : std::array<SegmentRole, 3> { SegmentRole::InitSegment, SegmentRole::SegmentIndex, SegmentRole::TrackRunSegment })
            {
                auto& queue = mDataQueue[role];
                if (accept[role] && queue.size() > 0)
                {
                    stream.open(mConfig.filename,
                                (std::ios::in | std::ios::out | std::ios::binary) |
                                (role == SegmentRole::InitSegment
                                 ? std::ios::trunc
                                 : std::ios::ate));
                    if (!stream)
                    {
                        throw CannotOpenFile(mConfig.filename);
                    }

                    for (auto& data : queue)
                    {
                        writeData(stream, data, role);
                    }
                    queue.clear();

                    stream.close();
                    if (!stream)
                    {
                        throw CannotWriteException(mConfig.filename);
                    }
                }
            }

            return { { Data() } };
        }
    }

}  // namespace VDD
