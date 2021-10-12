
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
#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>

#include <reader/mp4vrfilereaderinterface.h>
#include "mp4vrfilestreamfile.h"

#include "common/utils.h"
#include "mp4loader.h"
#include "omaf/parser/h265parser.hpp"

#include "mp4vrfilestreamgeneric.h"

namespace VDD
{
    namespace
    {
        struct MP4VRDeleter
        {
            void operator()(MP4VR::MP4VRFileReaderInterface* aReader)
            {
                MP4VR::MP4VRFileReaderInterface::Destroy(aReader);
            }
        };

        enum MP4LoaderErrors
        {
            UnsupportedVRMetadataVersion =
                MP4VR::MP4VRFileReaderInterface::ALLOCATOR_ALREADY_SET + 1,
            OMAFNonVIInput
        };

        const std::pair<int32_t, const char*> kMp4vrErrors[] = {
            {MP4VR::MP4VRFileReaderInterface::OK, "OK"},
            {MP4VR::MP4VRFileReaderInterface::FILE_OPEN_ERROR, "FILE_OPEN_ERROR"},
            {MP4VR::MP4VRFileReaderInterface::FILE_HEADER_ERROR, "FILE_HEADER_ERROR"},
            {MP4VR::MP4VRFileReaderInterface::FILE_READ_ERROR, "FILE_READ_ERROR"},
            {MP4VR::MP4VRFileReaderInterface::UNSUPPORTED_CODE_TYPE, "UNSUPPORTED_CODE_TYPE"},
            {MP4VR::MP4VRFileReaderInterface::INVALID_FUNCTION_PARAMETER,
             "INVALID_FUNCTION_PARAMETER"},
            {MP4VR::MP4VRFileReaderInterface::INVALID_CONTEXT_ID, "INVALID_CONTEXT_ID"},
            {MP4VR::MP4VRFileReaderInterface::INVALID_ITEM_ID, "INVALID_ITEM_ID"},
            {MP4VR::MP4VRFileReaderInterface::INVALID_PROPERTY_INDEX, "INVALID_PROPERTY_INDEX"},
            {MP4VR::MP4VRFileReaderInterface::INVALID_SAMPLE_DESCRIPTION_INDEX,
             "INVALID_SAMPLE_DESCRIPTION_INDEX"},
            {MP4VR::MP4VRFileReaderInterface::PROTECTED_ITEM, "PROTECTED_ITEM"},
            {MP4VR::MP4VRFileReaderInterface::UNPROTECTED_ITEM, "UNPROTECTED_ITEM"},
            {MP4VR::MP4VRFileReaderInterface::UNINITIALIZED, "UNINITIALIZED"},
            {MP4VR::MP4VRFileReaderInterface::NOT_APPLICABLE, "NOT_APPLICABLE"},
            {MP4VR::MP4VRFileReaderInterface::MEMORY_TOO_SMALL_BUFFER, "MEMORY_TOO_SMALL_BUFFER"},
            {MP4VR::MP4VRFileReaderInterface::INVALID_SEGMENT, "INVALID_SEGMENT"},
            {MP4VR::MP4VRFileReaderInterface::ALLOCATOR_ALREADY_SET, "ALLOCATOR_ALREADY_SET"},
            {UnsupportedVRMetadataVersion, "Unsupported VR metadata version"},
            {OMAFNonVIInput, "Viewport dependent input detected, but not accepted"}};

        std::string findMP4LoaderErrorMessage(int rc)
        {
            std::string error = "Unknown error";
            for (auto msg : kMp4vrErrors)
            {
                if (msg.first == rc)
                {
                    error = msg.second;
                    break;
                }
            }
            return error;
        }

        void checkAndThrow(int32_t aRc)
        {
            if (aRc != MP4VR::MP4VRFileReaderInterface::OK)
            {
                throw MP4LoaderError(aRc);
            }
        }

        /** @brief getSampleDurations returns an array that can be indexed by the same index as the
         * aTrackInformation.sampleProperties array and returns, for each sample, its duration. */
        std::vector<FrameDuration> getSampleDurations(
            const MP4VR::TrackInformation& aTrackInformation)
        {
            std::vector<FrameDuration> sampleDurations(aTrackInformation.sampleProperties.numElements);

            std::vector<std::pair<size_t, const MP4VR::SampleInformation*>> sampleInfo(
                aTrackInformation.sampleProperties.numElements);
            for (size_t index = 0; index < aTrackInformation.sampleProperties.numElements; ++index)
            {
                sampleDurations[index] = {
                    aTrackInformation.sampleProperties[index].sampleDurationTS,
                    aTrackInformation.timeScale};
            }

            return sampleDurations;
        }

