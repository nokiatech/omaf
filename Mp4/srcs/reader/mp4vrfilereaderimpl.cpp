
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
#include "mp4vrfilereaderimpl.hpp"

#include <limits.h>

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstring>
#include <fstream>

#include "api/reader/mp4vrfiledatatypes.h"
#include "audiosampleentrybox.hpp"
#include "avcconfigurationbox.hpp"
#include "avcdecoderconfigrecord.hpp"
#include "avcsampleentry.hpp"
#include "buildinfo.hpp"
#include "cleanaperturebox.hpp"
#include "customallocator.hpp"
#include "dynamicviewpointsampleentrybox.hpp"
#include "hevcdecoderconfigrecord.hpp"
#include "hevcsampleentry.hpp"
#include "initialviewingorientationsampleentry.hpp"
#include "initialviewpointsampleentrybox.hpp"
#include "log.hpp"
#include "mediadatabox.hpp"
#include "metabox.hpp"
#include "metadatasampleentrybox.hpp"
#include "moviebox.hpp"
#include "moviefragmentbox.hpp"
#include "mp4audiosampleentrybox.hpp"
#include "mp4vrfilereaderutil.hpp"
#include "overlaysampleentrybox.hpp"
#include "segmentindexbox.hpp"
#include "segmenttypebox.hpp"
#include "urimetasampleentrybox.hpp"
#include "recommendedviewportsampleentry.hpp"

#if defined(__GNUC__) && defined(__DEPRECATED)
#undef __DEPRECATED
#endif
#include <strstream>

namespace MP4VR
{
    const char* ident = "$Id: MP4VR version " MP4VR_BUILD_VERSION " $";

    namespace
    {

        ISOBMFF::Optional<ImdaId> getImdaIdFromDataEntries(const Vector<std::shared_ptr<const DataEntryBox>>& aDataEntries,
                                                           const MovieFragmentBox& aMoofBox)
        {
            ISOBMFF::Optional<ImdaId> imdaId;
            for (auto entry : aDataEntries)
            {
                if (auto imda = std::dynamic_pointer_cast<const DataEntryImdaBox>(entry))
                {
                    imdaId = {imda->getImdaRefIdentifier()};
                }
                else if (auto snim = std::dynamic_pointer_cast<const DataEntrySeqNumImdaBox>(entry))
                {
                    imdaId = {ImdaId(aMoofBox.getMovieFragmentHeaderBox().getSequenceNumber())};
                }
            }

            return imdaId;
        }

        ISOBMFF::Optional<ImdaId> getImdaIdFromMoofMeta(const MovieFragmentBox& aMoofBox)
        {
            ISOBMFF::Optional<ImdaId> imdaId;

            if (auto metaBox = aMoofBox.getMetaBox())
            {
                Vector<std::shared_ptr<const DataEntryBox>> entries =
                    metaBox->getDataInformationBox().getDataReferenceBox().getDataEntries();

                imdaId = getImdaIdFromDataEntries(entries, aMoofBox);
            }

            return imdaId;
        }

        Vector<std::shared_ptr<const DataEntryBox>> getSnimDataEntries(const TrackBox& aTrak)
        {
            const MediaBox& mdia                                = aTrak.getMediaBox();
            const MediaInformationBox& minf                     = mdia.getMediaInformationBox();
            const DataInformationBox& dinf                      = minf.getDataInformationBox();
            const DataReferenceBox& dref                        = dinf.getDataReferenceBox();
            Vector<std::shared_ptr<const DataEntryBox>> entries = dref.getDataEntries();
            return entries;
        }
    }  // namespace

    CustomAllocator::CustomAllocator()
    {
        // nothing
    }

    CustomAllocator::~CustomAllocator()
    {
        // nothing
    }

    MP4VR_DLL_PUBLIC MP4VRFileReaderInterface::ErrorCode
    MP4VRFileReaderInterface::SetCustomAllocator(CustomAllocator* customAllocator)
    {
        if (!setCustomAllocator(customAllocator))
        {
            return ErrorCode::ALLOCATOR_ALREADY_SET;
        }
        else
        {
            return ErrorCode::OK;
        }
    }

    MP4VR_DLL_PUBLIC MP4VRFileReaderInterface* MP4VRFileReaderInterface::Create()
    {
        return CUSTOM_NEW(MP4VRFileReaderImpl, ());
    }

    MP4VR_DLL_PUBLIC void MP4VRFileReaderInterface::Destroy(MP4VRFileReaderInterface* mp4vrinterface)
    {
        CUSTOM_DELETE(mp4vrinterface, MP4VRFileReaderInterface);
    }

    int32_t MP4VRFileReaderImpl::initialize(const char* fileName)
    {
        int32_t rc;
        mFileStream.reset(openFile(fileName));
        rc = initialize(&*mFileStream);
        if (rc != ErrorCode::OK)
        {
            mFileStream.reset();
        }
        return rc;
    }

    int32_t MP4VRFileReaderImpl::initialize(StreamInterface* stream)
    {
        InitSegmentId initSegmentId = 0;  // for offline files use 0 as initialization segment id.
        SegmentId segmentId         = 0;  // 0 index of segmentPropertiesMap is reserved for initialization segment data

        UniquePtr<InternalStream> internalStream(CUSTOM_NEW(InternalStream, (stream)));

        if (!internalStream->good())
        {
            return ErrorCode::FILE_OPEN_ERROR;
        }

        auto& io  = mInitSegmentPropertiesMap[initSegmentId].segmentPropertiesMap[segmentId].io;
        io.stream = std::move(internalStream);
        io.size   = io.stream->size();

        try
        {
            int32_t error = readStream(initSegmentId, segmentId);
            if (error)
            {
                return error;
            }
        }
        catch (const Exception& exc)
        {
            logError() << "Error: " << exc.what() << std::endl;
            return ErrorCode::FILE_READ_ERROR;
        }
        catch (const std::exception& e)
        {
            logError() << "Error: " << e.what() << std::endl;
            return ErrorCode::FILE_READ_ERROR;
        }
        return ErrorCode::OK;
    }

    MP4VRFileReaderImpl::MP4VRFileReaderImpl()
        : mState(State::UNINITIALIZED)
    {
    }

    void MP4VRFileReaderImpl::dumpSegmentInfo() const
    {
#ifndef DISABLE_UNCOVERED_CODE
        auto align = [](size_t n, String str) { return (str.size() < n) ? str + String(n - str.size(), ' ') : str; };

        for (const auto& initSegment : mInitSegmentPropertiesMap)
        {
            for (const auto& segment : initSegment.second.segmentPropertiesMap)
            {
                SegmentId segmentId                        = segment.first;
                const SegmentProperties& segmentProperties = segment.second;

                std::cout << "segment " << segmentId << std::endl;

                for (const auto& track : segmentProperties.trackInfos)
                {
                    ContextId trackId          = track.first;
                    auto timeScale             = initSegment.second.initTrackInfos.at(trackId).timeScale;
                    const TrackInfo& trackInfo = track.second;

                    std::cout << "  track " << trackId << std::endl;
                    size_t n = 0;
                    for (const auto& sample : trackInfo.samples)
                    {
                        n += sample.compositionTimes.size();
                    }
                    std::cout << "    durationTS: " << trackInfo.durationTS << std::endl;
                    std::cout << "    durationTS in ms: " << trackInfo.durationTS * 1000 / timeScale << std::endl;
                    std::cout << "    earliestPTSTS: " << trackInfo.earliestPTSTS << std::endl;
                    std::cout << "    earliestPTSTS in ms: " << trackInfo.earliestPTSTS * 1000 / timeScale << std::endl;
                    std::cout << "    noSidxFallbackPTSTS: " << trackInfo.noSidxFallbackPTSTS << std::endl;
                    std::cout << "    noSidxFallbackPTSTS in ms: " << trackInfo.noSidxFallbackPTSTS * 1000 / timeScale
                              << std::endl;
                    std::cout << "    # samples: " << trackInfo.samples.size() << std::endl;
                    std::cout << "    # of composition times: " << n << std::endl;
                    if (trackInfo.samples.size())
                    {
                        auto showSample = [](const SampleInfo& info) {
                            std::cout << "      sampleId: " << info.sampleId << std::endl;
                            for (auto compositionTime : info.compositionTimes)
                            {
                                std::cout << "      composition: " << compositionTime << std::endl;
                            }
                            for (auto compositionTimeTS : info.compositionTimesTS)
                            {
                                std::cout << "      compositionTS: " << compositionTimeTS << std::endl;
                            }
                        };
                        std::cout << "    first sample:" << std::endl;
                        showSample(trackInfo.samples.front());
                        std::cout << "    last sample:" << std::endl;
                        showSample(trackInfo.samples.back());

                        size_t count = 0;
                        for (const SampleInfo& sample : trackInfo.samples)
                        {
                            OStringStream ss;
                            ss << "[" << sample.sampleId << ":";
                            bool first = true;
                            for (auto& ct : sample.compositionTimes)
                            {
                                ss << (first ? "" : " ") << ct;
                                first = false;
                            }
                            ss << ",";
                            ss << "ts: ";
                            for (auto& ctTS : sample.compositionTimesTS)
                            {
                                ss << ctTS;
                            }
                            ss << ",dur: " << sample.sampleDuration;
                            ss << "]";
                            std::cout << align(16, ss.str());
                            ++count;
                            if (count % 6 == 0)
                            {
                                std::cout << "\n";
                            }
                        }
                        if (count % 6 != 0)
                        {
                            std::cout << "\n";
                        }
                    }
                }
            }
        }
#endif  // DISABLE_UNCOVERED_CODE
    }

    const InitTrackInfo& MP4VRFileReaderImpl::getInitTrackInfo(InitSegmentTrackId initSegTrackId) const
    {
        return mInitSegmentPropertiesMap.at(initSegTrackId.first).initTrackInfos.at(initSegTrackId.second);
    }

    InitTrackInfo& MP4VRFileReaderImpl::getInitTrackInfo(InitSegmentTrackId initSegTrackId)
    {
        return mInitSegmentPropertiesMap.at(initSegTrackId.first).initTrackInfos.at(initSegTrackId.second);
    }

    bool MP4VRFileReaderImpl::hasTrackInfo(InitSegmentId initSegmentId, SegmentTrackId segTrackId) const
    {
        const auto& segmentProperties =
            mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.find(segTrackId.first);
        if (segmentProperties != mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.end())
        {
            const auto& trackInfo = segmentProperties->second.trackInfos.find(segTrackId.second);
            return trackInfo != segmentProperties->second.trackInfos.end();
        }
        else
        {
            return false;
        }
    }

    const TrackInfo& MP4VRFileReaderImpl::getTrackInfo(InitSegmentId initSegmentId, SegmentTrackId segTrackId) const
    {
        return mInitSegmentPropertiesMap.at(initSegmentId)
            .segmentPropertiesMap.at(segTrackId.first)
            .trackInfos.at(segTrackId.second);
    }

    TrackInfo& MP4VRFileReaderImpl::getTrackInfo(InitSegmentId initSegmentId, SegmentTrackId segTrackId)
    {
        return mInitSegmentPropertiesMap.at(initSegmentId)
            .segmentPropertiesMap.at(segTrackId.first)
            .trackInfos.at(segTrackId.second);
    }

