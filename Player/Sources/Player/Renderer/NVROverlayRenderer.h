
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

#include "NVRNamespace.h"

#include "Foundation/NVRHashMap.h"
#include "Renderer/NVREquirectangularMesh.h"
#include "Renderer/NVRVideoRenderer.h"
#include "Renderer/NVRVideoShader.h"

#include "Renderer/NVRIdentityRenderer.h"

OMAF_NS_BEGIN
class OverlayRenderer : public VideoRenderer
{
public:
    OverlayRenderer();
    ~OverlayRenderer();

public:  // Implements PanoramaRenderer
    virtual void_t render(const HeadTransform& headTransform,
                          const RenderSurface& renderSurface,
                          const CoreProviderSources& sources,
                          const TextureID& renderMask = TextureID::Invalid,
                          const RenderingParameters& renderingParameters = RenderingParameters());

private:  // Implements PanoramaRenderer
    virtual void_t createImpl();
    virtual void_t destroyImpl();

private:
    void_t create(VideoPixelFormat::Enum textureFormat);

    VideoShader mOpaqueShader;
    VideoShader mMaskShader;

    MemoryAllocator& mAllocator;
    typedef HashMap<sourceid_t, EquirectangularMesh*> EquirectTileMeshMap;

    bool_t mDirtyShader;

    // geometry for rendering overlay (always recalculated)
    EquirectangularMesh* mGeometryOmni;
    EquirectangularMesh* mGeometry2D;

    // used for viewport relative rendering
    IdentityRenderer* mIdentityRenderer;
};
OMAF_NS_END