        std::map<ConfigType, std::vector<std::uint8_t>> makeDecoderConfig(
            MP4VR::DynArray<MP4VR::DecoderSpecificInfo> aDecoderInfos)
        {
            std::map<ConfigType, std::vector<std::uint8_t>> decoderConfig;
            for (auto& decInfo : aDecoderInfos)
            {
                ConfigType type;
                switch (decInfo.decSpecInfoType)
                {
                case MP4VR::AVC_SPS:
                    type = ConfigType::SPS;
                    break;
                case MP4VR::AVC_PPS:
                    type = ConfigType::PPS;
                    break;
                case MP4VR::HEVC_SPS:
                    type = ConfigType::SPS;
                    break;
                case MP4VR::HEVC_PPS:
                    type = ConfigType::PPS;
                    break;
                case MP4VR::HEVC_VPS:
                    type = ConfigType::VPS;
                    break;
                case MP4VR::AudioSpecificConfig:
                    type = ConfigType::AudioSpecificConfig;
                    break;
                default:
                    assert(((void)"Unknown decoder specific info type", false));
                }

                decoderConfig[type] = {decInfo.decSpecInfoData.begin(),
                                       decInfo.decSpecInfoData.end()};
            }
            return decoderConfig;
        }

        CodedFormat formatOfTrack(MP4VR::MP4VRFileReaderInterface& aReader,
                                  const MP4VR::TrackInformation& aTrack)
        {
            MP4VR::FourCC decoderCodeType;
            if (aTrack.sampleProperties.numElements)
            {
                auto firstSampleId = aTrack.sampleProperties[0].sampleId;
                int32_t rc =
                    aReader.getDecoderCodeType(aTrack.trackId, firstSampleId, decoderCodeType);
                checkAndThrow(rc);

                return decoderCodeType == "avc1"
                           ? CodedFormat::H264
                           : decoderCodeType == "avc3"
                                 ? CodedFormat::H264
                                 : decoderCodeType == "hev1"
                                       ? CodedFormat::H265
                                       : decoderCodeType == "hvc1"
                                             ? CodedFormat::H265
                                             : decoderCodeType == "hvc2"
                                                   ? CodedFormat::H265Extractor
                                                   : decoderCodeType == "mp4a" ? CodedFormat::AAC
                                                                               : CodedFormat::None;
            }
            else
            {
                return CodedFormat::None;
            }
        }

        // calculates the average bitrate and the sliding window maximum bitrate for a track
        Bitrate determineBitrate(MP4VR::MP4VRFileReaderInterface& aReader,
                                 MP4VR::TrackInformation& aTrack,
                                 FrameDuration aMaxBitrateWindowSize,
                                 Optional<FrameDuration> aOverrideFrameDuration)
        {
            Bitrate bitrate {};
            uint64_t totalBytes {};
            FrameDuration totalDuration {};
            std::list<std::pair<uint32_t, FrameDuration>> window;
            std::uint32_t windowBytes {};
            FrameDuration windowDuration;

            std::vector<FrameDuration> durations;
            if (!aOverrideFrameDuration)
            {
                durations = getSampleDurations(aTrack);
            }

            for (size_t sampleIndex = 0; sampleIndex < aTrack.sampleProperties.numElements; ++sampleIndex)
            {
                auto& sample = aTrack.sampleProperties[sampleIndex];
                uint32_t sampleSize = 0;
                int32_t rc = aReader.getTrackSampleData(aTrack.trackId, sample.sampleId, nullptr, sampleSize);
                if (rc != MP4VR::MP4VRFileReaderInterface::MEMORY_TOO_SMALL_BUFFER &&
                    rc != MP4VR::MP4VRFileReaderInterface::OK)
                {
                    throw MP4LoaderError(rc);
                }
                auto duration = aOverrideFrameDuration ? *aOverrideFrameDuration : durations[sampleIndex];
                window.push_back({ sampleSize, duration });
                windowBytes += sampleSize;
                windowDuration += duration;
                if (windowDuration > aMaxBitrateWindowSize)
                {
                    windowBytes -= window.front().first;
                    windowDuration -= window.front().second;
                    window.pop_front();
                    if (windowDuration > FrameDuration { 0, 1 })
                    {
                        auto curBitrate = static_cast<std::uint32_t>((FrameDuration { 8 * windowBytes, 1 } / windowDuration).asDouble());
                        bitrate.maxBitrate = std::max(bitrate.maxBitrate, curBitrate);
                    }
                }
                totalBytes += sampleSize;
                totalDuration += duration;
            }
            if (windowDuration > FrameDuration { 0, 1 })
            {
                auto curBitrate = static_cast<std::uint32_t>((FrameDuration { 8 * windowBytes, 1 } / windowDuration).asDouble());
                bitrate.maxBitrate = std::max(bitrate.maxBitrate, curBitrate);
            }
            if (totalDuration > FrameDuration { 0, 1 })
            {
                bitrate.avgBitrate = static_cast<std::uint32_t>((FrameDuration { 8 * totalBytes, 1 } / totalDuration).asDouble());
            }
            return bitrate;
        }
    } // anonymous namespace

