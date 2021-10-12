
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
#include "metamodifyprocessor.h"

namespace VDD
{
    MetaModifyProcessor::MetaModifyProcessor(const Config& aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    MetaModifyProcessor::~MetaModifyProcessor() = default;

    StorageType MetaModifyProcessor::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    std::vector<Streams> MetaModifyProcessor::process(const Streams& aStreams)
    {
        std::vector<Data> streams;

        std::vector<Meta> metas;
        metas.reserve(aStreams.size());
        for (auto& stream : aStreams)
        {
            metas.push_back(stream.getMeta());
        }
        metas = mConfig.metaModifyCallback(metas);
        assert(metas.size() == aStreams.size());

        auto streamIt = aStreams.begin();
        for (size_t index = 0u; index < metas.size(); ++index)
        {
            streams.push_back(Data(*streamIt, metas.at(index), streamIt->getStreamId()));
            ++streamIt;
        }

        return { Streams(streams.begin(), streams.end()) };
    }
}
