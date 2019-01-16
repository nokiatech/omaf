
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
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "api/streamsegmenter/frame.hpp"
#include "avccommondefs.hpp"
#include "utils.hpp"

#include "mp4access.hpp"

#include "avcsampleentry.hpp"
#include "filetypebox.hpp"
#include "hevcconfigurationbox.hpp"
#include "hevcdecoderconfigrecord.hpp"
#include "hevcsampleentry.hpp"
#include "initialviewingorientationsampleentry.hpp"
#include "mediadatabox.hpp"
#include "moviebox.hpp"
#include "moviefragmentbox.hpp"
#include "mp4audiosampleentrybox.hpp"
#include "segmentindexbox.hpp"
#include "segmenttypebox.hpp"
#include "urimetasampleentrybox.hpp"
#include "userdatabox.hpp"

#include "api/streamsegmenter/segmenterapi.hpp"
#include "api/streamsegmenter/track.hpp"
#include "private.hpp"
#include "segmenter.hpp"

namespace StreamSegmenter
{
    namespace Segmenter
    {
        namespace
        {
            typedef std::vector<Track> TrackInfo;
            typedef std::vector<TrackId> TrackIds;

            TrackFrames& findTrackFramesByTrackId(FrameAccessors& frameAccessor, TrackId trackId)
            {
                auto it = std::find_if(frameAccessor.begin(), frameAccessor.end(),
                                       [&](TrackFrames& segment) { return (segment.trackMeta.trackId == trackId); });
                assert(it != frameAccessor.end());
                return *it;
            }

            TrackIds trackIdsOfFrameAccessors(const FrameAccessors& aFrameAccessors)
            {
                TrackIds trackIds;

                for (const auto& frameAccessor : aFrameAccessors)
                {
                    trackIds.push_back(frameAccessor.trackMeta.trackId);
                }

                return trackIds;
            }

            // This is used while actually writing the data down
            struct SegmentMoofInfo
            {
                SegmentTrackInfo trackInfo;
                std::int32_t moofToDataOffset;
            };

            typedef std::map<TrackId, SegmentMoofInfo> SegmentMoofInfos;

            void writeMoof(std::ostream& aOut,
                           const TrackIds& aTrackIds,
                           const Segment& aSegment,
                           const SegmentMoofInfos& aSegmentMoofInfos,
                           const std::map<TrackId, Frames>& aFrames)
            {
                Vector<MOVIEFRAGMENTS::SampleDefaults> sampleDefaults;
                for (auto trackId : aTrackIds)
                {
                    sampleDefaults.push_back(MOVIEFRAGMENTS::SampleDefaults{
                        trackId.get(),  // trackId
                        1,              // defaultSampleDescriptionIndex
                        0,              // defaultSampleDuration
                        0,              // defaultSampleSize
                        {0}             // defaultSampleFlags
                    });
                }
                MovieFragmentBox moof(sampleDefaults);
                moof.getMovieFragmentHeaderBox().setSequenceNumber(aSegment.sequenceId.get());
                for (auto trackId : aTrackIds)
                {
                    const auto& segmentMoofInfo = aSegmentMoofInfos.find(trackId)->second;
                    const auto& trackMeta       = segmentMoofInfo.trackInfo.trackMeta;
                    auto traf                   = makeCustomUnique<TrackFragmentBox, TrackFragmentBox>(sampleDefaults);
                    auto trun                   = makeCustomUnique<TrackRunBox, TrackRunBox>(
                        uint8_t(1), 0u | TrackRunBox::TrackRunFlags::SampleDurationPresent |
                                        TrackRunBox::TrackRunFlags::SampleSizePresent |
                                        TrackRunBox::TrackRunFlags::SampleCompositionTimeOffsetsPresent |
                                        TrackRunBox::TrackRunFlags::SampleFlagsPresent);
                    auto tfdt =
                        makeCustomUnique<TrackFragmentBaseMediaDecodeTimeBox, TrackFragmentBaseMediaDecodeTimeBox>();

                    tfdt->setBaseMediaDecodeTime(
                        uint64_t((segmentMoofInfo.trackInfo.t0.cast<RatU64>() / trackMeta.timescale).asDouble()));
                    assert(segmentMoofInfo.trackInfo.t0 >= FrameTime(0, 1));
                    traf->getTrackFragmentHeaderBox().setTrackId(trackMeta.trackId.get());
                    traf->getTrackFragmentHeaderBox().setFlags(0 | TrackFragmentHeaderBox::DefaultBaseIsMoof);

                    trun->setDataOffset(segmentMoofInfo.moofToDataOffset);

                    FrameTime time = segmentMoofInfo.trackInfo.t0;

                    for (const auto& frame : aFrames.find(trackId)->second)
                    {
                        std::uint64_t frameSize = frame.getSize();
                        FrameInfo frameInfo     = frame.getFrameInfo();

                        TrackRunBox::SampleDetails s;
                        s.version1.sampleDuration =
                            std::uint32_t((frameInfo.duration / trackMeta.timescale).asDouble());
                        s.version1.sampleSize                  = std::uint32_t(frameSize);
                        s.version1.sampleFlags                 = {frameInfo.sampleFlags.flagsAsUInt};
                        s.version1.sampleCompositionTimeOffset = std::int32_t(
                            ((frameInfo.cts.front() - time) / trackMeta.timescale.cast<RatS64>()).asDouble());
                        trun->addSampleDetails(s);

                        time += frameInfo.duration.cast<FrameTime>();
                    }
                    trun->setSampleCount((uint32_t) aFrames.find(trackId)->second.size());

                    traf->setTrackFragmentDecodeTimeBox(std::move(tfdt));
                    traf->addTrackRunBox(std::move(trun));
                    moof.addTrackFragmentBox(std::move(traf));
                }

                BitStream bs;
                moof.writeBox(bs);
                auto data = bs.getStorage();
                aOut.write(reinterpret_cast<const char*>(&data[0]), std::streamsize(data.size()));
            }

            void flush(BitStream& aBs, std::ostream& aOut)
            {
                auto data = aBs.getStorage();
                aOut.write(reinterpret_cast<const char*>(&data[0]), std::streamsize(data.size()));
                aBs.clear();
            }

            void copyBox(const ::Box& src, ::Box& dst)
            {
                BitStream bs;
                const_cast<::Box&>(src).writeBox(bs);
                bs.reset();
                dst.parseBox(bs);
            }

            template <typename T>
            UniquePtr<T> cloneBox(const T& src)
            {
                UniquePtr<T> dst(new T);
                BitStream bs;
                const_cast<T&>(src).writeBox(bs);
                bs.reset();
                dst->parseBox(bs);
                return dst;
            }

            std::unique_ptr<MediaHeaderBox> makeMediaHeaderBox(MediaDescription aTrackMediaDescription,
                                                               RatU64 aTimescale)
            {
                UniquePtr<::MediaHeaderBox> box = makeCustomUnique<::MediaHeaderBox, ::MediaHeaderBox>();

                uint64_t creationTime     = aTrackMediaDescription.creationTime;
                uint64_t modificationTime = aTrackMediaDescription.modificationTime;
                RatU64 duration           = aTrackMediaDescription.duration;
                box->setTimeScale(std::uint32_t(aTimescale.per1().asDouble()));
                box->setCreationTime(creationTime);
                box->setModificationTime(modificationTime);
                box->setDuration(std::uint32_t((duration / aTimescale).asDouble()));

                return Utils::make_unique<MediaHeaderBox>(std::move(box));
            }

            void fillTrackHeaderBox(::TrackHeaderBox& aTrackHeaderBox, const MediaDescription& aMediaDescription)
            {
                uint64_t creationTime     = aMediaDescription.creationTime;
                uint64_t modificationTime = aMediaDescription.modificationTime;
                RatU64 duration           = aMediaDescription.duration.minimize();

                aTrackHeaderBox.setFlags(1 | 3);  // Track_enabled | Track_in_movie
                aTrackHeaderBox.setCreationTime(creationTime);
                aTrackHeaderBox.setModificationTime(modificationTime);
                aTrackHeaderBox.setDuration(std::uint32_t(duration.num));
            }

            uint32_t timescaleOfTrackSegments(const TrackSegment trackSegments)
            {
                std::vector<RatU64> trackTimescales;
                for (auto trackInfo : trackSegments)
                {
                    trackTimescales.push_back(trackInfo.second.trackInfo.trackMeta.timescale);
                }
                std::vector<RatU64*> trackTimescaleRefs;
                for (auto& timescale : trackTimescales)
                {
                    trackTimescaleRefs.push_back(&timescale);
                }
                shareDenominators(trackTimescaleRefs.begin(), trackTimescaleRefs.end());
                std::sort(trackTimescales.begin(), trackTimescales.end());

                uint32_t timescale = uint32_t(trackTimescales[0].per1().asDouble());
                return timescale;
            }

