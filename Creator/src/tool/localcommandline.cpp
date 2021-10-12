
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
#include <algorithm>
#include <fstream>
#include <functional>
#include <cassert>
#include <cstring>
#include <sstream>

#include "localcommandline.h"

// NOTE: This file will be removed once the symbol conflict issue between streamsegmenter and heif is corrected

namespace LocalCopy
{
    namespace CommandLine
    {
        ParseResult::ParseResult(OK)
            : mOK(true)
        {
            // nothing
        }

        ParseResult::ParseResult(Fail, std::string aError)
            : mOK(false)
            , mError(aError)
        {
            // nothing
        }

        ParseResult::ParseResult()
            : mOK(false)
            , mError("ParseResult not iniialized")
        {
            // nothing
        }

        bool ParseResult::isOK() const
        {
            return mOK;
        }

        std::string ParseResult::getError() const
        {
            return mError;
        }

        Parse::Parse(Optional<std::string> aArgumentName,
                     IsRequiredType aIsRequired,
                     CanBeRepeatedType aCanBeRepeated)
            : mArgumentName(aArgumentName)
            , mIsRequired(aIsRequired == IsRequired)
            , mCanBeRepeated(aCanBeRepeated == CanBeRepeated)
        {
            // nothing
        }

        Parse::~Parse() = default;

        Optional<std::string> Parse::argumentName() const
        {
            return mArgumentName;
        }

        bool Parse::isRequired() const
        {
            return mIsRequired;
        }

        bool Parse::canBeRepeated() const
        {
            return mCanBeRepeated;
        }

        String::String(std::string aArgumentName,
                       OptionallyReference<std::string> aResult)
            : Parse(aArgumentName, aResult.isRequired())
            , mResult(aResult)
        {
            // nothing
        }

        ParseResult String::parse(Arguments aArguments) const
        {
            mResult.set(aArguments[0]);
            return ParseResult::OK();
        }

        InputFilename::InputFilename(std::string aArgumentName,
            OptionallyReference<std::string> aResult)
            : Parse(aArgumentName, aResult.isRequired())
            , mResult(aResult)
        {
            // nothing
        }

        ParseResult InputFilename::parse(Arguments aArguments) const
        {
            std::ifstream input(aArguments[0]);
            if (input)
            {
                mResult.set(aArguments[0]);
                return ParseResult::OK();
            }
            else
            {
                return ParseResult(ParseResult::Fail(), "Cannot find input file " + aArguments[0]);
            }
        }

        Flag::Flag(bool& aResult)
            : Parse({}, IsOptional)
            , mResult(aResult)
        {
            // nothing
        }

        ParseResult Flag::parse(Arguments) const
        {
            mResult = true;
            return ParseResult::OK();
        }

        Arg::Arg(Optional<char> aSwitch, Optional<std::string> aName,
                 Parse* aParse, std::string aHelp)
            : mSwitch(aSwitch)
            , mName(aName)
            , mParse(aParse)
            , mHelp(aHelp)
        {
            // nothing
        }

        Arg::Arg(Arg&&) = default;

        Arg::~Arg() = default;

        Optional<char> Arg::getSwitch() const
        {
            return mSwitch;
        }

        Optional<std::string> Arg::getName() const
        {
            return mName;
        }

        std::string Arg::getHelp() const
        {
            return mHelp;
        }

        bool Arg::isPositional() const
        {
            return !mSwitch && !mName;
        }

        std::string Arg::getDescription() const
        {
            std::string str;
            if (mName)
            {
                str = "--" + *mName;
                if (mSwitch)
                {
                    str += " (-" + std::string(1, *mSwitch) + ")";
                }
            }
            else if (mSwitch)
            {
                str = "-" + std::string(1, *mSwitch) + "";
            }
            else
            {
                str = "remaining arguments";
            }
            return str;
        }

        Optional<std::string> Arg::argumentName() const
        {
            return mParse->argumentName();
        }

        bool Arg::isRequired() const
        {
            return mParse->isRequired();
        }

        bool Arg::canBeRepeated() const
        {
            return mParse->canBeRepeated();
        }

