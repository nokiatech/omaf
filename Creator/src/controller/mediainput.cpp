
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
#include "mediainput.h"

#include <reader/mp4vrfilereaderinterface.h>
#include <reader/mp4vrfilestreaminterface.h>
#include "medialoader/mp4loader.h"

namespace VDD
{
    std::unique_ptr<MediaLoader> MediaInputConfig::makeMediaLoader(bool aVideoNalStartCodes) const
    {
        try
        {
            return Utils::make_unique<MP4Loader>(getMP4LoaderConfig(), aVideoNalStartCodes);
        }
        catch (MP4LoaderError& mp4Error)
        {
            if (mp4Error.getCode() == MP4VR::MP4VRFileReaderInterface::FILE_OPEN_ERROR)
            {
                throw mp4Error;
            }
            else
            {
                try
                {
                    return Utils::make_unique<H265Loader>(getH265LoaderConfig(),
                                                          aVideoNalStartCodes);
                }
                catch (ConfigValueReadError& h265Error)
                {
                    throw MediaLoaderException("Failed to open input as MP4 for " + filename + " (" +
                                               mp4Error.message() +
                                               "); error from H265: " + h265Error.message());
                }
            }
        }
    }

    Optional<FrameDuration> getMediaDurationConsumingSource(MediaSource& aMediaSource)
    {
        if (aMediaSource.getDuration())
        {
            return aMediaSource.getDuration();
        }
        else
        {
            bool eof = false;
            FrameDuration duration;
            while (!eof)
            {
                auto frames = aMediaSource.produce();
                if (frames.size())
                {
                    eof = frames[0].isEndOfStream();
                    if (!eof)
                    {
                        auto meta = frames[0][0].getCodedFrameMeta();
                        duration += meta.duration;
                    }
                }
            }
            return duration;
        }
    }
}  // namespace VDD