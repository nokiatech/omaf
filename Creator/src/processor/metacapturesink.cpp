
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
#include "metacapturesink.h"

namespace VDD
{
    MetaCaptureSink::MetaCaptureSink(const Config&)
    {
        // nothing
    }

    MetaCaptureSink::~MetaCaptureSink() = default;

    StorageType MetaCaptureSink::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    Future<std::vector<Meta>> MetaCaptureSink::getMeta() const
    {
        return mMeta;
    }

    void MetaCaptureSink::consume(const Streams& aStreams)
    {
        if (mFirst)
        {
            mFirst = false;
            if (!aStreams.isEndOfStream())
            {
                std::vector<Meta> meta;
                for (auto& data: aStreams)
                {
                    meta.push_back(data.getMeta());
                }
                mMeta.set(meta);
            }
        }
    }
}