    MP4LoaderError::MP4LoaderError(int32_t aCode, Optional<std::string> aFilename)
        : MediaLoaderException("MP4LoaderError")
        , mCode(aCode)
        , mFilename(aFilename)
    {
        // nothing
    }

    std::string MP4LoaderError::message() const
    {
        std::string error = findMP4LoaderErrorMessage(mCode);

        auto message = "Error while processing MP4: " + error;
        if (mFilename)
        {
            message += " (" + *mFilename + ")";
        }
        return message;
    }

    int32_t MP4LoaderError::getCode() const
    {
        return mCode;
    }

    MP4LoaderError::~MP4LoaderError() = default;


    struct MP4LoaderFrameInfo
    {
        CodedFrameMeta frameMeta;
    };

    namespace {
        FrameDuration getTrackDuration(const MP4VR::TrackInformation& aTrackInfo)
        {
            if (aTrackInfo.sampleProperties.numElements) {
                auto& ss = aTrackInfo.sampleProperties;
                auto& first = ss[0];
                auto& last = ss[ss.numElements - 1];
                return FrameDuration(
                    last.earliestTimestampTS + last.sampleDurationTS - first.earliestTimestampTS,
                    aTrackInfo.timeScale);
            }
            else
            {
                return FrameDuration();
            }
        }
    }

    MP4LoaderSource::MP4LoaderSource(std::shared_ptr<MP4VR::MP4VRFileReaderInterface> aReader,
                                     TrackId aTrackId,
                                     const MP4VR::TrackInformation& aTrackInfo,
                                     std::list<MP4LoaderFrameInfo>&& aFrames,
                                     bool aVideoNalStartCodes,
                                     size_t aFrameLimit,
                                     std::shared_ptr<MP4LoaderSegmentKeeper> aSegmentKeeper)
        : mSegmentKeeper(aSegmentKeeper)
        , mReader(aReader)
        , mTrackId(aTrackId)
        , mTrackInfo(new MP4VR::TrackInformation(aTrackInfo))
        , mFrames(aFrames)
        , mFrameIterator(mFrames.begin())
        , mVideoNalStartCodes(aVideoNalStartCodes)
        , mFrameLimit(aFrameLimit)
        , mDuration(getTrackDuration(aTrackInfo))
    {
        // nothing
    }

    MP4LoaderSource::~MP4LoaderSource() = default;

    CodedFormat MP4LoaderSource::getFormat() const
    {
        return formatOfTrack(*mReader, *mTrackInfo);
    }

    FrameDuration MP4LoaderSource::getCurrentFrameDuration() const
    {
        if (mFrameIterator != mFrames.end())
        {
            return mFrameIterator->frameMeta.duration;
        }
        else
        {
            return {};
        }
    }

    std::list<TrackId> MP4LoaderSource::getTrackReferences(const std::string& aFourCC) const
    {
        std::list<TrackId> tracks;
        for (const MP4VR::TypeToTrackIDs& typeAndTrack : mTrackInfo->referenceTrackIds)
        {
            if (typeAndTrack.type == MP4VR::FourCC(aFourCC.c_str()))
            {
                std::copy(typeAndTrack.trackIds.begin(), typeAndTrack.trackIds.end(), std::back_inserter(tracks));
            }
        }
        return tracks;
    }

