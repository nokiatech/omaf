
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
#ifndef SPHEREREGIONSAMPLEENTRYBOX_HPP
#define SPHEREREGIONSAMPLEENTRYBOX_HPP

#include "bitstream.hpp"
#include "commontypes.hpp"
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
        // OMAF 7.7.2.3 says that there is always exactly 1 region, but for future this allows already multiple
        std::vector<SphereRegion> regions;
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
