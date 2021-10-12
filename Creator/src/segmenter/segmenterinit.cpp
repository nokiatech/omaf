
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
#include "segmenterinit.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#include "segmenter.h"  // for SegmentRole
#include "async/futureops.h"

namespace VDD
{
    void appendScalTrafIndexMap(TrackToScalTrafIndexMap& aMap,
                                const SegmenterInit::Config& aSegmenterInitConfig,
                                TrackId aExtractorTrackId)
    {
        int8_t index = 1;

        // Map each track group to its tracks
        std::map<TrackGroupId, std::set<TrackId>> tracksOfId;
        for (auto& track: aSegmenterInitConfig.tracks)
        {
            if (track.second.alte)
            {
                tracksOfId[*track.second.alte].insert(track.first);
            }
            else
            {
                tracksOfId[TrackGroupId(track.first.get())].insert(track.first);
            }
        }

        // scal actuall references to track groups, not track ids, so some type mapping occurs
        for (TrackId trackGroupId :
             aSegmenterInitConfig.tracks.at(aExtractorTrackId).trackReferences.at("scal"))
        {
            for (TrackId track : tracksOfId[TrackGroupId(trackGroupId.get())])
            {
                aMap[aExtractorTrackId][track] = index;
            }
            ++index;
        }

        assert(aMap.size());
    }

    UnsupportedCodecException::UnsupportedCodecException() : Exception("UnsupportedCodecException")
    {
        // nothing
    }

    SegmenterInit::SegmenterInit(SegmenterInit::Config aConfig) : mConfig(aConfig)
    {
        assert(mConfig.tracks.size());
        for (auto& trackIdAndConfig : mConfig.tracks)
        {
            mFirstFrameRemaining.insert(trackIdAndConfig.first);
        }
    }

    SegmenterInit::~SegmenterInit() = default;

    StorageType SegmenterInit::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    std::string SegmenterInit::getGraphVizDescription()
    {
        std::ostringstream st;
        bool first = true;
        for (auto streamId :
             /* sort: */ std::set<StreamId>{mConfig.streamIds.begin(), mConfig.streamIds.end()})
        {
            st << (first ? "" : ",");
            first = false;
            st << streamId;
        }
        return st.str();
    }

    StreamSegmenter::Segmenter::InitSegment SegmenterInit::makeInitSegment(bool aFragmented)
    {
        StreamSegmenter::Segmenter::MovieDescription movieDescription;
        movieDescription.creationTime = 0;
        movieDescription.modificationTime = 0;
        movieDescription.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        StreamSegmenter::BrandSpec fileType = {std::string("isom"), 512, {"isom", "iso6"}};
        if (!mOmafVideoTrackBrand.empty())
        {
            fileType.compatibleBrands.push_back(mOmafVideoTrackBrand);
        }
        if (!mOmafAudioTrackBrand.empty())
        {
            fileType.compatibleBrands.push_back(mOmafAudioTrackBrand);
        }
        movieDescription.fileType = fileType;

        if (auto fileMeta = mConfig.fileMeta)
        {
            movieDescription.fileMeta = waitFuture(*fileMeta);
        }

        if (mConfig.duration)
        {
            movieDescription.duration = *mConfig.duration;
            movieDescription.fragmentDuration = *mConfig.duration;
        }

        auto initSegment = StreamSegmenter::Segmenter::makeInitSegment(
            mTrackDescriptions, movieDescription, aFragmented);

        return initSegment;
    }

    void SegmenterInit::updateTrackDescription(
        StreamSegmenter::Segmenter::TrackDescription& aTrackDescription,
        const SegmenterInit::TrackConfig& aTrackConfig, const Meta&) const
    {
        aTrackDescription.trackReferences = aTrackConfig.trackReferences;
        if (aTrackConfig.alte)
        {
            StreamSegmenter::Segmenter::ALTE alte;
            alte.groupId = *aTrackConfig.alte;
            aTrackDescription.alte = alte;
        }
        if (mConfig.mode == OutputMode::OMAFV2)
        {
            StreamSegmenter::Segmenter::OMAFv2Link omafv2;
            omafv2.dataReferenceKind = StreamSegmenter::Segmenter::DataReferenceKind::Snim;
            aTrackDescription.omafv2 = omafv2;
        }
    }

