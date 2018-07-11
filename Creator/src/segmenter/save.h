
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

#include "processor/processor.h"

namespace VDD
{
    namespace FileNameTemplate
    {
        struct TemplateArguments
        {
            StreamSegmenter::Segmenter::SequenceId sequenceId;
        };
        class InvalidTemplate : public std::runtime_error
        {
        public:
            InvalidTemplate()
                : std::runtime_error("InvalidTemplate")
            {
            }
        };

        std::string applyTemplate(std::string aTemplate, TemplateArguments aTemplateArguments);
    }

    class CannotWriteException : public Exception
    {
    public:
        CannotWriteException(std::string aFilename);

        std::string message() const override;

        std::string getFilename() const;

    private:
        std::string mFilename;
    };

    class Save : public Processor
    {
    public:
        struct Config
        {
            /** @brief File name template following the Dash filename
             * templates syntax. Uses
             * StreamSegmenter::MPD::applyTemplate for generating actual
             * filenames.
             *
             * @see StreamSegmenter::MPD::applyTemplate */
            std::string fileTemplate;

            /** @brief Save signals each completed save by returning an empty data.
             * This allow disabling the actual writing part but still keeping
             * that behavior.
             */
            bool disable;
        };

        Save(Config aConfig);
        ~Save() override;

        StorageType getPreferredStorageType() const override
        {
            return StorageType::Fragmented;
        }

        // returns an empty Data upon processing data; this is useful for determining when the data
        // has been saved
        std::vector<Views> process(const Views& data) override;

    private:
        Config mConfig;
        StreamSegmenter::Segmenter::SequenceId mSequenceId;
    };
}

