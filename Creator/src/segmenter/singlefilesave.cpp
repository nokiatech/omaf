
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
#include "singlefilesave.h"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

#include "save.h"       // for CannotWriteException
#include "segmenter.h"  // for SegmentRoleTag

namespace VDD
{
    SingleFileSave::SingleFileSave(Config aConfig) : mConfig(aConfig)
    {
        if (aConfig.requireInitSegment)
        {
            mRolesPending.insert(SegmentRole::InitSegment);
        }
        if (aConfig.expectSegmentIndex)
        {
            mRolesPending.insert(SegmentRole::SegmentIndex);
        }
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

        auto ofs0 = aStream.tellp();

        switch (aData.getStorageType())
        {
        case StorageType::Fragmented:
        {
            const FragmentedDataReference& frags = aData.getFragmentedDataReference();

            for (size_t n = 0; n < frags.data.size(); ++n)
            {
                auto& data = dynamic_cast<const CPUDataReference&>(*frags.data[n]);
                log(LogLevel::Info) << "SingleFileSave " << getId() << " fragment " << data.size[0] << std::endl;
                aStream.write(reinterpret_cast<const char*>(data.address[0]),
                              std::streamsize(data.size[0]));
            }
        }
        break;

        case StorageType::CPU:
        {
            const CPUDataReference& data = aData.getCPUDataReference();

            log(LogLevel::Info) << "SingleFileSave " << getId() << " CPU data " << data.size[0] << std::endl;
            aStream.write(reinterpret_cast<const char*>(data.address[0]),
                          std::streamsize(data.size[0]));
        }
        break;

        default:
            assert(0);  // not supported
        }

        auto ofs1 = aStream.tellp();

        std::string roleStr = to_string(aRole);

        log(LogLevel::Info) << "SingleFileSave " << getId() << " Wrote " << roleStr << " to " << ofs0 << " .. " << ofs1 << " = "
                            << ofs1 - ofs0 << std::endl;

        if (aRole == SegmentRole::SegmentIndex && originalPosition != *mSidxPosition)
        {
            aStream.seekp(originalPosition);
        }
    }

    std::vector<Streams> SingleFileSave::process(const Streams& aStreams)
    {
        size_t index = 0;
        bool finished = false;
        std::unique_lock<std::mutex> lock(mRolesPendingMutex);
        for (auto& frame : aStreams)
        {
            auto tag = frame.getMeta().findTag<SegmentRoleTag>();

            // the SegmentRoleTag is required for SingleFileSave
            if (!tag)
            {
                auto& output = log(LogLevel::Assert);
                output << "id " << getId() << " Pending roles:";
                for (auto role : mRolesPending)
                {
                    output << " " << to_string(role);
                }
                output << std::endl;
            }
            assert(tag);

            auto tagValue = tag->get();
            mRolesPending.erase(tagValue);
            if (aStreams.isEndOfStream())
            {
                finished = finished | (mRolesPending.size() == 0);
            }
            else if (mConfig.disable)
            {
                return {{Data()}};
            }
            else
            {
                std::ofstream stream;

                assert(mConfig.expectSegmentIndex    || tagValue != SegmentRole::SegmentIndex);
                assert(mConfig.expectTrackRunSegment || tagValue != SegmentRole::TrackRunSegment);
                assert(mConfig.expectImdaSegment     || tagValue != SegmentRole::ImdaSegment);

                mDataQueue[tagValue].push_back(frame);

                bool tryRules = true;
                while (tryRules)
                {
                    for (auto role : std::array<SegmentRole, 4>{
                             SegmentRole::InitSegment, SegmentRole::SegmentIndex,
                             SegmentRole::TrackRunSegment, SegmentRole::ImdaSegment})
                    {
                        tryRules = false;
                        std::map<SegmentRole, bool> accept;
                        accept[SegmentRole::InitSegment] = true;
                        accept[SegmentRole::SegmentIndex] =
                            mRolesPending.count(SegmentRole::InitSegment) == 0;
                        accept[SegmentRole::TrackRunSegment] =
                            accept[SegmentRole::SegmentIndex] &&
                            mRolesPending.count(SegmentRole::SegmentIndex) == 0;
                        accept[SegmentRole::ImdaSegment] =
                            accept[SegmentRole::SegmentIndex] &&
                            mRolesPending.count(SegmentRole::SegmentIndex) == 0;

                        auto& queue = mDataQueue[role];
                        if (accept[role] && queue.size() > 0)
                        {
                            if (mConfig.expectImdaSegment)
                            {
                                // when expecting imda segments, we expect first a track run
                                // segment, then an imda follows, and then again, etc

                                switch (role)
                                {
                                case SegmentRole::InitSegment:   // fall through
                                case SegmentRole::SegmentIndex:  // fall through
                                    break;
                                case SegmentRole::TrackRunSegment:
                                {
                                    if (mConfig.expectImdaSegment)
                                    {
                                        tryRules = true;
                                        mRolesPending.insert(SegmentRole::ImdaSegment);
                                        mRolesPending.erase(SegmentRole::TrackRunSegment);
                                    }
                                    break;
                                }
                                case SegmentRole::ImdaSegment:
                                {
                                    if (mConfig.expectTrackRunSegment)
                                    {
                                        tryRules = true;
                                        mRolesPending.erase(SegmentRole::ImdaSegment);
                                        mRolesPending.insert(SegmentRole::TrackRunSegment);
                                    }
                                    break;
                                }
                                }
                            }

                            Utils::ensurePathForFilename(mConfig.filename);
                            stream.open(mConfig.filename,
                                        (std::ios::in | std::ios::out | std::ios::binary) |
                                            (mFirstWrite ? std::ios::trunc : std::ios::ate));
                            mFirstWrite = false;

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
                }
            }
            ++index;
        }
        if (finished)
        {
            return {{Data(EndOfStream())}};
        }
        else
        {
            return {{Data()}};
        }
    }

    std::string SingleFileSave::getGraphVizDescription()
    {
        std::unique_lock<std::mutex> lock(mRolesPendingMutex);
        std::ostringstream st;
        st << "SingleFileSave to " << mConfig.filename
           << (mConfig.expectImdaSegment ? "\nexpect imda" : "")
           << (mConfig.expectTrackRunSegment ? "\nexpect trun" : "")
            ;
        st << "\nPending:";
        for (auto role: mRolesPending)
        {
            st << " " << to_string(role);
        }
        return st.str();
    }
}  // namespace VDD
