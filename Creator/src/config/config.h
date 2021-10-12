
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
#pragma once

#include <functional>
#include <iostream>
#include <list>
#include <regex>
#include <set>
#include <string>

#include "common/exceptions.h"
#include "common/optional.h"
#include "jsonlib/json.hh"

namespace VDD
{
    /** @brief General base class for all configuration related errors */
    class ConfigError : public Exception
    {
    public:
        ConfigError(std::string aName);
        ~ConfigError() override;
    };

    /** JSON traversal type: either we traverse an object's field with the given name, or a
       field of an array */
    enum class ConfigTraverseType
    {
        None,       // invalid json traversal
        Field,      // traverse json object field
        ArrayIndex  // traverse json array index
    };

    /** @brief Describes a single step of traversal of ConfigTraverseType. It can contain either
     * a string or an index. */
    class ConfigTraverse
    {
    public:
        ConfigTraverse() = default;
        ConfigTraverse(std::string aField);
        ConfigTraverse(Json::ArrayIndex aIndex);
        ~ConfigTraverse() = default;

        bool operator==(const ConfigTraverse& aConfigTraverse) const;

        ConfigTraverseType getType() const;

        std::string asString() const;
        std::string getField() const;
        Json::ArrayIndex getIndex() const;

    private:
        ConfigTraverseType mTraverseType = ConfigTraverseType::None;
        std::string mField;
        Json::ArrayIndex mIndex = 0;
    };

    using ConfigPath = std::list<ConfigTraverse>;

    class JsonMergeStrategy
    {
    public:
        virtual ~JsonMergeStrategy();

        /** @brief Mutate left side object to achieve merge */
        virtual void object(Json::Value& aLeft, const Json::Value& aRight,
                            const ConfigPath& aConfigPath) const = 0;
        virtual void array(Json::Value& aLeft, const Json::Value& aRight,
                           const ConfigPath& aConfigPath) const = 0;
        virtual void other(Json::Value& aLeft, const Json::Value& aRight,
                           const ConfigPath& aConfigPath) const = 0;

        void merge(Json::Value& aLeft, const Json::Value& aRight,
                   const ConfigPath& aConfigPath) const;
    };

    class DefaultJsonMergeStrategy : public JsonMergeStrategy
    {
    public:
        DefaultJsonMergeStrategy();
        virtual ~DefaultJsonMergeStrategy();
        void object(Json::Value& aLeft, const Json::Value& aRight,
                    const ConfigPath& aConfigPath) const override;
        void array(Json::Value& aLeft, const Json::Value& aRight,
                   const ConfigPath& aConfigPath) const override;
        void other(Json::Value& aLeft, const Json::Value& aRight,
                   const ConfigPath& aConfigPath) const override;
    };

    extern const DefaultJsonMergeStrategy defaultJsonMergeStrategy;

    class FillBlanksJsonMergeStrategy : public DefaultJsonMergeStrategy
    {
    public:
        FillBlanksJsonMergeStrategy();
        virtual ~FillBlanksJsonMergeStrategy();
        void array(Json::Value& aLeft, const Json::Value& aRight,
                   const ConfigPath& aConfigPath) const override;
        void other(Json::Value& aLeft, const Json::Value& aRight,
                   const ConfigPath& aConfigPath) const override;
    };

    extern const FillBlanksJsonMergeStrategy fillBlanksJsonMergeStrategy;

    std::string stringOfConfigPath(const ConfigPath& aPath);
    ConfigPath configPathOfString(std::string aString);

    /** @brief A json or IO error occurred while reading the input */
    class ConfigLoadError : public ConfigError
    {
    public:
        ConfigLoadError(std::string aJsonError);
        ~ConfigLoadError() override;

        std::string message() const override;

    private:
        std::string mJsonError;
    };

    /** @brief General base class for JSON value related errors (ie. value missing, invalid
     * format, etc) that can be connected to a certain JSON value. */
    class ConfigKeyError : public ConfigError
    {
    public:
        ConfigKeyError(std::string aName, std::string aUserMessage, const ConfigPath& aPath);
        ~ConfigKeyError() override;

        std::string message() const override;

    private:
        std::string mUserMessage;
        std::string mPath;
    };

    class ConfigValue;

