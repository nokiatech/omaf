
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
#include "config.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>

#include "common/utils.h"
#include "jsonlib/json.hh"

namespace VDD
{
    namespace {
        void stripJsonComments(Json::Value& aJson)
        {
            switch (aJson.type()) {
                case Json::nullValue:
                case Json::intValue:
                case Json::uintValue:
                case Json::realValue:
                case Json::stringValue:
                case Json::booleanValue:
                    break;
                case Json::objectValue:
                    aJson.removeMember("//");
                    // fall through
                case Json::arrayValue:
                    for (auto& el : aJson)
                    {
                        stripJsonComments(el);
                    }
                    break;
            }
        }
    }

    JsonMergeStrategy::~JsonMergeStrategy() = default;

    void JsonMergeStrategy::merge(Json::Value& aLeft, const Json::Value& aRight,
                                  const ConfigPath& aConfigPath) const
    {
        if (aLeft.type() == Json::objectValue && aRight.type() == Json::objectValue)
        {
            object(aLeft, aRight, aConfigPath);
        }
        else if (aLeft.type() == Json::arrayValue && aRight.type() == Json::arrayValue)
        {
            array(aLeft, aRight, aConfigPath);
        }
        else
        {
            // perhaps there could be a check that the types should match? that might
            // make it difficult to remove values at merge, if that is desirable,
            // though.
            other(aLeft, aRight, aConfigPath);
        }
    }

    const DefaultJsonMergeStrategy defaultJsonMergeStrategy = DefaultJsonMergeStrategy();

    DefaultJsonMergeStrategy::DefaultJsonMergeStrategy() = default;
    DefaultJsonMergeStrategy::~DefaultJsonMergeStrategy() = default;

    void DefaultJsonMergeStrategy::object(Json::Value& aLeft, const Json::Value& aRight,
                                          const ConfigPath& aConfigPath) const
    {
        using set = std::set<std::string>;
        set left = Utils::setify(aLeft.getMemberNames());
        set right = Utils::setify(aRight.getMemberNames());
        set added;
        std::set_difference(right.begin(), right.end(), left.begin(), left.end(),
                            std::inserter(added, added.end()));
        set common;
        std::set_intersection(left.begin(), left.end(), right.begin(), right.end(),
                              std::inserter(common, common.end()));

        for (auto& add : added)
        {
            aLeft[add] = aRight[add];
        }
        for (auto& update : common)
        {
            merge(aLeft[update], aRight[update],
                  Utils::append(aConfigPath, ConfigTraverse(update)));
        }
    }

    void DefaultJsonMergeStrategy::array(Json::Value& aLeft, const Json::Value& aRight,
                                         const ConfigPath&) const
    {
        aLeft = aRight;
    }

    void DefaultJsonMergeStrategy::other(Json::Value& aLeft, const Json::Value& aRight,
                                         const ConfigPath&) const
    {
        aLeft = aRight;
    }

    const FillBlanksJsonMergeStrategy fillBlanksJsonMergeStrategy = FillBlanksJsonMergeStrategy();

    FillBlanksJsonMergeStrategy::FillBlanksJsonMergeStrategy() = default;
    FillBlanksJsonMergeStrategy::~FillBlanksJsonMergeStrategy() = default;

    void FillBlanksJsonMergeStrategy::array(Json::Value&, const Json::Value&,
                                            const ConfigPath&) const
    {
        // no merge for arrays
    }

    void FillBlanksJsonMergeStrategy::other(Json::Value&, const Json::Value&,
                                            const ConfigPath&) const
    {
        // no merge for values
    }

    namespace
    {
        using ConfigWarnings = std::list<std::string>;

        std::string stringOfJsonValueType(Json::ValueType aType)
        {
            switch (aType)
            {
            case Json::nullValue:
                return "'null'";
            case Json::intValue:
                return "signed integer";
            case Json::uintValue:
                return "unsigned integer";
            case Json::realValue:
                return "double";
            case Json::stringValue:
                return "string";
            case Json::booleanValue:
                return "bool";
            case Json::arrayValue:
                return "array";
            case Json::objectValue:
                return "object";
            }
            return "unknown";
        }
    }  // namespace

