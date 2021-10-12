
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
#ifndef MP4VRFILEREADERIMPL_HPP
#define MP4VRFILEREADERIMPL_HPP

#include <fstream>
#include <functional>
#include <istream>
#include <memory>

#include "api/reader/mp4vrfiledatatypes.h"
#include "api/reader/mp4vrfilereaderinterface.h"
#include "channellayoutbox.hpp"
#include "customallocator.hpp"
#include "decodepts.hpp"
#include "elementarystreamdescriptorbox.hpp"
#include "filetypebox.hpp"
#include "googlespatialaudiobox.hpp"
#include "googlesphericalvideov2box.hpp"
#include "googlestereoscopic3dbox.hpp"
#include "metabox.hpp"
#include "moviebox.hpp"
#include "moviefragmentbox.hpp"
#include "mp4vrfiledatatypesinternal.hpp"
#include "mp4vrfilesegment.hpp"
#include "mp4vrfilestreamgeneric.hpp"
#include "mp4vrfilestreaminternal.hpp"
#include "segmentindexbox.hpp"
#include "smallvector.hpp"
#include "writeoncemap.hpp"

class CleanAperture;
class AvcDecoderConfigurationRecord;
class HevcDecoderConfigurationRecord;

namespace MP4VR
{
    /** @brief Interface for reading MP4VR File from the filesystem. */
    class MP4VRFileReaderImpl : public MP4VRFileReaderInterface
    {
    public:
        MP4VRFileReaderImpl();
        virtual ~MP4VRFileReaderImpl() = default;

        /**
         * FileReaderException exception class.
         */
        class FileReaderException : public Exception
        {
        public:
            FileReaderException(MP4VRFileReaderInterface::ErrorCode status, const String& message = "")
                : mStatusCode(status)
                , mMessage(message)
            {
            }

            const char* what() const throw()
            {
                if (mMessage != "")
                {
                    return mMessage.c_str();
                }

                switch (mStatusCode)
                {
                case ErrorCode::FILE_OPEN_ERROR:
                    return "Unable to open input file.";
                case ErrorCode::FILE_HEADER_ERROR:
                    return "Error while reading file header.";
                case ErrorCode::FILE_READ_ERROR:
                    return "Error while reading file.";
                case ErrorCode::UNSUPPORTED_CODE_TYPE:
                    return "Unsupported code type.";
                case ErrorCode::INVALID_FUNCTION_PARAMETER:
                    return "Invalid API method parameter.";
                case ErrorCode::INVALID_CONTEXT_ID:
                    return "Invalid context ID parameter.";
                case ErrorCode::INVALID_ITEM_ID:
                    return "Invalid item ID parameter.";
                case ErrorCode::INVALID_PROPERTY_INDEX:
                    return "Invalid item property index.";
                case ErrorCode::INVALID_SAMPLE_DESCRIPTION_INDEX:
                    return "Invalid sample description index.";
                case ErrorCode::PROTECTED_ITEM:
                    return "The item is unaccessible because it is protected.";
                case ErrorCode::UNPROTECTED_ITEM:
                    return "The item is unaccessible because it is protected.";
                case ErrorCode::UNINITIALIZED:
                    return "Reader not initialized. Call initialize() first.";
                case ErrorCode::NOT_APPLICABLE:
                default:
                    return "Unspecified error.";
                }
            }

            ErrorCode getStatusCode() const
            {
                return mStatusCode;
            }

        private:
            MP4VRFileReaderInterface::ErrorCode mStatusCode;
            String mMessage;
        };

    public:  // From MP4VRFileReaderInterface
        /// @see MP4VRFileReaderInterface::initialize()
        int32_t initialize(const char* fileName) override;

        /// @see MP4VRFileReaderInterface::initialize()
        int32_t initialize(StreamInterface* stream) override;

        /// @see MP4VRFileReaderInterface::close()
        void close() override;

        /// @see MP4VRFileReaderInterface::getMajorBrand()
        int32_t getMajorBrand(FourCC& majorBrand,
                              uint32_t initializationSegmentId = 0,
                              uint32_t segmentId               = UINT32_MAX) const override;

        /// @see MP4VRFileReaderInterface::getMinorBrand()
        int32_t getMinorVersion(uint32_t& minorVersion,
                                uint32_t initializationSegmentId = 0,
                                uint32_t segmentId               = UINT32_MAX) const override;

        /// @see MP4VRFileReaderInterface::getCompatibleBrands()
        int32_t getCompatibleBrands(DynArray<FourCC>& compatibleBrands,
                                    uint32_t initializationSegmentId = 0,
                                    uint32_t segmentId               = UINT32_MAX) const override;

