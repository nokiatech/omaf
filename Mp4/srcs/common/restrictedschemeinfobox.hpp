
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
#ifndef RESTRICTEDSCHEMEINFOBOX_HPP
#define RESTRICTEDSCHEMEINFOBOX_HPP

#include <cstdint>
#include <functional>
#include "bbox.hpp"
#include "compatibleschemetypebox.hpp"
#include "customallocator.hpp"
#include "originalformatbox.hpp"
#include "projectedomnivideobox.hpp"
#include "schemetypebox.hpp"
#include "stereovideobox.hpp"

/** @brief RestrictedSchemeInfo Box class.
 *  @details **/
class RestrictedSchemeInfoBox : public Box
{
public:
    RestrictedSchemeInfoBox();
    RestrictedSchemeInfoBox(const RestrictedSchemeInfoBox&);
    virtual ~RestrictedSchemeInfoBox() = default;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream&);

    /** @brief Parses a RestrictedSchemeInfoBox bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream&);

    virtual FourCCInt getOriginalFormat() const;
    virtual void setOriginalFormat(FourCCInt);

    virtual FourCCInt getSchemeType() const;

    SchemeTypeBox& getSchemeTypeBox() const;
    void addSchemeTypeBox(UniquePtr<SchemeTypeBox>);

    bool hasSchemeTypeBox() const;

    ProjectedOmniVideoBox& getProjectedOmniVideoBox() const;
    void addProjectedOmniVideoBox(UniquePtr<ProjectedOmniVideoBox>);

    StereoVideoBox& getStereoVideoBox() const;
    void addStereoVideoBox(UniquePtr<StereoVideoBox>);
    bool hasStereoVideoBox() const;

    Vector<CompatibleSchemeTypeBox*> getCompatibleSchemeTypes() const;
    void addCompatibleSchemeTypeBox(UniquePtr<CompatibleSchemeTypeBox>);

private:
    // using pointer to keep also information if a box was found
    UniquePtr<OriginalFormatBox> mOriginalFormatBox;
    UniquePtr<SchemeTypeBox> mSchemeTypeBox;
    UniquePtr<ProjectedOmniVideoBox> mProjectedOmniVideoBox;  // parsed only if scheme type is "podv"
    UniquePtr<StereoVideoBox> mStereoVideoBox;                // must exist in schi box iff scheme type is "stvi"
    Vector<UniquePtr<CompatibleSchemeTypeBox>> mCompatibleSchemeTypes;  // zero or more
};

#endif
