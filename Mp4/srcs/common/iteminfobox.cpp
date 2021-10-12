
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
#include "iteminfobox.hpp"

#include <stdexcept>

using namespace std;

ItemInfoBox::ItemInfoBox()
    : ItemInfoBox(0)
{
}

ItemInfoBox::ItemInfoBox(const uint8_t version)
    : FullBox("iinf", version, 0)
    , mItemInfoList()
{
}

uint32_t ItemInfoBox::getEntryCount() const
{
    return static_cast<uint32_t>(mItemInfoList.size());
}

std::vector<std::uint32_t> ItemInfoBox::getItemIds() const
{
    std::vector<std::uint32_t> itemIds;
    for (const auto& entry : mItemInfoList)
    {
        itemIds.push_back(entry.getItemID());
    }
    return itemIds;
}

void ItemInfoBox::addItemInfoEntry(const ItemInfoEntry& infoEntry)
{
    mItemInfoList.push_back(infoEntry);
}

const ItemInfoEntry& ItemInfoBox::getItemInfoEntry(const std::uint32_t idx) const
{
    return mItemInfoList.at(idx);
}

ItemInfoEntry ItemInfoBox::getItemById(const uint32_t itemId) const
{
    for (const auto& item : mItemInfoList)
    {
        if (item.getItemID() == itemId)
        {
            return item;
        }
    }
    throw RuntimeError("Requested ItemInfoEntry not found.");
}

void ItemInfoBox::clear()
{
    mItemInfoList.clear();
}

void ItemInfoBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);

    if (getVersion() == 0)
    {
        bitstr.write16Bits(static_cast<std::uint16_t>(mItemInfoList.size()));
    }
    else
    {
        bitstr.write32Bits(static_cast<unsigned int>(mItemInfoList.size()));
    }

    for (auto& entry : mItemInfoList)
    {
        entry.writeBox(bitstr);
    }

    updateSize(bitstr);
}

void ItemInfoBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);

    size_t entryCount = 0;

    if (getVersion() == 0)
    {
        entryCount = bitstr.read16Bits();
    }
    else
    {
        entryCount = bitstr.read32Bits();
    }

    for (size_t i = 0; i < entryCount; ++i)
    {
        ItemInfoEntry infoEntry;
        infoEntry.parseBox(bitstr);
        addItemInfoEntry(infoEntry);
    }
}

unsigned int ItemInfoBox::countNumberOfItems(FourCCInt itemType)
{
    unsigned int numberOfItems = 0;

    for (const auto& entry : mItemInfoList)
    {
        if (entry.getItemType() == itemType)
        {
            ++numberOfItems;
        }
    }

    return numberOfItems;
}

// return item and its index for the specified itemType and itemID
ItemInfoEntry* ItemInfoBox::findItemWithTypeAndID(FourCCInt itemType, const unsigned int itemID, unsigned int& index)
{
    ItemInfoEntry* entry   = nullptr;
    unsigned int currIndex = 0;

    for (auto i = mItemInfoList.begin(); i != mItemInfoList.end(); ++i)
    {
        if (i->getItemType() == itemType)
        {
            if (i->getItemID() == itemID)
            {
                entry = &(*i);
                index = currIndex;
                break;
            }
            else
            {
                ++currIndex;
            }
        }
    }

    return entry;
}

ItemInfoEntry::ItemInfoEntry()
    : FullBox("infe", 0, 0)
    , mItemID(0)
    , mItemProtectionIndex(0)
    , mItemName()
    , mContentType()
    , mContentEncoding()
    , mExtensionType()
    , mItemType()
    , mItemUriType()
{
}

ItemInfoEntry::~ItemInfoEntry()
{
}

void ItemInfoEntry::setItemID(const uint32_t id)
{
    mItemID = id;
}

uint32_t ItemInfoEntry::getItemID() const
{
    return mItemID;
}

void ItemInfoEntry::setItemProtectionIndex(const uint16_t idx)
{
    mItemProtectionIndex = idx;
}

uint16_t ItemInfoEntry::getItemProtectionIndex() const
{
    return mItemProtectionIndex;
}

