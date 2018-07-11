
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
#ifndef COMMON_EXCEPTIONS_H
#define COMMON_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace VDD
{
    class Exception : public std::runtime_error
    {
    public:
        Exception(std::string aWhat);
        virtual ~Exception();

        const char* what() const throw() override;

        // override this for better exception messages
        virtual std::string message() const;

    private:
        const std::string mWhat;
    };

    class CannotOpenFile : public Exception
    {
    public:
        CannotOpenFile(std::string filename);

        std::string message() const override;

    private:
        std::string mFilename;
    };
}

#endif  // COMMON_EXCEPTIONS_H
