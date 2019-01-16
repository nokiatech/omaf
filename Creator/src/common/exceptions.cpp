
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
#include "exceptions.h"

namespace VDD
{
    Exception::Exception(std::string aWhat) : std::runtime_error(aWhat.c_str()), mWhat(aWhat)
    {
        // nothing
    }

    Exception::~Exception() = default;;

    const char* Exception::what() const throw()
    {
        return mWhat.c_str();
    }

    std::string Exception::message() const
    {
        return mWhat;
    }

    CannotOpenFile::CannotOpenFile(std::string aFilename)
        : Exception("CannotOpenFile")
        , mFilename(aFilename)
    {
        // nothing
    }

    std::string CannotOpenFile::message() const
    {
        return "Cannot open " + mFilename;
    }
}

