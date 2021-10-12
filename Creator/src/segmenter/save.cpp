
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
#include <fstream>

#include "save.h"
#include "common/utils.h"

namespace VDD
{
    int sUsedTemplateIndex;
    std::map<std::string, int> sUsedTemplates; // don't let code to write to the same template twice

    namespace FileNameTemplate
    {
        std::string applyTemplate(std::string aTemplate, TemplateArguments aTemplateArguments)
        {
            std::string result;
            bool keywordApplied = false;
            for (std::string::size_type index = 0; index != aTemplate.size();)
            {
                if (aTemplate[index] == '$')
                {
                    auto end = aTemplate.find('$', index + 1);
                    if (end == std::string::npos)
                    {
                        throw InvalidTemplate("No matchind $ found");
                    }
                    std::string keyword = aTemplate.substr(index + 1, end - (index + 1));
                    if (keyword == "Number")
                    {
                        result += Utils::to_string(aTemplateArguments.sequenceId.get());
                    }
                    else
                    {
                        throw InvalidTemplate("Invalid parameter in the template");
                    }

                    index = end + 1;
                    keywordApplied = true;
                }
                else
                {
                    result += aTemplate[index];
                    ++index;
                }
            }
            return result;
        }

        InvalidTemplate::InvalidTemplate(std::string aMessage)
            : Exception("InvalidTemplate"), mMessage(aMessage)
        {
            // nothing
        }

        std::string InvalidTemplate::message() const
        {
            return mMessage;
        }
    }  // namespace FileNameTemplate

    CannotWriteException::CannotWriteException(std::string aFilename)
        : Exception("CannotWriteException"), mFilename(aFilename)
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
        if (sUsedTemplates.count(aConfig.fileTemplate))
        {
            std::cerr << "Template " << aConfig.fileTemplate << " used before with index "
                      << sUsedTemplates[aConfig.fileTemplate] << std::endl;
            abort();
        }
        sUsedTemplates[aConfig.fileTemplate] = sUsedTemplateIndex;
        sUsedTemplateIndex += 1;
    }

    Save::~Save()
    {
        // nothing
    }

    std::vector<Streams> Save::process(const Streams& streams)
    {
        if (mConfig.disable)
        {
            if (streams.isEndOfStream())
            {
                return { streams };
            }
            else
            {
                return { { Data() } };
            }
        }
        else if (!streams.isEndOfStream())
        {
            FileNameTemplate::TemplateArguments arguments;
            arguments.sequenceId = mSequenceId;
            std::string name = FileNameTemplate::applyTemplate(mConfig.fileTemplate, arguments);
            if (name == mPrevName)
            {
                throw FileNameTemplate::InvalidTemplate(
                    "Template resulted in overwriting file just written");
            }
            Utils::ensurePathForFilename(name);
            mPrevName = name;
            std::ofstream stream(name, std::ios::binary);

            if (!stream)
            {
                throw CannotOpenFile(name);
            }

            for (auto& frame: streams)
            {
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
            }
            ++mSequenceId;

            stream.close();
            if (!stream)
            {
                throw CannotWriteException(name);
            }

            return { { Data() } };
        }
        else
        {
            return { streams };
        }
    }

    std::string Save::getGraphVizDescription()
    {
        return "Save with template " + mConfig.fileTemplate;
    }

}  // namespace VDD
