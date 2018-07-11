
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
#include <algorithm>

#include "aacimport.h"
#include "common/exceptions.h"
#include "common/bitparser.h"
#include "common/bitgenerator.h"

namespace VDD
{
    UnsupportedFormat::UnsupportedFormat()
        : Exception("UnsupportedFormat")
    {
        // nothing
    }

    namespace {
        // https://wiki.multimedia.cx/index.php/MPEG-4_Audio#Audio_Object_Types
        enum class AudioObjectType {
            Null,               // 0
            AAC_Main,           // 1
            AAC_LC,             // 2 (Low Complexity)
            AAC_SSR,            // 3 (Scalable Sample Rate)
            AAC_LTP,            // 4 (Long Term Prediction)
            SBR,                // 5 (Spectral Band Replication)
            AAC_Scalable,       // 6
        };

        // Could also be 960, but it seems 1024 is a lot more popular and 960 isn't supported by some pretty major
        // software (ie. Windows or FFmpeg don't support it.). The number cannot be determined from the ADTS header.
        const std::uint64_t kNumberOfSamplesInFrame = 1024;

        // https://wiki.multimedia.cx/index.php/MPEG-4_Audio#Sampling_Frequencies
        const std::uint32_t kAdtsFrequencyTable[] = {
            96000,
            88200,
            64000,
            48000,
            44100,
            32000,
            24000,
            22050,
            16000,
            12000,
            11025,
            8000,
            7350,
            0,
            0,
            0 };

        const uint8_t kInvalidFrequencyIndex = 255;

        uint8_t findAdtsFrequncyIndex(uint32_t aHz)
        {
            auto at = std::find(std::begin(kAdtsFrequencyTable),
                                std::end(kAdtsFrequencyTable),
                                aHz);
            if (at == std::end(kAdtsFrequencyTable))
            {
                return kInvalidFrequencyIndex;
            }
            else
            {
                return at - std::begin(kAdtsFrequencyTable);
            }
        }

        struct ADTS {
            unsigned length;
            uint32_t frequency;
            uint8_t channelConfig;
            bool hasCrc;
        };

        template <typename T>
        bool parseAdts(BitParser<T>&& parser, ADTS& adts)
        {
            adts = {};
            /* https://wiki.multimedia.cx/index.php?title=ADTS */
            uint32_t a = parser.readn(12); // syncword 0xFFF, all bits must be 1
            if (a != 0xfff) return false;
            uint8_t b = parser.read1();    // MPEG Version: 0 for MPEG-4, 1 for MPEG-2
            if (b != 0) return false;
            uint32_t c = parser.readn(2);  // Layer: always 0
            if (c != 0) return false;
            uint32_t d = parser.readn(1);  // protection absent, Warning, set to 1 if there is no CRC and 0 if there is CRC
            adts.hasCrc = d == 0;
            /* uint32_t e = */ parser.readn(2);  // profile, the MPEG-4 Audio Object Type minus 1
            uint32_t frequencyIndex = parser.readn(4); // MPEG-4 Sampling Frequency Index (15 is forbidden)
            if (frequencyIndex < sizeof(kAdtsFrequencyTable) / sizeof(*kAdtsFrequencyTable))
            {
                adts.frequency = kAdtsFrequencyTable[frequencyIndex];
            }
            /* uint32_t g = */ parser.readn(1);  // private bit, guaranteed never to be used by MPEG, set to 0 when encoding, ignore when decoding
            adts.channelConfig = static_cast<uint8_t>(parser.readn(3));  // MPEG-4 Channel Configuration (in the case of 0, the channel configuration is sent via an inband PCE)
            /* uint32_t i = */ parser.readn(1);  // originality, set to 0 when encoding, ignore when decoding
            /* uint32_t j = */ parser.readn(1);  // home, set to 0 when encoding, ignore when decoding
            /* uint32_t k = */ parser.readn(1);  // copyrighted id bit, the next bit of a centrally registered copyright identifier, set to 0 when encoding, ignore when decoding
            /* uint32_t l = */ parser.readn(1);  // copyright id start, signals that this frame's copyright id bit is the first bit of the copyright id, set to 0 when encoding, ignore when decoding
            uint32_t m = parser.readn(13); // frame length, this value must include 7 or 9 bytes of header length: FrameLength = (ProtectionAbsent == 1 ? 7 : 9) + size(AACFrame)
            adts.length = m - (adts.hasCrc ? 9 : 7);
            /* uint32_t o = */ parser.readn(11); // Buffer fullness
            /* uint32_t p = */ parser.readn(2);  // Number of AAC frames (RDBs) in ADTS frame minus 1, for maximum compatibility always use 1 AAC frame per ADTS frame

            // this is read afterwards if it exists (hasCrc)
            //uint8_t q = parser.readn(16);  //CRC if protection absent is 0

            return parser.eod();
        }

        struct AudioSpecificConfig {
            AudioObjectType objectType;
            uint32_t frequency;
            uint8_t channelConfig;
            std::vector<std::uint8_t> aotSpecifcConfig;
        };

