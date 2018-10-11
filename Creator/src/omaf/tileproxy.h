
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

#include <memory>
#include <vector>

#include "async/future.h"
#include "processor/processor.h"
#include "processor/data.h"
#include "common/rational.h"
#include "tileconfig.h"


namespace VDD {

    class TileProxy : public Processor
    {
    public:
        struct Config
        {
            size_t tileCount = 0;
            StreamAndTrackIds extractors;
            TileMergeConfig tileMergingConfig;
            PipelineOutput outputMode;
        };
        TileProxy(Config config);
        virtual ~TileProxy() = default;
        virtual StorageType getPreferredStorageType() const override;
        virtual std::vector<Views> process(const Views& data) override;

    protected:

        virtual bool extractorDataCollectionReady() const;
        virtual bool extractorReadyForEoS() const;
        virtual void processExtractors(std::vector<Views>& aTiles);
        virtual void createEoS(std::vector<Views>& aTiles);
        virtual CPUDataVector createExtractorSEI(const RegionPacking& aRegionPacking, unsigned int aTemporalIdPlus1);

    private:
        Data collectExtractors();

    protected:
        TileMergeConfig mTileMergingConfig;
        size_t mTileCount;

        //cache for extractors
        std::map<StreamId, std::list<VDD::Data>> mExtractorCache;
        bool mExtractorSEICreated = false;
        PipelineOutput mOutputMode;

    private:
        StreamId mExtractorStreamId;
        TrackId mExtractorTrackId;
    };

}
