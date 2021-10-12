
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
#include <cassert>

#include "googlevrvideoinput.h"
#include "controllerconfigure.h"
#include "controllerops.h"

#include "medialoader/mp4loader.h"


namespace VDD
{
    ExpectedExactlyOneVRTrack::ExpectedExactlyOneVRTrack()
        : Exception("ExpectedExactlyOneVRTrack")
    {
        // nothing
    }

    UnsupportedStereoscopicLayout::UnsupportedStereoscopicLayout(std::string aMessage)
        : Exception("UnsupportedStereoscopicLayout")
        , mMessage(aMessage)
    {
        // nothing
    }

    std::string UnsupportedStereoscopicLayout::message() const
    {
        return "Unsupported stereoscopic layout: " + mMessage;
    }

    /** @brief Given an MP4Loader, retrieve its Google VR Video Metadata, if any */
    Optional<GoogleVRVideoMetadata> loadGoogleVRVideoMetadata(MP4Loader& aMP4Loader)
    {
        auto trackIds = aMP4Loader.getTracksBy([](MediaSource& aSource)
                                               {
                                                   return (!!aSource.getStereoScopic3D()
                                                           || !!aSource.getSphericalVideoV1()
                                                           || !!aSource.getSphericalVideoV2());
                                               });
        if (trackIds.size() > 0)
        {
            if (trackIds.size() > 1)
            {
                throw ExpectedExactlyOneVRTrack();
            }
            auto trackId = *trackIds.begin();
            auto source = aMP4Loader.sourceForTrack(trackId);

            GoogleVRVideoMetadata metadata {};
            metadata.vrTrackId = trackId;

            if (auto stereoscopic = source->getStereoScopic3D())
            {
                metadata.mode = videoInputOfProperty(*stereoscopic);
            }
            else
            {
                metadata.mode = VideoInputMode::Mono;
            }

            if (auto sphericalV1 = source->getSphericalVideoV1())
            {
                // these are always true:
                if (sphericalV1->projectionType != MP4VR::ProjectionType::EQUIRECTANGULAR
                    || !sphericalV1->spherical
                    || !sphericalV1->stitched)
                {
                    throw UnsupportedStereoscopicLayout("Spherical V1 flags are invalid");
                }
                // this we don't support yet
                if (sphericalV1->croppedAreaImageWidthPixels != 0
                    || sphericalV1->croppedAreaImageHeightPixels != 0
                    || sphericalV1->croppedAreaLeftPixels != 0
                    || sphericalV1->croppedAreaTopPixels != 0)
                {
                    throw UnsupportedStereoscopicLayout("Cropping not supported");
                }
            }

            if (auto sphericalV2 = source->getSphericalVideoV2())
            {
                // unsupported cases
                if (sphericalV2->projectionType != MP4VR::ProjectionType::EQUIRECTANGULAR)
                {
                    // cubemap has its own layout and can have padding too, not trivial to use
                    throw UnsupportedStereoscopicLayout("Only equirectangular projection supported");
                }
                auto& eq = sphericalV2->projection.equirectangular;
                if (eq.boundsTopFP != 0
                    || eq.boundsBottomFP != 0
                    || eq.boundsLeftFP != 0
                    || eq.boundsRightFP != 0)
                {
                    throw UnsupportedStereoscopicLayout("Non-zero bounds not supported");
                }
            }

            return metadata;
        }
        else
        {
            return {};
        }
    }

}