        ParseResult Arg::parse(Arguments aArguments) const
        {
            ParseResult result = mParse->parse(aArguments);
            if (!result.isOK())
            {
                result = ParseResult(ParseResult::Fail(), "Error while parsing " + getDescription() + ": " + result.getError());
            }
            return result;
        }

        ParseError::ParseError(std::string aError)
            : std::runtime_error("ParseError")
            , mError(aError)
        {
            // nothing
        }

        const char* ParseError::what() const noexcept
        {
            return mError.c_str();
        }

        Parser::Parser(std::string aAppName)
            : mAppName(aAppName)
        {
            // nothing
        }

        Parser::Parser(Parser&&) = default;

        Parser::~Parser() = default;

        Parser& Parser::add(Arg&& aArg)
        {
            mArgHolder.push_back(std::move(aArg));
            Arg* ptr = &mArgHolder.back();
            if (ptr->getSwitch())
            {
                mSwitches[*ptr->getSwitch()] = ptr;
            }
            if (ptr->getName())
            {
                mLongSwitches[*ptr->getName()] = ptr;
            }
            if (!ptr->getSwitch() && !ptr->getName())
            {
                assert(!mRemaining);
                mRemaining = ptr;
            }
            return *this;
        }

        ParseResult Parser::tryArgParse(const Arg& aArg, const Arguments aArguments)
        {
            try
            {
                return aArg.parse(aArguments);
            }
            catch (ParseError& parseError)
            {
                return ParseResult(ParseResult::Fail(),
                                   std::string("Failed to parse argument") +
                                   (aArg.argumentName() ? (std::string(" to ") + *aArg.argumentName()) : "") +
                                   std::string(": ") + parseError.what());
            }
        }

        ParseResult Parser::parse(const std::vector<std::string>& aArgs) const
        {
            ParseResult parseResult = ParseResult::OK();
            Arguments remaining;
            std::set<const Arg*> remainingRequiredArgs;
            for (const Arg& arg : mArgHolder)
            {
                if (arg.isRequired())
                {
                    remainingRequiredArgs.insert(&arg);
                }
            }
            for (size_t argIndex = 0; argIndex < aArgs.size() && parseResult.isOK(); ++argIndex)
            {
                const std::string& option = aArgs[argIndex];
                // we require at least one argument after '-'
                if (option.size() >= 1 && option[0] == '-')
                {
                    // special case for handling "stdin" argument
                    if (option == "-")
                    {
                        if (!mRemaining)
                        {
                            parseResult = ParseResult(ParseResult::Fail(), "Unexpected argument " + option);
                        }
                        else
                        {
                            parseResult = mRemaining->parse({ option });
                            remainingRequiredArgs.erase(mRemaining);
                        }
                    }
                    else if (option[1] == '-') // double-dash
                    {
                        std::string name = option.substr(2);
                        auto it = mLongSwitches.find(name);
                        if (it != mLongSwitches.end())
                        {
                            Arg& arg = *it->second;
                            if (arg.argumentName())
                            {
                                ++argIndex;
                                if (argIndex < aArgs.size())
                                {
                                    parseResult = tryArgParse(*it->second, { aArgs[argIndex] });
                                    remainingRequiredArgs.erase(it->second);
                                }
                                else
                                {
                                    parseResult = ParseResult(ParseResult::Fail(), "Missing argument to " + option);
                                }
                            }
                            else
                            {
                                parseResult = tryArgParse(*it->second, {});
                                remainingRequiredArgs.erase(it->second);
                            }
                        }
                        else
                        {
                            parseResult = ParseResult(ParseResult::Fail(), "Unknown switch " + option);
                        }
                    }
                    else
                    {
                        // -asdf arg1 arg2 arg3 arg4
                        for (size_t n = 0; n < option.size() - 1 && parseResult.isOK(); ++n)
                        {
                            char sw = option[n + 1];
                            auto it = mSwitches.find(sw);
                            if (it == mSwitches.end())
                            {
                                parseResult = ParseResult(ParseResult::Fail(), "Unknown switch -" + std::string(1, sw));
                            }
                            else
                            {
                                auto& arg = *it->second;
                                if (arg.argumentName())
                                {
                                    ++argIndex;
                                    if (argIndex < aArgs.size())
                                    {
                                        parseResult = tryArgParse(arg, { aArgs[argIndex] });
                                        remainingRequiredArgs.erase(&arg);
                                    }
                                    else
                                    {
                                        parseResult = ParseResult(ParseResult::Fail(), "Missing argument to switch " + std::string(1, sw));
                                    }
                                }
                                else
                                {
                                    parseResult = tryArgParse(arg, {});
                                    remainingRequiredArgs.erase(&arg);
                                }
                            }
                        }
                    }
                }
                else
                {
                    remaining.push_back(option);
                }
            }
            if (parseResult.isOK() && remaining.size())
            {
                if (mRemaining)
                {
                    parseResult = tryArgParse(*mRemaining, remaining);
                    remainingRequiredArgs.erase(mRemaining);
                }
                else
                {
                    parseResult = ParseResult(ParseResult::Fail(), "Unexpected arguments in command line");
                }
            }
            if (parseResult.isOK() && remainingRequiredArgs.size())
            {
                std::string remainingStr;
                for (auto* arg : remainingRequiredArgs)
                {
                    if (remainingStr.size())
                    {
                        remainingStr += ", ";
                    }
                    remainingStr += arg->getDescription();
                }
                parseResult = ParseResult(ParseResult::Fail(), "Following required parameters missing: " + remainingStr);
            }
            return parseResult;
        }

