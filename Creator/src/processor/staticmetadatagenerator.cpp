
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
#include "staticmetadatagenerator.h"

namespace VDD
{
    StaticMetadataGenerator::StaticMetadataGenerator(const Config& aConfig)
        : mConfig(aConfig)
        , mIndex(0)
    {
        for (auto& sample : mConfig.metadataSamples)
        {
            std::vector<std::vector<uint8_t>> dataVec{ sample };
            CPUDataVector dataRef(std::move(dataVec));
            mData.push_back(Data(std::move(dataRef), CodedFrameMeta()));
        }
    }

    StaticMetadataGenerator::~StaticMetadataGenerator() = default;

    std::vector<Views> StaticMetadataGenerator::process(const Views& aData)
    {
        if (aData[0].isEndOfStream())
        {
            return { aData };
        }
        else
        {
            CodedFrameMeta codedMeta = aData[0].getCodedFrameMeta();
            if (codedMeta.presIndex % mConfig.mediaToMetaSampleRate == 0)
            {
                codedMeta.format = CodedFormat::TimedMetadata;

                Meta meta(codedMeta);
                mIndex %= mData.size();
                return{ { Data(mData[mIndex++], meta) } };
            }
            else
            {
                std::vector<Views> frames;
                return frames;
            }
        }
    }
} // namespace VDD
