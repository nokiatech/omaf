
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
#ifndef MOVIEFRAGMENTBOX_HPP
#define MOVIEFRAGMENTBOX_HPP

#include <memory>
#include "bbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "metabox.hpp"
#include "moviefragmentheaderbox.hpp"
#include "trackextendsbox.hpp"
#include "trackfragmentbox.hpp"

#include "api/isobmff/optional.h"
/**
 * @brief  Movie Fragment Box class
 * @details 'moof' box implementation as specified in the ISOBMFF specification.
 */
class MovieFragmentBox : public Box
{
public:
    MovieFragmentBox(Vector<MOVIEFRAGMENTS::SampleDefaults>& sampleDefaults);
    virtual ~MovieFragmentBox() = default;

    /** @return Reference to the contained MovieFragmentHeaderBox. */
    MovieFragmentHeaderBox& getMovieFragmentHeaderBox();
    const MovieFragmentHeaderBox& getMovieFragmentHeaderBox() const;

    /**
     * Add a TrackFragmentBox to MovieFragmentBox
     * @param trackFragmentBox TrackFragmentBox to add. */
    void addTrackFragmentBox(UniquePtr<TrackFragmentBox> trackFragmentBox);

    /** @return Pointers to all contained TrackFragmentBoxes. */
    Vector<TrackFragmentBox*> getTrackFragmentBoxes();

    /** @brief Sets movie fragment first byte offset inside the segment data */
    void setMoofFirstByteOffset(std::uint64_t moofFirstByteOffset);

    /** @return std::uint64_t Gets movie fragment first byte offset inside the segment data */
    std::uint64_t getMoofFirstByteOffset();

    /** @return MetaBox Return meta box, if any */
    const ISOBMFF::Optional<MetaBox>& getMetaBox() const;

    /** @return Return the already existing metabox if any, or add one and return that */
    MetaBox& addMetaBox();

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
    MovieFragmentHeaderBox mMovieFragmentHeaderBox;
    Vector<UniquePtr<TrackFragmentBox>> mTrackFragmentBoxes;  ///< Contained TrackFragmentBoxes
    Vector<MOVIEFRAGMENTS::SampleDefaults>& mSampleDefaults;
    std::uint64_t mFirstByteOffset;  // Offset of 1st byte of this moof inside its segment/file
    ISOBMFF::Optional<MetaBox> mMetaBox;
};

#endif /* end of include guard: MOVIEFRAGMENTBOX_HPP */