    ConfigTraverse::ConfigTraverse(std::string aField)
        : mTraverseType(ConfigTraverseType::Field), mField(aField)
    {
        // nothing
    }

    ConfigTraverse::ConfigTraverse(Json::ArrayIndex aIndex)
        : mTraverseType(ConfigTraverseType::ArrayIndex), mIndex(aIndex)
    {
        // nothing
    }

    bool ConfigTraverse::operator==(const ConfigTraverse& aOther) const
    {
        switch (mTraverseType)
        {
        case ConfigTraverseType::None:
        {
            return aOther.mTraverseType == ConfigTraverseType::None;
        }
        break;
        case ConfigTraverseType::ArrayIndex:
        {
            return mIndex == aOther.mIndex;
        }
        break;
        case ConfigTraverseType::Field:
        {
            return mField == aOther.mField;
        }
        break;
        }
        assert(false);
        return false;
    }

    ConfigTraverseType ConfigTraverse::getType() const
    {
        return mTraverseType;
    }

    std::string ConfigTraverse::asString() const
    {
        switch (mTraverseType)
        {
        case ConfigTraverseType::ArrayIndex:
        {
            return "[" + Utils::to_string(mIndex) + "]";
            break;
        }
        case ConfigTraverseType::Field:
        {
            return mField;
        }
        case ConfigTraverseType::None:
        {
            assert(false);
            break;
        }
        }
        assert(false);
        return "";
    }

    std::string ConfigTraverse::getField() const
    {
        return mField;
    }

    Json::ArrayIndex ConfigTraverse::getIndex() const
    {
        return mIndex;
    }

    ConfigError::ConfigError(std::string aName) : Exception(aName)
    {
        // nothing
    }

    ConfigError::~ConfigError() = default;

    ConfigValueReadError::ConfigValueReadError(std::string aMessage)
        : ConfigError("ConfigValueReadError"), mMessage(aMessage)
    {
        // nothing
    }

    ConfigValueReadError::~ConfigValueReadError() = default;

    std::string ConfigValueReadError::message() const
    {
        return mMessage;
    }

    ConfigLoadError::ConfigLoadError(std::string aJsonError)
        : ConfigError("ConfigLoadError"), mJsonError(aJsonError)
    {
        // nothing
    }

    ConfigLoadError::~ConfigLoadError() = default;

    std::string ConfigLoadError::message() const
    {
        return "Error while loading JSON: " + mJsonError;
    }

    ConfigValueTypeMismatches::ConfigValueTypeMismatches(Json::ValueType aExpectedType,
                                                         const ConfigValue& aNode)
        : ConfigValueTypeMismatches(stringOfJsonValueType(aExpectedType), aNode)
    {
        // nothing
    }

    ConfigValueTypeMismatches::~ConfigValueTypeMismatches() = default;

    ConfigValueTypeMismatches::ConfigValueTypeMismatches(Json::ValueType aExpectedJsonType,
                                                         std::string aExpectedType,
                                                         const ConfigValue& aNode)
        : ConfigKeyError("ConfigValueTypeMismatches",
                         "expected type " + aExpectedType + +"(" +
                             stringOfJsonValueType(aExpectedJsonType) + ") but has type " +
                             (aNode ? stringOfJsonValueType(aNode->type()) : "nill") +
                             " in configuration with value " + aNode.singleLineRepresentation(),
                         aNode.getPath())
    {
        // nothing
    }

    ConfigValueTypeMismatches::ConfigValueTypeMismatches(std::string aExpectedType,
                                                         const ConfigValue& aNode)
        : ConfigKeyError("ConfigValueTypeMismatches",
                         "expected type " + aExpectedType + " but has type " +
                             (aNode ? stringOfJsonValueType(aNode->type()) : "nill") +
                             " in configuration with value " + aNode.singleLineRepresentation(),
                         aNode.getPath())
    {
        // nothing
    }