        // https://wiki.multimedia.cx/index.php/MPEG-4_Audio#Audio_Specific_Config
        std::vector<uint8_t> audioSpecificConfigToBytes(const AudioSpecificConfig& aCfg)
        {
            BitGenerator g;
            if (uint8_t(aCfg.objectType) >= 32)
            {
                g.uintn(5, 31);
                g.uintn(6, uint8_t(aCfg.objectType) - 32);
            }
            else
            {
                g.uintn(5, uint8_t(aCfg.objectType));
            }
            uint8_t freqIdx = findAdtsFrequncyIndex(aCfg.frequency);
            if (freqIdx != kInvalidFrequencyIndex)
            {
                g.uintn(4, freqIdx);
            }
            else
            {
                g.uintn(4, 15);
                g.uintn(24, aCfg.frequency);
            }
            g.uintn(4, aCfg.channelConfig);
            for (std::uint8_t byte: aCfg.aotSpecifcConfig)
            {
                g.uintn(8, byte);
            }
            return g.moveData();
        }

        Bitrate determineBitrate(std::ifstream& aFile)
        {
            char adtsBuf[7];
            bool ok = true;
            FrameDuration fileDuration;
            std::uint64_t numberOfBytes = 0;
            std::uint64_t maxBitrate = 0;
            ADTS adts;

            aFile.read(adtsBuf, sizeof(adtsBuf));
            numberOfBytes += sizeof(adtsBuf);
            if (!aFile)
            {
                ok = false;
            }
            else if (!parseAdts(BitParser<const char*>(adtsBuf, adtsBuf + sizeof(adtsBuf)), adts)
                     || adts.frequency == 0)
            {
                throw UnsupportedFormat();
            }
            else
            {
                if (adts.hasCrc)
                {
                    char crc[2];
                    numberOfBytes += 2;
                    aFile.read(crc, 2);
                    ok = !!aFile;
                }
                else
                {
                    ok = true;
                }
                if (ok){
                    aFile.seekg(aFile.tellg() + std::streampos(adts.length));

                    ok = !!aFile;
                    if (ok)
                    {
                        auto frameDuration = FrameDuration(kNumberOfSamplesInFrame, adts.frequency);
                        numberOfBytes += adts.length;
                        fileDuration += frameDuration;
                        uint32_t currentBitrate = static_cast<uint32_t>(adts.length * 8 / frameDuration.asDouble());
                        if (currentBitrate > maxBitrate)
                        {
                            maxBitrate = currentBitrate;
                        }
                    }
                }
                if (!ok)
                {
                    std::cerr << "AACImport: premature end of AAC file" << std::endl;
                }
            }

            Bitrate bitrate;
            bitrate.avgBitrate = static_cast<uint32_t>(numberOfBytes * 8 / fileDuration.asDouble());
            bitrate.maxBitrate = maxBitrate;

            return bitrate;
        }
    }

    AACImport::AACImport(Config aConfig)
        : mConfig(aConfig)
        , mInput(aConfig.dataFilename, std::ifstream::binary)
    {
        if (!mInput)
        {
            throw CannotOpenFile(aConfig.dataFilename);
        }
        else
        {
            mBitrate = determineBitrate(mInput);
            mInput.clear();
            mInput.seekg(std::streampos(0));
        }
    }

    AACImport::~AACImport() = default;

    std::vector<Views> AACImport::produce()
    {
        if (isAborted())
        {
            return {{ Data(EndOfStream()) }};
        }

        std::vector<Views> frames;
        char adtsBuf[7];
        bool ok = true;
        mInput.read(adtsBuf, sizeof(adtsBuf));
        ADTS adts;
        if (!mInput)
        {
            ok = false;
        }
        else if (!parseAdts(BitParser<const char*>(adtsBuf, adtsBuf + sizeof(adtsBuf)), adts)
                 || adts.frequency == 0)
        {
            throw UnsupportedFormat();
        }
        else
        {
            if (adts.hasCrc)
            {
                char crc[2];
                mInput.read(crc, 2);
                ok = !!mInput;
            }
            else
            {
                ok = true;
            }
            if (ok){
                std::vector<uint8_t> buffer(adts.length);
                mInput.read(reinterpret_cast<char*>(&buffer[0]), adts.length);

                ok = !!mInput;
                if (ok)
                {
                    CPUDataVector dataRef({ std::move(buffer) });
                    CodedFrameMeta meta;
                    AudioSpecificConfig asc {};
                    asc.objectType = AudioObjectType::AAC_LC;
                    asc.frequency = adts.frequency;
                    asc.channelConfig = adts.channelConfig;
                    /* no AOT */

                    meta.format = CodedFormat::AAC;
                    meta.duration = FrameDuration(kNumberOfSamplesInFrame, adts.frequency);
                    meta.channelConfig = adts.channelConfig;
                    meta.samplingFrequency = adts.frequency;
                    meta.type = FrameType::IDR;
                    meta.bitrate = mBitrate;
                    meta.presIndex = mIndex;
                    meta.inCodingOrder = true;
                    ++mIndex;
                    meta.decoderConfig.insert({ ConfigType::AudioSpecificConfig,
                                                audioSpecificConfigToBytes(asc) });
                    frames.push_back({ Data(std::move(dataRef), meta) });
                }
            }
            if (!ok)
            {
                std::cerr << "AACImport: premature end of AAC file" << std::endl;
            }
        }

        if (!ok)
        {
            frames.push_back({ Data(EndOfStream()) });
        }
        return frames;
    }
}