            void setTrackReferences(TrackReferenceBox& trefBox,
                                    const std::map<std::string, std::set<TrackId>>& trackReferences)
            {
                for (auto& tref : trackReferences)
                {
                    Vector<std::uint32_t> trackIds;
                    for (auto& track : tref.second)
                    {
                        trackIds.push_back(track.get());
                    }
                    TrackReferenceTypeBox trefTypeBox(FourCCInt{String{tref.first.begin(), tref.first.end()}});
                    trefTypeBox.setTrackIds(trackIds);
                    trefBox.addTrefTypeBox(trefTypeBox);
                }
            }
        }  // anonymous namespace

        Metadata::Metadata()  = default;
        Metadata::~Metadata() = default;

        void Metadata::setSoftware(const std::string& aSoftware)
        {
            mSoftware = aSoftware;
        }

        void Metadata::setVersion(const std::string& aVersion)
        {
            mVersion = aVersion;
        }

        void Metadata::setOther(const std::string& aKey, const std::string& aValue)
        {
            mOthers[aKey] = aValue;
        }

        std::string Metadata::getSoftware() const
        {
            return mSoftware;
        }

        std::string Metadata::getVersion() const
        {
            return mVersion;
        }

        std::map<std::string, std::string> Metadata::getOthers() const
        {
            return mOthers;
        }

        InitSegment::InitSegment() = default;

        InitSegment::InitSegment(const InitSegment& other)
            : moov(other.moov ? new MovieBox{cloneBox(*other.moov->movieBox)} : nullptr)
            , ftyp(other.ftyp ? new FileTypeBox{cloneBox(*other.ftyp->fileTypeBox)} : nullptr)
        {
            // nothing
        }

        InitSegment::~InitSegment() = default;

        InitSegment& InitSegment::operator=(const InitSegment& other)
        {
            if (other.moov)
            {
                moov.reset(new MovieBox{cloneBox(*other.moov->movieBox)});
            }
            else
            {
                moov.reset();
            }
            return *this;
        }

        InitSegment& InitSegment::operator=(InitSegment&& other)
        {
            moov = std::move(other.moov);
            return *this;
        }

        void writeSegmentHeader(std::ostream& aOut)
        {
            BitStream bs;

            SegmentTypeBox styp;
            styp.setMajorBrand("msdh");
            styp.addCompatibleBrand("msdh");
            styp.addCompatibleBrand("msix");
            styp.writeBox(bs);

            flush(bs, aOut);
        }

        void writeTrackRunWithData(std::ostream& aOut, const Segment& aSegment)
        {
            TrackIds trackIds = Utils::vectorOfKeys(aSegment.tracks);
            std::map<TrackId, Frames> frames;
            for (auto trackInfo : aSegment.tracks)
            {
                frames.insert(std::make_pair(trackInfo.first, trackInfo.second.frames));
            }

            auto moofOffset = aOut.tellp();
            SegmentMoofInfos segmentMoofInfos;

            for (auto trackId : trackIds)
            {
                SegmentMoofInfo moofInfo = {aSegment.tracks.find(trackId)->second.trackInfo, 0};
                segmentMoofInfos.insert(std::make_pair(trackId, std::move(moofInfo)));
            }

            writeMoof(aOut, trackIds, aSegment, segmentMoofInfos, frames);

            std::vector<uint8_t> header;
            header.push_back(0);
            header.push_back(0);
            header.push_back(0);
            header.push_back(0);
            header.push_back(uint8_t('m'));
            header.push_back(uint8_t('d'));
            header.push_back(uint8_t('a'));
            header.push_back(uint8_t('t'));
            auto mdatOffset = aOut.tellp();
            aOut.write(reinterpret_cast<const char*>(&header[0]), std::streamsize(header.size()));
            uint64_t size = header.size();
            for (auto trackId : trackIds)
            {
                SegmentMoofInfo moofInfo  = {aSegment.tracks.find(trackId)->second.trackInfo,
                                            std::int32_t(aOut.tellp() - std::streamoff(moofOffset))};
                segmentMoofInfos[trackId] = std::move(moofInfo);
                for (const auto& frame : frames.find(trackId)->second)
                {
                    const auto& frameData = *frame;
                    const auto& data      = frameData.data;
                    aOut.write(reinterpret_cast<const char*>(&data[0]), std::streamsize(data.size()));
                    size += data.size();
                }
            }
            std::streampos afterMdat = aOut.tellp();

            header[0] = uint8_t((size >> 24) & 0xff);
            header[1] = uint8_t((size >> 16) & 0xff);
            header[2] = uint8_t((size >> 8) & 0xff);
            header[3] = uint8_t((size >> 0) & 0xff);
            aOut.seekp(mdatOffset);
            aOut.write(reinterpret_cast<const char*>(&header[0]), std::streamsize(4));

            aOut.seekp(moofOffset);
            writeMoof(aOut, trackIds, aSegment, segmentMoofInfos, frames);
            aOut.seekp(afterMdat);
        }

        void writeInitSegment(std::ostream& aOut, const InitSegment& aInitSegment)
        {
            BitStream stream;

            aInitSegment.ftyp->fileTypeBox->writeBox(stream);

            // writeMoov();
#if 0
            {
                MovieBox moov;
                MovieHeaderBox& mvhd = moov.getMovieHeaderBox();
                mvhd.setTimeScale(1000000);
                mvhd.setNextTrackID(2);
                UniquePtr<TrackBox> trackBox(CUSTOM_NEW(TrackBox, ()));

                auto& tkhd = trackBox->getTrackHeaderBox();
                tkhd.setTrackID(1);
                tkhd.setWidth(2048);
                tkhd.setHeight(2048);


                // MediaBox& mdia = trackBox->getMediaBox();
                // MediaHeaderBox& mdhd = mdia.getMediaHeaderBox();
                // HandlerBox& hdlr = mdia.getHandlerBox();
                // MediaInformationBox& minf = mdia.getMediaInformationBox();
                // SampleTableBox& stbl = minf.getSampleTableBox();
                // SampleDescriptionBox& stsd = stbl.getSampleDescriptionBox();
                // UniquePtr<SampleEntryBox> mp4v(CUSTOM_NEW(SacmpleEntryBox, ("mp4v")));
                // stsd.addSampleEntry(std::move(mp4v));
                // VideoMediaHeaderBox& vmhd = minf.getVideoMediaHeaderBox();
                // hdlr.setName("vide");
                // mdhd.setTimeScale(1000000);

                // moov.addTrackBox(std::move(trackBox));
                // moov.writeBox(stream);
            }
#endif
            aInitSegment.moov->movieBox->writeBox(stream);

            auto data = stream.getStorage();
            aOut.write(reinterpret_cast<const char*>(&data[0]), std::streamsize(data.size()));
        }

        TrackDescription::TrackDescription() = default;
        TrackDescription::TrackDescription(TrackDescription&& aOther)
            : trackMeta(std::move(aOther.trackMeta))
            , sampleEntryBoxes(std::move(aOther.sampleEntryBoxes))
            , mediaHeaderBox(std::move(aOther.mediaHeaderBox))
            , handlerBox(std::move(aOther.handlerBox))
            , trackHeaderBox(std::move(aOther.trackHeaderBox))
            , trackReferences(std::move(aOther.trackReferences))
            , alternateGroup(std::move(aOther.alternateGroup))
        {
            // nothing
        }

        TrackDescription::TrackDescription(TrackMeta aTrackMeta,
                                           std::list<std::unique_ptr<SampleEntryBox>> aSampleEntryBoxes,
                                           std::unique_ptr<MediaHeaderBox>&& aMediaHeaderBox,
                                           std::unique_ptr<HandlerBox>&& aHandlerBox,
                                           std::unique_ptr<TrackHeaderBox>&& aTrackHeaderBox)
            : trackMeta(aTrackMeta)
            , sampleEntryBoxes(std::move(aSampleEntryBoxes))
            , mediaHeaderBox(std::move(aMediaHeaderBox))
            , handlerBox(std::move(aHandlerBox))
            , trackHeaderBox(std::move(aTrackHeaderBox))
        {
            // nothing
        }

        TrackDescription::~TrackDescription() = default;

        SampleEntry::SampleEntry()  = default;
        SampleEntry::~SampleEntry() = default;

