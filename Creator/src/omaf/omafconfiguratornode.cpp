
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
#include "omafconfiguratornode.h"
#include "parser/h265parser.hpp"
#include "omafproperties.h"

namespace VDD
{
    OmafConfiguratorNode::OmafConfiguratorNode(Config aConfig) : mConfig(aConfig)
    {
        // nothing
    }

    OmafConfiguratorNode::~OmafConfiguratorNode()
    {
        // nothing
    }

    std::vector<Streams> OmafConfiguratorNode::process(const Streams& aStreams)
    {
        std::vector<Streams> frames;

        if (mConfig.counter)
        {
            ++*mConfig.counter;
        }

        if (aStreams.isEndOfStream())
        {
            mEnd = true;
        }
        if (mEnd)
        {
            frames.push_back({Data(EndOfStream())});
        }
        else if (mFirst)
        {
            // The very first Data package

            std::vector<uint8_t> fpSeiNAL;
            std::vector<Meta> meta;
            for (auto& view : aStreams)
            {
                meta.push_back(view.getMeta());
            }

            if (mConfig.videoOutput != PipelineOutputVideo::Mono)
            {
                createFramePackingSEI(aStreams.front(), fpSeiNAL);
            }
            mFirst = false;

            // Create OMAF projection SEI
            std::vector<uint8_t> seiNAL;
            createProjectionSEI(aStreams.front(), seiNAL);
            // create new Data with the SEI in front, and the original data after it
            std::vector<std::uint8_t> videoData(seiNAL);
            if (!fpSeiNAL.empty())
            {
                // add framepacking, if available
                videoData.insert(videoData.end(), fpSeiNAL.begin(), fpSeiNAL.end());
            }
            size_t i = 0;

            for (auto& view : aStreams)
            {
                const uint8_t* data = static_cast<const std::uint8_t*>(view.getCPUDataReference().address[0]);
                const uint8_t* dataEnd = data + view.getCPUDataReference().size[0];
                videoData.insert(videoData.end(), data, dataEnd);
                std::vector<std::vector<std::uint8_t>> matrix(1, std::move(videoData));

                frames.push_back({ Data(CPUDataVector(std::move(matrix)),
                    meta[i++],
                    view.getStreamId()) });
            }
        }
        else
        {
            // normal case, not first or last frame
            frames.push_back({ aStreams.front() });
        }

        return frames;
    }

    void OmafConfiguratorNode::createProjectionSEI(const Data& input, std::vector<uint8_t>& seiNal)
    {
        // input is in NAL unit format: length + data (with EPB). However we need just the NAL header (2 bytes), so EPB is not relevant, and hence we get RBSP just by copying the data to bitstr
        const std::uint8_t* inputNAL = static_cast<const std::uint8_t*>(input.getCPUDataReference().address[0]);
        Parser::BitStream bitstr;
        for (int i = 4; i < 7; i++)
        {
            bitstr.write8Bits(inputNAL[i]);
        }
        H265::NalUnitHeader naluHeader;
        H265Parser::parseNalUnitHeader(bitstr, naluHeader);
        bitstr.clear();

        std::uint32_t length = OMAF::createProjectionSEI(bitstr, static_cast<int>(naluHeader.mNuhTemporalIdPlus1));
        Parser::BitStream seiLengthField;
        seiLengthField.write32Bits(length);

        seiNal.insert(seiNal.end(), seiLengthField.getStorage().begin(), seiLengthField.getStorage().end());
        seiNal.insert(seiNal.end(), bitstr.getStorage().begin(), bitstr.getStorage().end());
    }