    bool MP4VRFileReaderImpl::getPrecedingSegment(InitSegmentId initSegmentId,
                                                  SegmentId curSegmentId,
                                                  SegmentId& precedingSegmentId) const
    {
        auto& segmentProperties = mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(curSegmentId);
        auto& sequenceToSegment = mInitSegmentPropertiesMap.at(segmentProperties.initSegmentId).sequenceToSegment;
        auto iterator           = segmentProperties.sequences.empty()
                            ? sequenceToSegment.end()
                            : sequenceToSegment.find(*segmentProperties.sequences.begin());

        // This can only happen when processing init segment
        if (iterator == sequenceToSegment.end())
        {
            return false;
        }
        else
        {
            if (iterator != sequenceToSegment.begin())
            {
                --iterator;
                precedingSegmentId = iterator->second;
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    const TrackInfo* MP4VRFileReaderImpl::getPrecedingTrackInfo(InitSegmentId initSegmentId,
                                                                SegmentTrackId segTrackId) const
    {
        SegmentId curSegmentId = segTrackId.first;
        ContextId trackId      = segTrackId.second;
        SegmentId precedingSegmentId;
        const TrackInfo* trackInfo = nullptr;
        while (!trackInfo && getPrecedingSegment(initSegmentId, curSegmentId, precedingSegmentId))
        {
            auto& segmentProperties =
                mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(precedingSegmentId);
            if (segmentProperties.trackInfos.count(trackId))
            {
                trackInfo = &segmentProperties.trackInfos.at(trackId);
            }
            curSegmentId = precedingSegmentId;
        }
        return trackInfo;
    }

    const SampleInfoVector& MP4VRFileReaderImpl::getSampleInfo(InitSegmentId initSegmentId,
                                                               SegmentTrackId segTrackId,
                                                               ItemId& sampleBase) const
    {
        auto& segmentProperties = mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segTrackId.first);
        sampleBase              = getTrackInfo(initSegmentId, segTrackId).itemIdBase;
        return segmentProperties.trackInfos.at(segTrackId.second).samples;
    }

    const ParameterSetMap* MP4VRFileReaderImpl::getParameterSetMap(Id id) const
    {
        auto contextId              = id.first.second;
        InitSegmentId initSegmentId = id.first.first;
        SegmentId segmentId;
        int result = segmentIdOf(id, segmentId);
        if (result != ErrorCode::OK)
        {
            return nullptr;
        }
        const auto& segmentProperties = mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segmentId);
        const auto& initSegmentProperties = mInitSegmentPropertiesMap.at(segmentProperties.initSegmentId);
        SegmentTrackId segTrackId         = SegmentTrackId(segmentId, contextId);
        const auto& itemIndexIterator     = segmentProperties.itemToParameterSetMap.find(
            std::make_pair(id.first, id.second - getTrackInfo(initSegmentId, segTrackId).itemIdBase));
        if (itemIndexIterator != segmentProperties.itemToParameterSetMap.end())
        {
            if (initSegmentProperties.initTrackInfos.at(id.first.second)
                    .parameterSetMaps.count(itemIndexIterator->second))
            {
                return &initSegmentProperties.initTrackInfos.at(id.first.second)
                            .parameterSetMaps.at(itemIndexIterator->second);
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }

    void MP4VRFileReaderImpl::close()
    {
        mState = State::UNINITIALIZED;
        while (!mInitSegmentPropertiesMap.empty())
        {
            invalidateInitializationSegment(mInitSegmentPropertiesMap.begin()->first.get());
        }
    }

    void MP4VRFileReaderImpl::setupSegmentSidxFallback(InitSegmentId initSegmentId, SegmentTrackId segTrackId)
    {
        SegmentId segmentId     = segTrackId.first;
        ContextId trackId       = segTrackId.second;
        auto& segmentProperties = mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segmentId);

        DecodePts::PresentationTimeTS segmentDurationTS = 0;
        for (auto& trackInfo : segmentProperties.trackInfos)
        {
            segmentDurationTS = std::max(segmentDurationTS, trackInfo.second.durationTS);
        }

        SegmentId precedingSegmentId;
        DecodePts::PresentationTimeTS curSegmentStartTS;
        if (const TrackInfo* trackInfo = getPrecedingTrackInfo(initSegmentId, segTrackId))
        {
            curSegmentStartTS = trackInfo->noSidxFallbackPTSTS;
        }
        else
        {
            curSegmentStartTS = 0;
        }
        DecodePts::PresentationTimeTS afterSampleDurationPTSTS = curSegmentStartTS + segmentDurationTS;

        if (afterSampleDurationPTSTS > segmentProperties.trackInfos.at(trackId).noSidxFallbackPTSTS)
        {
            segmentProperties.trackInfos.at(trackId).noSidxFallbackPTSTS = afterSampleDurationPTSTS;
        }
    }

    void MP4VRFileReaderImpl::updateCompositionTimes(InitSegmentId initSegmentId, SegmentId segmentId)
    {
        // store information about overall segment:
        for (auto& trackTrackInfo :
             mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segmentId).trackInfos)
        {
            auto& trackInfo = trackTrackInfo.second;
            if (trackInfo.pMap.size())
            {
                // Set composition times from Pmap, which considers also edit lists
                for (const auto& pair : trackInfo.pMap)
                {
                    trackInfo.samples.at(pair.second).compositionTimes.push_back(std::uint64_t(pair.first));
                }
                for (const auto& pair : trackInfo.pMapTS)
                {
                    trackInfo.samples.at(pair.second).compositionTimesTS.push_back(std::uint64_t(pair.first));
                }

                // adjust the duration of last composed sample to match the total duration, but only if we are using
                // edit lists
                if (trackInfo.hasEditList)
                {
                    auto& last = *trackInfo.pMapTS.rbegin();
                    trackInfo.samples.at(last.second).sampleDuration =
                        std::min(trackInfo.samples.at(last.second).sampleDuration,
                                 static_cast<std::uint32_t>(trackInfo.durationTS - last.first));
                }
            }
        }
    }

    Segments MP4VRFileReaderImpl::segmentsBySequence(InitSegmentId initSegmentId)
    {
        return Segments(*this, initSegmentId);
    }

    ConstSegments MP4VRFileReaderImpl::segmentsBySequence(InitSegmentId initSegmentId) const
    {
        return ConstSegments(*this, initSegmentId);
    }

    int32_t MP4VRFileReaderImpl::parseInitializationSegment(StreamInterface* streamInterface, uint32_t initSegId)
    {
        InitSegmentId initSegmentId = initSegId;
        SegmentId segmentId = 0;  // all "segment" info for initialization segment goes to key=0 of SegmentPropertiesMap

        if (mInitSegmentPropertiesMap.count(initSegmentId))
        {
            return MP4VRFileReaderInterface::OK;
        }

        State prevState = mState;
        mState          = State::INITIALIZING;

        SegmentProperties& segmentProperties = mInitSegmentPropertiesMap[initSegmentId].segmentPropertiesMap[segmentId];
        SegmentIO& io                        = segmentProperties.io;
        io.stream.reset(CUSTOM_NEW(InternalStream, (streamInterface)));
        if (io.stream->peekEof())
        {
            mState = prevState;
            io.stream.reset();
            return MP4VRFileReaderInterface::FILE_READ_ERROR;
        }
        io.size = streamInterface->size();

        segmentProperties.initSegmentId = initSegmentId;
        segmentProperties.segmentId     = segmentId;

        bool ftypFound       = false;
        bool moovFound       = false;
        bool earliestPTSRead = false;

        int32_t error = ErrorCode::OK;
        if (io.stream->peekEof())
        {
            error = ErrorCode::FILE_HEADER_ERROR;
        }

        try
        {
            InitSegmentProperties& initSegmentProperties = mInitSegmentPropertiesMap[initSegmentId];

            while (!error && !io.stream->peekEof())
            {
                String boxType;
                std::int64_t boxSize = 0;
                BitStream bitstream;
                error = readBoxParameters(io, boxType, boxSize);
                if (!error)
                {
                    if (boxType == "ftyp")
                    {
                        if (ftypFound == true)
                        {
                            error = ErrorCode::FILE_READ_ERROR;
                            break;
                        }
                        ftypFound = true;

                        error = readBox(io, bitstream);
                        if (!error)
                        {
                            FileTypeBox ftyp;
                            ftyp.parseBox(bitstream);

                            // Check supported brands
                            Set<String> supportedBrands;

                            if (ftyp.checkCompatibleBrand("nvr1"))
                            {
                                supportedBrands.insert("[nvr1] ");
                            }

                            logInfo() << "Compatible brands found:" << std::endl;
                            for (auto brand : supportedBrands)
                            {
                                logInfo() << " " << brand << std::endl;
                            }

                            initSegmentProperties.ftyp = ftyp;
                        }
                    }
                    else if (boxType == "sidx")
                    {
                        error = readBox(io, bitstream);
                        if (!error)
                        {
                            SegmentIndexBox sidx;
                            sidx.parseBox(bitstream);

                            if (!earliestPTSRead)
                            {
                                earliestPTSRead = true;
                            }
                            makeSegmentIndex(sidx, initSegmentProperties.segmentIndex, io.stream->tell());
                        }
                    }
                    else if (boxType == "moov")
                    {
                        if (moovFound == true)
                        {
                            error = ErrorCode::FILE_READ_ERROR;
                            break;
                        }
                        moovFound = true;

                        error = readBox(io, bitstream);
                        if (!error)
                        {
                            MovieBox moov;
                            moov.parseBox(bitstream);
                            initSegmentProperties.moovProperties  = extractMoovProperties(moov);
                            initSegmentProperties.trackProperties = fillTrackProperties(initSegmentId, segmentId, moov);
                            initSegmentProperties.trackReferences =
                                trackGroupsOfTrackProperties(initSegmentProperties.trackProperties);
                            initSegmentProperties.movieTimeScale = moov.getMovieHeaderBox().getTimeScale();
                            mMatrix                              = moov.getMovieHeaderBox().getMatrix();
                        }
                    }
                    else if (boxType == "moof")
                    {
                        logWarning() << "Skipping root level 'moof' box - not allowed in Initialization Segment"
                                     << std::endl;
                        error = skipBox(io);
                    }
                    else if (boxType == "meta")
                    {
                        error = readBox(io, bitstream);
                        if (!error)
                        {
                            initSegmentProperties.meta = MetaBox();
                            initSegmentProperties.meta->parseBox(bitstream);
                        }
                    }
                    else if (boxType == "mdat")
                    {
                        // skip mdat as its handled elsewhere
                        error = skipBox(io);
                    }
                    else
                    {
                        logWarning() << "Skipping root level box of unknown type '" << boxType << "'" << std::endl;
                        error = skipBox(io);
                    }
                }
            }
        }
        catch (Exception& exc)
        {
            logError() << "parseInitializationSegment Exception Error: " << exc.what() << std::endl;
            error = ErrorCode::FILE_READ_ERROR;
        }
        catch (std::exception& e)
        {
            logError() << "parseInitializationSegment std::exception Error:: " << e.what() << std::endl;
            error = ErrorCode::FILE_READ_ERROR;
        }

        if (!error && (!ftypFound || !moovFound))
        {
            error = ErrorCode::FILE_HEADER_ERROR;
        }

        if (!error)
        {
            for (auto& trackInfo : segmentProperties.trackInfos)
            {
                setupSegmentSidxFallback(initSegmentId, std::make_pair(segmentId, trackInfo.first));
            }

            updateCompositionTimes(initSegmentId, segmentId);

            // peek() sets eof bit for the stream. Clear stream to make sure it is still accessible. seekg() in C++11
            // should clear stream after eof, but this does not seem to be always happening.
            if ((!io.stream->good()) && (!io.stream->eof()))
            {
                return ErrorCode::FILE_READ_ERROR;
            }
            io.stream->clear();

            mInitSegmentPropertiesMap[initSegmentId].fileFeature = getFileFeatures();

            mState = State::READY;
        }
        else
        {
            invalidateInitializationSegment(initSegmentId.get());
        }
        return error;
    }

    int32_t MP4VRFileReaderImpl::invalidateInitializationSegment(uint32_t initSegmentId)
    {
        bool isInitSegment = !!mInitSegmentPropertiesMap.count(initSegmentId);
        if (isInitSegment)
        {
            for (auto& initTrackInfo : mInitSegmentPropertiesMap.at(initSegmentId).initTrackInfos)
            {
                mContextInfoMap.erase(InitSegmentTrackId(initSegmentId, initTrackInfo.first));
            }
            mInitSegmentPropertiesMap.erase(initSegmentId);
        }
        return (isInitSegment) ? ErrorCode::OK : ErrorCode::INVALID_SEGMENT;
    }

    int32_t MP4VRFileReaderImpl::iterateBoxes(
        SegmentIO& aIo,
        std::function<ISOBMFF::Optional<int32_t>(String boxType, BitStream& bitstream)> aHandler)
    {
        int32_t iterateError = ErrorCode::OK;
        while (!iterateError && !aIo.stream->peekEof())
        {
            String boxType;
            std::int64_t boxSize = 0;
            BitStream bitstream;
            iterateError = readBoxParameters(aIo, boxType, boxSize);
            if (!iterateError)
            {
                if (auto handlerError = aHandler(boxType, bitstream))
                {
                    iterateError = *handlerError;
                }
                else
                {
                    iterateError = skipBox(aIo);
                }
            }
        }
        return iterateError;
    }

    int32_t MP4VRFileReaderImpl::parseSegment(StreamInterface* streamInterface,
                                              uint32_t initSegmentId,
                                              uint32_t segmentId,
                                              uint64_t earliestPTSinTS)
    {
        if (mInitSegmentPropertiesMap.count(initSegmentId) &&
            mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.count(segmentId))
        {
            return MP4VRFileReaderInterface::OK;
        }
        if (!mInitSegmentPropertiesMap.count(initSegmentId))
        {
            return MP4VRFileReaderInterface::INVALID_SEGMENT;
        }

        State prevState = mState;
        mState          = State::INITIALIZING;

        SegmentProperties& segmentProperties =
            mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap[segmentId];
        SegmentIO& io = segmentProperties.io;
        io.stream.reset(CUSTOM_NEW(InternalStream, (streamInterface)));
        if (io.stream->peekEof())
        {
            mState = prevState;
            io.stream.reset();
            return MP4VRFileReaderInterface::FILE_READ_ERROR;
        }
        io.size = streamInterface->size();

        segmentProperties.initSegmentId = initSegmentId;
        segmentProperties.segmentId     = segmentId;

        bool stypFound       = false;
        bool earliestPTSRead = false;
        Map<ContextId, DecodePts::PresentationTimeTS> earliestPTSTS;

        int32_t error = ErrorCode::OK;
        try
        {
            auto initialOffset = io.stream->tell();
            error = iterateBoxes(io, [&](String boxType, BitStream& /* bitstream */) {
                ISOBMFF::Optional<int32_t> iterateError;
                if (boxType == "imda")
                {
                    iterateError = parseImda(segmentProperties);
                }
                return iterateError;
            });
            if (error == ErrorCode::OK)
            {
                io.stream->clear();
                io.stream->seek(initialOffset);
                error = iterateBoxes(io, [&](String boxType, BitStream& bitstream) {
                    ISOBMFF::Optional<int32_t> iterateError;
                    if (boxType == "styp")
                    {
                        iterateError = readBox(io, bitstream);
                        if (!*iterateError)
                        {
                            SegmentTypeBox styp;
                            styp.parseBox(bitstream);

                            if (stypFound == false)
                            {
                                // only save first styp of the segment - rest can be ignored per spec.
                                segmentProperties.styp = styp;
                                stypFound              = true;
                            }
                        }
                    }
                    else if (boxType == "sidx")
                    {
                        iterateError = readBox(io, bitstream);
                        if (!*iterateError)
                        {
                            SegmentIndexBox sidx;
                            sidx.parseBox(bitstream);

                            if (!earliestPTSRead)
                            {
                                earliestPTSRead = true;

                                for (auto& initTrackInfo : mInitSegmentPropertiesMap.at(initSegmentId).initTrackInfos)
                                {
                                    ContextId trackId = initTrackInfo.first;
                                    earliestPTSTS[trackId] =
                                        DecodePts::PresentationTimeTS(sidx.getEarliestPresentationTime());
                                }
                            }
                        }
                    }
                    else if (boxType == "moof")
                    {
                        // we need to save moof start byte for possible trun dataoffset depending on its flags.
                        const StreamInterface::offset_t moofFirstByte = io.stream->tell();

                        iterateError = readBox(io, bitstream);
                        if (!*iterateError)
                        {
                            MovieFragmentBox moof(
                                mInitSegmentPropertiesMap.at(initSegmentId).moovProperties.fragmentSampleDefaults);
                            moof.setMoofFirstByteOffset(static_cast<uint64_t>(moofFirstByte));
                            moof.parseBox(bitstream);

                            if (!earliestPTSRead)
                            {
                                // no sidx information about pts available. Use previous segment last sample pts.
                                for (auto& initTrackInfo : mInitSegmentPropertiesMap.at(initSegmentId).initTrackInfos)
                                {
                                    ContextId trackId = initTrackInfo.first;
                                    if (earliestPTSinTS != UINT64_MAX)
                                    {
                                        earliestPTSTS[trackId] = DecodePts::PresentationTimeTS(earliestPTSinTS);
                                    }
                                    else if (const TrackInfo* precTrackInfo = getPrecedingTrackInfo(
                                                 initSegmentId, SegmentTrackId(segmentId, trackId)))
                                    {
                                        earliestPTSTS[trackId] = precTrackInfo->noSidxFallbackPTSTS;
                                    }
                                    else
                                    {
                                        earliestPTSTS[trackId] = 0;
                                    }
                                }

                                earliestPTSRead = true;
                            }

                            ContextIdPresentationTimeTSMap earliestPTSTSForTrack;
                            for (auto& trackFragmentBox : moof.getTrackFragmentBoxes())
                            {
                                auto trackId = ContextId(trackFragmentBox->getTrackFragmentHeaderBox().getTrackId());
                                earliestPTSTSForTrack.insert(std::make_pair(trackId, earliestPTSTS.at(trackId)));
                            }

                            addToTrackProperties(initSegmentId, segmentId, moof, earliestPTSTSForTrack);
                        }
                    }
                    else if (boxType == "mdat")
                    {
                        // skip mdat as its handled elsewhere
                    }
                    else if (boxType == "imda")
                    {
                        // handled already
                    }
                    else
                    {
                        logWarning() << "Skipping root level box of unknown type '" << boxType << "'" << std::endl;
                    }
                    return iterateError;
                });
            }
        }
        catch (Exception& exc)
        {
            logError() << "parseSegment Exception Error: " << exc.what() << std::endl;
            error = ErrorCode::FILE_READ_ERROR;
        }
        catch (std::exception& e)
        {
            logError() << "parseSegment std::exception Error:: " << e.what() << std::endl;
            error = ErrorCode::FILE_READ_ERROR;
        }

        if (!error)
        {
            for (auto& trackInfo : segmentProperties.trackInfos)
            {
                setupSegmentSidxFallback(initSegmentId, std::make_pair(segmentId, trackInfo.first));
            }

            updateCompositionTimes(initSegmentId, segmentId);

            // peek() sets eof bit for the stream. Clear stream to make sure it is still accessible. seekg() in C++11
            // should clear stream after eof, but this does not seem to be always happening.
            if ((!io.stream->good()) && (!io.stream->eof()))
            {
                return ErrorCode::FILE_READ_ERROR;
            }
            io.stream->clear();
            mState = State::READY;
        }
        else
        {
            invalidateSegment(initSegmentId, segmentId);
        }
        return error;
    }

    int32_t MP4VRFileReaderImpl::parseImda(SegmentProperties& aSegmentProperties)
    {
        SegmentIO& io = aSegmentProperties.io;
        const std::int64_t startLocation = io.stream->tell();

        std::int64_t boxSize;
        int32_t error = readBytes(io, 4, boxSize);
        if (error)
        {
            return error;
        }

        String boxTypeStr(4, 0);
        io.stream->read(&boxTypeStr[0], StreamInterface::offset_t(boxTypeStr.size()));
        if (!io.stream->good())
        {
            return MP4VRFileReaderInterface::FILE_READ_ERROR;
        }

        // ensured by the called of parseImda
        assert(FourCCInt(boxTypeStr) == FourCCInt("imda"));

        std::int64_t imda;
        error = readBytes(io, 4, imda);
        if (error)
        {
            return error;
        }

        if (aSegmentProperties.imda.count(imda))
        {
            return ErrorCode::INVALID_SEGMENT;
        }
        aSegmentProperties.imda[imda] = ImdaInfo{startLocation + 4 + 4 + 4, startLocation + boxSize};

        // skip the rest
        seekInput(io, startLocation + boxSize);

        return MP4VRFileReaderInterface::OK;
    }

    int32_t MP4VRFileReaderImpl::invalidateSegment(uint32_t initSegmentId, uint32_t segmentId)
    {
        if (!mInitSegmentPropertiesMap.count(initSegmentId))
        {
            return MP4VRFileReaderInterface::INVALID_SEGMENT;
        }

        bool isSegment = !!mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.count(segmentId);
        if (isSegment)
        {
            auto& segmentProperties = mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segmentId);

            // check that there is valid init seg
            bool hasValidInitSeg = !!mInitSegmentPropertiesMap.count(segmentProperties.initSegmentId);
            if (hasValidInitSeg)
            {
                auto& sequenceToSegment =
                    mInitSegmentPropertiesMap.at(segmentProperties.initSegmentId).sequenceToSegment;
                for (auto& sequence : segmentProperties.sequences)
                {
                    sequenceToSegment.erase(sequence);
                }
                mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.erase(segmentId);
            }
            else
            {
                return ErrorCode::INVALID_SEGMENT;
            }
        }

        return (isSegment) ? ErrorCode::OK : ErrorCode::INVALID_SEGMENT;
    }