        /// @see MP4VRFileReaderInterface::getFileInformation()
        int32_t getFileInformation(FileInformation& fileinfo, uint32_t initializationSegmentId = 0) const override;

        int32_t getFileGroupsList(GroupsListProperty& groupsList, uint32_t initializationSegmentId = 0) const override;

        /// @see MP4VRFileReaderInterface::getTrackProperties()
        int32_t getTrackInformations(DynArray<TrackInformation>& trackInfos) const override;

        /// @see MP4VRFileReaderInterface::getDisplayWidth()
        int32_t getDisplayWidth(uint32_t trackId, uint32_t& displayWidth) const override;

        /// @see MP4VRFileReaderInterface::getDisplayHeight()
        int32_t getDisplayHeight(uint32_t trackId, uint32_t& displayHeight) const override;

        /// @see MP4VRFileReaderInterface::getDisplayWidth()
        int32_t getDisplayWidthFP(uint32_t trackId, uint32_t& displayWidth) const override;

        /// @see MP4VRFileReaderInterface::getDisplayHeight()
        int32_t getDisplayHeightFP(uint32_t trackId, uint32_t& displayHeight) const override;

        /// @see MP4VRFileReaderInterface::getWidth()
        int32_t getWidth(uint32_t trackId, uint32_t itemId, uint32_t& width) const override;

        /// @see MP4VRFileReaderInterface::getHeight()
        int32_t getHeight(uint32_t trackId, uint32_t itemId, uint32_t& height) const override;

        /// @see MP4VRFileReaderInterface::getPlaybackDurationInSecs()
        int32_t getPlaybackDurationInSecs(uint32_t trackId, double& durationInSecs) const override;

        /// @see MP4VRFileReaderInterface::getTrackItemListByType()
        int32_t getTrackSampleListByType(uint32_t trackId,
                                         TrackSampleType itemType,
                                         DynArray<uint32_t>& itemIds) const override;

        /// @see MP4VRFileReaderInterface::getItemType()
        int32_t getTrackSampleType(uint32_t trackId, uint32_t itemId, FourCC& trackItemType) const override;

        /// @see MP4VRFileReaderInterface::getItemData()
        int32_t getTrackSampleData(uint32_t trackId,
                                   uint32_t itemId,
                                   char* memoryBuffer,
                                   uint32_t& memoryBufferSize,
                                   bool videoByteStreamHeaders = true) override;

        /// @see MP4VRFileReaderInterface::getItemData()
        int32_t getTrackSampleOffset(uint32_t trackId,
                                     uint32_t itemIdApi,
                                     uint64_t& sampleOffset,
                                     uint32_t& sampleLength) override;

        /// @see MP4VRFileReaderInterface::getDecoderConfiguration()
        int32_t getDecoderConfiguration(uint32_t trackId,
                                        uint32_t itemId,
                                        DynArray<DecoderSpecificInfo>& decoderInfos) const override;

        /// @see MP4VRFileReaderInterface::getItemTimestamps()
        int32_t getTrackTimestamps(uint32_t trackId, DynArray<TimestampIDPair>& timestamps) const override;

        /// @see MP4VRFileReaderInterface::getTimestampsOfItem()
        int32_t getTimestampsOfSample(uint32_t trackId, uint32_t itemId, DynArray<uint64_t>& timestamps) const override;

        /// @see MP4VRFileReaderInterface::getItemsInDecodingOrder()
        int32_t getSamplesInDecodingOrder(uint32_t trackId,
                                          DynArray<TimestampIDPair>& itemDecodingOrder) const override;

        /// @see MP4VRFileReaderInterface::getItemDecodeDependencies()
        int32_t getSyncSampleId(uint32_t trackId,
                                uint32_t sampleId,
                                MP4VR::SeekDirection direction,
                                uint32_t& syncSampleId) const override;

        /// @see MP4VRFileReaderInterface::getDecoderCodeType()
        int32_t getDecoderCodeType(uint32_t trackId, uint32_t itemId, FourCC& decoderCodeType) const override;

        /// see MP4VRFileReaderInterface::getSampleDuration()
        int32_t getSampleDuration(uint32_t trackId, uint32_t sampleId, uint32_t& sampleDuration) const override;

        /// @see MP4VRFileReaderInterface::getPropertyChnl()
        int32_t getPropertyChnl(uint32_t trackId, uint32_t sampleId, chnlProperty& aProp) const override;

        /// @see MP4VRFileReaderInterface::getPropertySpatialAudio()
        int32_t getPropertySpatialAudio(uint32_t trackId,
                                        uint32_t sampleId,
                                        SpatialAudioProperty& spatialaudioproperty) const override;

