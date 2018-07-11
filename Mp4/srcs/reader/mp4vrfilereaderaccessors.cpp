
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
#include "customallocator.hpp"
#include "hevcsampleentry.hpp"
#include "mp4vrfiledatatypesinternal.hpp"
#include "mp4vrfilereaderimpl.hpp"
#include "mp4vrfilereaderutil.hpp"
#include "mp4vrhvc2extractor.hpp"

#include "log.hpp"

#include <cassert>
#include <cstring>

namespace MP4VR
{
    namespace
    {
        template <typename T, typename Container>
        DynArray<T> makeDynArray(const Container& container)
        {
            DynArray<T> array(container.size());
            for (typename Container::const_iterator it = container.begin(); it != container.end(); ++it)
            {
                array.elements[it - container.begin()] = T(*it);
            }
            return array;
        }

        template <typename Container, typename Map>
        auto makeDynArrayMap(const Container& container, Map map) -> DynArray<decltype(map(*container.begin()))>
        {
            DynArray<decltype(map(*container.begin()))> array(container.size());
            for (typename Container::const_iterator it = container.begin(); it != container.end(); ++it)
            {
                array.elements[it - container.begin()] = map(*it);
            }
            return array;
        }

        template <typename T, typename Container>
        DynArray<DynArray<T>> makeDynArray2d(const Container& container)
        {
            DynArray<DynArray<T>> array(container.size());
            for (typename Container::const_iterator it = container.begin(); it != container.end(); ++it)
            {
                array.elements[it - container.begin()] = makeDynArray<T>(*it);
            }
            return array;
        }

        template <typename Container>
        DynArray<FourCC> makeFourCCDynArray(const Container& container)
        {
            DynArray<FourCC> array(container.size());
            for (typename Container::const_iterator it = container.begin(); it != container.end(); ++it)
            {
                array.elements[it - container.begin()] = FourCC(it->c_str());
            }
            return array;
        }
    }  // anonymous namespace

    int32_t MP4VRFileReaderImpl::getMajorBrand(FourCC& majorBrand,
                                               uint32_t initializationSegmentId,
                                               uint32_t segmentId) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        bool isInitSegment = mInitSegmentPropertiesMap.count(initializationSegmentId) > 0;
        if (!isInitSegment)
        {
            return ErrorCode::INVALID_SEGMENT;
        }

        bool isSegment = false;
        if (segmentId != UINT32_MAX)
        {
            isSegment = mInitSegmentPropertiesMap.at(initializationSegmentId).segmentPropertiesMap.count(segmentId) > 0;
            if (!isSegment)
            {
                return ErrorCode::INVALID_SEGMENT;
            }
        }

        majorBrand = FourCC((isSegment ? mInitSegmentPropertiesMap.at(initializationSegmentId)
                                             .segmentPropertiesMap.at(segmentId)
                                             .styp.getMajorBrand()
                                       : mInitSegmentPropertiesMap.at(initializationSegmentId).ftyp.getMajorBrand())
                                .c_str());
        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getMinorVersion(uint32_t& minorVersion,
                                                 uint32_t initializationSegmentId,
                                                 uint32_t segmentId) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        bool isInitSegment = mInitSegmentPropertiesMap.count(initializationSegmentId) > 0;
        if (!isInitSegment)
        {
            return ErrorCode::INVALID_SEGMENT;
        }

        bool isSegment = false;
        if (segmentId != UINT32_MAX)
        {
            isSegment = mInitSegmentPropertiesMap.at(initializationSegmentId).segmentPropertiesMap.count(segmentId) > 0;
            if (!isSegment)
            {
                return ErrorCode::INVALID_SEGMENT;
            }
        }

        minorVersion = (isSegment ? mInitSegmentPropertiesMap.at(initializationSegmentId)
                                        .segmentPropertiesMap.at(segmentId)
                                        .styp.getMinorVersion()
                                  : mInitSegmentPropertiesMap.at(initializationSegmentId).ftyp.getMinorVersion());

        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getCompatibleBrands(DynArray<FourCC>& compatibleBrands,
                                                     uint32_t initializationSegmentId,
                                                     uint32_t segmentId) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        bool isInitSegment = mInitSegmentPropertiesMap.count(initializationSegmentId) > 0;
        if (!isInitSegment)
        {
            return ErrorCode::INVALID_SEGMENT;
        }

        bool isSegment = false;
        if (segmentId != UINT32_MAX)
        {
            isSegment = mInitSegmentPropertiesMap.at(initializationSegmentId).segmentPropertiesMap.count(segmentId) > 0;
            if (!isSegment)
            {
                return ErrorCode::INVALID_SEGMENT;
            }
        }

        compatibleBrands = makeFourCCDynArray(
            isSegment ? mInitSegmentPropertiesMap.at(initializationSegmentId)
                            .segmentPropertiesMap.at(segmentId)
                            .styp.getCompatibleBrands()
                      : mInitSegmentPropertiesMap.at(initializationSegmentId).ftyp.getCompatibleBrands());

        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getFileInformation(FileInformation& fileInfo, uint32_t initializationSegmentId) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        bool isInitSegment = mInitSegmentPropertiesMap.count(initializationSegmentId) > 0;
        if (!isInitSegment)
        {
            return ErrorCode::INVALID_SEGMENT;
        }
        fileInfo.features = mInitSegmentPropertiesMap.at(initializationSegmentId).fileFeature.getFeatureMask();
        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getTrackInformations(DynArray<TrackInformation>& trackInfos) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        size_t totalSize = 0;
        for (auto& propMap : mInitSegmentPropertiesMap)
        {
            totalSize += propMap.second.initTrackInfos.size();
        }