    int32_t MP4VRFileReaderImpl::getSegmentIndex(uint32_t initSegmentId, DynArray<SegmentInformation>& segmentIndex)
    {
        bool isInitSegment = !!mInitSegmentPropertiesMap.count(initSegmentId);

        if (isInitSegment)
        {
            segmentIndex = mInitSegmentPropertiesMap.at(initSegmentId).segmentIndex;
        }
        else
        {
            return ErrorCode::INVALID_SEGMENT;
        }
        return ErrorCode::OK;
    }


    int32_t MP4VRFileReaderImpl::parseSegmentIndex(StreamInterface* streamInterface,
                                                   DynArray<SegmentInformation>& segmentIndex)
    {
        SegmentIO io;
        io.stream.reset(CUSTOM_NEW(InternalStream, (streamInterface)));
        if (io.stream->peekEof())
        {
            io.stream.reset();
            return MP4VRFileReaderInterface::FILE_READ_ERROR;
        }
        io.size = streamInterface->size();

        int32_t error          = ErrorCode::OK;
        bool segmentIndexFound = false;
        try
        {
            while (!error && !io.stream->peekEof())
            {
                String boxType;
                std::int64_t boxSize = 0;
                BitStream bitstream;
                error = readBoxParameters(io, boxType, boxSize);
                if (!error)
                {
                    if (boxType == "sidx")
                    {
                        error = readBox(io, bitstream);
                        if (!error)
                        {
                            SegmentIndexBox sidx;
                            sidx.parseBox(bitstream);
                            makeSegmentIndex(sidx, segmentIndex, io.stream->tell());
                            segmentIndexFound = true;
                            break;
                        }
                    }
                    else if (boxType == "styp" || boxType == "moof" || boxType == "mdat")
                    {
                        // skip as we are not interested in it.
                        error = skipBox(io);
                    }
                    else
                    {
                        logWarning() << "Skipping root level box of unknown type '" << boxType << "'" << std::endl;
                        error = skipBox(io);
                    }
                }
            }
        }
        catch (Exception& exc)
        {
            logError() << "parseSegmentIndex Exception Error: " << exc.what() << std::endl;
            error = ErrorCode::FILE_READ_ERROR;
        }
        catch (std::exception& e)
        {
            logError() << "parseSegmentIndex std::exception Error:: " << e.what() << std::endl;
            error = ErrorCode::FILE_READ_ERROR;
        }

        if (!segmentIndexFound)
        {
            logError() << "parseSegmentIndex couldn't find sidx box: " << std::endl;
            error = ErrorCode::INVALID_SEGMENT;
        }

        if (!error)
        {
            // peek() sets eof bit for the stream. Clear stream to make sure it is still accessible. seekg() in C++11
            // should clear stream after eof, but this does not seem to be always happening.
            if ((!io.stream->good()) && (!io.stream->eof()))
            {
                return ErrorCode::FILE_READ_ERROR;
            }
            io.stream->clear();
        }
        return error;
    }

    /* ********************************************************************** */
    /* *********************** Common private methods *********************** */
    /* ********************************************************************** */

    void MP4VRFileReaderImpl::isInitialized() const
    {
        if (!(mState == State::INITIALIZING || mState == State::READY))
        {
            throw FileReaderException(ErrorCode::UNINITIALIZED);
        }
    }

    int MP4VRFileReaderImpl::isInitializedError() const
    {
        if (!(mState == State::INITIALIZING || mState == State::READY))
        {
            return ErrorCode::UNINITIALIZED;
        }
        return ErrorCode::OK;
    }

    MP4VRFileReaderImpl::ContextType MP4VRFileReaderImpl::getContextType(const InitSegmentTrackId initSegTrackId) const
    {
        const auto contextInfo = mContextInfoMap.find(initSegTrackId);
        if (contextInfo == mContextInfoMap.end())
        {
            throw FileReaderException(ErrorCode::INVALID_CONTEXT_ID);
        }

        return contextInfo->second.contextType;
    }

    int MP4VRFileReaderImpl::getContextTypeError(const InitSegmentTrackId initSegTrackId,
                                                 MP4VRFileReaderImpl::ContextType& contextType) const
    {
        const auto contextInfo = mContextInfoMap.find(initSegTrackId);
        if (contextInfo == mContextInfoMap.end())
        {
            return ErrorCode::INVALID_CONTEXT_ID;
        }

        contextType = contextInfo->second.contextType;
        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::readStream(InitSegmentId initSegmentId, SegmentId segmentId)
    {
        mState = State::INITIALIZING;

        SegmentIO& io = mInitSegmentPropertiesMap[initSegmentId].segmentPropertiesMap[segmentId].io;
        mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segmentId).initSegmentId = initSegmentId;
        mInitSegmentPropertiesMap.at(initSegmentId).segmentPropertiesMap.at(segmentId).segmentId     = segmentId;

        bool ftypFound = false;
        bool moovFound = false;

        int32_t error = ErrorCode::OK;
        if (io.stream->peekEof())
        {
            error = ErrorCode::FILE_HEADER_ERROR;
        }

        error = iterateBoxes(io, [&](String boxType, BitStream& bitstream) {
            ISOBMFF::Optional<int32_t> iterateError;
            if (boxType == "ftyp")
            {
                if (ftypFound == true)
                {
                    return ISOBMFF::Optional<int32_t>(ErrorCode::FILE_READ_ERROR);
                }
                ftypFound = true;

                iterateError = readBox(io, bitstream);
                if (!*iterateError)
                {
                    FileTypeBox ftyp;
                    ftyp.parseBox(bitstream);

                    // Check supported brands
                    Set<String> supportedBrands;

                    if (ftyp.checkCompatibleBrand("nvr1"))
                    {
                        supportedBrands.insert("[nvr1] ");
                    }

                    logInfo() << "Compatible brands found:" << std::endl;
                    for (auto brand : supportedBrands)
                    {
                        logInfo() << " " << brand << std::endl;
                    }

                    mInitSegmentPropertiesMap[initSegmentId].ftyp = ftyp;
                }
            }
            else if (boxType == "styp")
            {
                iterateError = readBox(io, bitstream);
                if (!*iterateError)
                {
                    SegmentTypeBox styp;
                    styp.parseBox(bitstream);

                    // Check supported brands
                    Set<String> supportedBrands;

                    logInfo() << "Compatible brands found:" << std::endl;
                    for (auto brand : supportedBrands)
                    {
                        logInfo() << " " << brand << std::endl;
                    }
                }
            }
            else if (boxType == "sidx")
            {
                iterateError = readBox(io, bitstream);
                if (!*iterateError)
                {
                    SegmentIndexBox sidx;
                    sidx.parseBox(bitstream);
                }
            }
            else if (boxType == "moov")
            {
                if (moovFound == true)
                {
                    return ISOBMFF::Optional<int32_t>(ErrorCode::FILE_READ_ERROR);
                }
                moovFound = true;

                iterateError = readBox(io, bitstream);
                if (!*iterateError)
                {
                    MovieBox moov;
                    moov.parseBox(bitstream);
                    mInitSegmentPropertiesMap[initSegmentId].moovProperties = extractMoovProperties(moov);
                    mInitSegmentPropertiesMap[initSegmentId].trackProperties =
                        fillTrackProperties(initSegmentId, segmentId, moov);
                    mInitSegmentPropertiesMap[initSegmentId].trackReferences =
                        trackGroupsOfTrackProperties(mInitSegmentPropertiesMap[initSegmentId].trackProperties);
                    mInitSegmentPropertiesMap[initSegmentId].movieTimeScale = moov.getMovieHeaderBox().getTimeScale();
                    addSegmentSequence(initSegmentId, segmentId, 0);
                    mMatrix = moov.getMovieHeaderBox().getMatrix();
                }
            }
            else if (boxType == "moof")
            {
                // we need to save moof start byte for possible trun dataoffset depending on its flags.
                const StreamInterface::offset_t moofFirstByte = io.stream->tell();

                iterateError = readBox(io, bitstream);
                if (!*iterateError)
                {
                    MovieFragmentBox moof(
                        mInitSegmentPropertiesMap.at(initSegmentId).moovProperties.fragmentSampleDefaults);
                    moof.setMoofFirstByteOffset(static_cast<uint64_t>(moofFirstByte));
                    moof.parseBox(bitstream);

                    ContextIdPresentationTimeTSMap earliestPTSTSForTrack;
                    addToTrackProperties(initSegmentId, segmentId, moof, earliestPTSTSForTrack);

                    // Check sample dataoffsets against fragment data size
                    SegmentProperties& segmentProperties =
                        mInitSegmentPropertiesMap[initSegmentId].segmentPropertiesMap[segmentId];
                    for (auto& trackFragmentBox : moof.getTrackFragmentBoxes())
                    {
                        auto trackId         = ContextId(trackFragmentBox->getTrackFragmentHeaderBox().getTrackId());
                        TrackInfo& trackInfo = segmentProperties.trackInfos[trackId];

                        if (trackInfo.samples.size())
                        {
                            auto& sample = *trackInfo.samples.rbegin();
                            // ignore missing data for samples
                            if (sample.dataOffset)
                            {
                                int64_t sampleDataEndOffset =
                                    static_cast<int64_t>(*sample.dataOffset + sample.dataLength);
                                if (sampleDataEndOffset > io.size || sampleDataEndOffset < 0)
                                {
                                    throw RuntimeError(
                                        "MP4VRFileReaderImpl::readStream sample data offset outside of movie "
                                        "fragment "
                                        "data");
                                }
                            }
                        }
                    }
                }
            }
            else if (boxType == "meta")
            {
                iterateError = readBox(io, bitstream);
                if (!*iterateError)
                {
                    mInitSegmentPropertiesMap[initSegmentId].meta = MetaBox();
                    mInitSegmentPropertiesMap[initSegmentId].meta->parseBox(bitstream);
                }
            }
            else if (boxType == "mdat")
            {
                // skip mdat as its handled elsewhere
            }
            else
            {
                logWarning() << "Skipping root level box of unknown type '" << boxType << "'" << std::endl;
            }
            return iterateError;
        });

        if (!error && (!ftypFound || !moovFound))
        {
            error = ErrorCode::FILE_HEADER_ERROR;
        }

        if (!error)
        {
            updateCompositionTimes(initSegmentId, segmentId);

            // peek() sets eof bit for the stream. Clear stream to make sure it is still accessible. seekg() in C++11
            // should clear stream after eof, but this does not seem to be always happening.
            if ((!io.stream->good()) && (!io.stream->eof()))
            {
                return ErrorCode::FILE_READ_ERROR;
            }
            io.stream->clear();
            mInitSegmentPropertiesMap[initSegmentId].fileFeature = getFileFeatures();
            mState                                               = State::READY;
        }
        else
        {
            invalidateInitializationSegment(initSegmentId.get());
        }
        return error;
    }


