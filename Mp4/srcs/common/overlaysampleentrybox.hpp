
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
#ifndef OVERLAYSAMPLEENTRYBOX_HPP
#define OVERLAYSAMPLEENTRYBOX_HPP

#include "api/isobmff/commontypes.h"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "metadatasampleentrybox.hpp"
#include "overlayconfigbox.hpp"

/**
 * @brief OMAF OverlaySampleEntryBox class.
 */

class OverlaySampleEntryBox : public MetaDataSampleEntryBox
{
public:
    struct OverlayConfigSample
    {
        std::vector<std::uint16_t> activeOverlayIds;
        ISOBMFF::Optional<OverlayStruct> addlActiveOverlays;

        void write(ISOBMFF::BitStream& bitstr) const;

        void parse(ISOBMFF::BitStream& bitstr);
    };

    OverlaySampleEntryBox();
    virtual ~OverlaySampleEntryBox() = default;

    virtual OverlaySampleEntryBox* clone() const override;

    OverlayConfigBox& getOverlayConfig();

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    void writeBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Parses bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    void parseBox(ISOBMFF::BitStream& bitstr) override;


private:
    OverlayConfigBox mOverlayConfig;
};

#endif  // OVERLAYSAMPLEENTRYBOX_HPP
