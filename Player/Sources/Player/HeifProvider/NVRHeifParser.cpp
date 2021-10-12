
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
#include "NVRHeifParser.h"

#include "Media/NVRMediaPacket.h"
#include "NVRHeifImageStream.h"


#include "Foundation/NVRClock.h"
#include "Foundation/NVRFileUtilities.h"
#include "Foundation/NVRLogger.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"

// works only for arrays which have same type of elements
template <typename T>
MP4VR::DynArray<T> heifArrayToMp4vr(HEIF::Array<T>& src)
{
    MP4VR::DynArray<T> ret(src.size);
    for (size_t i = 0; i < src.size; i++)
    {
        ret[i] = src[i];
    }
    return ret;
}

OMAF_NS_BEGIN
OMAF_LOG_ZONE(MP4VRParser)

HeifParser::HeifParser()
    : mAllocator(*MemorySystem::DefaultHeapAllocator())
    , mReader(OMAF_NULL)
    , mImageIndex(0)
    , mFileStream()

{
    mReader = HEIF::Reader::Create();
}

HeifParser::~HeifParser()
{
    mReader->close();
    HEIF::Reader::Destroy(mReader);
    mReader = OMAF_NULL;
    mFileStream.close();
}

const CoreProviderSourceTypes& HeifParser::getVideoSourceTypes()
{
    return mMetaDataParser.getVideoSourceTypes();
}

const CoreProviderSources& HeifParser::getVideoSources()
{
    return mMetaDataParser.getVideoSources();
}

Error::Enum HeifParser::openInput(const PathName& mediaUri)
{
    if (mFileStream.open(mediaUri) != Error::OK)
    {
        return Error::FILE_NOT_FOUND;
    }

    HEIF::ErrorCode result = mReader->initialize(&mFileStream);
    if (result != HEIF::ErrorCode::OK)
    {
        return Error::FILE_OPEN_FAILED;
    }


    HEIF::FileInformation properties;
    if (mReader->getFileInformation(properties) != HEIF::ErrorCode::OK)
    {
        return Error::FILE_OPEN_FAILED;  //??
    }
    if ((properties.features & HEIF::FileFeatureEnum::HasRootLevelMetaBox) == false)
    {
        return Error::FILE_NOT_SUPPORTED;
    }

    // check brands
    bool_t playable = false;

    HEIF::FourCC major;
    mReader->getMajorBrand(major);
    if (major == HEIF::FourCC("mif1"))
    {
        playable = true;
    }

    HEIF::Array<HEIF::FourCC> brands;
    mReader->getCompatibleBrands(brands);
    for (HEIF::FourCC* it = brands.begin(); it != brands.end(); it++)
    {
        if (*it == HEIF::FourCC("heic"))
        {
            playable = true;
        }
    }

    if (!playable)
    {
        OMAF_LOG_D("No known HEIF brand found");
        return Error::FILE_NOT_SUPPORTED;
    }

    readImageList();

    return Error::OK;
}

Error::Enum HeifParser::readImageList()
{
    HEIF::ImageId primary;
    auto hasPrimaryItem = mReader->getPrimaryItem(primary) == HEIF::ErrorCode::OK;

    HEIF::FileInformation fileInformation;
    mReader->getFileInformation(fileInformation);

    HEIF::Array<HEIF::ImageId> imageIds;
    mReader->getMasterImages(imageIds);

    if (!hasPrimaryItem)
    {
        primary = imageIds[0];
    }

    // create list of imageids where primary image is first
    MP4VR::DynArray<uint32_t> ret(imageIds.size);
    ret[0] = primary.get();
    uint32_t pos = 1;
    for (auto imageId : imageIds)
    {
        if (imageId != primary)
        {
            // file item parameters from fileInformation
            bool_t isHidden = true;
            for (auto& itemInfo : fileInformation.rootMetaBoxInformation.itemInformations)
            {
                if (itemInfo.itemId == imageId)
                {
// heif github release
#ifdef USE_FIRST_OMAF_HEIF_GITHUB_RELEASE
                    isHidden = (itemInfo.features & HEIF::ImageFeatureEnum::IsHiddenImage) > 0;
#else
                    isHidden = (itemInfo.features & HEIF::ItemFeatureEnum::IsHiddenImage) > 0;
#endif
                    break;
                }
            }

            if (!isHidden)
            {
                ret[pos++] = imageId.get();
            }
        }
    }

    ret.size = pos;
    mImageIds = ret;

    return Error::OK;
}

