
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

#include <cstdint>
#include <vector>
#include <deque>

#include "Platform/OMAFDataTypes.h"
#include "NVRNamespace.h"

#include <mfidl.h>

OMAF_NS_BEGIN

namespace WMFDecoder {

    enum class DecoderInputType {
        ADTS,
        AAC_LC
    };

    class AACDecoder {

    public:

        AACDecoder(DecoderInputType inputStreamType, uint32_t inputSampleRate, uint32_t inputChannels, uint32_t outputChannels);
        ~AACDecoder();

        /**
         * Add AAC/ADTS frame to decoder and decode it to output buffer.
         * 
         * @return number of bytes in output buffer after decoding AAC. 
         */
        size_t feedFrame(std::vector<uint8_t> &aacOrAdts);

        /**
         * Try to read number of bytes requested from output buffer. 
         *
         * If there is not enough data in output, return all the data from the buffer.
         * 
         * @param bytesToRead Number of bytes that should be read from the buffer. If 0 then return all
         *                    data that is available.
         */
        std::vector<uint8_t> readOutputBuffer(size_t bytesToRead);

        uint32_t getAACSampleRate() const;
        uint32_t getAACChannels() const;

    private:

        /**
         * Returns true if adding frame was ok, false if decoder is full and cannot accept data until some of it has been consumed first.
         * Throws an error on any other failure.
         */
        bool tryToPushFrameToDecoder(std::vector<uint8_t> &aacOrAdts);

        // Tries to read output from decoder, returns an empty vector if no output found.
        std::vector<uint8_t> readDecoderOutput();

        // queries available output types from decoder and tries to find one matching output channel count
        void resetOutputType();

        std::deque<uint8_t> mPCMOutputBuffer;

        uint32_t mOutputChannels;
        uint32_t mSampleBits;

        uint32_t mAACChannels;
        uint32_t mAACSampleRate;

        IMFMediaType *mInputType = NULL;
        IMFMediaType *mOutputType = NULL;
        MFT_OUTPUT_STREAM_INFO mOutStreamInfo = { 0 };
        MFT_OUTPUT_DATA_BUFFER mOutputSample = { 0 };
        IMFTransform *mAACTransformer = NULL;
    };
}

OMAF_NS_END

