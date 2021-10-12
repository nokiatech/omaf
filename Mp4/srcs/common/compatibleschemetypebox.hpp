
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
#ifndef COMPATIBLESCHEMETYPEBOX_HPP
#define COMPATIBLESCHEMETYPEBOX_HPP

#include "schemetypebox.hpp"


/** @brief CompatibleSchemeType from OMAFv1 specs
 *  @details The semantics of the syntax elements are identical to the semantics of the syntax elements with the same
 * name in SchemeTypeBox.
 */
class CompatibleSchemeTypeBox : public SchemeTypeBox
{
public:
    // NOTE: this should not be descendant of schemetype, but both should inherit common base implementation,
    //       if that causes any problems it shoud be fixed
    CompatibleSchemeTypeBox();
    virtual ~CompatibleSchemeTypeBox() = default;
};

#endif /* end of include guard: COMPATIBLESCHEMETYPEBOX_HPP */
