
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

#include "Renderer/NVRVideoRenderer.h"
#include "Renderer/NVREquirectangularMesh.h"
#include "Renderer/NVRVideoShader.h"

OMAF_NS_BEGIN
    class EquirectangularRenderer
    : public VideoRenderer
    {
        public:

            EquirectangularRenderer();
            ~EquirectangularRenderer();

        public: // Implements PanoramaRenderer

            virtual ProjectionType::Enum getType() const;

            virtual void_t render(const HeadTransform& headTransform, const RenderSurface& renderSurface, const CoreProviderSources& sources, const TextureID& renderMask = TextureID::Invalid, const RenderingParameters& renderingParameters = RenderingParameters());

        private: // Implements PanoramaRenderer

            virtual void_t createImpl();
            virtual void_t destroyImpl();

        private:

            VideoShader mOpaqueShader;
            VideoShader mMaskShader;
            EquirectangularMesh mGeometry;
            bool_t mDirtyShader;
    };
OMAF_NS_END
