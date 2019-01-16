
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
#include "urimetasampleentrybox.hpp"
#include "log.hpp"

UriMetaSampleEntryBox::UriMetaSampleEntryBox()
    : MetaDataSampleEntryBox("urim")
    , mUriBox()
    , mVRMetaDataType(VRTMDType::UNKNOWN)
    , mHasUriInitBox(false)
    , mUriInitBox()
{
}

UriBox& UriMetaSampleEntryBox::getUriBox()
{
    return mUriBox;
}

bool UriMetaSampleEntryBox::hasUriInitBox()
{
    return mHasUriInitBox;
}

UriInitBox& UriMetaSampleEntryBox::getUriInitBox()
{
    return mUriInitBox;
}

UriMetaSampleEntryBox::VRTMDType UriMetaSampleEntryBox::getVRTMDType() const
{
    return mVRMetaDataType;
}

void UriMetaSampleEntryBox::writeBox(BitStream& bitstr)
{
    MetaDataSampleEntryBox::writeBox(bitstr);
    mUriBox.writeBox(bitstr);

    if (mHasUriInitBox)
    {
        mUriInitBox.writeBox(bitstr);
    }

    updateSize(bitstr);
}

void UriMetaSampleEntryBox::parseBox(BitStream& bitstr)
{
    MetaDataSampleEntryBox::parseBox(bitstr);
    UriInitBox::InitBoxType initBoxType = UriInitBox::InitBoxType::UNKNOWN;

    bool uriFound = false;
    while (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "uri ")
        {
            mUriBox.parseBox(subBitstr);
            uriFound        = true;
            mVRMetaDataType = VRTMDType::UNKNOWN;
        }
        else if (boxType == "uriI")
        {
            mHasUriInitBox = true;
            mUriInitBox.setInitBoxType(initBoxType);
            mUriInitBox.parseBox(subBitstr);
        }
        // unsupported boxes are skipped
    }
    if (!uriFound)
    {
        throw RuntimeError("UriMetaSampleEntryBox couldn't found URI box");
    }
}

UriMetaSampleEntryBox* UriMetaSampleEntryBox::clone() const
{
    UriMetaSampleEntryBox* box = CUSTOM_NEW(UriMetaSampleEntryBox, ());

    auto mutableThis = const_cast<UriMetaSampleEntryBox*>(this);

    {
        BitStream bs;
        mutableThis->writeBox(bs);
        bs.reset();
        box->parseBox(bs);
    }

    return box;
}

const Box* UriMetaSampleEntryBox::getConfigurationBox() const
{
    logError() << "UriMetaSampleEntryBox::getConfigurationBox() not impelmented " << std::endl;
    return nullptr;
}

const DecoderConfigurationRecord* UriMetaSampleEntryBox::getConfigurationRecord() const
{
    logError() << "UriMetaSampleEntryBox::getConfigurationRecord() not impelmented" << std::endl;
    return nullptr;
}