        trackInfos               = DynArray<TrackInformation>(totalSize);
        uint32_t outTrackIdxBase = 0;
        size_t i                 = 0;
        for (auto const& initSegment : mInitSegmentPropertiesMap)
        {
            InitSegmentId initSegmentId = initSegment.first;
            uint32_t outTrackIdx        = outTrackIdxBase;

            for (auto const& trackPropsKv : initSegment.second.trackProperties)
            {
                ContextId trackId                      = trackPropsKv.first;
                const TrackProperties& trackProperties = trackPropsKv.second;
                //            const TrackInfo& trackInfo = trackInfosKv.second;
                trackInfos.elements[outTrackIdx].initSegmentId = initSegmentId.get();
                trackInfos.elements[outTrackIdx].trackId       = toTrackId(std::make_pair(initSegment.first, trackId));
                trackInfos.elements[outTrackIdx].alternateGroupId = trackProperties.alternateGroupId;
                trackInfos.elements[outTrackIdx].features         = trackProperties.trackFeature.getFeatureMask();
                trackInfos.elements[outTrackIdx].vrFeatures       = trackProperties.trackFeature.getVRFeatureMask();
                trackInfos.elements[outTrackIdx].timeScale =
                    mInitSegmentPropertiesMap.at(initSegmentId).initTrackInfos.at(trackId).timeScale;
                trackInfos.elements[outTrackIdx].frameRate = {};
                String tempURI                             = trackProperties.trackURI;
                tempURI.push_back('\0');  // make sure URI is null terminated.
                trackInfos.elements[outTrackIdx].trackURI = makeDynArray<char>(tempURI);
                trackInfos.elements[outTrackIdx].alternateTrackIds =
                    makeDynArray<unsigned int>(trackProperties.alternateTrackIds);

                trackInfos.elements[outTrackIdx].referenceTrackIds =
                    DynArray<TypeToTrackIDs>(trackProperties.referenceTrackIds.size());
                i = 0;
                for (auto const& reference : trackProperties.referenceTrackIds)
                {
                    trackInfos.elements[outTrackIdx].referenceTrackIds[i].type = FourCC(reference.first.getUInt32());
                    trackInfos.elements[outTrackIdx].referenceTrackIds[i].trackIds =
                        makeDynArrayMap(reference.second, [&](ContextId aContextId) {
                            return toTrackId({initSegment.first, aContextId});
                        });
                    i++;
                }

                trackInfos.elements[outTrackIdx].trackGroupIds =
                    DynArray<TypeToTrackIDs>(trackProperties.trackGroupIds.size());
                i = 0;
                for (auto const& group : trackProperties.trackGroupIds)
                {
                    trackInfos.elements[outTrackIdx].trackGroupIds[i].type = FourCC(group.first.getUInt32());
                    trackInfos.elements[outTrackIdx].trackGroupIds[i].trackIds =
                        makeDynArray<unsigned int>(group.second);
                    i++;
                }
                outTrackIdx++;
            }

            // rewind and loop again, but this time we are interested only in samples

            Map<uint32_t, size_t> trackSampleCounts;  // sum of the number of samples for each outTrackIdx

            SmallVector<ContextId, 16> initSegTrackIds;
            {
                uint32_t count = 0;
                for (auto const& initTrackInfosKv : mInitSegmentPropertiesMap.at(initSegmentId).initTrackInfos)
                {
                    initSegTrackIds.push_back(initTrackInfosKv.first);
                    trackSampleCounts[count + outTrackIdxBase] = 0u;
                    ++count;
                }
            }


            for (auto const& allSegmentProperties : initSegment.second.segmentPropertiesMap)
            {
                if (allSegmentProperties.second.initSegmentId == initSegmentId)
                {
                    auto& segTrackInfos = allSegmentProperties.second.trackInfos;
                    outTrackIdx         = outTrackIdxBase;
                    for (ContextId trackId : initSegTrackIds)
                    {
                        auto trackInfo = segTrackInfos.find(trackId);
                        if (trackInfo != segTrackInfos.end())
                        {
                            trackSampleCounts[outTrackIdx] += trackInfo->second.samples.size();
                        }
                        ++outTrackIdx;
                    }
                }
            }

            for (auto const& trackSampleCount : trackSampleCounts)
            {
                auto counttrackid                                  = trackSampleCount.first;
                auto count                                         = trackSampleCount.second;
                trackInfos.elements[counttrackid].sampleProperties = DynArray<SampleInformation>(count);
                trackInfos.elements[counttrackid].maxSampleSize    = 0;
            }

            Map<uint32_t, size_t> sampleOffset;
            for (auto const& segment : segmentsBySequence(initSegmentId))
            {
                outTrackIdx = outTrackIdxBase;
                for (ContextId trackId : initSegTrackIds)
                {
                    auto trackInfoIt = segment.trackInfos.find(trackId);
                    if (trackInfoIt != segment.trackInfos.end())
                    {
                        auto& trackInfo = trackInfoIt->second;
                        i               = sampleOffset[outTrackIdx];

                        if (trackInfo.hasTtyp)
                        {
                            auto& tBox                                          = trackInfo.ttyp;
                            trackInfos.elements[outTrackIdx].hasTypeInformation = true;

                            trackInfos.elements[outTrackIdx].type.majorBrand   = tBox.getMajorBrand().c_str();
                            trackInfos.elements[outTrackIdx].type.minorVersion = tBox.getMinorVersion();

                            Vector<FourCC> convertedCompatibleBrands;
                            for (auto& compatibleBrand : tBox.getCompatibleBrands())
                            {
                                convertedCompatibleBrands.push_back(FourCC(compatibleBrand.c_str()));
                            }
                            trackInfos.elements[outTrackIdx].type.compatibleBrands =
                                makeDynArray<FourCC>(convertedCompatibleBrands);
                        }
                        else
                        {
                            trackInfos.elements[outTrackIdx].hasTypeInformation = false;
                        }

                        if (trackInfo.samples.size() > 0)
                        {
                            std::uint32_t delta;
                            if (trackInfo.samples.size() >= 3)
                            {
                                delta = trackInfo.samples.at(1).sampleDuration;
                            }
                            else
                            {
                                delta = trackInfo.samples.at(0).sampleDuration;
                            }
                            auto timeScale =
                                mInitSegmentPropertiesMap.at(initSegmentId).initTrackInfos.at(trackId).timeScale;
                            trackInfos.elements[outTrackIdx].frameRate = Rational{timeScale, delta};
                        }

                        for (auto const& sample : trackInfo.samples)
                        {
                            trackInfos.elements[outTrackIdx].sampleProperties[i].sampleId =
                                ItemId(sample.sampleId).get();
                            trackInfos.elements[outTrackIdx].sampleProperties[i].sampleEntryType =
                                FourCC(sample.sampleEntryType.getUInt32());
                            trackInfos.elements[outTrackIdx].sampleProperties[i].sampleDescriptionIndex =
                                sample.sampleDescriptionIndex.get();
                            trackInfos.elements[outTrackIdx].sampleProperties[i].sampleType = sample.sampleType;
                            trackInfos.elements[outTrackIdx].sampleProperties[i].initSegmentId =
                                segment.initSegmentId.get();
                            trackInfos.elements[outTrackIdx].sampleProperties[i].segmentId = segment.segmentId.get();
                            if (sample.compositionTimes.size())
                            {
                                trackInfos.elements[outTrackIdx].sampleProperties[i].earliestTimestamp =
                                    sample.compositionTimes.at(0);
                                trackInfos.elements[outTrackIdx].sampleProperties[i].earliestTimestampTS =
                                    sample.compositionTimesTS.at(0);
                            }
                            else
                            {
                                trackInfos.elements[outTrackIdx].sampleProperties[i].earliestTimestamp   = 0;
                                trackInfos.elements[outTrackIdx].sampleProperties[i].earliestTimestampTS = 0;
                            }
                            trackInfos.elements[outTrackIdx].sampleProperties[i].sampleFlags.flagsAsUInt =
                                sample.sampleFlags.flagsAsUInt;
                            trackInfos.elements[outTrackIdx].sampleProperties[i].sampleDurationTS =
                                sample.sampleDuration;

                            unsigned int sampleSize = sample.dataLength;
                            if (sampleSize > trackInfos.elements[outTrackIdx].maxSampleSize)
                            {
                                trackInfos.elements[outTrackIdx].maxSampleSize = sampleSize;
                            }
                            i++;
                        }
                        sampleOffset[outTrackIdx] = i;
                    }
                    ++outTrackIdx;
                }
            }

            outTrackIdxBase += static_cast<uint32_t>(initSegment.second.initTrackInfos.size());
        }

        // V2: fill in also the SampleInformation::segmentId
        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::getDisplayWidth(uint32_t trackId, uint32_t& displayWidth) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        ContextType contextType;
        int error = getContextTypeError(ofTrackId(trackId), contextType);
        if (error)
        {
            return error;
        }

