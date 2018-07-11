
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
#include <cassert>

#include "bitstream.hpp"
#include "log.hpp"
#include "originalformatbox.hpp"
#include "projectedomnivideobox.hpp"
#include "restrictedschemeinfobox.hpp"
#include "schemetypebox.hpp"

using std::endl;

RestrictedSchemeInfoBox::RestrictedSchemeInfoBox()
    : Box("rinf")
    , mProjectedOmniVideoBox()
    , mStereoVideoBox()
{
}

RestrictedSchemeInfoBox::RestrictedSchemeInfoBox(const RestrictedSchemeInfoBox& box)
    : Box(box)
    , mOriginalFormatBox(box.mOriginalFormatBox ? std::move(makeCustomUnique<OriginalFormatBox, OriginalFormatBox>(
                                                      *box.mOriginalFormatBox))
                                                : nullptr)
    , mSchemeTypeBox(box.mSchemeTypeBox ? std::move(makeCustomUnique<SchemeTypeBox, SchemeTypeBox>(*box.mSchemeTypeBox))
                                        : nullptr)
    , mProjectedOmniVideoBox(
          box.mProjectedOmniVideoBox
              ? std::move(makeCustomUnique<ProjectedOmniVideoBox, ProjectedOmniVideoBox>(*box.mProjectedOmniVideoBox))
              : nullptr)
    , mStereoVideoBox(box.mStereoVideoBox
                          ? std::move(makeCustomUnique<StereoVideoBox, StereoVideoBox>(*box.mStereoVideoBox))
                          : nullptr)
{
    for (auto& schemeTypeBox : box.mCompatibleSchemeTypes)
    {
        mCompatibleSchemeTypes.push_back(
            makeCustomUnique<CompatibleSchemeTypeBox, CompatibleSchemeTypeBox>(*schemeTypeBox));
    }
}


void RestrictedSchemeInfoBox::writeBox(BitStream& bitstr)
{
    writeBoxHeader(bitstr);
    if (mOriginalFormatBox)
    {
        BitStream subStream;
        mOriginalFormatBox->writeBox(bitstr);
        bitstr.writeBitStream(subStream);
    }
    if (mSchemeTypeBox)
    {
        BitStream subStream;
        mSchemeTypeBox->writeBox(bitstr);
        bitstr.writeBitStream(subStream);
    }
    for (auto& compatibleScheme : mCompatibleSchemeTypes)
    {
        BitStream subStream;
        compatibleScheme->writeBox(bitstr);
        bitstr.writeBitStream(subStream);
    }

    if (mSchemeTypeBox->getSchemeType() == "podv")
    {
        BitStream povdStream;
        mProjectedOmniVideoBox->writeBox(povdStream);

        BitStream stviStream;
        if (mStereoVideoBox)
        {
            mStereoVideoBox->writeBox(stviStream);
        }

        // write schi + povd
        BitStream schiStream;
        schiStream.writeBoxHeaders("schi", povdStream.getSize() + stviStream.getSize());
        schiStream.writeBitStream(povdStream);
        schiStream.writeBitStream(stviStream);
        bitstr.writeBitStream(schiStream);
    }

    updateSize(bitstr);
}

