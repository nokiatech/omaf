
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
#pragma once

#include "NVRNamespace.h"
#include "OMAFPlayerDataTypes.h"

#include "Provider/NVRCoreProvider.h"

OMAF_NS_BEGIN

namespace ProjectionType
{
    enum Enum
    {
        INVALID = -1,

        EQUIRECTANGULAR,
        EQUIRECTANGULAR_TILE,
        LAMBERT,
        SOURCE_PICKING,
        CUBEMAP,

        IDENTITY,   //No projection

		RAW, //Projects multiple content types to a planar view
		
        COUNT
    };
}

class VideoRenderer
{
public:

    VideoRenderer();
    virtual ~VideoRenderer();

    void_t create();
    void_t destroy();

    bool_t isValid();

public:

    virtual ProjectionType::Enum getType() const = 0;

    virtual void_t render(const HeadTransform& headTransform, const RenderSurface& renderSurface, const CoreProviderSources& sources, const TextureID& renderMask = TextureID::Invalid, const RenderingParameters& renderingParameters = RenderingParameters()) = 0;

private:

    virtual void_t createImpl() = 0;
    virtual void_t destroyImpl() = 0;

private:

    bool_t mValid;
};

OMAF_NS_END