        namespace
        {
            struct CompareArgs
            {
                bool operator()(const Arg& l, const Arg& r) const
                {
                    // first parameters with names
                    // then with single-character switches go first,
                    // then the rest
                    return (l.getName() && !r.getName() ? true :
                            !l.getName() && r.getName() ? false :
                            l.getName() && r.getName() ? *l.getName() < *r.getName() :
                            l.getSwitch() && !r.getSwitch() ? true :
                            !l.getSwitch() && r.getSwitch() ? false :
                            l.getSwitch() && r.getSwitch() ? *l.getSwitch() < *r.getSwitch() :
                            false);
                }
            };

            std::string argUsageLabel(const Arg& aArg, bool aShortOnly = false)
            {
                std::ostringstream st;
                if (aArg.getName() && !aShortOnly)
                {
                    st << "--" << *aArg.getName();
                    if (aArg.argumentName())
                    {
                        st << " " << *aArg.argumentName();
                    }
                    if (aArg.getSwitch())
                    {
                        st << " | ";
                    }
                }
                if (aArg.getSwitch())
                {
                    st << "-" << *aArg.getSwitch();
                    if (aArg.argumentName())
                    {
                        st << " " << *aArg.argumentName();
                    }
                }
                return st.str();
            }
        } // anonymous namespace

        void Parser::usage(std::ostream& aStream) const
        {
            std::list<std::reference_wrapper<const Arg>> args { mArgHolder.begin(), mArgHolder.end() };
            args.sort(CompareArgs());

            aStream << "usage: " << mAppName;

            for (auto& arg : args)
            {
                const char* brackets = arg.get().isRequired() ? "()" : "{}";
                aStream << " " << brackets[0] << argUsageLabel(arg) << brackets[1];
                if (arg.get().canBeRepeated()) {
                    aStream << " {" << argUsageLabel(arg, true) << " ..}";
                }
            }

            aStream << std::endl;
            aStream << std::endl;

            size_t longest = 0;
            for (auto& arg : args)
            {
                longest = std::max(longest, argUsageLabel(arg).size());
            }

            for (auto& arg : args)
            {
                std::string label = argUsageLabel(arg);
                aStream << label
                        << std::string(longest - label.size() + 1, ' ')
                        << arg.get().getHelp()
                        << (arg.get().isRequired() ? " (required)" : " (optional)")
                        << std::endl;
            }
            aStream << std::endl;
        }
    } // namespace VDD::CommandLine
} // namespace VDD