void RestrictedSchemeInfoBox::parseBox(BitStream& bitstr)
{
    // rinf header
    parseBoxHeader(bitstr);

    // if there a data available in the file
    while (bitstr.numBytesLeft() > 0)
    {
        // Extract contained box bitstream and type
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "frma")
        {
            mOriginalFormatBox = std::move(makeCustomUnique<OriginalFormatBox, OriginalFormatBox>());
            mOriginalFormatBox->parseBox(subBitstr);
        }
        else if (boxType == "schm")
        {
            mSchemeTypeBox = std::move(makeCustomUnique<SchemeTypeBox, SchemeTypeBox>());
            mSchemeTypeBox->parseBox(subBitstr);
        }
        else if (boxType == "csch")
        {
            auto compatibeSchemeTypeBox = makeCustomUnique<CompatibleSchemeTypeBox, CompatibleSchemeTypeBox>();
            compatibeSchemeTypeBox->parseBox(subBitstr);
            mCompatibleSchemeTypes.push_back(std::move(compatibeSchemeTypeBox));
        }
        else if (boxType == "schi")
        {
            if (!mSchemeTypeBox)
            {
                throw RuntimeError("Scheme type box was not found, before scheme info box");
            }

            // skip schi box headers
            subBitstr.readBoxHeaders(boxType);

            FourCCInt subSchiBoxType;

            // try to parse internals only if there is enough bytes to contain box inside

            while (subBitstr.numBytesLeft() > 16)
            {
                BitStream subSchiBitstr = subBitstr.readSubBoxBitStream(subSchiBoxType);

                auto schemeType = mSchemeTypeBox->getSchemeType().getString();

                if (schemeType == "podv")
                {
                    if (subSchiBoxType == "povd")
                    {
                        mProjectedOmniVideoBox =
                            std::move(makeCustomUnique<ProjectedOmniVideoBox, ProjectedOmniVideoBox>());
                        mProjectedOmniVideoBox->parseBox(subSchiBitstr);
                    }

                    if (subSchiBoxType == "stvi")
                    {
                        mStereoVideoBox = std::move(makeCustomUnique<StereoVideoBox, StereoVideoBox>());
                        mStereoVideoBox->parseBox(subSchiBitstr);
                    }
                }
                else
                {
                    logWarning() << "Skipping unsupported scheme type '" << schemeType << std::endl;
                    break;
                }
            }
        }
        else
        {
            logWarning() << "Skipping unknown box in rinf'" << boxType << "'" << std::endl;
        }
    }
}

FourCCInt RestrictedSchemeInfoBox::getOriginalFormat() const
{
    if (!mOriginalFormatBox)
    {
        throw RuntimeError("Frma box was not found");
    }
    return mOriginalFormatBox->getOriginalFormat();
}

void RestrictedSchemeInfoBox::setOriginalFormat(FourCCInt origFormat)
{
    if (!mOriginalFormatBox)
    {
        mOriginalFormatBox = makeCustomUnique<OriginalFormatBox, OriginalFormatBox>();
    }

    mOriginalFormatBox->setOriginalFormat(origFormat);
}

FourCCInt RestrictedSchemeInfoBox::getSchemeType() const
{
    if (!mSchemeTypeBox)
    {
        throw RuntimeError("Schm box was not found");
    }
    return mSchemeTypeBox->getSchemeType();
}

SchemeTypeBox& RestrictedSchemeInfoBox::getSchemeTypeBox() const
{
    return *mSchemeTypeBox;
}

void RestrictedSchemeInfoBox::addSchemeTypeBox(UniquePtr<SchemeTypeBox> schemeTypeBox)
{
    mSchemeTypeBox = std::move(schemeTypeBox);
}

bool RestrictedSchemeInfoBox::hasSchemeTypeBox() const
{
    return !!mSchemeTypeBox;
}

ProjectedOmniVideoBox& RestrictedSchemeInfoBox::getProjectedOmniVideoBox() const
{
    if (!mProjectedOmniVideoBox)
    {
        throw RuntimeError("Povd box was not found");
    }
    return *mProjectedOmniVideoBox;
}

void RestrictedSchemeInfoBox::addProjectedOmniVideoBox(UniquePtr<ProjectedOmniVideoBox> povdBox)
{
    mProjectedOmniVideoBox = std::move(povdBox);
}

StereoVideoBox& RestrictedSchemeInfoBox::getStereoVideoBox() const
{
    if (!mStereoVideoBox)
    {
        throw RuntimeError("Stvi box was not found");
    }
    return *mStereoVideoBox;
}

void RestrictedSchemeInfoBox::addStereoVideoBox(UniquePtr<StereoVideoBox> stviBox)
{
    mStereoVideoBox = std::move(stviBox);
}

bool RestrictedSchemeInfoBox::hasStereoVideoBox() const
{
    return !!mStereoVideoBox;
}

Vector<CompatibleSchemeTypeBox*> RestrictedSchemeInfoBox::getCompatibleSchemeTypes() const
{
    Vector<CompatibleSchemeTypeBox*> schemeTypes;
    for (auto& schemeType : mCompatibleSchemeTypes)
    {
        schemeTypes.push_back(schemeType.get());
    }
    return schemeTypes;
}

void RestrictedSchemeInfoBox::addCompatibleSchemeTypeBox(UniquePtr<CompatibleSchemeTypeBox> compatibleSchemeType)
{
    mCompatibleSchemeTypes.push_back(std::move(compatibleSchemeType));
}