        std::unique_ptr<SampleEntryBox> MP4AudioSampleEntry::makeSampleEntryBox() const
        {
            auto box                              = makeCustomUnique<::MP4AudioSampleEntryBox, ::SampleEntryBox>();
            ElementaryStreamDescriptorBox& esdBox = box->getESDBox();
            ElementaryStreamDescriptorBox::ES_Descriptor esd{};
            box->setSampleSize(sampleSize);
            box->setChannelCount(channelCount);
            box->setSampleRate(sampleRate);
            box->setDataReferenceIndex(1);
            esd.ES_DescrTag     = 3;
            esd.flags           = dependsOnEsId ? 0x80 : 0;
            esd.ES_ID           = esId;
            esd.dependsOn_ES_ID = dependsOnEsId;
            if (url.size())
            {
                esd.URLlength = static_cast<uint8_t>(url.size());
                esd.URLstring = {url.begin(), url.end()};
            }

            esd.decConfigDescr.DecoderConfigDescrTag = 4;
            esd.decConfigDescr.streamType            = 0x05;  // ISO/IEC 14496-1:2010(E), Table 6, 0x05 AudioStream
            esd.decConfigDescr.objectTypeIndication =
                0x40;  // Audio ISO/IEC 14496-3 (d) http://www.mp4ra.org/object.html
            esd.decConfigDescr.bufferSizeDB                       = bufferSize;
            esd.decConfigDescr.avgBitrate                         = avgBitrate;
            esd.decConfigDescr.maxBitrate                         = maxBitrate;
            esd.decConfigDescr.decSpecificInfo.DecSpecificInfoTag = 5;
            esd.decConfigDescr.decSpecificInfo.size               = static_cast<uint32_t>(decSpecificInfo.size());
            esd.decConfigDescr.decSpecificInfo.DecSpecificInfo.resize(decSpecificInfo.size());
            for (size_t i = 0; i < decSpecificInfo.size(); ++i)
            {
                esd.decConfigDescr.decSpecificInfo.DecSpecificInfo[i] = static_cast<uint8_t>(decSpecificInfo[i]);
            }

            esdBox.setESDescriptor(esd);

            if (nonDiegetic)
            {
                box->setNonDiegeticAudioBox(NonDiegeticAudioBox());
            }
            if (ambisonic)
            {
                const auto& amb = *ambisonic;
                SpatialAudioBox sab;
                sab.setAmbisonicType(amb.type);
                sab.setAmbisonicOrder(amb.order);
                sab.setAmbisonicChannelOrdering(amb.channelOrdering);
                sab.setAmbisonicNormalization(amb.normalization);
                sab.setChannelMap(Vector<uint32_t>{amb.channelMap.begin(), amb.channelMap.end()});
                box->setSpatialAudioBox(sab);
            }

            if (channelLayout)
            {
                const ChannelLayout& chnl = *channelLayout;
                ChannelLayoutBox b;

                b.setChannelCount(channelCount);
                if (chnl.streamStructure & 1)  // stream structured
                {
                    b.setDefinedLayout(static_cast<uint8_t>(chnl.layout));
                    if (chnl.layout == 0)
                    {
                        for (ChannelPosition pos : chnl.positions)
                        {
                            ChannelLayoutBox::ChannelLayout layout{};
                            layout.speakerPosition = static_cast<uint8_t>(pos.speakerPosition);
                            layout.azimuth         = static_cast<int16_t>(pos.azimuth);
                            layout.elevation       = static_cast<int8_t>(pos.elevation);
                            b.addChannelLayout(layout);
                        }
                    }
                    else
                    {
                        uint64_t omittedChannelsMap{};
                        for (auto omitted : chnl.omitted)
                        {
                            omittedChannelsMap = omittedChannelsMap | (1ull << omitted);
                        }
                        b.setOmittedChannelsMap(omittedChannelsMap);
                    }
                }
                if (chnl.streamStructure & 2)  // object structured
                {
                    b.setObjectCount(static_cast<uint8_t>(chnl.objectCount));
                }

                box->setChannelLayoutBox(b);
            }

            return Utils::make_unique<SampleEntryBox>(Utils::static_cast_unique_ptr<::SampleEntryBox>(std::move(box)));
        }

        uint32_t MP4AudioSampleEntry::getWidthFP() const
        {
            return 0;
        }

        uint32_t MP4AudioSampleEntry::getHeightFP() const
        {
            return 0;
        }

        uint8_t StreamSegmenter::Segmenter::RwpkRectRegion::packingType() const
        {
            return 0;  // RECT packing type
        }

        std::unique_ptr<StreamSegmenter::Segmenter::Region>
        StreamSegmenter::Segmenter::RwpkRectRegion::makeRegion() const
        {
            auto newRegion         = makeCustomUnique<RegionWisePackingBox::Region, RegionWisePackingBox::Region>();
            newRegion->packingType = (RegionWisePackingBox::PackingType) packingType();
            newRegion->rectangularPacking = makeCustomUnique<RegionWisePackingBox::RectangularRegionWisePacking,
                                                             RegionWisePackingBox::RectangularRegionWisePacking>();
            newRegion->rectangularPacking->projRegWidth    = projWidth;
            newRegion->rectangularPacking->projRegHeight   = projHeight;
            newRegion->rectangularPacking->projRegTop      = projTop;
            newRegion->rectangularPacking->projRegLeft     = projLeft;
            newRegion->rectangularPacking->packedRegWidth  = packedWidth;
            newRegion->rectangularPacking->packedRegHeight = packedHeight;
            newRegion->rectangularPacking->packedRegTop    = packedTop;
            newRegion->rectangularPacking->packedRegLeft   = packedLeft;
            newRegion->rectangularPacking->transformType   = transformType;

            newRegion->guardBandFlag = false;
            if (guardBand)
            {
                newRegion->guardBandFlag                            = true;
                newRegion->rectangularPacking->gbNotUsedForPredFlag = guardBand->notUsedForPredFlag;
                newRegion->rectangularPacking->topGbHeight          = guardBand->topHeight;
                newRegion->rectangularPacking->leftGbWidth          = guardBand->leftWidth;
                newRegion->rectangularPacking->bottomGbHeight       = guardBand->bottomHeight;
                newRegion->rectangularPacking->rightGbWidth         = guardBand->rightWidth;
                newRegion->rectangularPacking->gbType0              = guardBand->type0;
                newRegion->rectangularPacking->gbType1              = guardBand->type1;
                newRegion->rectangularPacking->gbType2              = guardBand->type2;
                newRegion->rectangularPacking->gbType3              = guardBand->type3;
            }

            return Utils::make_unique<Region>(
                Utils::static_cast_unique_ptr<::RegionWisePackingBox::Region>(std::move(newRegion)));
        }

        uint32_t VisualSampleEntry::getWidthFP() const
        {
            return uint32_t(width) << 16;
        }

        uint32_t VisualSampleEntry::getHeightFP() const
        {
            return uint32_t(height) << 16;
        }