    /** @brief A value conflicts an existing one */
    class ConfigConflictError : public ConfigError
    {
    public:
        ConfigConflictError(std::string aUserMessage,
                            const ConfigValue& aInitial,
                            const ConfigValue& aSecond);
        ~ConfigConflictError() override;

        std::string message() const override;

    private:
        std::string mUserMessage;
        std::string mInitial;
        std::string mSecond;
    };

    /** This is thrown from a read*-function if there was a parse error and its location
     * cannot otherwise be pinpointed (ie. it originates from operator>>) */
    class ConfigValueReadError : public ConfigError
    {
    public:
        ConfigValueReadError(std::string aMessage);
        ~ConfigValueReadError() override;

        std::string message() const override;

    private:
        std::string mMessage;
    };

    /** @brief The JSON contained type other than the expected */
    class ConfigValueTypeMismatches : public ConfigKeyError
    {
    public:
        ConfigValueTypeMismatches(Json::ValueType aExpectedType, const ConfigValue& aNode);
        ConfigValueTypeMismatches(Json::ValueType aExpectedJsonType, std::string aExpectedType,
                                  const ConfigValue& aNode);
        ConfigValueTypeMismatches(std::string aExpectedType, const ConfigValue& aNode);
        ~ConfigValueTypeMismatches() override;
    };

    /** @brief The JSON type of the value was correct, but the value didn't pass the
        validation criteria. */
    class ConfigValueInvalid : public ConfigKeyError
    {
    public:
        ConfigValueInvalid(std::string aMessage, const ConfigValue& aNode);
        ~ConfigValueInvalid() override;
    };

    /** @brief A pair of values do not match validation against each other */
    class ConfigValuePairInvalid : public ConfigKeyError
    {
    public:
        ConfigValuePairInvalid(std::string aMessage, const ConfigValue& aNode1, const ConfigValue& aNode2);
        ~ConfigValuePairInvalid() override;

    private:
    };

    /** @brief The given key does not exist in the JSON */
    class ConfigKeyNotFound : public ConfigKeyError
    {
    public:
        ConfigKeyNotFound(const ConfigPath& aPath);
        ~ConfigKeyNotFound() override;
    };

    /** @brief The given key was expected to be an object but wasn't while traversing
        the JSON tree */
    class ConfigKeyNotObject : public ConfigKeyError
    {
    public:
        ConfigKeyNotObject(const ConfigPath& aPath);
        ~ConfigKeyNotObject() override;
    };

    class Config;

    /** @brief Base class for ConfigValue and ConfigValueWithFallback, useful for polymorphic
     * function calls. Beware slicing. */
    class ConfigValueBase
    {
    public:
        virtual ~ConfigValueBase();

        /** @brief The same, but for multiple hops */
        virtual ConfigValue traverse(const ConfigPath& aPath) const = 0;

        /** @brief The same, but for multiple hops, and returns an invalid ConfigPath if no
         * such key is found. Similar to Config::tryTraverse. */
        virtual ConfigValue tryTraverse(const ConfigPath& aPath) const = 0;
    };

    /** @brief The basic value for referring to the JSON values in the configuration.
     *
     * Note that the memory of these JSON objects is managed by Config, so the Config object
     * must exist for the ConfigValue objects to be usable.
     */
    class ConfigValue : public ConfigValueBase
    {
    public:
        /** @brief A default-constructed useless value. .valid() returns false. */
        ConfigValue();

        ConfigValue(const ConfigValue& aValue) = default;

        ~ConfigValue() = default;

        ConfigValue& operator=(const ConfigValue& aOther) = default;

        /** @brief A traversal operator. aPath can contain exactly one element.
         */
        ConfigValue operator[](const std::string& aPath) const;

        /** @brief The same, but for multiple hops */
        ConfigValue traverse(const ConfigPath& aPath) const override;

        /** @brief The same, but for multiple hops, and returns an invalid ConfigPath if no
         * such key is found. Similar to Config::tryTraverse. */
        ConfigValue tryTraverse(const ConfigPath& aPath) const override;

        /** @brief Replaces values in the left configuration with values found
         * from the right. This invalidates all other ConfigValues under aLeft.
         *
         * For the purpose of the merge algorithm, the merge is rooted at the given nodes.
         *
         * The config value itself must be in valid state.
         */
        ConfigValue& merge(const ConfigValue& aRight,
                           const JsonMergeStrategy& aJsonMergeStrategy = defaultJsonMergeStrategy);