void ItemInfoEntry::setItemName(const String& name)
{
    mItemName = name;
}

const String& ItemInfoEntry::getItemName() const
{
    return mItemName;
}

void ItemInfoEntry::setContentType(const String& contentType)
{
    mContentType = contentType;
}

const String& ItemInfoEntry::getContentType() const
{
    return mContentType;
}

void ItemInfoEntry::setContentEncoding(const String& contentEncoding)
{
    mContentEncoding = contentEncoding;
}
const String& ItemInfoEntry::getContentEncoding() const
{
    return mContentEncoding;
}

void ItemInfoEntry::setExtensionType(const String& extensionType)
{
    mExtensionType = extensionType;
}

const String& ItemInfoEntry::getExtensionType() const
{
    return mExtensionType;
}

void ItemInfoEntry::setItemType(FourCCInt itemType)
{
    mItemType = itemType;
}

FourCCInt ItemInfoEntry::getItemType() const
{
    return mItemType;
}

void ItemInfoEntry::setItemUriType(const String& itemUriType)
{
    mItemUriType = itemUriType;
}
const String& ItemInfoEntry::getItemUriType() const
{
    return mItemUriType;
}

void ItemInfoEntry::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);

    if (getVersion() == 0 || getVersion() == 1)
    {
        bitstr.write16Bits(static_cast<std::uint16_t>(mItemID));
        bitstr.write16Bits(mItemProtectionIndex);
        bitstr.writeZeroTerminatedString(mItemName);
        bitstr.writeZeroTerminatedString(mContentType);
        bitstr.writeZeroTerminatedString(mContentEncoding);
    }
    if (getVersion() == 1)
    {
        bitstr.writeString(mExtensionType);
        mItemInfoExtension->write(bitstr);
    }
    if (getVersion() >= 2)
    {
        if (getVersion() == 2)
        {
            bitstr.write16Bits(static_cast<std::uint16_t>(mItemID));
        }
        else if (getVersion() == 3)
        {
            bitstr.write32Bits(mItemID);
        }
        bitstr.write16Bits(mItemProtectionIndex);
        bitstr.write32Bits(mItemType.getUInt32());
        bitstr.writeZeroTerminatedString(mItemName);
        if (mItemType == "mime")
        {
            bitstr.writeZeroTerminatedString(mContentType);
            bitstr.writeZeroTerminatedString(mContentEncoding);
        }
        else if (mItemType == "uri ")
        {
            bitstr.writeZeroTerminatedString(mItemUriType);
        }
    }

    updateSize(bitstr);
}

void FDItemInfoExtension::write(ISOBMFF::BitStream& bitstr)
{
    bitstr.writeZeroTerminatedString(mContentLocation);
    bitstr.writeZeroTerminatedString(mContentMD5);
    bitstr.write32Bits((uint32_t)((mContentLength >> 32) & 0xffffffff));
    bitstr.write32Bits((uint32_t)(mContentLength & 0xffffffff));
    bitstr.write32Bits((uint32_t)((mTransferLength >> 32) & 0xffffffff));
    bitstr.write32Bits((uint32_t)(mTransferLength & 0xffffffff));
    bitstr.write8Bits(mEntryCount);
    for (unsigned int i = 0; i < mEntryCount; i++)
    {
        bitstr.write32Bits(mGroupID.at(i));
    }
}