        void VisualSampleEntry::makePovdBoxes(std::unique_ptr<SampleEntryBox>& box) const
        {
            if (projectionFormat)
            {
                auto rinf = makeCustomUnique<RestrictedSchemeInfoBox, RestrictedSchemeInfoBox>();

                // setup stuff inside rinf... read from bot original format, override box's type with "resv" ...
                rinf->setOriginalFormat(box->sampleEntryBox->getType());

                auto schemeType = makeCustomUnique<SchemeTypeBox, SchemeTypeBox>();
                schemeType->setSchemeType("podv");
                rinf->addSchemeTypeBox(std::move(schemeType));

                auto povdBox = makeCustomUnique<ProjectedOmniVideoBox, ProjectedOmniVideoBox>();

                povdBox->getProjectionFormatBox().setProjectionType(
                    (ProjectionFormatBox::ProjectionType) *projectionFormat);

                if (rwpk)
                {
                    auto rwpkBox = makeCustomUnique<RegionWisePackingBox, RegionWisePackingBox>();
                    rwpkBox->setProjPictureWidth(rwpk->projPictureWidth);
                    rwpkBox->setProjPictureHeight(rwpk->projPictureHeight);
                    rwpkBox->setPackedPictureWidth(rwpk->packedPictureWidth);
                    rwpkBox->setPackedPictureHeight(rwpk->packedPictureHeight);
                    rwpkBox->setConstituentPictureMatchingFlag(rwpk->constituenPictureMatchingFlag);

                    for (auto& region : rwpk->regions)
                    {
                        rwpkBox->addRegion(std::move(region->makeRegion()->region));
                    }

                    povdBox->setRegionWisePackingBox(std::move(rwpkBox));
                }

                if (covi)
                {
                    auto coviBox = makeCustomUnique<CoverageInformationBox, CoverageInformationBox>();
                    coviBox->setCoverageShapeType((CoverageInformationBox::CoverageShapeType) covi->coverageShape);
                    coviBox->setViewIdcPresenceFlag(covi->viewIdcPresenceFlag);
                    coviBox->setDefaultViewIdc((ViewIdcType) covi->defaultViewIdc);

                    for (auto& region : covi->regions)
                    {
                        auto sphereRegion     = makeCustomUnique<CoverageInformationBox::CoverageSphereRegion,
                                                             CoverageInformationBox::CoverageSphereRegion>();
                        sphereRegion->viewIdc = (ViewIdcType) region->viewIdc;
                        sphereRegion->region.centreAzimuth   = region->centreAzimuth;
                        sphereRegion->region.centreElevation = region->centreElevation;
                        sphereRegion->region.centreTilt      = region->centreTilt;
                        sphereRegion->region.azimuthRange    = region->azimuthRange;
                        sphereRegion->region.elevationRange  = region->elevationRange;
                        sphereRegion->region.interpolate     = region->interpolate;

                        coviBox->addSphereRegion(std::move(sphereRegion));
                    }

                    povdBox->setCoverageInformationBox(std::move(coviBox));
                }

                for (auto& compatibleScheme : compatibleSchemes)
                {
                    auto compatSchemeBox = makeCustomUnique<CompatibleSchemeTypeBox, CompatibleSchemeTypeBox>();
                    compatSchemeBox->setSchemeType(FourCCInt(compatibleScheme.type.c_str()));
                    compatSchemeBox->setSchemeVersion(compatibleScheme.version);
                    compatSchemeBox->setSchemeUri(compatibleScheme.uri.c_str());
                    rinf->addCompatibleSchemeTypeBox(std::move(compatSchemeBox));
                }

                if (stvi)
                {
                    auto stviBox = makeCustomUnique<StereoVideoBox, StereoVideoBox>();
                    stviBox->setStereoScheme(StereoVideoBox::StereoSchemeType::Povd);
                    StereoVideoBox::StereoIndicationType stereoIndicationType;
                    stereoIndicationType.Povd.useQuincunxSampling = 0;
                    stereoIndicationType.Povd.compositionType     = (StereoVideoBox::PovdFrameCompositionType) *stvi;
                    stviBox->setStereoIndicationType(stereoIndicationType);
                    rinf->addStereoVideoBox(std::move(stviBox));
                }

                if (rotn)
                {
                    auto rotnBox = makeCustomUnique<RotationBox, RotationBox>();
                    rotnBox->setRotation({rotn->yaw, rotn->pitch, rotn->roll});
                    povdBox->setRotationBox(std::move(rotnBox));
                }

                rinf->addProjectedOmniVideoBox(std::move(povdBox));

                box->sampleEntryBox->setType("resv");
                box->sampleEntryBox->addRestrictedSchemeInfoBox(std::move(rinf));
            }
        }

        std::unique_ptr<HandlerBox> AvcVideoSampleEntry::makeHandlerBox() const
        {
            auto handlerBox = Utils::make_unique<HandlerBox>(makeCustomUnique<::HandlerBox, ::HandlerBox>());
            handlerBox->handlerBox->setHandlerType("vide");
            handlerBox->handlerBox->setName("VideoHandler");
            return handlerBox;
        }

        std::unique_ptr<SampleEntryBox> AvcVideoSampleEntry::makeSampleEntryBox() const
        {
            auto box = makeCustomUnique<::AvcSampleEntry, ::SampleEntryBox>();

            box->setDataReferenceIndex(1);

            AvcConfigurationBox& cfg = box->getAvcConfigurationBox();

            AvcDecoderConfigurationRecord decCfg = cfg.getConfiguration();

            if (decCfg.makeConfigFromSPS({sps.begin(), sps.end()}))
            {
                decCfg.addNalUnit({sps.begin(), sps.end()}, AvcNalUnitType::SPS);
                decCfg.addNalUnit({pps.begin(), pps.end()}, AvcNalUnitType::PPS);
                cfg.setConfiguration(decCfg);
                box->setWidth(width);
                box->setHeight(height);

                auto wrappedSampleEntry =
                    Utils::make_unique<SampleEntryBox>(Utils::static_cast_unique_ptr<::SampleEntryBox>(std::move(box)));
                makePovdBoxes(wrappedSampleEntry);
                return wrappedSampleEntry;
            }
            else
            {
                return nullptr;
            }
        }

        std::unique_ptr<HandlerBox> HevcVideoSampleEntry::makeHandlerBox() const
        {
            auto handlerBox = Utils::make_unique<HandlerBox>(makeCustomUnique<::HandlerBox, ::HandlerBox>());
            handlerBox->handlerBox->setHandlerType("vide");
            handlerBox->handlerBox->setName("VideoHandler");
            return handlerBox;
        }

        std::unique_ptr<SampleEntryBox> HevcVideoSampleEntry::makeSampleEntryBox() const
        {
            auto box = makeCustomUnique<::HevcSampleEntry, ::SampleEntryBox>();

            HevcConfigurationBox& cfg = box->getHevcConfigurationBox();

            box->setDataReferenceIndex(1);

            HevcDecoderConfigurationRecord decCfg = cfg.getConfiguration();

            decCfg.makeConfigFromSPS({sps.begin(), sps.end()}, framerate);
            std::uint8_t arrayCompleteness = 1;
            decCfg.addNalUnit({vps.begin(), vps.end()}, HevcNalUnitType::VPS, arrayCompleteness);
            decCfg.addNalUnit({sps.begin(), sps.end()}, HevcNalUnitType::SPS, arrayCompleteness);
            decCfg.addNalUnit({pps.begin(), pps.end()}, HevcNalUnitType::PPS, arrayCompleteness);
            cfg.setConfiguration(decCfg);
            box->setWidth(width);
            box->setHeight(height);

            box->setType(sampleEntryType.value);

            auto wrappedSampleEntry =
                Utils::make_unique<SampleEntryBox>(Utils::static_cast_unique_ptr<::SampleEntryBox>(std::move(box)));
            makePovdBoxes(wrappedSampleEntry);
            return wrappedSampleEntry;
        }

        std::unique_ptr<HandlerBox> MP4AudioSampleEntry::makeHandlerBox() const
        {
            std::unique_ptr<HandlerBox> handlerBox =
                Utils::make_unique<HandlerBox>(makeCustomUnique<::HandlerBox, ::HandlerBox>());
            handlerBox->handlerBox->setHandlerType("soun");
            handlerBox->handlerBox->setName("SoundHandler");
            return handlerBox;
        }

        uint32_t TimedMetadataSampleEntry::getWidthFP() const
        {
            return 0;
        }

        uint32_t TimedMetadataSampleEntry::getHeightFP() const
        {
            return 0;
        }

        std::unique_ptr<SampleEntryBox> URIMetadataSampleEntry::makeSampleEntryBox() const
        {
            auto box = makeCustomUnique<::UriMetaSampleEntryBox, ::SampleEntryBox>();
            box->setDataReferenceIndex(1);

            UriBox& uriBox = box->getUriBox();
            uriBox.setUri({uri.begin(), uri.end()});

            if (init.size())
            {
                UriInitBox& uriInitBox = box->getUriInitBox();
                uriInitBox.setUriInitializationData({init.begin(), init.end()});
            }

            return Utils::make_unique<SampleEntryBox>(Utils::static_cast_unique_ptr<::SampleEntryBox>(std::move(box)));
        }

        std::unique_ptr<HandlerBox> URIMetadataSampleEntry::makeHandlerBox() const
        {
            auto handlerBox = Utils::make_unique<HandlerBox>(makeCustomUnique<::HandlerBox, ::HandlerBox>());
            handlerBox->handlerBox->setHandlerType("meta");
            handlerBox->handlerBox->setName("DataHandler");
            return handlerBox;
        }

        std::unique_ptr<SampleEntryBox> InitialViewingOrientationSampleEntry::makeSampleEntryBox() const
        {
            auto box = makeCustomUnique<::InitialViewingOrientation, ::SampleEntryBox>();
            box->setDataReferenceIndex(
                1);  // segmenter seems to support currently only one sample description entry per track
            return Utils::make_unique<SampleEntryBox>(Utils::static_cast_unique_ptr<::SampleEntryBox>(std::move(box)));
        }

        std::unique_ptr<HandlerBox> InitialViewingOrientationSampleEntry::makeHandlerBox() const
        {
            auto handlerBox = Utils::make_unique<HandlerBox>(makeCustomUnique<::HandlerBox, ::HandlerBox>());
            handlerBox->handlerBox->setHandlerType("meta");
            handlerBox->handlerBox->setName("DataHandler");
            return handlerBox;
        }

        FrameData StreamSegmenter::Segmenter::HevcExtractorSampleConstructor::toFrameData() const
        {
            BitStream sampleStream;

            // unsigned int(8) track_ref_index;
            // signed int(8) sample_offset;
            // unsigned int((lengthSizeMinusOne + 1) * 8) data_offset;
            // unsigned int((lengthSizeMinusOne + 1) * 8) data_length;

            sampleStream.write8Bits(trackId);
            sampleStream.write8Bits(sampleOffset);

            // @note HevcDecoderConfigurationRecord.lengthSizeMinusOne is 3 so this should be correct
            //       but to be completely compatible, code should decide if it needs to write 1,2 or 4 bytes
            //       depending on HevcDecoderConfigurationRecord.lengthSizeMinusOne
            sampleStream.write32Bits(dataOffset);
            sampleStream.write32Bits(dataLength);

            return StreamSegmenter::FrameData(sampleStream.getStorage().begin(), sampleStream.getStorage().end());
        }