        /** @brief Is this object both initialized with a JSON value and is that JSON value
         * other than nill? */
        bool valid() const;
        explicit operator bool() const;

        /** @brief Dereferencing operators should the underlying JSON value be useful.
         * Currently these are useful in particular with the exception classes, and it's
         * perhaps less clutter to just allow everyone to use these instead of friend'ing
         * the applicable exception classes.
         *
         * Try to avoid the use of these operators in client code. */
        const Json::Value& operator*() const;
        const Json::Value* operator->() const;

        /** @brief If this value is an object or an array, return the subnodes */
        std::vector<ConfigValue> childValues() const;

        /** Path to this node from the root node (for printing warnings/errors) */
        ConfigPath getPath() const;

        /** Base directory for relative filename paths in json  */
        std::string getRelativeFilenamePath() const;

        /** Name of this node */
        std::string getName() const;

        /** Accessor for producing nice single-line representations of the contents */
        std::string singleLineRepresentation() const;

        /** A low-ish level function for marking this branch completely visited the purpose
         * of generating warnings about unused configuration values */
        void markBranchVisited();

    private:
        friend class Config;

        /** @brief Construct a value at given path; aValue may be nullptr */
        ConfigValue(const Config& aConfig, std::string aName, const ConfigPath& aParentPath,
                    Json::Value* aValue);

        const Config* mConfig = nullptr;
        std::string mName;
        ConfigPath mPath;

        Json::Value* mValue = nullptr;
    };

    template <typename T>
    class ParsedValue
    {
    public:
        template <typename Convert>
        ParsedValue(const ConfigValue& aConfig, Convert aConvert);


        template <typename Convert>
        ParsedValue<T>& set(const ConfigValue& aConfig, Convert aConvert);

        const T& operator*() const;
        const T* operator->() const;

        const ConfigValue& getConfigValue() const;

        // this is meant for postponed initialization only; it cannot be determined if the
        // object is initialized with a config
        ParsedValue() = default;
        ParsedValue(const ParsedValue<T>&) = default;
        ParsedValue(ParsedValue<T>&&) = default;
        ParsedValue<T>& operator=(const ParsedValue&) = default;
        ParsedValue<T>& operator=(ParsedValue&&) = default;
        ~ParsedValue() = default;

    private:
        struct Value
        {
            ConfigValue configValue;
            T value;
        };
        Optional<Value> mValue;
    };

    template <typename Convert>
    auto readParsedValue(Convert aConvert) -> std::function<
        ParsedValue<decltype(aConvert(ConfigValue()))>(const ConfigValue& aConfig)>;

    // considers only value, not config
    template <typename T>
    bool operator<(const ParsedValue<T>& a, const ParsedValue<T>& b);

    template <typename T>
    bool operator>(const ParsedValue<T>& a, const ParsedValue<T>& b);

    template <typename T>
    bool operator<=(const ParsedValue<T>& a, const ParsedValue<T>& b);

    template <typename T>
    bool operator>=(const ParsedValue<T>& a, const ParsedValue<T>& b);

    template <typename T>
    bool operator==(const ParsedValue<T>& a, const ParsedValue<T>& b);

    /** @brief Simple ConfigValue-like wrapper for dealing more easily with fallback values */
    class ConfigValueWithFallback : public ConfigValueBase
    {
    public:
        ConfigValueWithFallback();
        ConfigValueWithFallback(const ConfigValue& aConfig);
        ConfigValueWithFallback(const ConfigValue& aConfig, const ConfigValue& aFallback);

        const ConfigValue operator[](std::string aKey) const;

        const ConfigValue& get() const;

        std::string getName() const;

        /** @brief The same, but for multiple hops */
        ConfigValue traverse(const ConfigPath& aPath) const override;

        /** @brief The same, but for multiple hops, and returns an invalid ConfigPath if no
         * such key is found. Similar to Config::tryTraverse. */
        ConfigValue tryTraverse(const ConfigPath& aPath) const override;

    private:
        ConfigValue mConfig;
        ConfigValue mFallback;
    };

    using JsonNodeSet = std::set<const Json::Value*>;

    class Config
    {
    public:
        Config();

