
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
#include "metagateprocessor.h"

namespace VDD
{
    MetaGateProcessor::MetaGateProcessor(const Config& aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    MetaGateProcessor::~MetaGateProcessor() = default;

    StorageType MetaGateProcessor::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    std::vector<Views> MetaGateProcessor::process(const Views& aViews)
    {
        if (mFirst)
        {
            mFirst = false;
            if (!aViews[0].isEndOfStream())
            {
                std::vector<CommonFrameMeta> meta;
                for (auto& data: aViews)
                {
                    meta.push_back(data.getCommonFrameMeta());
                }
                mAllowPass = mConfig.metaCallback(meta);
            }
        }
        if (mAllowPass)
        {
            return { aViews };
        }
        else
        {
            return { { Data(EndOfStream()) } };
        }
    }
}
