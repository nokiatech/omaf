
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
#include "segmenternonfrag.h"
#include <streamsegmenter/segmenterapi.hpp>
#include "common/optional.h"
#include "common/utils.h"

namespace VDD
{

    SegmenterNonFrag::SegmenterNonFrag(Config aConfig)
        : Segmenter(aConfig.segmenterConfig, false)
        , mInitSegment(aConfig.initConfig)
    {
        mOutputStream.reset(new std::ofstream(aConfig.fileName, std::ios::binary));

        mMovieWriter.reset(StreamSegmenter::MovieWriter::create(*mOutputStream));

        mWaitForInitSegment = true;
    }

    SegmenterNonFrag::~SegmenterNonFrag()
    {
        mMovieWriter->finalize();
    }

    std::vector<Views> SegmenterNonFrag::process(const Views& data)
    {
        if (mWaitForInitSegment)
        {
            // we need to generate init segment first, before any media segments get written to output

            mInitSegment.process(data);
            auto initSegment = mInitSegment.prepareInitSegment();
            if (initSegment)
            {
                mMovieWriter->writeInitSegment(*initSegment);
                mWaitForInitSegment = false;
            }
        }
        return Segmenter::process(data);
    }

    void SegmenterNonFrag::writeSegment(StreamSegmenter::Segmenter::Segments& aSegments, std::vector<Views>& aFrames)
    {
        for (auto segment : aSegments)
        {
            mMovieWriter->writeSegment(segment);
        }
    }

}  // namespace VDD
