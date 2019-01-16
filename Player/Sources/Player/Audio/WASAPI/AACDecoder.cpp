
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
#define WASAPI_LOGS
#include "AACDecoder.h"

#include "Foundation/NVRLogger.h"
#include "Context.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>

#include <cmath>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "strmiids.lib")

OMAF_NS_BEGIN
OMAF_LOG_ZONE(AACDecoder)

#define CHECK_HR_AND_THROW(hr, msg) \
    if (hr != S_OK) { \
        WASAPIImpl::write_message(msg);    \
        WASAPIImpl::write_message("\nError: %.2X (%s)", hr, getErrorEnum(hr));\
        throw std::exception("Error: " msg);\
    }

char* getErrorEnum(HRESULT hr) {
    switch (hr) {
    case E_NOTIMPL: return "E_NOTIMPL";
    case E_INVALIDARG: return "E_INVALIDARG";
    case MF_E_BUFFERTOOSMALL: return "MF_E_BUFFERTOOSMALL";
    case MF_E_INVALIDMEDIATYPE: return "MF_E_INVALIDMEDIATYPE";
    case MF_E_INVALIDREQUEST: return "MF_E_INVALIDREQUEST";
    case MF_E_INVALIDSTREAMNUMBER: return "MF_E_INVALIDSTREAMNUMBER";
    case MF_E_TOPO_CODEC_NOT_FOUND: return "MF_E_TOPO_CODEC_NOT_FOUND";
    case MF_E_INVALIDTYPE: return "MF_E_INVALIDTYPE";
    case MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING: return "MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING";
    case MF_E_TRANSFORM_TYPE_NOT_SET: return "MF_E_TRANSFORM_TYPE_NOT_SET";
    case MF_E_UNSUPPORTED_D3D_TYPE: return "MF_E_UNSUPPORTED_D3D_TYPE";
    case MF_E_NO_SAMPLE_DURATION: return "MF_E_NO_SAMPLE_DURATION";
    case MF_E_NO_SAMPLE_TIMESTAMP: return "MF_E_NO_SAMPLE_TIMESTAMP";
    case MF_E_NOTACCEPTING: return "MF_E_NOTACCEPTING";
    case E_UNEXPECTED: return "E_UNEXPECTED";
    case MF_E_TRANSFORM_NEED_MORE_INPUT: return "MF_E_TRANSFORM_NEED_MORE_INPUT";
    case MF_E_TRANSFORM_STREAM_CHANGE: return "MF_E_TRANSFORM_STREAM_CHANGE";
    case MF_E_NO_MORE_TYPES: return "MF_E_NO_MORE_TYPES";
    case S_OK: return "S_OK";
    default: return "UNKNOWN CODE";
    }
}

uint8_t getInputSampleRateIndex(uint32_t sampleRate) {
    switch (sampleRate) {
    case 96000: return 0;
    case 88200: return 1;
    case 64000: return 2;
    case 48000: return 3;
    case 44100: return 4;
    case 32000: return 5;
    case 24000: return 6;
    case 22050: return 7;
    case 16000: return 8;
    case 12000: return 9;
    case 11025: return 10;
    case 8000:  return 11;
    case 7350:  return 12;
    default:    return -1;
    }
}

namespace WMFDecoder {