        FrameData StreamSegmenter::Segmenter::HevcExtractorInlineConstructor::toFrameData() const
        {
            FrameData data;

            // unsigned int(8) length;
            // unsigned int(8) inline_data[length];

            data.push_back(inlineData.size());
            data.insert(data.end(), inlineData.begin(), inlineData.end());
            return data;
        }

        FrameData StreamSegmenter::Segmenter::HevcExtractor::toFrameData() const
        {
            FrameData sampleData;

            if (!sampleConstructor && !inlineConstructor)
            {
                throw std::runtime_error("Either sampleConstruct or inlineConstruct must be set for ExtractorSample");
            }
            if (!!inlineConstructor)
            {
                sampleData.push_back(2);  // constructor_type 2
                auto sampleBody = inlineConstructor->toFrameData();
                sampleData.insert(sampleData.end(), sampleBody.begin(), sampleBody.end());
            }
            if (!!sampleConstructor)
            {
                sampleData.push_back(0);  // constructor_type 0 ISO/IEC FDIS 14496-15:2014(E) A.7.2
                auto sampleBody = sampleConstructor->toFrameData();
                sampleData.insert(sampleData.end(), sampleBody.begin(), sampleBody.end());
            }

            return sampleData;
        }

        FrameData StreamSegmenter::Segmenter::HevcExtractorTrackFrameData::toFrameData() const
        {
            FrameData completeFrame;

            // reserve space for size of NAL unit
            completeFrame.push_back(0);
            completeFrame.push_back(0);
            completeFrame.push_back(0);
            completeFrame.push_back(0);

            // NAL unit header
            // 1 bit forbidden zero bit
            // 6 bits nal_unit_type shall be set to 49 for ISO/IEC 23008-2 video
            // 6 bits nuh_layer_id, shall be 0
            // 3 bits nuh_temporal_id_plus1 set to the lowest of the fields in all the aggregated NAL units.

            const std::uint16_t forbiddenZero = (0 << 15) & 0b1000000000000000;
            const std::uint16_t nalUnitType   = (49 << 9) & 0b0111111000000000;
            const std::uint16_t nuhLayerId    = (0 << 3) & 0b0000000111111000;
            const std::uint16_t nuhTemporalId = (nuhTemporalIdPlus1 << 0) & 0b0000000000000111;

            std::uint16_t nalUnitHeader = forbiddenZero | nalUnitType | nuhLayerId | nuhTemporalId;

            completeFrame.push_back(nalUnitHeader >> 8);
            completeFrame.push_back(nalUnitHeader & 0xff);

            for (auto& sample : samples)
            {
                auto sampleData = sample.toFrameData();
                completeFrame.insert(completeFrame.end(), sampleData.begin(), sampleData.end());
            }

            // Each NAL unit is preceeded by length of the NAL unit
            // HevcDecoderConfigurationRecord.mLengthSizeMinus1 == 3 so we use 32bit field length
            // ISO/IEC FDIS 14496-15:2014(E) 4.3.2
            std::uint32_t length = completeFrame.size() - 4;
            completeFrame[0]     = (length >> 24) & 0xff;
            completeFrame[1]     = (length >> 16) & 0xff;
            completeFrame[2]     = (length >> 8) & 0xff;
            completeFrame[3]     = (length >> 0) & 0xff;

            return completeFrame;
        }

        TrackDescription::TrackDescription(TrackMeta aTrackMeta,
                                           MediaDescription aTrackMediaDescription,
                                           const SampleEntry& aSampleEntry)
            : trackMeta(aTrackMeta)
        {
            sampleEntryBoxes.push_back(aSampleEntry.makeSampleEntryBox());
            mediaHeaderBox = makeMediaHeaderBox(aTrackMediaDescription, aTrackMeta.timescale);
            handlerBox     = aSampleEntry.makeHandlerBox();
            trackHeaderBox = Utils::make_unique<TrackHeaderBox>(makeCustomUnique<::TrackHeaderBox, ::TrackHeaderBox>());
            trackHeaderBox->trackHeaderBox->setTrackID(aTrackMeta.trackId.get());
            trackHeaderBox->trackHeaderBox->setWidth(aSampleEntry.getWidthFP());
            trackHeaderBox->trackHeaderBox->setHeight(aSampleEntry.getHeightFP());
            fillTrackHeaderBox(*trackHeaderBox->trackHeaderBox, aTrackMediaDescription);
        }

        TrackDescriptions trackDescriptionsOfMp4(const MP4Access& aMp4)
        {
            TrackDescriptions trackDescriptions;
            ::MovieBox moov;
            aMp4.parseBox(moov);
            for (TrackBox* trackBox : moov.getTrackBoxes())
            {
                MediaBox& mediaBox                         = trackBox->getMediaBox();
                MediaInformationBox& mediaInformationBox   = mediaBox.getMediaInformationBox();
                const ::MediaHeaderBox& mdhdBox            = mediaBox.getMediaHeaderBox();
                const String& handlerName                  = mediaBox.getHandlerBox().getName();
                SampleTableBox& sampleTableBox             = mediaInformationBox.getSampleTableBox();
                SampleDescriptionBox& sampleDescriptionBox = sampleTableBox.getSampleDescriptionBox();
                Vector<::SampleEntryBox*> sampleEntries    = sampleDescriptionBox.getSampleEntries<::SampleEntryBox>();

                TrackDescription& trackDescription = trackDescriptions[trackBox->getTrackHeaderBox().getTrackID()];
                trackDescription.trackMeta         = {
                    trackBox->getTrackHeaderBox().getTrackID(), RatU64(1, mdhdBox.getTimeScale()),
                    handlerName == "VideoHandler"
                        ? MediaType::Video
                        : handlerName == "SoundHandler"
                              ? MediaType::Audio
                              : handlerName == "DataHandler" ? MediaType::Data : MediaType::Other};
                trackDescription.mediaHeaderBox =
                    Utils::make_unique<MediaHeaderBox>(cloneBox(trackBox->getMediaBox().getMediaHeaderBox()));
                trackDescription.handlerBox =
                    Utils::make_unique<HandlerBox>(cloneBox(trackBox->getMediaBox().getHandlerBox()));
                trackDescription.trackHeaderBox =
                    Utils::make_unique<TrackHeaderBox>(cloneBox(trackBox->getTrackHeaderBox()));

                auto& entries = trackDescription.sampleEntryBoxes;
                for (auto sampleEntry : sampleEntries)
                {
                    UniquePtr<::SampleEntryBox> sampleEntryBoxCopy(sampleEntry->clone());
                    // std::unique_ptr<MediaHeaderBox> mediaHeaderBox = cloneBox();
                    // std::unique_ptr<HandlerBox> handlerBox;

                    BitStream a;
                    BitStream b;
                    sampleEntry->writeBox(a);
                    sampleEntryBoxCopy->writeBox(b);
                    assert(a.getStorage() == b.getStorage());

                    entries.push_back(Utils::make_unique<SampleEntryBox>(std::move(sampleEntryBoxCopy)));
                }
                // TrackDescription sampleDescription {
                //     std::move(sampleEntryBoxCopy)
                // };
            }

            return trackDescriptions;
        }

        MovieDescription makeMovieDescription(const MovieHeaderBox& aMovieHeaderBox)
        {
            auto matrix = aMovieHeaderBox.getMatrix();
            return MovieDescription{
                aMovieHeaderBox.getCreationTime(),                                      // creationTime
                aMovieHeaderBox.getModificationTime(),                                  // modificationTime
                RatU64(aMovieHeaderBox.getDuration(), aMovieHeaderBox.getTimeScale()),  // duration
                std::vector<int32_t>(matrix.begin(), matrix.end())                      // matrix
            };
        }

        namespace
        {
            void fillMovieHaderBox(MovieHeaderBox& aMovieHeaderBox,
                                   const MovieDescription& aMovieDescription,
                                   RatU64 aTimescale)
            {
                uint64_t creationTime     = aMovieDescription.creationTime;
                uint64_t modificationTime = aMovieDescription.modificationTime;
                RatU64 duration           = aMovieDescription.duration;
                aMovieHeaderBox.setTimeScale(std::uint32_t(aTimescale.per1().asDouble()));
                aMovieHeaderBox.setCreationTime(creationTime);
                aMovieHeaderBox.setModificationTime(modificationTime);
                aMovieHeaderBox.setDuration(std::uint32_t((duration / aTimescale).asDouble()));
            }

