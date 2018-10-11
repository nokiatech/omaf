
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
#pragma once

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

#include <mp4vrfiledatatypes.h>

#include "Provider/NVRCoreProvider.h"
#include "VideoDecoder/NVRVideoDecoderConfig.h"
#include "Foundation/NVRPathName.h"
#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
#include "DashProvider/NVRMPDAttributes.h"
#endif
OMAF_NS_BEGIN


    class MetadataParser
    {
    public:
        struct Rwpk
        {
            uint32_t projWidth;
            uint32_t projHeight;
            uint32_t projLeft;
            uint32_t projTop;
            uint8_t transform;
            uint16_t packedWidth;
            uint16_t packedHeight;
            uint16_t packedLeft;
            uint16_t packedTop;
        };

    public:

        MetadataParser();
        ~MetadataParser();

        void reset();

        static bool_t parseUri(const char_t* uri, BasicSourceInfo& data);
        Error::Enum parseOmafCubemapRegionMetadata(const MP4VR::RegionWisePackingProperty& aRwpk, BasicSourceInfo& aBasicSourceInfo);
        Error::Enum parseOmafEquirectRegionMetadata(const MP4VR::RegionWisePackingProperty& aRwpk, BasicSourceInfo& aBasicSourceInfo);

        const CoreProviderSources& getVideoSources();
        const CoreProviderSourceTypes& getVideoSourceTypes();

        bool_t setVideoMetadata(BasicSourceInfo sourceInfo, sourceid_t& sourceId, const VideoInfo& stream);

        bool_t setVideoMetadataPackageMono(sourceid_t sourceId, const VideoInfo& stream, SourceType::Enum sourceType);

        bool_t setVideoMetadataPackageStereo(sourceid_t sourceIdLeft, sourceid_t sourceIdRight, const VideoInfo& leftStream, const VideoInfo& rightStream, BasicSourceInfo sourceInfo);
        bool_t setVideoMetadataPackage1ChStereo(sourceid_t sourceId, const VideoInfo& stream, SourcePosition::Enum position, SourceType::Enum sourceType);

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
        bool_t setVideoMetadataPackageEquirectTile(sourceid_t& sourceId, SourceDirection::Enum sourceDirection, StereoRole::Enum stereoRole, const VASTileViewport& viewport, const VideoInfo& stream);
#endif
        bool_t forceVideoTo(SourcePosition::Enum aPosition);

        bool_t setRotation(const Rotation& aRotation);

    private:
        void_t addVideoSource(CoreProviderSource* source);

        static void_t parseCubemapFaceInfo(const PathName& pathName, size_t pos, size_t len, uint32_t &faceOrder, uint32_t &faceOrientation);

        bool_t setVideoMetadataPackageMonoCubemap(sourceid_t sourceId, const VideoInfo& stream, BasicSourceInfo sourceInfo);

        bool_t setVideoMetadataPackageStereoCubemap(sourceid_t sourceIdLeft, sourceid_t sourceIdRight, const VideoInfo& leftStream, const VideoInfo& rightStream, BasicSourceInfo sourceInfo);

        bool_t setVideoMetadataPackageMultiResMonoCubemap(sourceid_t& sourceId, const VideoInfo& stream, BasicSourceInfo sourceInfo);
        bool_t setVideoMetadataPackageMultiResStereoCubemap(sourceid_t& sourceIdLeft, const VideoInfo& leftStream, const VideoInfo& rightStream, BasicSourceInfo sourceInfo);

        bool_t setVideoMetadataPackageErpRegions(sourceid_t& aSourceId, const VideoInfo& aStream, BasicSourceInfo aSourceInfo);

    private:

        CoreProviderSources mVideoSources;
        CoreProviderSources mInactiveVideoSources;
        CoreProviderSourceTypes mVideoSourceTypes;
    };
OMAF_NS_END
