
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
#ifndef SCHEMETYPEBOX_HPP
#define SCHEMETYPEBOX_HPP

#include <cstdint>
#include "fullbox.hpp"

/** @brief SchemeTypeBox Contains information about what kind of data is found inside SchemeInfoBox
 *  @details Defined in the ISOBMFF standard. **/
class SchemeTypeBox : public FullBox
{
public:
    SchemeTypeBox();
    virtual ~SchemeTypeBox() = default;

    void setSchemeType(FourCCInt);
    FourCCInt getSchemeType() const;

    void setSchemeVersion(std::uint32_t);
    std::uint32_t getSchemeVersion() const;

    void setSchemeUri(const String& uri);
    const String& getSchemeUri() const;

    /// @see Box::writeBox()
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /// @see Box::parseBox()
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    FourCCInt mSchemeType;
    uint32_t mSchemeVersion;
    String mSchemeUri;
};

#endif
