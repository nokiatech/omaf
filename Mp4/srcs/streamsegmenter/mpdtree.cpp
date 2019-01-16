
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
#include <cmath>
#include <ctime>
#include <map>

#include "api/streamsegmenter/mpdtree.hpp"
#include "utils.hpp"

namespace StreamSegmenter
{
    namespace MPDTree
    {
        // bunch of helper functions for writing XML
        namespace
        {
            struct UTCDateTime
            {
                std::time_t utcTime;
            };

            struct FR
            {
                FrameRate frameRate;
            };

            std::string toXml(std::string val)
            {
                return val;
            }

            std::string toXml(const char* val)
            {
                return std::string(val);
            }

            std::string toXml(std::uint32_t x)
            {
                return Utils::to_string(x);
            }

            std::string toXml(const Bool& x)
            {
                return x.val ? "true" : "false";
            }

            std::string toXml(const BoolOrNumber& x)
            {
                return x.isNumber ? toXml(x.numberValue) : toXml(Bool{x.boolValue});
            }

            std::string toXml(std::int32_t x)
            {
                return Utils::to_string(x);
            }

            std::string toXml(std::uint64_t x)
            {
                return Utils::to_string(x);
            }

            // std::string toXml(SubRepId x)
            //{
            //    return Utils::to_string(x.get());
            //}

            std::string toXml(const Duration& x)
            {
                std::ostringstream st;
                double duration   = x.asDouble();
                long milliseconds = long(fmod(duration, 1) * 1000);
                long seconds      = long(duration);
                long hours        = seconds / 60 / 60;
                long minutes      = seconds / 60 % 60;
                st << "PT";
                if (hours)
                {
                    st << hours << "H";
                }
                if (hours || minutes)
                {
                    st << minutes << "M";
                }
                st << seconds % 60;
                if (milliseconds)
                {
                    st << "." << std::setw(3) << std::setfill('0') << milliseconds;
                    st << std::setw(0);
                }
                st << "S";

                return st.str();
            }

            std::string toXml(RatU64 x)
            {
                std::ostringstream st;
                if (x.den == 1)
                {
                    st << Utils::to_string(x.num);
                }
                else
                {
                    st << Utils::to_string(x.num) << "/" << Utils::to_string(x.den);
                }

                return st.str();
            }

            std::string toXml(RatS64 x)
            {
                std::ostringstream st;
                if (x.den == 1)
                {
                    st << Utils::to_string(x.num);
                }
                else
                {
                    st << Utils::to_string(x.num) << "/" << Utils::to_string(x.den);
                }

                return st.str();
            }

            std::string toXml(RepresentationType x)
            {
                switch (x)
                {
                case RepresentationType::Static:
                    return "static";
                case RepresentationType::Dynamic:
                    return "dynamic";
                default:
                    assert(false);
                    return "";
                }
            }

            std::string toXml(UTCDateTime utc)
            {
                std::ostringstream st;
                struct tm newtime;
#ifdef _MSC_VER
                gmtime_s(&newtime, &utc.utcTime);
#else
                gmtime_r(&utc.utcTime, &newtime);
#endif
                st << std::put_time(&newtime, "%FT%T");
                return st.str();
            }

            std::string toXml(FR fr)
            {
                std::ostringstream st;
                if (fr.frameRate.den == 1)
                {
                    return Utils::to_string(fr.frameRate.num);
                }

                st << toXml(std::uint64_t(fr.frameRate.num)) << "/" << toXml(std::uint64_t(fr.frameRate.den));
                return st.str();
            }

            std::string toXml(double x)
            {
                return Utils::to_string(x);
            }

            std::string toXml(const VideoScanType x)
            {
                switch (x)
                {
                case VideoScanType::Interlaced:
                    return "interlaced";
                case VideoScanType::Progressive:
                    return "progressive";
                case VideoScanType::Unknown:
                    return "unknown";
                default:
                    return "invalid_video_scan_type";
                }
            }