    namespace
    {
        template <typename T>
        Optional<T> getVrProperty(MP4VR::MP4VRFileReaderInterface& aReader,
                                  MP4VR::FeatureBitMask aFeature,
                                  int32_t (MP4VR::MP4VRFileReaderInterface::*aProperty)(uint32_t trackId, uint32_t sampleId, T& property) const,
                                  const MP4VR::TrackInformation& aTrackInfo)
        {
            if ((aTrackInfo.vrFeatures & aFeature) != 0 && aTrackInfo.sampleProperties.numElements)
            {
                auto firstSampleId = aTrackInfo.sampleProperties[0].sampleId;

                T property;
                int32_t rc = (aReader.*aProperty)(aTrackInfo.trackId, firstSampleId, property);
                if (rc != MP4VR::MP4VRFileReaderInterface::OK)
                {
                    std::cerr << "Failed to read VR property from input file ("
                              << findMP4LoaderErrorMessage(rc) << "); ignored" << std::endl;
                    return {};
                }
                else
                {
                    return property;
                }
            }
            else
            {
                return {};
            }
        }
    } // anonymous namespace

    MP4VR::ProjectionFormatProperty MP4LoaderSource::getProjection() const
    {
        MP4VR::ProjectionFormatProperty projectionFormat;
        int32_t rc = mReader->getPropertyProjectionFormat(mTrackInfo->trackId, mTrackInfo->sampleProperties[0].sampleId, projectionFormat);
        checkAndThrow(rc);
        return projectionFormat;
    }

    MP4VR::PodvStereoVideoConfiguration MP4LoaderSource::getFramePacking() const
    {
        MP4VR::PodvStereoVideoConfiguration stereoMode;
        int32_t rc = mReader->getPropertyStereoVideoConfiguration(mTrackInfo->trackId, 0, stereoMode);
        checkAndThrow(rc);
        return stereoMode;
    }

    Optional<MP4VR::StereoScopic3DProperty> MP4LoaderSource::getStereoScopic3D() const
    {
        return getVrProperty(*mReader,
                             MP4VR::TrackFeatureEnum::HasVRGoogleStereoscopic3D,
                             &MP4VR::MP4VRFileReaderInterface::getPropertyStereoScopic3D,
                             *mTrackInfo);
    }

    Optional<MP4VR::SphericalVideoV1Property> MP4LoaderSource::getSphericalVideoV1() const
    {
        return getVrProperty(*mReader,
                             MP4VR::TrackFeatureEnum::HasVRGoogleV1SpericalVideo,
                             &MP4VR::MP4VRFileReaderInterface::getPropertySphericalVideoV1,
                             *mTrackInfo);
    }

    Optional<MP4VR::SphericalVideoV2Property> MP4LoaderSource::getSphericalVideoV2() const
    {
        return getVrProperty(*mReader,
                             MP4VR::TrackFeatureEnum::HasVRGoogleV2SpericalVideo,
                             &MP4VR::MP4VRFileReaderInterface::getPropertySphericalVideoV2,
                             *mTrackInfo);
    }

    FrameRate MP4LoaderSource::getFrameRate() const
    {
        auto rat = mTrackInfo->frameRate;
        return { rat.num, rat.den };
    }

    FrameDuration MP4LoaderSource::getTimeScale() const
    {
        return { 1, mTrackInfo->timeScale };
    }

    Dimensions MP4LoaderSource::getDimensions() const
    {
        uint32_t width;
        uint32_t height;
        checkAndThrow(mReader->getDisplayWidth(mTrackInfo->trackId, width));
        checkAndThrow(mReader->getDisplayHeight(mTrackInfo->trackId, height));
        return { width, height };
    }

