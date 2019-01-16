
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
#include "mp4vrhvc2extractor.hpp"
#include "bitstream.hpp"
#include "log.hpp"

namespace Hvc2Extractor
{
    bool parseExtractorNal(const DataVector& NalData,
                           ExtractorSample& extractorSample,
                           uint8_t lengthSizeMinus1,
                           uint64_t& extractionSize)
    {
        BitStream nalUnits(NalData);
        ExtNalHdr extNalHdr;
        uint32_t extractors = 0;
        uint64_t inlineSizes = 0;
        std::size_t order_idx = 1;

        while (nalUnits.numBytesLeft() > 0)
        {
            // depends on lengthSizeMinus1 ISO/IEC FDIS 14496-15:2014(E) 4.3.3.1
            std::size_t counter = 0;

            switch (lengthSizeMinus1)
            {
            case 0:
                counter = nalUnits.read8Bits();
                break;
            case 1:
                counter = nalUnits.read16Bits();
                break;
            case 3:
                counter = nalUnits.read32Bits();
                break;
            default:
                throw std::runtime_error("Invalid lengthSizeMinus1 while parsing extractor");
            }

            // read NALUnitHeader() with two bytes;
            extNalHdr.forbidden_zero_bit    = (uint8_t) nalUnits.readBits(1);
            extNalHdr.nal_unit_type         = (uint8_t) nalUnits.readBits(6);
            extNalHdr.nuh_layer_id          = (uint8_t) nalUnits.readBits(6);
            extNalHdr.nuh_temporal_id_plus1 = (uint8_t) nalUnits.readBits(3);

            counter = counter - 2;  // 2 bytes read;
            if (extNalHdr.nal_unit_type == 49)
            {
                ExtractorSample::Extractor extractor;
                for (; counter > 0; counter--)
                {
                    std::uint8_t constructor_type = (uint8_t) nalUnits.readBits(8);

                    switch (constructor_type)
                    {
                    case 0:
                        logInfo() << "Sample Construct" << std::endl;
                        ExtractorSample::SampleConstruct sampleConstruct;
                        sampleConstruct.order_idx        = (uint8_t) order_idx;
                        sampleConstruct.constructor_type = (uint8_t) constructor_type;
                        sampleConstruct.track_ref_index  = (uint8_t) nalUnits.readBits(8);
                        counter--;
                        // track_ref_index starts with one in ISOBMFF but MP4VR reader has index starting from 0
                        sampleConstruct.track_ref_index = sampleConstruct.track_ref_index - 1;
                        sampleConstruct.sample_offset   = (int8_t) nalUnits.readBits(8);
                        counter--;
                        sampleConstruct.data_offset = nalUnits.readBits((lengthSizeMinus1 + 1) * 8);
                        counter -= (lengthSizeMinus1 + 1);
                        sampleConstruct.data_length = nalUnits.readBits((lengthSizeMinus1 + 1) * 8);
                        counter -= (lengthSizeMinus1 + 1);
                        if (sampleConstruct.data_length < UINT32_MAX)
                        {
                            extractionSize += sampleConstruct.data_length;
                        }
                        extractor.sampleConstruct.push_back(sampleConstruct);
                        order_idx = order_idx + 1;
                        break;
                    case 2:
                        logInfo() << "Inline Construct" << std::endl;
                        ExtractorSample::InlineConstruct inlineConstruct;
                        inlineConstruct.order_idx        = (uint8_t) order_idx;
                        inlineConstruct.constructor_type = (uint8_t) constructor_type;
                        inlineConstruct.data_length      = (uint8_t) nalUnits.readBits(8);
                        for (std::uint8_t i = 0; i < inlineConstruct.data_length; i++)
                        {
                            inlineConstruct.inline_data.push_back((uint8_t) nalUnits.readBits(8));
                        }
                        inlineSizes += inlineConstruct.data_length;
                        extractor.inlineConstruct.push_back(inlineConstruct);
                        counter   = counter - inlineConstruct.data_length - 1;  // Bytes read in inline construct
                        order_idx = order_idx + 1;
                        break;
                    }
                }
                extractorSample.extractors.push_back(extractor);
                extractors++;
            }
            else
            {
                // ignore it. It may be e.g. SEI NAL for OMAF, but we don't need it there.
                logInfo() << "Other than extractor NAL found, type: " << extNalHdr.nal_unit_type << std::endl;
                nalUnits.skipBytes(counter);
            }
        }
        if (extractors)
        {
            if (extractionSize > 0) //note that if constructor has UINT32_MAX as data_length, the extraction size is not incremented above, and it is hence 0 if all are UINT32_MAX
            {
                extractionSize += inlineSizes;
            }
            return true;
        }
        else
        {
            return false;
        }
    }
}  // namespace Hvc2Extractor
