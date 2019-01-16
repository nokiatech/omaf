
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
#include <fstream>

#include "save.h"

namespace VDD
{
    namespace FileNameTemplate
    {
        std::string applyTemplate(std::string aTemplate, TemplateArguments aTemplateArguments)
        {
            std::string result;
            for (std::string::size_type index = 0; index != aTemplate.size();)
            {
                if (aTemplate[index] == '$')
                {
                    auto end = aTemplate.find('$', index + 1);
                    if (end == std::string::npos)
                    {
                        throw InvalidTemplate();
                    }
                    std::string keyword = aTemplate.substr(index + 1, end - (index + 1));
                    if (keyword == "Number")
                    {
                        result += Utils::to_string(aTemplateArguments.sequenceId.get());
                    }
                    else
                    {
                        throw InvalidTemplate();
                    }

                    index = end + 1;
                }
                else
                {
                    result += aTemplate[index];
                    ++index;
                }
            }
            return result;
        }
    }

    CannotWriteException::CannotWriteException(std::string aFilename)
        : Exception("CannotWriteException")
        , mFilename(aFilename)
    {
        // nothing
    }

    std::string CannotWriteException::message() const
    {
        return mFilename;
    }

    std::string CannotWriteException::getFilename() const
    {
        return mFilename;
    }

    Save::Save(Config aConfig) : mConfig(aConfig), mSequenceId(1)
    {
        // nothing
    }

    Save::~Save()
    {
        // nothing
    }

    std::vector<Views> Save::process(const Views& streams)
    {
        if (mConfig.disable)
        {
            if (streams[0].isEndOfStream())
            {
                return { streams };
            }
            else
            {
                return { { Data() } };
            }
        }
        else if (!streams[0].isEndOfStream())
        {
            FileNameTemplate::TemplateArguments arguments;
            arguments.sequenceId = mSequenceId;
            std::string name = FileNameTemplate::applyTemplate(mConfig.fileTemplate, arguments);
            std::ofstream stream(name, std::ios::binary);

            if (!stream)
            {
                throw CannotOpenFile(name);
            }

            switch (streams[0].getStorageType())
            {
            case StorageType::Fragmented:
            {
                const FragmentedDataReference& frags = streams[0].getFragmentedDataReference();

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
                const CPUDataReference& data = streams[0].getCPUDataReference();

                stream.write(reinterpret_cast<const char*>(data.address[0]),
                             std::streamsize(data.size[0]));
            }
            break;

            default:
                assert(0);  // not supported
            }

            stream.close();
            if (!stream)
            {
                throw CannotWriteException(name);
            }

            ++mSequenceId;

            return { { Data() } };
        }
        else
        {
            return { streams };
        }
    }

}  // namespace VDD