    int MP4LoaderSource::getGOPLength(bool& aIsFixed) const
    {
        std::vector<size_t> idrLocations;
        for (size_t i = 0; i < mTrackInfo->sampleProperties.numElements; i++)
        {
            if (mTrackInfo->sampleProperties[i].sampleType == MP4VR::OUTPUT_REFERENCE_FRAME)
            {
                idrLocations.push_back(i);
            }
        }
        int length = 0;
        aIsFixed = true;
        for (std::vector<size_t>::iterator it = idrLocations.begin()+1; it != idrLocations.end(); ++it)
        {
            int l = *it - *(it - 1);
            if (length == 0)
            {
                length = l;
            }
            else if (length != l)
            {
                // raise a flag, since this can lead to invalid DASH segments? Not critical for mp4 output
                aIsFixed = false;
            }
        }
        if (idrLocations.size() == 1)
        {
            aIsFixed = false; // meaning that can't really divide to GOPs
            return mTrackInfo->sampleProperties.numElements;
        }
        return length;
    }

    bool MP4LoaderSource::getTiles(size_t& aTilesX, size_t& aTilesY)
    {
        H265::PictureParameterSet pps;
        std::vector<uint8_t> nalUnitRBSP;
        H265Parser::convertToRBSP(mFrames.begin()->frameMeta.decoderConfig[ConfigType::PPS], nalUnitRBSP, true);
        Parser::BitStream bitstr(nalUnitRBSP);
        H265::NalUnitHeader naluHeader;
        H265Parser::parseNalUnitHeader(bitstr, naluHeader);

        H265Parser::parsePPS(bitstr, pps);
        if (pps.mTilesEnabledFlag)
        {
            aTilesX = static_cast<size_t>(pps.mNumTileColumnsMinus1) + 1;
            aTilesY = static_cast<size_t>(pps.mNumTileRowsMinus1) + 1;
            return true;
        }
        else
        {
            return false;
        }
    }

    void MP4LoaderSource::getCtuSize(int& aCtuSize)
    {
        H265::SequenceParameterSet sps;
        std::vector<uint8_t> nalUnitRBSP;
        H265Parser::convertToRBSP(mFrames.begin()->frameMeta.decoderConfig[ConfigType::SPS], nalUnitRBSP, true);
        Parser::BitStream bitstr(nalUnitRBSP);
        H265::NalUnitHeader naluHeader;
        H265Parser::parseNalUnitHeader(bitstr, naluHeader);

        H265Parser::parseSPS(bitstr, sps);

        aCtuSize = (int)pow(2, (sps.mLog2MinLumaCodingBlockSizeMinus3 + 3) + (sps.mLog2DiffMaxMinLumaCodingBlockSize));
    }

    Optional<size_t> MP4LoaderSource::getNumberOfFrames() const
    {
        return mFrames.size();
    }

    Optional<FrameDuration> MP4LoaderSource::getDuration() const
    {
        return mDuration;
    }

    std::vector<Streams> MP4LoaderSource::produce()
    {
        if (isAborted() || mFrameIterator == mFrames.end() || (mFrameLimit != ~0u && mFrameCounter >= mFrameLimit))
        {
            return {{ Data(EndOfStream()) }};
        }

        using namespace StreamSegmenter;
        std::vector<Streams> frames;

        const MP4LoaderFrameInfo& frameInfo(*(mFrameIterator));

        uint32_t bufferSize = 0;
        int32_t rc = mReader->getTrackSampleData(frameInfo.frameMeta.trackId.get(),
                                                 static_cast<uint32_t>(frameInfo.frameMeta.codingIndex),
                                                 nullptr, bufferSize, mVideoNalStartCodes);
        assert(rc == MP4VR::MP4VRFileReaderInterface::MEMORY_TOO_SMALL_BUFFER);

        std::vector<std::uint8_t> frameData(bufferSize);
        rc = mReader->getTrackSampleData(frameInfo.frameMeta.trackId.get(), static_cast<uint32_t>(frameInfo.frameMeta.codingIndex),
                                         reinterpret_cast<char*>(&frameData[0]), bufferSize, mVideoNalStartCodes);
        checkAndThrow(rc);

        CPUDataVector dataReference { { std::move(frameData) } };

        Data data(std::move(dataReference), frameInfo.frameMeta);
        frames.push_back({ std::move(data) });
        ++mFrameIterator;
        ++mFrameCounter;

        return frames;
    }

