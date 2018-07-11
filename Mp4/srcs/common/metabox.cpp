
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
#include "metabox.hpp"

MetaBox::MetaBox()
    : FullBox("meta", 0, 0)
    , mHandlerBox()
    , mHasPrimaryItemBox()
    , mPrimaryItemBox()
    , mHasDataInformationBox()
    , mDataInformationBox()
    , mHasItemLocationBox()
    , mItemLocationBox()
    , mHasItemProtectionBox()
    , mItemProtectionBox()
    , mHasItemInfoBox()
    , mItemInfoBox(2)
    , mHasItemReferenceBox()
    , mItemReferenceBox()
    , mHasItemDataBox()
    , mItemDataBox()
    , mHasXmlBox()
    , mXmlBox()
{
}

FourCCInt MetaBox::getHandlerType() const
{
    return mHandlerBox.getHandlerType();
}

void MetaBox::setHandlerType(FourCCInt handler)
{
    mHandlerBox.setHandlerType(handler);
}

const PrimaryItemBox& MetaBox::getPrimaryItemBox() const
{
    return mPrimaryItemBox;
}

void MetaBox::setPrimaryItem(const std::uint32_t itemId)
{
    mHasPrimaryItemBox = true;
    mPrimaryItemBox.setItemId(itemId);

    if (mItemLocationBox.hasItemIdEntry(itemId))
    {
        auto urlBox = makeCustomShared<DataEntryUrlBox>();
        urlBox->setFlags(1);  // Flag 0x01 tells the data is in this file. DataEntryUrlBox will write only its header.
        const std::uint16_t index = mDataInformationBox.addDataEntryBox(urlBox);
        mItemLocationBox.setItemDataReferenceIndex(itemId, index);
    }
}

const ItemInfoBox& MetaBox::getItemInfoBox() const
{
    return mItemInfoBox;
}

void MetaBox::setItemInfoBox(const ItemInfoBox& itemInfoBox)
{
    mItemInfoBox    = itemInfoBox;
    mHasItemInfoBox = true;
}

const ItemLocationBox& MetaBox::getItemLocationBox() const
{
    return mItemLocationBox;
}

const ItemReferenceBox& MetaBox::getItemReferenceBox() const
{
    return mItemReferenceBox;
}

const DataInformationBox& MetaBox::getDataInformationBox() const
{
    return mDataInformationBox;
}

void MetaBox::addItemReference(FourCCInt type, const std::uint32_t fromId, const std::uint32_t toId)
{
    mHasItemReferenceBox = true;
    mItemReferenceBox.add(type, fromId, toId);
}

void MetaBox::addIloc(const std::uint32_t itemId,
                      const std::uint32_t offset,
                      const std::uint32_t length,
                      const std::uint32_t baseOffset)
{
    mHasItemLocationBox = true;

    ItemLocationExtent locationExtent;
    locationExtent.mExtentOffset = offset;
    locationExtent.mExtentLength = length;

    ItemLocation itemLocation;
    itemLocation.setItemID(itemId);
    itemLocation.setBaseOffset(baseOffset);
    itemLocation.addExtent(locationExtent);

    mItemLocationBox.addLocation(itemLocation);
}

void MetaBox::addItem(const std::uint32_t itemId, FourCCInt type, const String& name, const bool hidden)
{
    mHasItemInfoBox = true;

    ItemInfoEntry infoBox;
    infoBox.setVersion(2);
    infoBox.setItemType(type);
    infoBox.setItemID(itemId);
    infoBox.setItemName(name);

    if (hidden)
    {
        // Set (flags & 1) == 1 to indicate the item is hidden.
        infoBox.setFlags(1);
    }

    mItemInfoBox.addItemInfoEntry(infoBox);
}

void MetaBox::addMdatItem(const std::uint32_t itemId,
                          FourCCInt type,
                          const String& name,
                          const std::uint32_t baseOffset)
{
    mHasItemLocationBox = true;

    addItem(itemId, type, name);

    ItemLocation itemLocation;
    itemLocation.setItemID(itemId);
    itemLocation.setBaseOffset(baseOffset);
    itemLocation.setConstructionMethod(ItemLocation::ConstructionMethod::FILE_OFFSET);
    mItemLocationBox.addLocation(itemLocation);
}