        /// @see MP4VRFileReaderInterface::getPropertyStereoScopic3DV2()
        int32_t getPropertyStereoScopic3D(uint32_t trackId,
                                          uint32_t sampleId,
                                          StereoScopic3DProperty& stereoscopicproperty) const override;

        /// @see MP4VRFileReaderInterface::getPropertySphericalVideoV1()
        int32_t getPropertySphericalVideoV1(uint32_t trackId,
                                            uint32_t sampleId,
                                            SphericalVideoV1Property& sphericalproperty) const override;

        /// @see MP4VRFileReaderInterface::getPropertySphericalVideoV2()
        int32_t getPropertySphericalVideoV2(uint32_t trackId,
                                            uint32_t sampleId,
                                            SphericalVideoV2Property& sphericalproperty) const override;

        /// @see MP4VRFileReaderInterface::getPropertyRegionWisePacking()
        int32_t getPropertyRegionWisePacking(uint32_t trackId,
                                             uint32_t sampleId,
                                             RegionWisePackingProperty& aProp) const override;

        /// @see MP4VRFileReaderInterface::getPropertyCoverageInformation()
        int32_t getPropertyCoverageInformation(uint32_t trackId,
                                               uint32_t sampleId,
                                               CoverageInformationProperty& aProp) const override;

        /// @see MP4VRFileReaderInterface::getPropertyProjectionFormat()
        int32_t getPropertyProjectionFormat(uint32_t trackId,
                                            uint32_t sampleId,
                                            ProjectionFormatProperty& aProp) const override;

        /// @see MP4VRFileReaderInterface::getPropertyOverlayConfig()
        int32_t getPropertyOverlayConfig(uint32_t trackId,
                                         uint32_t sampleId,
                                         OverlayConfigProperty& aProp) const override;

        /// @see MP4VRFileReaderInterface::getPropertyDynamicViewpointConfig()
        int32_t getPropertyDynamicViewpointConfig(
            uint32_t trackId,
            uint32_t sampleId,
            DynamicViewpointConfigProperty& dynamicViewpointConfigProperty) const override;

        /// @see MP4VRFileReaderInterface::getPropertyInitialViewpointConfig()
        int32_t getPropertyInitialViewpointConfig(
            uint32_t trackId,
            uint32_t sampleId,
            InitialViewpointConfigProperty& initialViewpointConfigProperty) const override;

        /// @see MP4VRFileReaderInterface::getPropertySchemeTypes()
        int32_t getPropertySchemeTypes(uint32_t trackId,
                                       uint32_t sampleId,
                                       SchemeTypesProperty& schemeTypesProperty) const override;

        /// @see MP4VRFileReaderInterface::getPropertyStereoVideoConfiguration()
        int32_t getPropertyStereoVideoConfiguration(uint32_t trackId,
                                                    uint32_t sampleId,
                                                    PodvStereoVideoConfiguration& stereoVideoProperty) const override;

        /// @see MP4VRFileReaderInterface::getPropertyRotation()
        int32_t getPropertyRotation(uint32_t trackId,
                                    uint32_t sampleId,
                                    RotationProperty& rotationProperty) const override;

        int32_t getPropertyRecommendedViewport(uint32_t trackId,
                                               uint32_t sampleId,
                                               RecommendedViewportProperty& property) const override;

    private:
        // getProperty returns a copy as for some reason a const reference wouldn't do the trick (crash)
        template <typename Property>
        int32_t getPropertyGeneric(uint32_t trackId,
                                   uint32_t sampleId,
                                   Property& property,
                                   std::function<PropertyMap<typename std::remove_reference<Property>::type>(const InitTrackInfo&)> getPropertyMap) const;

        uint32_t lookupTrackInfo(uint32_t trackId,
                                 uint32_t sampleId,
                                 InitTrackInfo& initTrackInfo,
                                 SampleDescriptionIndex& index) const;

        template <typename Property>
        Property fetchSampleProp(const PropertyMap<Property>& propertiesMap,
                                 const SampleDescriptionIndex& index,
                                 uint32_t& result) const;

    public:
        /// @see MP4VRFileReaderInterface::parseInitializationSegment()
        int32_t parseInitializationSegment(StreamInterface* streamInterface, uint32_t initSegmentId) override;

        /// @see MP4VRFileReaderInterface::parseInitializationSegment()
        int32_t invalidateInitializationSegment(uint32_t initSegmentId) override;

