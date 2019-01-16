
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
#ifndef STREAMSEGMENTER_PRIVATE_HPP
#define STREAMSEGMENTER_PRIVATE_HPP

#include <memory>

#include "customallocator.hpp"
#include "filetypebox.hpp"
#include "handlerbox.hpp"
#include "mediaheaderbox.hpp"
#include "moviebox.hpp"
#include "regionwisepackingbox.hpp"
#include "trackheaderbox.hpp"

#include "api/streamsegmenter/segmenterapi.hpp"

namespace StreamSegmenter
{
    namespace Segmenter
    {
        struct FileTypeBox
        {
            UniquePtr<::FileTypeBox> fileTypeBox;
            FileTypeBox(UniquePtr<::FileTypeBox>&& aFileTypeBox)
                : fileTypeBox(std::move(aFileTypeBox))
            {
                // nothing
            }
        };

        struct MovieBox
        {
            UniquePtr<::MovieBox> movieBox;
            MovieBox(UniquePtr<::MovieBox>&& aMovieBox)
                : movieBox(std::move(aMovieBox))
            {
                // nothing
            }
        };

        struct MediaHeaderBox
        {
            UniquePtr<::MediaHeaderBox> mediaHeaderBox;
            MediaHeaderBox(UniquePtr<::MediaHeaderBox>&& aMediaHeaderBox)
                : mediaHeaderBox(std::move(aMediaHeaderBox))
            {
                // nothing
            }
        };

        struct HandlerBox
        {
            UniquePtr<::HandlerBox> handlerBox;
            HandlerBox(UniquePtr<::HandlerBox>&& aHandlerBox)
                : handlerBox(std::move(aHandlerBox))
            {
                // nothing
            }
        };

        struct TrackHeaderBox
        {
            UniquePtr<::TrackHeaderBox> trackHeaderBox;
            TrackHeaderBox(UniquePtr<::TrackHeaderBox>&& aTrackHeaderBox)
                : trackHeaderBox(std::move(aTrackHeaderBox))
            {
                // nothing
            }
        };

        struct SampleEntryBox
        {
            UniquePtr<::SampleEntryBox> sampleEntryBox;
            SampleEntryBox(UniquePtr<::SampleEntryBox>&& aSampleEntryBox)
                : sampleEntryBox(std::move(aSampleEntryBox))
            {
                // nothing
            }
        };

        struct Region
        {
            UniquePtr<::RegionWisePackingBox::Region> region;
            Region(UniquePtr<::RegionWisePackingBox::Region>&& aRegion)
                : region(std::move(aRegion))
            {
                // nothing
            }
        };
    }
}  // namespace StreamSegmenter::Segmenter

#endif  // STREAMSEGMENTER_PRIVATE_HPP