void ItemInfoEntry::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);

    if (getVersion() == 0 || getVersion() == 1)
    {
        mItemID              = bitstr.read16Bits();
        mItemProtectionIndex = bitstr.read16Bits();
        bitstr.readZeroTerminatedString(mItemName);
        bitstr.readZeroTerminatedString(mContentType);
        if (bitstr.numBytesLeft() > 0)  // This is an optional field
        {
            bitstr.readZeroTerminatedString(mContentEncoding);
        }
    }
    if (getVersion() == 1)
    {
        if (bitstr.numBytesLeft() > 0)  // This is an optional field
        {
            bitstr.readStringWithLen(mExtensionType, 4);
        }
        if (bitstr.numBytesLeft() > 0)  // This is an optional field
        {
            FDItemInfoExtension* itemInfoExt = CUSTOM_NEW(FDItemInfoExtension, ());
            mItemInfoExtension.reset(itemInfoExt);
            itemInfoExt->parse(bitstr);
        }
    }
    if (getVersion() >= 2)
    {
        if (getVersion() == 2)
        {
            mItemID = bitstr.read16Bits();
        }
        else if (getVersion() == 3)
        {
            mItemID = bitstr.read32Bits();
        }
        mItemProtectionIndex = bitstr.read16Bits();
        mItemType            = bitstr.read32Bits();
        bitstr.readZeroTerminatedString(mItemName);
        if (mItemType == "mime")
        {
            bitstr.readZeroTerminatedString(mContentType);
            if (bitstr.numBytesLeft() > 0)  // This is an optional field
            {
                bitstr.readZeroTerminatedString(mContentEncoding);
            }
        }
        else if (mItemType == "uri ")
        {
            bitstr.readZeroTerminatedString(mItemUriType);
        }
    }
}

void FDItemInfoExtension::parse(ISOBMFF::BitStream& bitstr)
{
    bitstr.readZeroTerminatedString(mContentLocation);
    bitstr.readZeroTerminatedString(mContentMD5);
    mContentLength = ((uint64_t) bitstr.read32Bits()) << 32;
    mContentLength += bitstr.read32Bits();
    mTransferLength = ((uint64_t) bitstr.read32Bits()) << 32;
    mTransferLength += bitstr.read32Bits();
    mEntryCount = bitstr.read8Bits();
    for (unsigned int i = 0; i < mEntryCount; i++)
    {
        mGroupID.at(i) = bitstr.read32Bits();
    }
}

ItemInfoEntry* ItemInfoBox::findItemWithType(FourCCInt itemType, const unsigned int index)
{
    ItemInfoEntry* entry   = nullptr;
    unsigned int currIndex = 0;

    for (auto i = mItemInfoList.begin(); i != mItemInfoList.end(); ++i)
    {
        if (i->getItemType() == itemType)
        {
            if (index == currIndex)
            {
                entry = &(*i);
                break;
            }
            else
            {
                ++currIndex;
            }
        }
    }

    return entry;
}

Vector<ItemInfoEntry> ItemInfoBox::getItemsByType(FourCCInt itemType) const
{
    Vector<ItemInfoEntry> items;
    for (const auto& i : mItemInfoList)
    {
        if (i.getItemType() == itemType)
        {
            items.push_back(i);
        }
    }
    return items;
}

FDItemInfoExtension::FDItemInfoExtension()
    : mContentLocation()
    , mContentMD5()
    , mContentLength(0)
    , mTransferLength(0)
    , mEntryCount(0)
    , mGroupID(256, 0)
{
}

void FDItemInfoExtension::setContentLocation(const String& location)
{
    mContentLocation = location;
}

const String& FDItemInfoExtension::getContentLocation()
{
    return mContentLocation;
}

void FDItemInfoExtension::setContentMD5(const String& md5)
{
    mContentMD5 = md5;
}

const String& FDItemInfoExtension::getContentMD5()
{
    return mContentMD5;
}

void FDItemInfoExtension::setContentLength(const uint64_t length)
{
    mContentLength = length;
}

uint64_t FDItemInfoExtension::getContentLength()
{
    return mContentLength;
}

void FDItemInfoExtension::setTranferLength(const uint64_t length)
{
    mTransferLength = length;
}

uint64_t FDItemInfoExtension::getTranferLength()
{
    return mTransferLength;
}

void FDItemInfoExtension::setNumGroupID(const uint8_t numID)
{
    mEntryCount = numID;
}

uint8_t FDItemInfoExtension::getNumGroupID()
{
    return mEntryCount;
}

void FDItemInfoExtension::setGroupID(const std::uint32_t idx, const uint32_t id)
{
    mGroupID.at(idx) = id;
}

uint32_t FDItemInfoExtension::getGroupID(const std::uint32_t idx)
{
    return mGroupID.at(idx);
}
