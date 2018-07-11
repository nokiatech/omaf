
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
#include "segmenter/segmenter.h"

namespace VDD
{
    /** A method to save data that is composed by two separate sources: SegmenterInit and
     * Segmenter. They tag the output data with SegmentRole::InitSegment and TrackRunSegment,
     * correspondingly, and so SingleFileSave can determine what kind of data it is saving, so it
     * knows to save init segment first and then just let all the other data fall through. This also
     * allows the input data to come always in the slot 0.
     *
     * If it happens so that it SingleFileSave is given a large number of track runs before the init
     * segment, the internal buffering can grow large. Let's hope this is not going to be the
     * case.
     */
    class SingleFileSave : public Processor
    {
    public:
        struct Config
        {
            /** @brief Output file name */
            std::string filename;

            /** @brief SingleFileSave signals each completed save by returning an empty data.
             * This allow disabling the actual writing part but still keeping
             * that behavior.
             */
            bool disable;
        };

        SingleFileSave(Config aConfig);
        ~SingleFileSave() override;

        StorageType getPreferredStorageType() const override
        {
            return StorageType::Fragmented;
        }

        // returns an empty Data upon processing data; this is useful for determining when the data
        // has been saved
        std::vector<Views> process(const Views& data) override;

    private:
        Config mConfig;

        // for track runs and potentially segment indices waiting to be written
        std::map<SegmentRole, std::list<Data>> mDataQueue;

        // once this is empty, we may produce EndOfStream when we receive one
        std::set<SegmentRole> mRolesPending;

        Optional<std::ostream::pos_type> mSidxPosition;

        void writeData(std::ostream& aStream, const Data& aData, SegmentRole aRole);
    };
}