    AACDecoder::AACDecoder(
        DecoderInputType inputStreamType, 
        uint32_t inputSampleRate, 
        uint32_t inputChannels, 
        uint32_t outputChannels
    ) :
        mOutputChannels(outputChannels), mSampleBits(16), mAACChannels(inputChannels), mAACSampleRate(inputSampleRate)
    {
        HRESULT hr = S_OK;
        bool adtsInput = inputStreamType == DecoderInputType::ADTS;

        CHECK_HR_AND_THROW(MFStartup(MF_VERSION), "Could not startup MF");

        UINT32 flags = MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER;
        IMFActivate **activate = nullptr;
        UINT32 formatCount = 0;
        MFT_REGISTER_TYPE_INFO inputFilterAAC = { MFMediaType_Audio, MFAudioFormat_AAC };

        CHECK_HR_AND_THROW(
            MFTEnumEx(MFT_CATEGORY_AUDIO_DECODER, flags, &inputFilterAAC, NULL, &activate, &formatCount), 
            "Could no create transform node"
        );

        if (formatCount > 0) {
            WASAPIImpl::write_message("AACDecoder: Found %i formats", formatCount);
            CHECK_HR_AND_THROW(activate[0]->ActivateObject(IID_PPV_ARGS(&mAACTransformer)), "Could not get decoder.");
        }
        else {
            WASAPIImpl::write_message("AACDecoder Error: No activate objects for AAC");
        }

        for (UINT32 i = 0; i < formatCount; i++) {
            activate[i]->Release();
        }
        CoTaskMemFree(activate);

        // Setup input type
        CHECK_HR_AND_THROW(MFCreateMediaType(&mInputType), "Could not create media type");
        CHECK_HR_AND_THROW(mInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio), "Could not set major type for output media");
        CHECK_HR_AND_THROW(mInputType->SetGUID(MF_MT_SUBTYPE, adtsInput ? MFAudioFormat_ADTS : MFAudioFormat_AAC), "Could not set subtype for output media");

