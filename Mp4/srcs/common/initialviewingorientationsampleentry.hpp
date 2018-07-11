
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
#ifndef INITIALVIEWINGORIENTETAIONSAMPLEENTRYBOX_HPP
#define INITIALVIEWINGORIENTETAIONSAMPLEENTRYBOX_HPP

#include "bitstream.hpp"
#include "commontypes.hpp"
#include "customallocator.hpp"
#include "sphereregionsampleentrybox.hpp"


/** @brief OMAF InitialViewingOrientation sample description entry.
 */

class InitialViewingOrientation : public SphereRegionSampleEntryBox
{
public:
    struct InitialViewingOrientationSample : SphereRegionSample
    {
        bool refreshFlag = false;

        InitialViewingOrientationSample();
    };

    InitialViewingOrientation();
    virtual ~InitialViewingOrientation() = default;

    virtual InitialViewingOrientation* clone() const override;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
};

#endif  // INITIALVIEWINGORIENTETAIONSAMPLEENTRYBOX_HPP
