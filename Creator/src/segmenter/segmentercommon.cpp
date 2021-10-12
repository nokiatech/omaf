
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
#include "segmentercommon.h"

namespace VDD
{
    StreamSegmenter::Segmenter::WriterMode writerModeOfOutputMode(OutputMode aOutputMode)
    {
        StreamSegmenter::Segmenter::WriterMode writerMode =
            StreamSegmenter::Segmenter::WriterMode::CLASSIC;
        switch (aOutputMode)
        {
        case OutputMode::None:
        case OutputMode::OMAFV1:
            writerMode = StreamSegmenter::Segmenter::WriterMode::CLASSIC;
            break;
        case OutputMode::OMAFV2:
            writerMode = StreamSegmenter::Segmenter::WriterMode::OMAFV2;
            break;
        }
        return writerMode;
    }

    std::vector<std::uint8_t> makeXtyp(const SegmentHeader& aSegmentHeader)
    {
        std::vector<std::uint8_t> box;
        box.push_back(uint8_t(0));
        box.push_back(uint8_t(0));
        box.push_back(uint8_t(0));
        box.push_back(uint8_t(0));

        const char styp[5]{"styp"};
        const char ftyp[5]{"ftyp"};
        const char sibm[5]{"sibm"};
        const char imds[5]{"imds"};

        switch (aSegmentHeader.type)
        {
        case Xtyp::Styp:
        {
            std::copy(styp + 0, styp + 4, std::back_inserter(box));
        }
        break;
        case Xtyp::Ftyp:
        {
            std::copy(ftyp + 0, ftyp + 4, std::back_inserter(box));
        }
        break;
        case Xtyp::Sibm:
        {
            std::copy(sibm + 0, sibm + 4, std::back_inserter(box));
        }
        break;
        case Xtyp::Imds:
        {
            std::copy(imds + 0, imds + 4, std::back_inserter(box));
        }
        break;
        }

        std::copy(aSegmentHeader.brand4cc.begin(), aSegmentHeader.brand4cc.end(), std::back_inserter(box));

        box.push_back(uint8_t(0));
        box.push_back(uint8_t(0));
        box.push_back(uint8_t(0));
        box.push_back(uint8_t(0));

        std::copy(aSegmentHeader.brand4cc.begin(), aSegmentHeader.brand4cc.end(), std::back_inserter(box));

        auto size = box.size();
        box[0] = uint8_t((size >> 24) & 0xff);
        box[1] = uint8_t((size >> 16) & 0xff);
        box[2] = uint8_t((size >> 8) & 0xff);
        box[3] = uint8_t((size >> 0) & 0xff);

        return box;
    }
}  // namespace VDD