    ConfigValueInvalid::ConfigValueInvalid(std::string aMessage, const ConfigValue& aNode)
        : ConfigKeyError(
              "ConfigValueInvalid",
              "invalid value: " + aMessage + ", provided " + aNode.singleLineRepresentation(),
              aNode.getPath())
    {
        // nothing
    }

    ConfigValueInvalid::~ConfigValueInvalid() = default;

    ConfigValuePairInvalid::ConfigValuePairInvalid(std::string aMessage, const ConfigValue& aNode1,
                                                   const ConfigValue& aNode2)
        : ConfigKeyError("ConfigValuePairInvalid",
                         "invalid value: " + aMessage + ", provided " +
                             aNode1.singleLineRepresentation() + " and " +
                             aNode2.singleLineRepresentation(),
                         aNode1.getPath())
    {
        // nothing
    }

    ConfigValuePairInvalid::~ConfigValuePairInvalid() = default;

    ConfigKeyError::ConfigKeyError(std::string aName, std::string aUserMessage,
                                   const ConfigPath& aPath)
        : ConfigError(aName), mUserMessage(aUserMessage), mPath(stringOfConfigPath(aPath))
    {
        // nothing
    }

    ConfigKeyError::~ConfigKeyError() = default;

    std::string ConfigKeyError::message() const
    {
        return "Configuration " + mUserMessage + " at " + mPath;
    }

    ConfigConflictError::ConfigConflictError(std::string aUserMessage,
                                             const ConfigValue& aInitial, const ConfigValue& aSecond)
        : ConfigError("ConfigConflictError")
        , mUserMessage(aUserMessage)
        , mInitial(stringOfConfigPath(aInitial.getPath()))
        , mSecond(stringOfConfigPath(aSecond.getPath()))
    {
        // nothing
    }

    ConfigConflictError::~ConfigConflictError() = default;

    std::string ConfigConflictError::message() const
    {
        return mUserMessage + " at " + mSecond + " conflicting earlier config at " + mInitial;
    }

    ConfigKeyNotFound::ConfigKeyNotFound(const ConfigPath& aPath)
        : ConfigKeyError("ConfigKeyNotFound", "key not found", aPath)
    {
        // nothing
    }

    ConfigKeyNotFound::~ConfigKeyNotFound() = default;

    ConfigKeyNotObject::ConfigKeyNotObject(const ConfigPath& aPath)
        : ConfigKeyError("ConfigKeyNotObject", "expected JSON object but non-object key found",
                         aPath)
    {
        // nothing
    }

    ConfigKeyNotObject::~ConfigKeyNotObject() = default;

    ConfigValueBase::~ConfigValueBase() = default;

    ConfigValue::ConfigValue(const Config& aConfig, std::string aName,
                             const ConfigPath& aParentPath, Json::Value* aValue)
        : mConfig(&aConfig), mName(aName), mPath(Utils::append(aParentPath, aName)), mValue(aValue)
    {
        // nothing
    }

    ConfigValue::ConfigValue() = default;

    const Json::Value& ConfigValue::operator*() const
    {
        if (!mValue)
        {
            throw ConfigKeyNotFound(getPath());
        }
        else
        {
            mConfig->visitNode(*mValue);
            return *mValue;
        }
    }

    const Json::Value* ConfigValue::operator->() const
    {
        if (!mValue)
        {
            throw ConfigKeyNotFound(getPath());
        }
        else
        {
            mConfig->visitNode(*mValue);
            return mValue;
        }
    }

    std::string ConfigValue::singleLineRepresentation() const
    {
        if (mValue)
        {
            Json::FastWriter writer;
            return Utils::replace(writer.write(*mValue), "\n", "");
        }
        else
        {
            return "null";
        }
    }

    void ConfigValue::markBranchVisited()
    {
        if (!mValue)
        {
            throw ConfigKeyNotFound(getPath());
        }
        else
        {
            mConfig->visitNode(*mValue);
            if (mValue->isArray() || mValue->isObject())
            {
                for (auto children : childValues())
                {
                    children.markBranchVisited();
                }
            }
        }
    }

