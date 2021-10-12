
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
#include "Platform/OMAFDataTypes.h"

#include <reader/mp4vrfiledatatypes.h>

#include "Foundation/NVRPathName.h"
#include "Provider/NVRCoreProvider.h"
#include "VideoDecoder/NVRVideoDecoderConfig.h"
#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
#include "DashProvider/NVRMPDAttributes.h"
#endif
OMAF_NS_BEGIN

class MP4MediaStream;

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
    Error::Enum parseOmafCubemapRegionMetadata(const MP4VR::RegionWisePackingProperty& aRwpk,
                                               BasicSourceInfo& aBasicSourceInfo);
    Error::Enum parseOmafEquirectRegionMetadata(const MP4VR::RegionWisePackingProperty& aRwpk,
                                                BasicSourceInfo& aBasicSourceInfo);

    const CoreProviderSources& getVideoSources();
    CoreProviderSources getVideoSources(streamid_t aStreamId);
    const CoreProviderSourceTypes& getVideoSourceTypes();

    bool_t setVideoMetadata(BasicSourceInfo sourceInfo, sourceid_t& sourceId, const VideoInfo& stream);

    bool_t setVideoMetadataPackageMono(sourceid_t sourceId, const VideoInfo& stream, SourceType::Enum sourceType);

    bool_t setVideoMetadataPackageStereo(sourceid_t sourceIdLeft,
                                         sourceid_t sourceIdRight,
                                         const VideoInfo& leftStream,
                                         const VideoInfo& rightStream,
                                         BasicSourceInfo sourceInfo);

    bool_t setVideoMetadataPackage1ChStereo(sourceid_t sourceId,
                                            const VideoInfo& stream,
                                            SourcePosition::Enum position,
                                            SourceType::Enum sourceType);

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
    bool_t setVideoMetadataPackageEquirectTile(sourceid_t& sourceId,
                                               SourceDirection::Enum sourceDirection,
                                               StereoRole::Enum stereoRole,
                                               const VASTileViewport& viewport,
                                               const VideoInfo& stream);
#endif

    /**
     * Set initial viewing orientation metadata
     */
    bool_t updateInitialViewingOrientationMetadata(const MP4MediaStream& refStream,
                                                   MP4VR::InitialViewingOrientationSample& invo);

    /**
     * Set initial overlay configuration for (video) track.
     *
     * @param refStream Video track's stream whom ovly configuration is being set. Configuration may be found from
     * either povd's or visual sample entry's ovly box.
     */
    bool_t setInitialOverlayMetadata(const MP4MediaStream& refStream, MP4VR::OverlayConfigProperty& ovly);

    /**
     * Dynamically control, which overlays will be active / parameters changed etc.
     *
     * This overrides all previous overlay changes made to this video track. So
     * changes are not tracked incrementally.
     *
     * @param refStreamId Stream id of video track source whose overlay data is being updated
     * @param refTrackConfig Original overlay configuration from referred track.
     * @param dyolConfig Updates which are applied to top of tracks original overlay configuration
     *                   which are described in metadata track's sample descriptor.
     * @param dyolSample Updates to the current overlay information.
     */
    bool_t updateOverlayMetadata(const MP4MediaStream& refStream,
                                 MP4VR::OverlayConfigProperty* refTrackConfig,
                                 MP4VR::OverlayConfigProperty* dyolConfig,
                                 MP4VR::OverlayConfigSample& ovlySample);

    bool_t forceVideoTo(SourcePosition::Enum aPosition);

    bool_t setRotation(const Rotation& aRotation);

private:
    void_t addVideoSource(CoreProviderSource* source);

    static void_t parseCubemapFaceInfo(const PathName& pathName,
                                       size_t pos,
                                       size_t len,
                                       uint32_t& faceOrder,
                                       uint32_t& faceOrientation);

    bool_t setVideoMetadataPackageMonoCubemap(sourceid_t sourceId, const VideoInfo& stream, BasicSourceInfo sourceInfo);

    bool_t setVideoMetadataPackageStereoCubemap(sourceid_t sourceIdLeft,
                                                sourceid_t sourceIdRight,
                                                const VideoInfo& leftStream,
                                                const VideoInfo& rightStream,
                                                BasicSourceInfo sourceInfo);

    bool_t setVideoMetadataPackageMultiResMonoCubemap(sourceid_t& sourceId,
                                                      const VideoInfo& stream,
                                                      BasicSourceInfo sourceInfo);
    bool_t setVideoMetadataPackageMultiResStereoCubemap(sourceid_t& sourceIdLeft,
                                                        const VideoInfo& leftStream,
                                                        const VideoInfo& rightStream,
                                                        BasicSourceInfo sourceInfo);

    bool_t setVideoMetadataPackageErpRegions(sourceid_t& aSourceId,
                                             const VideoInfo& aStream,
                                             BasicSourceInfo aSourceInfo);

    /**
     * Get / create overlay source for given visual media stream
     */
    OverlaySource* getOverlaySource(const MP4MediaStream& visualMediaStream, uint16_t overlayId);

    /**
     * Reads attributes from overlayParamteres and updates common source base
     * properties to match updated information
     */
    void_t updateOverlaySourceBaseProperties(OverlaySource* source);

private:
    CoreProviderSources mVideoSources;
    CoreProviderSources mInactiveVideoSources;
    CoreProviderSourceTypes mVideoSourceTypes;

    static std::vector<std::string> mAvailableOverlayGroups;

    typedef std::map<uint16_t, OverlaySource*> OverlaySourceMap;
    std::map<uint32_t, OverlaySourceMap> mOverlays;
};
OMAF_NS_END