        /// @see MP4VRFileReaderInterface::parseSegment()
        int32_t parseSegment(StreamInterface* streamInterface,
                             uint32_t initSegmentId,
                             uint32_t segmentId,
                             uint64_t earliestPTSinTS = UINT64_MAX) override;

        /// @see MP4VRFileReaderInterface::invalidateSegment()
        int32_t invalidateSegment(uint32_t initSegmentId, uint32_t segmentId) override;

        /// @see MP4VRFileReaderInterface::getSegmentIndex()
        int32_t getSegmentIndex(uint32_t initSegmentId, DynArray<SegmentInformation>& segmentIndex) override;

        /// @see MP4VRFileReaderInterface::parseSegmentIndex()
        int32_t parseSegmentIndex(StreamInterface* streamInterface,
                                  DynArray<SegmentInformation>& segmentIndex) override;

    public:
        // For debugging
        void dumpSegmentInfo() const;

    private:
        Map<InitSegmentId, InitSegmentProperties> mInitSegmentPropertiesMap;
        UniquePtr<StreamInterface> mFileStream;
        /* ********************************************************************** */
        /* ****************** Common for meta and track content ***************** */
        /* ********************************************************************** */

        // Next Sequence number replacing  for surviving through non-linearities in
        // MovieFragmentHeader.FragmentSequenceNumber
        Sequence mNextSequence = {};

        enum class State
        {
            UNINITIALIZED,  ///< State before starting to read file and after closing it
            INITIALIZING,   ///< State during parsing the file
            READY           ///< State after the file has been parsed and information extracted
        };
        State mState;  ///< Running state of the reader API implementation

        /// Context type classification
        enum class ContextType
        {
            META,
            TRACK,
            FILE,
            NOT_SET
        };

        /// General information about each root meta and track context
        struct ContextInfo
        {
            ContextType contextType          = ContextType::NOT_SET;
            bool isCoverImageSet             = false;
            ItemId coverImageId              = 0;
            bool isForcedLoopPlaybackEnabled = false;
        };
        Map<InitSegmentTrackId, ContextInfo> mContextInfoMap;

        friend class Segments;
        friend class ConstSegments;

        Segments segmentsBySequence(InitSegmentId initSegmentId);
        ConstSegments segmentsBySequence(InitSegmentId initSegmentId) const;

        /** @throws FileReaderException StatusCode=[UNINITIALIZED] if input file has not been read yet */
        void isInitialized() const;

        /** @returns ErrorCOde=[UNINITIALIZED] if input file has not been read yet */
        int isInitializedError() const;

        /**
         * Identify context by context id.
         * @param [in] id Context ID
         * @throws FileReaderException, StatusCode=[UNINITIALIZED, INVALID_CONTEXT_ID]
         * @return Context type based on mContextInfoMap. */
        ContextType getContextType(InitSegmentTrackId id) const;

        /**
         * Identify context by context id.
         * @param [in] id Context ID
         * @param [out] contextType Context type
         * @return ErrorCode::INVALID_CONTEXT_ID or ErrorCode::NO_ERROR. */
        int getContextTypeError(const InitSegmentTrackId id, ContextType& contextType) const;

        /** Parse input stream, fill mFileProperties and implementation internal data structures. */
        int32_t readStream(InitSegmentId initSegmentId, SegmentId segmentId);

        /**
         * Iterate boxes till the end of an Io by calling the given handler function
         * @param [in] aIo IO to use for readaing
         * @param [in] aHandler Callback to use for each box. If the handler consumes aIo (ie. read or skip),
                                it must return the error code; otherwise it must not, and iterateBoxes will
                                handle the skipping of the box
        */
        int32_t iterateBoxes(SegmentIO& aIo,
                             std::function<ISOBMFF::Optional<int32_t>(String boxType, BitStream& bitstream)> aHandler);

        int32_t parseImda(SegmentProperties& aSegmentProperties);

        FileFeature getFileFeatures() const;

        int32_t readBoxParameters(SegmentIO& io, String& boxType, std::int64_t& boxSize);
        int32_t readBox(SegmentIO& io, BitStream& bitstream);
        int32_t skipBox(SegmentIO& io);

        /** Get dimensions for item
         * @param [in]  segTrackId Track or meta context id
         * @param [in]  itemId    Item ID from track or meta context
         * @param [out] width     Width if the item in pixels
         * @param [out] height    Height if the item in pixels */
        int getImageDimensions(uint32_t trackId, uint32_t itemId, std::uint32_t& width, std::uint32_t& height) const;

        /** Get items of a context
         * @param [in]  segTrackId Track or meta context id
         * @param [in][out]  items  items of a context */
        int32_t getContextItems(InitSegmentTrackId segTrackId, IdVector& items) const;