    bool ConfigValue::valid() const
    {
        if (mValue)
        {
            mConfig->visitNode(*mValue);
        }
        return mValue && mValue->type() != Json::nullValue;
    }

    ConfigValue::operator bool() const
    {
        return valid();
    }

    int readInt(const ConfigValue& aNode)
    {
        if (!aNode)
        {
            throw ConfigKeyNotFound(aNode.getPath());
        }
        else if (aNode->isConvertibleTo(Json::intValue))
        {
            return aNode->asInt();
        }
        else
        {
            throw ConfigValueTypeMismatches(Json::intValue, aNode);
        }
    }

    float readFloat(const ConfigValue& aNode)
    {
        if (!aNode)
        {
            throw ConfigKeyNotFound(aNode.getPath());
        }
        else if (aNode->isConvertibleTo(Json::realValue))
        {
            return aNode->asFloat();
        }
        else
        {
            throw ConfigValueTypeMismatches(Json::realValue, aNode);
        }
    }

    double readDouble(const ConfigValue& aNode)
    {
        if (!aNode)
        {
            throw ConfigKeyNotFound(aNode.getPath());
        }
        else if (aNode->isConvertibleTo(Json::realValue))
        {
            return aNode->asDouble();
        }
        else
        {
            throw ConfigValueTypeMismatches(Json::realValue, aNode);
        }
    }

    std::string readString(const ConfigValue& aNode)
    {
        if (!aNode)
        {
            throw ConfigKeyNotFound(aNode.getPath());
        }
        else if (aNode->isString())
        {
            return aNode->asString();
        }
        else
        {
            throw ConfigValueTypeMismatches(Json::stringValue, aNode);
        }
    }

    std::string readFilename(const ConfigValue& aNode)
    {
        auto stringVal = readString(aNode);
        auto relativePathBase = aNode.getRelativeFilenamePath();

        // if string starts with ./ or ../ interpret it as relative path
        if (relativePathBase != "" &&
            (stringVal.substr(0, 2) == "./" || stringVal.substr(0, 3) == "../"))
        {
            if (relativePathBase[relativePathBase.length() - 1] != '/')
            {
                relativePathBase = relativePathBase + "/";
            }
            return relativePathBase + stringVal;
        }

        return stringVal;
    }

    auto readRegexValidatedString(std::regex aRegex, std::string aMessage)
        -> std::function<std::string(const ConfigValue&)>
    {
        return [=](const ConfigValue& aNode) {
            std::string str = readString(aNode);

            if (std::regex_match(str, aRegex))
            {
                return str;
            }
            else
            {
                throw ConfigValueInvalid(aMessage, aNode);
            }
        };
    }

    bool readBool(const ConfigValue& aNode)
    {
        if (!aNode)
        {
            throw ConfigKeyNotFound(aNode.getPath());
        }
        else if (aNode->isInt())
        {
            if (aNode->asInt() == 0 || aNode->asInt() == 1)
            {
                return aNode->asBool();
            }
            else
            {
                throw ConfigValueTypeMismatches(Json::booleanValue, aNode);
            }
        }
        else if (aNode->isBool())
        {
            return aNode->asBool();
        }
        else
        {
            throw ConfigValueTypeMismatches(Json::booleanValue, aNode);
        }
    }

    ConfigValue ConfigValue::operator[](const std::string& aPath) const
    {
        if (mConfig)
        {
            return mConfig->traverse(*this, {aPath});
        }
        else
        {
            throw ConfigKeyNotFound(Utils::append(getPath(), ConfigTraverse("..")));
        }
    }

    ConfigValue ConfigValue::traverse(const ConfigPath& aPath) const
    {
        if (mConfig)
        {
            return mConfig->traverse(*this, aPath);
        }
        else
        {
            throw ConfigKeyNotFound(Utils::append(getPath(), ConfigTraverse("..")));
        }
    }

    ConfigValue ConfigValue::tryTraverse(const ConfigPath& aPath) const
    {
        if (mConfig)
        {
            return mConfig->tryTraverse(*this, aPath);
        }
        else
        {
            return ConfigValue();
        }
    }