        // needed for plain AAC input
        CHECK_HR_AND_THROW(mInputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, inputSampleRate),
            "Error input media type sample rate.");
        CHECK_HR_AND_THROW(mInputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, inputChannels),
            "Error input media type channels.");

        // IF plain AAC the user data must be set too
        // https://docs.microsoft.com/en-us/windows/desktop/medfound/aac-decoder#example-media-types

        // last 2 bytes of user data
        // AudioSpecificConfig.audioObjectType = 2 (AAC LC) (5 bits)
        // AudioSpecificConfig.samplingFrequencyIndex = 3 (4 bits)
        // AudioSpecificConfig.channelConfiguration = 6 (4 bits)
        // GASpecificConfig.frameLengthFlag = 0 (1 bit)
        // GASpecificConfig.dependsOnCoreCoder = 0 (1 bit)
        // GASpecificConfig.extensionFlag = 0 (1 bit)

        uint8_t audioObjectType = adtsInput ? 0 : 2;
        uint8_t aacSampleRateIndex = getInputSampleRateIndex(inputSampleRate);
        uint8_t byte1 = (audioObjectType << 3) | (aacSampleRateIndex >> 1);
        uint8_t byte2 = (aacSampleRateIndex << 7) | (inputChannels << 3);

        UINT8 userData[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, byte1, byte2 };

        CHECK_HR_AND_THROW(mInputType->SetBlob(MF_MT_USER_DATA, userData, sizeof(userData)),
            "Error input user data.");

        CHECK_HR_AND_THROW(mAACTransformer->SetInputType(0, mInputType, 0), "Setting inputType failed");

        // find suitable output type
        resetOutputType();
    }

    AACDecoder::~AACDecoder() {
        mInputType && mInputType->Release();
        mOutputType && mOutputType->Release();
        mAACTransformer && mAACTransformer->Release();
        MFShutdown();
    }

    size_t AACDecoder::feedFrame(std::vector<uint8_t> &aacOrAdts) {
        auto isFull = tryToPushFrameToDecoder(aacOrAdts);
        OMAF_ASSERT(isFull, "AACDecoder was full when trying to add frame.");
        auto& pcm = readDecoderOutput();
        mPCMOutputBuffer.insert(mPCMOutputBuffer.end(), pcm.begin(), pcm.end());
        return mPCMOutputBuffer.size();
    }

    std::vector<uint8_t> AACDecoder::readOutputBuffer(size_t bytesToRead) {
        auto readLen = bytesToRead < mPCMOutputBuffer.size() ? bytesToRead : mPCMOutputBuffer.size();
        auto readStart = mPCMOutputBuffer.begin();
        auto readEnd = readStart + readLen;
        std::vector<uint8_t> res(readStart, readEnd);

        mPCMOutputBuffer.erase(readStart, readEnd);
        return res;
    }
    
    uint32_t AACDecoder::getAACSampleRate() const {
        return mAACSampleRate;
    }

    uint32_t AACDecoder::getAACChannels() const {
        return mAACChannels;
    }

    /**
    * Returns true if adding frame was ok, false if decoder is full and cannot accept data until some of it has been consumed first.
    * Throws an error on any other failure.
    */
    bool AACDecoder::tryToPushFrameToDecoder(std::vector<uint8_t> &aacOrAdts) {
        IMFSample *sample;
        MFCreateSample(&sample);
        IMFMediaBuffer *buffer;
        MFCreateMemoryBuffer(aacOrAdts.size(), &buffer);

        BYTE *bufferData = NULL;
        buffer->SetCurrentLength(aacOrAdts.size());
        buffer->Lock(&bufferData, NULL, NULL);
        memcpy(bufferData, &aacOrAdts[0], aacOrAdts.size());
        buffer->Unlock();
        sample->AddBuffer(buffer);

        HRESULT res = mAACTransformer->ProcessInput(0, sample, 0);

        if (res == MF_E_NOTACCEPTING) {
            WASAPIImpl::write_message("AACDecoder: Decoder full when trying to add new frame for decode MF_E_NOTACCEPTING");
            return false;
        }
        else if (res != S_OK) {
            throw std::exception((std::string("Feeding data to AAC decoder failed.") + std::string(getErrorEnum(res))).c_str());
        }
        else {
            // WASAPIImpl::write_message("AACDecoder: Writing input frame to decoder was success");
        }

        sample->Release();
        buffer->Release();
        return true;
    }

    /**
    * Tries to read output from decoder, returns an empty vector if no output found.
    */
    std::vector<uint8_t> AACDecoder::readDecoderOutput() {
        std::vector<uint8_t> output;
        HRESULT res;

        do {
            // Get output stream info for creating output sample buffers..
            CHECK_HR_AND_THROW(mAACTransformer->GetOutputStreamInfo(0, &mOutStreamInfo), "Could not read output stream info");

            IMFMediaBuffer *outBuffer = NULL;
            IMFSample *outSample = NULL;
            MFCreateSample(&outSample);
            MFCreateMemoryBuffer((DWORD)mOutStreamInfo.cbSize, &outBuffer);
            outSample->AddBuffer(outBuffer);
            mOutputSample.pSample = outSample;

            DWORD dwFlags;
            res = mAACTransformer->ProcessOutput(0, 1, &mOutputSample, &dwFlags);

            if (res == S_OK) {
                BYTE* sampleData;
                DWORD sampleLen;
                CHECK_HR_AND_THROW(outBuffer->Lock(&sampleData, NULL, &sampleLen), "Could not lock outBuffer.");
                output.insert(output.end(), sampleData, sampleData + sampleLen);
                outBuffer->Unlock();
            }
            else if (res == MF_E_TRANSFORM_STREAM_CHANGE) {
                resetOutputType();
            }
            else if (res != MF_E_TRANSFORM_NEED_MORE_INPUT) {
                WASAPIImpl::write_message("AACDecoder Unexpected fail: %s", getErrorEnum(res));
                throw std::exception((std::string("Reading PCM data from decoder failed.") + std::string(getErrorEnum(res))).c_str());
            }
            else {
                // WASAPIImpl::write_message("AACDecoder: need more input frames");
            }

            outBuffer->Release();
            outSample->Release();
        } while (res == S_OK);

        return output;
    }

    void AACDecoder::resetOutputType() {
        // find suitable output type
        uint32_t i = 0;
        do {
            CHECK_HR_AND_THROW(mAACTransformer->GetOutputAvailableType(0, i, &mOutputType), "Could not find suitable output format");

            GUID typesAudioFormat;
            UINT32 typesChannels;
            UINT32 typesSampleRate;
            UINT32 typesSampleBits;

            mOutputType->GetGUID(MF_MT_SUBTYPE, &typesAudioFormat);
            mOutputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &typesChannels);
            // samplerate seems to always match with source samplerate
            mOutputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &typesSampleRate);
            mOutputType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &typesSampleBits);

            WASAPIImpl::write_message("AACDecoder: finding suitable output type (channels: %i sample rate: %i sample depth: %i)", typesChannels, typesSampleRate, typesSampleBits);

            if (
                typesAudioFormat == MFAudioFormat_PCM &&
                typesChannels == mOutputChannels &&
                typesSampleBits == mSampleBits
                ) break;
            i++;
        } while (true);
        CHECK_HR_AND_THROW(mAACTransformer->SetOutputType(0, mOutputType, 0), "Could not select output type");
    }

}

OMAF_NS_END