    MP4VRFileReaderImpl::ItemInfoMap MP4VRFileReaderImpl::extractItemInfoMap(const MetaBox& metaBox) const
    {
#ifndef DISABLE_UNCOVERED_CODE
        ItemInfoMap itemInfoMap;

        const std::uint32_t countNumberOfItems = metaBox.getItemInfoBox().getEntryCount();
        for (std::uint32_t i = 0; i < countNumberOfItems; ++i)
        {
            const ItemInfoEntry& item  = metaBox.getItemInfoBox().getItemInfoEntry(i);
            FourCCInt type             = item.getItemType();
            const std::uint32_t itemId = item.getItemID();
            if (!isImageItemType(type))
            {
                ItemInfo itemInfo;
                itemInfo.type = type.getString();
                itemInfoMap.insert({itemId, itemInfo});
            }
        }

        return itemInfoMap;
#endif  // DISABLE_UNCOVERED_CODE
    }

    FileFeature MP4VRFileReaderImpl::getFileFeatures() const
    {
        FileFeature fileFeature;

        for (const auto& initSegment : mInitSegmentPropertiesMap)
        {
            for (const auto& trackProperties : initSegment.second.trackProperties)
            {
                if (trackProperties.second.trackFeature.hasFeature(TrackFeatureEnum::Feature::HasAlternatives))
                {
                    fileFeature.setFeature(FileFeatureEnum::Feature::HasAlternateTracks);
                }
            }
        }

        return fileFeature;
    }