        /**
         * @brief Check item protection status from ItemInfoEntry item_protection_index
         * @param segTrackId Meta context ID
         * @param itemId    ID of the item
         * @return True if the item is protected. Returns always false for other than meta contexts. */
        bool isProtected(uint32_t trackId, std::uint32_t itemId) const;

        /** Get item data from AVC bitstream
         *  @param [in]  rawItemData Raw AVC bitstream data
         *  @param [out] itemData    Retrieved item data.
         *  @pre initialize() has been called successfully. */
        void getAvcItemData(const DataVector& rawItemData, DataVector& itemData);

        /** Get item data from HEVC bitstream
         *  @param [in]  rawItemData Raw HEVC bitstream data
         *  @param [out] itemData    Retrieved item data.
         *  @pre initialize() has been called successfully. */
        void getHevcItemData(const DataVector& rawItemData, DataVector& itemData);

        /** Process item data from AVC bitstream
         *  @param [out] memoryBuffer    Retrieved item data.
         *  @param [out] memoryBufferSize    Retrieved item data size.
         *  @pre initialize() has been called successfully. */
        ErrorCode processAvcItemData(char* memoryBuffer, uint32_t& memoryBufferSize);

        /** Process item data from HEVC bitstream
         *  @param [out] memoryBuffer    Retrieved item data.
         *  @param [out] memoryBufferSize    Retrieved item data size.*
         *  @pre initialize() has been called successfully. */
        ErrorCode processHevcItemData(char* memoryBuffer, uint32_t& memoryBufferSize);


        /* ********************************************************************** */
        /* *********************** Meta-specific section  *********************** */
        /* ********************************************************************** */

        Map<SegmentTrackId, MetaBox> mMetaBoxMap;  ///< Map of read MetaBoxes

        /// Reader internal information about each image item
        struct ImageInfo
        {
            String type = "invalid";   ///< Image item type, should be "master", "hidden", "pre-computed", or "hvc1"
            std::uint32_t width  = 0;  ///< Width of the image from ispe property
            std::uint32_t height = 0;  ///< Height of the image from ispe property
            double displayTime   = 0;  ///< Display timestamp generated by the reader
        };
        typedef Map<ItemId, ImageInfo> ImageInfoMap;

        /// For non-image items (Exif or XML metadata)
        struct ItemInfo
        {
            String type;  ///< Item type from Item Information box
        };
        typedef Map<ItemId, ItemInfo> ItemInfoMap;

        /// Reader internal information about each MetaBox
        struct MetaBoxInfo
        {
            uint32_t displayableMasterImages = 0;      ///< Number of master images
            bool isForcedLoopPlaybackEnabled = false;  ///< True if looping has been manually enabled
            bool isForcedFpsSet              = false;  ///< True if FPS for MetaBox has been manually set
            float forcedFps                  = 0.0;    ///< Manually set FPS
            ImageInfoMap imageInfoMap;                 ///< Information of all image items
            ItemInfoMap itemInfoMap;                   ///< Information of other items
        };
        Map<SegmentTrackId, MetaBoxInfo> mMetaBoxInfo;  ///< MetaBoxInfo struct for each MetaBox context

        InitTrackInfo& getInitTrackInfo(InitSegmentTrackId initSegTrackId);
        const InitTrackInfo& getInitTrackInfo(InitSegmentTrackId initSegTrackId) const;

        TrackInfo& getTrackInfo(InitSegmentId initSegmentId, SegmentTrackId segTrackId);
        const TrackInfo& getTrackInfo(InitSegmentId initSegmentId, SegmentTrackId segTrackId) const;
        bool hasTrackInfo(InitSegmentId initSegmentId, SegmentTrackId segTrackId) const;

        const SampleInfoVector& getSampleInfo(InitSegmentId initSegmentId,
                                              SegmentTrackId segTrackId,
                                              ItemId& itemIdBase) const;

        const ParameterSetMap* getParameterSetMap(Id id) const;  //< returns null if not found

        /**
         * @brief Find the preceding segment of given segment by using segment sequence numbers
         * @return Returns true if such a segment was found, in which case the segment id is placed
         *         to the return parameter precedingSegmentId
         */
        bool getPrecedingSegment(InitSegmentId initSegmentId,
                                 SegmentId curSegmentId,
                                 SegmentId& precedingSegmentId) const;

        /**
         * @brief Find the preceding track info of given segment/track by using segment sequence numbers
         *        This search skips segments that don't have the corresponding track
         * @return Returns a pointer to the found TrackInfo if it exists, otherwise it returns nullptr
         */
        const TrackInfo* getPrecedingTrackInfo(InitSegmentId initSegmentId, SegmentTrackId segTrackId) const;

