
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
#include "Metadata/NVRMetadataParser.h"
#include "Media/NVRMP4VideoStream.h"
#include "Media/NVRMediaFormat.h"
#include "Metadata/NVRCubemapDefinitions.h"
#include "Metadata/NVROmafMetadataHelper.h"
#include "Provider/NVRCoreProviderSources.h"

#include "Foundation/NVRLogger.h"

#include "Math/OMAFMathFunctions.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(MetadataParser)

std::vector<std::string> MetadataParser::mAvailableOverlayGroups;

/**
 * Merge 2 overlay configurations together and return true if everything went fine.
 *
 * Returns false for example if base config does not have something that is later on
 * modified in addl or overlayIds doesn't match etc.
 *
 * If base and addl are the same struct function only does check that it doesn't relay
 * on any inherited values.
 */
bool_t mergeConfig(MP4VR::SingleOverlayProperty& result,
                   MP4VR::SingleOverlayProperty* base,
                   MP4VR::SingleOverlayProperty* addl)
{
    // overlay IDs must match
    if (result.overlayId != base->overlayId || result.overlayId != addl->overlayId)
    {
        return false;
    }

    auto merge = [](auto& res, auto& base, auto& addl) {

		
		//       because setting struct[4] must be possible and it has natural byte_count 0..
        if (addl.inheritFromOverlayConfigSampleEntry && base.inheritFromOverlayConfigSampleEntry)
        {
            return false;
        }
        else if (addl.inheritFromOverlayConfigSampleEntry)
        {
            res = base;
        }
        else
        {
            res = addl;
        }
        return true;
    };

	

    // merge every overlay control struct to result
    if (!merge(result.overlayInteraction, base->overlayInteraction, addl->overlayInteraction))
    {
        return false;
    }

    if (!merge(result.overlayLabel, base->overlayLabel, addl->overlayLabel))
    {
        return false;
    }

    if (!merge(result.overlayLayeringOrder, base->overlayLayeringOrder, addl->overlayLayeringOrder))
    {
        return false;
    }

    if (!merge(result.overlayOpacity, base->overlayOpacity, addl->overlayOpacity))
    {
        return false;
    }

    if (!merge(result.overlayPriority, base->overlayPriority, addl->overlayPriority))
    {
        return false;
    }

    if (!merge(result.overlaySourceRegion, base->overlaySourceRegion, addl->overlaySourceRegion))
    {
        return false;
    }

    if (!merge(result.recommendedViewportOverlay, base->recommendedViewportOverlay, addl->recommendedViewportOverlay))
    {
        return false;
    }

    if (!merge(result.sphereRelative2DOverlay, base->sphereRelative2DOverlay, addl->sphereRelative2DOverlay))
    {
        return false;
    }

    if (!merge(result.sphereRelativeOmniOverlay, base->sphereRelativeOmniOverlay, addl->sphereRelativeOmniOverlay))
    {
        return false;
    }

    if (!merge(result.viewportRelativeOverlay, base->viewportRelativeOverlay, addl->viewportRelativeOverlay))
    {
        return false;
    }

	if (!merge(result.associatedSphereRegion, base->associatedSphereRegion, addl->associatedSphereRegion))
    {
        return false;
    }

	if (!merge(result.overlayAlphaCompositing, base->overlayAlphaCompositing, addl->overlayAlphaCompositing))
    {
        return false;
    }

    return true;
}


bool_t mergeOverlayConfigs(MP4VR::SingleOverlayProperty& result,
                           MP4VR::SingleOverlayProperty* dyolOrOvlyTrackConfig,
                           MP4VR::SingleOverlayProperty* ovlySample)
{
    if (ovlySample)
    {		
        return mergeConfig(result, dyolOrOvlyTrackConfig, ovlySample);
    }        
    else
    {
		// just read from dyol sample entry
        return mergeConfig(result, dyolOrOvlyTrackConfig, dyolOrOvlyTrackConfig);
    }
}

/**
 * Finds corresponding overlay by ID from various configurations and merge all data to resulting overlay parameters
 */
bool_t mergeOverlayConfigs(MP4VR::SingleOverlayProperty& result,
                           MP4VR::OverlayConfigProperty* dyolConfig,
                           MP4VR::SingleOverlayProperty* ovlySample)
{
    // helper to find overlay with id from an array
    auto find = [](auto overlayId, auto& overlays) {
        for (auto& ovly : overlays)
        {
            if (ovly.overlayId == overlayId)
            {
                return &ovly;
            }
        }

        return (MP4VR::SingleOverlayProperty*) OMAF_NULL;
    };

    MP4VR::SingleOverlayProperty* dyol = find(result.overlayId, dyolConfig->overlays);
    return mergeOverlayConfigs(result, dyol, ovlySample);
}

MetadataParser::MetadataParser()
{
}

MetadataParser::~MetadataParser()
{
    reset();
}

void MetadataParser::reset()
{
    OMAF_LOG_D("Metadata Parser: %d RESETTING video sources", this);

    for (CoreProviderSources::Iterator it = mVideoSources.begin(); it != mVideoSources.end(); ++it)
    {
        OMAF_DELETE_HEAP(*it);
    }
    mVideoSources.clear();
    for (CoreProviderSources::Iterator it = mInactiveVideoSources.begin(); it != mInactiveVideoSources.end(); ++it)
    {
        OMAF_DELETE_HEAP(*it);
    }
    mInactiveVideoSources.clear();
}

const CoreProviderSourceTypes& MetadataParser::getVideoSourceTypes()
{
    return mVideoSourceTypes;
}

const CoreProviderSources& MetadataParser::getVideoSources()
{
    return mVideoSources;
}

CoreProviderSources MetadataParser::getVideoSources(streamid_t aStreamId)
{
    CoreProviderSources sources;
    for (CoreProviderSources::Iterator it = mVideoSources.begin(); it != mVideoSources.end(); ++it)
    {
        VideoFrameSource* vSrc = (VideoFrameSource*) (*it);

        if (vSrc->streamIndex == aStreamId)
        {
            sources.add(*it);
        }
    }
    return sources;
}

void_t MetadataParser::addVideoSource(CoreProviderSource* source)
{
    OMAF_LOG_D("Adding video source sourceId: %d type: %d to metadata parser: %d", source->sourceId, source->type, this);
    mVideoSources.add(source);
    bool_t found = false;
    // Check if this type has already added and add it if not
    for (CoreProviderSourceTypes::Iterator it = mVideoSourceTypes.begin(); it != mVideoSourceTypes.end(); ++it)
    {
        if ((*it) == source->type)
        {
            found = true;
        }
    }
    if (!found)
    {
        mVideoSourceTypes.add(source->type);
    }
}