    void OmafConfiguratorNode::createFramePackingSEI(const Data& input, std::vector<uint8_t>& seiNal)
    {
        // input is in NAL unit format: length + data (with EPB). However we need just the NAL header (2 bytes), so EPB is not relevant
        const std::uint8_t* inputNAL = static_cast<const std::uint8_t*>(input.getCPUDataReference().address[0]);
        Parser::BitStream bitstr;
        for (int i = 4; i < 7; i++)
        {
            bitstr.write8Bits(inputNAL[i]);
        }
        H265::NalUnitHeader naluHeader;
        H265Parser::parseNalUnitHeader(bitstr, naluHeader);
        int temporalIdPlus1 = static_cast<int>(naluHeader.mNuhTemporalIdPlus1);

        H265::FramePackingSEI packing;
        packing.fpArrangementId = 0;
        packing.fpCancel = 0;
        if (mConfig.videoOutput == PipelineOutputVideo::TopBottom)
        {
            packing.fpArrangementType = 4;
        }
        else if (mConfig.videoOutput == PipelineOutputVideo::SideBySide)
        {
            packing.fpArrangementType = 3;
        }
        // type = 5 is not supported

        packing.quincunxSamplingFlag = 0;
        packing.contentInterpretationType = 1;  // left is on top or on left
        packing.spatialFlipping = 0;
        packing.frame0Flipped = 0;
        packing.fieldViews = 0;
        packing.currentFrameIsFrame0 = 0;
        packing.frame0SelfContained = 0;
        packing.frame1SelfContained = 0;
        packing.frame0GridX = 0;
        packing.frame0GridY = 0;
        packing.frame1GridX = 0;    // just use dummy values, since in OMAF these fields are irrelevant, as semantics in OMAF section 7.5.1 apply 
        packing.frame1GridY = 0;
        packing.fpArrangementPersistence = 1; // continues until further notice / bitstream ends
        packing.upsampledAspectRatio = 0;

        bitstr.clear();

        std::uint32_t length = H265Parser::writeFramePackingSEINal(bitstr, packing, temporalIdPlus1);
        Parser::BitStream seiLengthField;
        seiLengthField.write32Bits(length);

        seiNal.insert(seiNal.end(), seiLengthField.getStorage().begin(), seiLengthField.getStorage().end());
        seiNal.insert(seiNal.end(), bitstr.getStorage().begin(), bitstr.getStorage().end());
    }

    // test code only
    void OmafConfiguratorNode::createFakeRwpk(VDD::CodedFrameMeta& aMeta, PipelineOutput aVideoOutput)
    {
        unsigned int fullWidth = aMeta.width;
        unsigned int fullHeight = aMeta.height;

        RegionPacking regionPacking;
        regionPacking.projPictureWidth = fullWidth;
        regionPacking.projPictureHeight = fullHeight;
        if (aVideoOutput == PipelineOutputVideo::Mono)
        {
            // mono
            regionPacking.constituentPictMatching = false;
            regionPacking.packedPictureWidth = fullWidth;
            regionPacking.packedPictureHeight = fullHeight;

            Region region1;
            region1.projTop = 0;
            region1.packedTop = fullHeight / 4;
            region1.projLeft = 0;
            region1.packedLeft = 0;
            region1.transform = 0;

            region1.projWidth = fullWidth / 2;
            region1.projHeight = fullHeight / 2;
            region1.packedWidth = fullWidth / 2;
            region1.packedHeight = fullHeight / 4;

            Region region2;
            region2.projTop = 0;
            region2.packedTop = fullHeight / 4;
            region2.projLeft = fullWidth / 2;
            region2.packedLeft = fullWidth / 2;
            region2.transform = 0;

            region2.projWidth = fullWidth / 2;
            region2.projHeight = fullHeight / 2;
            region2.packedWidth = fullWidth / 2;
            region2.packedHeight = fullHeight / 4;

            Region region3;
            region3.projTop = fullHeight / 2;
            region3.packedTop = fullHeight / 2;
            region3.projLeft = 0;
            region3.packedLeft = 0;
            region3.transform = 0;

            region3.projWidth = fullWidth / 2;
            region3.projHeight = fullHeight / 2;
            region3.packedWidth = fullWidth / 2;
            region3.packedHeight = fullHeight / 4;

            Region region4;
            region4.projTop = fullHeight / 2;
            region4.packedTop = fullHeight / 2;
            region4.projLeft = fullWidth / 2;
            region4.packedLeft = fullWidth / 2;
            region4.transform = 0;

            region4.projWidth = fullWidth / 2;
            region4.projHeight = fullHeight / 2;
            region4.packedWidth = fullWidth / 2;
            region4.packedHeight = fullHeight / 4;

            aMeta.regionPacking = regionPacking;
            aMeta.regionPacking.get().regions.push_back(std::move(region1));
            aMeta.regionPacking.get().regions.push_back(std::move(region2));
            aMeta.regionPacking.get().regions.push_back(std::move(region3));
            aMeta.regionPacking.get().regions.push_back(std::move(region4));
        }
        else if (aVideoOutput == PipelineOutputVideo::TopBottom)
        {
            // top-bottom framepacked stereo
            regionPacking.constituentPictMatching = true;
            regionPacking.packedPictureWidth = fullWidth;
            regionPacking.packedPictureHeight = fullHeight;
            aMeta.regionPacking = regionPacking;

            // First left
            Region region11;
            region11.projTop = 0;
            region11.packedTop = 0;
            region11.projLeft = 0;
            region11.packedLeft = 0;
            region11.transform = 0;

            region11.projWidth = fullWidth / 3;
            region11.projHeight = fullHeight / 4;
            region11.packedWidth = fullWidth / 3;
            region11.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region11));