        /**
         * @brief Sets the segment's sidx fallback value based on the time information available on
         *        preceding segment (per segment sequence number) if it exists
         */
        void setupSegmentSidxFallback(InitSegmentId initSegmentId, SegmentTrackId segTrackId);

        /**
         * Updates segment's sampleInfo's composition times per the trackInfo's pMap
         */
        void updateCompositionTimes(InitSegmentId initSegmentId, SegmentId segmentId);

        /**
         * @brief Extract information about non-image items of a MetaBox.
         * @param metaBox The MetaBox items of which to handle.
         * @return ItemInfoMap including information about non image type items. */
        ItemInfoMap extractItemInfoMap(const MetaBox& metaBox) const;

        /**
         * @brief Fill mImageToParameterSetMap and mParameterSetMap
         * @param segTrackId Meta context ID
         * @pre Filled mMetaBoxMap, filled mFileProperties.rootLevelMetaBoxProperties.imageFeaturesMap
         * @note To support multiple MetaBoxes, ImageFeaturesMap should be a parameter (not use one from root meta). */
        void processDecoderConfigProperties(const InitSegmentTrackId segTrackId);

        /**
         * @brief Extract MetaBoxInfo struct for reader internal use
         * @param metaBox   MetaBox to get information from
         * @param segTrackId Meta context ID
         * @pre Filled mMetaBoxMap
         * @return Filled MetaBoxInfo */
        MetaBoxInfo extractItems(SegmentIO& io, const MetaBox& metaBox, const InitSegmentTrackId segTrackId) const;

        /**
         * @brief Load item data.
         * @param metaBox The MetaBox where the item is located
         * @param itemId  ID of the item
         * @pre mInputStream is good
         * @return Item data */
        Vector<std::uint8_t> loadItemData(SegmentIO& io, const MetaBox& metaBox, const ItemId itemId) const;

        /**
         * @brief Load item data.
         * @param metaBox The MetaBox where the item is located
         * @param itemId  ID of the item
         * @param [out] data Item Data
         * @pre mInputStream is good */
        void readItem(SegmentIO& io, const MetaBox& metaBox, ItemId itemId, Vector<std::uint8_t>& data) const;

        /* *********************************************************************** */
        /* *********************** Track-specific section  *********************** */
        /* *********************************************************************** */

        Vector<std::int32_t> mMatrix;  ///< Video transformation matrix from the Movie Header Box

        /**
         * @brief Update mDecoderCodeTypeMap to include the data from the samples
         * @param [in] SampleInfoVector sampleInfo Samples to traverse
         */
        void updateDecoderCodeTypeMap(InitSegmentId initializationSegmentId,
                                      SegmentTrackId segTrackId,
                                      const SampleInfoVector& sampleInfo,
                                      std::size_t prevSampleInfoSize = 0);

        /**
         * @brief Update given itemToParameterSetMap
         * @param [in] SampleInfoVector sampleInfo Samples to traverse
         * @param [in] size_t prevSampleInfoSize Index to start interating on (to avoid duplicate work on consecutive
         * moofs)
         */
        void updateItemToParametersSetMap(ItemToParameterSetMap& itemToParameterSetMap,
                                          InitSegmentTrackId initSegTrackId,
                                          const SampleInfoVector& sampleInfo,
                                          size_t prevSampleInfoSize = 0);

        /**
         * @brief Add a mapping between segment number and a sequence
         * If the mapping already exists, nothing is changes. */
        void addSegmentSequence(InitSegmentId initializationSegmentId, SegmentId segmentId, Sequence sequence);

        /**
         * @brief Create a TrackGroupMap for radiply finding (ie.) alte track group mappings
         * @param [in] Track properties to build the data from
         * @return Filled TrackGroupMap */
        FourCCTrackReferenceInfoMap trackGroupsOfTrackProperties(const TrackPropertiesMap& aTrackProperties) const;

        /**
         * @brief Create a TrackPropertiesMap struct for the reader interface
         * @param [in] segmentId Segment id of the track.
         * @param [in] moovBox MovieBox to extract properties from
         * @return Filled TrackPropertiesMap */
        TrackPropertiesMap fillTrackProperties(InitSegmentId initializationSegmentId,
                                               SegmentId segmentId,
                                               MovieBox& moovBox);

        /**
         * For a given segment and track, find the first following (non-overlapping) ItemId
         */
        ItemId getFollowingItemId(InitSegmentId initializationSegmentId, SegmentTrackId segTrackId) const;