void_t MetadataParser::parseCubemapFaceInfo(const PathName& pathName,
                                            size_t pos,
                                            size_t len,
                                            uint32_t& faceOrder,
                                            uint32_t& faceOrientation)
{
    faceOrder = 0;
    faceOrientation = 0;
    if (pos != pathName.getLength())
    {
        // 0 - front 1 - left 2 - back 3 - right 4 - top 5 - bottom
        const char order[NUM_FACES] = {'F', 'L', 'B', 'R', 'U', 'D'};

        for (int i = 0; i < len - pos; ++i)
        {
            const char c = *pathName.at(pos + i);
            bool found = false;
            for (int j = 0; j < NUM_FACES; ++j)
            {
                if (order[j] == c)
                {
                    faceOrder |= (j << (i * BITS_FOR_FACE_ORDER));  // 3 bits to present 0-5
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                faceOrder = DEFAULT_FACE_ORDER;  // RLUDFB
                break;
            }
        }
    }
    else
    {
        faceOrder = DEFAULT_FACE_ORDER;
    }

    pos += NUM_FACES + 1;
    len = OMAF::min(pathName.getLength(), pos + NUM_FACES);
    if (len != pathName.getLength())
    {
        for (int i = 0; i < len - pos; ++i)
        {
            const char c = *pathName.at(pos + i);
            bool found = false;
            if (c >= '0' && c <= '7')  // 4 possible rotations and their mirrored versions
            {
                int oriIndex = (c - '0');
                faceOrientation |= (oriIndex)
                    << ((BITS_FOR_FACE_ORIENTATION * i));  // face_order_bits + 3 bits per rotation (0-7)
                found = true;
            }
            if (!found)
            {
                faceOrientation = 0;
                break;
            }
        }
    }
}

/*
 *  Map regions from packed to projected domain.
 *  The projected domain is equirectangular projected 360 video.
 *  Packed domain is the actual video content from decoder's output.
 *  Note! No transformations are supported. The regions can have packing in size, but no rotation etc.
 */
Error::Enum MetadataParser::parseOmafEquirectRegionMetadata(const MP4VR::RegionWisePackingProperty& aRwpk,
                                                            BasicSourceInfo& aBasicSourceInfo)
{
    return OmafMetadata::parseOmafEquirectRegions(aRwpk, aBasicSourceInfo.erpRegions, aBasicSourceInfo.sourceDirection);
}


/*
 *  Map regions from packed to projected domain.
 *  The projected domain should match with OMAF default cubemap layout.
 *  Packed domain is the actual video content from decoder's output.
 *  Note! This currently supports only cubemap to cubemap mapping; other mappings would require changes up to the
 * renderer level too.
 *  Note2: Framepacked stereo must have the same packing for both channels, with or without constituent picture
 * matching flag set
 */
Error::Enum MetadataParser::parseOmafCubemapRegionMetadata(const MP4VR::RegionWisePackingProperty& aRwpk,
                                                           BasicSourceInfo& aBasicSourceInfo)
{
    return OmafMetadata::parseOmafCubemapRegions(aRwpk, aBasicSourceInfo.sourceDirection,
                                                 aBasicSourceInfo.tiledCubeMap);
}

bool_t MetadataParser::parseUri(const char_t* uri, BasicSourceInfo& sourceInfo)
{

	
    sourceInfo.sourceDirection = SourceDirection::INVALID;
    sourceInfo.sourceType = SourceType::INVALID;

    const PathName& pathName = uri;

    if (pathName.findFirst("_M.", 0) != Npos || pathName.findFirst("_M_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::MONO;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
    else if (pathName.findFirst("_TB.", 0) != Npos || pathName.findFirst("_TB_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::TOP_BOTTOM;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
    else if (pathName.findFirst("_BT.", 0) != Npos || pathName.findFirst("_BT_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::BOTTOM_TOP;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
    else if (pathName.findFirst("_LR.", 0) != Npos || pathName.findFirst("_LR_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::LEFT_RIGHT;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
    else if (pathName.findFirst("_RL.", 0) != Npos || pathName.findFirst("_RL_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::RIGHT_LEFT;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
    else if (pathName.findFirst("_UD.", 0) != Npos || pathName.findFirst("_UD_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::TOP_BOTTOM;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
    else if (pathName.findFirst("_MC.", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::MONO;
        sourceInfo.sourceType = SourceType::CUBEMAP;
        sourceInfo.faceOrder = DEFAULT_FACE_ORDER;
        sourceInfo.faceOrientation = 0;
    }
    else if (pathName.findFirst("_MC_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::MONO;
        sourceInfo.sourceType = SourceType::CUBEMAP;
        size_t pos = pathName.findFirst("_MC_", 0) + 4;
        size_t len = OMAF::min(pathName.getLength(), pos + NUM_FACES);
        parseCubemapFaceInfo(pathName, pos, len, sourceInfo.faceOrder, sourceInfo.faceOrientation);
    }
    else if (pathName.findFirst("_UDC.", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::TOP_BOTTOM;
        sourceInfo.sourceType = SourceType::CUBEMAP;
        sourceInfo.faceOrder = DEFAULT_FACE_ORDER;
        sourceInfo.faceOrientation = 0;
    }
    else if (pathName.findFirst("_UDC_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::TOP_BOTTOM;
        sourceInfo.sourceType = SourceType::CUBEMAP;
        size_t pos = pathName.findFirst("_UDC_", 0) + 5;
        size_t len = OMAF::min(pathName.getLength(), pos + NUM_FACES);
        parseCubemapFaceInfo(pathName, pos, len, sourceInfo.faceOrder, sourceInfo.faceOrientation);
    }
    else if (pathName.findFirst("_ID.", 0) != Npos || pathName.findFirst("_ID_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::MONO;
        sourceInfo.sourceType = SourceType::IDENTITY;
    }
    else if (pathName.findFirst("_2LR.", 0) != Npos || pathName.findFirst("_2LR_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::DUAL_TRACK;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
    else if (pathName.findFirst("_S180.", 0) != Npos || pathName.findFirst("_S180_", 0) != Npos)
    {
        sourceInfo.sourceDirection = SourceDirection::LEFT_RIGHT;
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_180;
    }
    return (sourceInfo.sourceDirection != SourceDirection::INVALID && sourceInfo.sourceType != SourceType::INVALID);
}

bool_t MetadataParser::setVideoMetadata(BasicSourceInfo sourceInfo, sourceid_t& sourceId, const VideoInfo& stream)
{
    if (sourceInfo.sourceType == SourceType::IDENTITY)
    {
        return setVideoMetadataPackageMono(sourceId, stream, sourceInfo.sourceType);
    }

    if (sourceInfo.sourceType == SourceType::CUBEMAP)
    {
        if (sourceInfo.tiledCubeMap.cubeNumFaces == 0)
        {
            if (sourceInfo.sourceDirection == SourceDirection::MONO)
            {
                return setVideoMetadataPackageMonoCubemap(sourceId++, stream, sourceInfo);
            }
            else
            {
                return setVideoMetadataPackageStereoCubemap(sourceId++, sourceId++, stream, stream, sourceInfo);
            }
        }
        else
        {
            if (sourceInfo.sourceDirection == SourceDirection::MONO)
            {
                return setVideoMetadataPackageMultiResMonoCubemap(sourceId, stream, sourceInfo);
            }
            else
            {
                return setVideoMetadataPackageMultiResStereoCubemap(sourceId, stream, stream, sourceInfo);
            }
        }
    }


    if (sourceInfo.sourceType != SourceType::EQUIRECTANGULAR_PANORAMA &&
        sourceInfo.sourceType != SourceType::EQUIRECTANGULAR_TILES &&
        sourceInfo.sourceType != SourceType::EQUIRECTANGULAR_180)
    {
        OMAF_ASSERT(false, "Unsupported combination!");
        return false;
    }

    if (!sourceInfo.erpRegions.isEmpty())
    {
        return setVideoMetadataPackageErpRegions(sourceId, stream, sourceInfo);
    }
    else if (sourceInfo.sourceDirection == SourceDirection::MONO)
    {
        return setVideoMetadataPackageMono(sourceId, stream, sourceInfo.sourceType);
    }

    sourceid_t leftSource = sourceId++;
    sourceid_t rightSource = sourceId++;

    if (sourceInfo.sourceDirection == SourceDirection::BOTTOM_TOP ||
        sourceInfo.sourceDirection == SourceDirection::RIGHT_LEFT)
    {
        leftSource = rightSource;
        rightSource--;
    }

    return setVideoMetadataPackageStereo(leftSource, rightSource, stream, stream, sourceInfo);
}

bool_t MetadataParser::updateInitialViewingOrientationMetadata(const MP4MediaStream& refStream,
                                                               MP4VR::InitialViewingOrientationSample& invo)
{
    OMAF_LOG_D("INVO: ca: %f, ce: %f ct: %f", float(invo.region.centreAzimuth) / 65536,
               float(invo.region.centreElevation) / 65536, float(invo.region.centreTilt) / 65536);

    // set invo information to every source which is referring to linked  media stream (including overlays)
    for (auto i = 0; i < mVideoSources.getSize(); i++)
    {
        VideoFrameSource* vSrc = (VideoFrameSource*) mVideoSources[i];

        if (vSrc->streamIndex == refStream.getStreamId())
        {
            vSrc->initialViewingOrientation.orientation =
                makeQuaternion(OMAF::toRadians(float(invo.region.centreElevation) / 65536),
                               OMAF::toRadians(-float(invo.region.centreAzimuth) / 65536),
                               OMAF::toRadians(float(invo.region.centreTilt) / 65536), EulerAxisOrder::YXZ);

            vSrc->initialViewingOrientation.valid = true;
        }
    }

    return true;
}

bool_t MetadataParser::setInitialOverlayMetadata(const MP4MediaStream& mediaStream, MP4VR::OverlayConfigProperty& ovly)
{
    // initialize sources for overlays declared in mediaStreams's overlay configuration box
    for (MP4VR::SingleOverlayProperty& singleOverlay : ovly.overlays)
    {
        OverlaySource* ovlySource = getOverlaySource(mediaStream, singleOverlay.overlayId);
        mergeOverlayConfigs(ovlySource->overlayParameters, &singleOverlay, OMAF_NULL);

        // mAvailableOverlayGroups to be able to select between shown groups
        if (ovlySource->overlayParameters.overlayLabel.doesExist)
        {
            auto label = std::string(ovlySource->overlayParameters.overlayLabel.overlayLabel.begin(),
                                     ovlySource->overlayParameters.overlayLabel.overlayLabel.end());
            auto it = std::find(mAvailableOverlayGroups.begin(), mAvailableOverlayGroups.end(), label);
            if (it == mAvailableOverlayGroups.end())
            {
                mAvailableOverlayGroups.push_back(label);
            }
        }

        if (ovlySource->overlayParameters.associatedSphereRegion.doesExist)
        {
            OMAF_LOG_D("Overlay %u has associated sphere region so it is not active by default", ovlySource->overlayParameters.overlayId);
            ovlySource->userActivatable = true;
            ovlySource->active = false;
        }

        updateOverlaySourceBaseProperties(ovlySource);
    }

    return true;
}

bool_t MetadataParser::updateOverlayMetadata(const MP4MediaStream& refStream,
                                             MP4VR::OverlayConfigProperty* refTrackConfig,
                                             MP4VR::OverlayConfigProperty* dyolConfig,
                                             MP4VR::OverlayConfigSample& ovlySample)
{
    uint32_t refTrackId = refStream.getTrackId();

	
    auto& overlaysOfTrack = mOverlays[refStream.getTrackId()];

	/**
	 * Overlay config might be found from referred track, dyol config and from overlay sample:
	 * 
	 * 1. If ovlyId is found from sample, then the same ovlyId must be found from dyol sample entry as well
	 * 2. If ovlyId is found from dyol sample entry, but it is not activated in sample, overlay will be set inactive
	 * 3. If ovlyId is found from track's ovly box, but it is not found from dyol sample entry, then it is considered as active overlay
	 * 4. If ovlyId is found from dyol sample entry, then it must be also found from one of the referred tracks too 
	 *    (this is a bit harder to verify here, but on the other hand, player doesn't have to verify it)
	 */


	// deactivate by default overlay sources mentioned in dyolConfig (clause 2)
    for (auto ovly : dyolConfig->overlays)
    {
        auto ovlyId = ovly.overlayId;
        if (overlaysOfTrack.count(ovlyId) > 0)
        {
            overlaysOfTrack[ovlyId]->active = false;
        }
    }

	// activate all the overlays listed in sample's overlay activations 
    for (uint16_t ovlyId : ovlySample.activeOverlayIds)
    {
        // if this overlay source doesn't contain the overlay, don't try to update its properties
        if (overlaysOfTrack.count(ovlyId) == 0)
        {
            continue;
        }

        OverlaySource* ovlySource = getOverlaySource(refStream, ovlyId);
        OMAF_LOG_D("Activating overlay %hu, group %d", ovlyId, ovlySource->sourceGroup);
        ovlySource->active = true;
	
		mergeOverlayConfigs(ovlySource->overlayParameters, dyolConfig, OMAF_NULL);
        updateOverlaySourceBaseProperties(ovlySource);
    }

    // additional activations with possible parameter updates
    if (ovlySample.addlActiveOverlays)
    {
        for (MP4VR::SingleOverlayProperty& addlActiveOvly : ovlySample.addlActiveOverlays->overlays)
        {
            // if this overlay source doesn't contain the overlay, don't try to update its properties
            if (overlaysOfTrack.count(addlActiveOvly.overlayId) == 0)
            {
                continue;
            }

			OverlaySource* ovlySource = getOverlaySource(refStream, addlActiveOvly.overlayId);
            OMAF_LOG_D("Activating and setting parameters for overlay %hu, group %d", addlActiveOvly.overlayId,
                       ovlySource->sourceGroup);
            ovlySource->active = true;

            // activation + inline overrides for overlay control struct values
            mergeOverlayConfigs(ovlySource->overlayParameters, dyolConfig, &addlActiveOvly);
            updateOverlaySourceBaseProperties(ovlySource);
        }
    }

    return true;
}

OverlaySource* MetadataParser::getOverlaySource(const MP4MediaStream& visualMediaStream, uint16_t overlayId)
{
    static uint32_t sourceIdCounter = 2847929;

    auto& overlaysOfTrack = mOverlays[visualMediaStream.getTrackId()];

    if (overlaysOfTrack.count(overlayId) == 0)
    {
        OMAF_LOG_D("** metaparser: %d, StreamId: %d Creating overlay source for overlayId: %d", this, visualMediaStream.getStreamId(), overlayId);
        OverlaySource* source = OMAF_NEW_HEAP(OverlaySource);
        overlaysOfTrack[overlayId] = source;

        // select media source for overlay
        VideoFrameSource* videoSource = OMAF_NULL;
        OMAF_ASSERT(!mVideoSources.isEmpty(), "No video source!");

        OMAF_LOG_D("** metaparser: %d, Finding mVideoSources where streamIndex == StreamId: %d", this, visualMediaStream.getStreamId());
        for (CoreProviderSource* cpSource : mVideoSources)
        {
            auto vSource = (VideoFrameSource*) cpSource;
            OMAF_LOG_D("** Checking vSource: %d sourceId: %d, streamIndex: %d type: %d", vSource, vSource->sourceId,
                       vSource->streamIndex, vSource->type);

			
            if (vSource->streamIndex == visualMediaStream.getStreamId())
            {
                videoSource = vSource;
                break;
            }
        }
        if (videoSource == OMAF_NULL)
        {
            return OMAF_NULL;
        }

        videoSource->omitRender = true;  // currently video track
        source->mediaSource = videoSource;
        source->sourceId = sourceIdCounter++;
        source->streamIndex = source->mediaSource->streamIndex;
        source->active = true;

        // should be read from video tracks povd boxes... SPEC 7.13.2.3 is still a bit unclear about it.
        source->sourcePosition = SourcePosition::MONO;

        source->overlayParameters.overlayId = overlayId;
        OMAF_LOG_D(
            "*** Initialized OverlaySource for overlayId: %d with sourceId: %d, streamIndex: %d type: %d mediaSource: %d",
            source->overlayParameters.overlayId, source->sourceId, source->streamIndex, source->type,
            source->mediaSource);
		
		addVideoSource(source);
    }

    return overlaysOfTrack[overlayId];
}


bool_t MetadataParser::setVideoMetadataPackageMono(sourceid_t sourceId,
                                                   const VideoInfo& stream,
                                                   SourceType::Enum sourceType)
{
    OMAF_ASSERT(stream.streamId != OMAF_UINT8_MAX, "Stream ID not set");
    PanoramaSource* source;
    switch (sourceType)
    {
    case SourceType::EQUIRECTANGULAR_PANORAMA:
        source = OMAF_NEW_HEAP(EquirectangularSource);
        break;
    case SourceType::EQUIRECTANGULAR_TILES:
        EquirectangularTileSource* src;
        src = OMAF_NEW_HEAP(EquirectangularTileSource);
        src->centerLatitude = 0.0f;
        src->centerLongitude = 0.0f;
        src->spanLatitude = 180.0f;
        src->spanLongitude = 180.0f;
        source = src;
        break;
    case SourceType::IDENTITY:
        source = OMAF_NEW_HEAP(IdentitySource);
        break;
    default:
        OMAF_ASSERT(false, "Invalid source type");
        return false;
    }

    source->sourceId = sourceId;
    source->sourcePosition = SourcePosition::MONO;

    source->textureRect.x = 0.0f;
    source->textureRect.y = 0.0f;
    source->textureRect.w = 1.0f;
    source->textureRect.h = 1.0f;
    source->widthInPixels = stream.width;
    source->heightInPixels = stream.height;
    source->streamIndex = stream.streamId;

    addVideoSource(source);
    return true;
}

bool_t MetadataParser::setVideoMetadataPackageStereo(sourceid_t sourceIdLeft,
                                                     sourceid_t sourceIdRight,
                                                     const VideoInfo& leftStream,
                                                     const VideoInfo& rightStream,
                                                     BasicSourceInfo data)
{
    OMAF_ASSERT(leftStream.streamId != OMAF_UINT8_MAX, "Stream ID not set");
    OMAF_ASSERT(rightStream.streamId != OMAF_UINT8_MAX, "Stream ID not set");
    PanoramaSource* leftSource;
    PanoramaSource* rightSource;
    switch (data.sourceType)
    {
    case SourceType::EQUIRECTANGULAR_PANORAMA:
        leftSource = OMAF_NEW_HEAP(EquirectangularSource);
        rightSource = OMAF_NEW_HEAP(EquirectangularSource);
        break;
    case SourceType::EQUIRECTANGULAR_180:
        Equirectangular180Source* l;
        l = OMAF_NEW_HEAP(Equirectangular180Source);
        leftSource = l;

        Equirectangular180Source* r;
        r = OMAF_NEW_HEAP(Equirectangular180Source);
        rightSource = r;
        break;
    case SourceType::IDENTITY:
        leftSource = OMAF_NEW_HEAP(IdentitySource);
        rightSource = OMAF_NEW_HEAP(IdentitySource);
        break;
    default:
        OMAF_ASSERT(false, "Invalid source type");
        return false;
    }

    leftSource->sourceId = sourceIdLeft;
    rightSource->sourceId = sourceIdRight;

    leftSource->sourcePosition = SourcePosition::LEFT;
    rightSource->sourcePosition = SourcePosition::RIGHT;

    uint32_t frameWidth = leftStream.width;
    uint32_t frameHeight = leftStream.height;
    OMAF_ASSERT(leftStream.width == rightStream.width, "Stream widths must match");
    OMAF_ASSERT(leftStream.height == rightStream.height, "Stream heights must match");
    switch (data.sourceDirection)
    {
    case SourceDirection::LEFT_RIGHT:

        leftSource->textureRect.x = 0.0f;
        leftSource->textureRect.y = 0.0f;
        leftSource->textureRect.w = 0.5f;
        leftSource->textureRect.h = 1.0f;
        leftSource->widthInPixels = frameWidth / 2;
        leftSource->heightInPixels = frameHeight;

        rightSource->textureRect.x = 0.5f;
        rightSource->textureRect.y = 0.0f;
        rightSource->textureRect.w = 0.5f;
        rightSource->textureRect.h = 1.0f;
        rightSource->widthInPixels = frameWidth / 2;
        rightSource->heightInPixels = frameHeight;
        break;

    case SourceDirection::RIGHT_LEFT:

        leftSource->textureRect.x = 0.5f;
        leftSource->textureRect.y = 0.0f;
        leftSource->textureRect.w = 0.5f;
        leftSource->textureRect.h = 1.0f;
        leftSource->widthInPixels = frameWidth / 2;
        leftSource->heightInPixels = frameHeight;

        rightSource->textureRect.x = 0.0f;
        rightSource->textureRect.y = 0.0f;
        rightSource->textureRect.w = 0.5f;
        rightSource->textureRect.h = 1.0f;
        rightSource->widthInPixels = frameWidth / 2;
        rightSource->heightInPixels = frameHeight;
        break;

    case SourceDirection::TOP_BOTTOM:

        leftSource->textureRect.x = 0.0f;
        leftSource->textureRect.y = 0.5f;
        leftSource->textureRect.w = 1.0f;
        leftSource->textureRect.h = 0.5f;
        leftSource->widthInPixels = frameWidth;
        leftSource->heightInPixels = frameHeight / 2;

        rightSource->textureRect.x = 0.0f;
        rightSource->textureRect.y = 0.0f;
        rightSource->textureRect.w = 1.0f;
        rightSource->textureRect.h = 0.5f;
        rightSource->widthInPixels = frameWidth;
        rightSource->heightInPixels = frameHeight / 2;
        break;

    case SourceDirection::BOTTOM_TOP:

        leftSource->textureRect.x = 0.0f;
        leftSource->textureRect.y = 0.0f;
        leftSource->textureRect.w = 1.0f;
        leftSource->textureRect.h = 0.5f;
        leftSource->widthInPixels = frameWidth;
        leftSource->heightInPixels = frameHeight / 2;

        rightSource->textureRect.x = 0.0f;
        rightSource->textureRect.y = 0.5f;
        rightSource->textureRect.w = 1.0f;
        rightSource->textureRect.h = 0.5f;
        rightSource->widthInPixels = frameWidth;
        rightSource->heightInPixels = frameHeight / 2;
        break;

    case SourceDirection::DUAL_TRACK:

        leftSource->textureRect.x = 0.0f;
        leftSource->textureRect.y = 0.0f;
        leftSource->textureRect.w = 1.0f;
        leftSource->textureRect.h = 1.0f;
        leftSource->widthInPixels = frameWidth;
        leftSource->heightInPixels = frameHeight;

        rightSource->textureRect.x = 0.0f;
        rightSource->textureRect.y = 0.0f;
        rightSource->textureRect.w = 1.0f;
        rightSource->textureRect.h = 1.0f;
        rightSource->widthInPixels = frameWidth;
        rightSource->heightInPixels = frameHeight;
        break;

    default:
        OMAF_ASSERT(false, "");
    }
    rightSource->streamIndex = rightStream.streamId;
    leftSource->streamIndex = leftStream.streamId;
    addVideoSource(leftSource);
    addVideoSource(rightSource);
    return true;
}

bool_t MetadataParser::setVideoMetadataPackage1ChStereo(sourceid_t sourceId,
                                                        const VideoInfo& stream,
                                                        SourcePosition::Enum position,
                                                        SourceType::Enum sourceType)
{
    PanoramaSource* source;
    switch (sourceType)
    {
    case SourceType::EQUIRECTANGULAR_PANORAMA:
        source = OMAF_NEW_HEAP(EquirectangularSource);
        break;
    case SourceType::IDENTITY:
        source = OMAF_NEW_HEAP(IdentitySource);
        break;
    default:
        OMAF_ASSERT(false, "Invalid source type");
        return false;
    }

    source->sourceId = sourceId;

    source->sourcePosition = position;

    uint32_t frameWidth = stream.width;
    uint32_t frameHeight = stream.height;

    source->textureRect.x = 0.0f;
    source->textureRect.y = 0.0f;
    source->textureRect.w = 1.0f;
    source->textureRect.h = 1.0f;
    source->widthInPixels = frameWidth;
    source->heightInPixels = frameHeight;

    source->streamIndex = stream.streamId;
    addVideoSource(source);
    return true;
}

bool_t MetadataParser::setVideoMetadataPackageMonoCubemap(sourceid_t sourceId,
                                                          const VideoInfo& stream,
                                                          BasicSourceInfo data)
{
    OMAF_ASSERT(stream.streamId != OMAF_UINT8_MAX, "Stream ID not set");
    OMAF_LOG_D("Setting to mono cubemap");
    const int NUM_FACES = 6;

    CubeMapSource* cubeMapSource = OMAF_NEW_HEAP(CubeMapSource);
    cubeMapSource->sourcePosition = SourcePosition::MONO;
    cubeMapSource->sourceId = sourceId;

    cubeMapSource->cubeMap.cubeYaw = 0;
    cubeMapSource->cubeMap.cubePitch = 0;
    cubeMapSource->cubeMap.cubeRoll = 0;
    cubeMapSource->cubeMap.cubeNumFaces = NUM_FACES;

    float width = 1.f;
    float height = 1.f;
    const float oneThird = 1.f / 3.f;
    const float oneHalf = 1.f / 2.f;

    // 0 - front
    // 1 - left
    // 2 - back
    // 3 - right
    // 4 - up
    // 5 - down
    // int faceOrder[6] = { 1, 0, 3, 4, 5, 2 };
    int faceOrder[NUM_FACES] = {3, 1, 4, 5, 0, 2};
    CubeFaceSectionOrientation::Enum faceRotation[NUM_FACES] = {
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION};
    const int BITS_FOR_FACE_ORDER = 3;
    const int BITS_FOR_FACE_ORIENTATION = 3;
    const int TOTAL_BITS_NEEDED = (BITS_FOR_FACE_ORDER + BITS_FOR_FACE_ORIENTATION) * NUM_FACES;


    for (int i = 0; i < NUM_FACES; ++i)
    {
        faceOrder[i] = (((size_t) data.faceOrder) >> (i * BITS_FOR_FACE_ORDER)) & 0x7;
    }
    for (int i = 0; i < NUM_FACES; ++i)
    {
        int value = (((size_t) data.faceOrientation) >> (i * BITS_FOR_FACE_ORIENTATION)) & 0x7;
        faceRotation[i] = (CubeFaceSectionOrientation::Enum)((value == 0) ? 0 : (1 << value - 1));  // take the 0 out
    }

    for (int i = 0; i < NUM_FACES; ++i)
    {
        cubeMapSource->cubeMap.cubeFaces[i].faceIndex = faceOrder[i];
        cubeMapSource->cubeMap.cubeFaces[i].numCoordinates = 1;
        for (int j = 0; j < cubeMapSource->cubeMap.cubeFaces[i].numCoordinates; ++j)
        {
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceX =
                (i % 3) * (width / 3);  // normalized tile position in source texture
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceY = (i / 3) * (height / 2);
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceWidth =
                width / 3;  // normalized tile size in source texture
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceHeight = height / 2;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].originX =
                0.f;  // normalized tile position inside origin face
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].originY = 0.f;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].originWidth =
                1.f;  // normalized tile size inside origin face
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].originHeight = 1.f;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceOrientation = faceRotation[i];
        }
    }
    cubeMapSource->textureRect.x = 0.0f;
    cubeMapSource->textureRect.y = 0.0f;
    cubeMapSource->textureRect.w = 1.0f;
    cubeMapSource->textureRect.h = 1.0f;
    cubeMapSource->widthInPixels = stream.width;
    cubeMapSource->heightInPixels = stream.height;
    cubeMapSource->streamIndex = stream.streamId;
    addVideoSource(cubeMapSource);
    return true;
}

bool_t MetadataParser::setVideoMetadataPackageStereoCubemap(sourceid_t sourceIdLeft,
                                                            sourceid_t sourceIdRight,
                                                            const VideoInfo& leftStream,
                                                            const VideoInfo& rightStream,
                                                            BasicSourceInfo sourceInfo)
{
    OMAF_LOG_D("Setting up stereo CUBEMAP");
    const int NUM_FACES = 6;

    OMAF_ASSERT(leftStream.streamId != OMAF_UINT8_MAX, "Stream ID not set");
    OMAF_ASSERT(rightStream.streamId != OMAF_UINT8_MAX, "Stream ID not set");
    CubeMapSource* leftSource = OMAF_NEW_HEAP(CubeMapSource);
    CubeMapSource* rightSource = OMAF_NEW_HEAP(CubeMapSource);
    leftSource->sourcePosition = SourcePosition::LEFT;
    rightSource->sourcePosition = SourcePosition::RIGHT;
    leftSource->sourceId = sourceIdLeft;
    rightSource->sourceId = sourceIdRight;

    float32_t width = 0;
    float32_t height = 0;
    float32_t leftOriginX = 0;
    float32_t leftOriginY = 0;
    float32_t rightOriginX = 0;
    float32_t rightOriginY = 0;

    switch (sourceInfo.sourceDirection)
    {
    case SourceDirection::LEFT_RIGHT:
        leftSource->textureRect.x = 0.0f;
        leftSource->textureRect.y = 0.0f;
        leftSource->textureRect.w = 0.5f;
        leftSource->textureRect.h = 1.0f;
        leftSource->widthInPixels = leftStream.width / 2;
        leftSource->heightInPixels = leftStream.height;

        rightSource->textureRect.x = 0.5f;
        rightSource->textureRect.y = 0.0f;
        rightSource->textureRect.w = 0.5f;
        rightSource->textureRect.h = 1.0f;
        rightSource->widthInPixels = rightStream.width / 2;
        rightSource->heightInPixels = rightStream.height;
        break;
    case SourceDirection::RIGHT_LEFT:
        rightSource->textureRect.x = 0.0f;
        rightSource->textureRect.y = 0.0f;
        rightSource->textureRect.w = 0.5f;
        rightSource->textureRect.h = 1.0f;
        rightSource->widthInPixels = rightStream.width / 2;
        rightSource->heightInPixels = rightStream.height;

        leftSource->textureRect.x = 0.5f;
        leftSource->textureRect.y = 0.0f;
        leftSource->textureRect.w = 0.5f;
        leftSource->textureRect.h = 1.0f;
        leftSource->widthInPixels = leftStream.width / 2;
        leftSource->heightInPixels = leftStream.height;
        break;
    case SourceDirection::TOP_BOTTOM:
        leftSource->textureRect.x = 0.0f;
        leftSource->textureRect.y = 0.5f;
        leftSource->textureRect.w = 1.0f;
        leftSource->textureRect.h = 0.5f;
        leftSource->widthInPixels = leftStream.width;
        leftSource->heightInPixels = leftStream.height / 2;

        rightSource->textureRect.x = 0.0f;
        rightSource->textureRect.y = 0.0f;
        rightSource->textureRect.w = 1.0f;
        rightSource->textureRect.h = 0.5f;
        rightSource->widthInPixels = rightStream.width;
        rightSource->heightInPixels = rightStream.height / 2;
        break;
    case SourceDirection::BOTTOM_TOP:
        rightSource->textureRect.x = 0.0f;
        rightSource->textureRect.y = 0.5f;
        rightSource->textureRect.w = 1.0f;
        rightSource->textureRect.h = 0.5f;
        rightSource->widthInPixels = rightStream.width;
        rightSource->heightInPixels = rightStream.height / 2;

        leftSource->textureRect.x = 0.0f;
        leftSource->textureRect.y = 0.0f;
        leftSource->textureRect.w = 1.0f;
        leftSource->textureRect.h = 0.5f;
        leftSource->widthInPixels = leftStream.width;
        leftSource->heightInPixels = leftStream.height / 2;
        break;
    default:
        OMAF_ASSERT(false, "");
    }

    width = 1.f;
    height = 1.0;
    leftOriginX = 0;
    leftOriginY = 0;
    rightOriginX = 0;
    rightOriginY = 0;

    leftSource->cubeMap.cubeYaw = 0;
    leftSource->cubeMap.cubePitch = 0;
    leftSource->cubeMap.cubeRoll = 0;
    leftSource->cubeMap.cubeNumFaces = NUM_FACES;
    rightSource->cubeMap.cubeYaw = 0;
    rightSource->cubeMap.cubePitch = 0;
    rightSource->cubeMap.cubeRoll = 0;
    rightSource->cubeMap.cubeNumFaces = NUM_FACES;

    // 0 - front
    // 1 - left
    // 2 - back
    // 3 - right
    // 4 - up
    // 5 - down
    // int faceOrder[6] = { 1, 0, 3, 4, 5, 2 };
    int faceOrder[NUM_FACES] = {3, 1, 4, 5, 0, 2};
    CubeFaceSectionOrientation::Enum faceRotation[NUM_FACES] = {
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION,
        CubeFaceSectionOrientation::CUBE_FACE_ORIENTATION_NO_ROTATION};

    for (int i = 0; i < NUM_FACES; ++i)
    {
        faceOrder[i] = (((size_t) sourceInfo.faceOrder) >> (i * BITS_FOR_FACE_ORDER)) & 0x7;
    }
    for (int i = 0; i < NUM_FACES; ++i)
    {
        int value = (((size_t) sourceInfo.faceOrientation) >> (i * BITS_FOR_FACE_ORIENTATION)) & 0x7;
        faceRotation[i] = (CubeFaceSectionOrientation::Enum)((value == 0) ? 0 : (1 << value - 1));  // take the 0 out
    }

    // 0 - front
    // 1 - left
    // 2 - back
    // 3 - right
    // 4 - top
    // 5 - bottom
    for (int i = 0; i < NUM_FACES; ++i)
    {
        leftSource->cubeMap.cubeFaces[i].faceIndex = faceOrder[i];
        leftSource->cubeMap.cubeFaces[i].numCoordinates = 1;
        for (int j = 0; j < leftSource->cubeMap.cubeFaces[i].numCoordinates; ++j)
        {
            leftSource->cubeMap.cubeFaces[i].sections[j].sourceX =
                leftOriginX + (i % 3) * (width / 3);  // normalized tile position in source texture
            leftSource->cubeMap.cubeFaces[i].sections[j].sourceY = leftOriginY + (i / 3) * (height / 2);
            leftSource->cubeMap.cubeFaces[i].sections[j].sourceWidth =
                width / 3;  // normalized tile size in source texture
            leftSource->cubeMap.cubeFaces[i].sections[j].sourceHeight = height / 2;
            leftSource->cubeMap.cubeFaces[i].sections[j].originX = 0.f;  // normalized tile position inside origin face
            leftSource->cubeMap.cubeFaces[i].sections[j].originY = 0.f;
            leftSource->cubeMap.cubeFaces[i].sections[j].originWidth = 1.f;  // normalized tile size inside origin face
            leftSource->cubeMap.cubeFaces[i].sections[j].originHeight = 1.f;
            leftSource->cubeMap.cubeFaces[i].sections[j].sourceOrientation = faceRotation[i];
        }
        rightSource->cubeMap.cubeFaces[i].faceIndex = faceOrder[i];
        rightSource->cubeMap.cubeFaces[i].numCoordinates = 1;
        for (int j = 0; j < rightSource->cubeMap.cubeFaces[i].numCoordinates; ++j)
        {
            rightSource->cubeMap.cubeFaces[i].sections[j].sourceX =
                rightOriginX + (i % 3) * (width / 3);  // normalized tile position in source texture
            rightSource->cubeMap.cubeFaces[i].sections[j].sourceY = rightOriginY + (i / 3) * (height / 2);
            rightSource->cubeMap.cubeFaces[i].sections[j].sourceWidth =
                width / 3;  // normalized tile size in source texture
            rightSource->cubeMap.cubeFaces[i].sections[j].sourceHeight = height / 2;
            rightSource->cubeMap.cubeFaces[i].sections[j].originX = 0.f;  // normalized tile position inside origin face
            rightSource->cubeMap.cubeFaces[i].sections[j].originY = 0.f;
            rightSource->cubeMap.cubeFaces[i].sections[j].originWidth = 1.f;  // normalized tile size inside origin face
            rightSource->cubeMap.cubeFaces[i].sections[j].originHeight = 1.f;
            rightSource->cubeMap.cubeFaces[i].sections[j].sourceOrientation = faceRotation[i];
        }
    }
    leftSource->streamIndex = leftStream.streamId;
    rightSource->streamIndex = rightStream.streamId;
    addVideoSource(leftSource);
    addVideoSource(rightSource);

    return true;
}

bool_t MetadataParser::setVideoMetadataPackageMultiResMonoCubemap(sourceid_t& aSourceId,
                                                                  const VideoInfo& stream,
                                                                  BasicSourceInfo data)
{
    OMAF_ASSERT(stream.streamId != OMAF_UINT8_MAX, "Stream ID not set");
    OMAF_LOG_D("Setting to mono cubemap");
    const int NUM_FACES = 6;

    CubeMapSource* cubeMapSource = OMAF_NEW_HEAP(CubeMapSource);
    cubeMapSource->sourcePosition = SourcePosition::MONO;
    cubeMapSource->sourceId = aSourceId++;

    cubeMapSource->cubeMap.cubeYaw = 0;
    cubeMapSource->cubeMap.cubePitch = 0;
    cubeMapSource->cubeMap.cubeRoll = 0;
    cubeMapSource->cubeMap.cubeNumFaces = NUM_FACES;

    float width = 1.f;
    float height = 1.f;

    // 0 - front
    // 1 - left
    // 2 - back
    // 3 - right
    // 4 - up
    // 5 - down
    // int faceOrder[6] = { 1, 0, 3, 4, 5, 2 };

    for (int i = 0; i < NUM_FACES; ++i)
    {
        cubeMapSource->cubeMap.cubeFaces[i].faceIndex = data.tiledCubeMap.cubeFaces[i].faceIndex;
        cubeMapSource->cubeMap.cubeFaces[i].numCoordinates = data.tiledCubeMap.cubeFaces[i].numCoordinates;
        for (int j = 0; j < cubeMapSource->cubeMap.cubeFaces[i].numCoordinates; ++j)
        {
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceX =
                data.tiledCubeMap.cubeFaces[i].sections[j].sourceX;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceY =
                data.tiledCubeMap.cubeFaces[i].sections[j].sourceY;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceWidth =
                data.tiledCubeMap.cubeFaces[i].sections[j].sourceWidth;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceHeight =
                data.tiledCubeMap.cubeFaces[i].sections[j].sourceHeight;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].originX =
                data.tiledCubeMap.cubeFaces[i].sections[j].originX;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].originY =
                data.tiledCubeMap.cubeFaces[i].sections[j].originY;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].originWidth =
                data.tiledCubeMap.cubeFaces[i].sections[j].originWidth;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].originHeight =
                data.tiledCubeMap.cubeFaces[i].sections[j].originHeight;
            cubeMapSource->cubeMap.cubeFaces[i].sections[j].sourceOrientation =
                data.tiledCubeMap.cubeFaces[i].sections[j].sourceOrientation;
        }
    }
    cubeMapSource->textureRect.x = 0.0f;
    cubeMapSource->textureRect.y = 0.0f;
    cubeMapSource->textureRect.w = 1.0f;
    cubeMapSource->textureRect.h = 1.0f;
    cubeMapSource->widthInPixels = stream.width;
    cubeMapSource->heightInPixels = stream.height;
    cubeMapSource->streamIndex = stream.streamId;
    addVideoSource(cubeMapSource);
    return true;
}

bool_t MetadataParser::setVideoMetadataPackageMultiResStereoCubemap(sourceid_t& sourceIdLeft,
                                                                    const VideoInfo& leftStream,
                                                                    const VideoInfo& rightStream,
                                                                    BasicSourceInfo sourceInfo)
{
    return false;
}
#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
bool_t MetadataParser::setVideoMetadataPackageEquirectTile(sourceid_t& sourceId,
                                                           SourceDirection::Enum sourceDirection,
                                                           StereoRole::Enum stereoRole,
                                                           const VASTileViewport& viewport,
                                                           const VideoInfo& stream)
{
    OMAF_LOG_D("setVideoMetadataPackageEquirectTile");
    EquirectangularTileSource* sourcePrimary = OMAF_NEW_HEAP(EquirectangularTileSource);

    sourcePrimary->streamIndex = stream.streamId;
    sourcePrimary->sourceId = sourceId++;
    if (sourceDirection == SourceDirection::TOP_BOTTOM)
    {
        sourcePrimary->sourcePosition = SourcePosition::LEFT;
    }
    else if (stereoRole == StereoRole::LEFT)
    {
        sourcePrimary->sourcePosition = SourcePosition::LEFT;
    }
    else if (stereoRole == StereoRole::RIGHT)
    {
        sourcePrimary->sourcePosition = SourcePosition::RIGHT;
    }
    else
    {
        sourcePrimary->sourcePosition = SourcePosition::MONO;
    }

    sourcePrimary->centerLatitude = viewport.getCenterLatitude();
    sourcePrimary->centerLongitude = viewport.getCenterLongitude(
        VASLongitudeDirection::CLOCKWISE);  // the tile renderer operates with clockwise longitudes
    float64_t horFov, verFov;
    viewport.getSpan(horFov, verFov);
    sourcePrimary->spanLatitude = (float32_t) verFov;
    sourcePrimary->spanLongitude = (float32_t) horFov;
    sourcePrimary->relativeQuality = 100;  // not currently used

    if (sourceDirection == SourceDirection::TOP_BOTTOM)
    {
        // top-bottom tile is in this track
        sourcePrimary->textureRect.x = 0.0f;
        sourcePrimary->textureRect.y = 0.5f;
        sourcePrimary->textureRect.w = 1.0f;
        sourcePrimary->textureRect.h = 0.5f;
        sourcePrimary->widthInPixels = stream.width;
        sourcePrimary->heightInPixels = stream.height / 2;

        EquirectangularTileSource* sourceBottom = OMAF_NULL;
        sourceBottom = OMAF_NEW_HEAP(EquirectangularTileSource);
        sourceBottom->streamIndex = stream.streamId;
        sourceBottom->sourceId = sourceId++;

        sourceBottom->sourcePosition = SourcePosition::RIGHT;
        sourceBottom->textureRect.x = 0.0f;
        sourceBottom->textureRect.y = 0.0f;
        sourceBottom->textureRect.w = 1.0f;
        sourceBottom->textureRect.h = 0.5f;
        sourceBottom->centerLatitude = sourcePrimary->centerLatitude;
        sourceBottom->centerLongitude = sourcePrimary->centerLongitude;
        sourceBottom->spanLatitude = (float32_t) verFov;
        sourceBottom->spanLongitude = (float32_t) horFov;
        sourceBottom->relativeQuality = 100;  // not currently used
        sourcePrimary->widthInPixels = stream.width;
        sourcePrimary->heightInPixels = stream.height / 2;

        addVideoSource(sourcePrimary);
        addVideoSource(sourceBottom);
    }
    else
    {
        // full tile is in this track
        sourcePrimary->textureRect.x = 0.0f;
        sourcePrimary->textureRect.y = 0.0f;
        sourcePrimary->textureRect.w = 1.0f;
        sourcePrimary->textureRect.h = 1.0f;
        sourcePrimary->widthInPixels = stream.width;
        sourcePrimary->heightInPixels = stream.height;
        addVideoSource(sourcePrimary);
    }
    return true;
}
#endif

bool_t MetadataParser::forceVideoTo(SourcePosition::Enum aPosition)
{
    if (mVideoSources.getSize() > 1)
    {
        // framepacked source
        CoreProviderSource* source = mVideoSources.at(1);
        if (source->sourcePosition == SourcePosition::LEFT)
        {
            // source 1 is left, keep source 0 (right)
            mVideoSources.removeAt(1);
            mInactiveVideoSources.add(source);
        }
        else
        {
            // source 1 is right, keep it and remove source 0 (after delete the source at 1 moves to 0)
            source = mVideoSources.at(0);
            mVideoSources.removeAt(0);
            mInactiveVideoSources.add(source);
        }
        mVideoSources.at(0)->sourcePosition = SourcePosition::MONO;
        return true;
    }
    else if (mVideoSources.getSize() > 0)
    {
        mVideoSources.at(0)->sourcePosition = aPosition;
        if (mInactiveVideoSources.getSize() > 0)
        {
            // restore framepacked
            mVideoSources.add(mInactiveVideoSources.at(0));
            mInactiveVideoSources.removeAt(0);
        }
        return true;
    }
    return false;
}

bool_t MetadataParser::setVideoMetadataPackageErpRegions(sourceid_t& aSourceId,
                                                         const VideoInfo& aStream,
                                                         BasicSourceInfo aSourceInfo)
{
    OMAF_ASSERT(aStream.streamId != OMAF_UINT8_MAX, "Stream ID not set");

    for (RegionPacking::ConstIterator it = aSourceInfo.erpRegions.begin(); it != aSourceInfo.erpRegions.end(); ++it)
    {
        // we use tile source for regions, although it is based on latitude/longitude output coordinates, which can
        // cause rounding errors.
        EquirectangularTileSource* source = OMAF_NEW_HEAP(EquirectangularTileSource);

        source->sourceId = aSourceId++;
        source->textureRect.x = (*it).inputRect.x;
        // note that the player has origin in bottom-left corner of the input rectangle, whereas signaled
        // coordinates assume top-left origin
        source->textureRect.y = 1.f - (*it).inputRect.y - (*it).inputRect.h;
        source->textureRect.w = (*it).inputRect.w;
        source->textureRect.h = (*it).inputRect.h;

        if (aSourceInfo.sourceDirection == SourceDirection::TOP_BOTTOM)
        {
            source->centerLongitude = (*it).centerLongitude;
            source->spanLongitude = (*it).spanLongitude;

            if ((*it).centerLatitude >= 0.f)
            {
                // top of top-bottom
                source->sourcePosition = SourcePosition::LEFT;
                source->centerLatitude = (*it).centerLatitude * 2 - 90.f;
            }
            else
            {
                source->sourcePosition = SourcePosition::RIGHT;
                source->centerLatitude = (*it).centerLatitude * 2 + 90.f;
            }
            source->spanLatitude = (*it).spanLatitude * 2;
            source->widthInPixels = (uint16_t)(aStream.width * source->spanLongitude / 360.f);
            source->heightInPixels = (uint16_t)((aStream.height * source->spanLatitude / 2) / 180.f);
        }
        else if (aSourceInfo.sourceDirection == SourceDirection::LEFT_RIGHT)
        {
            source->centerLatitude = (*it).centerLatitude;
            source->spanLatitude = (*it).spanLatitude;

            if ((*it).centerLongitude >= 0.f)
            {
                // right of left-right
                source->sourcePosition = SourcePosition::RIGHT;
                source->centerLongitude = (*it).centerLongitude * 2 - 180.f;
            }
            else
            {
                source->sourcePosition = SourcePosition::LEFT;
                source->centerLongitude = (*it).centerLongitude * 2 + 180.f;
            }
            source->spanLongitude = (*it).spanLongitude * 2;
            source->widthInPixels = (uint16_t)((aStream.width * source->spanLongitude / 2) / 360.f);
            source->heightInPixels = (uint16_t)(aStream.height * source->spanLatitude / 180.f);
        }
        else
        {
            source->sourcePosition = SourcePosition::MONO;
            source->centerLongitude = (*it).centerLongitude;
            source->centerLatitude = (*it).centerLatitude;
            source->spanLongitude = (*it).spanLongitude;
            source->spanLatitude = (*it).spanLatitude;
            source->widthInPixels = (uint16_t)(aStream.width * source->spanLongitude / 360.f);
            source->heightInPixels = (uint16_t)(aStream.height * source->spanLatitude / 180.f);
        }


        source->streamIndex = aStream.streamId;

        addVideoSource(source);
    }
    return true;
}

void_t MetadataParser::updateOverlaySourceBaseProperties(OverlaySource* src)
{
    // only one of the overlay base types can exist at the same time
    src->mediaPlacement = src->overlayParameters.viewportRelativeOverlay.doesExist
        ? SourceMediaPlacement::VIEWPORT_RELATIVE_OVERLAY
        : SourceMediaPlacement::SPHERE_RELATIVE_OVERLAY;


	
    if (src->sourceGroup < 0)  // avoid unnecessary update - it can also create thread conflict, if renderer is
                               // accessing the same src and it has special meaning for group < 0
    {
        if (src->overlayParameters.overlayLabel.doesExist)
        {
            for (auto i = 0; i < mAvailableOverlayGroups.size(); i++)
            {
                if (mAvailableOverlayGroups[i] ==
                    std::string(src->overlayParameters.overlayLabel.overlayLabel.begin(),
                                src->overlayParameters.overlayLabel.overlayLabel.end()))
                {
                    OMAF_LOG_V("Assigning to group %s", mAvailableOverlayGroups[i].c_str());
                    src->sourceGroup = i;
                    break;
                }
            }
        }
    }
}

bool_t MetadataParser::setRotation(const Rotation& aRotation)
{
    // we really don't have (at least currently) cases where we had many sources that could have different rotation,
    // do we? Framepacked stereo is represented as a single mp4 track, and hence has a common rotation.
    bool set = false;
    for (CoreProviderSources::Iterator it = mVideoSources.begin(); it != mVideoSources.end(); ++it)
    {
        // Note! the parameters in makeQuaternion represent the angles around the axes.
        // OMAF parameters are meant to rotate the sphere, and in practice the viewport is then moved to the
        // opposite direction. The player internally uses Y axis as vertical axis and yaw is around it. OMAF has Z
        // axis for the same. Rotation direction is the same. + turns to left Pitch is around horizontal OMAF y axis
        // == x axis in the player, but rotation is opposite. OMAF + should turn down, not up => -1 Roll is around
        // OMAF x axis == z in the player, but rotation is opposite. OMAF + should turn right => -1 order YXZ ==
        // first yaw, then pitch, finally roll. TBD if this is the OMAF order?
        (*it)->extrinsicRotation =
            OMAF::makeQuaternion(-aRotation.pitch, aRotation.yaw, -aRotation.roll, OMAF::EulerAxisOrder::XYZ);
        set = true;
    }
    return set;
}
OMAF_NS_END