    int32_t MP4VRFileReaderImpl::readBytes(SegmentIO& io, const unsigned int count, std::int64_t& result)
    {
        std::int64_t value = 0;
        for (unsigned int i = 0; i < count; ++i)
        {
            value = (value << 8) | static_cast<int64_t>(io.stream->get());
            if (!io.stream->good())
            {
                return ErrorCode::FILE_READ_ERROR;
            }
        }

        result = value;
        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::readBoxParameters(SegmentIO& io, String& boxType, std::int64_t& boxSize)
    {
        const std::int64_t startLocation = io.stream->tell();

        // Read the 32-bit length field of the box
        int32_t error = readBytes(io, 4, boxSize);
        if (error)
        {
            return error;
        }

        // Read the four character string for boxType
        static const size_t TYPE_LENGTH = 4;
        boxType.resize(TYPE_LENGTH);
        io.stream->read(&boxType[0], TYPE_LENGTH);
        if (!io.stream->good())
        {
            return ErrorCode::FILE_READ_ERROR;
        }

        // Check if 64-bit largesize field is used
        if (boxSize == 1)
        {
            error = readBytes(io, 8, boxSize);
            if (error)
            {
                return error;
            }
        }

        int64_t boxEndOffset = startLocation + boxSize;
        if (boxSize < 8 || (boxEndOffset < 8) || ((io.size > 0) && (boxEndOffset > io.size)))
        {
            return ErrorCode::FILE_READ_ERROR;
        }

        // Seek to box beginning
        seekInput(io, startLocation);
        if (!io.stream->good())
        {
            return ErrorCode::FILE_READ_ERROR;
        }
        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::readBox(SegmentIO& io, BitStream& bitstream)
    {
        String boxType;
        std::int64_t boxSize = 0;

        int32_t error = readBoxParameters(io, boxType, boxSize);
        if (error)
        {
            return error;
        }

        Vector<uint8_t> data((std::uint64_t) boxSize);
        io.stream->read(reinterpret_cast<char*>(data.data()), boxSize);
        if (!io.stream->good())
        {
            return ErrorCode::FILE_READ_ERROR;
        }
        bitstream.clear();
        bitstream.reset();
        bitstream.write8BitsArray(data, std::uint64_t(boxSize));
        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::skipBox(SegmentIO& io)
    {
        const std::int64_t startLocation = io.stream->tell();

        String boxType;
        std::int64_t boxSize = 0;
        int32_t error        = readBoxParameters(io, boxType, boxSize);
        if (error)
        {
            return error;
        }

        seekInput(io, startLocation + boxSize);
        if (!io.stream->good())
        {
            return ErrorCode::FILE_READ_ERROR;
        }
        return ErrorCode::OK;
    }

    int MP4VRFileReaderImpl::getImageDimensions(unsigned int trackId,
                                                const std::uint32_t itemId,
                                                std::uint32_t& width,
                                                std::uint32_t& height) const
    {
        ContextType contextType;
        InitSegmentTrackId initSegTrackId = ofTrackId(trackId);
        InitSegmentId initSegmentId       = initSegTrackId.first;
        ItemId itemIdInternal(itemId);
        SegmentId segmentId;
        int result = segmentIdOf(initSegTrackId, itemIdInternal, segmentId);
        if (result != ErrorCode::OK)
        {
            return result;
        }
        SegmentTrackId segTrackId = std::make_pair(segmentId, initSegTrackId.second);

        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        switch (contextType)
        {
        case ContextType::META:
            height = mMetaBoxInfo.at(segTrackId).imageInfoMap.at(itemIdInternal).height;
            width  = mMetaBoxInfo.at(segTrackId).imageInfoMap.at(itemIdInternal).width;
            break;

        case ContextType::TRACK:
        {
            ItemId itemIdBase;
            auto& sampleInfoVector = getSampleInfo(initSegmentId, segTrackId, itemIdBase);
            auto& sampleInfo       = sampleInfoVector.at((itemIdInternal - itemIdBase).get());
            height                 = sampleInfo.height;
            width                  = sampleInfo.width;
            break;
        }

        default:
            return ErrorCode::INVALID_CONTEXT_ID;
        }
        return ErrorCode::OK;
    }

    int32_t MP4VRFileReaderImpl::getContextItems(InitSegmentTrackId initSegTrackId, IdVector& items) const
    {
        items.clear();
        auto trackId = initSegTrackId.second;
        for (const auto& segment : segmentsBySequence(initSegTrackId.first))
        {
            SegmentId segmentId       = segment.segmentId;
            SegmentTrackId segTrackId = std::make_pair(segmentId, trackId);
            if (getContextType(initSegTrackId) == ContextType::META)
            {
                items.reserve(mMetaBoxInfo.at(segTrackId).imageInfoMap.size());
                for (const auto& imageInfo : mMetaBoxInfo.at(segTrackId).imageInfoMap)
                {
                    items.push_back(imageInfo.first.get());
                }
            }
            else if (getContextType(initSegTrackId) == ContextType::TRACK)
            {
                if (hasTrackInfo(initSegTrackId.first, segTrackId))
                {
                    items.reserve(getTrackInfo(initSegTrackId.first, segTrackId).samples.size());
                    for (const auto& sampleInfo : getTrackInfo(initSegTrackId.first, segTrackId).samples)
                    {
                        items.push_back(sampleInfo.sampleId);
                    }
                }
            }
            else
            {
                return ErrorCode::INVALID_CONTEXT_ID;
            }
        }
        return ErrorCode::OK;
    }

    void MP4VRFileReaderImpl::getAvcItemData(const DataVector& rawItemData, DataVector& itemData)
    {
        BitStream bitstream(rawItemData);
        while (bitstream.numBytesLeft() > 0)
        {
            const unsigned int nalLength = bitstream.read32Bits();
            const uint8_t firstByte      = bitstream.read8Bits();
            //        AvcNalUnitType naluType = AvcNalUnitType(firstByte & 0x1f);

            itemData.push_back(0);  // Additional zero_byte required before parameter sets and the first NAL unit of the
                                    // frame or considered trailing zero byte of previous NAL
            itemData.push_back(0);
            itemData.push_back(0);
            itemData.push_back(1);

            itemData.push_back(firstByte);
            bitstream.read8BitsArray(itemData, nalLength - 1);
        }
    }

    void MP4VRFileReaderImpl::getHevcItemData(const DataVector& rawItemData, DataVector& itemData)
    {
        BitStream bitstream(rawItemData);
        while (bitstream.numBytesLeft() > 0)
        {
            const unsigned int nalLength = bitstream.read32Bits();
            const uint8_t firstByte      = bitstream.read8Bits();
            HevcNalUnitType naluType     = HevcNalUnitType((firstByte >> 1) & 0x3f);

            // Add start code before each NAL unit
            if (itemData.size() == 0 || naluType == HevcNalUnitType::VPS || naluType == HevcNalUnitType::SPS ||
                naluType == HevcNalUnitType::PPS)
            {
                itemData.push_back(
                    0);  // Additional zero_byte required before parameter sets and the first NAL unit of the frame
            }
            itemData.push_back(0);
            itemData.push_back(0);
            itemData.push_back(1);

            itemData.push_back(firstByte);
            bitstream.read8BitsArray(itemData, nalLength - 1);
        }
    }

    MP4VRFileReaderInterface::ErrorCode MP4VRFileReaderImpl::processAvcItemData(char* memoryBuffer,
                                                                                uint32_t& memoryBufferSize)
    {
        uint32_t outputOffset = 0;
        uint32_t byteOffset   = 0;
        uint32_t nalLength    = 0;

        while (outputOffset < memoryBufferSize)
        {
            nalLength                               = (uint8_t) memoryBuffer[outputOffset + byteOffset];
            memoryBuffer[outputOffset + byteOffset] = 0;
            byteOffset++;
            nalLength = (nalLength << 8) | (uint8_t) memoryBuffer[outputOffset + byteOffset];
            memoryBuffer[outputOffset + byteOffset] = 0;
            byteOffset++;
            nalLength = (nalLength << 8) | (uint8_t) memoryBuffer[outputOffset + byteOffset];
            memoryBuffer[outputOffset + byteOffset] = 0;
            byteOffset++;
            nalLength = (nalLength << 8) | (uint8_t) memoryBuffer[outputOffset + byteOffset];
            memoryBuffer[outputOffset + byteOffset] = 1;
            byteOffset++;
            // AvcNalUnitType naluType = AvcNalUnitType((uint8_t)memoryBuffer[outputOffset + byteOffset] & 0x1f);
            outputOffset += nalLength + 4;  // 4 bytes of nal length information
            byteOffset = 0;
        }
        return ErrorCode::OK;
    }

    MP4VRFileReaderInterface::ErrorCode MP4VRFileReaderImpl::processHevcItemData(char* memoryBuffer,
                                                                                 uint32_t& memoryBufferSize)
    {
        // Note! This inserts 4-byte start codes although in other than the 1st NAL the start code should be 3 bytes.
        // 4-bytes is easier to use here since NAL length is 4 bytes, so we don't need to do any data copying, but can
        // just replace the length field with start code However, decoders seem to be able to ignore the 1st 0-byte and
        // hence 4-byte works too

        uint32_t outputOffset = 0;
        uint32_t byteOffset   = 0;
        uint32_t nalLength    = 0;

        while (outputOffset < memoryBufferSize)
        {
            nalLength                               = (uint8_t) memoryBuffer[outputOffset + byteOffset];
            memoryBuffer[outputOffset + byteOffset] = 0;
            byteOffset++;
            nalLength = (nalLength << 8) | (uint8_t) memoryBuffer[outputOffset + byteOffset];
            memoryBuffer[outputOffset + byteOffset] = 0;
            byteOffset++;
            nalLength = (nalLength << 8) | (uint8_t) memoryBuffer[outputOffset + byteOffset];
            memoryBuffer[outputOffset + byteOffset] = 0;
            byteOffset++;
            nalLength = (nalLength << 8) | (uint8_t) memoryBuffer[outputOffset + byteOffset];
            memoryBuffer[outputOffset + byteOffset] = 1;
            byteOffset++;
            // HevcNalUnitType naluType = HevcNalUnitType(((uint8_t)memoryBuffer[outputOffset + byteOffset] >> 1) &
            // 0x3f);
            outputOffset += nalLength + 4;  // 4 bytes of nal length information
            byteOffset = 0;
        }
        return ErrorCode::OK;
    }

    void MP4VRFileReaderImpl::processDecoderConfigProperties(const InitSegmentTrackId /*initSegTrackId*/)
    {
// Decoder configuration gets special handling because that information is accessed by the reader
// implementation itself.
#if 0
        const ItemPropertiesBox& iprp = mMetaBoxMap.at(initSegTrackId).getItemPropertiesBox();

        for (const auto& imageProperties : mFileProperties.rootLevelMetaBoxProperties.imageFeaturesMap)
        {
            const ItemId imageId = imageProperties.first;

            // HVCC
            const std::uint32_t hvccIndex = iprp.findPropertyIndex(ItemPropertiesBox::PropertyType::HVCC, imageId);
            if (hvccIndex != 0)
            {
                mDecoderCodeTypeMap.insert(std::make_pair(Id(initSegTrackId, imageId), FourCCInt("hvc1"))); // Store decoder type for meta data decoding

                mSegmentPropertiesMap[initSegTrackId.first].itemToParameterSetMap.insert(std::make_pair(Id(initSegTrackId, imageId), Id(initSegTrackId, hvccIndex)));
                const HevcDecoderConfigurationRecord record =
                    iprp.getPropertyByIndex<HevcConfigurationBox>(hvccIndex - 1)->getConfiguration();
                const ParameterSetMap parameterSetMap = makeDecoderParameterSetMap(record);
                mParameterSetMap.insert(std::make_pair(Id(initSegTrackId, hvccIndex), parameterSetMap));
            }

            // AVCC
            const std::uint32_t avccIndex = iprp.findPropertyIndex(ItemPropertiesBox::PropertyType::AVCC, imageId);
            if (avccIndex != 0)
            {
                mDecoderCodeTypeMap.insert(std::make_pair(Id(initSegTrackId, imageId), FourCCInt("avc1"))); // Store decoder type for meta data decoding

                mSegmentPropertiesMap[initSegTrackId.first].itemToParameterSetMap.insert(std::make_pair(Id(initSegTrackId, imageId), Id(initSegTrackId, avccIndex)));
                const AvcDecoderConfigurationRecord record =
                    iprp.getPropertyByIndex<AvcConfigurationBox>(avccIndex - 1)->getConfiguration();
                const ParameterSetMap parameterSetMap = makeDecoderParameterSetMap(record);
                mParameterSetMap.insert(std::make_pair(Id(initSegTrackId, avccIndex), parameterSetMap));
            }
        }
#endif
    }

    /* *********************************************************************** */
    /* *********************** Track-specific methods  *********************** */
    /* *********************************************************************** */

    void MP4VRFileReaderImpl::updateDecoderCodeTypeMap(InitSegmentId initializationSegmentId,
                                                       SegmentTrackId segTrackId,
                                                       const SampleInfoVector& sampleInfo,
                                                       std::size_t prevSampleInfoSize)
    {
        auto& decoderCodeTypeMap = mInitSegmentPropertiesMap.at(initializationSegmentId)
                                       .segmentPropertiesMap.at(segTrackId.first)
                                       .trackInfos.at(segTrackId.second)
                                       .decoderCodeTypeMap;
        for (std::size_t sampleIndex = prevSampleInfoSize; sampleIndex < sampleInfo.size(); ++sampleIndex)
        {
            auto& info = sampleInfo[sampleIndex];
            decoderCodeTypeMap.insert(
                std::make_pair(info.sampleId, info.sampleEntryType));  // Store decoder type for track data decoding
        }
    }

    void MP4VRFileReaderImpl::updateItemToParametersSetMap(ItemToParameterSetMap& itemToParameterSetMap,
                                                           InitSegmentTrackId initSegTrackId,
                                                           const SampleInfoVector& sampleInfo,
                                                           std::size_t prevSampleInfoSize)
    {
        for (std::size_t sampleIndex = prevSampleInfoSize; sampleIndex < sampleInfo.size(); ++sampleIndex)
        {
            auto itemId            = Id(initSegTrackId, (std::uint32_t) sampleIndex);
            auto parameterSetMapId = sampleInfo[sampleIndex].sampleDescriptionIndex;
            itemToParameterSetMap.insert(std::make_pair(itemId, parameterSetMapId));
        }
    }

    void MP4VRFileReaderImpl::addSegmentSequence(InitSegmentId initializationSegmentId,
                                                 SegmentId segmentId,
                                                 Sequence sequence)
    {
        SegmentProperties& segmentProperties =
            mInitSegmentPropertiesMap.at(initializationSegmentId).segmentPropertiesMap[segmentId];
        segmentProperties.sequences.insert(sequence);
        auto& sequenceToSegment = mInitSegmentPropertiesMap.at(initializationSegmentId).sequenceToSegment;
        sequenceToSegment.insert(std::make_pair(sequence, segmentId));
    }

    FourCCTrackReferenceInfoMap
    MP4VRFileReaderImpl::trackGroupsOfTrackProperties(const TrackPropertiesMap& aTrackProperties) const
    {
        FourCCTrackReferenceInfoMap trgrMap;

        for (const auto& trackIdProperties : aTrackProperties)
        {
            ContextId refTrackId                   = trackIdProperties.first;
            const TrackProperties& trackProperties = trackIdProperties.second;

            // why dereference trgrMap so often? if we created the element up-front, we'd end up with empty track group
            // data structures

            auto scalIt = trackProperties.referenceTrackIds.find("scal");
            if (scalIt != trackProperties.referenceTrackIds.end())
            {
                const ContextIdVector& scal = scalIt->second;
                for (ContextId reference : scal)
                {
                    // problem? Maybe we could use solely this information to find the tracks.
                    TrackReferenceInfo& info = trgrMap["alte"][reference];
                    info.extractorTrackIds.insert(refTrackId);
                }
            }

            for (const auto& fourCCTrackGroupInfo : trackProperties.trackGroupInfoMap)
            {
                FourCCInt fourcc           = fourCCTrackGroupInfo.first;
                const TrackGroupInfo& info = fourCCTrackGroupInfo.second;
                for (std::uint32_t id : info.ids)
                {
                    trgrMap[fourcc][ContextId(id)].trackIds.insert(refTrackId);
                }
            }
        }

        return trgrMap;
    }

    TrackPropertiesMap MP4VRFileReaderImpl::fillTrackProperties(InitSegmentId initSegmentId,
                                                                SegmentId segmentId,
                                                                MovieBox& moovBox)
    {
        TrackPropertiesMap trackPropertiesMap;

        Vector<TrackBox*> trackBoxes = moovBox.getTrackBoxes();
        for (auto trackBox : trackBoxes)
        {
            TrackProperties trackProperties;
            std::pair<InitTrackInfo, TrackInfo> initAndTrackInfo =
                extractTrackInfo(trackBox, moovBox.getMovieHeaderBox().getTimeScale());
            InitTrackInfo& initTrackInfo = initAndTrackInfo.first;
            TrackInfo& trackInfo         = initAndTrackInfo.second;

            // only store information about tracks we actually understand.
            if (initTrackInfo.sampleEntryType == "hvc1" || initTrackInfo.sampleEntryType == "hev1" ||
                initTrackInfo.sampleEntryType == "hvc2" || initTrackInfo.sampleEntryType == "avc1" ||
                initTrackInfo.sampleEntryType == "avc3" || initTrackInfo.sampleEntryType == "mp4a" ||
                initTrackInfo.sampleEntryType == "urim" || initTrackInfo.sampleEntryType == "mp4v" ||
                initTrackInfo.sampleEntryType == "invo" || initTrackInfo.sampleEntryType == "dyol" ||
                initTrackInfo.sampleEntryType == "dyvp" || initTrackInfo.sampleEntryType == "invp" ||
                initTrackInfo.sampleEntryType == "rcvp")
            {
                ContextId contextId               = ContextId(trackBox->getTrackHeaderBox().getTrackID());
                InitSegmentTrackId initSegTrackId = std::make_pair(initSegmentId, contextId);
                SegmentTrackId segTrackId         = std::make_pair(segmentId, contextId);

                trackInfo.samples           = makeSampleInfoVector(trackBox);
                auto& initSegmentProperties = mInitSegmentPropertiesMap.at(initSegmentId);
                updateItemToParametersSetMap(
                    initSegmentProperties.segmentPropertiesMap[segmentId].itemToParameterSetMap, initSegTrackId,
                    trackInfo.samples);

                auto& storedTrackInfo =
                    initSegmentProperties.segmentPropertiesMap[segmentId].trackInfos[initSegTrackId.second];
                storedTrackInfo = std::move(trackInfo);
                mInitSegmentPropertiesMap[initSegmentId].initTrackInfos[initSegTrackId.second] =
                    std::move(initTrackInfo);

                fillSampleEntryMap(trackBox, initSegmentId);
                updateDecoderCodeTypeMap(initSegmentId, segTrackId, storedTrackInfo.samples);

                trackProperties.trackFeature      = getTrackFeatures(trackBox);
                trackProperties.referenceTrackIds = getReferenceTrackIds(trackBox);
                trackProperties.trackGroupInfoMap = getTrackGroupInfoMap(trackBox);
                trackProperties.alternateTrackIds = getAlternateTrackIds(trackBox, moovBox);
                trackProperties.alternateGroupId  = trackBox->getTrackHeaderBox().getAlternateGroup();
                trackProperties.dataEntries       = getSnimDataEntries(*trackBox);
                if (trackBox->getEditBox().get() && trackBox->getEditBox()->getEditListBox())
                {
                    trackProperties.editBox = trackBox->getEditBox();
                }

                ContextInfo contextInfo;
                contextInfo.contextType         = ContextType::TRACK;
                mContextInfoMap[initSegTrackId] = contextInfo;

                trackPropertiesMap.insert(std::make_pair(initSegTrackId.second, std::move(trackProperties)));
            }
            else
            {
                // otherwise later on we assume initTrackInfo field for this track exists when it in actuality doesn't.
            }
        }

        // Some TrackFeatures are easiest to set after first round of properties have already been filled.
        for (auto& trackProperties : trackPropertiesMap)
        {
            if (trackProperties.second.trackFeature.hasFeature(TrackFeatureEnum::Feature::IsVideoTrack))
            {
                for (auto& associatedTrack : trackProperties.second.referenceTrackIds["vdep"])
                {
                    if (trackPropertiesMap.count(associatedTrack))
                    {
                        trackPropertiesMap[associatedTrack].trackFeature.setFeature(
                            TrackFeatureEnum::Feature::HasAssociatedDepthTrack);
                    }
                }
            }
        }

        return trackPropertiesMap;
    }

    ItemId MP4VRFileReaderImpl::getFollowingItemId(InitSegmentId initializationSegmentId,
                                                   SegmentTrackId segTrackId) const
    {
        SegmentId segmentId = segTrackId.first;
        ContextId trackId   = segTrackId.second;
        ItemId nextItemIdBase;
        if (mInitSegmentPropertiesMap.count(initializationSegmentId) &&
            mInitSegmentPropertiesMap.at(initializationSegmentId).segmentPropertiesMap.count(segmentId))
        {
            auto& segmentProperties =
                mInitSegmentPropertiesMap.at(initializationSegmentId).segmentPropertiesMap.at(segmentId);
            if (segmentProperties.trackInfos.count(trackId))
            {
                auto& trackInfo = segmentProperties.trackInfos.at(trackId);
                if (trackInfo.samples.size())
                {
                    nextItemIdBase = trackInfo.samples.rbegin()->sampleId + 1;
                }
                else
                {
                    nextItemIdBase = trackInfo.itemIdBase;
                }
            }
        }
        return nextItemIdBase;
    }

    ItemId MP4VRFileReaderImpl::getPrecedingItemId(InitSegmentId initializationSegmentId,
                                                   SegmentTrackId segTrackId) const
    {
        SegmentId precedingSegmentId;
        ContextId trackId = segTrackId.second;
        ItemId itemId;
        SegmentId curSegmentId = segTrackId.first;

        // Go backwards segments till we find an item id, or we run out of segments
        while (getPrecedingSegment(initializationSegmentId, curSegmentId, precedingSegmentId) && itemId == ItemId(0))
        {
            if (mInitSegmentPropertiesMap.at(initializationSegmentId)
                    .segmentPropertiesMap.at(precedingSegmentId)
                    .trackInfos.count(trackId))
            {
                itemId = getFollowingItemId(initializationSegmentId, SegmentTrackId(precedingSegmentId, trackId));
            }
            curSegmentId = precedingSegmentId;
        }
        return itemId;
    }

    void MP4VRFileReaderImpl::addToTrackProperties(InitSegmentId initSegmentId,
                                                   SegmentId segmentId,
                                                   MovieFragmentBox& moofBox,
                                                   const ContextIdPresentationTimeTSMap& earliestPTSTS)
    {
        InitSegmentProperties& initSegmentProperties = mInitSegmentPropertiesMap[initSegmentId];
        SegmentProperties& segmentProperties         = initSegmentProperties.segmentPropertiesMap[segmentId];

        addSegmentSequence(initSegmentId, segmentId, mNextSequence);
        mNextSequence = mNextSequence.get() + 1;

        ISOBMFF::Optional<std::uint64_t> trackFragmentSampleDataOffset = 0;
        bool firstTrackFragment                                        = true;
        ISOBMFF::Optional<ImdaId> metaImda                             = getImdaIdFromMoofMeta(moofBox);

        Vector<TrackFragmentBox*> trackFragmentBoxes = moofBox.getTrackFragmentBoxes();
        for (auto& trackFragmentBox : trackFragmentBoxes)
        {
            auto trackId = ContextId(trackFragmentBox->getTrackFragmentHeaderBox().getTrackId());

            // maybe it's possible trackInfos gets updates in this loop, so we cannot use a direct pointer
            ISOBMFF::Optional<ContextId> baseTrackId;

            // wow
            if (initSegmentProperties.trackProperties.at(trackId).trackGroupInfoMap.count("alte") &&
                initSegmentProperties.trackProperties.at(trackId).trackGroupInfoMap.at("alte").ids.size())
            {
                ContextId trackGroup = ContextId(
                    initSegmentProperties.trackProperties.at(trackId).trackGroupInfoMap.at("alte").ids.front());
                if (initSegmentProperties.trackReferences.count("alte") &&
                    initSegmentProperties.trackReferences.at("alte").count(trackGroup) &&
                    initSegmentProperties.trackReferences.at("alte").at(trackGroup).extractorTrackIds.size())
                {
                    // for this purpose (item id base definition) it doesn't matter which track we choose
                    baseTrackId =
                        *initSegmentProperties.trackReferences.at("alte").at(trackGroup).extractorTrackIds.begin();
                    // maybe the track doesn't exist? then it's uncleanly caught by a later .at using the track
                }
            }

            SegmentId precedingSegmentId;
            // item id base in case of adding multiple subsegments?!
            TrackInfo& trackInfo      = segmentProperties.trackInfos[trackId];
            size_t prevSampleInfoSize = trackInfo.samples.size();
            bool hasSamples           = prevSampleInfoSize > 0;
            if (auto* timeBox = trackFragmentBox->getTrackFragmentBaseMediaDecodeTimeBox())
            {
                trackInfo.nextPTSTS = DecodePts::PresentationTimeTS(timeBox->getBaseMediaDecodeTime());
            }
            else if (!hasSamples)
            {
                auto it = earliestPTSTS.find(trackId);
                if (it != earliestPTSTS.end())
                {
                    trackInfo.nextPTSTS = it->second;
                }
                else
                {
                    trackInfo.nextPTSTS = 0;
                }
            }
            ItemId segmentItemIdBase =
                baseTrackId && segmentProperties.trackInfos.count(*baseTrackId) // HACK. why doesn't baseTrackId exist?
                    ? segmentProperties.trackInfos.at(*baseTrackId).itemIdBase
                    : hasSamples ? trackInfo.itemIdBase
                                 : getPrecedingItemId(initSegmentId, SegmentTrackId(segmentId, ContextId(trackId)));
            InitSegmentTrackId initSegTrackId  = std::make_pair(initSegmentId, trackId);
            const InitTrackInfo& initTrackInfo = getInitTrackInfo(initSegTrackId);
            uint32_t sampleDescriptionIndex = trackFragmentBox->getTrackFragmentHeaderBox().getSampleDescriptionIndex();

            Vector<TrackRunBox*> trackRunBoxes = trackFragmentBox->getTrackRunBoxes();
            ISOBMFF::Optional<ImdaId> mdiaImda = getImdaIdFromDataEntries(initSegmentProperties.trackProperties.at(trackId).dataEntries, moofBox);
            ISOBMFF::Optional<ImdaId> imda = mdiaImda ? mdiaImda : metaImda ? metaImda : ISOBMFF::Optional<ImdaId>{};
            for (const TrackRunBox* trackRunBox : trackRunBoxes)
            {
                ItemId trackrunItemIdBase =
                    trackInfo.samples.size() > 0 ? trackInfo.samples.back().sampleId + 1 : segmentItemIdBase;
                // figure out what is the base data offset for the samples in this trun box:
                Optional<std::uint64_t> baseDataOffset = {0};
                if (imda)
                {
                    if (segmentProperties.imda.count(*imda))
                    {
                        const ImdaInfo& imdaInfo = segmentProperties.imda.at(*imda);
                        baseDataOffset           = std::uint64_t(imdaInfo.begin);
                    }
                    else
                    {
                        baseDataOffset = {};
                    }
                }
                else if ((trackFragmentBox->getTrackFragmentHeaderBox().getFlags() &
                          TrackFragmentHeaderBox::BaseDataOffsetPresent) != 0)
                {
                    baseDataOffset = trackFragmentBox->getTrackFragmentHeaderBox().getBaseDataOffset();
                }
                else if ((trackFragmentBox->getTrackFragmentHeaderBox().getFlags() &
                          TrackFragmentHeaderBox::DefaultBaseIsMoof) != 0)
                {
                    baseDataOffset = moofBox.getMoofFirstByteOffset();
                }
                else
                {
                    if (firstTrackFragment)
                    {
                        baseDataOffset = moofBox.getMoofFirstByteOffset();
                    }
                    else
                    {
                        // use previous trackfragment last sample end as data offset for this trackfragment
                        baseDataOffset = trackFragmentSampleDataOffset;
                    }
                }
                if ((trackRunBox->getFlags() & TrackRunBox::DataOffsetPresent) != 0 && baseDataOffset)
                {
                    *baseDataOffset += std::uint32_t(trackRunBox->getDataOffset());
                }

                // @note: if we need to support more than one trun for track we need to calculate last sample PTS +
                // duration as new basePTSOffset for addSamplesToTrackInfo for next trun in same track. Need to take
                // care of case where the might be several different tracks inside same segment. using
                // segmentProperties.earliestPTSTS for all truns works for now as example clip only have 1 moof, 1x traf
                // for each track with 1 trun each.
                addSamplesToTrackInfo(trackInfo, initSegmentProperties, initTrackInfo,
                                      initSegmentProperties.trackProperties.at(trackId), baseDataOffset,
                                      sampleDescriptionIndex, segmentItemIdBase, trackrunItemIdBase, trackRunBox);
            }
            trackInfo.itemIdBase      = segmentItemIdBase;
            SegmentTrackId segTrackId = std::make_pair(segmentId, trackId);
            updateDecoderCodeTypeMap(initSegmentId, segTrackId, trackInfo.samples, prevSampleInfoSize);
            updateItemToParametersSetMap(
                mInitSegmentPropertiesMap[initSegmentId].segmentPropertiesMap[segmentId].itemToParameterSetMap,
                initSegTrackId, trackInfo.samples, prevSampleInfoSize);

            if (trackInfo.samples.size())
            {
                // update sample data offset in case it is needed to read next track fragment data offsets (base offset
                // not defined)
                auto& sample = *trackInfo.samples.rbegin();
                if (sample.dataOffset && trackFragmentSampleDataOffset)
                {
                    *trackFragmentSampleDataOffset =
                        *sample.dataOffset + sample.dataLength;
                }
                else
                {
                    trackFragmentSampleDataOffset = {};
                }
            }
            firstTrackFragment = false;
        }
    }

    void MP4VRFileReaderImpl::addSamplesToTrackInfo(TrackInfo& trackInfo,
                                                    const InitSegmentProperties& initSegmentProperties,
                                                    const InitTrackInfo& initTrackInfo,
                                                    const TrackProperties& trackProperties,
                                                    const ISOBMFF::Optional<std::uint64_t> baseDataOffset,
                                                    const uint32_t sampleDescriptionIndex,
                                                    ItemId itemIdBase,
                                                    ItemId trackrunItemIdBase,
                                                    const TrackRunBox* trackRunBox)
    {
        const Vector<TrackRunBox::SampleDetails>& samples = trackRunBox->getSampleDetails();
        const uint32_t sampleCount                        = static_cast<uint32_t>(samples.size());
        const DecodePts::SampleIndex itemIdOffset         = trackrunItemIdBase.get() - itemIdBase.get();

        // figure out PTS
        DecodePts decodePts;
        if (trackProperties.editBox)
        {
            trackInfo.hasEditList = true;
            decodePts.loadBox(trackProperties.editBox->getEditListBox(), initSegmentProperties.movieTimeScale,
                              initTrackInfo.timeScale);
        }
        decodePts.loadBox(trackRunBox);
        decodePts.unravelTrackRun();
        DecodePts::PMap localPMap;
        DecodePts::PMapTS localPMapTS;
        decodePts.applyLocalTime(static_cast<std::uint64_t>(trackInfo.nextPTSTS));
        decodePts.getTimeTrackRun(initTrackInfo.timeScale, localPMap);
        decodePts.getTimeTrackRunTS(localPMapTS);
        for (const auto& mapping : localPMap)
        {
            trackInfo.pMap.insert(std::make_pair(mapping.first, mapping.second + itemIdOffset));
        }
        for (const auto& mapping : localPMapTS)
        {
            trackInfo.pMapTS.insert(std::make_pair(mapping.first, mapping.second + itemIdOffset));
        }

        std::int64_t durationTS        = 0;
        auto sampleDataOffset = baseDataOffset;
        for (std::uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
        {
            SampleInfo sampleInfo{};
            sampleInfo.sampleId               = (trackrunItemIdBase + ItemId(sampleIndex)).get();
            sampleInfo.sampleEntryType        = initTrackInfo.sampleEntryType;
            sampleInfo.sampleDescriptionIndex = sampleDescriptionIndex;
            // sampleInfo.compositionTimes is filled at end of segment parsing (in ::parseSegment)
            sampleInfo.dataOffset = sampleDataOffset;
            sampleInfo.dataLength = samples.at(sampleIndex).version0.sampleSize;
            if (sampleDataOffset)
            {
                *sampleDataOffset += sampleInfo.dataLength;
            }
            if (initTrackInfo.sampleSizeInPixels.count(sampleDescriptionIndex))
            {
                sampleInfo.width  = initTrackInfo.sampleSizeInPixels.at(sampleDescriptionIndex).width;
                sampleInfo.height = initTrackInfo.sampleSizeInPixels.at(sampleDescriptionIndex).height;
            }
            else
            {
                sampleInfo.width  = 0;
                sampleInfo.height = 0;
            }
            sampleInfo.sampleDuration = samples.at(sampleIndex).version0.sampleDuration;
            sampleInfo.sampleType = samples.at(sampleIndex).version0.sampleFlags.flags.sample_is_non_sync_sample == 0
                                        ? OUTPUT_REFERENCE_FRAME
                                        : OUTPUT_NON_REFERENCE_FRAME;
            sampleInfo.sampleFlags.flagsAsUInt = samples.at(sampleIndex).version0.sampleFlags.flagsAsUInt;
            durationTS += samples.at(sampleIndex).version0.sampleDuration;
            trackInfo.samples.push_back(sampleInfo);
        }
        trackInfo.durationTS += durationTS;
        trackInfo.nextPTSTS += durationTS;
    }

    IdVector MP4VRFileReaderImpl::getAlternateTrackIds(TrackBox* trackBox, MovieBox& moovBox) const
    {
        IdVector trackIds;
        const std::uint16_t alternateGroupId = trackBox->getTrackHeaderBox().getAlternateGroup();

        if (alternateGroupId == 0)
        {
            return trackIds;
        }

        unsigned int trackId         = trackBox->getTrackHeaderBox().getTrackID();
        Vector<TrackBox*> trackBoxes = moovBox.getTrackBoxes();
        for (auto trackbox : trackBoxes)
        {
            const std::uint32_t alternateTrackId = trackbox->getTrackHeaderBox().getTrackID();
            if ((trackId != alternateTrackId) &&
                (alternateGroupId == trackbox->getTrackHeaderBox().getAlternateGroup()))
            {
                trackIds.push_back(alternateTrackId);
            }
        }

        return trackIds;
    }

    TrackFeature MP4VRFileReaderImpl::getTrackFeatures(TrackBox* trackBox) const
    {
        TrackFeature trackFeature;

        TrackHeaderBox tkhdBox        = trackBox->getTrackHeaderBox();
        HandlerBox& handlerBox        = trackBox->getMediaBox().getHandlerBox();
        SampleTableBox& stblBox       = trackBox->getMediaBox().getMediaInformationBox().getSampleTableBox();
        SampleDescriptionBox& stsdBox = stblBox.getSampleDescriptionBox();

        if (tkhdBox.getAlternateGroup() != 0)
        {
            trackFeature.setFeature(TrackFeatureEnum::Feature::HasAlternatives);
        }

        // hasSampleGroups
        if (stblBox.getSampleToGroupBoxes().size() != 0)
        {
            trackFeature.setFeature(TrackFeatureEnum::Feature::HasSampleGroups);
        }

        if (handlerBox.getHandlerType() == "vide")
        {
            trackFeature.setFeature(TrackFeatureEnum::Feature::IsVideoTrack);

            const Vector<VisualSampleEntryBox*> sampleEntries = stsdBox.getSampleEntries<VisualSampleEntryBox>();
            for (const auto& sampleEntry : sampleEntries)
            {
                if (sampleEntry->isStereoscopic3DBoxPresent())
                {
                    trackFeature.setVRFeature(TrackFeatureEnum::VRFeature::HasVRGoogleStereoscopic3D);
                }
                if (sampleEntry->isSphericalVideoV2BoxBoxPresent())
                {
                    trackFeature.setVRFeature(TrackFeatureEnum::VRFeature::HasVRGoogleV2SpericalVideo);
                }
            }

            if (trackBox->getHasSphericalVideoV1Box())
            {
                trackFeature.setVRFeature(TrackFeatureEnum::VRFeature::HasVRGoogleV1SpericalVideo);
                if (trackBox->getSphericalVideoV1Box().getGlobalMetadata().stereoMode !=
                    SphericalVideoV1Box::V1StereoMode::UNDEFINED)
                {
                    trackFeature.setVRFeature(TrackFeatureEnum::VRFeature::HasVRGoogleStereoscopic3D);
                }
            }
        }

        if (handlerBox.getHandlerType() == "soun")
        {
            trackFeature.setFeature(TrackFeatureEnum::Feature::IsAudioTrack);

            const Vector<MP4AudioSampleEntryBox*> sampleEntries = stsdBox.getSampleEntries<MP4AudioSampleEntryBox>();
            for (const auto& sampleEntry : sampleEntries)
            {
                if (sampleEntry->getType() == "mp4a")
                {
                    if (sampleEntry->hasChannelLayoutBox())
                    {
                        if (sampleEntry->getChannelLayoutBox().getStreamStructure() == 1)  // = channelStructured
                        {
                            trackFeature.setVRFeature(TrackFeatureEnum::VRFeature::IsAudioLSpeakerChnlStructTrack);
                        }
                    }
                    if (sampleEntry->hasSpatialAudioBox())
                    {
                        trackFeature.setVRFeature(TrackFeatureEnum::VRFeature::IsVRGoogleSpatialAudioTrack);
                    }
                    if (sampleEntry->hasNonDiegeticAudioBox())
                    {
                        trackFeature.setVRFeature(TrackFeatureEnum::VRFeature::IsVRGoogleNonDiegeticAudioTrack);
                    }
                    break;
                }
            }
        }

        if (handlerBox.getHandlerType() == "meta")
        {
            trackFeature.setFeature(TrackFeatureEnum::Feature::IsMetadataTrack);

            const Vector<MetaDataSampleEntryBox*> sampleEntries = stsdBox.getSampleEntries<MetaDataSampleEntryBox>();
            for (const auto& sampleEntry : sampleEntries)
            {
                if (sampleEntry->getType() == "urim")
                {
                    UriMetaSampleEntryBox* uriMetaSampleEntry  = (UriMetaSampleEntryBox*) sampleEntry;
                    UriMetaSampleEntryBox::VRTMDType vrTMDType = uriMetaSampleEntry->getVRTMDType();

                    switch (vrTMDType)
                    {
                    case UriMetaSampleEntryBox::VRTMDType::UNKNOWN:
                    default:
                        break;
                    }
                    break;
                }
            }
        }
        return trackFeature;
    }

    TypeToContextIdsMap MP4VRFileReaderImpl::getReferenceTrackIds(TrackBox* trackBox) const
    {
        TypeToContextIdsMap trackReferenceMap;

        if (trackBox->getHasTrackReferences())
        {
            const Vector<TrackReferenceTypeBox>& trackReferenceTypeBoxes =
                trackBox->getTrackReferenceBox().getTrefTypeBoxes();
            for (const auto& trackReferenceTypeBox : trackReferenceTypeBoxes)
            {
                trackReferenceMap[trackReferenceTypeBox.getType()] = map<std::uint32_t, Vector, Vector, ContextId>(
                    trackReferenceTypeBox.getTrackIds(), [](std::uint32_t x) {
                        if ((x >> 16) != 0)
                        {
                            throw FileReaderException(ErrorCode::INVALID_CONTEXT_ID, "getTrackReferenceTrackIds");
                        }
                        return ContextId(x);
                    });
            }
        }

        return trackReferenceMap;
    }

    TrackGroupInfoMap MP4VRFileReaderImpl::getTrackGroupInfoMap(TrackBox* trackBox) const
    {
        TrackGroupInfoMap trackGroupInfoMap;

        if (trackBox->getHasTrackGroup())
        {
            const auto& trackGroupTypeBoxes = trackBox->getTrackGroupBox().getTrackGroupTypeBoxes();
            for (unsigned int i = 0; i < trackGroupTypeBoxes.size(); i++)
            {
                IdVector trackGroupID;
                const TrackGroupTypeBox& box = trackGroupTypeBoxes.at(i);
                trackGroupID.push_back(box.getTrackGroupId());
                TrackGroupInfo trackGroupInfo{};
                trackGroupInfo.ids = trackGroupID;
                trackGroupInfoMap[box.getType()] = trackGroupInfo;
            }
        }

        return trackGroupInfoMap;
    }

    TypeToIdsMap MP4VRFileReaderImpl::getSampleGroupIds(TrackBox* trackBox) const
    {
        Map<FourCCInt, IdVector> sampleGroupIdsMap;

        SampleTableBox& stblBox = trackBox->getMediaBox().getMediaInformationBox().getSampleTableBox();
        const Vector<SampleToGroupBox> sampleToGroupBoxes = stblBox.getSampleToGroupBoxes();
        for (const auto& sampleToGroupBox : sampleToGroupBoxes)
        {
            const unsigned int numberOfSamples = sampleToGroupBox.getNumberOfSamples();
            IdVector sampleIds(numberOfSamples);
            for (unsigned int i = 0; i < numberOfSamples; ++i)
            {
                if (sampleToGroupBox.getSampleGroupDescriptionIndex(i) != 0)
                {
                    sampleIds.push_back(i);
                }
            }
            sampleGroupIdsMap[sampleToGroupBox.getGroupingType()] = sampleIds;
        }

        return sampleGroupIdsMap;
    }

    std::pair<InitTrackInfo, TrackInfo> MP4VRFileReaderImpl::extractTrackInfo(TrackBox* trackBox,
                                                                              uint32_t movieTimescale) const
    {
        InitTrackInfo initTrackInfo;
        TrackInfo trackInfo;

        MediaHeaderBox& mdhdBox                = trackBox->getMediaBox().getMediaHeaderBox();
        SampleTableBox& stblBox                = trackBox->getMediaBox().getMediaInformationBox().getSampleTableBox();
        TrackHeaderBox& trackHeaderBox         = trackBox->getTrackHeaderBox();
        const TimeToSampleBox& timeToSampleBox = stblBox.getTimeToSampleBox();
        std::shared_ptr<const CompositionOffsetBox> compositionOffsetBox = stblBox.getCompositionOffsetBox();

        initTrackInfo.width  = trackHeaderBox.getWidth();
        initTrackInfo.height = trackHeaderBox.getHeight();

        SampleDescriptionBox& stsdBox = stblBox.getSampleDescriptionBox();
        FourCCInt handlerType         = trackBox->getMediaBox().getHandlerBox().getHandlerType();

        if (handlerType == "vide")
        {
            VisualSampleEntryBox* sampleEntry =
                stsdBox.getSampleEntry<VisualSampleEntryBox>(1);  // get 1 index as truns sampleEntryType wont care
            if (sampleEntry)
            {
                initTrackInfo.sampleEntryType = sampleEntry->getType();
            }
        }
        else if (handlerType == "soun")
        {
            AudioSampleEntryBox* sampleEntry =
                stsdBox.getSampleEntry<AudioSampleEntryBox>(1);  // get 1 index as truns sampleEntryType wont care
            if (sampleEntry)
            {
                initTrackInfo.sampleEntryType = sampleEntry->getType();
            }
        }
        else if (handlerType == "meta")
        {
            MetaDataSampleEntryBox* sampleEntry =
                stsdBox.getSampleEntry<MetaDataSampleEntryBox>(1);  // get 1 index as truns sampleEntryType wont care
            if (sampleEntry)
            {
                initTrackInfo.sampleEntryType = sampleEntry->getType();
            }
        }

        initTrackInfo.timeScale = mdhdBox.getTimeScale();  // The number of time units that pass in a second

        std::shared_ptr<const EditBox> editBox = trackBox->getEditBox();
        DecodePts decodePts;
        decodePts.loadBox(&timeToSampleBox);
        decodePts.loadBox(compositionOffsetBox.get());
        if (editBox)
        {
            trackInfo.hasEditList          = true;
            const EditListBox* editListBox = editBox->getEditListBox();
            decodePts.loadBox(editListBox, movieTimescale, mdhdBox.getTimeScale());
        }
        if (!decodePts.unravel())
        {
            throw FileReaderException(ErrorCode::FILE_HEADER_ERROR);
        }

        // Always generate track duration regardless of the informatoin in the header
        trackInfo.durationTS = DecodePts::PresentationTimeTS(decodePts.getSpan());
        trackInfo.pMap       = decodePts.getTime(initTrackInfo.timeScale);
        trackInfo.pMapTS     = decodePts.getTimeTS();

        // Read track type if exists
        if (trackBox->getHasTrackTypeBox())
        {
            trackInfo.hasTtyp = true;
            trackInfo.ttyp    = trackBox->getTrackTypeBox();
        }

        return std::make_pair(initTrackInfo, trackInfo);
    }

    SampleInfoVector MP4VRFileReaderImpl::makeSampleInfoVector(TrackBox* trackBox) const
    {
        SampleInfoVector sampleInfoVector;

        SampleTableBox& stblBox       = trackBox->getMediaBox().getMediaInformationBox().getSampleTableBox();
        SampleDescriptionBox& stsdBox = stblBox.getSampleDescriptionBox();
        SampleToChunkBox& stscBox     = stblBox.getSampleToChunkBox();
        ChunkOffsetBox& stcoBox       = stblBox.getChunkOffsetBox();
        SampleSizeBox& stszBox        = stblBox.getSampleSizeBox();
        TimeToSampleBox& sttsBox      = stblBox.getTimeToSampleBox();
        const FourCCInt handlerType   = trackBox->getMediaBox().getHandlerBox().getHandlerType();

        const Vector<std::uint32_t> sampleSizeEntries = stszBox.getEntrySize();
        const Vector<std::uint64_t> chunkOffsets      = stcoBox.getChunkOffsets();
        const Vector<std::uint32_t> sampleDeltas      = sttsBox.getSampleDeltas();

        const unsigned int sampleCount = stszBox.getSampleCount();

        if (sampleCount > sampleSizeEntries.size() || sampleCount > sampleDeltas.size())
        {
            throw FileReaderException(ErrorCode::FILE_HEADER_ERROR);
        }

        sampleInfoVector.reserve(sampleCount);

        std::uint32_t previousChunkIndex = 0;  // Index is 1-based so 0 will not be used.
        for (std::uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
        {
            SampleInfo sampleInfo{};
            std::uint32_t sampleDescriptionIndex = 0;
            if (!stscBox.getSampleDescriptionIndex(sampleIndex, sampleDescriptionIndex))
            {
                throw FileReaderException(ErrorCode::FILE_HEADER_ERROR);
            }

            // Set basic sample information
            sampleInfo.sampleId               = sampleIndex;
            sampleInfo.dataLength             = sampleSizeEntries.at(sampleIndex);
            sampleInfo.sampleDescriptionIndex = sampleDescriptionIndex;

            std::uint32_t chunkIndex = 0;
            if (!stscBox.getSampleChunkIndex(sampleIndex, chunkIndex))
            {
                throw FileReaderException(ErrorCode::FILE_HEADER_ERROR);
            }

            if (chunkIndex != previousChunkIndex)
            {
                sampleInfo.dataOffset = chunkOffsets.at(chunkIndex - 1);
                previousChunkIndex    = chunkIndex;
            }
            else if (auto offset = sampleInfoVector.back().dataOffset)
            {
                sampleInfo.dataOffset = *offset + sampleInfoVector.back().dataLength;
            }
            else
            {
                sampleInfo.dataOffset = {};
            }

            sampleInfo.sampleDuration = sampleDeltas.at(sampleIndex);

            // Set dimensions
            if (handlerType == "vide")
            {
                if (const VisualSampleEntryBox* sampleEntry =
                        stsdBox.getSampleEntry<VisualSampleEntryBox>(sampleDescriptionIndex))
                {
                    sampleInfo.width           = sampleEntry->getWidth();
                    sampleInfo.height          = sampleEntry->getHeight();
                    sampleInfo.sampleEntryType = sampleEntry->getType();

                    // set default sampletype to non-ref by default
                    sampleInfo.sampleType                                  = OUTPUT_NON_REFERENCE_FRAME;
                    sampleInfo.sampleFlags.flags.sample_is_non_sync_sample = 1;
                }
            }
            else if (handlerType == "soun")
            {
                if (const AudioSampleEntryBox* sampleEntry =
                        stsdBox.getSampleEntry<AudioSampleEntryBox>(sampleInfo.sampleDescriptionIndex.get()))
                {
                    sampleInfo.sampleEntryType = sampleEntry->getType();
                    // all samples are reference frames for audio:
                    sampleInfo.sampleType = OUTPUT_REFERENCE_FRAME;
                }
            }
            else if (handlerType == "meta")
            {
                if (const MetaDataSampleEntryBox* sampleEntry =
                        stsdBox.getSampleEntry<MetaDataSampleEntryBox>(sampleInfo.sampleDescriptionIndex.get()))
                {
                    sampleInfo.sampleEntryType = sampleEntry->getType();
                    // all samples are reference frames for :
                    sampleInfo.sampleType = OUTPUT_REFERENCE_FRAME;
                }
            }

            sampleInfoVector.push_back(sampleInfo);
        }

        if (stblBox.hasSyncSampleBox() && (handlerType == "vide"))
        {
            const Vector<std::uint32_t> syncSamples = stblBox.getSyncSampleBox().get()->getSyncSampleIds();
            for (unsigned int i = 0; i < syncSamples.size(); ++i)
            {
                std::uint32_t syncSample                               = syncSamples.at(i) - 1;
                auto& sampleInfo                                       = sampleInfoVector.at(syncSample);
                sampleInfo.sampleType                                  = OUTPUT_REFERENCE_FRAME;
                sampleInfo.sampleFlags.flags.sample_is_non_sync_sample = 0;
            }
        }

        return sampleInfoVector;
    }

    MoovProperties MP4VRFileReaderImpl::extractMoovProperties(const MovieBox& moovBox) const
    {
        MoovProperties moovProperties;
        moovProperties.fragmentDuration = 0;

        if (moovBox.isMetaBoxPresent())
        {
            moovProperties.moovFeature.setFeature(MoovFeature::HasMoovLevelMetaBox);
        }

        if (moovBox.isMovieExtendsBoxPresent())
        {
            const MovieExtendsBox* mvexBox = moovBox.getMovieExtendsBox();
            if (mvexBox->isMovieExtendsHeaderBoxPresent())
            {
                const MovieExtendsHeaderBox& mehdBox = mvexBox->getMovieExtendsHeaderBox();
                moovProperties.fragmentDuration      = mehdBox.getFragmentDuration();
            }

            moovProperties.fragmentSampleDefaults.clear();
            for (const auto& track : mvexBox->getTrackExtendsBoxes())
            {
                moovProperties.fragmentSampleDefaults.push_back(track->getFragmentSampleDefaults());
            }
        }

        return moovProperties;
    }

    template <typename SampleEntryBox, typename GetPropertyMap>
    void MP4VRFileReaderImpl::fillSampleProperties(
        InitSegmentTrackId initSegTrackId,
        const SampleDescriptionBox& stsdBox,
        GetPropertyMap getPropertyMap)
    {
        const Vector<SampleEntryBox*> sampleEntries = stsdBox.getSampleEntries<SampleEntryBox>();
        unsigned int index                          = 1;

        // There is 3 level of inheritance in overlay configuration, so some better API could be good to have
        // currently if one queries first for video track, then for metadata track and in the end read metadata
        // sample for certain time, client will be able to eventually resolve overlay paramters...
        for (auto& entry : sampleEntries)
        {
            getPropertyMap(getInitTrackInfo(initSegTrackId)).insert(std::make_pair(index, entry->getSampleEntry()));
            ++index;
        }
    }

    void MP4VRFileReaderImpl::fillSampleEntryMap(TrackBox* trackBox, InitSegmentId initSegmentId)
    {
        InitSegmentTrackId initSegTrackId =
            std::make_pair(initSegmentId, ContextId(trackBox->getTrackHeaderBox().getTrackID()));
        SampleDescriptionBox& stsdBox =
            trackBox->getMediaBox().getMediaInformationBox().getSampleTableBox().getSampleDescriptionBox();
        auto& parameterSetMaps =
            mInitSegmentPropertiesMap[initSegTrackId.first].initTrackInfos[initSegTrackId.second].parameterSetMaps;
        //        mInitSegmentPropertiesMap[initSegTrackId.first].initTrackInfos[initSegTrackId.second].nalLengthSizeMinus1

        // Process HevcSampleEntries
        {
            const Vector<HevcSampleEntry*> sampleEntries = stsdBox.getSampleEntries<HevcSampleEntry>();
            unsigned int index                           = 1;
            for (auto& entry : sampleEntries)
            {
                ParameterSetMap parameterSetMap =
                    makeDecoderParameterSetMap(entry->getHevcConfigurationBox().getConfiguration());
                parameterSetMaps.insert(SampleDescriptionIndex(index), parameterSetMap);

                const auto* clapBox = entry->getClap();
                if (clapBox != nullptr)
                {
                    logInfo() << "CleanApertureBox reading not implemented" << std::endl;
                }

                SampleSizeInPixels size = {entry->getWidth(), entry->getHeight()};
                getInitTrackInfo(initSegTrackId).sampleSizeInPixels.insert(std::make_pair(index, size));

                if (entry->isStereoscopic3DBoxPresent())
                {
                    getInitTrackInfo(initSegTrackId)
                        .st3dProperties.insert(std::make_pair(index, makest3d(entry->getStereoscopic3DBox())));
                }
                else if (trackBox->getHasSphericalVideoV1Box())
                {
                    getInitTrackInfo(initSegTrackId)
                        .st3dProperties.insert(std::make_pair(index, makest3d(trackBox->getSphericalVideoV1Box())));
                }

                if (entry->isSphericalVideoV2BoxBoxPresent())
                {
                    getInitTrackInfo(initSegTrackId)
                        .sv3dProperties.insert(std::make_pair(index, makesv3d(entry->getSphericalVideoV2BoxBox())));
                }
                else if (trackBox->getHasSphericalVideoV1Box())
                {
                    getInitTrackInfo(initSegTrackId)
                        .googleSphericalV1Properties.insert(
                            std::make_pair(index, makeSphericalVideoV1Property(trackBox->getSphericalVideoV1Box())));
                }

                fillAdditionalBoxInfoForSampleEntry(getInitTrackInfo(initSegTrackId), index, *entry);
                getInitTrackInfo(initSegTrackId)
                    .nalLengthSizeMinus1.insert(std::make_pair(
                        index, entry->getHevcConfigurationBox().getConfiguration().getLengthSizeMinus1()));

                ++index;
            }
        }

        // Process AvcSampleEntries
        {
            const Vector<AvcSampleEntry*> sampleEntries = stsdBox.getSampleEntries<AvcSampleEntry>();
            unsigned int index                          = 1;
            for (auto& entry : sampleEntries)
            {
                ParameterSetMap parameterSetMap =
                    makeDecoderParameterSetMap(entry->getAvcConfigurationBox().getConfiguration());
                parameterSetMaps.insert(index, parameterSetMap);

                const auto* clapBox = entry->getClap();
                if (clapBox != nullptr)
                {
                    logInfo() << "CleanApertureBox reading not implemented" << std::endl;
                }

                SampleSizeInPixels size = {entry->getWidth(), entry->getHeight()};
                getInitTrackInfo(initSegTrackId).sampleSizeInPixels.insert(std::make_pair(index, size));

                if (entry->isStereoscopic3DBoxPresent())
                {
                    getInitTrackInfo(initSegTrackId)
                        .st3dProperties.insert(std::make_pair(index, makest3d(entry->getStereoscopic3DBox())));
                }
                else if (trackBox->getHasSphericalVideoV1Box())
                {
                    getInitTrackInfo(initSegTrackId)
                        .st3dProperties.insert(std::make_pair(index, makest3d(trackBox->getSphericalVideoV1Box())));
                }
                if (entry->isSphericalVideoV2BoxBoxPresent())
                {
                    getInitTrackInfo(initSegTrackId)
                        .sv3dProperties.insert(std::make_pair(index, makesv3d(entry->getSphericalVideoV2BoxBox())));
                }
                else if (trackBox->getHasSphericalVideoV1Box())
                {
                    getInitTrackInfo(initSegTrackId)
                        .googleSphericalV1Properties.insert(
                            std::make_pair(index, makeSphericalVideoV1Property(trackBox->getSphericalVideoV1Box())));
                }

                fillAdditionalBoxInfoForSampleEntry(getInitTrackInfo(initSegTrackId), index, *entry);
                getInitTrackInfo(initSegTrackId)
                    .nalLengthSizeMinus1.insert(std::make_pair(
                        index, entry->getAvcConfigurationBox().getConfiguration().getLengthSizeMinus1()));

                ++index;
            }
        }

        // Process MP4AudioSampleEntries
        {
            const Vector<MP4AudioSampleEntryBox*> sampleEntries = stsdBox.getSampleEntries<MP4AudioSampleEntryBox>();
            unsigned int index                                  = 1;
            for (auto& entry : sampleEntries)
            {
                ParameterSetMap parameterSetMap = makeDecoderParameterSetMap(entry->getESDBox());
                parameterSetMaps.insert(index, parameterSetMap);

                if (entry->hasChannelLayoutBox())
                {
                    getInitTrackInfo(initSegTrackId)
                        .chnlProperties.insert(std::make_pair(index, makeChnl(entry->getChannelLayoutBox())));
                }
                if (entry->hasSpatialAudioBox())
                {
                    getInitTrackInfo(initSegTrackId)
                        .sa3dProperties.insert(std::make_pair(index, makeSA3D(entry->getSpatialAudioBox())));
                }
                ++index;
            }
        }

        // Process directly written OverlaySampleEntries 'dyol' metadata sample entry (also read from povd for video
        // tracks)
        {
            const Vector<OverlaySampleEntryBox*> sampleEntries = stsdBox.getSampleEntries<OverlaySampleEntryBox>();
            unsigned int index                                 = 1;

            // There is 3 level of inheritance in overlay configuration, so some better API could be good to have
            // currently if one queries first for video track, then for metadata track and in the end read metadata
            // sample for certain time, client will be able to eventually resolve overlay paramters...
            for (auto& entry : sampleEntries)
            {
                auto& ovly = entry->getOverlayConfig();
                OverlayConfigProperty ovlyProp;
                ovlyProp.readFromCommon(ovly.getOverlayStruct());
                getInitTrackInfo(initSegTrackId).ovlyProperties.insert(std::make_pair(index, ovlyProp));
                ++index;
            }
        }

        fillSampleProperties<DynamicViewpointSampleEntryBox>(initSegTrackId, stsdBox,
                                                             [](InitTrackInfo& i) { return i.dyvpProperties; });

        fillSampleProperties<InitialViewpointSampleEntryBox>(initSegTrackId, stsdBox,
                                                             [](InitTrackInfo& i) { return i.invpProperties; });

        {
            const auto sampleEntries = stsdBox.getSampleEntries<RecommendedViewportSampleEntryBox>();
            unsigned int index       = 1;

            for (auto& entry : sampleEntries)
            {
                RecommendedViewportProperty property {};
                property.recommendedViewportInfo = entry->getRecommendedViewportInfo().get();
                property.sphereRegionConfig = entry->getSphereRegionConfig().get();
                getInitTrackInfo(initSegTrackId).rcvpProperties.insert(std::make_pair(index, property));
                ++index;
            }
        }
    }

    void MP4VRFileReaderImpl::fillAdditionalBoxInfoForSampleEntry(InitTrackInfo& trackInfo,
                                                                  unsigned int index,
                                                                  const SampleEntryBox& entry)
    {
        auto& visualEntry = dynamic_cast<const VisualSampleEntryBox&>(entry);
        if (visualEntry.getOverlayConfigBox())
        {
            auto ovly = visualEntry.getOverlayConfigBox();
            OverlayConfigProperty ovlyProp;
            ovlyProp.readFromCommon(ovly->getOverlayStruct());

            trackInfo.ovlyProperties.insert(std::make_pair(index, ovlyProp));
        }

        auto rinfBox = entry.getRestrictedSchemeInfoBox();
        if (rinfBox)
        {
            if (rinfBox->getSchemeType() == "podv")
            {
                auto& povdBox = rinfBox->getProjectedOmniVideoBox();

                ProjectionFormatProperty format = {
                    (OmafProjectionType) povdBox.getProjectionFormatBox().getProjectionType()};
                trackInfo.pfrmProperties.insert(std::make_pair(index, format));

                if (povdBox.hasRegionWisePackingBox())
                {
                    trackInfo.rwpkProperties.insert(std::make_pair(index, makerwpk(povdBox.getRegionWisePackingBox())));
                }

                if (povdBox.hasCoverageInformationBox())
                {
                    trackInfo.coviProperties.insert(
                        std::make_pair(index, makecovi(povdBox.getCoverageInformationBox())));
                }

                if (povdBox.hasRotationBox())
                {
                    auto rotation = povdBox.getRotationBox().getRotation();
                    trackInfo.rotnProperties.insert(
                        std::make_pair(index, RotationProperty{rotation.yaw, rotation.pitch, rotation.roll}));
                }
                else
                {
                    // default rotation if box not present
                    trackInfo.rotnProperties.insert(std::make_pair(index, RotationProperty{0, 0, 0}));
                }

                if (povdBox.hasOverlayConfigBox())
                {
                    auto& ovly = povdBox.getOverlayConfigBox();

                    OverlayConfigProperty ovlyProp;
                    ovlyProp.readFromCommon(ovly.getOverlayStruct());

                    trackInfo.ovlyProperties.insert(std::make_pair(index, ovlyProp));
                }
            }

            // rinf box has all the data about scheme types / compatible scheme types
            if (rinfBox->hasSchemeTypeBox())
            {
                SchemeTypesPropertyInternal schemeTypes{};
                schemeTypes.mainScheme = rinfBox->getSchemeTypeBox();
                for (auto compatSchemeType : rinfBox->getCompatibleSchemeTypes())
                {
                    schemeTypes.compatibleSchemes.push_back(*compatSchemeType);
                }
                trackInfo.schemeTypesProperties.insert(std::make_pair(index, schemeTypes));
            }

            // if stereovideo box was found, read it
            if (rinfBox->hasStereoVideoBox())
            {
                PodvStereoVideoConfiguration stviArrangement;
                stviArrangement = (PodvStereoVideoConfiguration) rinfBox->getStereoVideoBox()
                                      .getStereoIndicationType()
                                      .Povd.compositionType;
                trackInfo.stviProperties.insert(std::make_pair(index, stviArrangement));
            }
            else if (rinfBox->getSchemeType() == "podv")
            {
                // if not found, but scheme was podv, use default value as per OMAF 7.6.1.2
                trackInfo.stviProperties.insert(std::make_pair(index, PodvStereoVideoConfiguration::MONOSCOPIC));
            }
        }
    }

    int MP4VRFileReaderImpl::addDecodingDependencies(InitSegmentId initSegmentId,
                                                     SegmentTrackId segTrackId,
                                                     const DecodingOrderVector& itemDecodingOrder,
                                                     DecodingOrderVector& output) const
    {
        output.clear();
        output.reserve(itemDecodingOrder.size());

        InitSegmentTrackId initSegTrackId = std::make_pair(initSegmentId, segTrackId.second);

        ContextType contextType;
        int error = getContextTypeError(initSegTrackId, contextType);
        if (error)
        {
            return error;
        }

        // Add dependencies for each sample
        IdVector dependencies;
        for (const auto& entry : itemDecodingOrder)
        {
            dependencies.clear();
            if (contextType == ContextType::META)
            {
                dependencies.push_back(entry.first.get());
            }
            else if (contextType == ContextType::TRACK)
            {
            }
            else
            {
                return ErrorCode::INVALID_CONTEXT_ID;
            }

            // If only one dependency is given, it is the sample itself, so it is not added.
            if (!(dependencies.size() == 1 && dependencies.at(0) == entry.first.get()))
            {
                for (const auto& sampleId : dependencies)
                {
                    output.push_back(std::pair<ItemId, Timestamp>(sampleId, 0xffffffff));
                }
            }
            output.push_back(entry);
        }

        return ErrorCode::OK;
    }

    void MP4VRFileReaderImpl::makeSegmentIndex(const SegmentIndexBox& sidxBox,
                                               SegmentIndex& segmentIndex,
                                               int64_t dataOffsetAnchor)
    {
        uint64_t offset             = static_cast<uint64_t>(dataOffsetAnchor) + sidxBox.getFirstOffset();
        uint64_t cumulativeDuration = sidxBox.getEarliestPresentationTime();

        Vector<SegmentIndexBox::Reference> references = sidxBox.getReferences();
        segmentIndex                                  = DynArray<SegmentInformation>(references.size());
        for (uint32_t index = 0; index < references.size(); index++)
        {
            segmentIndex[index].segmentId = index + 1;  // leave 0 index for possible initseg 0 index with local files.
            segmentIndex[index].referenceId     = sidxBox.getReferenceId();
            segmentIndex[index].timescale       = sidxBox.getTimescale();
            segmentIndex[index].referenceType   = references.at(index).referenceType;
            segmentIndex[index].earliestPTSinTS = cumulativeDuration;
            cumulativeDuration += references.at(index).subsegmentDuration;
            segmentIndex[index].durationInTS    = references.at(index).subsegmentDuration;
            segmentIndex[index].startDataOffset = offset;
            segmentIndex[index].dataSize        = references.at(index).referencedSize;
            offset += segmentIndex[index].dataSize;
            segmentIndex[index].startsWithSAP = references.at(index).startsWithSAP;
            segmentIndex[index].SAPType       = references.at(index).sapType;
        }
    }

    unsigned MP4VRFileReaderImpl::toTrackId(InitSegmentTrackId id) const
    {
        InitSegmentId initSegmentId = id.first;
        ContextId contextId         = id.second;

        if ((contextId.get() >> 16) != 0)
        {
            // track ids >= 65536 are not supported
            throw FileReaderException(ErrorCode::FILE_HEADER_ERROR);
        }

        return (initSegmentId.get() << 16) | contextId.get();
    }

    InitSegmentTrackId MP4VRFileReaderImpl::ofTrackId(unsigned id) const
    {
        InitSegmentId initSegmentId((id >> 16) & 0xffff);
        ContextId contextId(id & 0xffff);

        return std::make_pair(initSegmentId, contextId);
    }

    uint64_t MP4VRFileReaderImpl::readNalLength(char* buffer) const
    {
        uint64_t refNalLength = 0;
        size_t i              = 0;
        refNalLength |= ((uint32_t)((uint8_t)(buffer[i++]))) << 24;
        refNalLength |= (((uint32_t)((uint8_t)(buffer[i++]))) << 16) & 0x00ff0000;
        refNalLength |= (((uint32_t)((uint8_t)(buffer[i++]))) << 8) & 0x0000ff00;
        refNalLength |= ((uint32_t)((uint8_t)(buffer[i++]))) & 0x000000ff;
        return refNalLength;
    }

    void MP4VRFileReaderImpl::writeNalLength(uint64_t length, char* buffer) const
    {
        buffer[0] = (char) ((0xff000000 & length) >> 24);
        buffer[1] = (char) ((0x00ff0000 & length) >> 16);
        buffer[2] = (char) ((0x0000ff00 & length) >> 8);
        buffer[3] = (char) ((uint8_t) length);
    }
}  // namespace MP4VR