            void updateMediaInformationBox(MediaInformationBox& aBox, const TrackDescription& aTrackDescription)
            {
                switch (aTrackDescription.trackMeta.type)
                {
                case MediaType::Audio:
                {
                    aBox.setMediaType(MediaInformationBox::MediaType::Sound);
                    break;
                }
                case MediaType::Video:
                {
                    aBox.setMediaType(MediaInformationBox::MediaType::Video);
                    break;
                }
                default:
                    aBox.setMediaType(MediaInformationBox::MediaType::Null);
                }
            }
        }  // anonymous namespace

        InitSegment makeInitSegment(const TrackDescriptions& aTrackDescriptions,
                                    const MovieDescription& aMovieDescription,
                                    const bool aFragmented)
        {
            InitSegment initSegment;

            std::vector<RatU64> timescales;
            std::vector<RatU64*> timescaleRefs;
            for (const auto& descr : aTrackDescriptions)
            {
                timescales.push_back(descr.second.trackMeta.timescale);
            }
            for (auto& timescale : timescales)
            {
                timescaleRefs.push_back(&timescale);
            }
            shareDenominators(timescaleRefs.begin(), timescaleRefs.end());
            std::sort(timescales.begin(), timescales.end());

            auto moovOut = makeCustomUnique<::MovieBox, ::MovieBox>();

            // Patch boxes: remove samples, add MovieExtendsBox
            UniquePtr<MovieExtendsBox> movieExtendsBoxOut(CUSTOM_NEW(MovieExtendsBox, ()));
            auto ftypOut = makeCustomUnique<::FileTypeBox, ::FileTypeBox>();
            if (aMovieDescription.fileType)
            {
                ftypOut->setMajorBrand(aMovieDescription.fileType->majorBrand.c_str());
                ftypOut->setMinorVersion(aMovieDescription.fileType->minorVersion);
                for (auto& compatBrand : aMovieDescription.fileType->compatibleBrands)
                {
                    ftypOut->addCompatibleBrand(compatBrand.c_str());
                }
            }
            else
            {
                ftypOut->setMajorBrand("isom");
                ftypOut->setMinorVersion(512);
                ftypOut->addCompatibleBrand("isom");
                ftypOut->addCompatibleBrand("iso2");
                ftypOut->addCompatibleBrand("mp41");
                ftypOut->addCompatibleBrand("mp42");
            }

            fillMovieHaderBox(moovOut->getMovieHeaderBox(), aMovieDescription, {1, 1000});

            TrackId maxTrackId;
            if (aTrackDescriptions.size())
            {
                maxTrackId = aTrackDescriptions.begin()->first;
            }

            for (const auto& trackIdTrackDescription : aTrackDescriptions)
            {
                const TrackId trackId                       = trackIdTrackDescription.first;
                const TrackDescription& trackDescription    = trackIdTrackDescription.second;
                UniquePtr<TrackBox> trackOut                = makeCustomUnique<TrackBox, TrackBox>();
                MediaBox& mediaBoxOut                       = trackOut->getMediaBox();
                MediaInformationBox& mediaInformationBoxOut = mediaBoxOut.getMediaInformationBox();
                SampleTableBox& sampleTableBoxOut           = mediaInformationBoxOut.getSampleTableBox();
                maxTrackId                                  = std::max(maxTrackId, trackId);

                updateMediaInformationBox(mediaInformationBoxOut, trackDescription);

                copyBox(*trackDescription.trackHeaderBox->trackHeaderBox, trackOut->getTrackHeaderBox());

                copyBox(*trackDescription.mediaHeaderBox->mediaHeaderBox, mediaBoxOut.getMediaHeaderBox());

                copyBox(*trackDescription.handlerBox->handlerBox, mediaBoxOut.getHandlerBox());

                // @note update the rest to follow the same pattern
                if (trackDescription.trackMeta.trackType)
                {
                    trackOut->setHasTrackTypeBox(true);
                    trackOut->getTrackTypeBox().setMajorBrand(trackDescription.trackMeta.trackType->majorBrand.c_str());
                    trackOut->getTrackTypeBox().setMinorVersion(trackDescription.trackMeta.trackType->minorVersion);
                    for (const auto& brand : trackDescription.trackMeta.trackType->compatibleBrands)
                    {
                        trackOut->getTrackTypeBox().addCompatibleBrand(brand.c_str());
                    }
                }

                if (trackDescription.trackReferences.size())
                {
                    trackOut->setHasTrackReferences(true);
                    setTrackReferences(trackOut->getTrackReferenceBox(), trackDescription.trackReferences);
                }

                if (trackDescription.trackMeta.type == StreamSegmenter::MediaType::Audio)
                {
                    trackOut->getTrackHeaderBox().setVolume(0x0100);
                }

                if (trackDescription.alternateGroup)
                {
                    trackOut->getTrackHeaderBox().setAlternateGroup(*trackDescription.alternateGroup);
                }

                if (trackDescription.obsp)
                {
                    TrackGroupTypeBox trgr("obsp", trackDescription.obsp->groupId.get());
                    trackOut->getTrackGroupBox().addTrackGroupTypeBox(trgr);
                    trackOut->setHasTrackGroup(true);
                }

                if (trackDescription.googleVRType)
                {
                    SphericalVideoV1Box::GlobalMetadata data{};

                    data.spherical         = true;
                    data.stitched          = true;
                    data.stitchingSoftware = "";
                    data.projectionType    = SphericalVideoV1Box::V1Projection::EQUIRECTANGULAR;

                    switch (*trackDescription.googleVRType)
                    {
                    case GoogleVRType::Mono:
                    {
                        data.stereoMode = SphericalVideoV1Box::V1StereoMode::MONO;
                    }
                    break;
                    case GoogleVRType::StereoLR:
                    {
                        data.stereoMode = SphericalVideoV1Box::V1StereoMode::STEREOSCOPIC_LEFT_RIGHT;
                    }
                    break;
                    case GoogleVRType::StereoTB:
                    {
                        data.stereoMode = SphericalVideoV1Box::V1StereoMode::STEREOSCOPIC_TOP_BOTTOM;
                    }
                    break;
                    }
                    trackOut->getSphericalVideoV1Box().setGlobalMetadata(data);
                    trackOut->setHasSphericalVideoV1Box(true);
                }

                // MediaBox& mediaBoxIn                        = trackIn->getMediaBox();

                // MediaInformationBox& mediaInformationBoxIn  = mediaBoxIn.getMediaInformationBox();

                // copyBox(mediaInformationBoxIn.getVideoMediaHeaderBox(),
                //         mediaInformationBoxOut.getVideoMediaHeaderBox());

                // copyBox(mediaInformationBoxIn.getNullMediaHeaderBox(),
                //         mediaInformationBoxOut.getNullMediaHeaderBox());

                // copyBox(mediaInformationBoxIn.getSoundMediaHeaderBox(),
                //         mediaInformationBoxOut.getSoundMediaHeaderBox());

                std::shared_ptr<DataEntryUrlBox> dataEntryBox(new DataEntryUrlBox(DataEntryUrlBox::SelfContained));
                mediaInformationBoxOut.getDataInformationBox().addDataEntryBox(dataEntryBox);

                SampleDescriptionBox& sampleDescriptionBox = sampleTableBoxOut.getSampleDescriptionBox();
                for (auto& sampleEntryBox : trackDescription.sampleEntryBoxes)
                {
                    UniquePtr<::SampleEntryBox> sampleEntryCopy(sampleEntryBox->sampleEntryBox->clone());
                    sampleDescriptionBox.addSampleEntry(std::move(sampleEntryCopy));
                }

                UniquePtr<TrackExtendsBox> trackExtendsBoxOut(CUSTOM_NEW(TrackExtendsBox, ()));
                MOVIEFRAGMENTS::SampleDefaults sampleDefaults = {
                    trackDescription.trackMeta.trackId.get(),
                    1,   // defaultSampleDescriptionIndex
                    0,   // defaultSampleDuration
                    0,   // defaultSampleSize
                    {0}  // defaultSampleFlags
                };
                trackExtendsBoxOut->setFragmentSampleDefaults(sampleDefaults);
                movieExtendsBoxOut->addTrackExtendsBox(std::move(trackExtendsBoxOut));
                moovOut->addTrackBox(std::move(trackOut));
            }
            moovOut->getMovieHeaderBox().setNextTrackID(maxTrackId.get() + 1);

            if (aFragmented)
            {
                moovOut->addMovieExtendsBox(std::move(movieExtendsBoxOut));
            }

            initSegment.moov.reset(new MovieBox{std::move(moovOut)});
            initSegment.ftyp.reset(new FileTypeBox{std::move(ftypOut)});

            return initSegment;
        }