    ConfigValue& ConfigValue::merge(const ConfigValue& aRight,
                                    const JsonMergeStrategy& aJsonMergeStrategy)
    {
        if (!valid())
        {
            throw ConfigKeyNotFound(getPath());
        }
        if (aRight)
        {
            aJsonMergeStrategy.merge(*mValue, *aRight.mValue, {});
        }
        return *this;
    }

    std::vector<ConfigValue> ConfigValue::childValues() const
    {
        if (!mValue)
        {
            throw ConfigKeyNotFound(getPath());
        }
        else if (mValue->isObject())
        {
            std::vector<ConfigValue> nodes;
            for (const auto& nodeName : mValue->getMemberNames())
            {
                nodes.push_back(mConfig->traverse(*this, {nodeName}));
            }
            return nodes;
        }
        else if (mValue->isArray())
        {
            std::vector<ConfigValue> nodes;
            for (Json::ArrayIndex index = Json::ArrayIndex(0); index < mValue->size(); ++index)
            {
                nodes.push_back(mConfig->traverse(*this, {index}));
            }
            return nodes;
        }
        else
        {
            throw ConfigValueTypeMismatches(Json::objectValue, *this);
        }
    }

    std::string ConfigValue::getName() const
    {
        return mName;
    }

    std::string ConfigValue::getRelativeFilenamePath() const
    {
        return mConfig->mRelativeInputFilenameBasePath;
    }

    ConfigPath ConfigValue::getPath() const
    {
        return mPath;
    }

    ConfigValueWithFallback::ConfigValueWithFallback()
    {
        // nothing
    }

    ConfigValueWithFallback::ConfigValueWithFallback(const ConfigValue& aConfig) : mConfig(aConfig)
    {
        // nothing
    }

    ConfigValueWithFallback::ConfigValueWithFallback(const ConfigValue& aConfig,
                                                     const ConfigValue& aFallback)
        : mConfig(aConfig), mFallback(aFallback)
    {
        // nothing
    }

    std::string ConfigValueWithFallback::getName() const
    {
        return mConfig ? mConfig.getName() : mFallback.getName();
    }

    const ConfigValue& ConfigValueWithFallback::get() const
    {
        return mConfig ? mConfig : mFallback;
    }

    const ConfigValue ConfigValueWithFallback::operator[](std::string aKey) const
    {
        auto primary = mConfig.tryTraverse({aKey});
        if (primary)
        {
            return primary;
        }
        else
        {
            auto secondary = mFallback.tryTraverse({aKey});
            return secondary;
        }
    }

    ConfigValue ConfigValueWithFallback::traverse(const ConfigPath& aPath) const
    {
        auto primary = mConfig.tryTraverse(aPath);
        if (primary)
        {
            return primary;
        }
        else
        {
            auto secondary = mFallback.traverse(aPath);
            return secondary;
        }
    }

    ConfigValue ConfigValueWithFallback::tryTraverse(const ConfigPath& aPath) const
    {
        auto primary = mConfig.tryTraverse(aPath);
        if (primary)
        {
            return primary;
        }
        else
        {
            auto secondary = mFallback.tryTraverse(aPath);
            return secondary;
        }
    }

    Config::Config() : mRootJson(Json::objectValue), mRoot(*this, "", {}, &mRootJson)
    {
        // nothing
    }

    Config::~Config() = default;

    bool Config::load(std::istream& aStream, std::string aName,
                      const JsonMergeStrategy& aJsonMergeStrategy)
    {
        Json::CharReaderBuilder builder;
        builder["collectComments"] = false;
        Json::Value value;
        std::string errs;
        bool ok = Json::parseFromStream(builder, aStream, &value, &errs);
        if (!ok)
        {
            throw ConfigLoadError(errs);
            return ok;
        }
        stripJsonComments(value);
        aJsonMergeStrategy.merge(mRootJson, value, {});
        mRoot = ConfigValue(*this, mRoot.getName().size() ? mRoot.getName() + "+" + aName : aName,
                            {}, &mRootJson);
        mVisitedNodes.clear();
        return ok;
    }