    void SegmenterInit::addH264VideoTrack(TrackId aTrackId, const Meta& aMeta)
    {
        const CodedFrameMeta& codedMeta = aMeta.getCodedFrameMeta();
        auto& track = mConfig.tracks.at(aTrackId);
        std::vector<std::uint8_t> sps = codedMeta.decoderConfig.at(ConfigType::SPS);
        std::vector<std::uint8_t> pps = codedMeta.decoderConfig.at(ConfigType::PPS);
        StreamSegmenter::TrackMeta trackMeta = mConfig.tracks.at(aTrackId).meta;
        StreamSegmenter::Segmenter::MediaDescription mediaDescription;
        StreamSegmenter::Segmenter::AvcVideoSampleEntry sampleEntry{};

        sampleEntry.width = codedMeta.width;
        sampleEntry.height = codedMeta.height;

        sampleEntry.sps = sps;
        sampleEntry.pps = pps;

        auto trackDescription =
            StreamSegmenter::Segmenter::TrackDescription(trackMeta, mediaDescription, sampleEntry);
        updateTrackDescription(trackDescription, track, aMeta);
        mTrackDescriptions.insert(std::make_pair(aTrackId, std::move(trackDescription)));
    }

    void SegmenterInit::addH265VideoTrack(TrackId aTrackId, const Meta& aMeta)
    {
        const CodedFrameMeta& codedMeta = aMeta.getCodedFrameMeta();
        auto& track = mConfig.tracks.at(aTrackId);
        std::vector<std::uint8_t> sps = codedMeta.decoderConfig.at(ConfigType::SPS);
        std::vector<std::uint8_t> pps = codedMeta.decoderConfig.at(ConfigType::PPS);
        std::vector<std::uint8_t> vps = codedMeta.decoderConfig.at(ConfigType::VPS);
        StreamSegmenter::TrackMeta trackMeta = mConfig.tracks.at(aTrackId).meta;
        StreamSegmenter::Segmenter::MediaDescription mediaDescription;
        mediaDescription.creationTime = 0;
        mediaDescription.modificationTime = 0;
        StreamSegmenter::Segmenter::HevcVideoSampleEntry sampleEntry{};

        sampleEntry.width = codedMeta.width;
        sampleEntry.height = codedMeta.height;
        sampleEntry.framerate = codedMeta.duration.per1().asDouble();

        sampleEntry.sps = sps;
        sampleEntry.pps = pps;
        sampleEntry.vps = vps;

        fillOmafStructures(aTrackId, codedMeta, sampleEntry, trackMeta);

        auto trackDescription =
            StreamSegmenter::Segmenter::TrackDescription(trackMeta, mediaDescription, sampleEntry);
        updateTrackDescription(trackDescription, track, aMeta);
        mTrackDescriptions.insert(std::make_pair(aTrackId, std::move(trackDescription)));
    }

    void SegmenterInit::addAACTrack(TrackId aTrackId, const Meta& aMeta,
                                    const TrackConfig& /* aTrackConfig */)
    {
        const CodedFrameMeta& codedMeta = aMeta.getCodedFrameMeta();
        auto& track = mConfig.tracks.at(aTrackId);
        std::vector<std::uint8_t> decSepcificInfo =
            codedMeta.decoderConfig.find(ConfigType::AudioSpecificConfig)->second;
        {
            StreamSegmenter::TrackMeta trackMeta = mConfig.tracks.at(aTrackId).meta;
            StreamSegmenter::Segmenter::MediaDescription mediaDescription;
            StreamSegmenter::Segmenter::MP4AudioSampleEntry sampleEntry{};

            sampleEntry.sampleSize = 16;
            sampleEntry.channelCount = 2;  // always 2
            sampleEntry.sampleRate = codedMeta.samplingFrequency;
            sampleEntry.esId = 1;
            sampleEntry.dependsOnEsId = 0;
            sampleEntry.bufferSize = 0;
            sampleEntry.maxBitrate = codedMeta.bitrate.maxBitrate;
            sampleEntry.avgBitrate = codedMeta.bitrate.avgBitrate;
            for (auto byte : decSepcificInfo)
            {
                sampleEntry.decSpecificInfo.push_back(static_cast<char>(byte));
            }

            if (mConfig.mode == OutputMode::OMAFV1)
            {
                mOmafAudioTrackBrand = "oa2d";
            }
            auto trackDescription = StreamSegmenter::Segmenter::TrackDescription(
                trackMeta, mediaDescription, sampleEntry);
            updateTrackDescription(trackDescription, track, aMeta);
            mTrackDescriptions.insert(std::make_pair(aTrackId, std::move(trackDescription)));
        }
    }

