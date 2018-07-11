
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
#ifndef STREAMSEGMENTER_EXCEPTIONS_HPP
#define STREAMSEGMENTER_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace StreamSegmenter
{
    /// The Error base class, used by all other exceptions introduced here
    class Error : public std::exception
    {
    public:
        Error(std::string aWhat)
            : mWhat(aWhat)
        {
        }

/// what kind of error; programmer-readable
#if defined(_MSC_VER) && _MSC_VER <= 1800
        const char* what() const
        {
            return mWhat.c_str();
        }
#else
        const char* what() const noexcept
        {
            return mWhat.c_str();
        }
#endif

        // user-facing message. defaults to what(), if not overridden.
        virtual std::string message() const
        {
            return what();
        }

    private:
        std::string mWhat;
    };

    class PrematureEndOfFile : public Error
    {
    public:
        PrematureEndOfFile()
            : Error("PrematureEndOfFile")
        {
        }
    };

    /** ConfigErrors are thrown when reading the configuration but also
    when the configuration is being processed while constructing the
    Creator. */
    class ConfigError : public Error
    {
    public:
        ConfigError(std::string aStr)
            : Error(aStr)
        {
        }
    };

    /** ConfigValueError indicates an error that can be attributed to a
    certain input configuration value */
    class ConfigValueError : public ConfigError
    {
    public:
        ConfigValueError(std::string aStr, std::string aPath);

        std::string getPath() const;

    private:
        std::string mPath;
    };

    /** ConfigInvalidValueError indicates an error that can be attributed
    to a certain input configuration value value an invalid value */
    class ConfigInvalidValueError : public ConfigValueError
    {
    public:
        ConfigInvalidValueError(std::string path, std::string explanation)
            : ConfigValueError("invalid value error", path)
            , mExplanation(explanation)
        {
        }

        virtual std::string message() const override
        {
            return "Invalid value for configuration key: " + getPath() + " (expected " + mExplanation + ")";
        }

    private:
        std::string mExplanation;
    };

    /** ConfigMissingValueError indicates an input configuration key was expected
    but it was missing */
    class ConfigMissingValueError : public ConfigValueError
    {
    public:
        ConfigMissingValueError(std::string path)
            : ConfigValueError("missing value error", path)
        {
        }

        virtual std::string message() const override
        {
            return "Configuration key missing: " + getPath();
        }
    };
}

#endif  // STREAMSEGMENTER_EXCEPTIONS_HPP
