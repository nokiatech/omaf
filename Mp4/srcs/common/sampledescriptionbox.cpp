
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
#include "sampledescriptionbox.hpp"

#include "avcsampleentry.hpp"
#include "hevcsampleentry.hpp"
#include "initialviewingorientationsampleentry.hpp"
#include "log.hpp"
#include "mp4audiosampleentrybox.hpp"
#include "mp4visualsampleentrybox.hpp"
#include "restrictedschemeinfobox.hpp"
#include "urimetasampleentrybox.hpp"


SampleDescriptionBox::SampleDescriptionBox()
    : FullBox("stsd", 0, 0)
{
}

void SampleDescriptionBox::addSampleEntry(UniquePtr<SampleEntryBox> sampleEntry)
{
    mIndex.push_back(std::move(sampleEntry));
}

void SampleDescriptionBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    bitstr.write32Bits(static_cast<unsigned int>(mIndex.size()));

    for (auto& entry : mIndex)
    {
        if (!entry)
        {
            throw RuntimeError(
                "SampleDescriptionBox::writeBox can not write file because an unknown sample entry type was present "
                "when the file was read.");
        }
        BitStream entryBitStr;
        entry->writeBox(entryBitStr);
        bitstr.writeBitStream(entryBitStr);
    }
    updateSize(bitstr);
}

void SampleDescriptionBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);

    const unsigned int entryCount = bitstr.read32Bits();

    UniquePtr<RestrictedSchemeInfoBox> entrysResvBox;

    for (unsigned int i = 0; i < entryCount; ++i)
    {
        // Extract contained box bitstream and type
        FourCCInt boxType;
        uint64_t entryStart      = bitstr.getPos();
        BitStream entryBitStream = bitstr.readSubBoxBitStream(boxType);

        // Add new sample entry types based on handler when necessary
        if (boxType == "hvc1" || boxType == "hev1" || boxType == "hvc2")
        {
            UniquePtr<HevcSampleEntry, SampleEntryBox> hevcSampleEntry(CUSTOM_NEW(HevcSampleEntry, ()));
            hevcSampleEntry->parseBox(entryBitStream);

            mIndex.push_back(std::move(hevcSampleEntry));
        }
        else if (boxType == "avc1" || boxType == "avc3")
        {
            UniquePtr<AvcSampleEntry, SampleEntryBox> avcSampleEntry(CUSTOM_NEW(AvcSampleEntry, ()));
            avcSampleEntry->parseBox(entryBitStream);
            mIndex.push_back(std::move(avcSampleEntry));
        }
        else if (boxType == "mp4a")
        {
            UniquePtr<MP4AudioSampleEntryBox, SampleEntryBox> mp4AudioSampleEntry(
                CUSTOM_NEW(MP4AudioSampleEntryBox, ()));
            mp4AudioSampleEntry->parseBox(entryBitStream);
            mIndex.push_back(std::move(mp4AudioSampleEntry));
        }
        else if (boxType == "urim")
        {
            UniquePtr<UriMetaSampleEntryBox, SampleEntryBox> uriMetaSampleEntry(CUSTOM_NEW(UriMetaSampleEntryBox, ()));
            uriMetaSampleEntry->parseBox(entryBitStream);
            mIndex.push_back(std::move(uriMetaSampleEntry));
        }
        else if (boxType == "invo")
        {
            UniquePtr<InitialViewingOrientation, SampleEntryBox> invoSampleEntry(
                CUSTOM_NEW(InitialViewingOrientation, ()));
            invoSampleEntry->parseBox(entryBitStream);
            mIndex.push_back(std::move(invoSampleEntry));
        }
        else if (boxType == "mp4v")
        {
            UniquePtr<MP4VisualSampleEntryBox, SampleEntryBox> mp4VisualSampleEntry(
                CUSTOM_NEW(MP4VisualSampleEntryBox, ()));
            mp4VisualSampleEntry->parseBox(entryBitStream);
            mIndex.push_back(std::move(mp4VisualSampleEntry));
        }
        else if (boxType == "resv")
        {
            // resv is a special case. It is normal video sample entry, with an additional rinf box, which describes how
            // sample entry is encoded etc. First we read info from rinf box, rewrite correct sample entry fromat to
            // to the stream replacing "resv" with original format e.g. "avc1". After rewrite stream is rewinded to
            // entry start position and additional rinf box is stored along next read sample entry.

            // seek restricted video until rinf box to find out original format etc.
            MP4VisualSampleEntryBox visualSampleEntryHeaderParser;
            visualSampleEntryHeaderParser.VisualSampleEntryBox::parseBox(entryBitStream);
            BitStream rinfBoxSubBitstream;
            while (entryBitStream.numBytesLeft() > 0)
            {
                FourCCInt resvBoxType;
                rinfBoxSubBitstream = entryBitStream.readSubBoxBitStream(resvBoxType);
                if (resvBoxType == "rinf")
                {
                    break;
                }
            }

            if (rinfBoxSubBitstream.getSize() == 0)
            {
                throw RuntimeError("There must be rinf box inside resv");
            }

            entrysResvBox = makeCustomUnique<RestrictedSchemeInfoBox, RestrictedSchemeInfoBox>();
            entrysResvBox->parseBox(rinfBoxSubBitstream);

            // rewind & rewrite the sample entry box type
            if (entrysResvBox->getOriginalFormat() == "resv")
            {
                throw RuntimeError("OriginalFormat cannot be resv");
            }

            std::uint32_t originalFormat = entrysResvBox->getOriginalFormat().getUInt32();
            bitstr.setPosition(entryStart);
            bitstr.setByte(entryStart + 4, (uint8_t)(originalFormat >> 24));
            bitstr.setByte(entryStart + 5, (uint8_t)(originalFormat >> 16));
            bitstr.setByte(entryStart + 6, (uint8_t)(originalFormat >> 8));
            bitstr.setByte(entryStart + 7, (uint8_t) originalFormat);

            i--;
            continue;
        }
        else
        {
            logWarning() << "Skipping unknown SampleDescriptionBox entry of type '" << boxType << "'" << std::endl;
            // Push nullptr to keep indexing correct, in case it will still be possible to operate with the file.
            mIndex.push_back(nullptr);
        }

        // added entry was transformed from revs box, add resv info to the entry
        if (entrysResvBox)
        {
            if (mIndex[i])
            {
                mIndex[i]->addRestrictedSchemeInfoBox(std::move(entrysResvBox));
            }
            else
            {
                entrysResvBox.release();
            }
        }
    }
}