    void SegmenterInit::addInvoTimeMetadataTrack(TrackId aTrackId, const Meta& aMeta)
    {
        auto& track = mConfig.tracks.at(aTrackId);
        StreamSegmenter::TrackMeta trackMeta = track.meta;

        StreamSegmenter::Segmenter::MediaDescription mediaDescription;
        StreamSegmenter::Segmenter::InitialViewingOrientationSampleEntry sampleEntry{};

        auto trackDescription =
            StreamSegmenter::Segmenter::TrackDescription(trackMeta, mediaDescription, sampleEntry);
        updateTrackDescription(trackDescription, track, aMeta);
        mTrackDescriptions.insert(std::make_pair(aTrackId, std::move(trackDescription)));
    }

    void SegmenterInit::addGenericTimedMetadataTrack(TrackId aTrackId, const Meta& aMeta)
    {
        auto& track = mConfig.tracks.at(aTrackId);
        StreamSegmenter::TrackMeta trackMeta = track.meta;

        StreamSegmenter::Segmenter::MediaDescription mediaDescription;

        std::shared_ptr<StreamSegmenter::Segmenter::SampleEntry> sampleEntry;
        if (auto sampleEntryTag = aMeta.findTag<SampleEntryTag>())
        {
            sampleEntry = sampleEntryTag->get();
        }
        else
        {
            assert(false);
        }

        auto trackDescription =
            StreamSegmenter::Segmenter::TrackDescription(trackMeta, mediaDescription, *sampleEntry);
        updateTrackDescription(trackDescription, track, aMeta);
        mTrackDescriptions.insert(std::make_pair(aTrackId, std::move(trackDescription)));
    }

    void SegmenterInit::addH265ExtractorTrack(TrackId aTrackId, const Meta& aMeta)
    {
        const CodedFrameMeta& codedMeta = aMeta.getCodedFrameMeta();
        std::vector<std::uint8_t> sps = codedMeta.decoderConfig.at(ConfigType::SPS);
        std::vector<std::uint8_t> pps = codedMeta.decoderConfig.at(ConfigType::PPS);
        std::vector<std::uint8_t> vps = codedMeta.decoderConfig.at(ConfigType::VPS);
        StreamSegmenter::TrackMeta trackMeta = mConfig.tracks.at(aTrackId).meta;
        StreamSegmenter::Segmenter::MediaDescription mediaDescription;
        mediaDescription.creationTime = 0;
        mediaDescription.modificationTime = 0;
        StreamSegmenter::Segmenter::HevcVideoSampleEntry sampleEntry{};

        sampleEntry.width = codedMeta.width;
        sampleEntry.height = codedMeta.height;
        sampleEntry.framerate = codedMeta.duration.per1().asDouble();
        sampleEntry.sampleEntryType = "hvc2";

        sampleEntry.sps = sps;
        sampleEntry.pps = pps;
        sampleEntry.vps = vps;

        fillOmafStructures(aTrackId, codedMeta, sampleEntry, trackMeta);

        auto& track = mConfig.tracks.at(aTrackId);
        auto trackDescription =
            StreamSegmenter::Segmenter::TrackDescription(trackMeta, mediaDescription, sampleEntry);
        updateTrackDescription(trackDescription, track, aMeta);
        mTrackDescriptions.insert(std::make_pair(aTrackId, std::move(trackDescription)));
    }

