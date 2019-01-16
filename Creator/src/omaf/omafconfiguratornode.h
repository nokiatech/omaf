
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
#pragma once
#include "processor/processor.h"
#include "controller/common.h"

namespace VDD
{
    /** @brief A Processor that simply copies its input to its output.

        Useful as an example or perhaps a place holder.
    */
    class OmafConfiguratorNode : public Processor
    {
    public:
        struct Config
        {
            size_t *counter; // if not null, incremented on each call to process()
            PipelineOutput videoOutput;
        };
        OmafConfiguratorNode(Config aConfig);
        ~OmafConfiguratorNode() override;

        StorageType getPreferredStorageType() const override
        {
            return StorageType::CPU;
        }

        std::vector<Views> process(const Views& data) override;

    private:
        void createProjectionSEI(const Data& input, std::vector<uint8_t>& seiNal);
        void createFramePackingSEI(const Data& input, std::vector<uint8_t>& seiNal);
        void createFakeRwpk(VDD::CodedFrameMeta& aMeta, PipelineOutput aVideoOutput);
    private:
        const Config mConfig;
        bool mEnd = false;  //< Have we received the empty frame signal in the end?
        bool mFirst = true; // Have we created OMAF SEI in the beginning of the stream?
    };
}

