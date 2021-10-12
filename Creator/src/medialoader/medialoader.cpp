
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
#include "medialoader.h"

namespace VDD
{
    std::list<TrackId> MediaSource::getTrackReferences(const std::string& /*aFourCC*/) const
    {
        return {};
    }

    MP4VR::ProjectionFormatProperty MediaSource::getProjection() const
    {
        return { MP4VR::EQUIRECTANGULAR };
    }

    MP4VR::PodvStereoVideoConfiguration MediaSource::getFramePacking() const
    {
        return MP4VR::MONOSCOPIC;
    }

    Optional<MP4VR::SphericalVideoV1Property> MediaSource::getSphericalVideoV1() const
    {
        return {};
    }

    Optional<MP4VR::SphericalVideoV2Property> MediaSource::getSphericalVideoV2() const
    {
        return {};
    }

    Optional<MP4VR::StereoScopic3DProperty> MediaSource::getStereoScopic3D() const
    {
        return {};
    }

    const MediaLoader::SourceConfig MediaLoader::defaultSourceConfig {};

    MediaLoader::MediaLoader() = default;
    MediaLoader::~MediaLoader() = default;
}  // namespace VDD
