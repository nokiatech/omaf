
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

#include "Media/NVRMP4MediaPacketQueue.h"

OMAF_NS_BEGIN

    class MP4VRMediaPacket
    {
    public:
        MP4VRMediaPacket(size_t bufferSize, MP4VRMediaPacketQueue* metaPackets = OMAF_NULL);
        virtual ~MP4VRMediaPacket();

    public:
        //Deletes old data, sets datasize to zero and allocates new mBuffer of specified size. 
        virtual void            resizeBuffer(size_t bufferSize);
        virtual uint8_t *       buffer() { return mBuffer; }
        virtual const uint8_t * buffer() const { return mBuffer; }
        virtual size_t          bufferSize() const { return mBufferSize; }
        virtual size_t          dataSize() const { return mDataSize; }
        virtual void_t          setDataSize(size_t dataSize) { mDataSize = dataSize; }

        virtual int         streamIndex() const { return mStreamIndex; }
        virtual void_t      setStreamIndex(int streamIndex) { mStreamIndex = streamIndex; }

        virtual int64_t     decodingTimeUs() const { return mDecodingTimeUs; }
        virtual void        setDecodingTimeUs(int64_t decodingTimeUs) { mDecodingTimeUs = decodingTimeUs; }

        virtual int64_t     durationUs() const { return mDurationUs; }
        virtual void_t      setDurationUs(int64_t durationUs) { mDurationUs = durationUs; }

        virtual void_t      setReConfigRequired(bool_t required) { mReConfigRequired = required; }
        virtual bool_t      isReConfigRequired() { return mReConfigRequired; }

        virtual void        setIsReferenceFrame(bool_t);
        virtual bool_t      isReferenceFrame() const;

        virtual int64_t     presentationTimeUs() const { return mPresentationTimeUs; }
        virtual void        setPresentationTimeUs(int64_t presentationTimeUs) { mPresentationTimeUs = presentationTimeUs; }

        virtual uint32_t    sampleId() { return mSampleId; }
        virtual void        setSampleId(uint32_t sampleId) { mSampleId = sampleId; }

        virtual MP4VRMediaPacketQueue* getMetaPacketsQueue();

        virtual size_t copyTo(const size_t srcOffset, uint8_t * destBuffer, const size_t destBufferSize);

    private:
        MP4VRMediaPacket(const MP4VRMediaPacket& mediaPacket);
        MP4VRMediaPacket& operator=(const MP4VRMediaPacket& mediaPacket);

        int64_t             mDecodingTimeUs;
        int64_t             mDurationUs;

    private:
        bool_t              mIsReferenceFrame;
        size_t              mBufferSize;
        size_t              mDataSize;

        int32_t             mStreamIndex;
        int64_t             mPresentationTimeUs;
        uint32_t            mSampleId;

        uint8_t*             mBuffer;
        bool_t               mReConfigRequired;

        MP4VRMediaPacketQueue* mMetaPackets;
    };
OMAF_NS_END