    std::vector<Streams> SegmenterInit::process(const Streams& aData)
    {
        std::vector<Streams> frames;

        if (mConfig.streamIds.size() > 0)
        {
            std::list<StreamId>::const_iterator findIter = std::find(
                mConfig.streamIds.begin(), mConfig.streamIds.end(), aData.front().getStreamId());
            if (findIter == mConfig.streamIds.end())
            {
                // not for us, ignore
                return frames;
            }
        }

        TrackId trackId = 1;
        if (auto trackIdTag = aData.front().getMeta().findTag<TrackIdTag>())
        {
            trackId = trackIdTag->get();
        }
        assert(mConfig.tracks.count(trackId));

        bool hadFirstFramesRemaining = mFirstFrameRemaining.size();
        bool endOfStream = aData.isEndOfStream();
        Optional<CodedFrameMeta> codedMeta;
        if (!endOfStream && mFirstFrameRemaining.count(trackId))
        {
            auto meta = aData.front().getMeta();
            codedMeta = meta.getCodedFrameMeta();

            switch (codedMeta->format)
            {
            case CodedFormat::H264:
                addH264VideoTrack(trackId, meta);
                break;
            case CodedFormat::H265:
                addH265VideoTrack(trackId, meta);
                break;
            case CodedFormat::AAC:
                addAACTrack(trackId, meta, mConfig.tracks.at(trackId));
                break;
            case CodedFormat::TimedMetadataInvo:
                addInvoTimeMetadataTrack(trackId, meta);
                break;
            case CodedFormat::TimedMetadataDyol:
            case CodedFormat::TimedMetadataDyvp:
            case CodedFormat::TimedMetadataInvp:
            case CodedFormat::TimedMetadataRcvp:
                addGenericTimedMetadataTrack(trackId, meta);
                break;
            case CodedFormat::H265Extractor:
                addH265ExtractorTrack(trackId, meta);
                break;
            case CodedFormat::None:
                throw UnsupportedCodecException();
                break;
            }
        }

        mFirstFrameRemaining.erase(trackId);

        bool hasFirstFramesRemaining = mFirstFrameRemaining.size();
        if (hadFirstFramesRemaining && !hasFirstFramesRemaining)
        {
            if (mConfig.writeToBitstream)
            {
                if (!endOfStream)
                {
                    std::ostringstream frameStream;
                    if (auto hd = mConfig.frameSegmentHeader)
                    {
                        auto x = makeXtyp(*hd);
                        frameStream.write(reinterpret_cast<const char*>(&x[0]), x.size());
                    }
                    StreamSegmenter::Segmenter::writeInitSegment(
                        frameStream, makeInitSegment(mConfig.fragmented));
                    std::string frameString(frameStream.str());
                    std::vector<std::vector<uint8_t>> dataVec(1);
                    dataVec[0].reserve(frameString.size());
                    for (auto x : frameString)
                    {
                        dataVec[0].push_back(std::uint8_t(x));
                    }
                    CPUDataVector dataRef(std::move(dataVec));
                    // codedMeta is copied so that the frame index is still indicative of data age,
                    // for
                    // scheduling purposes
                    Meta meta(*codedMeta);
                    meta.attachTag(SegmentRoleTag(SegmentRole::InitSegment));
                    frames.push_back({Data(std::move(dataRef), meta)});
                }
                Meta endMeta;
                endMeta.attachTag(SegmentRoleTag(SegmentRole::InitSegment));
                frames.push_back({Data(EndOfStream(), endMeta)});
            }
        }

        return frames;
    }

    Optional<StreamSegmenter::Segmenter::InitSegment> SegmenterInit::prepareInitSegment()
    {
        assert(!mConfig.writeToBitstream);

        if (mFirstFrameRemaining.empty())
        {
            // ready to generate it
            return makeInitSegment(mConfig.fragmented);
        }

        return {};
    }