        /**
         * For a given segment and track, find the last (non-overlapping) ItemId + 1 (or 0 if at the beginning)
         */
        ItemId getPrecedingItemId(InitSegmentId initializationSegmentId, SegmentTrackId segTrackId) const;

        typedef Map<ContextId, DecodePts::PresentationTimeTS> ContextIdPresentationTimeTSMap;
        /**
         * @brief Add to a TrackPropertiesMap struct for the reader interface
         * @param [in] segmentId Segment id of the track.
         * @param [in] moofBox MovieFragmentBox to extract properties from */
        void addToTrackProperties(InitSegmentId initSegmentId,
                                  SegmentId segmentId,
                                  MovieFragmentBox& moofBox,
                                  const ContextIdPresentationTimeTSMap& earliestPTSTS);

        /**
         * @brief Add samples to a addToTrackInfo for the reader interface
         * @param [in/out] trackInfo TrackInfo reference to add to.
         * @param [in] timeScale  std::uint32_t timescale for the samples.
         * @param [in] baseDataOffset  std::uint64_t baseDataOffset for the samples.
         * @param [in] ItemId itemIdBase sample id for the first sample in the current segment
         * @param [in] ItemId trackrunItemIdBase sample id for the first sample the current track run
         * @param [in] uint32_t sampleDescriptionIndex for samples in this run.
         * @param [in] trackRunBox TrackRunBox* to extract sample properties from */
        void addSamplesToTrackInfo(TrackInfo& trackInfo,
                                   const InitSegmentProperties& initSegmentProperties,
                                   const InitTrackInfo& initTrackInfo,
                                   const TrackProperties& trackProperties,
                                   const ISOBMFF::Optional<std::uint64_t> baseDataOffset,
                                   const uint32_t sampleDescriptionIndex,
                                   ItemId itemIdBase,
                                   ItemId trackrunItemIdBase,
                                   const TrackRunBox* trackRunBox);

        /**
         * @brief Create a MoovProperties struct for the reader interface
         * @param [in] moovBox MovieBox to extract properties from
         * @return Filled MoovProperties */
        MoovProperties extractMoovProperties(const MovieBox& moovBox) const;

        /**
         * @brief Fill mParameterSetMap and mTrackInfo clapProperties entries for a a TrackBox
         * @param [in] trackBox TrackBox to extract data from */
        void fillSampleEntryMap(TrackBox* trackBox, InitSegmentId initSegmentId);

        /**
         * @brief Fill properties map; used by fillSampleEntryMap
         * @param [in] stsdBox The SampleDescriptionBox
         * @param [in] getPropertyMap Retrieve the matching property map from given InitTrackInfo
         */
        template <typename SampleEntryBox, typename GetPropertyMap>
        void fillSampleProperties(InitSegmentTrackId initSegTrackId,
                                  const SampleDescriptionBox& stsdBox,
                                  GetPropertyMap getPropertyMap);

        /**
         * @brief Fills additional data found from restricted info scheme, ovly etc. boxes
         * of sample entry to init track info.
         */
        void fillAdditionalBoxInfoForSampleEntry(InitTrackInfo& trackInfo,
                                                 unsigned int index,
                                                 const SampleEntryBox& entry);

        /**
         * @brief Create a TrackFeature struct for the reader interface
         * @param [in] trackBox TrackBox to extract data from
         * @return Filled TrackFeature */
        TrackFeature getTrackFeatures(TrackBox* trackBox) const;

        /**
         * @brief Extract information about reference tracks for the reader interface
         * @param [in] trackBox TrackBox to extract data from
         * @return Reference track information */
        TypeToContextIdsMap getReferenceTrackIds(TrackBox* trackBox) const;

        /**
         * @brief Extract information about track groups for the reader interface
         * @param [in] trackBox TrackBox to extract data from
         * @return Track group information */
        TrackGroupInfoMap getTrackGroupInfoMap(TrackBox* trackBox) const;

        /**
         * @brief Extract information about sample group IDs for the reader interface
         * @param [in] trackBox TrackBox to extract data from
         * @return Sample grouping information */
        TypeToIdsMap getSampleGroupIds(TrackBox* trackBox) const;

        /**
         * @brief Extract information about alternate track IDs for a track for the reader interface
         * @param [in] trackBox TrackBox to look alternate track IDs for
         * @param [in] moovBox  MovieBox containing the TrackBox
         * @return IDs of alternate tracks. An empty vector if there was no alternates */
        IdVector getAlternateTrackIds(TrackBox* trackBox, MovieBox& moovBox) const;

