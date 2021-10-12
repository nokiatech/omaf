
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
#pragma once

#include <fstream>
#include <map>
#include <set>

#include <streamsegmenter/autosegmenter.hpp>
#include <streamsegmenter/segmenterapi.hpp>
#include <streamsegmenter/track.hpp>

#include "segmenter.h"
#include "segmenterinit.h"

namespace VDD
{
    /** Non-fragmented segmenter, used with file output.
     *  Manages also init segments, using SegmenterInit in passive mode.
     */
    class SegmenterNonFrag : public Segmenter
    {
    public:
        struct Config
        {
            std::string fileName;

            Segmenter::Config segmenterConfig;

            /** COnfig for the init segment, which is saved in the same file in nonfragmented case */
            SegmenterInit::Config initConfig;
        };

        SegmenterNonFrag(Config aConfig);
        ~SegmenterNonFrag() override;

        std::string getGraphVizDescription() override;

        std::vector<Streams> process(const Streams& data) override;

    protected:
        virtual void writeSegment(StreamSegmenter::Segmenter::Segments& aSegments, std::vector<Streams>& aFrames) override;

    private:
        Config mConfig;

        SegmenterInit mInitSegment;

        std::unique_ptr<StreamSegmenter::MovieWriter, SegmentMovieWriterDestructor> mMovieWriter;

        std::unique_ptr<std::ostream> mOutputStream;

    };
}

