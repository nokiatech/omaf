
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

#include <memory>
#include <vector>

#include "tileproxy.h"
#include "parser/bitstream.hpp"
#include "parser/h265parser.hpp"


namespace VDD {

    class TileProxyMultiRes : public TileProxy
    {
    public:
        TileProxyMultiRes(Config config);
        virtual ~TileProxyMultiRes() = default;

    protected:
        virtual void processExtractors(std::vector<Streams>& aTiles) override;
        virtual void createEoS(std::vector<Streams>& aTiles) override;

    private:
        Data collectExtractors(TileDirectionConfig& aDirection, bool aFirstPacket);
        void convertSliceHeader(std::vector<std::uint8_t>& aNaluHeader, H265::SliceHeader& aOldHeaderParsed, Parser::BitStream& aNewSliceHeader, uint64_t aCtuId,
            const H265::SequenceParameterSet& aSps, const H265::PictureParameterSet& aPps);

    private:
        std::unique_ptr<H265Parser>  mH265Parser = nullptr;
        bool mFirstExtractorRound = true;
    };

}
