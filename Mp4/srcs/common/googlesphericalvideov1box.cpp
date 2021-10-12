
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
#include "googlesphericalvideov1box.hpp"
#include <algorithm>
#include <string>
#include "log.hpp"

SphericalVideoV1Box::SphericalVideoV1Box()
    : Box("uuid")
    , mXMLMetadata()
    , mGlobalMetadata{}
{
    Vector<uint8_t> uuid = GOOGLE_SPHERICAL_VIDEO_V1_GLOBAL_UUID;
    setUserType(uuid);
}

void SphericalVideoV1Box::setGlobalMetadata(const GlobalMetadata& globalMetadata)
{
    mGlobalMetadata = globalMetadata;
}

const SphericalVideoV1Box::GlobalMetadata& SphericalVideoV1Box::getGlobalMetadata() const
{
    return mGlobalMetadata;
}

template <typename T>
void SphericalVideoV1Box::readTag(const String& tag, T& value)
{
    String xmltag = "<" + tag + ">";
    size_t offset = mXMLMetadata.find(xmltag);
    if (offset != std::string::npos)
    {
        size_t endOffset = mXMLMetadata.find("</" + tag + ">");
        if (endOffset != std::string::npos)
        {
            if (endOffset > offset && (endOffset - offset - xmltag.length()) > 0)
            {
                String valueString =
                    mXMLMetadata.substr(offset + xmltag.length(), endOffset - offset - xmltag.length());
                remove_if(valueString.begin(), valueString.end(), isspace);
                value = static_cast<T>(std::stoi(valueString.c_str(), nullptr));
            }
        }
    }
}

template <>
void SphericalVideoV1Box::readTag(const String& tag, bool& value)
{
    String xmltag = "<" + tag + ">";
    size_t offset = mXMLMetadata.find(xmltag);
    if (offset != std::string::npos)
    {
        size_t endOffset = mXMLMetadata.find("</" + tag + ">");
        if (endOffset != std::string::npos)
        {
            if (endOffset > offset && (endOffset - offset - xmltag.length()) > 0)
            {
                String valueString =
                    mXMLMetadata.substr(offset + xmltag.length(), endOffset - offset - xmltag.length());
                if (valueString.find("true") != std::string::npos || valueString.find("1") != std::string::npos)
                {
                    value = true;
                }
                else if (valueString.find("false") != std::string::npos || valueString.find("0") != std::string::npos)
                {
                    value = false;
                }
                else
                {
                    logWarning() << "Parsing Error in SphericalVideoV1Box/" << tag << " value " << valueString
                                 << std::endl;
                }
            }
        }
    }
}

template <typename T>
void SphericalVideoV1Box::writeTag(ISOBMFF::BitStream& bitstr, const String& tag, const T value)
{
    String xml = "<" + tag + ">";
    bitstr.writeString(xml);
    xml = std::to_string(value).c_str();
    bitstr.writeString(xml);
    xml = "</" + tag + ">\n";
    bitstr.writeString(xml);
}

template <>
void SphericalVideoV1Box::writeTag(ISOBMFF::BitStream& bitstr, const String& tag, const bool value)
{
    String xml = "<" + tag + ">";
    bitstr.writeString(xml);
    xml = value ? "true" : "false";
    bitstr.writeString(xml);
    xml = "</" + tag + ">\n";
    bitstr.writeString(xml);
}

template <>
void SphericalVideoV1Box::writeTag(ISOBMFF::BitStream& bitstr, const String& tag, const String value)
{
    String xml = "<" + tag + ">";
    bitstr.writeString(xml);
    bitstr.writeString(value);
    xml = "</" + tag + ">\n";
    bitstr.writeString(xml);
}

// This is so that character literals can be used as the value argument
template <>
void SphericalVideoV1Box::writeTag(ISOBMFF::BitStream& bitstr, const String& tag, const char* value)
{
    writeTag(bitstr, tag, String(value));
}

template <>
void SphericalVideoV1Box::readTag(const String& tag, String& value)
{
    String xmltag = "<" + tag + ">";
    size_t offset = mXMLMetadata.find(xmltag);
    if (offset != std::string::npos)
    {
        size_t endOffset = mXMLMetadata.find("</" + tag + ">");
        if (endOffset != std::string::npos)
        {
            if (endOffset > offset && (endOffset - offset - xmltag.length()) > 0)
            {
                value = mXMLMetadata.substr(offset + xmltag.length(), endOffset - offset - xmltag.length());
            }
        }
    }
}

