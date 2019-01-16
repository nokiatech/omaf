
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
#pragma once

#include <list>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <functional>

#include "common/optional.h"


namespace VDD
{
    namespace CommandLine
    {
        using Arguments = std::vector<std::string>;

        class ParseResult
        {
        public:
            struct OK {};
            struct Fail {};

            ParseResult();
            ParseResult(OK);
            explicit ParseResult(Fail, std::string aError);

            bool isOK() const;

            std::string getError() const;

        private:
            bool mOK;
            std::string mError;
        };

        class ParseError : public std::runtime_error
        {
        public:
            ParseError(std::string aError);

            const char* what() const noexcept override;

        private:
            std::string mError;
        };

        class Parse
        {
        public:
            enum IsRequiredType
            {
                IsRequired,
                IsOptional
            };

            enum CanBeRepeatedType
            {
                CannotBeRepeated,
                CanBeRepeated
            };

            Parse(Optional<std::string> aArgumentName,
                  IsRequiredType aIsRequired,
                  CanBeRepeatedType aCanBeRepeated = CannotBeRepeated);
            virtual ~Parse();

            Optional<std::string> argumentName() const;
            bool isRequired() const;
            bool canBeRepeated() const;

            virtual ParseResult parse(Arguments aArguments) const = 0;

        private:
            Optional<std::string> mArgumentName;
            bool mIsRequired;
            bool mCanBeRepeated;
        };

        template <typename T>
        class OptionallyReference
        {
        public:
            OptionallyReference(Optional<T>& aResult);
            OptionallyReference(T& aResult);
            OptionallyReference(T& aResult, const T& aDefaultValue);

            void set(const T& value) const;
            T& get() const;
            Parse::IsRequiredType isRequired() const;

        private:
            // one of these pointers is set
            Optional<T>* mResultOptional = nullptr;
            T* mResult = nullptr;
            Parse::IsRequiredType mIsRequired;
        };

        class String : public Parse
        {
        public:
            typedef std::string ResultType;

            String(std::string aArgumentName,
                   OptionallyReference<std::string> aResult);

            ParseResult parse(Arguments aArguments) const override;

        private:
            OptionallyReference<std::string> mResult;
        };

        template <typename SubParse, typename Container>
        class Accumulate : public Parse {
        public:
            typedef typename SubParse::ResultType ResultType;

            Accumulate(std::string aArgumentName,
                       OptionallyReference<Container> aResult);
            Accumulate(std::string aArgumentName,
                       OptionallyReference<Container> aResult,
                       std::function<typename Container::value_type(typename SubParse::ResultType)> aMapping);

            ParseResult parse(Arguments aArguments) const override;

        private:
            OptionallyReference<Container> mContainer;
            std::function<typename Container::value_type(typename SubParse::ResultType)> mMapping;
        };

        class InputFilename : public Parse
        {
        public:
            typedef std::string ResultType;

            InputFilename(std::string aArgumentName,
                          OptionallyReference<std::string> aResult);

            ParseResult parse(Arguments aArguments) const override;

        private:
            OptionallyReference<std::string> mResult;
        };

        template <typename T>
        class Enum : public Parse
        {
        public:
            typedef T ResultType;

            // aOptions[0] becomes T(0) etc
            Enum(OptionallyReference<T> aResult, std::vector<std::string>&& aOptions);

            ParseResult parse(Arguments aArguments) const override;

        private:
            OptionallyReference<T> mResult;
            std::vector<std::string> mOptions;
        };

        class Flag : public Parse
        {
        public:
            typedef bool ResultType;

            Flag(bool& aResult);

            ParseResult parse(Arguments aArguments) const override;

        private:
            bool& mResult;
        };

        class Arg
        {
        public:
            Arg(Optional<char> aSwitch, Optional<std::string> aName, Parse* aParse, std::string aHelp);

            Arg(Arg&&);

            Optional<char> getSwitch() const;
            Optional<std::string> getName() const;
            std::string getHelp() const;

            bool isPositional() const;

            std::string getDescription() const;

            Optional<std::string> argumentName() const;
            bool isRequired() const;
            bool canBeRepeated() const;

            ParseResult parse(Arguments aArguments) const;

            ~Arg();

        private:
            Optional<char> mSwitch;
            Optional<std::string> mName;
            std::unique_ptr<Parse> mParse;
            std::string mHelp;
        };

        class Parser
        {
        public:
            Parser(std::string aAppName);
            Parser(const Parser&) = delete;
            Parser(Parser&&);

            ~Parser();

            ParseResult parse(const std::vector<std::string>& aArgs) const;

            void usage(std::ostream& aStream) const;

            Parser& add(Arg&& aArg);
        private:
            std::string mAppName;

            // used for managing the memory
            std::list<Arg> mArgHolder;

            std::map<char, Arg*> mSwitches;
            std::map<std::string, Arg*> mLongSwitches;
            Arg* mRemaining = nullptr;    // may be null

            static ParseResult tryArgParse(const Arg& aArg, const Arguments aArguments);
        };
    }
}

#include "commandline.icpp"
