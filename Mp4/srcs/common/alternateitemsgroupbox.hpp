
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
#ifndef ALTERNATEITEMSGROUPBOX_HPP
#define ALTERNATEITEMSGROUPBOX_HPP

#include "customallocator.hpp"
#include "entitytogroupbox.hpp"

/**
 * Alternate Items GroupBox "altr" entity to group Box class
 *
 * Alternate items group by ISOBMFF 8.18.3.1 Definition
 *
 * @details Entity group box implementation as specified in the ISOBMFF specification.
 */
class AlternateItemsGroupBox : public EntityToGroupBox
{
public:
    AlternateItemsGroupBox();
    virtual ~AlternateItemsGroupBox() = default;
};

#endif /* ALTERNATEITEMSGROUPBOX_HPP */