        Segments makeSegmentsOfTracks(FrameAccessors&& aFrameAccessors, const SegmenterConfig& aSegmenterConfig)
        {
            TrackIds trackIds = trackIdsOfFrameAccessors(aFrameAccessors);

            std::map<TrackId, Frames> framesByTrackId;
            for (auto trackId : trackIds)
            {
                auto frames = findTrackFramesByTrackId(aFrameAccessors, trackId).frames;
                framesByTrackId.insert(std::make_pair(trackId, Frames(frames.begin(), frames.end())));
            }

            assert(aFrameAccessors.size());

            Segments segments;

            // @note anyprocesses isn't a good condition, as with current
            // approach we may get empty segments
            bool anyProcessed         = false;
            FrameTime segmentDuration = aSegmenterConfig.segmentDuration.cast<FrameTime>();
            FrameTime lastCts         = segmentDuration;
            FrameTime nextT0          = FrameTime(0, 1);
            SequenceId sequenceId     = SequenceId(aSegmenterConfig.baseSequenceId);

            do
            {
                Segment segment;
                segment.sequenceId = sequenceId;

                segment.t0       = nextT0;
                segment.duration = Duration();
                anyProcessed     = false;
                for (auto trackId : trackIds)
                {
                    auto& segmentTrack               = segment.tracks[trackId];
                    segmentTrack.trackInfo.t0        = nextT0;
                    segmentTrack.trackInfo.trackMeta = findTrackFramesByTrackId(aFrameAccessors, trackId).trackMeta;
                    auto& dstFrames                  = segmentTrack.frames;
                    auto& srcFrames                  = framesByTrackId.find(trackId)->second;
                    auto srcFramesIt                 = srcFrames.begin();

                    int numIDRs            = 0;
                    bool seenSecondIDR     = false;
                    bool firstFrame        = true;
                    auto durationCondition = [&](decltype(srcFrames.begin()) iterator) {
                        return iterator->getFrameInfo().cts.front() <= lastCts;
                    };

                    while (srcFramesIt != srcFrames.end() && (durationCondition(srcFramesIt) || !seenSecondIDR))
                    {
                        auto cur = srcFramesIt;
                        ++srcFramesIt;
                        if (firstFrame && !cur->getFrameInfo().isIDR)
                        {
                            std::cerr << "Warning: track " << trackId << " segment doesn't begin with IDR" << std::endl;
                        }

                        // when the duration condition doesn't hold anymore, keep looking for an IDR to cut the segment
                        // at
                        if (!durationCondition(cur) && cur->getFrameInfo().isIDR)
                        {
                            nextT0        = cur->getFrameInfo().cts.front();
                            seenSecondIDR = true;
                            // this terminates the loop
                        }
                        else
                        {
                            segment.duration += cur->getFrameInfo().duration;
                            dstFrames.push_back(*cur);
                            numIDRs += cur->getFrameInfo().isIDR;
                            anyProcessed = true;
                            srcFrames.pop_front();
                        }
                        firstFrame = false;
                    }
                }
                if (anyProcessed)
                {
                    segments.push_back(std::move(segment));
                }
                lastCts += segmentDuration;

                ++sequenceId;
            } while (anyProcessed);

            return segments;
        }

        Segments makeSegmentsOfMp4(const MP4Access& aMp4, const SegmenterConfig& aSegmenterConfig)
        {
            ::MovieBox moov;
            aMp4.parseBox(moov);

            FrameAccessors frameAccessors;

            for (auto trackBox : moov.getTrackBoxes())
            {
                std::shared_ptr<Track> track(new Track(aMp4, *trackBox, moov.getMovieHeaderBox().getTimeScale()));
                auto frames = track->getFrames();
                frameAccessors.push_back(TrackFrames{track->getTrackMeta(), std::move(frames)});
            }

            return makeSegmentsOfTracks(std::move(frameAccessors), aSegmenterConfig);
        }

    }  // namespace Segmenter


    using namespace Segmenter;

    class SidxWriterNone : public SidxWriter
    {
    public:
        void addSubsegment(Segmenter::Segment) override
        {
        }

        void setFirstSubsegmentOffset(std::streampos) override
        {
        }

        void addSubsegmentSize(std::streampos) override
        {
        }

        Utils::Optional<SidxInfo> writeSidx(std::ostream&, Utils::Optional<std::ostream::pos_type>) override
        {
            return {};
        }

        void setOutput(std::ostream*) override
        {
        }
    };

    SidxWriterImpl::SidxWriterImpl(size_t aExpectedSize)
        : mOutput(nullptr)
        , mFirstSubsegmentOffset(0)
        , mExpectedSize(aExpectedSize)
    {
        // nothing
    }

    SidxWriterImpl::~SidxWriterImpl() = default;

    void SidxWriterImpl::setOutput(std::ostream* aStream)
    {
        mOutput = aStream;
    }

    void SidxWriterImpl::addSubsegment(Segmenter::Segment aSubsegment)
    {
        mSubsegments.push_back(aSubsegment);
    }

    void SidxWriterImpl::addSubsegmentSize(std::streampos aSubsegmentSize)
    {
        mSubsegmentSizes.push_back(aSubsegmentSize);
    }

    void SidxWriterImpl::setFirstSubsegmentOffset(std::streampos aFirstSubsegmentOffset)
    {
        mFirstSubsegmentOffset = aFirstSubsegmentOffset;
    }

    Utils::Optional<SidxInfo> SidxWriterImpl::writeSidx(std::ostream& aOutput,
                                                        Utils::Optional<std::ostream::pos_type> aPosition)
    {
        auto& output      = mOutput ? *mOutput : aOutput;
        auto origPosition = output.tellp();
        if (aPosition)
        {
            output.seekp(*aPosition);
        }
        auto beginPosition = output.tellp();

        BitStream bs;

        auto& firstSegment = mSubsegments.front();
        auto earliest      = firstSegment.tracks.begin()->second.trackInfo.t0;
        for (auto trackInfo : firstSegment.tracks)
        {
            if (trackInfo.second.trackInfo.t0 < earliest)
            {
                earliest = trackInfo.second.trackInfo.t0;
            }
        }

        // note that mSubsegmentSizes may be empty
        auto segmentSizeIt = mSubsegmentSizes.begin();
        // there must always be one subsegment offset for indicating the size of the last
        // segment, unless there are no subsegment offsets
        // assert(mSubsegmentSizes.size() == 0 ||
        //        (mSubsegmentSizes.size() == mSubsegments.size() + 1));

        SegmentIndexBox sidx(1);
        sidx.setSpaceReserve(mExpectedSize);
        uint32_t timescale = timescaleOfTrackSegments(firstSegment.tracks);
        sidx.setReferenceId(1);
        sidx.setTimescale(timescale);
        sidx.setEarliestPresentationTime(std::uint64_t(earliest.minimize().num));

        sidx.setFirstOffset(uint64_t(mFirstSubsegmentOffset));

        Duration curTime;

        for (auto& segment : mSubsegments)
        {
            bool hasSegmentOffset = segmentSizeIt != mSubsegmentSizes.end();
            SegmentIndexBox::Reference r;
            r.referenceType = 0;
            if (hasSegmentOffset)
            {
                r.referencedSize = std::uint32_t(*segmentSizeIt);
            }
            else
            {
                r.referencedSize = 0;
            }
            r.subsegmentDuration = std::uint32_t((segment.duration * RatU64(timescale, 1)).asDouble());
            r.startsWithSAP      = 1;
            r.sapType            = 1;
            r.sapDeltaTime       = 0;
            sidx.addReference(r);

            curTime += segment.duration;
            if (hasSegmentOffset)
            {
                ++segmentSizeIt;
            }
        }

        sidx.writeBox(bs);

        flush(bs, output);

        auto size = output.tellp() - origPosition;
        output.seekp(origPosition);

        SidxInfo sidxInfo{};
        sidxInfo.position = beginPosition;
        sidxInfo.size     = static_cast<size_t>(size);
        return sidxInfo;
    }


    Writer* Writer::create()
    {
        return new WriterImpl;
    }

    void Writer::destruct(Writer* aWriter)
    {
        delete aWriter;
    }

    WriterImpl::WriterImpl()
        : mSidxWriter(new SidxWriterNone)
    {
        // nothing
    }

    WriterImpl::~WriterImpl() = default;

    SidxWriter* WriterImpl::newSidxWriter(size_t aExpectedSize)
    {
        mSidxWriter.reset(new SidxWriterImpl(aExpectedSize));
        return mSidxWriter.get();
    }

    void WriterImpl::setWriteSegmentHeader(bool aWriteSegmentHeader)
    {
        mWriteSegmentHeader = aWriteSegmentHeader;
    }

