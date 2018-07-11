
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
#include "metacapturesink.h"

namespace VDD
{
    MetaCaptureSink::MetaCaptureSink(const Config& aConfig)
        : mPickFirstOne(aConfig.pickFirstOne)
        , mStreamId(aConfig.streamId)
        , mDone(false)
    {
    }

    MetaCaptureSink::~MetaCaptureSink() = default;

    StorageType MetaCaptureSink::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    Future<std::vector<CodedFrameMeta>> MetaCaptureSink::getCodedFrameMeta() const
    {
        return mCodedFrameMeta;
    }

    void MetaCaptureSink::consume(const Views& aViews)
    {
        if (!mDone)
        {
            if (mPickFirstOne)
            {
                mDone = true;
                if (!aViews[0].isEndOfStream())
                {
                    std::vector<CodedFrameMeta> meta;
                    for (auto& data : aViews)
                    {
                        meta.push_back(data.getCodedFrameMeta());
                    }
                    mCodedFrameMeta.set(meta);
                }
            }
            else
            {
                if (!aViews[0].isEndOfStream())
                {
                    std::vector<CodedFrameMeta> meta;
                    for (auto& data : aViews)
                    {
                        if (data.getStreamId() == mStreamId)
                        {
                            auto codedMeta = data.getCodedFrameMeta();
                            meta.push_back(codedMeta);
                            mDone = true;
                        }
                    }
                    if (!meta.empty())
                    {
                        mCodedFrameMeta.set(meta);
                    }
                }
            }
        }
    }
}
