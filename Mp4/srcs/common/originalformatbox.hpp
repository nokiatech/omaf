
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
#ifndef ORIGINALFORMATBOX_HPP
#define ORIGINALFORMATBOX_HPP

#include "bbox.hpp"
#include "customallocator.hpp"

/**
 * @brief OriginalFormatBox
 * @details 'frma' box implementation as specified in the ISOBMFF specification.
 */
class OriginalFormatBox : public Box
{
public:
    OriginalFormatBox();
    virtual ~OriginalFormatBox() = default;

    FourCCInt getOriginalFormat() const;
    void setOriginalFormat(FourCCInt);

    virtual void writeBox(ISOBMFF::BitStream& bitstr);
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    FourCCInt mOriginalFormat;
};

#endif /* end of include guard: ORIGINALFORMATBOX_HPP */