Error::Enum HeifParser::selectNextImage(MP4VideoStreams& videoStreams)
{
    mMetaDataParser.reset();
    mImageIndex = mImageIndex % mImageIds.size;
    selectImage((HEIF::ImageId) mImageIds[mImageIndex], videoStreams);
    mImageIndex++;  // first call will select the first image
    return Error::OK;
}

Error::Enum HeifParser::selectImage(HEIF::ImageId imageId, MP4VideoStreams& videoStreams)
{
    // Read OMAF properties of primary image
    HEIF::Array<HEIF::ItemPropertyInfo> itemProperties;
    mReader->getItemProperties(imageId, itemProperties);

    HEIF::FramePackingProperty stvi = HEIF::FramePackingProperty::MONOSCOPIC;
    const auto PRFR_NOT_SET = (HEIF::OmafProjectionType)(-1);
    HEIF::ProjectionFormatProperty prfr = {PRFR_NOT_SET};
    HEIF::RegionWisePackingProperty rwpk;
    HEIF::Rotation rotn;
    HEIF::CoverageInformationProperty covi;
    HEIF::InitialViewingOrientation iivo;

    for (auto& propInfo : itemProperties)
    {
        switch (propInfo.type)
        {
        case HEIF::ItemPropertyType::STVI:
            mReader->getProperty(propInfo.index, stvi);
            break;
        case HEIF::ItemPropertyType::PRFR:
            mReader->getProperty(propInfo.index, prfr);
            break;
        case HEIF::ItemPropertyType::RWPK:
            mReader->getProperty(propInfo.index, rwpk);
            break;
        case HEIF::ItemPropertyType::ROTN:
            mReader->getProperty(propInfo.index, rotn);
            break;
        case HEIF::ItemPropertyType::COVI:
            mReader->getProperty(propInfo.index, covi);
            break;
        case HEIF::ItemPropertyType::IIVO:
            mReader->getProperty(propInfo.index, iivo);
            break;
        }
    }

    BasicSourceInfo sourceInfo;
    if (stvi == HEIF::FramePackingProperty::MONOSCOPIC)
    {
        sourceInfo.sourceDirection = SourceDirection::MONO;
    }
    else if (stvi == HEIF::FramePackingProperty::TOP_BOTTOM_PACKING)
    {
        sourceInfo.sourceDirection = SourceDirection::TOP_BOTTOM;
    }
    else if (stvi == HEIF::FramePackingProperty::SIDE_BY_SIDE_PACKING)
    {
        sourceInfo.sourceDirection = SourceDirection::LEFT_RIGHT;
    }

    if (prfr.format == HEIF::OmafProjectionType::EQUIRECTANGULAR)
    {
        sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
    }
    else if (prfr.format == HEIF::OmafProjectionType::CUBEMAP)
    {
        sourceInfo.sourceType = SourceType::CUBEMAP;
    }
    else if (prfr.format == PRFR_NOT_SET)
    {
        sourceInfo.sourceType = SourceType::IDENTITY;
    }

    HEIF::FourCC primaryItemType;
    mReader->getItemType(imageId, primaryItemType);

    if (primaryItemType == "grid")
    {
        HEIF::Grid grid;
        mReader->getItem(imageId, grid);

        if (sourceInfo.sourceType == SourceType::EQUIRECTANGULAR_PANORAMA)
        {
            sourceInfo.sourceType = SourceType::EQUIRECTANGULAR_TILES;
        }
        else if (sourceInfo.sourceType == SourceType::CUBEMAP)
        {
            sourceInfo.sourceType = SourceType::CUBEMAP_TILES;
        }


        for (uint32_t row = 0; row < grid.rows; row++)
        {
            for (uint32_t col = 0; col < grid.columns; col++)
            {
                auto& imageId = grid.imageIds[row * grid.columns + col];
                createVideoSourceFromImageItem(imageId, videoStreams, sourceInfo, col, row, grid.outputWidth,
                                               grid.outputHeight);
            }
        }
    }
    else
    {
        createVideoSourceFromImageItem(imageId, videoStreams, sourceInfo);
    }

    MP4VideoStream* first = *videoStreams.begin();
    mMediaInformation.width = first->getFormat()->width();
    mMediaInformation.height = first->getFormat()->height();
    mMediaInformation.frameRate = first->getFormat()->frameRate();
    mMediaInformation.durationUs = first->getFormat()->durationUs();
    mMediaInformation.numberOfFrames = first->getFrameCount();

    mMediaInformation.fileType = MediaFileType::HEIF;

    return Error::OK;
}

