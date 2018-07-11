
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
#ifndef PROJECTEDOMNIVIDEOBOX_HPP
#define PROJECTEDOMNIVIDEOBOX_HPP

#include <cstdint>
#include "bbox.hpp"
#include "coverageinformationbox.hpp"
#include "projectionformatbox.hpp"
#include "regionwisepackingbox.hpp"
#include "rotationbox.hpp"

/** @brief Container box having all the information required by podv scheme type
 *  @details Defined in the OMAF standard. **/
class ProjectedOmniVideoBox : public Box
{
public:
    ProjectedOmniVideoBox();
    ProjectedOmniVideoBox(const ProjectedOmniVideoBox&);
    virtual ~ProjectedOmniVideoBox() = default;

    ProjectionFormatBox& getProjectionFormatBox();

    RegionWisePackingBox& getRegionWisePackingBox();
    void setRegionWisePackingBox(UniquePtr<RegionWisePackingBox>);
    bool hasRegionWisePackingBox() const;

    CoverageInformationBox& getCoverageInformationBox();
    void setCoverageInformationBox(UniquePtr<CoverageInformationBox>);
    bool hasCoverageInformationBox() const;

    RotationBox& getRotationBox();
    void setRotationBox(UniquePtr<RotationBox>);
    bool hasRotationBox() const;

    /** @see Box::writeBox()
     */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @see Box::parseBox()
     */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);


    /** @brief Dump box data to logInfo() in human readable format.
     */
    void dump() const;

private:
    ProjectionFormatBox mProjectionFormatBox;
    UniquePtr<RegionWisePackingBox> mRegionWisePackingBox;
    UniquePtr<CoverageInformationBox> mCoverageInformationBox;
    UniquePtr<RotationBox> mRotationBox;
};

#endif