        /**
         * @brief Extract reader internal TrackInfo structure from TrackBox
         * @param [in] trackBox TrackBox to extract data from
         * @return Filled TrackInfo struct */
        std::pair<InitTrackInfo, TrackInfo> extractTrackInfo(TrackBox* trackBox, uint32_t movieTimescale) const;

        /**
         * @brief Extract reader internal information about samples
         * @param trackBox [in] trackBox TrackBox to extract data from
         * @param pMap Presentation map for the track
         * @return SampleInfoVector containing information about every sample of the track */
        SampleInfoVector makeSampleInfoVector(TrackBox* trackBox) const;

        /**
         * @brief Add sample decoding dependencies
         * @param segTrackId         ID of the track
         * @param itemDecodingOrder Items/samples to get decoding dependencies for
         * @param [out] output DecodingOrderVector with decoding dependencies
         * @return  ErrorCode::NO_ERROR, ::UNINITIALIZED or ::INVALID_CONTEXT_ID*/
        int addDecodingDependencies(InitSegmentId initSegmentId,
                                    SegmentTrackId segmentTrackId,
                                    const DecodingOrderVector& itemDecodingOrder,
                                    DecodingOrderVector& output) const;

        /**
         * @brief Make SegmentIndexBox array from SegmentIndexBox information
         * @param [in] sidxBox SegmentIndexBox  to get info from.
         * @param [out] segmentIndex SegmentIndex to store info into
         * @return  ErrorCode::NO_ERROR */
        void makeSegmentIndex(const SegmentIndexBox& sidxBox, SegmentIndex& segmentIndex, int64_t dataOffsetAnchor);

        /**
         * @brief Read bytes from stream to an integer value.
         * @param stream Stream to read bytes from.
         * @param count  Number of bytes to read (0-8).
         * @param result Read value
         * @return ErrorCode.
         */
        int32_t readBytes(SegmentIO& io, uint32_t count, std::int64_t& result);

        /**
         * @brief Seek to the desired offset in the input stream (counting from the beginning).
         *        Eliminates redundant seeks (and buffer discarding).
         * @param pos The offset to seek to.
         */
        void seekInput(SegmentIO& io, std::int64_t pos) const
        {
            if (io.stream->tell() != pos)
            {
                io.stream->seek(pos);
            }
        }

        /**
         * Convert a segment id + track id pair into a track id for the client
         * application. Segment id and track id must be 16 bit to fit into the
         * 32-bit track id.
         * @param id The id (pair of segment id and track id)
         * @return An id suitable for passing to the client application
         */
        unsigned toTrackId(InitSegmentTrackId id) const;

        /**
         * Convert a 32-bit track id (one that contains both init segment id and track id) to init segment id
         */
        InitSegmentTrackId ofTrackId(unsigned id) const;

        /** Given an init segment id and an item id find the segment id */
        int32_t segmentIdOf(InitSegmentTrackId initSegTrackId, ItemId itemId, SegmentId& segmentId) const;
        int32_t segmentIdOf(Id id, SegmentId& segmentId) const;

        int32_t getRefSampleDataInfo(uint32_t trackId,
                                     uint32_t itemIdApi,
                                     const InitSegmentId& initSegmentId,
                                     uint8_t trackReference,
                                     uint64_t& refSampleLength,
                                     uint64_t& refDataOffset);

        uint64_t readNalLength(char* buffer) const;
        void writeNalLength(uint64_t length, char* buffer) const;
    };

    /**
     * Match types from map that is passed as argument to be able to use auto variables when calling.
     */
    template <typename Property>
    inline Property MP4VRFileReaderImpl::fetchSampleProp(const PropertyMap<Property>& propertiesMap,
                                                         const SampleDescriptionIndex& index,
                                                         uint32_t& result) const
    {
        // if already failed no need to go further
        if (result != ErrorCode::OK)
        {
            return Property();
        }

        if (propertiesMap.count(index.get()) == 0)
        {
            result = ErrorCode::INVALID_SAMPLE_DESCRIPTION_INDEX;
            return Property();
        }

        return propertiesMap.at(index);
    }

    /** Generic "find value from associative container, or don't" function to use with maps.
     *
     * findOptional(key, map) returns map[key] if it exists, otherwise it returns a None
     */
    template <typename Container, typename Key>
    auto findOptional(const Key& aKey, const Container& aContainer) -> Optional<typename Container::mapped_type>
    {
        auto value = aContainer.find(aKey);
        if (value != aContainer.end())
        {
            return {value->second};
        }
        else
        {
            return {};
        }
    }
}  // namespace MP4VR

#endif /* MP4VRFILEREADERIMPL_HPP */