    void SegmenterInit::fillOmafStructures(
        TrackId aTrackId, const CodedFrameMeta& aMeta,
        StreamSegmenter::Segmenter::HevcVideoSampleEntry& aSampleEntry,
        StreamSegmenter::TrackMeta& aTrackMeta)
    {
        if (aMeta.projection)
        {
            if (aMeta.projection == ISOBMFF::makeOptional(OmafProjectionType::Equirectangular))
            {
                aSampleEntry.projectionFormat =
                    StreamSegmenter::Segmenter::ProjectionFormat::Equirectangular;
            }
            else
            {
                aSampleEntry.projectionFormat =
                    StreamSegmenter::Segmenter::ProjectionFormat::Cubemap;
            }
        }
        if (mConfig.packedSubPictures)
        {
            // viewport dependent
            StreamSegmenter::BrandSpec trackType = {std::string("hevd"), 0, {"hevd"}};
            aTrackMeta.trackType = trackType;
            aSampleEntry.compatibleSchemes.push_back({"podv", 0, ""});
            aSampleEntry.compatibleSchemes.push_back({"ercm", 0, ""});
            mOmafVideoTrackBrand = "hevd";
        }
        else
        {
            StreamSegmenter::BrandSpec trackType = {std::string("hevi"), 0, {"hevi"}};
            aTrackMeta.trackType = trackType;
            aSampleEntry.compatibleSchemes.push_back({"podv", 0, ""});
            aSampleEntry.compatibleSchemes.push_back({"erpv", 0, ""});
            mOmafVideoTrackBrand = "hevi";
        }

        if (mConfig.tracks.at(aTrackId).pipelineOutput.getVideo() == PipelineOutputVideo::TopBottom)
        {
            aSampleEntry.stvi = StreamSegmenter::Segmenter::PodvStereoVideoInfo::TopBottomPacking;
        }
        else if (mConfig.tracks.at(aTrackId).pipelineOutput.getVideo() ==
                 PipelineOutputVideo::SideBySide)
        {
            aSampleEntry.stvi = StreamSegmenter::Segmenter::PodvStereoVideoInfo::SideBySidePacking;
        }
        else if (mConfig.tracks.at(aTrackId).pipelineOutput.getVideo() ==
                 PipelineOutputVideo::TemporalInterleaving)
        {
            aSampleEntry.stvi =
                StreamSegmenter::Segmenter::PodvStereoVideoInfo::TemporalInterleaving;
        }

        // pass overlay declarations if given to current track
        auto ovlysOpt = mConfig.tracks.at(aTrackId).overlays;
        if (ovlysOpt)
        {
            aSampleEntry.ovly = *ovlysOpt;
        }

        // rotation is supported for debugging only, no API to set it for now
        if (false)
        {
            StreamSegmenter::Segmenter::Rotation rotn;
            rotn.yaw = 45 * 65536;
            rotn.pitch = 0;
            rotn.roll = 0;
            aSampleEntry.rotn = rotn;
        }


        if (aMeta.sphericalCoverage)
        {
            aSampleEntry.covi = StreamSegmenter::Segmenter::CoverageInformation();
            if (aMeta.projection == ISOBMFF::makeOptional(OmafProjectionType::Equirectangular))
            {
                aSampleEntry.covi->coverageShape = StreamSegmenter::Segmenter::
                    CoverageInformationShapeType::TwoAzimuthAndTwoElevationCircles;
            }
            else
            {
                aSampleEntry.covi->coverageShape =
                    StreamSegmenter::Segmenter::CoverageInformationShapeType::FourGreatCircles;
            }
            if (aSampleEntry.stvi)
            {
                aSampleEntry.covi->defaultViewIdc =
                    StreamSegmenter::Segmenter::ViewIdc::LEFT_AND_RIGHT;
            }
            else
            {
                aSampleEntry.covi->defaultViewIdc = StreamSegmenter::Segmenter::ViewIdc::MONOSCOPIC;
            }
            aSampleEntry.covi->viewIdcPresenceFlag = false;

            auto region = std::unique_ptr<StreamSegmenter::Segmenter::CoverageInformationRegion>(
                new StreamSegmenter::Segmenter::CoverageInformationRegion());
            auto& sphericalCoverage = aMeta.sphericalCoverage.get();
            region->centreAzimuth = sphericalCoverage.cAzimuth;
            region->centreElevation = sphericalCoverage.cElevation;
            region->centreTilt = sphericalCoverage.cTilt;
            region->azimuthRange = sphericalCoverage.rAzimuth;
            region->elevationRange = sphericalCoverage.rElevation;
            region->interpolate = false;
            aSampleEntry.covi->regions.push_back(std::move(region));
        }

        if (aMeta.regionPacking)
        {
            aSampleEntry.rwpk = StreamSegmenter::Segmenter::RegionWisePacking();

            auto& regionPacking = aMeta.regionPacking.get();
            aSampleEntry.rwpk->constituenPictureMatchingFlag =
                regionPacking.constituentPictMatching;
            aSampleEntry.rwpk->projPictureHeight = regionPacking.projPictureHeight;
            aSampleEntry.rwpk->projPictureWidth = regionPacking.projPictureWidth;
            aSampleEntry.rwpk->packedPictureHeight = regionPacking.packedPictureHeight;
            aSampleEntry.rwpk->packedPictureWidth = regionPacking.packedPictureWidth;
            for (auto& regionIn : regionPacking.regions)
            {
                auto region = std::unique_ptr<StreamSegmenter::Segmenter::RwpkRectRegion>(
                    new StreamSegmenter::Segmenter::RwpkRectRegion());
                region->packedTop = regionIn.packedTop;
                region->packedLeft = regionIn.packedLeft;
                region->packedWidth = regionIn.packedWidth;
                region->packedHeight = regionIn.packedHeight;

                region->projTop = regionIn.projTop;
                region->projLeft = regionIn.projLeft;
                region->projWidth = regionIn.projWidth;
                region->projHeight = regionIn.projHeight;

                region->transformType = regionIn.transform;

                aSampleEntry.rwpk->regions.push_back(std::move(region));
            }
        }
    }
}  // namespace VDD