void MetaBox::addItemExtent(const std::uint32_t itemId, const std::uint32_t offset, const std::uint32_t length)
{
    mHasItemLocationBox = true;

    ItemLocationExtent locationExtent;
    locationExtent.mExtentOffset = offset;
    locationExtent.mExtentLength = length;
    mItemLocationBox.addExtent(itemId, locationExtent);
}

void MetaBox::addIdatItem(const std::uint32_t itemId, FourCCInt type, const String& name, const Vector<uint8_t>& data)
{
    mHasItemLocationBox = true;

    const unsigned int offset = mItemDataBox.addData(data);
    addItem(itemId, type, name);
    ItemLocationExtent locationExtent;
    locationExtent.mExtentOffset = offset;
    locationExtent.mExtentLength = data.size();

    ItemLocation itemLocation;
    itemLocation.setItemID(itemId);
    itemLocation.addExtent(locationExtent);
    itemLocation.setConstructionMethod(ItemLocation::ConstructionMethod::IDAT_OFFSET);
    mItemLocationBox.addLocation(itemLocation);
}

void MetaBox::addItemIdatExtent(const std::uint32_t itemId, const Vector<uint8_t>& data)
{
    mHasItemLocationBox = true;

    const unsigned int offset = mItemDataBox.addData(data);
    ItemLocationExtent locationExtent;
    locationExtent.mExtentOffset = offset;
    locationExtent.mExtentLength = data.size();
    mItemLocationBox.addExtent(itemId, locationExtent);
}

const ItemDataBox& MetaBox::getItemDataBox() const
{
    return mItemDataBox;
}

const ProtectionSchemeInfoBox& MetaBox::getProtectionSchemeInfoBox(std::uint16_t index) const
{
    return mItemProtectionBox.getEntry(index);
}

const XmlBox& MetaBox::getXmlBox() const
{
    return mXmlBox;
}

void MetaBox::setXmlBox(const XmlBox& xmlBox)
{
    mHasXmlBox = true;
    mXmlBox    = xmlBox;
}

void MetaBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);

    mHandlerBox.writeBox(bitstr);
    if (mHasPrimaryItemBox)
    {
        mPrimaryItemBox.writeBox(bitstr);
    }
    if (mHasDataInformationBox)
    {
        mDataInformationBox.writeBox(bitstr);
    }
    if (mHasItemLocationBox)
    {
        mItemLocationBox.writeBox(bitstr);
    }
    if (mHasItemProtectionBox)
    {
        mItemProtectionBox.writeBox(bitstr);
    }
    if (mHasItemInfoBox)
    {
        mItemInfoBox.writeBox(bitstr);
    }
    // Not writing optional box IPMPControlBox
    if (mHasItemReferenceBox)
    {
        mItemReferenceBox.writeBox(bitstr);
    }
    if (mHasItemDataBox)
    {
        mItemDataBox.writeBox(bitstr);
    }
    if (mHasXmlBox)
    {
        mXmlBox.writeBox(bitstr);
    }

    updateSize(bitstr);
}

void MetaBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);

    while (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "hdlr")
        {
            mHandlerBox.parseBox(subBitstr);
        }
        else if (boxType == "pitm")
        {
            mHasPrimaryItemBox = true;
            mPrimaryItemBox.parseBox(subBitstr);
        }
        else if (boxType == "iloc")
        {
            mHasItemLocationBox = true;
            mItemLocationBox.parseBox(subBitstr);
        }
        else if (boxType == "iinf")
        {
            mHasItemInfoBox = true;
            mItemInfoBox.parseBox(subBitstr);
        }
        else if (boxType == "iref")
        {
            mHasItemReferenceBox = true;
            mItemReferenceBox.parseBox(subBitstr);
        }
        else if (boxType == "dinf")
        {
            mHasDataInformationBox = true;
            mDataInformationBox.parseBox(subBitstr);
        }
        else if (boxType == "idat")
        {
            mHasItemDataBox = true;
            mItemDataBox.parseBox(subBitstr);
        }
        else if (boxType == "ipro")
        {
            mHasItemProtectionBox = true;
            mItemProtectionBox.parseBox(subBitstr);
        }
        else if (boxType == "xml ")
        {
            mHasXmlBox = true;
            mXmlBox.parseBox(subBitstr);
        }
        // unsupported boxes are skipped
    }
}