void SphericalVideoV1Box::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);
    String header =
        "<?xml "
        "version=\"1.0\"?><rdf:SphericalVideo\nxmlns:rdf=\"http://www.w3.org/1999/02/"
        "22-rdf-syntax-ns#\"\nxmlns:GSpherical=\"http://ns.google.com/videos/1.0/spherical/\">";
    bitstr.writeString(header);

    writeTag(bitstr, "GSpherical:Spherical", true);  // must be true on v1.0
    writeTag(bitstr, "GSpherical:Stitched", true);   // must be true on v1.0
    writeTag(bitstr, "GSpherical:StitchingSoftware", mGlobalMetadata.stitchingSoftware);
    writeTag(bitstr, "GSpherical:ProjectionType", "equirectangular");  // must be "equirectangular" on v1.0

    if (mGlobalMetadata.stereoMode != V1StereoMode::UNDEFINED)
    {
        String stereoMode;
        if (mGlobalMetadata.stereoMode == V1StereoMode::STEREOSCOPIC_TOP_BOTTOM)
        {
            stereoMode = "top-bottom";
        }
        else if (mGlobalMetadata.stereoMode == V1StereoMode::STEREOSCOPIC_LEFT_RIGHT)
        {
            stereoMode = "left-right";
        }
        else
        {
            stereoMode = "mono";
        }
        writeTag(bitstr, "GSpherical:StereoMode", stereoMode);
    }

    if (mGlobalMetadata.sourceCount)
    {
        writeTag(bitstr, "GSpherical:SourceCount", mGlobalMetadata.sourceCount);
    }
    if (mGlobalMetadata.initialViewHeadingDegrees)
    {
        writeTag(bitstr, "GSpherical:InitialViewHeadingDegrees", mGlobalMetadata.initialViewHeadingDegrees);
    }
    if (mGlobalMetadata.initialViewPitchDegrees)
    {
        writeTag(bitstr, "GSpherical:InitialViewPitchDegrees", mGlobalMetadata.initialViewPitchDegrees);
    }
    if (mGlobalMetadata.initialViewRollDegrees)
    {
        writeTag(bitstr, "GSpherical:InitialViewRollDegrees", mGlobalMetadata.initialViewRollDegrees);
    }
    if (mGlobalMetadata.timestamp)
    {
        writeTag(bitstr, "GSpherical:Timestamp", mGlobalMetadata.timestamp);
    }
    if (mGlobalMetadata.fullPanoWidthPixels)
    {
        writeTag(bitstr, "GSpherical:FullPanoWidthPixels", mGlobalMetadata.fullPanoWidthPixels);
    }
    if (mGlobalMetadata.fullPanoHeightPixels)
    {
        writeTag(bitstr, "GSpherical:FullPanoHeightPixels", mGlobalMetadata.fullPanoHeightPixels);
    }
    if (mGlobalMetadata.croppedAreaImageWidthPixels)
    {
        writeTag(bitstr, "GSpherical:CroppedAreaImageWidthPixels", mGlobalMetadata.croppedAreaImageWidthPixels);
    }
    if (mGlobalMetadata.croppedAreaImageHeightPixels)
    {
        writeTag(bitstr, "GSpherical:CroppedAreaImageHeightPixels", mGlobalMetadata.croppedAreaImageHeightPixels);
    }
    if (mGlobalMetadata.croppedAreaLeftPixels)
    {
        writeTag(bitstr, "GSpherical:CroppedAreaLeftPixels", mGlobalMetadata.croppedAreaLeftPixels);
    }
    if (mGlobalMetadata.croppedAreaTopPixels)
    {
        writeTag(bitstr, "GSpherical:CroppedAreaTopPixels", mGlobalMetadata.croppedAreaTopPixels);
    }

    String footer = "</rdf:SphericalVideo>";
    bitstr.writeString(footer);
    updateSize(bitstr);
}

