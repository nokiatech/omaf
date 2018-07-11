
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
#ifndef URIBOX_HPP
#define URIBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

#include <string>

/**
 * @brief  URI Box class
 * @details 'uri ' box implementation as specified in the ISOBMFF specification.
 */
class UriBox : public FullBox
{
public:
    UriBox();
    virtual ~UriBox() = default;

    /** @brief Sets the URI of the URIBox.
     *  @param [in] URI field as a string value  **/
    void setUri(const String& uri);

    /** @brief Get URI of the URIBox.
     *  @return name field as a string value  **/
    const String& getUri() const;

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
    String mUri;  // the URI as specified in the ISOBMFF specification//
};

#endif /* end of include guard: URIBOX_HPP */