    void WriterImpl::writeInitSegment(std::ostream& aOut, const Segmenter::InitSegment& aInitSegment)
    {
        Segmenter::writeInitSegment(aOut, aInitSegment);
    }

    void WriterImpl::writeSubsegments(std::ostream& aOut, const std::list<Segment> aSubsegments)
    {
        if (mWriteSegmentHeader)
        {
            writeSegmentHeader(aOut);
        }
        for (auto& subsegment : aSubsegments)
        {
            mSidxWriter->addSubsegment(subsegment);
        }
        auto sidxInfo = mSidxWriter->writeSidx(aOut, {});
        for (auto& subsegment : aSubsegments)
        {
            auto before = aOut.tellp();
            writeTrackRunWithData(aOut, subsegment);
            auto after = aOut.tellp();
            mSidxWriter->addSubsegmentSize(after - before);
        }
        if (sidxInfo)
        {
            mSidxWriter->writeSidx(aOut, sidxInfo->position);
        }

        // assert that we wrote as much as in the first round..
        // assert(!aSubsegments.size() || aOut.tellp() - afterSidxPosition == subsegmentOffsets.front());
    }

    void WriterImpl::writeSegment(std::ostream& aOut, const Segment aSegment)
    {
        writeSubsegments(aOut, {aSegment});
    }


    MovieWriter* MovieWriter::create(std::ostream& aOut)
    {
        return new MovieWriterImpl(aOut);
    }

    void MovieWriter::destruct(MovieWriter* aWriter)
    {
        delete aWriter;
    }

    MovieWriterImpl::MovieWriterImpl(std::ostream& aOut)
        : mOut(aOut)
    {
        // nothing
    }

    MovieWriterImpl::~MovieWriterImpl()
    {
        finalize();
    }

    void MovieWriterImpl::writeInitSegment(const Segmenter::InitSegment& aInitSegment)
    {
        assert(!mInitSegment);
        mInitSegment = aInitSegment;

        for (auto trackBox : mInitSegment->moov->movieBox->getTrackBoxes())
        {
            TrackId trackId = trackBox->getTrackHeaderBox().getTrackID();
            mTrackInfo.emplace(std::make_pair(trackId, Info{}));
            Info& info           = mTrackInfo.at(trackId);
            info.trackBox        = trackBox;
            SampleTableBox& minf = info.trackBox->getMediaBox().getMediaInformationBox().getSampleTableBox();
            minf.getSampleToChunkBox().setSampleCountMaxSafety(std::numeric_limits<std::int64_t>::max());
        }

        BitStream stream;
        aInitSegment.ftyp->fileTypeBox->writeBox(stream);
        flush(stream, mOut);

        mMdatHeaderOffset = mOut.tellp();

        // @note currently do always 64bit mdat
        stream.write32Bits(0);
        stream.writeString("mdat");
        flush(stream, mOut);
    }

    void MovieWriterImpl::writeSegment(const Segmenter::Segment& aSegment)
    {
        assert(mInitSegment);

        for (auto& trackSegment : aSegment.tracks)
        {
            auto& trackId = trackSegment.first;
            auto& segment = trackSegment.second;
            Info& info    = mTrackInfo.at(trackId);

            std::streamsize offset = mOut.tellp();
            info.chunkOffsets.push_back(std::uint64_t(offset));

            SampleTableBox& stbl = info.trackBox->getMediaBox().getMediaInformationBox().getSampleTableBox();

            ::MediaHeaderBox& mdhd = info.trackBox->getMediaBox().getMediaHeaderBox();

            std::shared_ptr<const CompositionOffsetBox> compositionOffsetBox = stbl.getCompositionOffsetBox();

            Duration scaler{mdhd.getTimeScale(), 1};

            TimeToSampleBox& stts = stbl.getTimeToSampleBox();

            for (auto& frameProxy : segment.frames)
            {
                Frame frame = *frameProxy;
                mOut.write(reinterpret_cast<char*>(&frame.data[0]), static_cast<std::streamsize>(frame.data.size()));
                info.sampleSizes.push_back(static_cast<uint32_t>(frame.data.size()));
                stts.addSampleDelta(std::uint32_t((frame.info.duration * scaler).asDouble()));
                info.duration += std::uint32_t((frame.info.duration * scaler).asDouble());

                auto ctsOffset =
                    std::uint32_t(((frame.info.cts.front() - info.decodingTime) * scaler.cast<FrameTime>()).asDouble());
                if (!info.compositionOffsets.size() || info.compositionOffsets.back().mSampleOffset != ctsOffset)
                {
                    info.compositionOffsets.push_back({1, ctsOffset});
                }
                else
                {
                    ++info.compositionOffsets.back().mSampleCount;
                }

                if (frame.info.isIDR)
                {
                    info.syncSampleIndices.push_back(info.sampleIndex);
                }
                else
                {
                    info.anyNonSyncSample = true;
                }

                info.decodingTime += frame.info.duration.cast<FrameTime>();
                ++info.sampleIndex;
            }

            info.dtsCtsOffset = segment.trackInfo.dtsCtsOffset;

            if (info.chunks.size() == 0 || info.chunks.back().samplesPerChunk != segment.frames.size())
            {
                SampleToChunkBox::ChunkEntry chunk{};

                chunk.firstChunk             = static_cast<uint32_t>(info.chunkOffsets.size());
                chunk.samplesPerChunk        = static_cast<uint32_t>(segment.frames.size());
                chunk.sampleDescriptionIndex = 1;

                info.chunks.push_back(chunk);
            }
        }
    }

    void MovieWriterImpl::finalize()
    {
        if (!mFinalized)
        {
            mFinalized = true;

            BitStream stream;

            std::streampos moovOffset = mOut.tellp();

            mOut.seekp(mMdatHeaderOffset);

            // @note currently always write 64bit sized mdat
            stream.write32Bits(static_cast<uint32_t>(moovOffset - mMdatHeaderOffset));
            flush(stream, mOut);

            mOut.seekp(moovOffset);
            uint64_t movieDuration = 0;

            for (auto& trackInfo : mTrackInfo)
            {
                Info& info   = trackInfo.second;
                auto movieTs = mInitSegment->moov->movieBox->getMovieHeaderBox().getTimeScale();
                auto mediaTs = info.trackBox->getMediaBox().getMediaHeaderBox().getTimeScale();

                if (info.dtsCtsOffset.num)
                {
                    EditBox edts;
                    std::shared_ptr<EditListBox> editListBox(new EditListBox);
                    EditListBox::EntryVersion0 shift{};
                    shift.mSegmentDuration   = 0;
                    shift.mMediaTime         = std::int32_t((FrameTime{mediaTs, 1} * info.dtsCtsOffset).asDouble());
                    shift.mMediaRateInteger  = 1;
                    shift.mMediaRateFraction = 0;
                    editListBox->addEntry(shift);
                    edts.setEditListBox(editListBox);
                    info.trackBox->setEditBox(edts);
                }
                SampleTableBox& stbl = info.trackBox->getMediaBox().getMediaInformationBox().getSampleTableBox();
                stbl.getChunkOffsetBox().setChunkOffsets({info.chunkOffsets.begin(), info.chunkOffsets.end()});
                stbl.getSampleSizeBox().setSampleCount(static_cast<uint32_t>(info.sampleSizes.size()));
                stbl.getSampleSizeBox().setEntrySize({info.sampleSizes.begin(), info.sampleSizes.end()});
                if (info.compositionOffsets.size() != 1 || info.compositionOffsets.front().mSampleOffset != 0)
                {
                    CompositionOffsetBox ctts;
                    for (auto ct : info.compositionOffsets)
                    {
                        ctts.addCompositionOffsetEntryVersion0(ct);
                    }
                    stbl.setCompositionOffsetBox(ctts);
                }
                if (info.anyNonSyncSample)
                {
                    SyncSampleBox stss;
                    for (auto sampleIndex : info.syncSampleIndices)
                    {
                        stss.addSample(sampleIndex + 1);
                    }
                    stbl.setSyncSampleBox(stss);
                }
                SampleToChunkBox& stsc = stbl.getSampleToChunkBox();
                for (auto chunk : info.chunks)
                {
                    stsc.addChunkEntry(chunk);
                }
                info.trackBox->getMediaBox().getMediaHeaderBox().setDuration(info.duration);
                uint64_t trackDuration = static_cast<uint64_t>(ceil(double(info.duration * movieTs) / mediaTs));
                info.trackBox->getTrackHeaderBox().setDuration(trackDuration);
                if (trackDuration > movieDuration)
                {
                    movieDuration = trackDuration;
                }
            }
            mInitSegment->moov->movieBox->getMovieHeaderBox().setDuration(movieDuration);
            mInitSegment->moov->movieBox->writeBox(stream);
            flush(stream, mOut);
        }
    }

}  // namespace StreamSegmenter