    MP4Loader::MP4Loader(MP4Loader::Config aConfig, bool aVideoNalStartCodes)
        : mReader(MP4VR::MP4VRFileReaderInterface::Create(), MP4VRDeleter())
        , mVideoNalStartCodes(aVideoNalStartCodes)
    {
        assert(mReader.get());

        if (!aConfig.initFilename)
        {
            int32_t rc = mReader->initialize(aConfig.filename.c_str());
            checkAndThrow(rc);
        }
        else
        {
            mSegmentKeeper.reset(new MP4LoaderSegmentKeeper);
            mSegmentKeeper->initSegment = openFile(aConfig.initFilename->c_str());
            if (mSegmentKeeper->initSegment)
            {
                uint32_t segmentId = 1;
                uint32_t initSegmentId;
                {
                    initSegmentId = segmentId;
                    int32_t rc = mReader->parseInitializationSegment(mSegmentKeeper->initSegment.get(), segmentId);
                    checkAndThrow(rc);
                    ++segmentId;
                }

                unsigned segmentNumber = aConfig.startNumber ? *aConfig.startNumber : 1u;
                bool keepIterating = true;
                while (keepIterating)
                {
                    std::string filename = Utils::replace(aConfig.filename, "$Number$", std::to_string(segmentNumber));
                    auto segment = openFile(filename.c_str());
                    if (segment)
                    {
                        {
                            int32_t rc = mReader->parseSegment(segment.get(), initSegmentId, segmentId);
                            checkAndThrow(rc);
                            ++segmentId;
                        }

                        mSegmentKeeper->mediaSegments.push_back(std::move(segment));
                        ++segmentNumber;
                    }
                    else
                    {
                        keepIterating = false;
                    }
                }

                if (mSegmentKeeper->mediaSegments.size() == 0)
                {
                    throw MP4LoaderError(MP4VR::MP4VRFileReaderInterface::FILE_OPEN_ERROR, aConfig.filename);
                }
            }
            else
            {
                throw MP4LoaderError(MP4VR::MP4VRFileReaderInterface::FILE_OPEN_ERROR, aConfig.initFilename);
            }
        }
    }