void SphericalVideoV1Box::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);
    bitstr.readStringWithLen(mXMLMetadata, static_cast<uint32_t>(bitstr.numBytesLeft()));

    // <GSpherical:Spherical>true</GSpherical:Spherical>
    String tag = "GSpherical:Spherical";
    readTag(tag, mGlobalMetadata.spherical);

    // <GSpherical:Stitched>true</GSpherical:Stitched>
    tag = "GSpherical:Stitched";
    readTag(tag, mGlobalMetadata.stitched);

    // <GSpherical:StitchingSoftware>OpenCV for Windows v2.4.9</GSpherical:StitchingSoftware>
    tag = "GSpherical:StitchingSoftware";
    readTag(tag, mGlobalMetadata.stitchingSoftware);

    // <GSpherical:ProjectionType>equirectangular</GSpherical:ProjectionType>
    tag = "GSpherical:ProjectionType";
    String projection;
    readTag(tag, projection);
    std::transform(projection.begin(), projection.end(), projection.begin(), ::tolower);
    if (projection == "equirectangular")
    {
        mGlobalMetadata.projectionType = V1Projection::EQUIRECTANGULAR;
    }
    else
    {
        logWarning() << "Parsing Error in SphericalVideoV1Box/" << tag << " value " << projection << std::endl;
    }

    // <GSpherical:StereoMode>top-bottom</GSpherical:StereoMode>
    tag = "GSpherical:StereoMode";
    String stereoMode;
    readTag(tag, stereoMode);
    std::transform(stereoMode.begin(), stereoMode.end(), stereoMode.begin(), ::tolower);
    if (stereoMode == "mono")
    {
        mGlobalMetadata.stereoMode = V1StereoMode::MONO;
    }
    else if (stereoMode == "top-bottom")
    {
        mGlobalMetadata.stereoMode = V1StereoMode::STEREOSCOPIC_TOP_BOTTOM;
    }
    else if (stereoMode == "left-right")
    {
        mGlobalMetadata.stereoMode = V1StereoMode::STEREOSCOPIC_LEFT_RIGHT;
    }
    else
    {
        mGlobalMetadata.stereoMode = V1StereoMode::UNDEFINED;
    }

    // <GSpherical:SourceCount>6</GSpherical:SourceCount>
    tag = "GSpherical:SourceCount";
    readTag(tag, mGlobalMetadata.sourceCount);

    // <GSpherical:InitialViewHeadingDegrees>90</GSpherical:InitialViewHeadingDegrees>
    tag = "GSpherical:InitialViewHeadingDegrees";
    readTag(tag, mGlobalMetadata.initialViewHeadingDegrees);

    // <GSpherical:InitialViewPitchDegrees>0</GSpherical:InitialViewPitchDegrees>
    tag = "GSpherical:InitialViewPitchDegrees";
    readTag(tag, mGlobalMetadata.initialViewPitchDegrees);

    // <GSpherical:InitialViewRollDegrees>0</GSpherical:InitialViewRollDegrees>
    tag = "GSpherical:InitialViewRollDegrees";
    readTag(tag, mGlobalMetadata.initialViewRollDegrees);

    // <GSpherical:Timestamp>1400454971</GSpherical:Timestamp>
    tag = "GSpherical:Timestamp";
    readTag(tag, mGlobalMetadata.timestamp);

    // <GSpherical:FullPanoWidthPixels>1900</GSpherical:FullPanoWidthPixels>
    tag = "GSpherical:FullPanoWidthPixels";
    readTag(tag, mGlobalMetadata.fullPanoWidthPixels);

    // <GSpherical:FullPanoHeightPixels>960</GSpherical:FullPanoHeightPixels>
    tag = "GSpherical:FullPanoHeightPixels";
    readTag(tag, mGlobalMetadata.fullPanoHeightPixels);

    // <GSpherical:CroppedAreaImageWidthPixels>1920</GSpherical:CroppedAreaImageWidthPixels>
    tag = "GSpherical:CroppedAreaImageWidthPixels";
    readTag(tag, mGlobalMetadata.croppedAreaImageWidthPixels);

    // <GSpherical:CroppedAreaImageHeightPixels>1080</GSpherical:CroppedAreaImageHeightPixels>
    tag = "GSpherical:CroppedAreaImageHeightPixels";
    readTag(tag, mGlobalMetadata.croppedAreaImageHeightPixels);

    // <GSpherical:CroppedAreaLeftPixels>15</GSpherical:CroppedAreaLeftPixels>
    tag = "GSpherical:CroppedAreaLeftPixels";
    readTag(tag, mGlobalMetadata.croppedAreaLeftPixels);

    // <GSpherical:CroppedAreaTopPixels>60</GSpherical:CroppedAreaTopPixels>
    tag = "GSpherical:CroppedAreaTopPixels";
    readTag(tag, mGlobalMetadata.croppedAreaTopPixels);
}