        if (contextType == ContextType::TRACK)
        {
            displayWidth = getInitTrackInfo(ofTrackId(trackId)).width >> 16;
            return ErrorCode::OK;
        }
        else
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
    }


    int32_t MP4VRFileReaderImpl::getDisplayHeight(uint32_t trackId, uint32_t& displayHeight) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        ContextType contextType;
        int error = getContextTypeError(ofTrackId(trackId), contextType);
        if (error)
        {
            return error;
        }

        if (contextType == ContextType::TRACK)
        {
            displayHeight = getInitTrackInfo(ofTrackId(trackId)).height >> 16;
            return ErrorCode::OK;
        }
        else
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
    }

    int32_t MP4VRFileReaderImpl::getDisplayWidthFP(uint32_t trackId, uint32_t& displayWidth) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        ContextType contextType;
        int error = getContextTypeError(ofTrackId(trackId), contextType);
        if (error)
        {
            return error;
        }

        if (contextType == ContextType::TRACK)
        {
            displayWidth = getInitTrackInfo(ofTrackId(trackId)).width;
            return ErrorCode::OK;
        }
        else
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
    }

    int32_t MP4VRFileReaderImpl::getDisplayHeightFP(uint32_t trackId, uint32_t& displayHeight) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        ContextType contextType;
        int error = getContextTypeError(ofTrackId(trackId), contextType);
        if (error)
        {
            return error;
        }

        if (contextType == ContextType::TRACK)
        {
            displayHeight = getInitTrackInfo(ofTrackId(trackId)).height;
            return ErrorCode::OK;
        }
        else
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
    }

    int32_t MP4VRFileReaderImpl::getWidth(uint32_t trackId, uint32_t itemId, uint32_t& width) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        std::uint32_t tempheight = 0;
        return getImageDimensions(trackId, itemId, width, tempheight);
    }


    int32_t MP4VRFileReaderImpl::getHeight(uint32_t trackId, uint32_t itemId, uint32_t& height) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        std::uint32_t tempwidth = 0;
        return getImageDimensions(trackId, itemId, tempwidth, height);
    }


    int32_t MP4VRFileReaderImpl::getPlaybackDurationInSecs(uint32_t trackId, double& durationInSecs) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto initSegTrackId = ofTrackId(trackId);
        double duration     = 0.0;
        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        InitSegmentId initSegmentId = initSegTrackId.first;
        ContextId initTrackId       = initSegTrackId.second;

        switch (contextType)
        {
        case ContextType::TRACK:
        {
            std::int64_t maxTimeUs = 0;
            std::uint32_t timescale =
                mInitSegmentPropertiesMap.at(initSegmentId).initTrackInfos.at(initTrackId).timeScale;
            for (const auto& segment : mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap)
            {
                if (segment.second.initSegmentId == initSegTrackId.first)
                {
                    const auto& trackInfo = segment.second.trackInfos.find(initSegTrackId.second);
                    if (trackInfo != segment.second.trackInfos.end())
                    {
                        if (trackInfo->second.samples.size())
                        {
                            for (const auto& compositionTimesTS : trackInfo->second.samples.back().compositionTimesTS)
                            {
                                maxTimeUs = std::max(
                                    maxTimeUs,
                                    int64_t((compositionTimesTS + trackInfo->second.samples.back().sampleDuration) *
                                            1000000 / timescale));
                            }
                        }
                    }
                }
            }
            duration = maxTimeUs / 1000000.0;
            break;
        }

        default:
            for (const auto& segment : mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap)
            {
                double curDuration = 0.0;
                if (segment.second.initSegmentId == initSegTrackId.first)
                {
                    SegmentId segmentId       = segment.first;
                    SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);
                    switch (contextType)
                    {
                    case ContextType::META:
                        if (mMetaBoxInfo.at(segTrackId).isForcedFpsSet)
                        {
                            curDuration = mMetaBoxInfo.at(segTrackId).displayableMasterImages /
                                          mMetaBoxInfo.at(segTrackId).forcedFps;
                        }
                        else
                        {
                            logWarning()
                                << "getPlaybackDurationInSecs() called for meta context, but forced FPS was not set"
                                << std::endl;
                        }
                        break;
                    case ContextType::FILE:
                        // Find maximum curDuration of contexts
                        for (const auto& contextInfo : mContextInfoMap)
                        {
                            double contextDuration;
                            error = getPlaybackDurationInSecs(toTrackId(contextInfo.first), contextDuration);
                            if (error)
                            {
                                return error;
                            }
                            if (contextDuration > curDuration)
                            {
                                curDuration = contextDuration;
                            }
                        }
                        break;
                    default:
                        return ErrorCode::INVALID_CONTEXT_ID;
                    }
                }
                duration += curDuration;
            }
        }

        durationInSecs = duration;
        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getTrackSampleListByType(uint32_t trackId,
                                                          TrackSampleType itemType,
                                                          DynArray<uint32_t>& itemIds) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        IdVector allItems;
        int32_t error = getContextItems(initSegTrackId, allItems);
        if (error)
        {
            return error;
        }

        Vector<uint32_t> matches;
        ContextType contextType;
        error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        bool foundAny = false;

        for (const auto& segment : segmentsBySequence(initSegTrackId.first))
        {
            foundAny                  = true;
            SegmentTrackId segTrackId = std::make_pair(segment.segmentId, initSegTrackId.second);
            if (contextType == ContextType::TRACK)
            {
                if (itemType == TrackSampleType::out_ref)
                {
                    if (hasTrackInfo(initSegmentId, segTrackId))
                    {
                        ItemId sampleBase;
                        const SampleInfoVector& sampleInfo = getSampleInfo(initSegmentId, segTrackId, sampleBase);
                        for (uint32_t index = 0; index < sampleInfo.size(); ++index)
                        {
                            if (sampleInfo[index].sampleType == SampleType::OUTPUT_REFERENCE_FRAME)
                            {
                                matches.push_back((ItemId(index) + sampleBase).get());
                            }
                        }
                    }
                }
                else if (itemType == TrackSampleType::non_out_ref)
                {
                    if (hasTrackInfo(initSegmentId, segTrackId))
                    {
                        ItemId sampleBase;
                        const SampleInfoVector& sampleInfo = getSampleInfo(initSegmentId, segTrackId, sampleBase);
                        for (uint32_t index = 0; index < sampleInfo.size(); ++index)
                        {
                            if (sampleInfo[index].sampleType == SampleType::NON_OUTPUT_REFERENCE_FRAME)
                            {
                                matches.push_back((ItemId(index) + sampleBase).get());
                            }
                        }
                    }
                }
                else if (itemType == TrackSampleType::out_non_ref)
                {
                    if (hasTrackInfo(initSegmentId, segTrackId))
                    {
                        ItemId sampleBase;
                        const SampleInfoVector& sampleInfo = getSampleInfo(initSegmentId, segTrackId, sampleBase);
                        for (uint32_t index = 0; index < sampleInfo.size(); ++index)
                        {
                            if (sampleInfo[index].sampleType == SampleType::OUTPUT_NON_REFERENCE_FRAME)
                            {
                                matches.push_back((ItemId(index) + sampleBase).get());
                            }
                        }
                    }
                }
                else if (itemType == TrackSampleType::display)
                {
                    if (hasTrackInfo(initSegmentId, segTrackId))
                    {
                        IdVector sampleIds;
                        // Collect frames to display
                        ItemId sampleBase;
                        const SampleInfoVector& sampleInfo = getSampleInfo(initSegmentId, segTrackId, sampleBase);
                        for (uint32_t index = 0; index < sampleInfo.size(); ++index)
                        {
                            if (sampleInfo[index].sampleType == SampleType::OUTPUT_NON_REFERENCE_FRAME ||
                                sampleInfo[index].sampleType == SampleType::OUTPUT_REFERENCE_FRAME)
                            {
                                sampleIds.push_back((ItemId(index) + sampleBase).get());
                            }
                        }

                        // Collect every presentation time stamp of every sample
                        Vector<ItemIdTimestampPair> samplePresentationTimes;
                        for (auto sampleId : sampleIds)
                        {
                            auto& trackInfo = getTrackInfo(initSegmentId, segTrackId);
                            const auto singleSamplePresentationTimes =
                                trackInfo.samples.at((ItemId(sampleId) - trackInfo.itemIdBase).get()).compositionTimes;
                            for (auto sampleTime : singleSamplePresentationTimes)
                            {
                                samplePresentationTimes.push_back(std::make_pair(sampleId, sampleTime));
                            }
                        }

                        // Sort to display order using composition times
                        std::sort(samplePresentationTimes.begin(), samplePresentationTimes.end(),
                                  [&](ItemIdTimestampPair a, ItemIdTimestampPair b) { return a.second < b.second; });

                        // Push sample ids to the result
                        for (auto pair : samplePresentationTimes)
                        {
                            matches.push_back(pair.first.get());
                        }
                    }
                }
                else if (itemType == TrackSampleType::samples)
                {
                    matches = allItems;
                }
                else
                {
                    return ErrorCode::INVALID_FUNCTION_PARAMETER;
                }
            }
        }

        if (!foundAny)
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }

        itemIds = makeDynArray<uint32_t>(matches);
        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getTrackSampleType(uint32_t trackId, uint32_t itemId, FourCC& trackItemType) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        SegmentId segmentId;
        int32_t result = segmentIdOf(initSegTrackId, itemId, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);
        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        FourCCInt boxtype;
        switch (contextType)
        {
        case ContextType::TRACK:
        {
            ItemId sampleBase;
            auto& sampleInfo = getSampleInfo(initSegmentId, segTrackId, sampleBase);
            boxtype          = sampleInfo.at((ItemId(itemId) - sampleBase).get()).sampleEntryType;
            trackItemType    = FourCC(boxtype.getUInt32());
            return ErrorCode::OK;
        }

        default:
            return ErrorCode::INVALID_CONTEXT_ID;
        }
    }

    int32_t MP4VRFileReaderImpl::segmentIdOf(InitSegmentTrackId initSegTrackId,
                                             ItemId itemId,
                                             SegmentId& segmentId) const
    {
        bool wentPast = false;

        if (mInitSegmentPropertiesMap.empty() || !mInitSegmentPropertiesMap.count(initSegTrackId.first) ||
            mInitSegmentPropertiesMap.at(initSegTrackId.first).segmentPropertiesMap.empty())
        {
            return ErrorCode::INVALID_SEGMENT;
        }

        const auto& segs = segmentsBySequence(initSegTrackId.first);
        for (auto segmentIt = segs.begin(); !wentPast && segmentIt != segs.end(); ++segmentIt)
        {
            if (segmentIt->initSegmentId == initSegTrackId.first)
            {
                auto track = segmentIt->trackInfos.find(initSegTrackId.second);
                if (track != segmentIt->trackInfos.end())
                {
                    if (track->second.itemIdBase > ItemId(itemId))
                    {
                        wentPast = true;
                    }
                    else
                    {
                        segmentId = segmentIt->segmentId;
                    }
                }
            }
        }

        int ret      = ErrorCode::INVALID_ITEM_ID;
        auto segment = mInitSegmentPropertiesMap.at(initSegTrackId.first).segmentPropertiesMap.find(segmentId);
        if (segment != mInitSegmentPropertiesMap.at(initSegTrackId.first).segmentPropertiesMap.end())
        {
            auto track = segment->second.trackInfos.find(initSegTrackId.second);
            if (track != segment->second.trackInfos.end())
            {
                if (itemId - track->second.itemIdBase < ItemId(std::uint32_t(track->second.samples.size())))
                {
                    ret = ErrorCode::OK;
                }
            }
        }
        return ret;
    }

    int32_t MP4VRFileReaderImpl::segmentIdOf(Id id, SegmentId& segmentId) const
    {
        return segmentIdOf(id.first, id.second, segmentId);
    }

    int32_t MP4VRFileReaderImpl::getRefSampleDataInfo(uint32_t trackId,
                                                      uint32_t itemIdApi,
                                                      const InitSegmentId& initSegmentId,
                                                      uint8_t trackReference,
                                                      uint64_t& refSampleLength,
                                                      uint64_t& refDataOffset)
    {
        auto trackContextId = ofTrackId(trackId).second;
        auto refTrackContextId =
            mInitSegmentPropertiesMap[initSegmentId].trackProperties[trackContextId].referenceTrackIds["scal"].at(
                trackReference);

        // Create a new pair: init segment from the extractor but track context id from the referred media track where
        // data is extracted from.
        InitSegmentTrackId refInitSegTrackId = std::make_pair(initSegmentId, refTrackContextId);

        SegmentId refSegmentId;
        int32_t result = segmentIdOf(refInitSegTrackId, itemIdApi, refSegmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId refSegTrackId = std::make_pair(refSegmentId, refInitSegTrackId.second);
        ItemId refItemId             = ItemId(itemIdApi) - getTrackInfo(initSegmentId, refSegTrackId).itemIdBase;

        refDataOffset   = getTrackInfo(initSegmentId, refSegTrackId).samples.at(refItemId.get()).dataOffset;
        refSampleLength = getTrackInfo(initSegmentId, refSegTrackId).samples.at(refItemId.get()).dataLength;

        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::getTrackSampleData(uint32_t trackId,
                                                    uint32_t itemIdApi,
                                                    char* memoryBuffer,
                                                    uint32_t& memoryBufferSize,
                                                    bool videoByteStreamHeaders)
    {
        uint32_t spaceAvailable = memoryBufferSize;
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        SegmentId segmentId;
        int32_t result = segmentIdOf(initSegTrackId, itemIdApi, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);
        ItemId itemId             = ItemId(itemIdApi) - getTrackInfo(initSegmentId, segTrackId).itemIdBase;

        SegmentIO& io = mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segmentId).io;
        // read NAL data to bitstream object
        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }
        switch (contextType)
        {
        case ContextType::TRACK:
        {
            // The requested frame should be one that is available
            if (itemId.get() >= getTrackInfo(initSegmentId, segTrackId).samples.size())
            {
                return ErrorCode::INVALID_ITEM_ID;
            }

            const uint32_t sampleLength = getTrackInfo(initSegmentId, segTrackId).samples.at(itemId.get()).dataLength;
            if (memoryBufferSize < sampleLength)
            {
                memoryBufferSize = sampleLength;
                return ErrorCode::MEMORY_TOO_SMALL_BUFFER;
            }

            seekInput(io, (std::int64_t) getTrackInfo(initSegmentId, segTrackId).samples.at(itemId.get()).dataOffset);
            io.stream->read(memoryBuffer, sampleLength);
            memoryBufferSize = sampleLength;

            if (!io.stream->good())
            {
                return ErrorCode::FILE_READ_ERROR;
            }
            break;
        }
        default:
            return ErrorCode::INVALID_CONTEXT_ID;
        }

        // Process bitstream by codec
        FourCC codeType;
        error = getDecoderCodeType(toTrackId(initSegTrackId), itemIdApi, codeType);
        if (error)
        {
            return error;
        }

        if (codeType == "avc1" || codeType == "avc3")
        {
            // Get item data from AVC bitstream
            if (videoByteStreamHeaders)
            {
                return processAvcItemData(memoryBuffer, memoryBufferSize);
            }
            else
            {
                return ErrorCode::OK;
            }
        }
        else if (codeType == "hvc1" || codeType == "hev1")
        {
            // Get item data from HEVC bitstream
            if (videoByteStreamHeaders)
            {
                return processHevcItemData(memoryBuffer, memoryBufferSize);
            }
            else
            {
                return ErrorCode::OK;
            }
        }
        // If the codeType is found to be of hvc2 type
        else if (codeType == "hvc2")
        {
            // Load the extrator NAL into memory buffer, which is copied into
            // extractorSampleBuffer for further processing
            Vector<std::uint8_t> extractorSampleBuffer(memoryBuffer, memoryBuffer + memoryBufferSize);

            std::uint8_t nalLengthSizeMinus1 = 3;
            ItemId sampleBase;
            auto& sampleInfo             = getSampleInfo(initSegmentId, segTrackId, sampleBase);
            SampleDescriptionIndex index = sampleInfo.at((ItemId(itemIdApi) - sampleBase).get()).sampleDescriptionIndex;
            if (getInitTrackInfo(initSegTrackId).nalLengthSizeMinus1.count(index.get()) != 0)
            {
                nalLengthSizeMinus1 = getInitTrackInfo(initSegTrackId).nalLengthSizeMinus1.at(index);
            }

            Hvc2Extractor::ExtNalDat extNalDat;
            uint64_t extractionSize = 0;
            uint64_t tolerance      = 0;

            // If the current NAL is affirmed to be an extractor NAL, parse it to extNalDat

            if (Hvc2Extractor::parseExtractorNal(extractorSampleBuffer, extNalDat, nalLengthSizeMinus1, extractionSize))
            {
                if (extractionSize > UINT32_MAX)
                {
                    // the size from extractors is not reliable. Make an estimate based on sample lengths of the
                    // referred tracks
                    extractionSize = 0;
                    for (std::vector<Hvc2Extractor::ExtNalDat::SampleConstruct>::iterator sampleConstruct =
                             extNalDat.sampleConstruct.begin();
                         sampleConstruct != extNalDat.sampleConstruct.end(); ++sampleConstruct)
                    {
                        uint64_t refSampleLength = 0;
                        uint64_t refDataOffset   = 0;
                        result =
                            getRefSampleDataInfo(trackId, itemIdApi, initSegmentId, (*sampleConstruct).track_ref_index,
                                                 refSampleLength, refDataOffset);
                        if (result != ErrorCode::OK)
                        {
                            return result;
                        }
                        extractionSize += refSampleLength;
                    }
                    // + add 10% tolerance (inline constructors can result in more data in the extracted samples than
                    // the original samples, but should be less than 10%)
                    tolerance = (uint64_t)(extractionSize / 10);
                    extractionSize += tolerance;
                }
                if (extractionSize > (uint64_t) spaceAvailable)
                {
                    // add an additional tolerance to requested size; this could save some useless realloc's in client
                    // side in the next runs (assuming the client reuses the buffers)
                    memoryBufferSize = (uint32_t)(extractionSize + tolerance);
                    return ErrorCode::MEMORY_TOO_SMALL_BUFFER;
                }

                // Extract bytes from the inline and sample constructs
                uint32_t extractedBytes            = 0;
                char* buffer                       = memoryBuffer;
                char* inlineNalLengthFieldPosition = nullptr;
                size_t inlineLength                = 0;
                std::vector<Hvc2Extractor::ExtNalDat::SampleConstruct>::iterator sampleConstruct;
                std::vector<Hvc2Extractor::ExtNalDat::InlineConstruct>::iterator inlineConstruct;

                // We loop through both constructors, until both of them are empty. They are often interleaved, but not
                // always through the whole sequence.
                for (sampleConstruct = extNalDat.sampleConstruct.begin(),
                    inlineConstruct  = extNalDat.inlineConstruct.begin();
                     sampleConstruct != extNalDat.sampleConstruct.end() ||
                     inlineConstruct != extNalDat.inlineConstruct.end();)
                {
                    if (inlineConstruct != extNalDat.inlineConstruct.end() &&
                        (sampleConstruct == extNalDat.sampleConstruct.end() ||
                         (*inlineConstruct).order_idx < (*sampleConstruct).order_idx))
                    {
                        // copy the inline part - note: std::copy with iterators give warning in Visual Studio, so the
                        // good old memcpy is used instead
                        memcpy(buffer, (*inlineConstruct).inline_data.data(), (*inlineConstruct).inline_data.size());
                        inlineLength                 = (*inlineConstruct).inline_data.size();
                        inlineNalLengthFieldPosition = buffer;  // in case we use the same extractor for several bitrate
                                                                // variants, the NAL size from inline construct is not
                                                                // valid if the size from extractor is not valid either
                        buffer += (*inlineConstruct).data_length;
                        extractedBytes += (*inlineConstruct).data_length;
                        ++inlineConstruct;
                    }
                    else if (sampleConstruct != extNalDat.sampleConstruct.end())
                    {
                        // read the sample from the referenced track

                        uint64_t refSampleLength = 0;
                        uint64_t refDataOffset   = 0;
                        result =
                            getRefSampleDataInfo(trackId, itemIdApi, initSegmentId, (*sampleConstruct).track_ref_index,
                                                 refSampleLength, refDataOffset);
                        if (result != ErrorCode::OK)
                        {
                            return result;
                        }
                        uint64_t dataOffset = refDataOffset + (*sampleConstruct).data_offset;
                        // Extract the referenced sample into memoryBuffer from io stream
                        uint64_t bytesToCopy = refSampleLength;
                        if ((*sampleConstruct).data_length == 0)
                        {
                            // bytes to copy is taken from the bitstream (length field referenced by data_offset)
                            seekInput(io, (std::int64_t) dataOffset);
                            io.stream->read(buffer, (nalLengthSizeMinus1 + 1));
                            bytesToCopy = 0;
                            size_t i    = 0;
                            if (nalLengthSizeMinus1 > 3)
                            {
                                bytesToCopy = (((uint64_t) buffer[0]) << 56) | (((uint64_t) buffer[1]) << 48) |
                                              (((uint64_t) buffer[2]) << 40) | (((uint64_t) buffer[3]) << 32);
                                i = 4;
                            }
                            bytesToCopy |= (((uint64_t) buffer[i]) << 24) | (((uint64_t) buffer[i + 1]) << 16) |
                                           (((uint64_t) buffer[i + 2]) << 8) | ((uint64_t) buffer[i + 3]);
                            buffer += (nalLengthSizeMinus1 + 1);
                            extractedBytes += (nalLengthSizeMinus1 + 1);
                            dataOffset = 0;  // we've seeked already
                        }
                        else if ((*sampleConstruct).data_offset + (*sampleConstruct).data_length > refSampleLength)
                        {
                            if ((*sampleConstruct).data_offset > refSampleLength)
                            {
                                // something is wrong, the offset and sample lengths do not match at all
                                return ErrorCode::INVALID_SEGMENT;
                            }
                            // clip the length of copied data block to the length of the actual sample
                            bytesToCopy = refSampleLength - (*sampleConstruct).data_offset;
                            if (inlineNalLengthFieldPosition != nullptr)
                            {
                                // need to rewrite the NAL length field as the value from inline constructor is no
                                // longer valid
                                uint64_t actualNalLength = bytesToCopy + inlineLength - (nalLengthSizeMinus1 + 1);
                                size_t i                 = 0;
                                if (nalLengthSizeMinus1 > 3)
                                {
                                    inlineNalLengthFieldPosition[i++] = (uint8_t)(actualNalLength >> 56);
                                    inlineNalLengthFieldPosition[i++] = (uint8_t)(actualNalLength >> 48);
                                    inlineNalLengthFieldPosition[i++] = (uint8_t)(actualNalLength >> 40);
                                    inlineNalLengthFieldPosition[i++] = (uint8_t)(actualNalLength >> 32);
                                }
                                inlineNalLengthFieldPosition[i++] = (uint8_t)(actualNalLength >> 24);
                                inlineNalLengthFieldPosition[i++] = (uint8_t)(actualNalLength >> 16);
                                inlineNalLengthFieldPosition[i++] = (uint8_t)(actualNalLength >> 8);
                                inlineNalLengthFieldPosition[i++] = (uint8_t)(actualNalLength);
                            }
                            else
                            {
                                // there was no inline constructor? (*sampleConstruct).data_offset should now point to
                                // the length field of the NAL to be copied (0 or some later NAL), so we may have the
                                // right size in the NAL length field, and it becomes the first bytes in resulting NAL
                                // too, right?
                            }
                        }
                        else
                        {
                            bytesToCopy = (*sampleConstruct).data_length;
                        }

                        if (extractedBytes + (uint32_t) bytesToCopy > spaceAvailable)
                        {
                            memoryBufferSize = extractedBytes + (uint32_t) bytesToCopy;
                            return ErrorCode::MEMORY_TOO_SMALL_BUFFER;
                        }
                        seekInput(io, (std::int64_t) dataOffset);
                        io.stream->read(buffer, bytesToCopy);
                        buffer += bytesToCopy;
                        extractedBytes += (uint32_t) bytesToCopy;
                        ++sampleConstruct;
                        inlineNalLengthFieldPosition = nullptr;
                        inlineLength                 = 0;
                    }
                }
                memoryBufferSize = extractedBytes;
                if (videoByteStreamHeaders)
                {
                    // Process the extracted NAL sample (insertion of start code + EPB)
                    return processHevcItemData(memoryBuffer, memoryBufferSize);
                }

                return ErrorCode::OK;
            }
            return ErrorCode::UNSUPPORTED_CODE_TYPE;  // hvc2 but unknown extractor?
        }
        else if ((codeType == "mp4a") || (codeType == "invo") || (codeType == "urim") || (codeType == "mp4v"))
        {
            // already valid data - do nothing.
            return ErrorCode::OK;
        }
        else
        {
            // Code type not supported
            return ErrorCode::UNSUPPORTED_CODE_TYPE;
        }
    }

    int32_t MP4VRFileReaderImpl::getTrackSampleOffset(uint32_t trackId,
                                                      uint32_t itemIdApi,
                                                      uint64_t& sampleOffset,
                                                      uint32_t& sampleLength)
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        SegmentId segmentId;
        int32_t result = segmentIdOf(initSegTrackId, itemIdApi, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);
        ItemId itemId             = ItemId(itemIdApi) - getTrackInfo(initSegmentId, segTrackId).itemIdBase;

        // read NAL data to bitstream object
        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }
        if (contextType == ContextType::TRACK)
        {
            // The requested frame should be one that is available
            if (itemId.get() >= getTrackInfo(initSegmentId, segTrackId).samples.size())
            {
                return ErrorCode::INVALID_ITEM_ID;
            }
            sampleLength = getTrackInfo(initSegmentId, segTrackId).samples.at(itemId.get()).dataLength;
            sampleOffset = getTrackInfo(initSegmentId, segTrackId).samples.at(itemId.get()).dataOffset;
        }
        else
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getDecoderConfiguration(uint32_t trackId,
                                                         uint32_t itemId,
                                                         DynArray<DecoderSpecificInfo>& decoderInfos) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);

        // Was it an image/sample?
        if (auto* parameterSetMap = getParameterSetMap(Id(initSegTrackId, itemId)))
        {
            decoderInfos = DynArray<DecoderSpecificInfo>(parameterSetMap->size());

            int i = 0;
            for (auto const& entry : *parameterSetMap)
            {
                DecoderSpecificInfo decSpecInfo;
                decSpecInfo.decSpecInfoType = entry.first;
                decSpecInfo.decSpecInfoData = makeDynArray<unsigned char>(entry.second);
                decoderInfos.elements[i++]  = decSpecInfo;
            }
            return ErrorCode::OK;
        }
        return ErrorCode::INVALID_ITEM_ID;  // or invalid context...?
    }


    int32_t MP4VRFileReaderImpl::getTrackTimestamps(uint32_t trackId, DynArray<TimestampIDPair>& timestamps) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        WriteOnceMap<Timestamp, ItemId> timestampMap;

        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        for (const auto& segment : mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap)
        {
            if (segment.second.initSegmentId == initSegTrackId.first)
            {
                SegmentId segmentId       = segment.first;
                SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);
                switch (contextType)
                {
                case ContextType::TRACK:
                {
                    if (hasTrackInfo(initSegmentId, segTrackId))
                    {
                        for (const auto& sampleInfo : getTrackInfo(initSegmentId, segTrackId).samples)
                        {
                            for (auto compositionTime : sampleInfo.compositionTimes)
                            {
                                timestampMap.insert(std::make_pair(compositionTime, sampleInfo.sampleId));
                            }
                        }
                    }
                    break;
                }

                case ContextType::META:
                {
                    if (mMetaBoxInfo.at(segTrackId).isForcedFpsSet == true)
                    {
                        for (const auto& imageInfo : mMetaBoxInfo.at(segTrackId).imageInfoMap)
                        {
                            if (imageInfo.second.type == "master")
                            {
                                timestampMap.insert(std::make_pair(static_cast<Timestamp>(imageInfo.second.displayTime),
                                                                   imageInfo.first));
                            }
                        }
                    }
                    else
                    {
                        return ErrorCode::INVALID_CONTEXT_ID;
                    }
                    break;
                }

                default:
                    return ErrorCode::INVALID_CONTEXT_ID;
                }
            }
        }

        timestamps = DynArray<TimestampIDPair>(timestampMap.size());
        uint32_t i = 0;
        for (auto const& entry : timestampMap)
        {
            TimestampIDPair pair;
            pair.timeStamp           = entry.first;
            pair.itemId              = entry.second.get();
            timestamps.elements[i++] = pair;
        }

        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getTimestampsOfSample(uint32_t trackId,
                                                       uint32_t itemIdApi,
                                                       DynArray<uint64_t>& timestamps) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        SegmentId segmentId;
        int32_t result = segmentIdOf(initSegTrackId, itemIdApi, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        ItemId itemId = ItemId(itemIdApi) -
                        getTrackInfo(initSegmentId, SegmentTrackId(segmentId, initSegTrackId.second)).itemIdBase;
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);

        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        switch (contextType)
        {
        case ContextType::TRACK:
        {
            const auto& displayTimes =
                getTrackInfo(initSegmentId, segTrackId).samples.at(itemId.get()).compositionTimes;
            timestamps = makeDynArray<uint64_t>(displayTimes);
            break;
        }

        case ContextType::META:
        {
            if (mMetaBoxInfo.at(segTrackId).isForcedFpsSet == true)
            {
                timestamps = DynArray<uint64_t>(1);
                timestamps.elements[0] =
                    static_cast<Timestamp>(mMetaBoxInfo.at(segTrackId).imageInfoMap.at(itemId.get()).displayTime);
            }
            else
            {
                return ErrorCode::INVALID_CONTEXT_ID;
            }
            break;
        }

        default:
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
        }
        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getSamplesInDecodingOrder(uint32_t trackId,
                                                           DynArray<TimestampIDPair>& itemDecodingOrder) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        DecodingOrderVector itemDecodingOrderVector;
        itemDecodingOrderVector.clear();

        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        for (const auto& segment : mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap)
        {
            if (segment.second.initSegmentId == initSegTrackId.first)
            {
                SegmentId segmentId       = segment.first;
                SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);
                switch (contextType)
                {
                case ContextType::TRACK:
                {
                    if (hasTrackInfo(initSegmentId, segTrackId))
                    {
                        const auto& samples = getTrackInfo(initSegmentId, segTrackId).samples;
                        itemDecodingOrderVector.reserve(samples.size());
                        for (const auto& sample : samples)
                        {
                            for (const auto compositionTime : sample.compositionTimes)
                            {
                                itemDecodingOrderVector.push_back(std::make_pair(sample.sampleId, compositionTime));
                            }
                        }

                        // Sort using composition times
                        std::sort(itemDecodingOrderVector.begin(), itemDecodingOrderVector.end(),
                                  [&](ItemIdTimestampPair a, ItemIdTimestampPair b) { return a.second < b.second; });
                    }
                    break;
                }
                case ContextType::META:
                {
#ifndef DISABLE_UNCOVERED_CODE
                    if (mMetaBoxInfo.at(segTrackId).isForcedFpsSet == true)
                    {
                        itemDecodingOrderVector.reserve(mMetaBoxInfo.at(segTrackId).imageInfoMap.size());
                        for (const auto& image : mMetaBoxInfo.at(segTrackId).imageInfoMap)
                        {
                            itemDecodingOrderVector.push_back(std::pair<std::uint32_t, Timestamp>(
                                image.first.get(), static_cast<Timestamp>(image.second.displayTime)));
                        }
                    }
                    else
                    {
                        return ErrorCode::INVALID_CONTEXT_ID;
                    }
                    break;
#endif  // DISABLE_UNCOVERED_CODE
                }
                default:
                {
                    return ErrorCode::INVALID_CONTEXT_ID;
                }
                }
            }
        }

        itemDecodingOrder = DynArray<TimestampIDPair>(itemDecodingOrderVector.size());
        uint32_t i        = 0;
        for (auto const& entry : itemDecodingOrderVector)
        {
            TimestampIDPair pair;
            pair.timeStamp                  = entry.second;
            pair.itemId                     = entry.first.get();
            itemDecodingOrder.elements[i++] = pair;
        }
        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::getSyncSampleId(uint32_t trackId,
                                                 uint32_t sampleId,
                                                 SeekDirection direction,
                                                 uint32_t& syncSampleId) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        SegmentId segmentId;
        int32_t result = segmentIdOf(initSegTrackId, sampleId, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);
        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        switch (contextType)
        {
        case ContextType::TRACK:
        {
            break;
        }
        case ContextType::META:
        default:
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
        }

        ItemId sampleBase;
        auto* sampleInfo =
            hasTrackInfo(initSegmentId, segTrackId) ? &getSampleInfo(initSegmentId, segTrackId, sampleBase) : nullptr;
        if (!sampleInfo || (ItemId(sampleId) - sampleBase).get() >= sampleInfo->size())
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        syncSampleId = 0xffffffff;

        // note: the index can go negative when seeking backwards
        int32_t index = int32_t((ItemId(sampleId) - sampleBase).get());

        auto& segmentProperties = mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segmentId);
        auto& sequenceToSegment = mInitSegmentPropertiesMap.at(segmentProperties.initSegmentId).sequenceToSegment;
        auto sequenceIterator   = sequenceToSegment.find(*segmentProperties.sequences.begin());

        auto checkIndexRange = [&]() {
            bool hasSamples = sampleInfo && (index >= 0 && index < int32_t(sampleInfo->size()));
            if (direction == SeekDirection::NEXT)
            {
                while (!hasSamples && sequenceIterator != sequenceToSegment.end())
                {
                    sequenceIterator = sequenceToSegment.find(*mInitSegmentPropertiesMap.at(initSegmentId)
                                                                   .segmentPropertiesMap.at(sequenceIterator->second)
                                                                   .sequences.rbegin());
                    ++sequenceIterator;

                    if (sequenceIterator != sequenceToSegment.end())
                    {
                        segTrackId = SegmentTrackId(sequenceIterator->second, initSegTrackId.second);
                        sampleInfo = hasTrackInfo(initSegmentId, segTrackId)
                                         ? &getSampleInfo(initSegmentId, segTrackId, sampleBase)
                                         : nullptr;
                        index      = 0;
                        hasSamples = sampleInfo && (index >= 0 && index < int32_t(sampleInfo->size()));
                    }
                }
            }
            else if (direction == SeekDirection::PREVIOUS)
            {
                while (!hasSamples && sequenceIterator != sequenceToSegment.begin())
                {
                    sequenceIterator = sequenceToSegment.find(*mInitSegmentPropertiesMap.at(initSegmentId)
                                                                   .segmentPropertiesMap.at(sequenceIterator->second)
                                                                   .sequences.begin());
                    --sequenceIterator;
                    if (sequenceIterator != sequenceToSegment.begin())
                    {
                        segTrackId = SegmentTrackId(sequenceIterator->second, initSegTrackId.second);
                        sampleInfo = hasTrackInfo(initSegmentId, segTrackId)
                                         ? &getSampleInfo(initSegmentId, segTrackId, sampleBase)
                                         : nullptr;
                        if (sampleInfo != nullptr)
                        {
                            index      = int32_t(sampleInfo->size()) - 1;
                            hasSamples = sampleInfo && (index >= 0 && index < int32_t(sampleInfo->size()));
                        }
                        else
                        {
                            hasSamples = false;
                        }
                    }
                }
            }
            return hasSamples;
        };

        // If going forward and the input sampleId refers to a sync
        // sample, go one forward.
        if (direction == SeekDirection::NEXT && (index >= 0 && index < int32_t(sampleInfo->size())) &&
            (*sampleInfo)[uint32_t(index)].sampleType == SampleType::OUTPUT_REFERENCE_FRAME)
        {
            ++index;
        }
        bool hasSamplesInSegment = checkIndexRange();

        while (syncSampleId == 0xffffffff && hasSamplesInSegment)
        {
            assert(sampleInfo);
            if ((*sampleInfo)[uint32_t(index)].sampleType == SampleType::OUTPUT_REFERENCE_FRAME)
            {
                syncSampleId = (ItemId(uint32_t(index)) + sampleBase).get();
            }
            else
            {
                if (direction == SeekDirection::PREVIOUS)
                {
                    --index;
                }
                else
                {
                    ++index;
                }

                hasSamplesInSegment = checkIndexRange();
            }
        }
        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::getDecoderCodeType(uint32_t trackId, uint32_t itemId, FourCC& decoderCodeType) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        SegmentId segmentId;
        int32_t result = segmentIdOf(initSegTrackId, itemId, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);
        const auto segmentPropertiesIt =
            mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.find(segmentId);
        if (segmentPropertiesIt == mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.end())
        {
            return ErrorCode::INVALID_ITEM_ID;
        }
        const auto& segmentProperties = segmentPropertiesIt->second;
        const auto trackInfoIt        = segmentProperties.trackInfos.find(segTrackId.second);
        if (trackInfoIt == segmentProperties.trackInfos.end())
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
        const auto& trackInfo          = trackInfoIt->second;
        const auto& decoderCodeTypeMap = trackInfo.decoderCodeTypeMap;

        auto iter = decoderCodeTypeMap.find(ItemId(itemId));
        if (iter != decoderCodeTypeMap.end())
        {
            decoderCodeType = iter->second.getUInt32();
            return ErrorCode::OK;
        }

        // Was it an image/sample?
        const auto parameterSetIdIt = segmentProperties.itemToParameterSetMap.find(
            Id(initSegTrackId, ItemId(itemId) - getTrackInfo(initSegmentId, segTrackId).itemIdBase));
        if (parameterSetIdIt == segmentProperties.itemToParameterSetMap.end())
        {
            return ErrorCode::INVALID_ITEM_ID;
        }
        const SampleDescriptionIndex parameterSetId = parameterSetIdIt->second;
        // Assume it's in the same segment.. Does this happen?
        iter = decoderCodeTypeMap.find(ItemId(parameterSetId.get()));
        if (iter != decoderCodeTypeMap.end())
        {
            decoderCodeType = iter->second.getUInt32();
            return ErrorCode::OK;
        }
        return ErrorCode::INVALID_ITEM_ID;  // or invalid context...?
    }


    int32_t MP4VRFileReaderImpl::getSampleDuration(uint32_t trackId, uint32_t sampleId, uint32_t& sampleDuration) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        SegmentId segmentId;
        int32_t result = segmentIdOf(initSegTrackId, sampleId, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);

        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        switch (contextType)
        {
        case ContextType::TRACK:
        {
            break;
        }
        case ContextType::META:
        default:
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }
        }

        ItemId itemId = ItemId(sampleId) - getTrackInfo(initSegmentId, segTrackId).itemIdBase;

        sampleDuration =
            (uint32_t)(uint64_t(getTrackInfo(initSegmentId, segTrackId).samples.at(itemId.get()).sampleDuration) *
                       1000) /
            getInitTrackInfo(initSegTrackId).timeScale;
        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::getPropertyChnl(uint32_t trackId, uint32_t sampleId, chnlProperty& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.chnlProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp.streamStructure    = propIn.streamStructure;
            aProp.definedLayout      = propIn.definedLayout;
            aProp.omittedChannelsMap = propIn.omittedChannelsMap;
            aProp.objectCount        = propIn.objectCount;
            aProp.channelCount       = propIn.channelCount;
            aProp.channelLayouts     = makeDynArray<ChannelLayout>(propIn.channelLayouts);
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertySpatialAudio(uint32_t trackId,
                                                         uint32_t sampleId,
                                                         SpatialAudioProperty& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.sa3dProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp.version                  = propIn.version;
            aProp.ambisonicType            = propIn.ambisonicType;
            aProp.ambisonicOrder           = propIn.ambisonicOrder;
            aProp.ambisonicChannelOrdering = propIn.ambisonicChannelOrdering;
            aProp.ambisonicNormalization   = propIn.ambisonicNormalization;
            aProp.channelMap               = makeDynArray<uint32_t>(propIn.channelMap);
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertyStereoScopic3D(uint32_t trackId,
                                                           uint32_t sampleId,
                                                           StereoScopic3DProperty& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.st3dProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp = propIn;
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertySphericalVideoV1(uint32_t trackId,
                                                             uint32_t sampleId,
                                                             SphericalVideoV1Property& sphericalproperty) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.googleSphericalV1Properties, index, result);

        if (result == ErrorCode::OK)
        {
            sphericalproperty = propIn;
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertySphericalVideoV2(uint32_t trackId,
                                                             uint32_t sampleId,
                                                             SphericalVideoV2Property& sphericalproperty) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.sv3dProperties, index, result);

        if (result == ErrorCode::OK)
        {
            sphericalproperty = propIn;
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertyRegionWisePacking(uint32_t trackId,
                                                              uint32_t sampleId,
                                                              RegionWisePackingProperty& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.rwpkProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp.constituentPictureMatchingFlag = propIn.constituentPictureMatchingFlag;
            aProp.packedPictureHeight            = propIn.packedPictureHeight;
            aProp.packedPictureWidth             = propIn.packedPictureWidth;
            aProp.projPictureHeight              = propIn.projPictureHeight;
            aProp.projPictureWidth               = propIn.projPictureWidth;
            aProp.regions                        = makeDynArray<RegionWisePackingRegion>(propIn.regions);
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertyCoverageInformation(uint32_t trackId,
                                                                uint32_t sampleId,
                                                                CoverageInformationProperty& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.coviProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp.coverageShapeType   = propIn.coverageShapeType;
            aProp.defaultViewIdc      = (ViewIdc) propIn.defaultViewIdc;
            aProp.viewIdcPresenceFlag = propIn.viewIdcPresenceFlag;
            aProp.sphereRegions       = makeDynArray<CoverageSphereRegion>(propIn.sphereRegions);
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertyProjectionFormat(uint32_t trackId,
                                                             uint32_t sampleId,
                                                             ProjectionFormatProperty& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.pfrmProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp = propIn;
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertyStereoVideoConfiguration(uint32_t trackId,
                                                                     uint32_t sampleId,
                                                                     PodvStereoVideoConfiguration& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.stviProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp = propIn;
        }

        return result;
    }

    int32_t MP4VRFileReaderImpl::getPropertyRotation(uint32_t trackId, uint32_t sampleId, Rotation& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.rotnProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp = propIn;
        }

        return result;
    }


    int32_t MP4VRFileReaderImpl::getPropertySchemeTypes(uint32_t trackId,
                                                        uint32_t sampleId,
                                                        SchemeTypesProperty& aProp) const
    {
        InitTrackInfo initTrackInfo;
        SampleDescriptionIndex index;
        uint32_t result = lookupTrackInfo(trackId, sampleId, initTrackInfo, index);
        auto propIn     = fetchSampleProp(initTrackInfo.schemeTypesProperties, index, result);

        if (result == ErrorCode::OK)
        {
            aProp.mainScheme.type    = propIn.mainScheme.getSchemeType().getUInt32();
            aProp.mainScheme.version = propIn.mainScheme.getSchemeVersion();
            aProp.mainScheme.uri     = makeDynArray<char>(propIn.mainScheme.getSchemeUri());

            Vector<SchemeType> compatibleTypes;
            for (auto& compatibleScheme : propIn.compatibleSchemes)
            {
                SchemeType compType{};
                compType.type    = compatibleScheme.getSchemeType().getUInt32();
                compType.version = compatibleScheme.getSchemeVersion();
                compType.uri     = makeDynArray<char>(compatibleScheme.getSchemeUri());
                compatibleTypes.push_back(compType);
            }
            aProp.compatibleSchemeTypes = makeDynArray<SchemeType>(compatibleTypes);
        }

        return result;
    }

    uint32_t MP4VRFileReaderImpl::lookupTrackInfo(uint32_t trackId,
                                                  uint32_t sampleId,
                                                  InitTrackInfo& initTrackInfo,
                                                  SampleDescriptionIndex& index) const
    {
        if (isInitializedError())
        {
            return ErrorCode::UNINITIALIZED;
        }

        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        SegmentId segmentId;
        int32_t result = segmentIdOf(initSegTrackId, sampleId, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);

        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        if (contextType == ContextType::TRACK)
        {
            ItemId sampleBase;
            auto& sampleInfo = getSampleInfo(initSegmentId, segTrackId, sampleBase);
            index            = sampleInfo.at((ItemId(sampleId) - sampleBase).get()).sampleDescriptionIndex;
            initTrackInfo    = getInitTrackInfo(initSegTrackId);
            return ErrorCode::OK;
        }

        return ErrorCode::INVALID_CONTEXT_ID;
    }


}  // namespace MP4VR
