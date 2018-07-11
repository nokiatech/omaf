
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
#include "config.hpp"

#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "api/streamsegmenter/exceptions.hpp"

namespace StreamSegmenter
{
    namespace Config
    {
        namespace
        {
            // Used for avoiding accidentally passing plain json values to objects
            class JsonAccessor
            {
            public:
                // implicit construction ok
                // note: parent must be alive till the the destruction of this object
                JsonAccessor(const Json::Value& aValue,
                             const std::string aName     = "*unknown*",
                             const JsonAccessor* aParent = nullptr)
                    : mName(aName)
                    , mValue(aValue)
                    , mParent(aParent)
                    , mError(false)
                {
                    if (aValue.isArray())
                    {
                        for (size_t c = 0; c < aValue.size(); ++c)
                        {
                            std::stringstream iStr;
                            iStr << c;
                            mAvailableKeys.insert(iStr.str());
                        }
                    }
                    else if (aValue.isObject())
                    {
                        for (auto& member : aValue.getMemberNames())
                        {
                            mAvailableKeys.insert(member);
                        }
                    }
                }

                JsonAccessor(JsonAccessor&& other)
                    : mName(std::move(other.mName))
                    , mValue(std::move(other.mValue))
                    , mParent(other.mParent)
                    , mAvailableKeys(std::move(other.mAvailableKeys))
                    , mError(other.mError)
                {
                    // nothing
                }

                JsonAccessor(const JsonAccessor&) = delete;
                void operator=(const JsonAccessor&) = delete;

                ~JsonAccessor()
                {
                    if (!getError() && mAvailableKeys.size())
                    {
                        for (const auto& key : mAvailableKeys)
                        {
                            std::cerr << "Warning: unused key ";
                            if (mParent)
                            {
                                std::cerr << mParent->getPath() << "/";
                            }
                            std::cerr << mName << "/" << key << std::endl;
                        }
                    }
                }

                const Json::Value& operator()() const
                {
                    return mValue;
                }

                JsonAccessor operator[](const std::string& key) const
                {
                    mAvailableKeys.erase(key);
                    return JsonAccessor(mValue[key], key, this);
                }

                JsonAccessor operator[](const Json::ArrayIndex i) const
                {
                    std::stringstream iStr;
                    iStr << i;
                    mAvailableKeys.erase(iStr.str());
                    return JsonAccessor(mValue[i], "[" + iStr.str() + "]", this);
                }

                bool getError() const
                {
                    if (mParent)
                    {
                        return mParent->getError();
                    }
                    else
                    {
                        return mError;
                    }
                }

                // sets the error state that propagates all the way to parent
                // this disables the writing out the warning about unused keys
                // in the destructor
                void setError() const
                {
                    if (mParent)
                    {
                        mParent->setError();
                    }
                    else
                    {
                        mError = true;
                    }
                }

                std::string getPath() const
                {
                    if (mParent)
                    {
                        return mParent->getPath() + "/" + mName;
                    }
                    else
                    {
                        return mName;
                    }
                }

            private:
                std::string mName;
                const Json::Value& mValue;
                const JsonAccessor* mParent;
                mutable std::set<std::string> mAvailableKeys;  // used for tracking unused keys
                mutable bool mError;                           // only used if mParent == null
            };

            JsonAccessor required(const JsonAccessor& aParent, std::string aKey)
            {
                JsonAccessor value = aParent[aKey];
                if (value().isNull())
                {
                    value.setError();
                    throw ConfigMissingValueError(value.getPath());
                }
                else
                {
                    return value;
                }
            }

            // removed JsonAccessor optional(const JsonAccessor& aParent, std::string aKey)

            // removed JsonAccessor requiredIf(bool condition, const JsonAccessor& aParent, std::string aKey)

            // Takes a JSON array, a function to convert a JSON element to a
            // value, and returns the list of values
            template <typename U>
            auto readList(const JsonAccessor& aValue, U aFromJson) -> std::list<decltype(aFromJson(aValue()))>
            {
                std::list<decltype(aFromJson(aValue()))> result;
                if (!aValue().isNull())
                {
                    for (Json::ArrayIndex i = 0; i < aValue().size(); ++i)
                    {
                        result.push_back(aFromJson(aValue[i]));
                    }
                }
                return result;
            }

            template <typename U>
            auto readVector(const JsonAccessor& aValue, U aFromJson) -> std::vector<decltype(aFromJson(aValue()))>
            {
                if (!aValue().isNull())
                {
                    if (aValue().isArray())
                    {
                        std::vector<decltype(aFromJson(aValue()))> result(aValue().size());
                        for (Json::ArrayIndex i = 0; i < aValue().size(); ++i)
                        {
                            result[i] = aFromJson(aValue[i]);
                        }
                        return result;
                    }
                    else
                    {
                        aValue.setError();
                        throw ConfigInvalidValueError(aValue.getPath(), "vector");
                    }
                }
                else
                {
                    return std::vector<decltype(aFromJson(aValue()))>();
                }
            }

