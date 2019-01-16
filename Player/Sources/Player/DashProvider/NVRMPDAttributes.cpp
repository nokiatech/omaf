
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
#include "NVRMPDAttributes.h"
#include "NVRMPDExtension.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashAttributes);

    namespace MPDAttribute
    {
        /*
        * Represents the MPEG-DASH Streaming common attributes and elements.
        */
        const FixedString64 VIDEO_CONTENT = FixedString64("video");
        const FixedString64 AUDIO_CONTENT = FixedString64("audio");
        const FixedString64 METADATA_CONTENT = FixedString64("application");

        const FixedString64 ROLE_URI = FixedString64("urn:mpeg:dash:role:2011");
        const FixedString64 ROLE_METADATA = FixedString64("metadata");
        const FixedString64 ROLE_STEREO_URI = FixedString64("urn:mpeg:dash:stereoid:2011");
        const char_t* ROLE_LEFT = "l";
        const char_t* ROLE_RIGHT = "r";
        const char_t* FRAME_PACKING_SIDE_BY_SIDE = "3";
        const char_t* FRAME_PACKING_TOP_BOTTOM = "4";
        const char_t* FRAME_PACKING_TEMPORAL = "5";

        const MimeType VIDEO_MIME_TYPE = MimeType("video/mp4");
        const MimeType AUDIO_MIME_TYPE = MimeType("audio/mp4");
        const MimeType META_MIME_TYPE = MimeType("application/mp4");

        const FixedString64 SUPPLEMENTAL_PROP = FixedString64("SupplementalProperty");
        const FixedString64 ESSENTIAL_PROP = FixedString64("EssentialProperty");

        const char_t* kAssociationIdKey = "associationId";
        const char_t* kAssociationTypeKey = "associationType";
        const char_t* kAssociationTypeValue = "cdsc";
    }


    StereoRole::Enum DashAttributes::getStereoRole(DashComponents& aDashComponents)
    {
        StereoRole::Enum stereoRole = StereoRole::UNKNOWN;
        const std::vector<dash::mpd::IDescriptor*>& role = aDashComponents.adaptationSet->GetRole();
        // there is no GetRole for representations in libDash, so checking only from adaptation set
        // Further, the role is a vector of strings, but there is no meaning of having role identifiers in any other place than in the front of the vector
        if (role.size() > 0 && MPDAttribute::ROLE_STEREO_URI.compare(role[0]->GetSchemeIdUri().c_str()) == ComparisonResult::EQUAL)
        {
            if (StringFindFirst(role[0]->GetValue().c_str(), MPDAttribute::ROLE_LEFT) != Npos)
            {
                stereoRole = StereoRole::LEFT;
                // The index in the role field has no use - at least currently
                int32_t videoChannelIndex = atoi(role[0]->GetValue().c_str() + 1); // the string format is ln, where l indicates left, and n indicates the view index
                OMAF_LOG_D("Adaptation set for left video channel %d", videoChannelIndex);
            }
            else if (StringFindFirst(role[0]->GetValue().c_str(), MPDAttribute::ROLE_RIGHT) != Npos)
            {
                stereoRole = StereoRole::RIGHT;
                // The index in the role field has no use - at least currently
                int32_t videoChannelIndex = atoi(role[0]->GetValue().c_str() + 1); // the string format is rn, where r indicates right, and n indicates the view index
                OMAF_LOG_D("Adaptation set for right video channel %d", videoChannelIndex);
            }
        }
        return stereoRole;
    }


    static SourceDirection::Enum checkFramePacking(const std::vector<dash::xml::INode *>& nodes)
    {
        SourceDirection::Enum direction = SourceDirection::INVALID;
        for (std::vector<dash::xml::INode *>::const_iterator node = nodes.cbegin(); node != nodes.cend(); node++)
        {
            if (MPDAttribute::ESSENTIAL_PROP.compare((*node)->GetName().c_str()) == ComparisonResult::EQUAL)
            {
                // we use essential property to indicate panorama, viewport, and frame-packing nodes
                if (!(*node)->GetNodes().empty())
                {
                    const std::vector<dash::xml::INode *>& subnodes = (*node)->GetNodes();
                    for (size_t index = 0; index < subnodes.size()-1; ++index)
                    {
                        if ((StringCompare("schemeIdUri", subnodes[index]->GetName().c_str()) == ComparisonResult::EQUAL)
                            && (StringCompare(MPDExtension::FRAME_PACKING, subnodes[index]->GetText().c_str()) == ComparisonResult::EQUAL)
                            && (StringCompare("value", subnodes[index+1]->GetName().c_str()) == ComparisonResult::EQUAL))
                        {
                            OMAF_LOG_D("Essential info: frame packing %s", subnodes[index + 1]->GetText().c_str());
                            if (StringCompare(MPDAttribute::FRAME_PACKING_TOP_BOTTOM, subnodes[index + 1]->GetText().c_str()) == ComparisonResult::EQUAL)
                            {
                                direction = SourceDirection::TOP_BOTTOM;    // we still don't know which one is left channel, which right
                            }
                            else if (StringCompare(MPDAttribute::FRAME_PACKING_SIDE_BY_SIDE, subnodes[index + 1]->GetText().c_str()) == ComparisonResult::EQUAL)
                            {
                                direction = SourceDirection::LEFT_RIGHT;    // we still don't know which one is left channel, which right
                            }
                            else
                            {
                                // not supported, some are not allowed in OMAF, but temporal packing is valid for OMAF but we don't support it for now
                                direction = SourceDirection::INVALID;
                            }
                        }
                    }
                }
                else if (!(*node)->GetAttributes().empty())
                {
                    if ((StringCompare(MPDExtension::FRAME_PACKING, (*node)->GetAttributeValue("schemeIdUri").c_str()) == ComparisonResult::EQUAL))
                    {
                        OMAF_LOG_D("Essential info: frame packing %s", (*node)->GetAttributeValue("value").c_str());
                        if (StringCompare(MPDAttribute::FRAME_PACKING_TOP_BOTTOM, (*node)->GetAttributeValue("value").c_str()) == ComparisonResult::EQUAL)
                        {
                            direction = SourceDirection::TOP_BOTTOM;    // we still don't know which one is left channel, which right
                        }
                        else if (StringCompare(MPDAttribute::FRAME_PACKING_SIDE_BY_SIDE, (*node)->GetAttributeValue("value").c_str()) == ComparisonResult::EQUAL)
                        {
                            direction = SourceDirection::LEFT_RIGHT;    // we still don't know which one is left channel, which right
                        }
                        else
                        {
                            // not supported, some are not allowed in OMAF, but temporal packing is valid for OMAF but we don't support it for now
                            direction = SourceDirection::INVALID;
                        }
                    }
                }
            }
        }
        return direction;
    }

    SourceDirection::Enum DashAttributes::getFramePacking(DashComponents& aDashComponents)
    {
        const std::vector<dash::xml::INode *>& nodes = aDashComponents.adaptationSet->GetAdditionalSubNodes();
        SourceDirection::Enum direction = checkFramePacking(nodes);
        if (direction != SourceDirection::INVALID)
        {
            // found in adaptation set
            return direction;
        }
        return SourceDirection::INVALID;
    }



OMAF_NS_END