Error::Enum HeifParser::readFrame(MP4MediaStream& stream)
{
    HeifImageStream& imageStream = (HeifImageStream&) stream;

    if (imageStream.isPrimaryRead())
    {
        return Error::END_OF_FILE;
    }
    MP4VRMediaPacket* packet = imageStream.getNextEmptyPacket();

    if (packet == OMAF_NULL)
    {
        // stream.mTrack may be NULL and then we can't read data
        return Error::END_OF_FILE;
    }
    packet->setSampleId(0);  // initialize to 0, set to real sampleId later


    HEIF::ErrorCode result;

    HEIF::ImageId streamImageId = imageStream.getFormat()->getDecoderConfigInfoId();

    packet->setReConfigRequired(false);

    bool_t bytestreamMode = false;

    if (stream.IsVideo())
    {
        bytestreamMode = VideoDecoderManager::getInstance()->isByteStreamHeadersMode();
    }

    uint64_t packetSize = packet->bufferSize();

    result = mReader->getItemData(streamImageId, packet->buffer(), packetSize);

    if (result == HEIF::ErrorCode::BUFFER_SIZE_TOO_SMALL)
    {
        // retry with reallocated larger buffer
        packet->resizeBuffer(packetSize);
        result = mReader->getItemData(streamImageId, packet->buffer(), packetSize);
    }

    if (result != HEIF::ErrorCode::OK)
    {
        stream.returnEmptyPacket(packet);
        return Error::INVALID_DATA;
    }

    packet->setDataSize(packetSize);
    packet->setPresentationTimeUs(0);

    packet->setDurationUs(1000);  // just a fake duration
    packet->setIsReferenceFrame(true);
    packet->setDecodingTimeUs(Clock::getMicroseconds());  // we use current time as the decoding timestamp; it just
                                                          // indicates the decoding order.

    stream.storeFilledPacket(packet);

    if (result == HEIF::ErrorCode::OK)
    {
        ((HeifImageStream&) stream).setPrimaryAsRead();
        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

const MediaInformation& HeifParser::getMediaInformation() const
{
    return mMediaInformation;
}

void addRegion(BasicSourceInfo& sourceInfo,
               uint32_t inputX,
               uint32_t inputY,
               float32_t left,
               float32_t top,
               float32_t right,
               float32_t bottom,
               float32_t width,
               float32_t height,
               uint32_t gridWidth,
               uint32_t gridHeight,
               uint32_t imageWidth,
               uint32_t imageHeight)
{
    // center point in -0.5 .. 0.5 coordinates
    float32_t centerYPos = (((top + bottom) / 2) - 0.5);
    float32_t centerXPos = (((left + right) / 2) - 0.5);

    // degrees in range -180 .. 180 and -90 .. 90
    float32_t centerLongDegrees = centerXPos * 360;
    float32_t centerLatDegrees = centerYPos * (-180);  // upwards is 90 degrees

    // current tile size converted to degrees
    float32_t spanLatitude = (float32_t(height) / gridHeight) * 180;
    float32_t spanLongitude = (float32_t(width) / gridWidth) * 360;

    Region tile;
    tile.inputRect.x = inputX;
    tile.inputRect.y = inputY;
    tile.inputRect.w = float32_t(width) / imageWidth;
    tile.inputRect.h = float32_t(height) / imageHeight;
    tile.centerLatitude = centerLatDegrees;
    tile.centerLongitude = centerLongDegrees;
    tile.spanLatitude = spanLatitude;
    tile.spanLongitude = spanLongitude;

    sourceInfo.erpRegions.add(tile);
}

Error::Enum HeifParser::createVideoSourceFromImageItem(HEIF::ImageId imageId,
                                                       MP4VideoStreams& videoStreams,
                                                       BasicSourceInfo sourceInfo,
                                                       uint32_t col,
                                                       uint32_t row,
                                                       uint32_t gridWidth,
                                                       uint32_t gridHeight)
{
    HEIF::FourCC codecFourCC;
    mReader->getDecoderCodeType(imageId, codecFourCC);
    HEIF::FeatureBitMask vrFeatures = 0;
    MediaFormat* format =
        OMAF_NEW(mAllocator, MediaFormat)(MediaFormat::Type::IsVideo, vrFeatures, codecFourCC.value, UNKNOWN_MIME_TYPE);

    if (format == OMAF_NULL)
    {
        return Error::OUT_OF_MEMORY;
    }

    uint32_t height = 0, width = 0;
    mReader->getWidth(imageId, width);
    mReader->getHeight(imageId, height);

    // calculate where grid item should be projected and how big
    if (gridWidth > 0 && gridHeight > 0)
    {
        bool_t isLastCol = ((col + 1) * width) >= gridWidth;
        bool_t isLastRow = ((row + 1) * height) >= gridHeight;
        uint32_t lastColWidth = gridWidth % width;
        uint32_t lastRowHeight = gridHeight % height;
        uint32_t currentTileHeight = height;
        uint32_t currentTileWidth = width;
        if (isLastCol && lastColWidth > 0)
        {
            currentTileWidth = lastColWidth;
        }
        if (isLastRow && lastRowHeight > 0)
        {
            currentTileHeight = lastRowHeight;
        }

        // projected place for tile in 0..1 coordinates
        float32_t projectedTop = float32_t(row * height) / gridHeight;
        float32_t projectedBottom = float32_t(row * height + currentTileHeight) / gridHeight;
        float32_t projectedLeft = float32_t(col * width) / gridWidth;
        float32_t projectedRight = float32_t(col * width + currentTileWidth) / gridWidth;

        // if STEREO and tile is crossing TOP-BOTTOM or LEFT-RIGHT line, create 2 separate regions
        if (sourceInfo.sourceDirection == SourceDirection::TOP_BOTTOM && projectedTop < 0.5f && projectedBottom > 0.5f)
        {
            auto tileProjectedHeight = projectedBottom - projectedTop;
            auto topTileHeight = float32_t(currentTileHeight) * ((0.5f - projectedTop) / tileProjectedHeight);
            auto bottomTileHeight = float32_t(currentTileHeight) - topTileHeight;
            auto bottomInputY = topTileHeight;

            addRegion(sourceInfo, 0, 0, projectedLeft, projectedTop, projectedRight, 0.5f, currentTileWidth,
                      topTileHeight, gridWidth, gridHeight, width, height);
            addRegion(sourceInfo, 0, bottomInputY, projectedLeft, 0.5f, projectedRight, projectedBottom,
                      currentTileWidth, bottomTileHeight, gridWidth, gridHeight, width, height);
        }
        else if (sourceInfo.sourceDirection == SourceDirection::LEFT_RIGHT && projectedLeft < 0.5f &&
                 projectedRight > 0.5f)
        {
            auto tileProjectedWidth = projectedRight - projectedLeft;
            auto leftTileWidth = float32_t(currentTileWidth) * ((0.5f - projectedLeft) / tileProjectedWidth);
            auto rightTileWidth = float32_t(currentTileWidth) - leftTileWidth;
            auto rightInputX = leftTileWidth;

            addRegion(sourceInfo, 0, 0, projectedLeft, projectedTop, projectedRight, 0.5f, leftTileWidth,
                      currentTileHeight, gridWidth, gridHeight, width, height);
            addRegion(sourceInfo, rightInputX, 0, projectedLeft, 0.5f, projectedRight, projectedBottom, rightTileWidth,
                      currentTileHeight, gridWidth, gridHeight, width, height);
        }
        else
        {
            // MONO image or source is completely in single stereo region
            addRegion(sourceInfo, 0, 0, projectedLeft, projectedTop, projectedRight, projectedBottom, currentTileWidth,
                      currentTileHeight, gridWidth, gridHeight, width, height);
        }
    }

    HEIF::DecoderConfiguration parameterSet;
    if (mReader->getDecoderParameterSets(imageId, parameterSet) != HEIF::ErrorCode::OK)
    {
        return Error::INVALID_DATA;
    }

    // convert HEIF::DecoderSpecificInfo to MP4VR::DecoderSpecificInfo
    MP4VR::DynArray<MP4VR::DecoderSpecificInfo> info(parameterSet.decoderSpecificInfo.size);
    for (size_t i = 0; i < parameterSet.decoderSpecificInfo.size; i++)
    {
        auto& item = parameterSet.decoderSpecificInfo[i];
        info[i].decSpecInfoData = heifArrayToMp4vr(item.decSpecInfoData);
        info[i].decSpecInfoType = (MP4VR::DecSpecInfoType) item.decSpecInfoType;
    }

    format->setDecoderConfigInfo(info, imageId.get());
    format->setWidth(width);
    format->setHeight(height);
    format->setDuration(0);
    format->setFrameRate(1);

    HeifImageStream* stream = OMAF_NEW(mAllocator, HeifImageStream)(format);
    if (stream == OMAF_NULL)
    {
        OMAF_DELETE(mAllocator, format);
        return Error::OUT_OF_MEMORY;
    }

    stream->setStreamId(VideoDecoderManager::getInstance()->generateUniqueStreamID());

    videoStreams.add(stream);

    sourceid_t sourceId = stream->getStreamId() * 256;
    mMetaDataParser.setVideoMetadata(sourceInfo, sourceId, stream->getDecoderConfig());

    return Error::OK;
}

OMAF_NS_END
