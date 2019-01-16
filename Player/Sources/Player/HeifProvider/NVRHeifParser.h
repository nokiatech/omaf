
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
#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaInformation.h"
#include "Media/NVRMP4VideoStream.h"
#include "Metadata/NVRMetadataParser.h"
#include "Foundation/NVRPathName.h"
#include "NVRHeifFileStreamer.h"
#include "heifreader.h"

OMAF_NS_BEGIN


    class MP4VideoStream;
    class MP4MediaStream;

    class HeifParser 
    {

    public:

        HeifParser();
        virtual ~HeifParser();

        /**
         * Opens a given input.
         * @param mediaUri The media URI to open.
         * @return true if successful, false otherwise.
         */
        virtual Error::Enum openInput(const PathName& mediaUri);

        Error::Enum readImageList();

        Error::Enum selectNextImage(MP4VideoStreams & videoStreams);

        Error::Enum selectImage(HEIF::ImageId imageId, MP4VideoStreams& videoStreams);

        /**
         * Returns read frame as a MediaPacket.
         */
        virtual Error::Enum readFrame(MP4MediaStream& stream);

        virtual const CoreProviderSources& getVideoSources();
        virtual const CoreProviderSourceTypes& getVideoSourceTypes();

        const MediaInformation& getMediaInformation() const;

    private:
        Error::Enum createVideoSourceFromImageItem(HEIF::ImageId imageId, MP4VideoStreams& videoStreams, BasicSourceInfo sourceInfo, uint32_t col = 0, uint32_t row = 0, uint32_t gridWidth = 0, uint32_t gridHeight = 0);

        MemoryAllocator& mAllocator;

        HEIF::Reader* mReader;
        uint32_t mImageIndex;
        MP4VR::DynArray<uint32_t> mImageIds;

        MetadataParser mMetaDataParser;
        
        HeifFileStreamer mFileStream;

        MediaInformation mMediaInformation;

    };
OMAF_NS_END