    void Config::setRelativeInputFilenameBasePath(const std::string aBasePath)
    {
        mRelativeInputFilenameBasePath = aBasePath;
    }


    namespace
    {
        Json::Value jsonValueOfString(std::string aString)
        {
            Json::Value value;
            std::istringstream st(aString);
            int first = st.peek();
            if (!st)
            {
                value = Json::Value("");
            }
            else
            {
                char firstChar = static_cast<char>(first);
                // If it's a JSON object or a JSON array, parse it as-is
                if (firstChar == '{' || firstChar == '[')
                {
                    Json::CharReaderBuilder builder;
                    builder["collectComments"] = false;
                    std::string errs;
                    bool ok = Json::parseFromStream(builder, st, &value, &errs);
                    if (!ok)
                    {
                        throw ConfigLoadError(errs);
                    }
                    stripJsonComments(value);
                }
                else
                {
                    // If it can be read as a number, make a JSON integer (well
                    // double really, but we don't use doubles in VDD
                    // configuration)
                    int n;
                    st >> n;
                    if (st && st.peek() == EOF)
                    {
                        value = Json::Value(n);
                    }
                    else
                    {
                        // Otherwise use the contents as a string
                        st.clear();
                        st.seekg(std::streampos(0));
                        value = Json::Value(st.str());
                    }
                }
            }
            return value;
        }

        // {"foo.bar-baz", 42} maps into { "foo" { "bar_baz": 42 } }
        Json::Value jsonOfKeyValue(std::string aKey, const Json::Value& aValue)
        {
            bool valueSet = false;
            std::string keyName;
            Json::Value parent(Json::objectValue);

            for (size_t n = 0; !valueSet && n < aKey.size(); ++n)
            {
                if (aKey[n] == '.')
                {
                    parent[keyName] = jsonOfKeyValue(aKey.substr(n + 1), aValue);
                    valueSet = true;
                }
                else
                {
                    keyName += aKey[n];
                }
            }

            if (!valueSet)
            {
                parent[keyName] = aValue;
            }

            return parent;
        }

        // foo.bar-baz=42 maps into { "foo" { "bar_baz": 42 } }
        Json::Value jsonOfArgument(std::string aString)
        {
            Json::Value value;
            std::string keyName;
            Json::Value parent(Json::objectValue);

            for (size_t n = 0; value.type() == Json::nullValue && n < aString.size(); ++n)
            {
                if (aString[n] == '.')
                {
                    parent[keyName] = jsonOfArgument(aString.substr(n + 1));
                    value = parent;
                }
                else if (aString[n] == '=')
                {
                    parent[keyName] = jsonValueOfString(aString.substr(n + 1));
                    value = parent;
                }
                else if (aString[n] == '-')
                {
                    keyName += '_';
                }
                else
                {
                    keyName += aString[n];
                }
            }

            return value;
        }
    }  // namespace

    void Config::commandline(const std::vector<std::string>& mArgs,
                             const JsonMergeStrategy& aJsonMergeStrategy)
    {
        for (size_t n = 1; n < mArgs.size(); ++n)
        {
            auto& arg = mArgs[n];
            if (arg.size() > 2 && arg.substr(0, 2) == "--")
            {
                aJsonMergeStrategy.merge(mRootJson, jsonOfArgument(arg.substr(2)), {});
                mRoot =
                    ConfigValue(*this, mRoot.getName().size() ? mRoot.getName() + "+arg" : "arg",
                                {}, &mRootJson);
            }
            else
            {
                std::ifstream file(arg);
                if (!file)
                {
                    throw ConfigLoadError("Failed to open " + arg);
                }
                load(file, arg, aJsonMergeStrategy);
            }
        }
        mVisitedNodes.clear();
    }