        /** @brief Load the given input stream with the given name. The name is useful
            for producing error messages that indicate in which file the JSON fragment
            originates from.

            When loading more than one file the contents of the latter file is merged
            with the first one.
         */
        bool load(std::istream& aStream, std::string aName,
                  const JsonMergeStrategy& aJsonMergeStrategy = defaultJsonMergeStrategy);

        /**
         * Sets path to which filenames are parsed as relative when loading
         * configuration.
         */
        void setRelativeInputFilenameBasePath(const std::string aBasePath);

        /** @brief Process command line arguments. This processes both json
         * config file names and given override arguments in the order they
         * appear in the command line.
         *
         * --foo.bar-baz=42 maps into JSON { "foo" { "bar_baz": 42 } }
         *
         * @param mArgs Input arguments include the 0th argument for program name
         */
        void commandline(const std::vector<std::string>& mArgs,
                         const JsonMergeStrategy& aJsonMergeStrategy = defaultJsonMergeStrategy);

        /** @brief Process given key-value pair in a similar fashion command line arguments
         * of form
         * --key=value are processed. There is one difference: the maping of '-'->'_' in key
         * names is not performed.
         */
        void setKeyValue(const std::string& aKey, const std::string& aValue,
                         const JsonMergeStrategy& aJsonMergeStrategy = defaultJsonMergeStrategy);

        /** @brief Work similarly as setKeyValue, except the value can be provided directly
         * instead of using heuristics to convert strings to json objects or strings etc.
         */
        void setKeyJsonValue(
            const std::string& aKey, const Json::Value& aValue,
            const JsonMergeStrategy& aJsonMergeStrategy = defaultJsonMergeStrategy);

        /* copying is prohibited, because it would invalidate the pointer-based
         * mVisitedNodes table. The alternative might be use path-based visitation table,
         * but it would be bigger and would have similar problem regarding top-level names
         * changing.
         */
        Config(const Config& aOther) = delete;
        Config& operator=(const Config& aOther) = delete;

        ~Config();

        /** @brief Get the node pointed by aPath. ie. config["video"]["filename"] */
        ConfigValue operator[](const std::string& aPath) const;

        /** @brief Given a list of traverse instructions, find a node starting from given
         * node */
        ConfigValue traverse(const ConfigValue& aNode, const ConfigPath& aPath) const;

        /** @brief Given a list of traverse instructions, find a node starting from given
         * node. However, if a value is not found, do not throw an exception but just return
         * an invalid ConfigValue. It will still throw an exception if an non-object was
         * found when an object was expected. */
        ConfigValue tryTraverse(const ConfigValue& aNode, const ConfigPath& aPath) const;

        /** @brief Retrieve the root value. Useful with traversal functions. */
        ConfigValue root() const;

        /** @brief Were there warnings? If so, return a non-empty string */
        std::list<std::string> warnings() const;

        /** @brief Adds missing nodes from the right to the left. This invalidates
         * all ConfigValues under aLeft.
         *
         * For the purpose of the merge algorithm, the merge is rooted at the given nodes.
         */
        void fillMissing(const ConfigValue& aLeft, const ConfigValue& aRight,
                         const JsonMergeStrategy& aJsonMergeStrategy = defaultJsonMergeStrategy);

        /** @brief Replaces values in the left configuration with values found
         * from the right. This invalidates all ConfigValues under aLeft.
         *
         * For the purpose of the merge algorithm, the merge is rooted at the given nodes.
         */
        void merge(const ConfigValue& aLeft, const ConfigValue& aRight,
                   const JsonMergeStrategy& aJsonMergeStrategy = defaultJsonMergeStrategy);

        /** @brief Removes a subnode node of given node. This invalidates the subnode and
         * all ConfigValues below it. */
        void remove(const ConfigValue& aNode, ConfigTraverse aSubNode);

    private:
        // For finding out which nodes have not been visited, regardless of Config's
        // constness
        mutable JsonNodeSet mVisitedNodes;

        Json::Value mRootJson;
        ConfigValue mRoot;
        std::string mRelativeInputFilenameBasePath;

        friend class ConfigValue;
        // ConfigValue can use this to mark that it has visited this node regardless of
        // Config's constness status
        void visitNode(const Json::Value& aNode) const;

        friend std::ostream& operator<<(std::ostream& aStream, const Config& aConfig);
    };

    std::ostream& operator<<(std::ostream& aStream, const Config& aConfig);

    /** @brief Functions for parsing data from the JSON and throwing appropriate exceptions if
     * the types don't match. */
    int readInt(const ConfigValue& aNode);

