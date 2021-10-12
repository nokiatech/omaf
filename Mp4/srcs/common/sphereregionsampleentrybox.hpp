
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
#ifndef SPHEREREGIONSAMPLEENTRYBOX_HPP
#define SPHEREREGIONSAMPLEENTRYBOX_HPP

#include "api/isobmff/commontypes.h"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "metadatasampleentrybox.hpp"
#include "sphereregionconfigbox.hpp"

/** @brief OMAF SphereRegionSampleEntryBox class.
 */

class SphereRegionSampleEntryBox : public MetaDataSampleEntryBox
{
public:
    struct SphereRegionSample
    {
        SphereRegionSample();

        // OMAF 7.7.2.3 says that there is always exactly 1 region, but for future this allows already multiple
        std::vector<SphereRegion> regions;
    };

    /**
     * In recommended viewport timed metadata sphere region config box decides
     * if each sample contain range or if it does not.
     */
    struct SphereRegionWithoutRangeSample : public SphereRegionSample
    {
        void write(ISOBMFF::BitStream& bitstr)
        {
            SphereRegion& region = regions.at(0);
            region.write(bitstr, false);
        }

        void parse(ISOBMFF::BitStream& bitstr)
        {
            SphereRegion& region = regions.at(0);
            region.parse(bitstr, false);
        }
    };

    struct SphereRegionWithRangeSample : public SphereRegionSample
    {
        void write(ISOBMFF::BitStream& bitstr)
        {
            SphereRegion& region = regions.at(0);
            region.write(bitstr, true);
        }

        void parse(ISOBMFF::BitStream& bitstr)
        {
            SphereRegion& region = regions.at(0);
            region.parse(bitstr, true);
        }
    };


    SphereRegionSampleEntryBox(FourCCInt codingname);
    virtual ~SphereRegionSampleEntryBox() = default;

    SphereRegionConfigBox& getSphereRegionConfig();

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);


private:
    SphereRegionConfigBox mSphereRegionConfig;
};

#endif  // SPHEREREGIONSAMPLEENTRYBOX_HPP