    std::unique_ptr<MediaSource> MP4Loader::sourceForTrack(TrackId aTrackId,
                                                           const SourceConfig& aSourceConfig)
    {
        std::list<MP4LoaderFrameInfo> frames;

        MP4VR::DynArray<MP4VR::TrackInformation> tracks;

        int32_t rc = mReader->getTrackInformations(tracks);
        checkAndThrow(rc);
        MP4VR::TrackInformation trackInfo;

        // presOffset needs to be large enough so that for all codingTime it holds that codingTime
        // <= presTime + presOffset
        FrameTime presOffset{0, 1};

        for (auto& track: tracks)
        {
            MP4VR::FourCC codec;
            rc = mReader->getDecoderCodeType(track.trackId, track.sampleProperties[0].sampleId, codec);
            checkAndThrow(rc);
            //if (track.trackId == aTrackId.get())
            // We need to handle all tracks so that presOffset affects them all similarly
            {
                auto durations = getSampleDurations(track);

                trackInfo = track;

                if (track.sampleProperties.numElements)
                {
                    auto format = formatOfTrack(*mReader, track);

                    MP4VR::DynArray<MP4VR::DecoderSpecificInfo> decoderInfos;
                    rc = mReader->getDecoderConfiguration(track.trackId, track.sampleProperties[0].sampleId, decoderInfos);
                    if (rc != MP4VR::MP4VRFileReaderInterface::INVALID_ITEM_ID)
                    {
                        checkAndThrow(rc);
                    }

                    auto decoderConfig = makeDecoderConfig(decoderInfos);

                    Bitrate bitrate = determineBitrate(*mReader,
                                                       track,
                                                       FrameDuration { 4, 1 },
                                                       aSourceConfig.overrideFrameDuration);

                    uint32_t width, height;
                    mReader->getWidth(track.trackId, track.sampleProperties[0].sampleId, width);
                    mReader->getHeight(track.trackId, track.sampleProperties[0].sampleId, height);

                    // acquire presentation indices:
                    // 1) Sort sample properties by .earliestTimestampTS
                    // 2) Assign mapping from sampleId to index in the array
                    // 3) That index is now presentation index

                    std::map<std::uint32_t, std::uint32_t> sampleIdToPresIndex;
                    {
                        std::vector<MP4VR::SampleInformation> sortedSampleProperties{
                            track.sampleProperties.begin(), track.sampleProperties.end()};
                        std::sort(sortedSampleProperties.begin(), sortedSampleProperties.end(),
                                  [](const MP4VR::SampleInformation& a,
                                     const MP4VR::SampleInformation& b) {
                                      return a.earliestTimestampTS < b.earliestTimestampTS;
                                  });
                        for (size_t index = 0; index < sortedSampleProperties.size(); ++index)
                        {
                            sampleIdToPresIndex[sortedSampleProperties[index].sampleId] = Index(index);
                        }
                    }

                    auto frameRate = FrameTime{static_cast<int64_t>(track.frameRate.num),
                                               static_cast<int64_t>(track.frameRate.den)};

                    for (size_t sampleIndex = 0; sampleIndex < track.sampleProperties.numElements; ++sampleIndex)
                    {
                        auto& sample = track.sampleProperties[sampleIndex];
                        CodedFrameMeta meta;
                        meta.inCodingOrder = true;
                        meta.codingIndex = sample.sampleId;
                        meta.presIndex = sampleIdToPresIndex.at(sample.sampleId);
                        meta.codingTime = FrameTime(meta.codingIndex, 1) / frameRate;
                        meta.presTime = {
                            FrameTime(static_cast<std::int64_t>(sample.earliestTimestampTS),
                                      static_cast<std::int64_t>(track.timeScale))};
                        if (meta.presTime + presOffset < meta.codingTime)
                        {
                            presOffset = meta.codingTime - meta.presTime;
                        }
                        meta.duration = aSourceConfig.overrideFrameDuration ? *aSourceConfig.overrideFrameDuration : durations[sampleIndex];
                        meta.trackId = track.trackId;
                        meta.format = format;
                        meta.samplingFrequency = track.timeScale; // actually provides a useful value for audio only
                        meta.bitrate = bitrate;
                        meta.width = width;
                        meta.height = height;
                        if (sampleIndex == 0)
                        {
                            meta.decoderConfig = decoderConfig;
                        }
                        meta.type =
                            sample.sampleType == MP4VR::OUTPUT_REFERENCE_FRAME
                            ? FrameType::IDR
                            : FrameType::NONIDR;

                        if (track.trackId == aTrackId.get())
                        {
                            frames.push_back(MP4LoaderFrameInfo { meta });
                        }
                    }
                }
            }
        }

        for (auto& frame : frames)
        {
            frame.frameMeta.presTime += presOffset;
            assert(frame.frameMeta.presTime >= frame.frameMeta.codingTime);
        }

        return Utils::static_cast_unique_ptr<MediaSource>(
            std::unique_ptr<MP4LoaderSource>(new MP4LoaderSource(
                mReader, aTrackId, trackInfo, std::move(frames), mVideoNalStartCodes,
                aSourceConfig.frameLimit ? *aSourceConfig.frameLimit : ~0u, mSegmentKeeper)));
    }

    std::set<TrackId> MP4Loader::getTracksByFormat(std::set<CodedFormat> aFormats)
    {
        std::set<TrackId> trackIds;
        MP4VR::DynArray<MP4VR::TrackInformation> tracks;

        int32_t rc = mReader->getTrackInformations(tracks);
        checkAndThrow(rc);

        for (auto& track : tracks)
        {
            auto format = formatOfTrack(*mReader, track);
            if (format == CodedFormat::H265Extractor)
            {
                throw MP4LoaderError(OMAFNonVIInput);
            }
        }

        for (auto& track: tracks)
        {
            auto format = formatOfTrack(*mReader, track);
            if (aFormats.count(format))
            {
                trackIds.insert(track.trackId);
            }
        }
        return trackIds;
    }

    std::set<TrackId> MP4Loader::getTracksBy(std::function<bool(MediaSource& aSource)> aPredicate)
    {
        std::set<TrackId> trackIds;
        MP4VR::DynArray<MP4VR::TrackInformation> tracks;

        int32_t rc = mReader->getTrackInformations(tracks);
        checkAndThrow(rc);

        for (auto& track: tracks)
        {
            if (aPredicate(*sourceForTrack(track.trackId)))
            {
                trackIds.insert(track.trackId);
            }
        }
        return trackIds;
    }

    MP4Loader::~MP4Loader() = default;
}  // namespace VDD
