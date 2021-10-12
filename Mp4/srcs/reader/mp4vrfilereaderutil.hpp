
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
#ifndef MP4VRFILEREADERUTIL_HPP_
#define MP4VRFILEREADERUTIL_HPP_

#include <functional>
#include <iterator>
#include <memory>

#include "mp4vrfiledatatypesinternal.hpp"
#include "mp4vrfilereaderimpl.hpp"
#include "urimetasampleentrybox.hpp"

namespace MP4VR
{
    /* *********************************************************************** */
    /* ************************* Helper functions **************************** */
    /* *********************************************************************** */

    /**
     * Helper function to get item type from  the Item Information Box inside a MetaBox
     * @param metaBox Pointer to the MetaBox
     * @param itemId ID of the item
     * @return 4CC type name */
    FourCCInt getRawItemType(const MetaBox& metaBox, uint32_t itemId);

    /**
     * @brief Convert ChannelLayoutBox to MP4VRFileReaderInterface::chnlProperty
     * @param channelLayoutBox Pointer to input ChannelLayoutBox
     * @return A chnlProperty filled with data from ChannelLayoutBox */
    chnlPropertyInternal makeChnl(const ChannelLayoutBox& channelLayoutBox);

    /**
     * @brief Convert SpatialAudioBox to MP4VRFileReaderInterface::SA3DProperty
     * @param spatialAudioBox Pointer to input SpatialAudioBox
     * @return A SA3DPropertyInternal filled with data from SpatialAudioBox */
    SA3DPropertyInternal makeSA3D(const SpatialAudioBox& spatialAudioBox);

    /**
     * @brief Convert Stereoscopic3D to MP4VRFileReaderInterface::StereoScopic3DProperty
     * @param stereoscopic3DBox Pointer to input Stereoscopic3D box
     * @return A StereoScopic3DProperty filled with data from Stereoscopic3D */
    StereoScopic3DProperty makest3d(const Stereoscopic3D* stereoscopic3DBox);

    /**
     * @brief Convert SphericalVideoV1Box to MP4VRFileReaderInterface::StereoScopic3DProperty
     * @param stereoscopic3DBox Reference to input SphericalVideoV1Box box
     * @return A StereoScopic3DProperty filled with data from SphericalVideoV1Box */
    StereoScopic3DProperty makest3d(const SphericalVideoV1Box& stereoscopic3DBox);

    /**
     * @brief Convert SphericalVideoV1Box to MP4VRFileReaderInterface::SphericalVideoV1Property
     * @param sphericalVideoBox Reference to input SphericalVideoV1Box
     * @return A SphericalVideoV1Property filled with data from SphericalVideoV1Box */
    SphericalVideoV1Property makeSphericalVideoV1Property(const SphericalVideoV1Box& sphericalVideoBox);

    /**
     * @brief Convert SphericalVideoV2Box to MP4VRFileReaderInterface::SphericalVideoV2Property
     * @param sphericalVideoBox Pointer to input SphericalVideoV2Box
     * @return A SphericalVideoV2Property filled with data from SphericalVideoV2Box */
    SphericalVideoV2Property makesv3d(const SphericalVideoV2Box* sphericalVideoBox);

    /**
     * @brief Convert RegionWisePackingBox to MP4VRFileReaderInterface::RegionWisePackingPropertyInternal
     * @param rwpkBox Reference to input RegionWisePackingBox
     * @return A RegionWisePackingPropertyInternal filled with data from RegionWisePackingBox */
    RegionWisePackingPropertyInternal makerwpk(const RegionWisePackingBox& rwpkBox);

    /**
     * @brief Convert CoverageInformationBox to MP4VRFileReaderInterface::CoverageInformationPropertyInternal
     * @param coviBox Reference to input CoverageInformationBox
     * @return A CoverageInformationPropertyInternal filled with data from CoverageInformationBox */
    CoverageInformationPropertyInternal makecovi(const CoverageInformationBox& coviBox);

    /**
     * @brief Recognize image item types ("hvc1", "grid", "iovl", "iden")
     * @param type 4CC Item type from Item Info Entry
     * @return True if the 4CC is an image type */
    bool isImageItemType(FourCCInt type);

    /**
     * @brief Check if one or more references from an item exists.
     * @param metaBox       MetaBox containing the item and possible references
     * @param itemId        ID of the item to search references for
     * @param referenceType 4CC reference type
     * @return True if one or more referenceType references from the item exists. */
    bool doReferencesFromItemIdExist(const MetaBox& metaBox, uint32_t itemId, FourCCInt referenceType);

    /**
     * @brief Check if one or more references to an item exists.
     * @param metaBox       MetaBox containing the item and possible references
     * @param itemId        ID of the item to search references for
     * @param referenceType 4CC reference type
     * @return True if one or more referenceType references to the item exists. */
    bool doReferencesToItemIdExist(const MetaBox& metaBox, uint32_t itemId, FourCCInt referenceType);

    /** Move decoder configuration parameter data to ParamSetMap
     * @param record AVC decoder configuration
     * @return Decoder parameters */
    ParameterSetMap makeDecoderParameterSetMap(const AvcDecoderConfigurationRecord& record);

    /** Move decoder configuration parameter data to ParamSetMap
     * @param record HEVC decoder configuration
     * @return Decoder parameters */
    ParameterSetMap makeDecoderParameterSetMap(const HevcDecoderConfigurationRecord& record);

    /** Move DecoderSpecificInfo parameter data to ParamSetMap from ElementaryStreamDescriptorBox
     * @param ElementaryStreamDescriptorBox
     * @return DecoderSpecificInfo parameters */
    ParameterSetMap makeDecoderParameterSetMap(const ElementaryStreamDescriptorBox& record);

    /**
     * @brief Find if there is certain type reference or references from tracks to a track.
     * @param trackPropertiesMap TrackPropertiesMap containing information about all tracks of the file
     * @param trackId            ID of the TrackBox to look references to
     * @param referenceType      4CC reference type to look for
     * @return True if a reference was found */
    bool isAnyLinkedToWithType(const TrackPropertiesMap& trackPropertiesMap,
                               ContextId trackId,
                               FourCCInt referenceType);


    /**
     * @brief Maps containers to others with a mapping function
     */
    template <typename InType,
              template <typename U> class InContainer,
              template <typename V> class OutContainer = InContainer,
              typename OutType                         = InType>
    OutContainer<OutType> map(const InContainer<InType>& input, std::function<OutType(const InType&)> func)
    {
        OutContainer<OutType> output;
        std::transform(input.begin(), input.end(), std::back_inserter(output), func);
        return output;
    }
}  // namespace MP4VR

#endif  // MP4VRFILEREADERUTIL_HPP_