            std::string toXml(const OmafProjectionType& x)
            {
                return Utils::to_string((std::uint32_t) x);
            }

            std::string toXml(const OmafRwpkPackingType& x)
            {
                return Utils::to_string((std::uint32_t) x);
            }

            std::string toXml(const OmafViewType& x)
            {
                return Utils::to_string((std::uint32_t) x);
            }

            std::string toXml(const OmafShapeType& x)
            {
                return Utils::to_string((std::uint32_t) x);
            }

            std::string toXml(const OmafQualityType& x)
            {
                return Utils::to_string((std::uint32_t) x);
            }

            template <typename T>
            std::string toXml(const SpaceSeparatedAttrList<T> x)
            {
                std::stringstream ret;

                ret << toXml(*x.begin());

                for (auto i = std::next(x.begin()); i != x.end(); i++)
                {
                    ret << " " << toXml(*i);
                }

                return ret.str();
            }

            template <typename T>
            std::string toXml(const CommaSeparatedAttrList<T> x)
            {
                std::stringstream ret;

                ret << toXml(*x.begin());

                for (auto i = std::next(x.begin()); i != x.end(); i++)
                {
                    ret << "," << toXml(*i);
                }

                return ret.str();
            }

            // TODO: broken regarding UTF8 encoding
            std::string quotedXML(std::string x)
            {
                // https://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references#Predefined_entities_in_XML
                static std::map<char, std::string> predefinedEntities = {
                    {'"', "quot"}, {'&', "amp"}, {'\'', "apos"}, {'<', "lt"}, {'>', "gt"}};

                std::string quoted;
                quoted.reserve(x.size());

                for (char c : x)
                {
                    if (predefinedEntities.count(c))
                    {
                        quoted.append("&");
                        quoted.append(predefinedEntities.at(c));
                        quoted.append(";");
                    }
                    else
                    {
                        quoted.append(1u, c);
                    }
                }

                return quoted;
            }

            /**
             * Writes XML attribute key and value and wraps value to double quotes
             * for writing. Also adds separator after attibute.
             *
             * If 0 is passed as separator, then no separator is added.
             */

            template <typename T>
            std::string xmlAttribute(std::string key, T value, char separator = '\n')
            {
                auto xmlAttrValue   = toXml(value);
                bool hasDoubleQuote = xmlAttrValue.find("\"") != std::string::npos;
                bool hasSingleQuote = xmlAttrValue.find("'") != std::string::npos;

                std::string quoteMark = hasDoubleQuote ? "\'" : "\"";

                if (hasDoubleQuote && hasSingleQuote)
                {
                    throw std::runtime_error("XML attribute cannot have single and double quotes in a same string");
                }

                if (separator == 0)
                {
                    return key + "=" + quoteMark + toXml(value) + quoteMark;
                }
                else
                {
                    return key + "=" + quoteMark + toXml(value) + quoteMark + separator;
                }
            }

            std::string indent(std::uint16_t level)
            {
                return std::string(level, ' ');
            }

            template <typename T>
            void outputAttrIfExist(std::ostream& out,
                                   std::uint16_t indentLevel,
                                   std::string key,
                                   T& value,
                                   char separator = 0)
            {
                if (value)
                {
                    out << indent(indentLevel) << xmlAttribute(key, *value, separator);
                }
            };

            template <typename T>
            void addAttrIfExist(AttributeList& attrList, std::string key, const Utils::Optional<T>& value)
            {
                if (value)
                {
                    attrList.push_back({key, toXml(*value)});
                }
            }

            template <typename T>
            void addAttrIfExist(AttributeList& attrList, std::string key, const SpaceSeparatedAttrList<T>& value)
            {
                if (value.size() > 0)
                {
                    attrList.push_back({key, toXml(value)});
                }
            }

            template <typename T>
            void addAttrIfExist(AttributeList& attrList, std::string key, const CommaSeparatedAttrList<T>& value)
            {
                if (value.size() > 0)
                {
                    attrList.push_back({key, toXml(value)});
                }
            }