            template <typename U>
            auto readSet(const JsonAccessor& aValue, U aFromJson) -> std::set<decltype(aFromJson(aValue()))>
            {
                std::set<decltype(aFromJson(aValue()))> result;
                if (!aValue().isNull())
                {
                    if (aValue().isArray())
                    {
                        for (Json::ArrayIndex i = 0; i < aValue().size(); ++i)
                        {
                            result.insert(aFromJson(aValue[i]));
                        }
                    }
                    else
                    {
                        aValue.setError();
                        throw ConfigInvalidValueError(aValue.getPath(), "set");
                    }
                }
                return result;
            }

            template <typename U>
            auto readMap(const JsonAccessor& aValue, std::function<bool(const std::string&)> aValidateKey, U aFromJson)
                -> std::map<std::string, decltype(aFromJson(aValue()))>
            {
                std::map<std::string, decltype(aFromJson(aValue()))> result;
                if (!aValue().isNull())
                {
                    if (aValue().isObject())
                    {
                        for (auto iterator = aValue().begin(); iterator != aValue().end(); ++iterator)
                        {
                            const std::string& k = iterator.name();
                            const auto& v        = aValue[k];
                            if (aValidateKey(k))
                            {
                                result.insert(std::make_pair(k, aFromJson(v)));
                            }
                            else
                            {
                                aValue.setError();
                                throw ConfigInvalidValueError(v.getPath(), "map key");
                            }
                        }
                    }
                    else
                    {
                        aValue.setError();
                        throw ConfigInvalidValueError(aValue.getPath(), "map");
                    }
                }
                return result;
            }

            std::string readString(const JsonAccessor& aValue)
            {
                return aValue().asString();
            }

            template <typename T>
            Rational<T> readRational(const JsonAccessor& aValue)
            {
                if (aValue().isString())
                {
                    std::string string = aValue().asString();
                    auto separator     = string.find('/');
                    if (separator == std::string::npos)
                    {
                        throw ConfigInvalidValueError(aValue.getPath(), "rational");
                    }
                    T num;
                    T den;
                    {
                        std::istringstream stream(string.substr(0, separator));
                        stream >> num;
                        if (!stream)
                        {
                            throw ConfigInvalidValueError(aValue.getPath(), "rational");
                        }
                    }
                    {
                        std::istringstream stream(string.substr(separator + 1, string.size() - (separator + 1)));
                        stream >> den;
                        if (!stream)
                        {
                            throw ConfigInvalidValueError(aValue.getPath(), "rational");
                        }
                    }
                    return Rational<T>(num, den);
                }
                else
                {
                    aValue.setError();
                    throw ConfigInvalidValueError(aValue.getPath(), "rational");
                }
            }

            // removed int readInt(const JsonAccessor& aValue)

            int readPositiveInt(const JsonAccessor& aValue)
            {
                try
                {
                    int x = aValue().asInt();
                    if (x <= 0)
                    {
                        aValue.setError();
                        throw ConfigInvalidValueError(aValue.getPath(), "positive integer");
                    }
                    return x;
                }
                catch (Json::LogicError&)
                {
                    aValue.setError();
                    throw ConfigInvalidValueError(aValue.getPath(), "positive integer");
                }
            }

            auto readSegmentDuration = readRational<Segmenter::Duration::value>;

            auto readTimescale = readRational<FrameDuration::value>;

            Input readInput(const JsonAccessor& aValue)
            {
                Input input{readString(aValue)};
                return input;
            }

            MPD readMPD(const JsonAccessor& aValue)
            {
                MPD mpd;
                mpd.outputMPD = readString(required(aValue, "output_mpd"));
                return mpd;
            }

        }  // anonymous namespace

        Config load(const Json::Value& aConfig)
        {
            Config config;
            JsonAccessor root(aConfig, "config file");
            config.input           = readInput(required(root, "input"));
            config.segmentDuration = readSegmentDuration(required(root, "segment_duration"));
            config.mpd             = readMPD(required(root, "mpd"));
            config.timescale       = readTimescale(required(root, "timescale"));
            config.width           = std::uint16_t(readPositiveInt(required(root, "width")));
            config.height          = std::uint16_t(readPositiveInt(required(root, "height")));
            return config;
        }
    }
}  // namespace StreamSegmenter::Config