    /** @brief Returns a function to read anything from an integer that can be casted safely to
     * T without overflows or underflows. */
    template <typename T>
    std::function<T(const ConfigValue& aNode)> readIntegral(std::string aTypeName);

    const auto readSize = readIntegral<size_t>("size");

    /** @brief Returns a function to read and bound-check an integer with the given reader (ie. readIntegral) */
    template <typename Read>
    auto readRangedIntegral(std::string aName, Read aReader, decltype(aReader({})) aMin,
                            decltype(aReader({})) aMax)
        -> std::function<decltype(aReader({}))(const ConfigValue& aNode)>;

    const auto readUInt = readIntegral<unsigned int>("unsigned int");

    double readDouble(const ConfigValue& aNode);

    float readFloat(const ConfigValue& aNode);

    std::string readString(const ConfigValue& aNode);

    std::string readFilename(const ConfigValue& aNode);

    auto readRegexValidatedString(std::regex aRegex, std::string aMessage)
        -> std::function<std::string(const ConfigValue&)>;

    template <typename T, typename U>
    std::function<U(const ConfigValue&)> mapConfigValue(
        std::function<T(const ConfigValue&)> aReadValue, std::function<U(const T&)> aMapValue);

    bool readBool(const ConfigValue& aNode);

    /** @brief Returns a function to read anything from a string that is convertible to T with
     * iostream operators. The indirection of returning a function is to facilitate making named
     * versions of this function easily. */
    template <typename T>
    std::function<T(const ConfigValue& aNode)> readGeneric(std::string aTypeName);

    const auto readUint32 = readIntegral<std::uint32_t>("uint32");

    const auto readInt32 = readIntegral<std::int32_t>("int32");

    const auto readUint16 = readIntegral<std::uint16_t>("uint16");

    const auto readInt16 = readIntegral<std::int16_t>("int16");

    const auto readUint8 = readIntegral<std::uint8_t>("uint8");

    const auto readInt8 = readIntegral<std::int8_t>("int8");

    /** @brief Reads a generic pair of values separated by a separator (uses iostream to perform
     * actual reading) */
    template <typename T, typename U>
    std::function<std::pair<T, U>(const ConfigValue& aNode)> readPair(std::string aTypeName,
                                                                      char aSeparator);

    /** @brief Reads a list of type returned by the reader function individually for each
     * element.
     */
    template <typename Read>
    auto readList(std::string aTypeName, Read aReader)
        -> std::function<std::list<decltype(aReader({}))>(const ConfigValue& aNode)>;

    /** @brief Reads a set of type returned by the reader function individually for each
     * element. Duplicate values cause a ConfigValueInvalid to be thrown. */
    template <typename Read>
    auto readSet(std::string aTypeName, Read aReader)
        -> std::function<std::set<decltype(aReader({}))>(const ConfigValue& aNode)>;

    /** @brief Reads a vector of type returned by the reader function individually for each
     * element.
     */
    template <typename Read>
    auto readVector(std::string aTypeName, Read aReader)
        -> std::function<std::vector<decltype(aReader({}))>(const ConfigValue& aNode)>;

    /** @brief Reads an optional value of type returned by the reader function. */
    template <typename Read>
    auto readOptional(Read aReader)
        -> std::function<Optional<decltype(aReader({}))>(const ConfigValue& aNode)>;

    /** @brief More practical function for reading optional configuration value with a default
     * value.
     *
     * Example: optionWithDefault(mConfig->root(), "debug.parallel.perf", "performance logging
     * enabled", readBool, false)
     */
    template <typename Read>
    auto optionWithDefault(const ConfigValueBase& aConfig, std::string aPath, Read aReader,
                           decltype(aReader({})) aDefault) -> decltype(aReader({}));

    /** @brief Given a container of ConfigValues, try to evaluate
     * aReader[configValue[aFieldName]] for each of them. The purpose is to search a value from
     * multiple objects in a fallback-fashion.
     *
     * Example: int a = readFieldFallback({ current, defaults }, "a", readInt);
     */
    template <typename Reader, typename Container>
    auto readFieldFallback(const Container& aFallbacks, std::string aFieldName, Reader aReader)
        -> decltype(aReader(ConfigValue()));
}  // namespace VDD

#include "config.icpp"
