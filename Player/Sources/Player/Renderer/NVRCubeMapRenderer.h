
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
#include "Math/OMAFVector2.h"
#include "Math/OMAFVector3.h"
#include "Math/OMAFVector4.h"

#include "Renderer/NVRVideoRenderer.h"
#include "Renderer/NVRCubeMapMesh.h"

#include "Renderer/NVRVideoShader.h"

OMAF_NS_BEGIN
    class CubeMapRenderer
    : public VideoRenderer
    {
        public:

            CubeMapRenderer();
            ~CubeMapRenderer();

        public: // Implements PanoramaRenderer

            virtual ProjectionType::Enum getType() const;

            virtual void_t render(const HeadTransform& headTransform, const RenderSurface& renderSurface, const CoreProviderSources& sources, const TextureID& renderMask = TextureID::Invalid, const RenderingParameters& renderingParameters = RenderingParameters());

        private: // Implements PanoramaRenderer

            virtual void_t createImpl();
            virtual void_t destroyImpl();

            void_t initializeCubeMapData(CubeMapSource* source);
            void_t createCubemapVideoShader(VideoShader& shader, VideoPixelFormat::Enum videoTextureFormat, const bool_t maskEnabled = false);

        private:

            struct FaceData
            {
                Vector2 mFacePixelPadding;
                Vector3 mOrientation;
                uint32_t mNumCoordinateSets;
                uint32_t* mIndices; // face index for the coordinate set
                Vector4* mSourceCoordinates; // source x,y, width, height
                Vector4* mTargetCoordinates; // target x,y, width, height
                uint32_t* mSourceOrientations; // CubeFaceSectionOrientation::Enum
            };
            void_t setCubemapFaceCoords(VideoShader& shader, const FaceData& faceData);

            VideoShader mOpaqueShader;

            CubeMapMesh mGeometry;
            bool_t mDirtyShader;
            FaceData mFaceData;
    };
OMAF_NS_END
