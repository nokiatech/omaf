
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
#include "NVRMPDOmafAttributes.h"
#include "NVRMPDExtension.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashOmafAttributes);

    namespace MPDAttribute
    {
        /*
        * Represents the MPEG-DASH Streaming common attributes and elements.
        */
        const MimeType VIDEO_MIME_TYPE_OMAF_VI = MimeType("video/mp4 profiles='hevi'");
        const MimeType VIDEO_MIME_TYPE_OMAF_VI_ESC = MimeType("video/mp4 profiles=\"hevi\"");
        const MimeType VIDEO_MIME_TYPE_OMAF_VD = MimeType("video/mp4 profiles='hevd'");
        const MimeType VIDEO_MIME_TYPE_OMAF_VD_ESC = MimeType("video/mp4 profiles=\"hevd\"");
    }


    static SourceType::Enum checkOMAFVideoProjection(const std::vector<dash::xml::INode *>& nodes)
    {
        SourceType::Enum projection = SourceType::INVALID;
        for (std::vector<dash::xml::INode *>::const_iterator node = nodes.cbegin(); node != nodes.cend(); node++)
        {
            if (MPDAttribute::ESSENTIAL_PROP.compare((*node)->GetName().c_str()) == ComparisonResult::EQUAL)
            {
                if (!(*node)->GetAttributes().empty())
                {
                    if (StringCompare(MPDExtension::OMAF_PROJECTION_NODE, (*node)->GetAttributeValue("schemeIdUri").c_str()) == ComparisonResult::EQUAL)
                    {
                        if (StringCompare(MPDExtension::OMAF_PROJECTION_TYPE_EQR, (*node)->GetAttributeValue(MPDExtension::OMAF_PROJECTION_TYPE).c_str()) == ComparisonResult::EQUAL)
                        {
                            // equirect panorama
                            OMAF_LOG_D("Essential info: OMAF equirect panorama");
                            projection = SourceType::EQUIRECTANGULAR_PANORAMA;
                        }
                        else if (StringCompare(MPDExtension::OMAF_PROJECTION_TYPE_CUBE, (*node)->GetAttributeValue(MPDExtension::OMAF_PROJECTION_TYPE).c_str()) == ComparisonResult::EQUAL)
                        {
                            // cubemap
                            OMAF_LOG_D("Essential info: OMAF cubemap");
                            projection = SourceType::CUBEMAP;
                        }
                    }
                }
                else if (!(*node)->GetNodes().empty())
                {
                    // this is for the case where the property is split into xml-hierarchy. Not sure if doing it is valid 
                    const std::vector<dash::xml::INode *>& subnodes = (*node)->GetNodes();
                    for (size_t index = 0; index < subnodes.size() - 1; ++index)
                    {
                        if ((StringCompare("schemeIdUri", subnodes[index]->GetName().c_str()) == ComparisonResult::EQUAL)
                            && (StringCompare(MPDExtension::OMAF_PROJECTION_NODE, subnodes[index]->GetText().c_str()) == ComparisonResult::EQUAL)
                            && (StringCompare(MPDExtension::OMAF_PROJECTION_TYPE, subnodes[index + 1]->GetName().c_str()) == ComparisonResult::EQUAL))
                        {
                            if (StringCompare(MPDExtension::OMAF_PROJECTION_TYPE_EQR, subnodes[index + 1]->GetText().c_str()) == ComparisonResult::EQUAL)
                            {
                                // equirect panorama
                                OMAF_LOG_D("Essential info: OMAF equirect panorama");
                                projection = SourceType::EQUIRECTANGULAR_PANORAMA;
                                break;
                            }
                            else if (StringCompare(MPDExtension::OMAF_PROJECTION_TYPE_CUBE, subnodes[index + 1]->GetText().c_str()) == ComparisonResult::EQUAL)
                            {
                                // cubemap
                                OMAF_LOG_D("Essential info: OMAF cubemap");
                                projection = SourceType::CUBEMAP;
                                break;
                            }
                        }
                    }
                }
            }
        }
        return projection;
    }

    static SourceType::Enum checkOMAFVideoProjection(const std::vector<std::string>& aCodecs, bool_t& aReliable)
    {
        for (size_t codecIndex = 0; codecIndex < aCodecs.size(); codecIndex++)
        {
            if (aCodecs.at(codecIndex).find(MPDExtension::OMAF_CODECS_PROJECTION_EQR) != std::string::npos) 
            {
                aReliable = true;
                return SourceType::EQUIRECTANGULAR_PANORAMA;
            }
            else if (aCodecs.at(codecIndex).find(MPDExtension::OMAF_CODECS_PROJECTION_ERCM) != std::string::npos) 
            {
                // tiled equirect or cubemap, but since we don't support cubemap we can't rely on this alone
                aReliable = false;
                return SourceType::EQUIRECTANGULAR_TILES;
            }
        }
        return SourceType::INVALID;
    }

    SourceType::Enum DashOmafAttributes::getOMAFVideoProjection(DashComponents& aDashComponents)
    {
        // The most reliable way to check video projection in OMAF is from codecs-string. The Projection Format property seems redundant
        const std::vector<std::string> codecs = aDashComponents.adaptationSet->GetCodecs();
        bool_t reliable = false;
        SourceType::Enum projectionFromCodecs = checkOMAFVideoProjection(codecs, reliable);
        if (projectionFromCodecs != SourceType::INVALID && reliable)
        {
            // found from adaptation set
            return projectionFromCodecs;
        }
        if (projectionFromCodecs == SourceType::INVALID)
        {
            // search from representation too
            const std::vector<std::string> codecsRep = aDashComponents.representation->GetCodecs();
            projectionFromCodecs = checkOMAFVideoProjection(codecsRep, reliable);
            if (projectionFromCodecs != SourceType::INVALID && reliable)
            {
                // found from representation
                return projectionFromCodecs;
            }
        }
        // else the codecs didn't contain the projection info (codecs should be the primary source though), check the projection property
        // definition in representation overrides adaptation set, so check it first
        const std::vector<dash::xml::INode *>& reprNodes = aDashComponents.representation->GetAdditionalSubNodes();
        SourceType::Enum projection = checkOMAFVideoProjection(reprNodes);
        if (projection != SourceType::INVALID)
        {
            // found from representation. The codecs has also the hint if there are tiles, but there are other properties where we can interpret that
            return projection;
        }
        // not found, check from adaptation set
        const std::vector<dash::xml::INode *>& nodes = aDashComponents.adaptationSet->GetAdditionalSubNodes();
        projection = checkOMAFVideoProjection(nodes);
        if (projection != SourceType::INVALID)
        {
            // found from adaptation set. The codecs has also the hint if there are tiles, but there are other properties where we can interpret that
            return projection;
        }
        const std::vector<dash::xml::INode *>& mpdNodes = aDashComponents.mpd->GetAdditionalSubNodes();
        projection = checkOMAFVideoProjection(mpdNodes);
        if (projection != SourceType::INVALID)
        {
            // found from adaptation set. The codecs has also the hint if there are tiles, but there are other properties where we can interpret that
            return projection;
        }

        // projection type is a mandatory DASH attribute, but in case it is still missing, we must rely on the codecs, even if it was unreliable (in practice assuming equirect tiles but could be cubemap too)
        return projectionFromCodecs;
    }

    static Error::Enum checkOMAFVideoViewport(const std::vector<dash::xml::INode *>& nodes, DashAttributes::Coverage& aCoverage, SourceType::Enum aProjection, const char_t* aPropertyName, const char_t* aNodeName, const char_t* aSubNodeName)
    {
        bool_t metadataFound = false;
        for (std::vector<dash::xml::INode *>::const_iterator node = nodes.cbegin(); node != nodes.cend(); node++)
        {
            if (MPDAttribute::SUPPLEMENTAL_PROP.compare((*node)->GetName().c_str()) == ComparisonResult::EQUAL)
            {
                if (!(*node)->GetAttributes().empty())
                {
                    if (StringCompare(aPropertyName, (*node)->GetAttributeValue("schemeIdUri").c_str()) == ComparisonResult::EQUAL)
                    {
                        const std::vector<dash::xml::INode*> subnodes = (*node)->GetNodes();
                        if (subnodes.size() > 0 && StringCompare(aNodeName, subnodes[0]->GetName().c_str()) == ComparisonResult::EQUAL)
                        {
                            bool_t shapeEquirect = true;
                            if (subnodes[0]->HasAttribute("shape_type"))
                            {
                                int32_t type = atoi(subnodes[0]->GetAttributeValue("shape_type").c_str());
                                if (type == 0 && aProjection == SourceType::CUBEMAP)
                                {
                                    // 4 great circles
                                    // relevant with cubemap
                                    OMAF_LOG_V("Coverage@shape_type indicates 4 great circles");
                                    shapeEquirect = false;
                                }
                                else if (type == 1 && aProjection == SourceType::EQUIRECTANGULAR_PANORAMA) // TODO when do we detect 180?
                                {
                                    // 2 azimuth & 2 elevation circles
                                    // maps to a rectangle in equirectangular
                                    shapeEquirect = true;
                                }
                                else
                                {
                                    // not supported
                                    OMAF_LOG_W("Coverage@shape_type not supported or not aligned with projection");
                                    return Error::NOT_SUPPORTED;
                                }
                            }
                            else if (aProjection == SourceType::CUBEMAP)
                            {
                                // default is 0, must be used with cubemap only
                                OMAF_LOG_V("Coverage@shape_type not present and default is 4 great circles => cubemap");
                                shapeEquirect = false;
                            }
                            else
                            {
                                OMAF_LOG_W("Coverage@shape_type not present and default is 4 great circles => cubemap, but projection is not cubemap");
                                return Error::NOT_SUPPORTED;
                            }
                            bool_t idc = false;
                            if (subnodes[0]->HasAttribute("view_idc_presence_flag"))
                            {
                                if (subnodes[0]->GetAttributeValue("view_idc_presence_flag").find("1") != std::string::npos ||
                                    subnodes[0]->GetAttributeValue("view_idc_presence_flag").find("true") != std::string::npos)
                                {
                                    idc = true;
                                }
                            }
                            else
                            {
                                // default is 0
                            }
                            if (idc && subnodes[0]->HasAttribute("default_view_idc"))
                            {
                                // is absent if view_idc_presence_flag is 1 and vice versa
                                // would be useful in stereo, at least in asymmetric quality stereo cases. Ignored for now
                            }
                            const std::vector<dash::xml::INode*> coverageNode = subnodes[0]->GetNodes();

                            if (coverageNode.size() > 0 && StringCompare(aSubNodeName, coverageNode[0]->GetName().c_str()) == ComparisonResult::EQUAL)
                            {
                                for (size_t i = 0; i < coverageNode.size(); i++)
                                {
                                    if (idc && coverageNode[0]->HasAttribute("view_idc"))
                                    {
                                        // is absent if view_idc_presence_flag is 1 and vice versa
                                        // would be useful in stereo, at least in asymmetric quality stereo cases. Ignored for now
                                    }
                                    if (coverageNode[0]->HasAttribute("centre_azimuth") && coverageNode[0]->HasAttribute("centre_elevation") 
                                        && coverageNode[0]->HasAttribute("azimuth_range") && coverageNode[0]->HasAttribute("elevation_range"))
                                    {
                                        aCoverage.azimuthCenter = float64_t(atoi(coverageNode[0]->GetAttributeValue("centre_azimuth").c_str())) / 65536;
                                        aCoverage.elevationCenter = float64_t(atoi(coverageNode[0]->GetAttributeValue("centre_elevation").c_str())) / 65536;
                                        //aCoverage.tiltCenter = float64_t(atoi(coverageNode[0]->GetAttributeValue("centre_tilt").c_str())) / 65536;
                                        aCoverage.azimuthRange = float64_t(atoi(coverageNode[0]->GetAttributeValue("azimuth_range").c_str())) / 65536;
                                        aCoverage.elevationRange = float64_t(atoi(coverageNode[0]->GetAttributeValue("elevation_range").c_str())) / 65536;
                                        aCoverage.shapeEquirect = shapeEquirect;
                                        //TODO if there are multiple coverageInfo's, we need multiple coverages as well. But that is more likely to happen in stereo case, so fix it later
                                        metadataFound = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (metadataFound)
        {
            return Error::OK;
        }
        else
        {
            return Error::OK_NO_CHANGE;
        }
    }

    Error::Enum DashOmafAttributes::getOMAFVideoViewport(DashComponents& aDashComponents, DashAttributes::Coverage& aCoverage, SourceType::Enum aProjection)
    {
        bool_t metadataFound = false;
        const std::vector<dash::xml::INode *>& nodes = aDashComponents.adaptationSet->GetAdditionalSubNodes();

        // check covi in adaptation set level
        if (checkOMAFVideoViewport(nodes, aCoverage, aProjection, MPDExtension::OMAF_CC_PROPERTY, MPDExtension::OMAF_CC_NODE, MPDExtension::OMAF_COVERAGE_NODE) == Error::OK)
        {
            return Error::OK;
        }

        // check srqr in adaptation set level
        if (checkOMAFVideoViewport(nodes, aCoverage, aProjection, MPDExtension::OMAF_SRQR_PROPERTY, MPDExtension::OMAF_SRQR_NODE, MPDExtension::OMAF_SRQR_QUALITY_NODE) == Error::OK)
        {
            return Error::OK;
        }

        // check srqr in representation level
        const std::vector<dash::xml::INode *>& reprNodes = aDashComponents.representation->GetAdditionalSubNodes();
        return checkOMAFVideoViewport(reprNodes, aCoverage, aProjection, MPDExtension::OMAF_SRQR_PROPERTY, MPDExtension::OMAF_SRQR_NODE, MPDExtension::OMAF_SRQR_QUALITY_NODE);
    }

    static Error::Enum checkOMAFVideoRWPK(const std::vector<dash::xml::INode *>& nodes)
    {
        for (std::vector<dash::xml::INode *>::const_iterator node = nodes.cbegin(); node != nodes.cend(); node++)
        {
            if (MPDAttribute::ESSENTIAL_PROP.compare((*node)->GetName().c_str()) == ComparisonResult::EQUAL)
            {
                if (!(*node)->GetAttributes().empty())
                {
                    if (StringCompare(MPDExtension::OMAF_RWPK_PROPERTY, (*node)->GetAttributeValue("schemeIdUri").c_str()) == ComparisonResult::EQUAL)
                    {
                        const std::vector<dash::xml::INode*> subnodes = (*node)->GetNodes();
                        if (subnodes.size() > 0 && StringCompare(MPDExtension::OMAF_RWPK_TYPE, subnodes[0]->GetName().c_str()) == ComparisonResult::EQUAL)
                        {
                            const std::vector<dash::xml::INode*> rwpkNode = subnodes[0]->GetNodes();
                            if (rwpkNode.size() > 0 && StringCompare(rwpkNode[0]->GetText().c_str(), "0") == ComparisonResult::EQUAL)
                            {
                                // OK, rectangular packing
                                return Error::OK;
                            }
                            else
                            {
                                // all other values are unspecified, so not supported (table 5.2/OMAF)
                                OMAF_LOG_W("Unsupported region wise packing");
                                return Error::NOT_SUPPORTED;
                            }
                        }
                        else
                        {
                            // use default == 0 == rectangular packing
                            return Error::OK;
                        }
                    }
                }
            }
        }
        return Error::OK_NO_CHANGE;
    }

    Error::Enum DashOmafAttributes::hasOMAFVideoRWPK(DashComponents& aDashComponents)
    {
        Error::Enum result = checkOMAFVideoRWPK(aDashComponents.adaptationSet->GetAdditionalSubNodes());
        if (result == Error::OK)
        {
            return result;
        }

        result = checkOMAFVideoRWPK(aDashComponents.representation->GetAdditionalSubNodes());
        if (result == Error::OK)
        {
            return result;
        }

        result = checkOMAFVideoRWPK(aDashComponents.mpd->GetAdditionalSubNodes());
        return result;
    }


    Error::Enum DashOmafAttributes::getOMAFQualityRanking(const std::vector<dash::xml::INode *>& aNodes, VASTileViewport& aViewPort, uint8_t& aQualityIndex, bool_t& aGlobal)
    {
        bool_t metadataFound = false;

        for (std::vector<dash::xml::INode *>::const_iterator node = aNodes.cbegin(); node != aNodes.cend(); node++)
        {
            if (MPDAttribute::SUPPLEMENTAL_PROP.compare((*node)->GetName().c_str()) == ComparisonResult::EQUAL)
            {
                if (!(*node)->GetAttributes().empty())
                {
                    if (StringCompare(MPDExtension::OMAF_SRQR_PROPERTY, (*node)->GetAttributeValue("schemeIdUri").c_str()) == ComparisonResult::EQUAL)
                    {
                        return getOMAFSRQR(node, aViewPort, aQualityIndex, aGlobal);
                    }
                    else if (StringCompare(MPDExtension::OMAF_2DQR_PROPERTY, (*node)->GetAttributeValue("schemeIdUri").c_str()) == ComparisonResult::EQUAL)
                    {
                        return getOMAF2DQR(node, aViewPort, aQualityIndex, aGlobal);
                    }
                }
            }
        }
        return Error::OK_NO_CHANGE;
    }

    Error::Enum DashOmafAttributes::getOMAFSRQR(std::vector<dash::xml::INode *>::const_iterator& aNode, VASTileViewport& aViewPort, uint8_t& aQualityIndex, bool_t& aGlobal)
    {
        const std::vector<dash::xml::INode*> subnodes = (*aNode)->GetNodes();
        if (subnodes.size() > 0 && StringCompare(MPDExtension::OMAF_SRQR_NODE, subnodes[0]->GetName().c_str()) == ComparisonResult::EQUAL)
        {
            if (subnodes[0]->HasAttribute("shape_type"))
            {
                int32_t type = atoi(subnodes[0]->GetAttributeValue("shape_type").c_str());
                if (type == 0)
                {
                    // 4 great circles
                    // relevant with cubemap 
                    OMAF_LOG_V("sphRegionQuality@shape_type indicates 4 great circles");
                    //return Error::NOT_SUPPORTED;
                }
                else if (type == 1)
                {
                    // 2 azimuth & 2 elevation circles
                    // maps to a rectangle in equirectangular => only supported option for now
                }
                else
                {
                    // not supported
                    return Error::NOT_SUPPORTED;
                }
            }
            else
            {
                // default is 0 
                OMAF_LOG_V("sphRegionQuality@shape_type not present and default is 4 great circles");
                //return Error::NOT_SUPPORTED;
            }
            if (subnodes[0]->HasAttribute("remaining_area_flag"))
            {
                // "0": all regions are specified
                // "1": all except last region are specified, i.e. last one represents the lower quality area (rest of the picture) - should not be relevant with tiles
            }
            else
            {
                // default is 0
            }
            if (subnodes[0]->HasAttribute("view_idc_presence_flag"))
            {
                // "0": view_idc is not signalled in each qualityInfo element
                // "1": is signalled; in stereo mode this enables to have different qualities for left and right eye. Ignored atm
            }
            else
            {
                // default is 0
            }
            if (subnodes[0]->HasAttribute("quality_ranking_local_flag"))
            {
                // if "0"/"false", ranking is global across adaptation sets, having value "0" here
                // if "1"/"true", ranking is local to this adaptation set
                if (subnodes[0]->GetAttributeValue("quality_ranking_local_flag").find("0") != std::string::npos ||
                    subnodes[0]->GetAttributeValue("quality_ranking_local_flag").find("false") != std::string::npos)
                {
                    aGlobal = true;
                }
                else
                {
                    aGlobal = false;
                }
            }
            else
            {
                // default is 0
                aGlobal = true;
            }
            if (subnodes[0]->HasAttribute("quality_type"))
            {
                int32_t type = atoi(subnodes[0]->GetAttributeValue("quality_type").c_str());
                // if "0", resolutions are equal in all regions
                // if "1", at least one region has a different resolution, not supported atm
                if (type == 1)
                {
                    return Error::NOT_SUPPORTED;
                }
            }
            else
            {
                // default is 0
            }
            if (subnodes[0]->HasAttribute("default_view_idc"))
            {
                // "0", all regions are mono
                // "1", all regions represent left view
                // "2", all regions represent right view
                // "3", all regions represent both left and right views
                // ignored atm
            }

            const std::vector<dash::xml::INode *>& qNodes = subnodes[0]->GetNodes();
            if (qNodes.size() > 0 && StringCompare(MPDExtension::OMAF_SRQR_QUALITY_NODE, qNodes[0]->GetName().c_str()) == ComparisonResult::EQUAL)
            {
                uint8_t rankingAvg = 0;
                float64_t l, r, t, b;
                aViewPort.getLeftRight(l, r);
                aViewPort.getTopBottom(t, b);
                for (size_t i = 0; i < qNodes.size(); i++)
                {
                    uint8_t ranking = 0;
                    if (qNodes[i]->HasAttribute("quality_ranking"))
                    {
                        ranking = (uint8_t)atoi(qNodes[i]->GetAttributeValue("quality_ranking").c_str());
                    }
                    if (qNodes[i]->HasAttribute("view_idc"))
                    {
                        // "0": mono
                        // "1": left
                        // "2": right
                        // "3": framepacked
                        // ignored atm, could enable different qualities for left and right eye
                    }
                    // Read orig_width and orig_height if ever needed - assuming it would be usable only with cases having region wise packing

                    float64_t centerA, centerE, rangeA, rangeE;
                    if (qNodes[i]->HasAttribute("centre_azimuth"))
                    {
                        centerA = float64_t(atoi(qNodes[i]->GetAttributeValue("centre_azimuth").c_str())) / 65536;
                    }
                    if (qNodes[i]->HasAttribute("centre_elevation"))
                    {
                        centerE = float64_t(atoi(qNodes[i]->GetAttributeValue("centre_elevation").c_str())) / 65536;
                    }
                    if (qNodes[i]->HasAttribute("centre_tilt"))
                    {
                        float64_t tilt = float64_t(atoi(qNodes[i]->GetAttributeValue("centre_tilt").c_str())) / 65536;
                    }
                    if (qNodes[i]->HasAttribute("azimuth_range"))
                    {
                        rangeA = float64_t(atoi(qNodes[i]->GetAttributeValue("azimuth_range").c_str())) / 65536;
                    }
                    if (qNodes[i]->HasAttribute("elevation_range"))
                    {
                        rangeE = float64_t(atoi(qNodes[i]->GetAttributeValue("elevation_range").c_str())) / 65536;
                    }

                    if ((centerA <= l) && (centerA >= r)
                        && (centerE >= b) && (centerE <= t)
                        && ((centerA - rangeA / 2) <= l) && ((centerA + rangeA/2) >= r)
                        && ((centerE - rangeE / 2) >= b) && ((centerE + rangeE / 2) <= t))
                    {
                        // the area matches with the tile
                        // TODO weight the avg by region size?
                        rankingAvg += ranking;
                    }
                    else
                    {
                        OMAF_LOG_W("sphRegionQuality defines a region that doesn't match with the adaptation set!");
                        return Error::INVALID_DATA;
                    }
                }
                rankingAvg = (uint8_t)(rankingAvg/qNodes.size());
                //TODO if there are multiple regions in a representation, we need to store the region info to representation too; single coverage is not enough. But that is more likely to happen in stereo case, so fix it later

                aQualityIndex = rankingAvg;
            }
            return Error::OK;
        }
        return Error::ITEM_NOT_FOUND;
    }

    Error::Enum DashOmafAttributes::getOMAF2DQR(std::vector<dash::xml::INode *>::const_iterator& aNode, VASTileViewport& aViewPort, uint8_t& aQualityIndex, bool_t& aGlobal)
    {
        const std::vector<dash::xml::INode*> subnodes = (*aNode)->GetNodes();
        if (subnodes.size() > 0 && StringCompare(MPDExtension::OMAF_2DQR_NODE, subnodes[0]->GetName().c_str()) == ComparisonResult::EQUAL)
        {
            if (subnodes[0]->HasAttribute("remaining_area_flag"))
            {
                // "0": all regions are specified
                // "1": all except last region are specified, i.e. last one represents the lower quality area (rest of the picture) - should not be relevant with tiles
            }
            else
            {
                // default is 0
            }
            if (subnodes[0]->HasAttribute("view_idc_presence_flag"))
            {
                // "0": view_idc is not signalled in each qualityInfo element
                // "1": is signalled; in stereo mode this enables to have different qualities for left and right eye. Ignored atm
            }
            else
            {
                // default is 0
            }
            if (subnodes[0]->HasAttribute("quality_ranking_local_flag"))
            {
                // if "0"/"false", ranking is global across adaptation sets, having value "0" here
                // if "1"/"true", ranking is local to this adaptation set
                if (subnodes[0]->GetAttributeValue("quality_ranking_local_flag").find("0") != std::string::npos ||
                    subnodes[0]->GetAttributeValue("quality_ranking_local_flag").find("false") != std::string::npos)
                {
                    aGlobal = true;
                }
                else
                {
                    aGlobal = false;
                }
            }
            else
            {
                // default is 0
                aGlobal = true;
            }
            if (subnodes[0]->HasAttribute("quality_type"))
            {
                int32_t type = atoi(subnodes[0]->GetAttributeValue("quality_type").c_str());
                // if "0", resolutions are equal in all regions
                // if "1", at least one region has a different resolution, not supported atm
                if (type == 1)
                {
                    return Error::NOT_SUPPORTED;
                }
            }
            else
            {
                // default is 0
            }
            if (subnodes[0]->HasAttribute("default_view_idc"))
            {
                // "0", all regions are mono
                // "1", all regions represent left view
                // "2", all regions represent right view
                // "3", all regions represent both left and right views
                // ignored atm
            }

            const std::vector<dash::xml::INode *>& qNodes = subnodes[0]->GetNodes();
            if (qNodes.size() > 0 && StringCompare("omaf:twoDqualityInfo", qNodes[0]->GetName().c_str()) == ComparisonResult::EQUAL)
            {
                uint8_t rankingAvg = 0;
                float64_t l, r, t, b;
                aViewPort.getLeftRight(l, r);
                aViewPort.getTopBottom(t, b);
                for (size_t i = 0; i < qNodes.size(); i++)
                {
                    uint8_t ranking = 0;
                    if (qNodes[i]->HasAttribute("quality_ranking"))
                    {
                        ranking = (uint8_t)atoi(qNodes[i]->GetAttributeValue("quality_ranking").c_str());
                    }
                    if (qNodes[i]->HasAttribute("view_idc"))
                    {
                        // "0": mono
                        // "1": left
                        // "2": right
                        // "3": framepacked
                        // ignored atm, could enable different qualities for left and right eye
                    }
                    // Read orig_width and orig_height if ever needed - assuming it would be usable only with cases having region wise packing

                    uint32_t leftOffset, topOffset, width, height;
                    if (qNodes[i]->HasAttribute("left_offset"))
                    {
                        leftOffset = atoi(qNodes[i]->GetAttributeValue("left_offset").c_str());
                    }
                    if (qNodes[i]->HasAttribute("top_offset"))
                    {
                        topOffset = atoi(qNodes[i]->GetAttributeValue("top_offset").c_str());
                    }
                    if (qNodes[i]->HasAttribute("region_width"))
                    {
                        width = atoi(qNodes[i]->GetAttributeValue("region_width").c_str());
                    }
                    if (qNodes[i]->HasAttribute("region_height"))
                    {
                        height = atoi(qNodes[i]->GetAttributeValue("region_height").c_str());
                    }
                    // TODO the values are in luma pixels, how to map to the viewport which deals with degrees?
                    // TODO weight the avg by region size?
                    rankingAvg += ranking;
                }
                rankingAvg = (uint8_t)(rankingAvg / qNodes.size());
                aQualityIndex = rankingAvg;
            }
            return Error::OK;
        }
        return Error::ITEM_NOT_FOUND;
    }

    static bool_t checkOMAFPreselection(DashComponents& aDashComponents, const FixedString64& aProperty, SupportingAdaptationSetIds& aReferredAdaptationSets)
    {
        const std::vector<dash::xml::INode *>& nodes = aDashComponents.adaptationSet->GetAdditionalSubNodes();
        for (std::vector<dash::xml::INode *>::const_iterator node = nodes.cbegin(); node != nodes.cend(); node++)
        {
            if (aProperty.compare((*node)->GetName().c_str()) == ComparisonResult::EQUAL)
            {
                if (!(*node)->GetAttributes().empty())
                {
                    if (StringCompare(MPDExtension::OMAF_PRESEL_PROPERTY, (*node)->GetAttributeValue("schemeIdUri").c_str()) == ComparisonResult::EQUAL)
                    {
                        const char_t* list = (*node)->GetAttributeValue("value").c_str();
                        const char_t* ids = list + StringFindFirst(list, ",") + 1;

                        // skip whitespace in the front of the list, after the comma
                        char_t c;
                        int skip = sscanf(ids, "%*[ \n\t]%c", &c);
                        ids += skip;
                        while (1)
                        {
                            // read whitespace-separated list of ids
                            uint32_t id = 0;
                            if (sscanf(ids, "%d", &id) == EOF)
                            {
                                break;
                            }
                            aReferredAdaptationSets.add(id);
                            size_t next = StringFindFirst(ids, " ");
                            if (next == Npos)
                            {
                                next = 0;
                            }
                            ids += next + 1;
                        }
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool_t DashOmafAttributes::getOMAFPreselection(DashComponents& aDashComponents, uint32_t aMyId)
    {
        SupportingAdaptationSetIds sets;
        if (checkOMAFPreselection(aDashComponents, MPDAttribute::ESSENTIAL_PROP, sets) && sets.contains(aMyId))
        {
            return true;
        }
        return false;
    }

    bool_t DashOmafAttributes::getOMAFPreselection(DashComponents& aDashComponents, uint32_t aMyId, SupportingAdaptationSetIds& aReferredAdaptationSets)
    {
        if (checkOMAFPreselection(aDashComponents, MPDAttribute::SUPPLEMENTAL_PROP, aReferredAdaptationSets))
        {
            aReferredAdaptationSets.remove(aMyId);
            return true;
        }
        return false;
    }

    bool_t DashOmafAttributes::getOMAFRepresentationDependencies(DashComponents& aDashComponents, RepresentationDependencies& aDependingRepresentationIds)
    {
        const std::vector<dash::mpd::IRepresentation*>& r = aDashComponents.adaptationSet->GetRepresentation();
        for (std::vector<dash::mpd::IRepresentation*>::const_iterator it = r.begin(); it != r.end(); ++it)
        {
            const std::vector<std::string>& dependencyIds = (*it)->GetDependencyId();
            DashDependency dependency;
            dependency.ownRepresentationId = FixedString128((*it)->GetId().c_str());
            for (std::vector<std::string>::const_iterator st = dependencyIds.begin(); st != dependencyIds.end(); ++st)
            {
                dependency.dependendableRepresentations.add(FixedString128((*st).c_str()));
            }
            aDependingRepresentationIds.add(dependency);
        }
        if (aDependingRepresentationIds.isEmpty())
        {
            return false;
        }
        return true;
    }


OMAF_NS_END
