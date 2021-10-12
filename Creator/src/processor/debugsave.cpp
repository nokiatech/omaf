
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
#include "debugsave.h"

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "common/utils.h"
#include "log/logstream.h"
#include "segmenter/segmenter.h"

namespace VDD
{
    DebugSave::DebugSave(Config aConfig) : mConfig(aConfig)
    {
        (void) mConfig; // for now
    }

    DebugSave::~DebugSave()
    {
        // nothing
    }

    std::string DebugSave::getGraphVizDescription()
    {
        std::ostringstream st;
        return st.str();
    }

    namespace
    {
        template <typename T>
        std::string padded(size_t size, T x)
        {
            std::ostringstream st;
            st << std::setfill('0') << std::setw(size) << x;
            return st.str();
        }
    }  // namespace

    void DebugSave::ready()
    {
        std::string idStr = std::to_string(getId());
        auto regex = R"(^debugsave-)" + idStr + R"(\.[0-9][0-9][0-9][0-9]\.[0-9][0-9].(data|meta)$)";
        for (auto removeFile : Utils::listDirectoryContents(".", false, regex))
        {
            std::remove(removeFile.c_str());
        }
    }

    void DebugSave::consume(const Streams& aStreams)
    {
        std::string idStr = std::to_string(getId());

        ++mCount;

        if (aStreams.isEndOfStream())
        {
            size_t index = 0;
            for (auto& frame : aStreams)
            {
                std::ofstream stream;
                std::string filename = "debugsave-" + idStr + "." + padded(4, mCount) + "." +
                                       padded(2, index) + ".meta";
                stream.open(filename, std::ios::out | std::ios::trunc);
                stream << "End of stream" << std::endl;
                if (auto tag = frame.getMeta().findTag<SegmentRoleTag>())
                {
                    stream << "SegmentRole: " << to_string(tag->get()) << std::endl;
                }
                stream.close();

                ++index;
            }
        }
        else
        {
            size_t index = 0;
            for (auto& frame : aStreams)
            {
                {
                    std::ofstream stream;
                    std::string filename = "debugsave-" + idStr + "." + padded(4, mCount) + "." +
                                           padded(2, index) + ".meta";
                    stream.open(filename, std::ios::out | std::ios::trunc);
                    stream << "Stream Id: " << frame.getStreamId().get() << std::endl;
                    if (auto tag = frame.getMeta().findTag<SegmentRoleTag>())
                    {
                        stream << "SegmentRole: " << to_string(tag->get()) << std::endl;
                    }
                    stream.close();
                }

                {
                    std::ofstream stream;
                    std::string filename = "debugsave-" + idStr + "." + padded(4, mCount) + "." +
                                           padded(2, index) + ".data";
                    stream.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);

                    switch (frame.getStorageType())
                    {
                    case StorageType::Fragmented:
                    {
                        const FragmentedDataReference& frags = frame.getFragmentedDataReference();

                        for (size_t n = 0; n < frags.data.size(); ++n)
                        {
                            auto& data = dynamic_cast<const CPUDataReference&>(*frags.data[n]);
                            stream.write(reinterpret_cast<const char*>(data.address[0]),
                                         std::streamsize(data.size[0]));
                        }
                    }
                    break;

                    case StorageType::CPU:
                    {
                        const CPUDataReference& data = frame.getCPUDataReference();

                        stream.write(reinterpret_cast<const char*>(data.address[0]),
                                     std::streamsize(data.size[0]));
                    }
                    break;

                    default:
                        assert(0);  // not supported
                    }

                    stream.close();
                    ++index;
                }
            }
        }
    }

}  // namespace VDD