            // Print out element + its attributes nicely
            // if attributes list length is > 180 chars, split element to be printed on multiple lines
            void writeXMLElementWithAttributes(std::ostream& out,
                                               std::uint16_t indentLevel,
                                               std::string elementName,
                                               AttributeList& attributes,
                                               bool selfClosing = false)
            {
                std::uint32_t attrLength = 0;
                for (auto& attr : attributes)
                {
                    // length and quotes and trailing whitespace
                    attrLength += attr.first.length() + attr.second.length() + 3;
                }

                if (attrLength > 200 && attributes.size() > 1)
                {
                    // first line prints the <element firstAttribute="value"
                    auto& head = *attributes.begin();
                    out << indent(indentLevel) << "<" << elementName << " " << xmlAttribute(head.first, head.second, 0);

                    // indent to the same level with first attribute
                    const auto attrIndentLevel = indentLevel + elementName.length() + 2;

                    for (auto i = std::next(attributes.begin()); i != attributes.end(); i++)
                    {
                        out << std::endl << indent(attrIndentLevel) << xmlAttribute(i->first, i->second, 0);
                    }

                    out << (selfClosing ? "/>" : ">") << std::endl;
                }
                else
                {
                    // print everything on the same line
                    out << indent(indentLevel) << "<" << elementName;
                    for (auto& attr : attributes)
                    {
                        out << " " << xmlAttribute(attr.first, attr.second, 0);
                    }
                    out << (selfClosing ? "/>" : ">") << std::endl;
                }
            }
        }


        //
        // Instantiate used template classes
        //

        template struct AdaptationSet<Representation>;
        template struct Period<ISOBMFAdaptationSet>;
        template struct MPDRoot<ISOBMFPeriod>;

        template struct AdaptationSet<OmafRepresentation>;
        template struct Period<OmafAdaptationSet>;
        template struct MPDRoot<OmafPeriod>;

        void URLType::writeXML(std::ostream& out, std::uint16_t indentLevel, std::string elementName) const
        {
            AttributeList attrs;
            addAttrIfExist(attrs, "sourceURL", sourceURL);
            addAttrIfExist(attrs, "range", range);
            writeXMLElementWithAttributes(out, indentLevel, elementName, attrs, true);
        }

        void PreselectionType::writeXML(std::ostream& out, std::uint16_t indentLevel, bool isSupplementalProperty) const
        {
            AttributeList attrs = {{"schemeIdUri", "urn:mpeg:dash:preselection:2016"},
                                   {"value", tag + "," + toXml(components)}};

            writeXMLElementWithAttributes(
                out, indentLevel, isSupplementalProperty ? "SupplementalProperty" : "EssentialProperty", attrs, true);
        }

        //
        // OmafProjectionFormat
        //

        void OmafProjectionFormat::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
        }