    void Config::setKeyJsonValue(const std::string& aKey, const Json::Value& aValue,
                                 const JsonMergeStrategy& aJsonMergeStrategy)
    {
        aJsonMergeStrategy.merge(mRootJson, jsonOfKeyValue(aKey, aValue), {});
        mRoot = ConfigValue(*this, mRoot.getName().size() ? mRoot.getName() + "+kv" : "kv", {},
                            &mRootJson);
        mVisitedNodes.clear();
    }

    void Config::setKeyValue(const std::string& aKey, const std::string& aValue,
                             const JsonMergeStrategy& aJsonMergeStrategy)
    {
        setKeyJsonValue(aKey, jsonValueOfString(aValue), aJsonMergeStrategy);
    }

    ConfigValue Config::operator[](const std::string& aPath) const
    {
        if (!mRoot)
        {
            throw ConfigKeyNotFound({ConfigTraverse("no config")});
        }
        return traverse(mRoot, configPathOfString(aPath));
    }

    ConfigValue Config::tryTraverse(const ConfigValue& aNode, const ConfigPath& aPath) const
    {
        try
        {
            return traverse(aNode, aPath);
        }
        catch (ConfigKeyNotFound&)
        {
            return ConfigValue();
        }
    }

    ConfigValue Config::root() const
    {
        return mRoot;
    }

    ConfigValue Config::traverse(const ConfigValue& aNode, const ConfigPath& aPath) const
    {
        ConfigValue node = aNode;
        visitNode(*node);
        for (const auto& traverse : aPath)
        {
            if (!node)
            {
                throw ConfigKeyNotFound(node.getPath());
            }
            else if (node->type() == Json::objectValue &&
                     traverse.getType() == ConfigTraverseType::Field)
            {
                ConfigValue prevNode = node;
                std::string nodeName = traverse.getField();

                node = ConfigValue(
                    *this, nodeName, prevNode.getPath(),
                    (*node.mValue).isMember(nodeName) ? &(*node.mValue)[nodeName] : nullptr);

                // It's OK to end up to an invalid node; existence of the key is
                // checked when its value is being inspected
                if (node)
                {
                    visitNode(*node);
                }
            }
            else if (node->type() == Json::arrayValue &&
                     traverse.getType() == ConfigTraverseType::ArrayIndex)
            {
                ConfigValue prevNode = node;
                std::string nodeName = "[" + Utils::to_string(traverse.getIndex()) + "]";
                node = ConfigValue(*this, nodeName, prevNode.getPath(),
                                   traverse.getIndex() < node.mValue->size()
                                       ? &(*node.mValue)[traverse.getIndex()]
                                       : nullptr);
                // It's OK to end up to an invalid node; existence of the key is
                // checked when its value is being inspected
                if (node)
                {
                    visitNode(*node);
                }
            }
            else
            {
                throw ConfigKeyNotObject(node.getPath());
            }
        }

        return node;
    }

    void Config::visitNode(const Json::Value& aNode) const
    {
        mVisitedNodes.insert(&aNode);
    }

    namespace
    {
        JsonNodeSet makeNodeSet(const Json::Value& aValue)
        {
            JsonNodeSet nodes;

            nodes.insert(&aValue);
            switch (aValue.type())
            {
            case Json::objectValue:
            case Json::arrayValue:
            {
                for (auto& obj : aValue)
                {
                    JsonNodeSet subNodes = makeNodeSet(obj);
                    nodes.insert(subNodes.begin(), subNodes.end());
                }
                break;
            }
            default:
                /* ignore */
                ;
            }

            return nodes;
        }

        struct LinkedStrings
        {
            std::string name;
            const LinkedStrings* parent;
        };

        std::string pathOfLinkedStrings(const LinkedStrings& aNode)
        {
            std::string str;
            const LinkedStrings* iterator = &aNode;
            ;
            while (iterator)
            {
                str = iterator->name + (str.size() ? "." + str : "");
                iterator = iterator->parent;
            }
            return str;
        }

