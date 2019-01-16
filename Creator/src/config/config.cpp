
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
#include <algorithm>
#include <iterator>
#include <fstream>
#include <set>

#include "config.h"

#include "common/utils.h"
#include "jsonlib/json.hh"

namespace VDD
{
    namespace {
        using ConfigWarnings = std::list<std::string>;

        template <typename T, typename U>
        T append(const T& aContainer, const U& aValue)
        {
            T ret(aContainer);
            ret.push_back(aValue);
            return ret;
        }

        void merge(Json::Value& aLeft, const Json::Value& aRight)
        {
            if (aLeft.type() == Json::objectValue &&
                aRight.type() == Json::objectValue)
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

                for (auto& add: added)
                {
                    aLeft[add] = aRight[add];
                }
                for (auto& update: common)
                {
                    merge(aLeft[update], aRight[update]);
                }
            }
            else if (aLeft.type() == Json::arrayValue &&
                     aRight.type() == Json::arrayValue)
            {
                // perhaps arrays should merged somehow as well instead of replace wholesale?
                aLeft = aRight;
            }
            else
            {
                // perhaps there could be a check that the types should match? that might make it
                // difficult to remove values at merge, if that is desirable, though.
                aLeft = aRight;
            }
        }

        std::string stringOfJsonValueType(Json::ValueType aType)
        {
            switch (aType)
            {
                case Json::nullValue: return "'null'";
                case Json::intValue: return "signed integer";
                case Json::uintValue: return "unsigned integer";
                case Json::realValue: return "double";
                case Json::stringValue: return "string";
                case Json::booleanValue: return "bool";
                case Json::arrayValue: return "array";
                case Json::objectValue: return "object";
            }
            return "unknown";
        }
    }

    ConfigTraverse::ConfigTraverse(std::string aField)
        : mTraverseType(ConfigTraverseType::Field)
        , mField(aField)
    {
        // nothing
    }

    ConfigTraverse::ConfigTraverse(Json::ArrayIndex aIndex)
        : mTraverseType(ConfigTraverseType::ArrayIndex)
        , mIndex(aIndex)
    {
        // nothing
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

    ConfigError::ConfigError(std::string aName)
        : Exception(aName)
    {
        // nothing
    }

    ConfigValueReadError::ConfigValueReadError(std::string aMessage)
        : ConfigError("ConfigValueReadError")
        , mMessage(aMessage)
    {
        // nothing
    }

    std::string ConfigValueReadError::message() const
    {
        return mMessage;
    }

    ConfigLoadError::ConfigLoadError(std::string aJsonError)
        : ConfigError("ConfigLoadError")
        , mJsonError(aJsonError)
    {
        // nothing
    }

    std::string ConfigLoadError::message() const
    {
        return "Error while loading JSON: " + mJsonError;
    }

    ConfigValueTypeMismatches::ConfigValueTypeMismatches(Json::ValueType aExpectedType, const ConfigValue& aNode)
        : ConfigValueTypeMismatches(stringOfJsonValueType(aExpectedType), aNode)
    {
        // nothing
    }

    ConfigValueTypeMismatches::ConfigValueTypeMismatches(std::string aExpectedType, const ConfigValue& aNode)
        : ConfigKeyError("ConfigValueTypeMismatches",
                         "expected type " + aExpectedType + " but has type " +
                         (aNode.valid() ? stringOfJsonValueType(aNode->type()) : "nill") + " in configuration with value " +
                         aNode.singleLineRepresentation(),
                         aNode.getPath())
    {
        // nothing
    }

    ConfigValueInvalid::ConfigValueInvalid(std::string aMessage, const ConfigValue& aNode)
        : ConfigKeyError("ConfigValueInvalid",
                         "invalid value: " + aMessage + " with value " +
                         aNode.singleLineRepresentation(),
                         aNode.getPath())
    {
        // nothing
    }

    ConfigKeyError::ConfigKeyError(std::string aName, std::string aUserMessage, const ConfigPath& aPath)
        : ConfigError(aName)
        , mUserMessage(aUserMessage)
        , mPath(stringOfConfigPath(aPath))
    {
        // nothing
    }

    std::string ConfigKeyError::message() const
    {
        return "Configuration " + mUserMessage + " at " + mPath;
    }

    ConfigKeyNotFound::ConfigKeyNotFound(const ConfigPath& aPath)
        : ConfigKeyError("ConfigKeyNotFound", "key not found", aPath)
    {
        // nothing
    }

    ConfigKeyNotObject::ConfigKeyNotObject(const ConfigPath& aPath)
        : ConfigKeyError("ConfigKeyNotObject", "expected JSON object but non-object key found", aPath)
    {
        // nothing
    }

    ConfigValueBase::~ConfigValueBase() = default;

    ConfigValue::ConfigValue(const Config& aConfig, std::string aName, const ConfigPath& aParentPath,
                             Json::Value* aValue)
        : mConfig(&aConfig)
        , mName(aName)
        , mPath(append(aParentPath, aName))
        , mValue(aValue)
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

    bool ConfigValue::valid() const
    {
        if (mValue)
        {
            mConfig->visitNode(*mValue);
        }
        return mValue && mValue->type() != Json::nullValue;
    }

    int readInt(const ConfigValue& aNode)
    {
        if (!aNode.valid())
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
        if (!aNode.valid())
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
        if (!aNode.valid())
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
        if (!aNode.valid())
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

    bool readBool(const ConfigValue& aNode)
    {
        if (!aNode.valid())
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
            return mConfig->traverse(*this, { aPath });
        }
        else
        {
            throw ConfigKeyNotFound(append(getPath(), ConfigTraverse("..")));
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
            throw ConfigKeyNotFound(append(getPath(), ConfigTraverse("..")));
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

    std::vector<ConfigValue> ConfigValue::childValues() const
    {
        if (!mValue)
        {
            throw ConfigKeyNotFound(getPath());
        }
        else if (mValue->isObject())
        {
            std::vector<ConfigValue> nodes;
            for (const auto& nodeName: mValue->getMemberNames())
            {
                nodes.push_back(mConfig->traverse(*this, { nodeName }));
            }
            return nodes;
        }
        else if (mValue->isArray())
        {
            std::vector<ConfigValue> nodes;
            for (Json::ArrayIndex index = Json::ArrayIndex(0);
                 index < mValue->size();
                 ++index)
            {
                nodes.push_back(mConfig->traverse(*this, { index }));
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

    ConfigPath ConfigValue::getPath() const
    {
        return mPath;
    }

    ConfigValueWithFallback::ConfigValueWithFallback()
    {
        // nothing
    }

    ConfigValueWithFallback::ConfigValueWithFallback(const ConfigValue& aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    ConfigValueWithFallback::ConfigValueWithFallback(const ConfigValue& aConfig, const ConfigValue& aFallback)
        : mConfig(aConfig)
        , mFallback(aFallback)
    {
        // nothing
    }

    std::string ConfigValueWithFallback::getName() const
    {
        return mConfig.valid() ? mConfig.getName() : mFallback.getName();
    }

    const ConfigValue& ConfigValueWithFallback::get() const
    {
        return mConfig.valid() ? mConfig : mFallback;
    }

    const ConfigValue ConfigValueWithFallback::operator[](std::string aKey) const
    {
        auto primary = mConfig.tryTraverse({ aKey });
        if (primary.valid())
        {
            return primary;
        }
        else
        {
            auto secondary = mFallback.tryTraverse({ aKey });
            return secondary;
        }
    }

    ConfigValue ConfigValueWithFallback::traverse(const ConfigPath& aPath) const
    {
        auto primary = mConfig.tryTraverse(aPath);
        if (primary.valid())
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
        if (primary.valid())
        {
            return primary;
        }
        else
        {
            auto secondary = mFallback.tryTraverse(aPath);
            return secondary;
        }
    }

    Config::Config()
        : mRootJson(Json::objectValue)
        , mRoot(*this, "", {}, &mRootJson)
    {
        // nothing
    }

    Config::~Config() = default;

    bool Config::load(std::istream& aStream, std::string aName)
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
        merge(mRootJson, value);
        mRoot = ConfigValue(*this,
                            mRoot.getName().size() ? mRoot.getName() + "+" + aName : aName,
                            {},
                            &mRootJson);
        mVisitedNodes.clear();
        return ok;
    }

    namespace {
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
                }
                else
                {
                    // If it can be read as a number, make a JSON integer (well double really, but
                    // we don't use doubles in VDD configuration)
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
                else {
                    keyName += aString[n];
                }
            }

            return value;
        }
}

    void Config::commandline(const std::vector<std::string>& mArgs)
    {
        for (size_t n = 1; n < mArgs.size(); ++n)
        {
            auto& arg = mArgs[n];
            if (arg.size() > 2 && arg.substr(0, 2) == "--")
            {
                merge(mRootJson, jsonOfArgument(arg.substr(2)));
                mRoot = ConfigValue(*this,
                                    mRoot.getName().size() ? mRoot.getName() + "+arg" : "arg",
                                    {},
                                    &mRootJson);
            }
            else
            {
                std::ifstream file(arg);
                if (!file)
                {
                    throw ConfigLoadError("Failed to open " + arg);
                }
                load(file, arg);
            }
        }
        mVisitedNodes.clear();
    }

    void Config::setKeyJsonValue(const std::string& aKey, const Json::Value& aValue)
    {
        merge(mRootJson, jsonOfKeyValue(aKey, aValue));
        mRoot = ConfigValue(*this,
                            mRoot.getName().size() ? mRoot.getName() + "+kv" : "kv",
                            {},
                            &mRootJson);
        mVisitedNodes.clear();
    }

    void Config::setKeyValue(const std::string& aKey, const std::string& aValue)
    {
        setKeyJsonValue(aKey, jsonValueOfString(aValue));
    }

    ConfigValue Config::operator[](const std::string& aPath) const
    {
        if (!mRoot.valid())
        {
            throw ConfigKeyNotFound({ ConfigTraverse("no config") });
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
        for (const auto& traverse: aPath)
        {
            if (!node.valid())
            {
                throw ConfigKeyNotFound(node.getPath());
            }
            else if (node->type() == Json::objectValue &&
                     traverse.getType() == ConfigTraverseType::Field)
            {
                ConfigValue prevNode = node;
                std::string nodeName = traverse.getField();

                node = ConfigValue(*this, nodeName, prevNode.getPath(),
                                   (*node.mValue).isMember(nodeName)
                                   ? &(*node.mValue)[nodeName]
                                   : nullptr);

                // It's OK to end up to an invalid node; existence of the key is checked when its
                // value is being inspected
                if (node.valid())
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
                // It's OK to end up to an invalid node; existence of the key is checked when its
                // value is being inspected
                if (node.valid())
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
                    for (auto& obj: aValue)
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
            const LinkedStrings* iterator = &aNode;;
            while (iterator)
            {
                str = iterator->name + (str.size() ? "." + str : "");
                iterator = iterator->parent;
            }
            return str;
        }

        ConfigWarnings makeNonVisitedWarnings(const LinkedStrings& aNode,
                                               const Json::Value& aValue,
                                               const JsonNodeSet& aNonVisited)
        {
            ConfigWarnings warnings;

            switch (aValue.type())
            {
                case Json::objectValue:
                {
                    for (auto& memberName: aValue.getMemberNames())
                    {
                        LinkedStrings next { memberName, &aNode };
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
                        LinkedStrings next { "[" + Utils::to_string(index) + "]", &aNode };
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
    }

    std::list<std::string> Config::warnings() const
    {
        JsonNodeSet allNodes = makeNodeSet(mRootJson);
        JsonNodeSet notVisited;
        std::set_difference(allNodes.begin(), allNodes.end(),
                            mVisitedNodes.begin(), mVisitedNodes.end(),
                            std::inserter(notVisited, notVisited.end()));

        LinkedStrings root { mRoot.getName(), nullptr };
        ConfigWarnings warnings = makeNonVisitedWarnings(root, mRootJson, notVisited);

        return warnings;
    }

    void Config::fillMissing(const ConfigValue& aLeft, const ConfigValue& aRight)
    {
        if (!aLeft.valid())
        {
            throw ConfigKeyNotFound(aLeft.getPath());
        }
        if (!aRight.valid())
        {
            throw ConfigKeyNotFound(aRight.getPath());
        }
        Json::Value filled = *aRight.mValue;
        merge(filled, *aLeft.mValue);
        *aLeft.mValue = filled;
    }

    void Config::remove(const ConfigValue& aNode, ConfigTraverse aTraverse)
    {
        if (!aNode.valid())
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
        for (auto& traverse: aPath)
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
        for (const auto& element: Utils::split(aString, '.'))
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
}
