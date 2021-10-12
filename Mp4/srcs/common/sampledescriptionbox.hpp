
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
#ifndef SAMPLEDESCRIPTIONBOX_HPP
#define SAMPLEDESCRIPTIONBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"
#include "sampleentrybox.hpp"


/** @brief SampleDescriptionBox class. Extends from FullBox.
 *  @details This class makes use of templates in order to support different sample entry types. **/
class SampleDescriptionBox : public FullBox
{
public:
    SampleDescriptionBox();
    ~SampleDescriptionBox() = default;

    /** @brief Add a new sample entry.
     *  @param [in] sampleEntry Sample Entry of type SampleEntryBox **/
    void addSampleEntry(UniquePtr<SampleEntryBox> sampleEntry);

    /** @brief Get the list of sample entries.
     *  @returns Vector of sample entries of defined type **/
    template <typename T>
    Vector<T*> getSampleEntries() const;

    /** @brief Get the sample entry at a particular index from the list.
     *         The type of the sample entry must match, or undefined
     *         behavior occurs.
     *  @param [in] index 1-based index of the sample entry
     *  @returns Sample Entry of defined type **/
    template <typename T>
    T* getSampleEntry(unsigned int index) const;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a SampleDescriptionBox bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    void parseBox(ISOBMFF::BitStream& bitstr);

private:
    Vector<UniquePtr<SampleEntryBox>> mIndex;  ///< Vector of sample entries
};

template <typename T>
Vector<T*> SampleDescriptionBox::getSampleEntries() const
{
    Vector<T*> result;
    for (auto& entry : mIndex)
    {
        T* ptr = dynamic_cast<T*>(entry.get());
        if (ptr)
        {
            result.push_back(ptr);
        }
    }
    return result;
}

template <typename T>
T* SampleDescriptionBox::getSampleEntry(const unsigned int index) const
{
    if (mIndex.size() < index || index == 0)
    {
        throw RuntimeError("SampleDescriptionBox::getSampleEntry invalid sample entry index.");
    }

    T* entryPtr = dynamic_cast<T*>(mIndex.at(index - 1).get());
    return entryPtr;
}

#endif /* end of include guard: SAMPLEDESCRIPTIONBOX_HPP */
