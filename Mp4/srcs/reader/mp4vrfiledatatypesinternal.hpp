
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
#ifndef MP4VRFILEDATATYPESINTERNAL_HPP_
#define MP4VRFILEDATATYPESINTERNAL_HPP_

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "api/reader/mp4vrfiledatatypes.h"
#include "customallocator.hpp"

#include "bbox.hpp"
#include "compatibleschemetypebox.hpp"
#include "decodepts.hpp"
#include "filetypebox.hpp"
#include "metabox.hpp"
#include "mp4vrfilestreamfile.hpp"
#include "mp4vrfilestreaminternal.hpp"
#include "schemetypebox.hpp"
#include "segmenttypebox.hpp"
#include "smallvector.hpp"
#include "trackextendsbox.hpp"
#include "tracktypebox.hpp"
#include "writeoncemap.hpp"

namespace MP4VR
{
    template <typename T, typename Tag>
    class IdBase
    {
    public:
        IdBase()
            : mId()
        {
            // nothing
        }
        // implicit in, but explicit out
        IdBase(T id)
            : mId(id)
        {
            // nothing
        }

        T get() const
        {
            return mId;
        }

    private:
        T mId;
    };

    template <typename T, typename Tag>
    class IdBaseExplicit : public IdBase<T, Tag>
    {
    public:
        IdBaseExplicit()
            : IdBase<T, Tag>()
        {
            // nothing
        }
        explicit IdBaseExplicit(T id)
            : IdBase<T, Tag>(id)
        {
            // nothing
        }
    };

    // IdBaseWithAdditions is like IdBase but provides + - += -= as well
    template <typename T, typename Tag>
    class IdBaseWithAdditions : public IdBase<T, Tag>
    {
    public:
        IdBaseWithAdditions()
            : IdBase<T, Tag>()
        {
            // nothing
        }
        // implicit in, but explicit out
        IdBaseWithAdditions(T id)
            : IdBase<T, Tag>(id)
        {
            // nothing
        }
    };

    template <typename T, typename Tag>
    bool operator<(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() < b.get();
    }

    template <typename T, typename Tag>
    bool operator==(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() == b.get();
    }

    template <typename T, typename Tag>
    bool operator!=(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() != b.get();
    }

    template <typename T, typename Tag>
    bool operator>(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() > b.get();
    }

    template <typename T, typename Tag>
    bool operator<=(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() <= b.get();
    }

