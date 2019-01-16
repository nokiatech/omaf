
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
#ifndef TRACKRUNBOX_HPP
#define TRACKRUNBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"
#include "moviefragmentsdatatypes.hpp"

/**
 * @brief  Track Run Box class
 * @details 'trun' box implementation as specified in the ISOBMFF specification.
 */
class TrackRunBox : public FullBox
{
public:
    TrackRunBox(uint8_t version = 0, std::uint32_t tr_flags = 0);
    virtual ~TrackRunBox() = default;

    struct SampleDetailsVersion0
    {
        uint32_t sampleDuration;
        uint32_t sampleSize;
        MOVIEFRAGMENTS::SampleFlags sampleFlags;
        uint32_t sampleCompositionTimeOffset;
    };

    struct SampleDetailsVersion1
    {
        uint32_t sampleDuration;
        uint32_t sampleSize;
        MOVIEFRAGMENTS::SampleFlags sampleFlags;
        int32_t sampleCompositionTimeOffset;
    };

    union SampleDetails {
        SampleDetailsVersion0 version0;
        SampleDetailsVersion1 version1;
    };

    enum TrackRunFlags
    {
        DataOffsetPresent                   = 0x000001,
        FirstSampleFlagsPresent             = 0x000004,
        SampleDurationPresent               = 0x000100,
        SampleSizePresent                   = 0x000200,
        SampleFlagsPresent                  = 0x000400,
        SampleCompositionTimeOffsetsPresent = 0x000800
    };

    /** @brief Set sampleCount of the TrackRunBox.
     *  @param [in] uint32_t sampleCount **/
    void setSampleCount(const uint32_t sampleCount);

    /** @brief Get sampleCount of the TrackRunBox.
     *  @return uint64_t as specified in 8.8.8.1  of ISO/IEC 14496-12:2015(E)**/
    uint32_t getSampleCount() const;

    /** @brief Set dataOffset of the TrackRunBox.
     *  @param [in] int32_t dataOffset **/
    void setDataOffset(const int32_t dataOffset);

    /** @brief Get dataOffset of the TrackRunBox.
     *  @return int32_t as specified in 8.8.8.1  of ISO/IEC 14496-12:2015(E)**/
    int32_t getDataOffset() const;

    /** @brief Set first sample flags of the TrackRunBox.
     *  @param [in] MOVIEFRAGMENTS::SampleFlags firstSampleFlags **/
    void setFirstSampleFlags(const MOVIEFRAGMENTS::SampleFlags firstSampleFlags);

    /** @brief Get first sample flags of the TrackRunBox.
     *  @return int32_t as specified in 8.8.8.1  of ISO/IEC 14496-12:2015(E)**/
    MOVIEFRAGMENTS::SampleFlags getFirstSampleFlags() const;

    /** @brief Add sample details of the TrackRunBox.
     *  @param [in] MOVIEFRAGMENTS::SampleFlags firstSampleFlags **/
    void addSampleDetails(SampleDetails sampleDetails);

    /** @brief Get first sample flags of the TrackRunBox.
     *  @return int32_t as specified in 8.8.8.1  of ISO/IEC 14496-12:2015(E)**/
    const Vector<SampleDetails>& getSampleDetails() const;

    /** @brief Set SampleDefaults for parsing the box.
     *  @param [in] MOVIEFRAGMENTS::SampleDefaults sampleDefaults **/
    void setSampleDefaults(MOVIEFRAGMENTS::SampleDefaults& sampleDefaults);

    /**
     * @brief Serialize box data to the ISOBMFF::BitStream.
     * @see Box::writeBox()
     */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /**
     * @brief Deserialize box data from the ISOBMFF::BitStream.
     * @see Box::parseBox()
     */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    bool mSampleDefaultsSet;
    MOVIEFRAGMENTS::SampleDefaults mSampleDefaults;

    uint32_t mSampleCount;
    // the following are optional fields
    int32_t mDataOffset;
    MOVIEFRAGMENTS::SampleFlags mFirstSampleFlags;
    Vector<SampleDetails> mSampleDetails;
};

#endif /* end of include guard: TRACKRUNBOX_HPP */
