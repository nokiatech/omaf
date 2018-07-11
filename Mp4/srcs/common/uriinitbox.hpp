
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
#ifndef URIINITBOX_HPP
#define URIINITBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

/**
 * @brief  URI Init Box class
 * @details 'uriI' box implementation as specified in the ISOBMFF specification.
 */
class UriInitBox : public FullBox
{
public:
    UriInitBox();
    virtual ~UriInitBox() = default;

    enum class InitBoxType
    {
        UNKNOWN = 0
    };

    /** @brief Gets the type of timed meta data.
     *  @return UriMetaSampleEntryBox::TMDType enumeration**/
    InitBoxType getInitBoxType() const;

    /** @brief Sets the type of timed meta data.
     *  @return UriMetaSampleEntryBox::TMDType enumeration**/
    void setInitBoxType(InitBoxType dataType);

    /** @brief Set Uri Initialization Data.
     *  @param [in] Reference to the Uri Initialization Data vector, uriInitializationData.**/
    void setUriInitializationData(const Vector<std::uint8_t>& uriInitData);

    /** @brief Get Uri Initialization Data.
     *  @returns Vector of uint8_t containing Uri Initialization Data**/
    Vector<std::uint8_t> getUriInitializationData() const;

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
    InitBoxType mInitBoxType;
    Vector<std::uint8_t> mUriInitializationData;
};

#endif /* end of include guard: URIBOX_HPP */