        ConfigWarnings makeNonVisitedWarnings(const LinkedStrings& aNode, const Json::Value& aValue,
                                              const JsonNodeSet& aNonVisited)
        {
            ConfigWarnings warnings;

            switch (aValue.type())
            {
            case Json::objectValue:
            {
                for (auto& memberName : aValue.getMemberNames())
                {
                    LinkedStrings next{memberName, &aNode};
                    const Json::Value& obj = aValue[memberName];
                    ConfigWarnings subNodes = makeNonVisitedWarnings(next, obj, aNonVisited);
                    warnings.splice(warnings.end(), std::move(subNodes));
                }
                break;
            }
            case Json::arrayValue:
            {
                for (Json::ArrayIndex index = 0u; index < aValue.size(); ++index)
                {
                    LinkedStrings next{"[" + Utils::to_string(index) + "]", &aNode};
                    const Json::Value& obj = aValue[index];
                    ConfigWarnings subNodes = makeNonVisitedWarnings(next, obj, aNonVisited);
                    warnings.splice(warnings.end(), std::move(subNodes));
                }
                break;
            }
            default:
                if (aNonVisited.count(&aValue))
                {
                    warnings.push_back(pathOfLinkedStrings(aNode) + " was not used");
                }
            }

            return warnings;
        }
    }  // namespace

    std::list<std::string> Config::warnings() const
    {
        JsonNodeSet allNodes = makeNodeSet(mRootJson);
        JsonNodeSet notVisited;
        std::set_difference(allNodes.begin(), allNodes.end(), mVisitedNodes.begin(),
                            mVisitedNodes.end(), std::inserter(notVisited, notVisited.end()));

        LinkedStrings root{mRoot.getName(), nullptr};
        ConfigWarnings warnings = makeNonVisitedWarnings(root, mRootJson, notVisited);

        return warnings;
    }

    void Config::fillMissing(const ConfigValue& aLeft, const ConfigValue& aRight,
                             const JsonMergeStrategy& aJsonMergeStrategy)
    {
        if (!aLeft)
        {
            throw ConfigKeyNotFound(aLeft.getPath());
        }
        if (!aRight)
        {
            throw ConfigKeyNotFound(aRight.getPath());
        }
        Json::Value filled = *aRight.mValue;
        aJsonMergeStrategy.merge(filled, *aLeft.mValue, {});
        *aLeft.mValue = filled;
    }

    void Config::merge(const ConfigValue& aLeft, const ConfigValue& aRight,
                       const JsonMergeStrategy& aJsonMergeStrategy)
    {
        if (!aLeft)
        {
            throw ConfigKeyNotFound(aLeft.getPath());
        }
        if (!aRight)
        {
            throw ConfigKeyNotFound(aRight.getPath());
        }
        Json::Value filled = *aLeft.mValue;
        aJsonMergeStrategy.merge(filled, *aRight.mValue, {});
        *aLeft.mValue = filled;
    }

    void Config::remove(const ConfigValue& aNode, ConfigTraverse aTraverse)
    {
        if (!aNode)
        {
            throw ConfigKeyNotFound(aNode.getPath());
        }
        switch (aTraverse.getType())
        {
        case ConfigTraverseType::ArrayIndex:
        {
            Json::Value value;
            aNode.mValue->removeIndex(aTraverse.getIndex(), &value);
            break;
        }
        case ConfigTraverseType::Field:
        {
            aNode.mValue->removeMember(aTraverse.getField());
            break;
        }
        case ConfigTraverseType::None:
        {
            break;
        }
        }
    }

    std::string stringOfConfigPath(const ConfigPath& aPath)
    {
        std::string str;
        bool first = true;
        for (auto& traverse : aPath)
        {
            if (!first && traverse.getType() == ConfigTraverseType::Field)
            {
                str += "::";
            }
            first = false;
            str += traverse.asString();
        }
        return str;
    }

    ConfigPath configPathOfString(std::string aString)
    {
        ConfigPath path;
        for (const auto& element : Utils::split(aString, '.'))
        {
            path.push_back(ConfigTraverse(element));
        }
        return path;
    }

    std::ostream& operator<<(std::ostream& aStream, const Config& aConfig)
    {
        Json::StyledStreamWriter writer;
        writer.write(aStream, *aConfig.root());
        return aStream;
    }
}  // namespace VDD