        void OmafProjectionFormat::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs = {{"schemeIdUri", "urn:mpeg:mpegI:omaf:2017:pf"}};
            addAttrIfExist(attrs, "omaf:projection_type", projectionType);
            writeXMLElementWithAttributes(out, indentLevel, "EssentialProperty", attrs, true);
        }

        //
        // OmafQualityInfo
        //

        void OmafQualityInfo::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
        }

        void OmafQualityInfo::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs;

            attrs.push_back({"quality_ranking", toXml(qualityRanking)});

            addAttrIfExist(attrs, "view_idc", viewIdc);

            addAttrIfExist(attrs, "orig_width", origWidth);
            addAttrIfExist(attrs, "orig_height", origHeight);

            if (sphere)
            {
                attrs.push_back({"centre_azimuth", toXml(sphere->centreAzimuth)});
                attrs.push_back({"centre_elevation", toXml(sphere->centreElevation)});
                attrs.push_back({"centre_tilt", toXml(sphere->centreTilt)});
                attrs.push_back({"azimuth_range", toXml(sphere->azimuthRange)});
                attrs.push_back({"elevation_range", toXml(sphere->elevationRange)});
            }

            writeXMLElementWithAttributes(out, indentLevel, "omaf:qualityInfo", attrs, true);
        }

        //
        // OmafSphereRegionWiseQuality
        //

        void OmafSphereRegionWiseQuality::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            // omaf:sphRegionQuality
            AttributeList srqrAttrs = {{"shape_type", toXml(shapeType)},
                                       {"remaining_area_flag", toXml(remainingArea)},
                                       {"quality_ranking_local_flag", toXml(qualityRankingLocal)},
                                       {"quality_type", toXml(qualityType)}};
            if (!defaultViewIdc)
            {
                srqrAttrs.push_back({"view_idc_presence_flag", "true"});
            }
            addAttrIfExist(srqrAttrs, "default_view_idc", defaultViewIdc);
            writeXMLElementWithAttributes(out, indentLevel, "omaf:sphRegionQuality", srqrAttrs);

            // omaf:qualityInfo elements
            for (auto& qualityInfo : qualityInfos)
            {
                qualityInfo.writeXML(out, indentLevel + 2);
            }

            out << indent(indentLevel) << "</omaf:sphRegionQuality>" << std::endl;
        }

        void OmafSphereRegionWiseQuality::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs = {{"schemeIdUri", "urn:mpeg:mpegI:omaf:2017:srqr"}};
            writeXMLElementWithAttributes(out, indentLevel, "SupplementalProperty", attrs);
            writeInnerXML(out, indentLevel + 2);
            out << indent(indentLevel) << "</SupplementalProperty>" << std::endl;
        }

        //
        // OmafRegionWiseMapping
        //

        void OmafRegionWisePacking::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
        }

        void OmafRegionWisePacking::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs = {{"schemeIdUri", "urn:mpeg:mpegI:omaf:2017:rwpk"}};
            addAttrIfExist(attrs, "omaf:packing_type", packingType);
            writeXMLElementWithAttributes(out, indentLevel, "EssentialProperty", attrs, true);
        }

        void VideoFramePacking::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
        }

        void VideoFramePacking::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs = { { "schemeIdUri", "urn:mpeg:mpegB:cicp:VideoFramePackingType" } };
            attrs.push_back({ "value", toXml(packingType) });

            writeXMLElementWithAttributes(out, indentLevel, "EssentialProperty", attrs, true);
        }

        void BaseURL::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            out << quotedXML(url);
        }

        void BaseURL::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            out << indent(indentLevel) << "<BaseURL>";
            writeInnerXML(out, indentLevel + 2);
            out << "</BaseURL>" << std::endl;
        }

        //
        // MPDNodeWithCommonAttributes
        //

        AttributeList MPDNodeWithCommonAttributes::getXMLAttributes() const
        {
            AttributeList ret;
            addAttrIfExist(ret, "profiles", profiles);
            addAttrIfExist(ret, "width", width);
            addAttrIfExist(ret, "height", height);
            addAttrIfExist(ret, "sar", sar);
            addAttrIfExist(ret, "frameRate", frameRate);

            addAttrIfExist(ret, "audioSamplingRate", audioSamplingRate);

            addAttrIfExist(ret, "mimeType", mimeType);
            addAttrIfExist(ret, "segmentProfiles", segmentProfiles);
            addAttrIfExist(ret, "codecs", codecs);
            addAttrIfExist(ret, "maximumSAPPeriod", maximumSAPPeriod);
            addAttrIfExist(ret, "startWithSAP", startWithSAP);
            addAttrIfExist(ret, "maxPlayoutRate", maxPlayoutRate);
            addAttrIfExist(ret, "codingDependency", codingDependency);
            addAttrIfExist(ret, "scanType", scanType);
            addAttrIfExist(ret, "selectionPriority", selectionPriority);
            addAttrIfExist(ret, "tag", tag);

            return ret;
        }

        //
        // Segments
        //

        SegmentTimelineEntry::SegmentTimelineEntry(std::uint64_t dIn)
            : d(dIn)
        {
        }

        SegmentTimelineEntry::SegmentTimelineEntry(std::uint64_t dIn, std::uint64_t tIn)
            : d(dIn)
            , t(tIn)
        {
        }

        SegmentTimelineEntry::SegmentTimelineEntry(std::uint64_t dIn, std::uint64_t tIn, std::uint64_t nIn)
            : d(dIn)
            , t(tIn)
            , n(nIn)
        {
        }

        SegmentTimelineEntry::SegmentTimelineEntry(std::uint64_t dIn,
                                                   std::uint64_t tIn,
                                                   std::uint64_t nIn,
                                                   std::uint64_t kIn)
            : d(dIn)
            , t(tIn)
            , n(nIn)
            , k(kIn)
        {
        }

        SegmentTimelineEntry::SegmentTimelineEntry(std::uint64_t dIn,
                                                   std::uint64_t tIn,
                                                   std::uint64_t nIn,
                                                   std::uint64_t kIn,
                                                   std::int32_t rIn)
            : d(dIn)
            , t(tIn)
            , n(nIn)
            , k(kIn)
            , r(rIn)
        {
        }

        void SegmentBase::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            out << indent(indentLevel) << "<SegmentBaseNotImplemented/>" << std::endl;
        }

        void SegmentList::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            out << indent(indentLevel) << "<SegmentListNotImplemented/>" << std::endl;
        }

        void SegmentTimeline::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            for (auto& s : ss)
            {
                AttributeList attrs = {{"d", toXml(s.d)}};

                addAttrIfExist(attrs, "t", s.t);
                addAttrIfExist(attrs, "n", s.n);
                addAttrIfExist(attrs, "k", s.k);
                addAttrIfExist(attrs, "r", s.r);

                writeXMLElementWithAttributes(out, indentLevel, "S", attrs, true);
            }
        }

        void SegmentTimeline::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            out << indent(indentLevel) << "<SegmentTimeline>" << std::endl;
            writeInnerXML(out, indentLevel + 2);
            out << indent(indentLevel) << "</SegmentTimeline>" << std::endl;
        }

        AttributeList SegmentBaseInformation::getXMLAttributes() const
        {
            AttributeList ret;

            addAttrIfExist(ret, "timescale", timescale);
            addAttrIfExist(ret, "presentationTimeOffset", presentationTimeOffset);
            addAttrIfExist(ret, "presentationDuration", presentationDuration);
            addAttrIfExist(ret, "timeShiftBufferDepth", timeShiftBufferDepth);
            addAttrIfExist(ret, "indexRange", indexRange);
            addAttrIfExist(ret, "indexRangeExact", indexRangeExact);
            addAttrIfExist(ret, "availabilityTimeOffset", availabilityTimeOffset);
            addAttrIfExist(ret, "availabilityTimeComplete", availabilityTimeComplete);

            return ret;
        }

        void SegmentBaseInformation::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            if (initElement)
            {
                initElement->writeXML(out, indentLevel, "Initialization");
            }

            if (representationIndex)
            {
                representationIndex->writeXML(out, indentLevel, "RepresentationIndex");
            }
        }

        AttributeList MultipleSegmentBaseInformation::getXMLAttributes() const
        {
            AttributeList ret;

            addAttrIfExist(ret, "duration", duration);
            addAttrIfExist(ret, "startNumber", startNumber);

            auto parentAttrs = SegmentBaseInformation::getXMLAttributes();
            ret.insert(ret.end(), parentAttrs.begin(), parentAttrs.end());

            return ret;
        }

        void MultipleSegmentBaseInformation::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            SegmentBaseInformation::writeInnerXML(out, indentLevel);

            if (segmentTimeLine)
            {
                segmentTimeLine->writeXML(out, indentLevel);
            }

            if (bitstreamSwitchingElement)
            {
                bitstreamSwitchingElement->writeXML(out, indentLevel, "BitstreamSwitching");
            }
        }

        void SegmentTemplate::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs;

            addAttrIfExist(attrs, "media", media);
            addAttrIfExist(attrs, "index", index);
            addAttrIfExist(attrs, "initialization", initialization);
            addAttrIfExist(attrs, "bitstreamSwitching", bitstreamSwitching);

            auto parentAttrs = MultipleSegmentBaseInformation::getXMLAttributes();
            attrs.insert(attrs.end(), parentAttrs.begin(), parentAttrs.end());

            std::stringstream innerXml;
            writeInnerXML(innerXml, indentLevel + 2);

            auto hasInnerElements = innerXml.str().length() > 0;

            writeXMLElementWithAttributes(out, indentLevel, "SegmentTemplate", attrs, !hasInnerElements);

            out << innerXml.str();

            if (hasInnerElements)
            {
                out << indent(indentLevel) << "</SegmentTemplate>" << std::endl;
            }
        }

        //
        // Representation
        //

        void Representation::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            if (segmentBase)
            {
                // @todo implement: segmentBase->writeXML(out, indentLevel);
            }

            if (segmentList)
            {
                // @todo implement: segmentList->writeXML(out, indentLevel);
            }

            if (segmentTemplate)
            {
                segmentTemplate->writeXML(out, indentLevel);
            }

            if (baseURL)
            {
                baseURL->writeXML(out, indentLevel);
            }
        }

        void Representation::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs = {{"id", id}, {"bandwidth", toXml(bandwidth)}};

            addAttrIfExist(attrs, "qualityRanking", qualityRanking);
            addAttrIfExist(attrs, "dependencyId", dependencyId);
            addAttrIfExist(attrs, "associationId", associationId);
            addAttrIfExist(attrs, "associationType", associationType);
            addAttrIfExist(attrs, "mediaStreamStructureId", mediaStreamStructureId);

            auto parentAttrs = MPDNodeWithCommonAttributes::getXMLAttributes();
            attrs.insert(attrs.end(), parentAttrs.begin(), parentAttrs.end());

            writeXMLElementWithAttributes(out, indentLevel, "Representation", attrs);

            writeInnerXML(out, indentLevel + 2);

            out << indent(indentLevel) << "</Representation>" << std::endl;
        }

        //
        // OmafRepresentation
        //

        void OmafRepresentation::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            Representation::writeInnerXML(out, indentLevel);

            if (regionWisePacking)
            {
                regionWisePacking->writeXML(out, indentLevel);
            }

            if (projectionFormat)
            {
                projectionFormat->writeXML(out, indentLevel);
            }

            for (auto& srqr : sphereRegionQualityRanks)
            {
                srqr.writeXML(out, indentLevel);
            }
        }

        void OmafRepresentation::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            Representation::writeXML(out, indentLevel);
        }

        //
        // AdaptationSet<>
        //

        template <class RepresentationType>
        AttributeList AdaptationSet<RepresentationType>::getXMLAttributes() const
        {
            AttributeList ret;
            addAttrIfExist(ret, "id", id);
            addAttrIfExist(ret, "group", group);

            auto parentAttrs = MPDNodeWithCommonAttributes::getXMLAttributes();
            ret.insert(ret.end(), parentAttrs.begin(), parentAttrs.end());

            addAttrIfExist(ret, "lang", lang);
            addAttrIfExist(ret, "contentType", contentType);
            addAttrIfExist(ret, "par", par);
            addAttrIfExist(ret, "minBandwidth", minBandwidth);
            addAttrIfExist(ret, "maxBandwidth", maxBandwidth);
            addAttrIfExist(ret, "minWidth", minWidth);
            addAttrIfExist(ret, "maxWidth", maxWidth);
            addAttrIfExist(ret, "minHeight", minHeight);
            addAttrIfExist(ret, "maxHeight", maxHeight);
            addAttrIfExist(ret, "minFramerate", minFramerate);
            addAttrIfExist(ret, "maxFramerate", maxFramerate);

            addAttrIfExist(ret, "segmentAlignment", segmentAlignment);
            addAttrIfExist(ret, "bitstreamSwitching", bitstreamSwitching);
            addAttrIfExist(ret, "subsegmentAlignment", subsegmentAlignment);
            addAttrIfExist(ret, "subsegmentStartsWithSAP", subsegmentStartsWithSAP);

            return ret;
        }

        template <class RepresentationType>
        void AdaptationSet<RepresentationType>::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            for (auto& vp : viewpoints)
            {
                AttributeList attrs = {{"schemeIdUri", "urn:mpeg:dash:viewpoint:2011"}, {"value", vp}};
                writeXMLElementWithAttributes(out, indentLevel, "Viewpoint", attrs, true);
            }

            if (preselection)
            {
                bool isSupplemental = !!id && preselection->components.front() == *id;
                preselection->writeXML(out, indentLevel, isSupplemental);
            }

            if (videoFramePacking)
            {
                videoFramePacking->writeXML(out, indentLevel);
            }

            for (auto& representation : representations)
            {
                representation.writeXML(out, indentLevel);
            }
        }

        template <class RepresentationType>
        void AdaptationSet<RepresentationType>::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            auto attrs = getXMLAttributes();

            writeXMLElementWithAttributes(out, indentLevel, "AdaptationSet", attrs);

            writeInnerXML(out, indentLevel + 2);

            out << indent(indentLevel) << "</AdaptationSet>" << std::endl;
        }

        //
        // OmafCoverageInfo
        //

        void OmafCoverageInfo::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
        }

        void OmafCoverageInfo::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs;

            addAttrIfExist(attrs, "view_idc", viewIdc);

            attrs.push_back({"centre_azimuth", toXml(region.centreAzimuth)});
            attrs.push_back({"centre_elevation", toXml(region.centreElevation)});
            attrs.push_back({"centre_tilt", toXml(region.centreTilt)});
            attrs.push_back({"azimuth_range", toXml(region.azimuthRange)});
            attrs.push_back({"elevation_range", toXml(region.elevationRange)});

            writeXMLElementWithAttributes(out, indentLevel, "omaf:coverageInfo", attrs, true);
        }

        //
        // OmafContentCoverage
        //

        void OmafContentCoverage::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            // omaf:cc
            AttributeList ccAttrs = {{"shape_type", toXml(shapeType)}};
            if (!defaultViewIdc)
            {
                ccAttrs.push_back({"view_idc_presence_flag", "true"});
            }
            addAttrIfExist(ccAttrs, "default_view_idc", defaultViewIdc);
            writeXMLElementWithAttributes(out, indentLevel, "omaf:cc", ccAttrs);

            // omaf:coverageInfo elements
            for (auto& coverageInfo : coverageInfos)
            {
                coverageInfo.writeXML(out, indentLevel + 2);
            }

            out << indent(indentLevel) << "</omaf:cc>" << std::endl;
        }

        void OmafContentCoverage::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs = {{"schemeIdUri", "urn:mpeg:mpegI:omaf:2017:cc"}};
            writeXMLElementWithAttributes(out, indentLevel, "SupplementalProperty", attrs);
            writeInnerXML(out, indentLevel + 2);
            out << indent(indentLevel) << "</SupplementalProperty>" << std::endl;
        }

        //
        // OmafAdaptationSet
        //

        void OmafAdaptationSet::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            if (projectionFormat)
            {
                projectionFormat->writeXML(out, indentLevel);
            }

            if (regionWisePacking)
            {
                regionWisePacking->writeXML(out, indentLevel);
            }

            for (auto& cc : contentCoverages)
            {
                cc.writeXML(out, indentLevel);
            }

            for (auto& srqr : sphereRegionQualityRanks)
            {
                srqr.writeXML(out, indentLevel);
            }

            AdaptationSet<OmafRepresentation>::writeInnerXML(out, indentLevel);
        }

        void OmafAdaptationSet::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            auto parentAttrs = AdaptationSet<OmafRepresentation>::getXMLAttributes();

            writeXMLElementWithAttributes(out, indentLevel, "AdaptationSet", parentAttrs);

            writeInnerXML(out, indentLevel + 2);

            out << indent(indentLevel) << "</AdaptationSet>" << std::endl;
        }

        //
        // Period<>
        //

        template <class AdaptationSetType>
        void Period<AdaptationSetType>::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            for (auto& adaptation : adaptationSets)
            {
                adaptation.writeXML(out, indentLevel);
            }
        }

        template <class AdaptationSetType>
        void Period<AdaptationSetType>::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs;

            addAttrIfExist(attrs, "start", start);
            addAttrIfExist(attrs, "duration", duration);
            addAttrIfExist(attrs, "id", id);

            writeXMLElementWithAttributes(out, indentLevel, "Period", attrs);
            writeInnerXML(out, indentLevel + 2);

            out << indent(indentLevel) << "</Period>" << std::endl;
        }

        //
        // MPDRoot<>
        //

        template <class PeriodType>
        AttributeList MPDRoot<PeriodType>::getXMLAttributes() const
        {
            AttributeList ret = {{"xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance"},
                                 {"xmlns", "urn:mpeg:dash:schema:mpd:2011"},
                                 {"xmlns:xlink", "http://www.w3.org/1999/xlink"},
                                 {"xsi:schemaLocation",
                                  "urn:mpeg:dash:schema:mpd:2011 "
                                  "http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-DASH_schema_files/"
                                  "DASH-MPD.xsd"},
                                 {"minBufferTime", toXml(minBufferTime)},
                                 {"type", toXml(type)}};

            if (mediaPresentationDuration)
            {
                ret.push_back({"mediaPresentationDuration", toXml(*mediaPresentationDuration)});
            }

            if (availabilityStartTime)
            {
                ret.push_back({"availabilityStartTime", toXml(UTCDateTime{*availabilityStartTime})});
            }

            if (publishTime)
            {
                ret.push_back({"publishTime", toXml(UTCDateTime{*publishTime})});
            }

            switch (profile)
            {
            case DashProfile::Live:
                ret.push_back({"profiles", "urn:mpeg:dash:profile:isoff-live:2011"});
                break;
            case DashProfile::OnDemand:
                ret.push_back({"profiles", "urn:mpeg:dash:profile:isoff-on-demand:2011"});
                break;
            }

            return ret;
        }

        template <class PeriodType>
        void MPDRoot<PeriodType>::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            for (auto& period : periods)
            {
                period.writeXML(out, indentLevel);
            }
        }

        template <class PeriodType>
        void MPDRoot<PeriodType>::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            AttributeList attrs;
            writeXMLElementWithAttributes(out, indentLevel, "TodoWriteProperIsoBmfElementHere", attrs, true);
        }

        //
        // OmafMPDRoot
        //

        void OmafMPDRoot::writeInnerXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            if (projectionFormat)
            {
                projectionFormat->writeXML(out, indentLevel);
            }

            if (regionWisePacking)
            {
                regionWisePacking->writeXML(out, indentLevel);
            }

            MPDRoot<OmafPeriod>::writeInnerXML(out, indentLevel);
        }

        void OmafMPDRoot::writeXML(std::ostream& out, std::uint16_t indentLevel) const
        {
            out << indent(indentLevel) << "<?xml " << xmlAttribute("version", "1.0", ' ')
                << xmlAttribute("encoding", "utf-8", 0) << "?>" << std::endl;

            AttributeList attrs = {{"xmlns:omaf", "urn:mpeg:mpegI:omaf:2017"}};
            auto parentAttrs    = MPDRoot<OmafPeriod>::getXMLAttributes();

            attrs.insert(attrs.end(), parentAttrs.begin(), parentAttrs.end());

            writeXMLElementWithAttributes(out, indentLevel, "MPD", attrs);

            writeInnerXML(out, indentLevel + 2);

            out << indent(indentLevel) << "</MPD>" << std::endl;
        }
    }
}