    template <typename T, typename Tag>
    bool operator>=(IdBase<T, Tag> a, IdBase<T, Tag> b)
    {
        return a.get() >= b.get();
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag> operator+(IdBaseWithAdditions<T, Tag> a, IdBaseWithAdditions<T, Tag> b)
    {
        return a.get() + b.get();
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag> operator-(IdBaseWithAdditions<T, Tag> a, IdBaseWithAdditions<T, Tag> b)
    {
        return a.get() - b.get();
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag>& operator+=(IdBaseWithAdditions<T, Tag>& a, IdBaseWithAdditions<T, Tag> b)
    {
        a = a.get() + b.get();
        return a;
    }

    template <typename T, typename Tag>
    IdBaseWithAdditions<T, Tag>& operator-=(IdBaseWithAdditions<T, Tag>& a, IdBaseWithAdditions<T, Tag> b)
    {
        a = a.get() - b.get();
        return a;
    }

    template <typename T, typename Tag>
    std::ostream& operator<<(std::ostream& stream, IdBase<T, Tag> value)
    {
        stream << value.get();
        return stream;
    }

    struct chnlPropertyInternal
    {
        std::uint8_t streamStructure;
        std::uint8_t definedLayout;
        std::uint64_t omittedChannelsMap;
        std::uint8_t objectCount;
        std::uint16_t channelCount;
        Vector<ChannelLayout> channelLayouts;  // as many as channelCount
    };

    struct RegionWisePackingPropertyInternal
    {
        bool constituentPictureMatchingFlag;
        uint32_t projPictureWidth;
        uint32_t projPictureHeight;
        uint16_t packedPictureWidth;
        uint16_t packedPictureHeight;
        Vector<RegionWisePackingRegion> regions;
    };

    struct CoverageInformationPropertyInternal
    {
        CoverageShapeType coverageShapeType;
        bool viewIdcPresenceFlag;
        ViewIdc defaultViewIdc;
        Vector<CoverageSphereRegion> sphereRegions;
    };

    struct SA3DPropertyInternal
    {
        uint8_t version;          // specifies the version of this box
        uint8_t ambisonicType;    // specifies the type of ambisonic audio represented
        uint32_t ambisonicOrder;  // specifies the order of the ambisonic sound field
        uint8_t
            ambisonicChannelOrdering;  // specifies the channel ordering (i.e., spherical harmonics component ordering)
        uint8_t ambisonicNormalization;  // specifies the normalization (i.e., spherical harmonics normalization)
        Vector<uint32_t> channelMap;  // maps audio channels in a given audio track to ambisonic component. Array size
                                      // specifies the number of audio channels.
    };

    struct SchemeTypesPropertyInternal
    {
        SchemeTypeBox mainScheme;
        Vector<CompatibleSchemeTypeBox> compatibleSchemes;
    };

    struct SampleSizeInPixels
    {
        uint32_t width;
        uint32_t height;
    };

    // Convenience types
    class SegmentIdTag
    {
    };
    class InitSegmentIdTag
    {
    };
    class ContextIdTag
    {
    };
    class ItemIdTag
    {
    };
    class SampleDescriptionIndexTag
    {
    };
    class SequenceTag
    {
    };

    typedef IdBase<std::uint32_t, SegmentIdTag> SegmentId;
    typedef IdBase<std::uint32_t, InitSegmentIdTag> InitSegmentId;
    typedef IdBaseExplicit<std::uint32_t, ContextIdTag>
        ContextId;  // also known as track id (after removing the api client mapping)
    typedef IdBaseExplicit<std::uint32_t, ContextIdTag> TrackGroupId;  // in the same number space as tracks
    typedef IdBase<std::uint32_t, SampleDescriptionIndexTag> SampleDescriptionIndex;
    typedef IdBase<std::uint32_t, SequenceTag> Sequence;
    typedef std::uint64_t Timestamp;
    typedef IdBaseWithAdditions<std::uint32_t, ItemIdTag> ItemId;
    typedef Vector<std::pair<ItemId, Timestamp>> DecodingOrderVector;
    typedef Vector<std::uint32_t> IdVector;
    typedef Vector<ContextId> ContextIdVector;
    typedef Vector<std::uint8_t> DataVector;
    typedef Vector<Timestamp> TimestampVector;
    typedef Map<Timestamp, ItemId> TimestampMap;
    typedef Map<FourCCInt, IdVector> TypeToIdsMap;
    typedef Map<FourCCInt, ContextIdVector> TypeToContextIdsMap;
    typedef Map<FourCCInt, Vector<IdVector>> GroupingMap;
    typedef Map<DecSpecInfoType, DataVector> ParameterSetMap;
    typedef DynArray<SegmentInformation> SegmentIndex;
    typedef IdBase<std::uint32_t, struct ImdaIdTag> ImdaId;

    typedef std::pair<InitSegmentId, ContextId> InitSegmentTrackId;
    typedef std::pair<SegmentId, ContextId> SegmentTrackId;

    class FileFeature
    {
    public:
        FileFeature()
            : mTrackId(0){};
        typedef Set<FileFeatureEnum::Feature> FileFeatureSet;

        bool hasFeature(FileFeatureEnum::Feature feature) const
        {
            return mFileFeatureSet.count(feature) != 0;
        }
        void setFeature(FileFeatureEnum::Feature feature)
        {
            mFileFeatureSet.insert(feature);
        }
        std::uint32_t getTrackId() const
        {
            return mTrackId;
        }
        void setTrackId(std::uint32_t id)
        {
            mTrackId = id;
        }
        unsigned int getFeatureMask() const
        {
            unsigned int mask = 0;
            for (auto set : mFileFeatureSet)
            {
                mask |= (unsigned int) set;
            }
            return mask;
        }

    private:
        std::uint32_t mTrackId;
        FileFeatureSet mFileFeatureSet;
    };

    class TrackFeature
    {
    public:
        typedef Set<TrackFeatureEnum::Feature> TrackFeatureSet;
        typedef Set<TrackFeatureEnum::VRFeature> TrackVRFeatureSet;

        bool hasFeature(TrackFeatureEnum::Feature feature) const
        {
            return mTrackFeatureSet.count(feature) != 0;
        }
        void setFeature(TrackFeatureEnum::Feature feature)
        {
            mTrackFeatureSet.insert(feature);
        }
        bool hasVRFeature(TrackFeatureEnum::VRFeature vrFeature) const
        {
            return mTrackVRFeatureSet.count(vrFeature) != 0;
        }
        void setVRFeature(TrackFeatureEnum::VRFeature vrFeature)
        {
            mTrackVRFeatureSet.insert(vrFeature);
        }
        unsigned int getFeatureMask() const
        {
            unsigned int mask = 0;
            for (auto set : mTrackFeatureSet)
            {
                mask |= (unsigned int) set;
            }
            return mask;
        }
        unsigned int getVRFeatureMask() const
        {
            unsigned int mask = 0;
            for (auto set : mTrackVRFeatureSet)
            {
                mask |= (unsigned int) set;
            }
            return mask;
        }

    private:
        TrackFeatureSet mTrackFeatureSet;
        TrackVRFeatureSet mTrackVRFeatureSet;
    };

    /** @brief MOOV level features flag enumeration. There may be a MOOv level Metabox */
    class MoovFeature
    {
    public:
        /// Enumerated list of Moov Features
        enum Feature
        {
            HasMoovLevelMetaBox,
            HasCoverImage
        };
        typedef Set<Feature> MoovFeatureSet;

        bool hasFeature(Feature feature) const
        {
            return mMoovFeatureSet.count(feature) != 0;
        }
        void setFeature(Feature feature)
        {
            mMoovFeatureSet.insert(feature);
        }

    private:
        MoovFeatureSet mMoovFeatureSet;
    };

    typedef std::pair<InitSegmentTrackId, ItemId> Id;          ///< Convenience type combining context and item IDs
    typedef std::pair<ItemId, Timestamp> ItemIdTimestampPair;  ///< Pair of Item/sample ID and timestamp

    /** @brief Moov Property definition that may contain a MetaBox */
    struct MoovProperties
    {
        MoovFeature moovFeature;
        // movie fragment support related properties, used internally only:
        uint64_t fragmentDuration;
        Vector<MOVIEFRAGMENTS::SampleDefaults> fragmentSampleDefaults;

        // MetaBoxProperties metaBoxProperties;
    };

    // Tells directly about the track group box contents
    struct TrackGroupInfo {
        IdVector ids; // Track group ids
    };
    using TrackGroupInfoMap = Map<FourCCInt, TrackGroupInfo>;

    /** @brief Track Property definition which contain sample properties.
     *
     * In the samplePropertiesMap, samples of the track are listed in the same order they appear
     * in the sample size or sample to chunk boxes.
     * Each sample is given an ID, which is used as the key of the map.
     */
    struct TrackProperties
    {
        std::uint32_t alternateGroupId;
        String trackURI;  /// < only used for timed metadata tracks
        TrackFeature trackFeature;
        IdVector alternateTrackIds;             ///< other tracks IDs with the same alternate_group id.
        TypeToContextIdsMap referenceTrackIds;  ///< <reference_type, reference track ID> (coming from 'tref')
        TrackGroupInfoMap trackGroupInfoMap;  ///< <group_type, track group info> ... coming from Track Group Box 'trgr'
        std::shared_ptr<const EditBox> editBox;  ///< If set, an edit list box exists
        Vector<std::shared_ptr<const DataEntryBox>> dataEntries; ///< If set, you can fish imda data from here
    };
    typedef WriteOnceMap<ContextId, TrackProperties> TrackPropertiesMap;  ///< <track_id/context_id, TrackProperties>

    /// Reader internal information about each sample.
    struct SampleInfo
    {
        SegmentId segmentId;
        std::uint32_t sampleId;  ///< based on the sample's entry order in the sample table; this is in decoding order
        SmallVector<std::uint64_t, 1>
            compositionTimes;  ///< Timestamps of the sample. Possible edit list is considered here.
        SmallVector<std::uint64_t, 1> compositionTimesTS;  ///< Timestamps of the sample in time scale units. Possible
                                                           ///< edit list is considered here.
        ISOBMFF::Optional<std::uint64_t> dataOffset = 0;   ///< File offset of sample data in bytes, if data is available
        std::uint32_t dataLength = 0;                      ///< Length of same in bytes
        std::uint32_t width      = 0;                      ///< Width of the frame
        std::uint32_t height     = 0;                      ///< Height of the frame
                                   //    IdVector decodeDependencies;                 ///< Direct decoding dependencies
        std::uint32_t sampleDuration = 0;  ///< Duration of sample in time scale units.

        FourCCInt sampleEntryType;  ///< coming from SampleDescriptionBox (codingname)
        SampleDescriptionIndex
            sampleDescriptionIndex;  ///< coming from SampleDescriptionBox index (sample_description_index)
        SampleType sampleType;       ///< coming from sample groupings
        SampleFlags sampleFlags;     ///< Sample Flags Field as defined in 8.8.3.1 of ISO/IEC 14496-12:2015(E)
    };
    typedef Vector<SampleInfo> SampleInfoVector;

    template <typename Property> using PropertyMap = WriteOnceMap<SampleDescriptionIndex, Property>;

    struct InitTrackInfo
    {
        std::uint32_t timeScale;  ///< timescale of the track.
        std::uint32_t width;      ///< display width in pixels, from 16.16 fixed point in TrackHeaderBox
        std::uint32_t height;     ///< display height in pixels, from 16.16 fixed point in TrackHeaderBox

        FourCCInt sampleEntryType;  /// sample type from this track. Passed to segments sampleproperties.

        WriteOnceMap<SampleDescriptionIndex, ParameterSetMap> parameterSetMaps;  ///< Extracted decoder parameter sets
        PropertyMap<chnlPropertyInternal>
            chnlProperties;  ///< Clean chnl data from sample description entries
        PropertyMap<SA3DPropertyInternal>
            sa3dProperties;  ///< Clean sa3d data from sample description entriesa
        PropertyMap<StereoScopic3DProperty>
            st3dProperties;  ///< Clean st3d data from sample description entries
        PropertyMap<SphericalVideoV1Property>
            googleSphericalV1Properties;  ///< Clean Google Global UUID data from sample description entries
        PropertyMap<SphericalVideoV2Property>
            sv3dProperties;  ///< Clean sv3d data from sample description entries
        PropertyMap<SampleSizeInPixels>
            sampleSizeInPixels;  ///< Clean sample size information from sample description entries
        PropertyMap<ProjectionFormatProperty>
            pfrmProperties;  ///< Projection format from povd sample description entries
        PropertyMap<OverlayConfigProperty>
            ovlyProperties;  ///< Overlay config from povd sample description entries
        PropertyMap<DynamicViewpointConfigProperty>
            dyvpProperties;  ///< Dynamic viewpoints config from dyvp sample description entries
        PropertyMap<InitialViewpointConfigProperty>
            invpProperties;  ///< Initial viewpoints config from invp sample description entries
        PropertyMap<RegionWisePackingPropertyInternal>
            rwpkProperties;  ///< Region-Wise mapping information from povd sample description entries
        PropertyMap<RotationProperty>
            rotnProperties;  ///< Rotation information from povd sample description entriess
        PropertyMap<PodvStereoVideoConfiguration>
            stviProperties;  ///< Stereo Video arrangement information from povd sample description entriess
        PropertyMap<CoverageInformationPropertyInternal>
            coviProperties;  ///< Coverage information from povd sample description entriess
        PropertyMap<RecommendedViewportProperty>
            rcvpProperties;  ///< Information from rcvp sample description entriess
        PropertyMap<SchemeTypesPropertyInternal> schemeTypesProperties;  ///< Scheme type and
                                                                         ///< compatible scheme
                                                                         ///< information from
                                                                         ///< resv sample
                                                                         ///< description
                                                                         ///< entriess
        PropertyMap<uint8_t> nalLengthSizeMinus1;
    };

    struct TrackInfo
    {
        ItemId itemIdBase;
        SampleInfoVector samples;  ///< Information about each sample in the TrackBox

        DecodePts::PresentationTimeTS durationTS    = 0;  ///< Track duration in time scale units, from TrackHeaderBox
        DecodePts::PresentationTimeTS earliestPTSTS = 0;  // time of the first sample in time scale units
        DecodePts::PresentationTimeTS noSidxFallbackPTSTS = 0;  // start PTS for next segment if no sidx

        /** time to use as the base when adding the next track run; this value is initialized with
        some derived value upon first use and the incremented by trackrun duration when one is read. */
        DecodePts::PresentationTimeTS nextPTSTS = 0;

        WriteOnceMap<ItemId, FourCCInt> decoderCodeTypeMap;  ///< Extracted decoder code types

        DecodePts::PMap pMap;      ///< Display timestamps, from edit list
        DecodePts::PMapTS pMapTS;  ///< Display timestamps in time scale units, from edit list

        bool hasEditList = false;  ///< Used to determine if updateCompositionTimes should edit the time of last sample
                                   ///< to match track duration

        bool hasTtyp = false;
        TrackTypeBox ttyp;  ///< TrackType info in case if available
    };

    struct SegmentIO
    {
        UniquePtr<InternalStream> stream;
        UniquePtr<StreamInterface> fileStream;  // only used when reading from files
        std::int64_t size = 0;
    };

    typedef WriteOnceMap<Id, SampleDescriptionIndex> ItemToParameterSetMap;

    struct ImdaInfo
    {
        StreamInterface::offset_t begin;
        StreamInterface::offset_t end;
    };

    struct SegmentProperties
    {
        InitSegmentId initSegmentId;  ///< Initialization segment track id for this segment
        SegmentId segmentId;
        Set<Sequence> sequences;  //< Generated sequence number for this segment instead of sequence numbers from the
                                  // movie fragment header box.

        SegmentIO io;

        SegmentTypeBox styp;  ///< Segment Type Box for later information retrieval

        Map<ContextId, TrackInfo> trackInfos;

        ItemToParameterSetMap
            itemToParameterSetMap;  ///< Map from every sample or image item to parameter set map entry

        Map<ImdaId, ImdaInfo> imda; ///< Imda segments contained in this segment
    };

    typedef Map<SegmentId, SegmentProperties> SegmentPropertiesMap;
    typedef Map<Sequence, SegmentId> SequenceToSegmentMap;

    /** Maps FourCCs to maps that map track ids opr track group ids to tracks */
    struct TrackReferenceInfo {
        Set<ContextId> trackIds;
        Set<ContextId> extractorTrackIds; // "hvc2" tracks that point to this track or track group
    };
    using TrackReferenceInfoMap = Map<ContextId, TrackReferenceInfo>;
    using FourCCTrackReferenceInfoMap = Map<FourCCInt, TrackReferenceInfoMap>;

    struct InitSegmentProperties
    {
        // In future? Vector<SegmentId> segments;

        FileFeature fileFeature;
        MoovProperties moovProperties;
        TrackPropertiesMap trackProperties;
        std::uint32_t movieTimeScale;

        FileTypeBox ftyp;  ///< File Type Box for later information retrieval
        ISOBMFF::Optional<MetaBox> meta;

        Map<ContextId, InitTrackInfo> initTrackInfos;
        FourCCTrackReferenceInfoMap trackReferences;

        SegmentPropertiesMap segmentPropertiesMap;
        SequenceToSegmentMap sequenceToSegment;

        SegmentIndex segmentIndex;
    };
}  // namespace MP4VR

#endif /* MP4VRFILEDATATYPESINTERNAL_HPP_ */