            Region region12;
            region12.projTop = 0;
            region12.packedTop = 0;
            region12.projLeft = fullWidth / 3;
            region12.packedLeft = fullWidth /3;
            region12.transform = 0;

            region12.projWidth = fullWidth / 3;
            region12.projHeight = fullHeight / 4;
            region12.packedWidth = fullWidth / 3;
            region12.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region12));

            Region region13;
            region13.projTop = 0;
            region13.packedTop = 0;
            region13.projLeft = 2 * fullWidth / 3;
            region13.packedLeft = 2 * fullWidth / 3;
            region13.transform = 0;

            region13.projWidth = fullWidth / 3;
            region13.projHeight = fullHeight / 4;
            region13.packedWidth = fullWidth / 3;
            region13.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region13));

            Region region14;
            region14.projTop = fullHeight / 4;
            region14.packedTop = fullHeight / 4;
            region14.projLeft = 0;
            region14.packedLeft = 0;
            region14.transform = 5;

            region14.projWidth = fullWidth / 3;
            region14.projHeight = fullHeight / 4;
            region14.packedWidth = fullWidth / 3;
            region14.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region14));

            Region region15;
            region15.projTop = fullHeight / 4;
            region15.packedTop = fullHeight / 4;
            region15.projLeft = fullWidth / 3;
            region15.packedLeft = fullWidth / 3;
            region15.transform = 0;

            region15.projWidth = fullWidth / 3;
            region15.projHeight = fullHeight / 4;
            region15.packedWidth = fullWidth / 3;
            region15.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region15));

            Region region16;
            region16.projTop = fullHeight / 4;
            region16.packedTop = fullHeight / 4;
            region16.projLeft = 2*fullWidth / 3;
            region16.packedLeft = 2* fullWidth / 3;
            region16.transform = 0;

            region16.projWidth = fullWidth/3;
            region16.projHeight = fullHeight / 4;
            region16.packedWidth = fullWidth/3;
            region16.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region16));


            // Then right
            /*
            Region region21;
            region21.projTop = fullHeight / 2;
            region21.packedTop = fullHeight / 2;
            region21.projLeft = 0;
            region21.packedLeft = 0;
            region21.transform = 0;

            region21.projWidth = fullWidth / 3;
            region21.projHeight = fullHeight / 4;
            region21.packedWidth = fullWidth / 3;
            region21.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region21));

            Region region22;
            region22.projTop = fullHeight / 2;
            region22.packedTop = fullHeight / 2;
            region22.projLeft = fullWidth / 3;
            region22.packedLeft = fullWidth / 3;
            region22.transform = 0;

            region22.projWidth = fullWidth / 3;
            region22.projHeight = fullHeight / 4;
            region22.packedWidth = fullWidth / 3;
            region22.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region22));

            Region region23;
            region23.projTop = fullHeight / 2;
            region23.packedTop = fullHeight / 2;
            region23.projLeft = 2 * fullWidth / 3;
            region23.packedLeft = 2 * fullWidth / 3;
            region23.transform = 0;

            region23.projWidth = fullWidth / 3;
            region23.projHeight = fullHeight / 4;
            region23.packedWidth = fullWidth / 3;
            region23.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region23));

            Region region24;
            region24.projTop = 3* fullHeight / 4;
            region24.packedTop = 3* fullHeight / 4;
            region24.projLeft = 0;
            region24.packedLeft = 0;
            region24.transform = 5;

            region24.projWidth = fullWidth / 3;
            region24.projHeight = fullHeight / 4;
            region24.packedWidth = fullWidth / 3;
            region24.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region24));

            Region region25;
            region25.projTop = 3*fullHeight / 4;
            region25.packedTop = 3*fullHeight / 4;
            region25.projLeft = fullWidth / 3;
            region25.packedLeft = fullWidth / 3;
            region25.transform = 0;

            region25.projWidth = fullWidth / 3;
            region25.projHeight = fullHeight / 4;
            region25.packedWidth = fullWidth / 3;
            region25.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region25));

            Region region26;
            region26.projTop = 3*fullHeight / 4;
            region26.packedTop = 3*fullHeight / 4;
            region26.projLeft = 2 * fullWidth / 3;
            region26.packedLeft = 2 * fullWidth / 3;
            region26.transform = 0;

            region26.projWidth = fullWidth / 3;
            region26.projHeight = fullHeight / 4;
            region26.packedWidth = fullWidth / 3;
            region26.packedHeight = fullHeight / 4;
            aMeta.regionPacking.get().regions.push_back(std::move(region26));
            */
        }
        else if (aVideoOutput == PipelineOutputVideo::SideBySide)
        {
            regionPacking.constituentPictMatching = true; 
            regionPacking.packedPictureWidth = fullWidth;
            regionPacking.packedPictureHeight = fullHeight;
            // Two regions per channel

            // First left
            Region region1;
            region1.projTop = 0;
            region1.packedTop = 0;
            region1.projLeft = 0;
            region1.packedLeft = 0;
            region1.transform = 0;

            region1.projWidth = fullWidth / 4;
            region1.projHeight = fullHeight / 2;
            region1.packedWidth = fullWidth / 4;
            region1.packedHeight = fullHeight / 2;

            Region region2;
            region2.projTop = fullHeight / 2;
            region2.packedTop = fullHeight / 2;
            region2.projLeft = 0;
            region2.packedLeft = 0;
            region2.transform = 0;

            region2.projWidth = fullWidth / 4;
            region2.projHeight = fullHeight / 2;
            region2.packedWidth = fullWidth / 4;
            region2.packedHeight = fullHeight / 2;
            /*
            // Then right
            Region region3;
            region3.projTop = 0;
            region3.packedTop = 0;
            region3.projLeft = fullWidth / 2;
            region3.packedLeft = fullWidth / 2;
            region3.transform = 0;

            region3.projWidth = fullWidth / 2;
            region3.projHeight = fullHeight / 2;
            region3.packedWidth = fullWidth / 2;
            region3.packedHeight = fullHeight / 2;

            Region region4;
            region4.projTop = fullHeight / 2;
            region4.packedTop = fullHeight / 2;
            region4.projLeft = fullWidth / 2;
            region4.packedLeft = fullWidth / 2;
            region4.transform = 0;

            region4.projWidth = fullWidth / 2;
            region4.projHeight = fullHeight / 2;
            region4.packedWidth = fullWidth / 2;
            region4.packedHeight = fullHeight / 2;
            */
            aMeta.regionPacking = regionPacking;
            aMeta.regionPacking.get().regions.push_back(std::move(region1));
            aMeta.regionPacking.get().regions.push_back(std::move(region2));
           /* aMeta.regionPacking.get().regions.push_back(std::move(region3));
            aMeta.regionPacking.get().regions.push_back(std::move(region4));*/

        }

    }
}  // namespace VDD
