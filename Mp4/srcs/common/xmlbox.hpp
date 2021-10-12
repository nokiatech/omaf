
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
#ifndef XMLBOX_HPP
#define XMLBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

#include <string>

/**
 * @brief  XML Box class
 * @details 'xml ' box implementation as specified in the ISOBMFF specification.
 */
class XmlBox : public FullBox
{
public:
    XmlBox();
    virtual ~XmlBox() = default;

    /** @brief Sets the XML of the URIBox.
     *  @param [in] XML field as a string value  **/
    void setContents(const String& xml);

    /** @brief Get XML of the URIBox.
     *  @return name field as a string value  **/
    const String& getContents() const;

    /**
     * @brief Serialize box data to the ISOBMFF::BitStream.
     * @see Box::writeBox()
     */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /**
     * @brief Deserialize box data from the ISOBMFF::BitStream.
     * @see Box::parseBox()
     */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    String mContents;  // the XML as specified in the ISOBMFF specification//
};

#endif /* end of include guard: XMLBOX_HPP */
