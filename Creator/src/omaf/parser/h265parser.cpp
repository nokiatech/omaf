
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
#include "bitstream.hpp"
#include "h265parser.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>


using namespace std;
using namespace H265;

void H265Parser::getSliceHeaderList(vector <SliceHeader>& sliceHeaderList)
{
    sliceHeaderList = mSliceHeaderList;
}

void H265Parser::getSliceHeaderOffset(vector<sliceHeaderOffset*>& SliceHeaderOffsetList)
{
    SliceHeaderOffsetList = mSliceHeaderOffsetList;
}

std::list<SequenceParameterSet*> H265Parser::getSpsList()
{
    return mSpsList;

}

std::list<PictureParameterSet*> H265Parser::getPpsList()
{
    return mPpsList;
}

void H265Parser::writeSPS(Parser::BitStream& bitstr, SequenceParameterSet& sps, bool rbsp)
{

    bitstr.writeBits(sps.mVpsId, 4);
    bitstr.writeBits(sps.mSpsMaxSubLayersMinus1, 3);
    bitstr.writeBits(sps.mSpsTemporalIdNestingFlag, 1);

    writeProfileTierLevel(bitstr, sps.mProfileTierLevel, sps.mSpsMaxSubLayersMinus1);

    bitstr.writeExpGolombCode(sps.mSpsId);
    bitstr.writeExpGolombCode(sps.mChromaFormatIdc);
    if (sps.mChromaFormatIdc == 3)
    {
        bitstr.writeBits(sps.mSeparateColourPlaneFlag, 1);
    }
    bitstr.writeExpGolombCode(sps.mPicWidthInLumaSamples);
    bitstr.writeExpGolombCode(sps.mPicHeightInLumaSamples);
    bitstr.writeBits(sps.mConformanceWindowFlag, 1);
    if (sps.mConformanceWindowFlag)
    {
        bitstr.writeExpGolombCode(sps.mConfWinLeftOffset);
        bitstr.writeExpGolombCode(sps.mConfWinRightOffset);
        bitstr.writeExpGolombCode(sps.mConfWinTopOffset);
        bitstr.writeExpGolombCode(sps.mConfWinBottomOffset);
    }
    bitstr.writeExpGolombCode(sps.mBitDepthLumaMinus8);
    bitstr.writeExpGolombCode(sps.mBitDepthChromaMinus8);
    bitstr.writeExpGolombCode(sps.mLog2MaxPicOrderCntLsbMinus4);
    bitstr.writeBits(sps.mSpsSubLayerOrderingInfoPresentFlag, 1);
    for (unsigned int i = (sps.mSpsSubLayerOrderingInfoPresentFlag ? 0 : sps.mSpsMaxSubLayersMinus1); i <= sps.mSpsMaxSubLayersMinus1; ++i)
    {
        bitstr.writeExpGolombCode(sps.mSpsMaxDecPicBufferingMinus1.at(i));
        bitstr.writeExpGolombCode(sps.mSpsMaxNumReorderPics.at(i));
        bitstr.writeExpGolombCode(sps.mSpsMaxLatencyIncreasePlus1.at(i));
    }
    bitstr.writeExpGolombCode(sps.mLog2MinLumaCodingBlockSizeMinus3);
    bitstr.writeExpGolombCode(sps.mLog2DiffMaxMinLumaCodingBlockSize);
    bitstr.writeExpGolombCode(sps.mLog2MinTransformBlockSizeMinus2);
    bitstr.writeExpGolombCode(sps.mLog2DiffMaxMinTransformBlockSize);
    bitstr.writeExpGolombCode(sps.mMaxTransformHierarchyDepthInter);
    bitstr.writeExpGolombCode(sps.mMaxTransformHierarchyDepthIntra);
    bitstr.writeBits(sps.mScalingListEenabledFlag, 1);
    if (sps.mScalingListEenabledFlag)
    {
        bitstr.writeBits(sps.mSpsScalingListDataPresentFlag, 1);
        if (sps.mSpsScalingListDataPresentFlag)
        {
            writeScalingListData(bitstr, sps.mScalingListData);
        }
    }
    bitstr.writeBits(sps.mAmpEnabledFlag, 1);
    bitstr.writeBits(sps.mSampleAdaptiveOffsetEnabledFlag, 1);
    bitstr.writeBits(sps.mPcmEnabledFlag, 1);
    if (sps.mPcmEnabledFlag)
    {
        bitstr.writeBits(sps.mPcmSampleBitDepthLumaMinus1, 4);
        bitstr.writeBits(sps.mPcmSampleBitDepthChromaMinus1, 4);
        bitstr.writeExpGolombCode(sps.mLog2MinPcmLumaCodingBlockSizeMinus3);
        bitstr.writeExpGolombCode(sps.mLog2DiffMaxMinPcmLumaCodingBlockSize);
        bitstr.writeBits(sps.mPcmLoopFilterDisabledFlag, 1);
    }
    bitstr.writeExpGolombCode(sps.mNumShortTermRefPicSets);
    sps.mShortTermRefPicSets.resize(sps.mNumShortTermRefPicSets);
    sps.mShortTermRefPicSetsDerived.resize(sps.mNumShortTermRefPicSets);
    for (unsigned int i = 0; i < sps.mNumShortTermRefPicSets; ++i)
    {
        writeShortTermRefPicSet(bitstr, sps.mShortTermRefPicSetsDerived, sps.mShortTermRefPicSets[i], i, sps.mNumShortTermRefPicSets);
        //deriveRefPicSetParams(sps.mShortTermRefPicSetsDerived, sps.mShortTermRefPicSets[i], sps.mShortTermRefPicSetsDerived[i], i);
    }
    bitstr.writeBits(sps.mLongTermRefPicsPresentFlag, 1);
    if (sps.mLongTermRefPicsPresentFlag)
    {
        bitstr.writeExpGolombCode(sps.mNumLongTermRefPicsSps);
        for (unsigned int i = 0; i < sps.mNumLongTermRefPicsSps; ++i)
        {
            bitstr.writeBits(sps.mLtRefPicPocLsbSps.at(i), sps.mLog2MaxPicOrderCntLsbMinus4 + 4);
            bitstr.writeBits(sps.mUsedByCurrPicLtSpsFlag.at(i), 1);
        }
    }
    bitstr.writeBits(sps.mSpsTemporalMvpEnabledFlag, 1);
    bitstr.writeBits(sps.mStrongIntraSmoothingEnabledFlag, 1);
    setVuiDefaults(sps.mProfileTierLevel, sps.mVuiParameters);
    bitstr.writeBits(sps.mVuiParametersPresentFlag, 1);
    if (sps.mVuiParametersPresentFlag)
    {
        //TODO
        //writeVuiParameters(bitstr, sps.mVuiParameters); //TODO
    }
    bitstr.writeBits(sps.mSpsExtensionFlag, 1);
    writeRbspTrailingBits(bitstr); // TODO

    if (!rbsp)
    {
        insertEPB(bitstr.getStorage());
    }
}

int H265Parser::writeProfileTierLevel(Parser::BitStream& bitstr, ProfileTierLevel& ptl, const unsigned int maxNumSubLayersMinus1)
{
    bitstr.writeBits(ptl.mGeneralProfileSpace, 2);
    bitstr.writeBits(ptl.mGeneralTierFlag, 1);
    bitstr.writeBits(ptl.mGeneralProfileIdc, 5);

    for (int i = 0; i < 32; ++i)
    {

        bitstr.writeBits(ptl.mGeneralProfileCompatibilityFlag[i], 1);

    }
    bitstr.writeBits(ptl.mGeneralProgressiveSourceFlag, 1);
    bitstr.writeBits(ptl.mGeneralInterlacedSourceFlag, 1);
    bitstr.writeBits(ptl.mGeneralNonPackedConstraintFlag, 1);
    bitstr.writeBits(ptl.mGeneralFrameOnlyConstraintFlag, 1);
    for (int i = 0; i < 2; ++i)  // general_reserved_zero_44bits
    {
        bitstr.writeBits(0, 22);
    }
    bitstr.writeBits(ptl.mGeneralLevelIdc, 8);
    for (unsigned int i = 0; i < maxNumSubLayersMinus1; ++i)
    {
        bitstr.writeBits(ptl.mSubLayerProfilePresentFlag.at(i), 1);
        bitstr.writeBits(ptl.mSubLayerLevelPresentFlag.at(i), 1);
    }
    if (maxNumSubLayersMinus1 > 0)
    {
        for (int i = maxNumSubLayersMinus1; i < 8; ++i)
        {
            bitstr.writeBits(0, 2);  // reserved_zeor_2bits
        }
    }
    ptl.mSubLayerProfileTierLevels.resize(maxNumSubLayersMinus1);
    for (unsigned int i = 0; i < maxNumSubLayersMinus1; ++i)
    {
        ptl.mSubLayerProfileTierLevels[i].mSubLayerTierFlag = 0;
        if (ptl.mSubLayerProfilePresentFlag[i])
        {
            bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerProfileSpace, 2);
            bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerTierFlag, 1);
            bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerProfileIdc, 5);
            for (int j = 0; j < 32; ++j)
            {
                bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerProfileCompatibilityFlag[j], 1);
            }
            bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerProgressiveSourceFlag, 1);
            bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerInterlacedSourceFlag, 1);
            bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerNonPackedConstraintFlag, 1);
            bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerFrameOnlyConstraintFlag, 1);
            for (int j = 0; j < 2; ++j)  // sub_layer_reserved_zero_44bits
            {
                bitstr.readBits(22);
            }
        }
        if (ptl.mSubLayerLevelPresentFlag[i])
        {
            bitstr.writeBits(ptl.mSubLayerProfileTierLevels[i].mSubLayerLevelIdc, 8);
        }
    }

    return 0;

}
int H265Parser::writeScalingListData(Parser::BitStream& bitstr, ScalingListData& scalingList)
{
    for (int sizeId = 0; sizeId < 4; sizeId++)
    {
        int numMatrix = (sizeId == 3) ? 2 : 6;
        scalingList.mScalingListPredModeFlag[sizeId].resize(numMatrix);
        scalingList.mScalingListPredMatrixIdDelta[sizeId].resize(numMatrix);
        if (sizeId > 1)
        {
            scalingList.mScalingListDcCoefMinus8[sizeId - 2].resize(numMatrix);
        }
        scalingList.mScalingList[sizeId].resize(numMatrix);

        for (int matrixId = 0; matrixId < numMatrix; matrixId++)
        {
            bitstr.writeBits(scalingList.mScalingListPredModeFlag[sizeId][matrixId], 1);
            if (!scalingList.mScalingListPredModeFlag[sizeId][matrixId])
            {
                bitstr.writeExpGolombCode(scalingList.mScalingListPredMatrixIdDelta[sizeId][matrixId]);
            }
            else
            {
                int nextCoef = 8;
                int coefNum = std::min(64, 1 << (4 + (sizeId << 1)));
                if (sizeId > 1)
                {
                    bitstr.writeExpGolombCode(scalingList.mScalingListDcCoefMinus8[sizeId - 2][matrixId]);
                    nextCoef = scalingList.mScalingListDcCoefMinus8[sizeId - 2][matrixId] + 8;
                }
                for (int i = 0; i < coefNum; ++i)
                {
                    int scalingListDeltaCoef = bitstr.readSignedExpGolombCode(); // TODO
                    nextCoef = (nextCoef + scalingListDeltaCoef + 256) % 256; // TODO
                    scalingList.mScalingList[sizeId][matrixId].push_back(nextCoef); // TODO
                }
            }
        }
    }

    return 0;
}

int H265Parser::writeShortTermRefPicSet(Parser::BitStream& bitstr, const vector<ShortTermRefPicSetDerived>& rpsList, ShortTermRefPicSet& rps, const unsigned int stRpsIdx, const unsigned int numShortTermRefPicSets)
{

    //rps.mInterRefPicSetPredictionFlag = 0;
    if (stRpsIdx != 0)
    {
        bitstr.writeBits(rps.mInterRefPicSetPredictionFlag, 1);
    }
    if (rps.mInterRefPicSetPredictionFlag)
    {
        if (stRpsIdx == numShortTermRefPicSets)
        {
            bitstr.writeExpGolombCode(rps.mDeltaIdxMinus1);
        }
        bitstr.writeBits(rps.mDeltaRpsSign, 1);
        bitstr.writeExpGolombCode(rps.mAbsDeltaRpsMinus1);
        const unsigned int refRpsIdx = stRpsIdx - (rps.mDeltaIdxMinus1 + 1);
        for (unsigned int j = 0; j <= rpsList[refRpsIdx].mNumDeltaPocs; ++j)
        {
            bitstr.writeBits(rps.mUsedByCurrPicFlag.at(j), 1);
            if (!rps.mUsedByCurrPicFlag[j])
            {
                bitstr.writeBits(rps.mUseDeltaFlag.at(j), 1);
            }
            else
            {
                rps.mUseDeltaFlag.push_back(1);
            }
        }
    }
    else
    {
        bitstr.writeExpGolombCode(rps.mNumNegativePics);
        bitstr.writeExpGolombCode(rps.mNumPositivePics);
        for (unsigned int i = 0; i < rps.mNumNegativePics; ++i)
        {
            bitstr.writeExpGolombCode(rps.mDeltaPocS0Minus1.at(i));
            bitstr.writeBits(rps.mUsedByCurrPicS0Flag.at(i), 1);
        }
        for (unsigned int i = 0; i < rps.mNumPositivePics; ++i)
        {
            bitstr.writeExpGolombCode(rps.mDeltaPocS1Minus1.at(i));
            bitstr.writeBits(rps.mUsedByCurrPicS1Flag.at(i), 1);
        }
    }

    return 0;
}
void H265Parser::writeRbspTrailingBits(Parser::BitStream& bitstr)
{
    bitstr.writeBits(1, 1);
    while ((8 - bitstr.getBitOffset()) & 0x7)
    {
        bitstr.writeBits(0, 1);
    }
}

void H265Parser::writePPS(Parser::BitStream& bitstr, PictureParameterSet& pps, bool rbsp)
{
    bitstr.writeExpGolombCode(pps.mPpsId);
    bitstr.writeExpGolombCode(pps.mSpsId);
    bitstr.writeBits(pps.mDependentSliceSegmentsEnabledFlag, 1);
    bitstr.writeBits(pps.mOutputFlagPresentFlag, 1);
    bitstr.writeBits(pps.mNumExtraSliceHeaderBits, 3);
    bitstr.writeBits(pps.mSignDataHidingEnabledFlag, 1);
    bitstr.writeBits(pps.mCabacInitPresentFlag, 1);
    bitstr.writeExpGolombCode(pps.mNumRefIdxL0DefaultActiveMinus1);
    bitstr.writeExpGolombCode(pps.mNumRefIdxL1DefaultActiveMinus1);
    bitstr.writeSignedExpGolombCode(pps.mInitQpMinus26);
    bitstr.writeBits(pps.mConstrainedIntraPredFlag, 1);
    bitstr.writeBits(pps.mTransformSkipEnabledFlag, 1);
    bitstr.writeBits(pps.mCuQpDeltaEnabledFlag, 1);
    if (pps.mCuQpDeltaEnabledFlag)
    {
        bitstr.writeExpGolombCode(pps.mDiffCuQpDeltaDepth);
    }
    bitstr.writeSignedExpGolombCode(pps.mPpsCbQpOffset);
    bitstr.writeSignedExpGolombCode(pps.mPpsCrQpOffset);
    bitstr.writeBits(pps.mPpsSliceChromaQpOffsetsPresentFlag, 1);
    bitstr.writeBits(pps.mWeightedPredFlag, 1);
    bitstr.writeBits(pps.mWeightedBipredFlag, 1);
    bitstr.writeBits(pps.mTransquantBypassEnabledFlag, 1);
    bitstr.writeBits(pps.mTilesEnabledFlag, 1);
    bitstr.writeBits(pps.mEntropyCodingSyncEnabledFlag, 1);
    if (pps.mTilesEnabledFlag)
    {
        bitstr.writeExpGolombCode(pps.mNumTileColumnsMinus1);
        bitstr.writeExpGolombCode(pps.mNumTileRowsMinus1);
        bitstr.writeBits(pps.mUniformSpacingFlag, 1);
        if (!pps.mUniformSpacingFlag)
        {
            for (unsigned int i = 0; i < pps.mNumTileColumnsMinus1; ++i)
            {
                bitstr.writeExpGolombCode(pps.mColumnWidthMinus1.at(i));
            }
            for (unsigned int i = 0; i < pps.mNumTileRowsMinus1; ++i)
            {
                bitstr.writeExpGolombCode(pps.mRowHeightMinus1.at(i));
            }
        }
        bitstr.writeBits(pps.mLoopFilterAcrossTilesEnabledFlag, 1);
    }
    bitstr.writeBits(pps.mPpsLoopFilterAcrossSicesEnabledFlag, 1);
    bitstr.writeBits(pps.mDeblockingFilterControlPresentFlag, 1);
    if (pps.mDeblockingFilterControlPresentFlag)
    {
        bitstr.writeBits(pps.mDeblockingFilterOverrideEnabledFlag, 1);
        bitstr.writeBits(pps.mPpsDeblockingFilterDisabledFlag, 1);
        if (!pps.mPpsDeblockingFilterDisabledFlag)
        {
            bitstr.writeSignedExpGolombCode(pps.mPpsBetaOffsetDiv2);
            bitstr.writeSignedExpGolombCode(pps.mPpsTcOffsetDiv2);
        }
    }
    bitstr.writeBits(pps.mPpsScalingListDataPresentFlag, 1);
    if (pps.mPpsScalingListDataPresentFlag)
    {
        writeScalingListData(bitstr, pps.mScalingListData);
    }
    bitstr.writeBits(pps.mListsModificationPresentFlag, 1);
    bitstr.writeExpGolombCode(pps.mLog2ParallelMergeLevelMinus2);
    bitstr.writeBits(pps.mSliceSegmentHeaderExtensionPresentFlag, 1);
    bitstr.writeBits(pps.mPpsExtensionFlag, 1);
    writeRbspTrailingBits(bitstr);

    if (!rbsp)
    {
        insertEPB(bitstr.getStorage());
    }
}

int H265Parser::writeSliceHeader(Parser::BitStream& bitstr, SliceHeader& slice,
    H265NalUnitType naluType, const SequenceParameterSet& sps, const PictureParameterSet& pps)
{
    bitstr.writeBits(slice.mFirstSliceSegmentInPicFlag, 1);
    if (naluType >= H265NalUnitType::CODED_SLICE_BLA_W_LP && naluType <= H265NalUnitType::RESERVED_IRAP_VCL23)
    {
        bitstr.writeBits(slice.mNoOutputOfPriorPicsFlag, 1);
    }
    bitstr.writeExpGolombCode(slice.mPpsId);
    if (!slice.mFirstSliceSegmentInPicFlag)
    {
        if (pps.mDependentSliceSegmentsEnabledFlag)
        {
            bitstr.writeBits(slice.mDependentSliceSegmentFlag, 1);
        }
        unsigned int minCbLog2SizeY = sps.mLog2MinLumaCodingBlockSizeMinus3 + 3;
        unsigned int ctbLog2SizeY = minCbLog2SizeY + sps.mLog2DiffMaxMinLumaCodingBlockSize;
        unsigned int ctbSizeY = 1 << ctbLog2SizeY;
        unsigned int picWidthInCtbsY = (sps.mPicWidthInLumaSamples + ctbSizeY - 1) / ctbSizeY;
        unsigned int picHeightInCtbsY = (sps.mPicHeightInLumaSamples + ctbSizeY - 1) / ctbSizeY;
        unsigned int picSizeInCtbsY = picWidthInCtbsY * picHeightInCtbsY;
        bitstr.writeBits(slice.mSliceSegmentAddress, ceilLog2(picSizeInCtbsY));
    }
    if (!slice.mDependentSliceSegmentFlag)
    {
        for (unsigned int i = 0; i < pps.mNumExtraSliceHeaderBits; ++i)
        {
            bitstr.writeBits(0, 1);  // slice_reserved_flag[i]
        }
        bitstr.writeExpGolombCode((unsigned int)slice.mSliceType);
        if (pps.mOutputFlagPresentFlag)
        {
            bitstr.writeBits(slice.mPicOutputFlag, 1);
        }
        if (sps.mSeparateColourPlaneFlag)
        {
            bitstr.writeBits(slice.mColourPlaneId, 2);
        }
        if (naluType != H265NalUnitType::CODED_SLICE_IDR_W_RADL && naluType != H265NalUnitType::CODED_SLICE_IDR_N_LP)
        {
            bitstr.writeBits(slice.mSlicePicOrderCntLsb, sps.mLog2MaxPicOrderCntLsbMinus4 + 4);
            bitstr.writeBits(slice.mShortTermRefPicSetSpsFlag, 1);
            //slice.mShortTermRefPicSetIdx = 0;
            if (!slice.mShortTermRefPicSetSpsFlag)
            {
                writeShortTermRefPicSet(bitstr, sps.mShortTermRefPicSetsDerived, slice.mShortTermRefPicSet, sps.mNumShortTermRefPicSets, sps.mNumShortTermRefPicSets);
            }
            else if (sps.mNumShortTermRefPicSets > 1)
            {
                bitstr.writeBits(slice.mShortTermRefPicSetIdx, ceilLog2(sps.mNumShortTermRefPicSets));
            }
            if (sps.mLongTermRefPicsPresentFlag)
            {
                if (sps.mNumLongTermRefPicsSps > 0)
                {
                    bitstr.writeExpGolombCode(slice.mNumLongTermSps);
                }
                bitstr.writeExpGolombCode(slice.mNumLongTermPics);

                for (unsigned i = 0; i < slice.mNumLongTermSps + slice.mNumLongTermPics; ++i)
                {
                    if (i < slice.mNumLongTermSps)
                    {
                        if (sps.mNumLongTermRefPicsSps > 1)
                        {
                            bitstr.writeBits(slice.mLtIdxSps[i], ceilLog2(sps.mNumLongTermRefPicsSps));
                        }
                    }
                    else
                    {
                        bitstr.writeBits(slice.mPocLsbLtSyntax[i], sps.mLog2MaxPicOrderCntLsbMinus4 + 4);
                        bitstr.writeBits(slice.mUsedByCurrPicLtFlag[i], 1);
                    }
                    bitstr.writeBits(slice.mDeltaPocMsbPresentFlag[i], 1);
                    if (slice.mDeltaPocMsbPresentFlag[i])
                    {
                        bitstr.writeExpGolombCode(slice.mDeltaPocMsbCycleLt[i]);
                    }
                }
            }
            if (sps.mSpsTemporalMvpEnabledFlag)
            {
                bitstr.writeBits(slice.mSliceTemporalMvpEnabledFlag, 1);
            }
        }
        if (sps.mSampleAdaptiveOffsetEnabledFlag)
        {
            bitstr.writeBits(slice.mSliceSaoLumaFlag, 1);
            bitstr.writeBits(slice.mSliceSaoChromaFlag, 1);
        }

        if (slice.mSliceType == SliceType::P || slice.mSliceType == SliceType::B)
        {
            const unsigned int currStRpsIdx = slice.mShortTermRefPicSetSpsFlag ? slice.mShortTermRefPicSetIdx : sps.mNumShortTermRefPicSets;
            const ShortTermRefPicSetDerived* currStRps = (currStRpsIdx == sps.mNumShortTermRefPicSets) ? &slice.mShortTermRefPicSetDerived : &sps.mShortTermRefPicSetsDerived[currStRpsIdx];
            slice.mNumPocTotalCurr = 0;
            for (unsigned int i = 0; i < currStRps->mNumNegativePics; ++i)
            {
                if (currStRps->mUsedByCurrPicS0[i])
                {
                    slice.mNumPocTotalCurr++;
                }
            }

            for (unsigned int i = 0; i < currStRps->mNumPositivePics; ++i)
            {
                if (currStRps->mUsedByCurrPicS1[i])
                {
                    slice.mNumPocTotalCurr++;
                }
            }
            for (unsigned int i = 0; i < slice.mNumLongTermSps + slice.mNumLongTermPics; ++i)
            {
                if (slice.mUsedByCurrPicLt[i])
                {
                    slice.mNumPocTotalCurr++;
                }
            }

            bitstr.writeBits(slice.mNumRefIdxActiveOverrideFlag, 1);
            if (slice.mNumRefIdxActiveOverrideFlag)
            {
                bitstr.writeExpGolombCode(slice.mNumRefIdxL0ActiveMinus1);
                if (slice.mSliceType == SliceType::B)
                {
                    bitstr.writeExpGolombCode(slice.mNumRefIdxL1ActiveMinus1);
                }
            }
            if (pps.mListsModificationPresentFlag && slice.mNumPocTotalCurr > 1)
            {
                parseRefPicListsModification(bitstr, slice, slice.mRefPicListsModification); // TODO 
            }
            if (slice.mSliceType == SliceType::B)
            {
                bitstr.writeBits(slice.mMvdL1ZeroFlag, 1);
            }
            if (pps.mCabacInitPresentFlag)
            {
                bitstr.writeBits(slice.mCabacInitFlag, 1);
            }
            if (slice.mSliceTemporalMvpEnabledFlag)
            {
                if (slice.mSliceType == SliceType::B)
                {
                    bitstr.writeBits(slice.mCollocatedFromL0Flag, 1);
                }
                if ((slice.mCollocatedFromL0Flag && slice.mNumRefIdxL0ActiveMinus1 > 0) ||
                    (!slice.mCollocatedFromL0Flag && slice.mNumRefIdxL1ActiveMinus1 > 0))
                {
                    bitstr.writeExpGolombCode(slice.mCollocatedRefIdx);
                }
            }
            if ((pps.mWeightedPredFlag && slice.mSliceType == SliceType::P) || (pps.mWeightedBipredFlag && slice.mSliceType == SliceType::B))
            {
                //parsePredWeightTable(bitstr, sps, slice, slice.mPredWeightTable); // TODO
            }
            bitstr.writeExpGolombCode(slice.mFiveMinusMaxNumMergeCand);
        }
        //std::cout << "writeSignedExpGolombCode" << std::endl;
        bitstr.writeSignedExpGolombCode(slice.mSliceQpDelta);
        if (pps.mPpsSliceChromaQpOffsetsPresentFlag)
        {
            bitstr.writeSignedExpGolombCode(slice.mSliceCbQpOffset);
            bitstr.writeSignedExpGolombCode(slice.mSliceCrQpOffset);
        }
        if (pps.mDeblockingFilterOverrideEnabledFlag)
        {
            bitstr.writeBits(slice.mDeblockingFilterOverrideFlag, 1);
        }
        if (slice.mDeblockingFilterOverrideFlag)
        {
            bitstr.writeBits(slice.mSliceDeblockingFilterDisabledFlag, 1);
            if (!slice.mSliceDeblockingFilterDisabledFlag)
            {
                bitstr.writeSignedExpGolombCode(slice.mSliceBetaOffsetDiv2);
                bitstr.writeSignedExpGolombCode(slice.mSliceTcOffsetDiv2);
            }
        }
        if (pps.mPpsLoopFilterAcrossSicesEnabledFlag && (slice.mSliceSaoLumaFlag || slice.mSliceSaoChromaFlag || !slice.mSliceDeblockingFilterDisabledFlag))
        {
            bitstr.writeBits(slice.mSliceLoopFilterAcrossSlicesEnabledFlag, 1);
            //bitstr.writeBits(0, 1);
        }
    }  // dependent slice segment
    if (pps.mTilesEnabledFlag || pps.mEntropyCodingSyncEnabledFlag)
    {   //std::cout << "writeExpGolombCode" << std::endl;
        bitstr.writeExpGolombCode(slice.mNumEntryPointOffsets);
        if (slice.mNumEntryPointOffsets > 0)
        {   //std::cout << "num entry point offset " << uint32_t(slice.mNumEntryPointOffsets) << std::endl;
            bitstr.writeExpGolombCode(slice.mOffsetLenMinus1);
            for (unsigned int i = 0; (i < slice.mNumEntryPointOffsets) && (slice.mEntryPointOffsetMinus1.size() > i); ++i)
            {   //std::cout << "mEntryPointOffsetMinus1" << std::endl;
                bitstr.writeBits(slice.mEntryPointOffsetMinus1.at(i), slice.mOffsetLenMinus1 + 1);
            }
        }
    }
    //std::cout << "writeRbspTrailingBits" << std::endl;
    writeRbspTrailingBits(bitstr);
//in theory we should call EPB (with offset set to skip the start code), but in practice slice header cannot have emulation
//    insertEPB(bitstr.getStorage());

    return 0;
    //if (pps.mSliceSegmentHeaderExtensionPresentFlag) // TODO
    //{
    //  unsigned int len = bitstr.readExpGolombCode();
    //  for (unsigned int i = 0; i < len; ++i)
    //  {
    //      bitstr.writeBits(0, 8);
    //  }
    //}
}

int H265Parser::writePredWeightTable(Parser::BitStream& bitstr, SequenceParameterSet& sps, SliceHeader& slice, PredWeightTable& pwTable)
{
    array<int, 2> weights;
    array<int, 2> offsets;

    bitstr.writeExpGolombCode(pwTable.mLumaLog2WeightDenom);
    if (sps.mChromaFormatIdc != 0)
    {
        bitstr.writeSignedExpGolombCode(pwTable.mDeltaChromaLog2WeightDenom);
    }
    for (unsigned int i = 0; i <= slice.mNumRefIdxL0ActiveMinus1; ++i)
    {
        bitstr.writeBits(pwTable.mLumaWeightL0Flag.at(i), 1);
    }
    for (unsigned int i = 0; i <= slice.mNumRefIdxL0ActiveMinus1; ++i)
    {
        // Vinod changed this from Ali's version (compiler threw an error)
        // (sps.mChromaFormatIdc != 0) ? bitstr.writeBits(pwTable.mChromaWeightL0Flag.at(i), 1) : 0;
        if (sps.mChromaFormatIdc != 0) {
            bitstr.writeBits(pwTable.mChromaWeightL0Flag.at(i), 1);
        }
    }
    pwTable.mDeltaChromaWeightL0.resize(slice.mNumRefIdxL0ActiveMinus1 + 1);
    pwTable.mDeltaChromaOffsetL0.resize(slice.mNumRefIdxL0ActiveMinus1 + 1);
    for (unsigned int i = 0; i <= slice.mNumRefIdxL0ActiveMinus1; ++i)
    {
        if (pwTable.mLumaWeightL0Flag[i])
        {
            bitstr.writeSignedExpGolombCode(pwTable.mDeltaLumaWeightL0.at(i));
            bitstr.writeSignedExpGolombCode(pwTable.mLumaOffsetL0.at(i));
        }
        //else
        //{
        //  pwTable.mDeltaLumaWeightL0.push_back(1 << pwTable.mLumaLog2WeightDenom);
        //  pwTable.mLumaOffsetL0.push_back(0);
        //}
        if (pwTable.mChromaWeightL0Flag[i])
        {
            for (int j = 0; j < 2; ++j)
            {
                bitstr.writeSignedExpGolombCode(weights[j]);
                bitstr.writeSignedExpGolombCode(offsets[j]);
            }
        }
        //else
        //{
        //  for (int j = 0; j < 2; ++j)
        //  {
        //      weights[j] = 1 << pwTable.mDeltaChromaLog2WeightDenom;
        //      offsets[j] = 0;
        //  }
        //}
        //pwTable.mDeltaChromaWeightL0.push_back(weights);
        //pwTable.mDeltaChromaOffsetL0.push_back(offsets);
    }

    if (slice.mSliceType == SliceType::B)
    {
        for (unsigned int i = 0; i <= slice.mNumRefIdxL1ActiveMinus1; ++i)
        {
            pwTable.mLumaWeightL1Flag.push_back(bitstr.readBits(1));
        }
        for (unsigned int i = 0; i <= slice.mNumRefIdxL1ActiveMinus1; ++i)
        {
            // Vinod changed this from Ali's version (compiler threw an error)
            // (sps.mChromaFormatIdc != 0) ? bitstr.writeBits(pwTable.mChromaWeightL1Flag.at(i), 1) : 0;
            if (sps.mChromaFormatIdc != 0) {
                bitstr.writeBits(pwTable.mChromaWeightL1Flag.at(i), 1);
            }
        }
        for (unsigned int i = 0; i <= slice.mNumRefIdxL1ActiveMinus1; ++i)
        {
            if (pwTable.mLumaWeightL1Flag[i])
            {
                bitstr.writeSignedExpGolombCode(pwTable.mDeltaLumaWeightL1.at(i));
                bitstr.writeSignedExpGolombCode(pwTable.mLumaOffsetL1.at(i));
            }
            //else
            //{
            //  pwTable.mDeltaLumaWeightL1.push_back(1 << pwTable.mLumaLog2WeightDenom);
            //  pwTable.mLumaOffsetL1.push_back(0);
            //}
            if (pwTable.mChromaWeightL1Flag[i])
            {
                for (int j = 0; j < 2; ++j)
                {
                    weights[j] = bitstr.readSignedExpGolombCode();
                    offsets[j] = bitstr.readSignedExpGolombCode();
                }
            }
            else
            {
                for (int j = 0; j < 2; ++j)
                {
                    weights[j] = 1 << pwTable.mDeltaChromaLog2WeightDenom;
                    offsets[j] = 0;
                }
            }
            pwTable.mDeltaChromaWeightL1.push_back(weights);
            pwTable.mDeltaChromaOffsetL1.push_back(offsets);
        }
    }

    return 0;
}

H265Parser::H265Parser() :
    mPicIndex(0),
    mPrevPicOrderCntLsb(0),
    mPrevPicOrderCntMsb(0)
{
}

H265Parser::~H265Parser()
{
    // delete sps'es
    for (auto& i : mSpsList)
    {
        delete i;
    }

    // delete pps'es
    for (auto& i : mPpsList)
    {
        delete i;
    }
}


bool H265Parser::parseParamSet(H265InputStream& aInputStream, ParserInterface::AccessUnit& accessUnit)
{
    accessUnit = {};  // Make it empty
    std::vector<uint8_t> currNalUnitData;
    // Parse VPS/SPS/PPS NAL unit - note mp4reader outputs param sets as bytestream mode always
    while (!aInputStream.eof())
    {
        readNextNalUnit(aInputStream, currNalUnitData);
        NalUnitHeader naluHeader;
        parseNalUnit(currNalUnitData, naluHeader);

        // store NAL unit to current AU
        switch (naluHeader.mH265NalUnitType)
        {
        case H265NalUnitType::VPS:
            accessUnit.mVpsNalUnits.push_back(currNalUnitData);
            break;
        case H265NalUnitType::SPS:
            accessUnit.mSpsNalUnits.push_back(currNalUnitData);
            break;
        case H265NalUnitType::PPS:
            accessUnit.mPpsNalUnits.push_back(currNalUnitData);
            break;
        default:
        {
            // else ignore, can be e.g. H265NalUnitType::SUFFIX_SEI - TODO is there something useful that we'd need? But we are splitting the bitstream, so e.g. hash would not be valid any more
            break;
        }
        }
    }
    return true;
}

bool H265Parser::parseNextAU(H265InputStream& aInputStream, ParserInterface::AccessUnit& accessUnit)
{
    bool isFirstNaluInAU = true;
    bool firstVclNaluFound = false;

    mSliceHeaderOffsetList.clear();

    NalUnitHeader naluHeader;
    int poc = 0;

    // Clear structures
    accessUnit = {};  // Make it empty

    // Parse all NAL units from the input (assuming input contains one video frame from MP4 == one access unit)
    while (!aInputStream.eof())
    {
        std::vector<uint8_t> currNalUnitData;
        readNextNalUnit(aInputStream, currNalUnitData);
        parseNalUnit(currNalUnitData, naluHeader);

        // store NAL unit to current AU
        switch (naluHeader.mH265NalUnitType)
        {
        case H265NalUnitType::VPS:
            accessUnit.mVpsNalUnits.push_back(currNalUnitData);
            break;
        case H265NalUnitType::SPS:
            accessUnit.mSpsNalUnits.push_back(currNalUnitData);
            break;
        case H265NalUnitType::PPS:
            accessUnit.mPpsNalUnits.push_back(currNalUnitData);
            break;
        default:
        {
            if (naluHeader.mH265NalUnitType <= H265NalUnitType::RESERVED_VCL31)
            {
                // is VCL NAL, add
                accessUnit.mNalUnits.push_back(currNalUnitData);
            }
            // else ignore, can be e.g. H265NalUnitType::SUFFIX_SEI - TODO is there something useful that we'd need? But we are splitting the bitstream, so e.g. hash would not be valid any more
            break;
        }
        }

        // initialize picture if the NAL unit is the first slice of the picture
        if (isFirstVclNaluInPic(currNalUnitData))
        {
            RefPicSet rps = {};
            Picture pic = {};

            SliceHeader& sliceHeader = *mSliceList[0];
            firstVclNaluFound = true;
            //picNaluHeader = naluHeader;

            poc = decodePoc(sliceHeader, naluHeader);

            mRefPicList0.clear();
            mRefPicList1.clear();

            // init reference picure set for non-IDR pictures
            if (naluHeader.mH265NalUnitType != H265NalUnitType::CODED_SLICE_IDR_W_RADL && naluHeader.mH265NalUnitType != H265NalUnitType::CODED_SLICE_IDR_N_LP)
            {
                decodeRefPicSet(sliceHeader, rps, poc);
                if (sliceHeader.mSliceType == SliceType::P || sliceHeader.mSliceType == SliceType::B)
                {
                    generateRefPicLists(sliceHeader, rps);
                }
            }

            // Initialize new picture
            pic.mH265NalUnitType = naluHeader.mH265NalUnitType;
            pic.mIsReferecePic = true;
            pic.mIsLongTermRefPic = false;
            pic.mIndex = mPicIndex;
            pic.mPoc = poc;
            pic.mWidth = sliceHeader.mSps->mPicWidthInLumaSamples;
            pic.mHeight = sliceHeader.mSps->mPicHeightInLumaSamples;
            pic.mSlicePicOrderCntLsb = sliceHeader.mSlicePicOrderCntLsb;
            pic.mPicOutputFlag = (sliceHeader.mPicOutputFlag != 0) ? true : false;

            // Add picture to Decoded Picture Buffer
            mDecodedPicBuffer.push_back(pic);

            // Set Access Unit params
            accessUnit.mPicIndex = pic.mIndex;
            accessUnit.mPoc = pic.mPoc;
            accessUnit.mPicWidth = pic.mWidth;
            accessUnit.mPicHeight = pic.mHeight;
            accessUnit.mIsIdr = pic.mH265NalUnitType == H265NalUnitType::CODED_SLICE_IDR_N_LP || pic.mH265NalUnitType == H265NalUnitType::CODED_SLICE_IDR_W_RADL;
            accessUnit.mIsCra = pic.mH265NalUnitType == H265NalUnitType::CODED_SLICE_CRA;
            accessUnit.mIsBla = pic.mH265NalUnitType == H265NalUnitType::CODED_SLICE_BLA_N_LP || pic.mH265NalUnitType == H265NalUnitType::CODED_SLICE_BLA_W_LP || pic.mH265NalUnitType == H265NalUnitType::CODED_SLICE_BLA_W_RADL;
            accessUnit.mPicOutputFlag = pic.mPicOutputFlag;
            getRefPicIndices(accessUnit.mRefPicIndices, mRefPicList0, mRefPicList1);
        }
        currNalUnitData.clear();
        isFirstNaluInAU = false;
    }

    accessUnit.mIsIntra = true;
    for (const auto i : mSliceList)
    {
        if (i->mSliceType != SliceType::I)
        {
            accessUnit.mIsIntra = false;
            break;
        }
    }

    // Delete slices
    for (auto i : mSliceList)
    {
        delete i;
    }
    mSliceList.clear();

    if (accessUnit.mNalUnits.size() > 0)
    {
        mPicIndex++;
        // printPicStats(pic);
        return true;
    }
    else
    {
        return false;
    }
}

void H265Parser::printPicStats(const Picture& /*pic*/)
{
    // cout << "POC: " << setw(3) << pic.mPoc;
    // cout << "  List0: ";
    for (auto i = mRefPicList0.begin(); i != mRefPicList0.end(); ++i)
    {
        // cout << setw(4) << (*i)->mPoc;
    }
    // cout << "    List1: ";
    for (auto i = mRefPicList1.begin(); i != mRefPicList1.end(); ++i)
    {
        // cout << setw(4) << (*i)->mPoc;
    }
    // cout << endl;
}

void H265Parser::getRefPicIndices(vector<unsigned int>& refPicIndices, const vector<Picture*>& refPicList0, const vector<Picture*>& refPicList1)
{
    refPicIndices.clear();

    // list0
    for (const auto i : refPicList0)
    {
        if (isUniquePicIndex(refPicIndices, i->mIndex))
        {
            refPicIndices.push_back(i->mIndex);
        }
    }

    // list1
    for (const auto i : refPicList1)
    {
        if (isUniquePicIndex(refPicIndices, i->mIndex))
        {
            refPicIndices.push_back(i->mIndex);
        }
    }
}

bool H265Parser::isUniquePicIndex(const vector<unsigned int>& refPicIndices, const unsigned int picIndex)
{
    bool isUniqueIndex = true;

    for (const auto i : refPicIndices)
    {
        if (i == picIndex)
        {
            isUniqueIndex = false;
            break;
        }
    }

    return isUniqueIndex;
}


int H265Parser::parseNalUnit(const vector<uint8_t>& nalUnit, NalUnitHeader& naluHeader)
{
    vector<uint8_t> nalUnitRBSP;

    convertToRBSP(nalUnit, nalUnitRBSP);
    Parser::BitStream bitstr(nalUnitRBSP); 
    parseNalUnitHeader(bitstr, naluHeader);

    // Only SPS, PPS and slice header are parsed
    if (naluHeader.mH265NalUnitType == H265NalUnitType::SPS)
    {
        SequenceParameterSet* sps = new SequenceParameterSet;
        parseSPS(bitstr, *sps);
        removeSps(sps->mSpsId);
        mSpsList.push_back(sps);
    }
    else if (naluHeader.mH265NalUnitType == H265NalUnitType::PPS)
    {
        PictureParameterSet* pps = new PictureParameterSet;
        parsePPS(bitstr, *pps);
        removePps(pps->mPpsId);
        mPpsList.push_back(pps);
    }
    else if (isVclNaluType(naluHeader.mH265NalUnitType))
    {
        SliceHeader* sliceHeader = new SliceHeader;
        sliceHeaderOffset* offset = new sliceHeaderOffset;
        parseSliceHeader(bitstr, *sliceHeader, naluHeader.mH265NalUnitType, mSpsList, mPpsList);
        offset->byteOffset = bitstr.getByteOffset();
        offset->bitOffset = bitstr.getBitOffset();
        mSliceHeaderOffsetList.push_back(offset);
        mSliceList.push_back(sliceHeader);
        mSliceHeaderList.push_back(*sliceHeader);
    }

    return 0;
}

SequenceParameterSet* H265Parser::findSps(const unsigned int spsId, const std::list<SequenceParameterSet*>& aSpsList)
{
    SequenceParameterSet* sps = nullptr;

    for (auto& i : aSpsList)
    {
        if (i->mSpsId == spsId)
        {
            sps = i;
            break;
        }
    }

    return sps;
}

PictureParameterSet* H265Parser::findPps(unsigned int ppsId, const std::list<PictureParameterSet*>& aPpsList)
{
    PictureParameterSet* pps = nullptr;

    for (auto& i : aPpsList)
    {
        if (i->mPpsId == ppsId)
        {
            pps = i;
            break;
        }
    }

    return pps;
}

void H265Parser::removeSps(const unsigned int spsId)
{
    auto i = mSpsList.begin();

    for (; i != mSpsList.end(); ++i)
    {
        if ((*i)->mSpsId == spsId)
        {
            delete (*i);
            mSpsList.erase(i);
            break;
        }
    }
}

bool H265Parser::isVclNaluType(const H265NalUnitType naluType)
{
    return (naluType >= H265NalUnitType::CODED_SLICE_TRAIL_N && naluType <= H265NalUnitType::RESERVED_VCL31);
}

void H265Parser::removePps(const unsigned int ppsId)
{
    auto i = mPpsList.begin();

    for (; i != mPpsList.end(); ++i)
    {
        if ((*i)->mPpsId == ppsId)
        {
            delete (*i);
            mPpsList.erase(i);
            break;
        }
    }
}

int H265Parser::decodePoc(const SliceHeader& slice, NalUnitHeader& naluHeader)
{
    int picOrderCntMsb;
    int picOrderCntVal;
    int maxPicOrderCnt = 1 << (slice.mSps->mLog2MaxPicOrderCntLsbMinus4 + 4);

    if ((slice.mSlicePicOrderCntLsb < mPrevPicOrderCntLsb) && ((int(mPrevPicOrderCntLsb) - int(slice.mSlicePicOrderCntLsb)) >= (maxPicOrderCnt / 2)))
    {
        picOrderCntMsb = mPrevPicOrderCntMsb + maxPicOrderCnt;
    }
    else if ((slice.mSlicePicOrderCntLsb > mPrevPicOrderCntLsb) && ((int(slice.mSlicePicOrderCntLsb) - int(mPrevPicOrderCntLsb)) > (maxPicOrderCnt / 2)))
    {
        picOrderCntMsb = mPrevPicOrderCntMsb - maxPicOrderCnt;
    }
    else
    {
        picOrderCntMsb = mPrevPicOrderCntMsb;
    }

    picOrderCntVal = picOrderCntMsb + slice.mSlicePicOrderCntLsb;

    if ((naluHeader.mNuhTemporalIdPlus1 - 1) == 0
        && !(naluHeader.mH265NalUnitType >= H265NalUnitType::CODED_SLICE_RADL_N && naluHeader.mH265NalUnitType <= H265NalUnitType::CODED_SLICE_RASL_R)
        && !(naluHeader.mH265NalUnitType <= H265NalUnitType::RESERVED_VCL_R15 && (int(naluHeader.mH265NalUnitType) & 1) == 0))
    {
        mPrevPicOrderCntLsb = slice.mSlicePicOrderCntLsb;
        mPrevPicOrderCntMsb = picOrderCntMsb;
    }

    return picOrderCntVal;
}

bool H265Parser::checkAccessUnitBoundary(const vector<uint8_t>& nalUnit, const bool isFirstNaluInAU, const bool firstVclNaluFound)
{
    H265NalUnitType naluType;

    if (nalUnit.size() == 0)
    {
        return true;
    }

    if (isFirstVclNaluInPic(nalUnit))
    {
        if (firstVclNaluFound)
        {
            // NALU is not the first vcl NALU of the current picture so it must be first NALU of the next picture
            return true;
        }
        else
        {
            return false;
        }
    }

    if (isFirstNaluInAU)
    {
        return false;
    }

    naluType = getH265NalUnitType(nalUnit);

    if (firstVclNaluFound &&
        (naluType == H265NalUnitType::ACCESS_UNIT_DELIMITER
            || naluType == H265NalUnitType::VPS || naluType == H265NalUnitType::SPS || naluType == H265NalUnitType::PPS
            || naluType == H265NalUnitType::PREFIX_SEI
            || (naluType >= H265NalUnitType::RESERVED_NVCL41 && naluType <= H265NalUnitType::RESERVED_NVCL44)
            || (naluType >= H265NalUnitType::UNSPECIFIED_48 && naluType <= H265NalUnitType::UNSPECIFIED_55)))
    {
        return true;
    }

    return false;
}

unsigned int H265Parser::ceilLog2(const unsigned int x)
{
    unsigned int i = 1;

    while (x > (1u << i))
    {
        ++i;
    }

    return i;
}

int H265Parser::parseNalUnitHeader(Parser::BitStream& bitstr, NalUnitHeader& naluHeader)
{
    bitstr.readBits(1);  // forbidden_zero_bit
    naluHeader.mH265NalUnitType = H265NalUnitType(bitstr.readBits(6));
    naluHeader.mNuhLayerId = bitstr.readBits(6);
    naluHeader.mNuhTemporalIdPlus1 = bitstr.readBits(3);

    return 0;
}


int H265Parser::parseProfileTierLevel(Parser::BitStream& bitstr, ProfileTierLevel& ptl, const unsigned int maxNumSubLayersMinus1)
{
    ptl.mGeneralProfileSpace = bitstr.readBits(2);
    ptl.mGeneralTierFlag = bitstr.readBits(1);
    ptl.mGeneralProfileIdc = bitstr.readBits(5);
    for (int i = 0; i < 32; ++i)
    {
        ptl.mGeneralProfileCompatibilityFlag[i] = bitstr.readBits(1); // TODO
    }
    ptl.mGeneralProgressiveSourceFlag = bitstr.readBits(1);
    ptl.mGeneralInterlacedSourceFlag = bitstr.readBits(1);
    ptl.mGeneralNonPackedConstraintFlag = bitstr.readBits(1);
    ptl.mGeneralFrameOnlyConstraintFlag = bitstr.readBits(1);
    for (int i = 0; i < 2; ++i)  // general_reserved_zero_44bits
    {
        bitstr.readBits(22);
    }
    ptl.mGeneralLevelIdc = bitstr.readBits(8);
    for (unsigned int i = 0; i < maxNumSubLayersMinus1; ++i)
    {
        ptl.mSubLayerProfilePresentFlag.push_back(bitstr.readBits(1));
        ptl.mSubLayerLevelPresentFlag.push_back(bitstr.readBits(1));
    }
    if (maxNumSubLayersMinus1 > 0)
    {
        for (int i = maxNumSubLayersMinus1; i < 8; ++i)
        {
            bitstr.readBits(2);  // reserved_zero_2bits
        }
    }
    ptl.mSubLayerProfileTierLevels.resize(maxNumSubLayersMinus1);
    for (unsigned int i = 0; i < maxNumSubLayersMinus1; ++i)
    {
        ptl.mSubLayerProfileTierLevels[i].mSubLayerTierFlag = 0;
        if (ptl.mSubLayerProfilePresentFlag[i])
        {
            ptl.mSubLayerProfileTierLevels[i].mSubLayerProfileSpace = bitstr.readBits(2);
            ptl.mSubLayerProfileTierLevels[i].mSubLayerTierFlag = bitstr.readBits(1);
            ptl.mSubLayerProfileTierLevels[i].mSubLayerProfileIdc = bitstr.readBits(5);
            for (int j = 0; j < 32; ++j)
            {
                ptl.mSubLayerProfileTierLevels[i].mSubLayerProfileCompatibilityFlag[j] = bitstr.readBits(1);
            }
            ptl.mSubLayerProfileTierLevels[i].mSubLayerProgressiveSourceFlag = bitstr.readBits(1);
            ptl.mSubLayerProfileTierLevels[i].mSubLayerInterlacedSourceFlag = bitstr.readBits(1);
            ptl.mSubLayerProfileTierLevels[i].mSubLayerNonPackedConstraintFlag = bitstr.readBits(1);
            ptl.mSubLayerProfileTierLevels[i].mSubLayerFrameOnlyConstraintFlag = bitstr.readBits(1);
            for (int j = 0; j < 2; ++j)  // sub_layer_reserved_zero_44bits
            {
                bitstr.readBits(22);
            }
        }
        if (ptl.mSubLayerLevelPresentFlag[i])
        {
            ptl.mSubLayerProfileTierLevels[i].mSubLayerLevelIdc = bitstr.readBits(8);
        }
    }

    return 0;
}

int H265Parser::parseShortTermRefPicSet(Parser::BitStream& bitstr, const vector<ShortTermRefPicSetDerived>& rpsList, ShortTermRefPicSet& rps, const unsigned int stRpsIdx, const unsigned int numShortTermRefPicSets)
{
    rps.mUsedByCurrPicFlag.clear();
    rps.mUseDeltaFlag.clear();
    rps.mDeltaPocS0Minus1.clear();
    rps.mUsedByCurrPicS0Flag.clear();
    rps.mDeltaPocS1Minus1.clear();
    rps.mUsedByCurrPicS1Flag.clear();

    rps.mInterRefPicSetPredictionFlag = 0;
    if (stRpsIdx != 0)
    {
        rps.mInterRefPicSetPredictionFlag = bitstr.readBits(1);
    }
    if (rps.mInterRefPicSetPredictionFlag)
    {
        rps.mDeltaIdxMinus1 = 0;
        if (stRpsIdx == numShortTermRefPicSets)
        {
            rps.mDeltaIdxMinus1 = bitstr.readExpGolombCode();
        }
        rps.mDeltaRpsSign = bitstr.readBits(1);
        rps.mAbsDeltaRpsMinus1 = bitstr.readExpGolombCode();
        const unsigned int refRpsIdx = stRpsIdx - (rps.mDeltaIdxMinus1 + 1);
        for (unsigned int j = 0; j <= rpsList[refRpsIdx].mNumDeltaPocs; ++j)
        {
            rps.mUsedByCurrPicFlag.push_back(bitstr.readBits(1));
            if (!rps.mUsedByCurrPicFlag[j])
            {
                rps.mUseDeltaFlag.push_back(bitstr.readBits(1));
            }
            else
            {
                rps.mUseDeltaFlag.push_back(1);
            }
        }
    }
    else
    {
        rps.mNumNegativePics = bitstr.readExpGolombCode();
        rps.mNumPositivePics = bitstr.readExpGolombCode();
        for (unsigned int i = 0; i < rps.mNumNegativePics; ++i)
        {
            rps.mDeltaPocS0Minus1.push_back(bitstr.readExpGolombCode());
            rps.mUsedByCurrPicS0Flag.push_back(bitstr.readBits(1));
        }
        for (unsigned int i = 0; i < rps.mNumPositivePics; ++i)
        {
            rps.mDeltaPocS1Minus1.push_back(bitstr.readExpGolombCode());
            rps.mUsedByCurrPicS1Flag.push_back(bitstr.readBits(1));
        }
    }

    return 0;
}

int H265Parser::deriveRefPicSetParams(const vector<ShortTermRefPicSetDerived>& rpsList, const ShortTermRefPicSet& rps, ShortTermRefPicSetDerived& rpsDerived, const unsigned int stRpsIdx)
{
    rpsDerived.mUsedByCurrPicS0.clear();
    rpsDerived.mUsedByCurrPicS1.clear();
    rpsDerived.mDeltaPocS0.clear();
    rpsDerived.mDeltaPocS1.clear();

    if (rps.mInterRefPicSetPredictionFlag)
    {
        unsigned int i = 0;
        int dPoc;
        const unsigned int refRpsIdx = stRpsIdx - (rps.mDeltaIdxMinus1 + 1);
        const int deltaRps = (1 - 2 * rps.mDeltaRpsSign) * (rps.mAbsDeltaRpsMinus1 + 1);

        const ShortTermRefPicSetDerived& refRps = rpsList[refRpsIdx];

        for (int j = refRps.mNumPositivePics - 1; j >= 0; --j)
        {
            dPoc = refRps.mDeltaPocS1[j] + deltaRps;
            if (dPoc < 0 && rps.mUseDeltaFlag[refRps.mNumNegativePics + j])
            {
                rpsDerived.mDeltaPocS0.push_back(dPoc);
                rpsDerived.mUsedByCurrPicS0.push_back(rps.mUsedByCurrPicFlag[refRps.mNumNegativePics + j]);
                ++i;
            }
        }
        if (deltaRps < 0 && rps.mUseDeltaFlag[refRps.mNumDeltaPocs])
        {
            rpsDerived.mDeltaPocS0.push_back(deltaRps);
            rpsDerived.mUsedByCurrPicS0.push_back(rps.mUsedByCurrPicFlag[refRps.mNumDeltaPocs]);
            ++i;
        }
        for (unsigned int j = 0; j < refRps.mNumNegativePics; ++j)
        {
            dPoc = refRps.mDeltaPocS0[j] + deltaRps;
            if (dPoc < 0 && rps.mUseDeltaFlag[j])
            {
                rpsDerived.mDeltaPocS0.push_back(dPoc);
                rpsDerived.mUsedByCurrPicS0.push_back(rps.mUsedByCurrPicFlag[j]);
                ++i;
            }
        }
        rpsDerived.mNumNegativePics = i;

        i = 0;
        for (int j = refRps.mNumNegativePics - 1; j >= 0; --j)
        {
            dPoc = refRps.mDeltaPocS0[j] + deltaRps;
            if (dPoc > 0 && rps.mUseDeltaFlag[j])
            {
                rpsDerived.mDeltaPocS1.push_back(dPoc);
                rpsDerived.mUsedByCurrPicS1.push_back(rps.mUsedByCurrPicFlag[j]);
                ++i;
            }
        }
        if (deltaRps > 0 && rps.mUseDeltaFlag[refRps.mNumDeltaPocs])
        {
            rpsDerived.mDeltaPocS1.push_back(deltaRps);
            rpsDerived.mUsedByCurrPicS1.push_back(rps.mUsedByCurrPicFlag[refRps.mNumDeltaPocs]);
            ++i;
        }
        for (unsigned int j = 0; j < refRps.mNumPositivePics; ++j)
        {
            dPoc = refRps.mDeltaPocS1[j] + deltaRps;
            if (dPoc > 0 && rps.mUseDeltaFlag[refRps.mNumNegativePics + j])
            {
                rpsDerived.mDeltaPocS1.push_back(dPoc);
                rpsDerived.mUsedByCurrPicS1.push_back(rps.mUsedByCurrPicFlag[refRps.mNumNegativePics + j]);
                ++i;
            }
        }
        rpsDerived.mNumPositivePics = i;
    }
    else
    {
        rpsDerived.mNumNegativePics = rps.mNumNegativePics;
        rpsDerived.mNumPositivePics = rps.mNumPositivePics;
        for (unsigned int i = 0; i < rps.mNumNegativePics; ++i)
        {
            rpsDerived.mUsedByCurrPicS0.push_back(rps.mUsedByCurrPicS0Flag[i]);
            if (i == 0)
            {
                rpsDerived.mDeltaPocS0.push_back(-(int(rps.mDeltaPocS0Minus1[i]) + 1));
            }
            else
            {
                rpsDerived.mDeltaPocS0.push_back(rpsDerived.mDeltaPocS0[i - 1] - (rps.mDeltaPocS0Minus1[i] + 1));
            }
        }
        for (unsigned int i = 0; i < rps.mNumPositivePics; ++i)
        {
            rpsDerived.mUsedByCurrPicS1.push_back(rps.mUsedByCurrPicS1Flag[i]);
            if (i == 0)
            {
                rpsDerived.mDeltaPocS1.push_back(rps.mDeltaPocS1Minus1[i] + 1);
            }
            else
            {
                rpsDerived.mDeltaPocS1.push_back(rpsDerived.mDeltaPocS1[i - 1] + (rps.mDeltaPocS1Minus1[i] + 1));
            }
        }
    }

    rpsDerived.mNumDeltaPocs = rpsDerived.mNumNegativePics + rpsDerived.mNumPositivePics;

    return 0;
}

int H265Parser::parseRefPicListsModification(Parser::BitStream& bitstr, const SliceHeader& slice, RefPicListsModification& refPicListsMod)
{
    unsigned int entryLen = ceilLog2(slice.mNumPocTotalCurr);

    refPicListsMod.mListEntryL0.clear();
    refPicListsMod.mListEntryL1.clear();

    refPicListsMod.mRefPicListModificationFlagL0 = bitstr.readBits(1);
    if (refPicListsMod.mRefPicListModificationFlagL0)
    {
        for (unsigned int i = 0; i <= slice.mNumRefIdxL0ActiveMinus1; ++i)
        {
            refPicListsMod.mListEntryL0.push_back(bitstr.readBits(entryLen));
        }
    }
    if (slice.mSliceType == SliceType::B)
    {
        refPicListsMod.mRefPicListModificationFlagL1 = bitstr.readBits(1);
        if (refPicListsMod.mRefPicListModificationFlagL1)
        {
            for (unsigned int i = 0; i <= slice.mNumRefIdxL1ActiveMinus1; ++i)
            {
                refPicListsMod.mListEntryL1.push_back(bitstr.readBits(entryLen));
            }
        }
    }

    return 0;
}

int H265Parser::decodeRefPicSet(SliceHeader& slice, RefPicSet& rps, const int poc)
{
    Picture* pic;
    ShortTermRefPicSetDerived& currStRps = *slice.mCurrStRps;
    int maxPicOrderCnt = 1 << (slice.mSps->mLog2MaxPicOrderCntLsbMinus4 + 4);

    rps.mPocStCurrBefore.clear();
    rps.mPocStCurrAfter.clear();
    rps.mPocStFoll.clear();
    rps.mPocLtCurr.clear();
    rps.mPocLtFoll.clear();
    rps.mCurrDeltaPocMsbPresentFlag.clear();
    rps.mFollDeltaPocMsbPresentFlag.clear();
    rps.mNumPocStCurrBefore = 0;
    rps.mNumPocStCurrAfter = 0;
    rps.mNumPocStFoll = 0;
    rps.mNumPocLtCurr = 0;
    rps.mNumPocLtFoll = 0;
    rps.mRefPicSetStCurrBefore.clear();
    rps.mRefPicSetStCurrAfter.clear();
    rps.mRefPicSetStFoll.clear();
    rps.mRefPicSetLtCurr.clear();
    rps.mRefPicSetLtFoll.clear();

    for (unsigned int i = 0; i < currStRps.mNumNegativePics; ++i)
    {
        if (currStRps.mUsedByCurrPicS0[i])
        {
            rps.mPocStCurrBefore.push_back(poc + currStRps.mDeltaPocS0[i]);
        }
        else
        {
            rps.mPocStFoll.push_back(poc + currStRps.mDeltaPocS0[i]);
        }
    }
    rps.mNumPocStCurrBefore = rps.mPocStCurrBefore.size();

    for (unsigned int i = 0; i < currStRps.mNumPositivePics; ++i)
    {
        if (currStRps.mUsedByCurrPicS1[i])
        {
            rps.mPocStCurrAfter.push_back(poc + currStRps.mDeltaPocS1[i]);
        }
        else
        {
            rps.mPocStFoll.push_back(poc + currStRps.mDeltaPocS1[i]);
        }
    }
    rps.mNumPocStCurrAfter = rps.mPocStCurrAfter.size();
    rps.mNumPocStFoll = rps.mPocStFoll.size();

    for (unsigned int i = 0; i < (slice.mNumLongTermSps + slice.mNumLongTermPics); ++i)
    {
        int pocLt = slice.mPocLsbLt[i];
        if (slice.mDeltaPocMsbPresentFlag[i])
        {
            pocLt += poc - deltaPocMsbCycleLt(slice, i) * maxPicOrderCnt - slice.mSlicePicOrderCntLsb;
        }
        if (slice.mUsedByCurrPicLt[i])
        {
            rps.mPocLtCurr.push_back(pocLt);
            rps.mCurrDeltaPocMsbPresentFlag.push_back(slice.mDeltaPocMsbPresentFlag[i]);
        }
        else
        {
            rps.mPocLtFoll.push_back(pocLt);
            rps.mFollDeltaPocMsbPresentFlag.push_back(slice.mDeltaPocMsbPresentFlag[i]);
        }
    }
    rps.mNumPocLtCurr = rps.mPocLtCurr.size();
    rps.mNumPocLtFoll = rps.mPocLtFoll.size();

    for (auto& i : mDecodedPicBuffer)
    {
        i.mIsReferecePic = false;
    }

    for (unsigned int i = 0; i < rps.mNumPocLtCurr; ++i)
    {
        if (!rps.mCurrDeltaPocMsbPresentFlag[i])
        {
            pic = findPicInDpbPocLsb(rps.mPocLtCurr[i]);
        }
        else
        {
            pic = findPicInDpbPoc(rps.mPocLtCurr[i]);
        }
        rps.mRefPicSetLtCurr.push_back(pic);
        if (pic != nullptr)
        {
            pic->mIsReferecePic = true;
            pic->mIsLongTermRefPic = true;
        }
    }
    for (unsigned int i = 0; i < rps.mNumPocLtFoll; ++i)
    {
        if (!rps.mFollDeltaPocMsbPresentFlag[i])
        {
            pic = findPicInDpbPocLsb(rps.mPocLtFoll[i]);
        }
        else
        {
            pic = findPicInDpbPoc(rps.mPocLtFoll[i]);
        }
        rps.mRefPicSetLtFoll.push_back(pic);
        if (pic != nullptr)
        {
            pic->mIsReferecePic = true;
            pic->mIsLongTermRefPic = true;
        }
    }

    for (unsigned int i = 0; i < rps.mNumPocStCurrBefore; ++i)
    {
        pic = findPicInDpbPoc(rps.mPocStCurrBefore[i]);
        rps.mRefPicSetStCurrBefore.push_back(pic);
        if (pic != nullptr)
        {
            pic->mIsReferecePic = true;
            pic->mIsLongTermRefPic = false;
        }
    }
    for (unsigned int i = 0; i < rps.mNumPocStCurrAfter; ++i)
    {
        pic = findPicInDpbPoc(rps.mPocStCurrAfter[i]);
        rps.mRefPicSetStCurrAfter.push_back(pic);
        if (pic != nullptr)
        {
            pic->mIsReferecePic = true;
            pic->mIsLongTermRefPic = false;
        }
    }
    for (unsigned int i = 0; i < rps.mNumPocStFoll; ++i)
    {
        pic = findPicInDpbPoc(rps.mPocStFoll[i]);
        rps.mRefPicSetStFoll.push_back(pic);
        if (pic != nullptr)
        {
            pic->mIsReferecePic = true;
            pic->mIsLongTermRefPic = false;
        }
    }

    return 0;
}

Picture* H265Parser::findPicInDpbPocLsb(const unsigned int pocLsb)
{
    Picture* pic = nullptr;

    for (auto& i : mDecodedPicBuffer)
    {
        if (i.mSlicePicOrderCntLsb == pocLsb)
        {
            pic = &i;
            break;
        }
    }

    return pic;
}

Picture* H265Parser::findPicInDpbPoc(const int poc)
{
    Picture* pic = nullptr;

    for (auto& i : mDecodedPicBuffer)
    {
        if (i.mPoc == poc)
        {
            pic = &i;
            break;
        }
    }

    return pic;
}

int H265Parser::deltaPocMsbCycleLt(SliceHeader& slice, const int i)
{
    if (i == 0 || i == int(slice.mNumLongTermSps))
    {
        return slice.mDeltaPocMsbCycleLt[i];
    }
    else
    {
        return slice.mDeltaPocMsbCycleLt[i] + deltaPocMsbCycleLt(slice, i - 1);
    }
}

int H265Parser::generateRefPicLists(SliceHeader& slice, RefPicSet& rps)
{
    unsigned int numRpsCurrTempList0;
    unsigned int rIdx;
    vector<Picture*> refPicListTemp0;
    vector<Picture*> refPicListTemp1;

    numRpsCurrTempList0 = std::max<unsigned int>(slice.mNumRefIdxL0ActiveMinus1 + 1, slice.mNumPocTotalCurr);

    mRefPicList0.clear();
    mRefPicList1.clear();

    rIdx = 0;
    while (rIdx < numRpsCurrTempList0)
    {
        for (unsigned int i = 0; i < rps.mNumPocStCurrBefore && rIdx < numRpsCurrTempList0; rIdx++, ++i)
        {
            refPicListTemp0.push_back(rps.mRefPicSetStCurrBefore[i]);
        }
        for (unsigned int i = 0; i < rps.mNumPocStCurrAfter && rIdx < numRpsCurrTempList0; rIdx++, ++i)
        {
            refPicListTemp0.push_back(rps.mRefPicSetStCurrAfter[i]);
        }
        for (unsigned int i = 0; i < rps.mNumPocLtCurr && rIdx < numRpsCurrTempList0; rIdx++, ++i)
        {
            refPicListTemp0.push_back(rps.mRefPicSetLtCurr[i]);
        }
    }

    for (rIdx = 0; rIdx <= slice.mNumRefIdxL0ActiveMinus1; rIdx++)
    {
        if (slice.mPps->mListsModificationPresentFlag && slice.mRefPicListsModification.mRefPicListModificationFlagL0)
        {
            mRefPicList0.push_back(refPicListTemp0[slice.mRefPicListsModification.mListEntryL0[rIdx]]);
        }
        else
        {
            mRefPicList0.push_back(refPicListTemp0[rIdx]);
        }
    }

    if (slice.mSliceType == SliceType::B)
    {
        const unsigned int numRpsCurrTempList1 = std::max<unsigned int>(slice.mNumRefIdxL1ActiveMinus1 + 1, slice.mNumPocTotalCurr);

        rIdx = 0;
        while (rIdx < numRpsCurrTempList1)
        {
            for (unsigned int i = 0; i < rps.mNumPocStCurrAfter && rIdx < numRpsCurrTempList1; rIdx++, ++i)
            {
                refPicListTemp1.push_back(rps.mRefPicSetStCurrAfter[i]);
            }
            for (unsigned int i = 0; i < rps.mNumPocStCurrBefore && rIdx < numRpsCurrTempList1; rIdx++, ++i)
            {
                refPicListTemp1.push_back(rps.mRefPicSetStCurrBefore[i]);
            }
            for (unsigned int i = 0; i < rps.mNumPocLtCurr && rIdx < numRpsCurrTempList1; rIdx++, ++i)
            {
                refPicListTemp1.push_back(rps.mRefPicSetLtCurr[i]);
            }
        }

        for (rIdx = 0; rIdx <= slice.mNumRefIdxL1ActiveMinus1; rIdx++)
        {
            if (slice.mPps->mListsModificationPresentFlag && slice.mRefPicListsModification.mRefPicListModificationFlagL1)
            {
                mRefPicList1.push_back(refPicListTemp1[slice.mRefPicListsModification.mListEntryL1[rIdx]]);
            }
            else
            {
                mRefPicList1.push_back(refPicListTemp1[rIdx]);
            }
        }
    }

    return 0;
}

int H265Parser::parseScalingListData(Parser::BitStream& bitstr, ScalingListData& scalingList)
{
    for (int sizeId = 0; sizeId < 4; sizeId++)
    {
        int numMatrix = (sizeId == 3) ? 2 : 6;
        scalingList.mScalingListPredModeFlag[sizeId].resize(numMatrix);
        scalingList.mScalingListPredMatrixIdDelta[sizeId].resize(numMatrix);
        if (sizeId > 1)
        {
            scalingList.mScalingListDcCoefMinus8[sizeId - 2].resize(numMatrix);
        }
        scalingList.mScalingList[sizeId].resize(numMatrix);

        for (int matrixId = 0; matrixId < numMatrix; matrixId++)
        {
            scalingList.mScalingListPredModeFlag[sizeId][matrixId] = bitstr.readBits(1);
            if (!scalingList.mScalingListPredModeFlag[sizeId][matrixId])
            {
                scalingList.mScalingListPredMatrixIdDelta[sizeId][matrixId] = bitstr.readExpGolombCode();
            }
            else
            {
                int nextCoef = 8;
                int coefNum = std::min(64, 1 << (4 + (sizeId << 1)));
                if (sizeId > 1)
                {
                    scalingList.mScalingListDcCoefMinus8[sizeId - 2][matrixId] = bitstr.readSignedExpGolombCode();
                    nextCoef = scalingList.mScalingListDcCoefMinus8[sizeId - 2][matrixId] + 8;
                }
                for (int i = 0; i < coefNum; ++i)
                {
                    int scalingListDeltaCoef = bitstr.readSignedExpGolombCode();
                    nextCoef = (nextCoef + scalingListDeltaCoef + 256) % 256;
                    scalingList.mScalingList[sizeId][matrixId].push_back(nextCoef);
                }
            }
        }
    }

    return 0;
}


void H265Parser::setVuiDefaults(ProfileTierLevel& ptl, VuiParameters& vui)
{
    vui = {};

    // The following syntax elements have explicit default values in the spec 
    vui.mAspectRatioIdc = 0;
    vui.mOverscanAppropriateFlag = 0;
    vui.mVideoFormat = 5;
    vui.mVideoFullRangeFlag = 0;
    vui.mCcolourPrimaries = 2;
    vui.mTransferCharacteristics = 2;
    vui.mMatrixCoeffs = 2;
    vui.mChromaSampleLocTypeTopField = 0;
    vui.mChromaSampleLocTypeBottomField = 0;
    vui.mNeutralChromaIndicationFlag = 0;
    vui.mFieldSeqFlag = 0;
    vui.mFrameFieldInfoPresentFlag = (ptl.mGeneralProgressiveSourceFlag && ptl.mGeneralInterlacedSourceFlag) ? 1 : 0;
    vui.mDefDispWinLeftOffset = 0;
    vui.mDefDispWinRightOffset = 0;
    vui.mDefDispWinTopOffset = 0;
    vui.mDefDispWinBottomOffset = 0;
    vui.mTilesFixedStructureFlag = 0;
    vui.mMotionVectorsOverPicBoundariesFlag = 1;
    vui.mRestrictedRefPicListsFlag = 0;
    vui.mMinSpatialSegmentationIdc = 0;
    vui.mMaxBytesPerPicDenom = 2;
    vui.mMaxBitsPerMinCuDenom = 1;
    vui.mLog2MaxMvLengthHorizontal = 15;
    vui.mLog2MaxMvLengthVertical = 15;
}

int H265Parser::parseVuiParameters(Parser::BitStream& bitstr, VuiParameters& vui)
{
    vui.mAspectRatioInfoPresentFlag = bitstr.readBits(1);
    if (vui.mAspectRatioInfoPresentFlag)
    {
        vui.mAspectRatioIdc = bitstr.readBits(8);
        if (vui.mAspectRatioIdc == EXTENDED_SAR)
        {
            vui.mSarWidth = bitstr.readBits(16);
            vui.mSarHeight = bitstr.readBits(16);
        }
    }
    vui.mOverscanInfoPresentFlag = bitstr.readBits(1);
    if (vui.mOverscanInfoPresentFlag)
    {
        vui.mOverscanAppropriateFlag = bitstr.readBits(1);
    }
    vui.mVideoSignalTypePresentFlag = bitstr.readBits(1);
    if (vui.mVideoSignalTypePresentFlag)
    {
        vui.mVideoFormat = bitstr.readBits(3);
        vui.mVideoFullRangeFlag = bitstr.readBits(1);
        vui.mColourDescriptionPresentFlag = bitstr.readBits(1);
        if (vui.mColourDescriptionPresentFlag)
        {
            vui.mCcolourPrimaries = bitstr.readBits(8);
            vui.mTransferCharacteristics = bitstr.readBits(8);
            vui.mMatrixCoeffs = bitstr.readBits(8);
        }
    }
    vui.mChromaLocInfoPresentFlag = bitstr.readBits(1);
    if (vui.mChromaLocInfoPresentFlag)
    {
        vui.mChromaSampleLocTypeTopField = bitstr.readExpGolombCode();
        vui.mChromaSampleLocTypeBottomField = bitstr.readExpGolombCode();
    }
    vui.mNeutralChromaIndicationFlag = bitstr.readBits(1);
    vui.mFieldSeqFlag = bitstr.readBits(1);
    vui.mFrameFieldInfoPresentFlag = bitstr.readBits(1);
    vui.mDefaultDisplayWindowFlag = bitstr.readBits(1);
    if (vui.mDefaultDisplayWindowFlag)
    {
        vui.mDefDispWinLeftOffset = bitstr.readExpGolombCode();
        vui.mDefDispWinRightOffset = bitstr.readExpGolombCode();
        vui.mDefDispWinTopOffset = bitstr.readExpGolombCode();
        vui.mDefDispWinBottomOffset = bitstr.readExpGolombCode();
    }
    vui.mVuiTimingInfoPresentFlag = bitstr.readBits(1);
    if (vui.mVuiTimingInfoPresentFlag)
    {
        vui.mVuiNumUnitsInTick = bitstr.readBits(32);
        vui.mVuiTimeScale = bitstr.readBits(32);
        vui.mVuiPocProportionalToTimingFlag = bitstr.readBits(1);
        if (vui.mVuiPocProportionalToTimingFlag)
        {
            vui.mVuiNumTicksPocDiffOneMinus1 = bitstr.readExpGolombCode();
            vui.mVuiHrdParametersPresentFlag = bitstr.readBits(1);
            if (vui.mVuiHrdParametersPresentFlag)
            {

            }
        }
    }
    vui.mBitstreamRestrictionFlag = bitstr.readBits(1);
    if (vui.mBitstreamRestrictionFlag)
    {
        vui.mTilesFixedStructureFlag = bitstr.readBits(1);
        vui.mMotionVectorsOverPicBoundariesFlag = bitstr.readBits(1);
        vui.mRestrictedRefPicListsFlag = bitstr.readBits(1);
        vui.mMinSpatialSegmentationIdc = bitstr.readExpGolombCode();
        vui.mMaxBytesPerPicDenom = bitstr.readExpGolombCode();
        vui.mMaxBitsPerMinCuDenom = bitstr.readExpGolombCode();
        vui.mLog2MaxMvLengthHorizontal = bitstr.readExpGolombCode();
        vui.mLog2MaxMvLengthVertical = bitstr.readExpGolombCode();
    }

    return 0;
}

int H265Parser::parseSubLayerHrd(Parser::BitStream& bitstr, SubLayerHrdParameters& hrd, const int cpbCnt, const unsigned int subPicHrdParamsPresentFlag)
{
    hrd.mBitRateValueMinus1.clear();
    hrd.mCpbSizeValueMinus1.clear();
    hrd.mCpbSizeDuValueMinus1.clear();
    hrd.mBitRateDuValueMinus1.clear();
    hrd.mCbrFlag.clear();

    for (int i = 0; i <= cpbCnt; ++i)
    {
        hrd.mBitRateValueMinus1.push_back(bitstr.readExpGolombCode());
        hrd.mCpbSizeValueMinus1.push_back(bitstr.readExpGolombCode());
        if (subPicHrdParamsPresentFlag)
        {
            hrd.mCpbSizeDuValueMinus1.push_back(bitstr.readExpGolombCode());
            hrd.mBitRateDuValueMinus1.push_back(bitstr.readExpGolombCode());
        }
        hrd.mCbrFlag.push_back(bitstr.readExpGolombCode());
    }

    return 0;
}

int H265Parser::parseHrdParameters(Parser::BitStream& bitstr, HrdParameters& hrd, const unsigned int commonInfPresentFlag, const unsigned int maxNumSubLayersMinus1)
{
    hrd.mSubPicHrdParamsPresentFlag = 0;
    if (commonInfPresentFlag)
    {
        hrd.mNalHrdParameterPresentFlag = bitstr.readBits(1);
        hrd.mVclHrdParametersPresentFlag = bitstr.readBits(1);
        if (hrd.mNalHrdParameterPresentFlag || hrd.mVclHrdParametersPresentFlag)
        {
            hrd.mSubPicHrdParamsPresentFlag = bitstr.readBits(1);
            if (hrd.mSubPicHrdParamsPresentFlag)
            {
                hrd.mTickDivisorMinus2 = bitstr.readBits(8);
                hrd.mDuCpbRemovalDelayIncrementLengthMinus1 = bitstr.readBits(5);
                hrd.mSubPicCpbParamsInPicTimingSeiFlag = bitstr.readBits(1);
                hrd.mDpbOutputDelayDuLengthMinus1 = bitstr.readBits(5);
            }
            hrd.mBitRateScale = bitstr.readBits(4);
            hrd.mCpbSizeScale = bitstr.readBits(4);
            if (hrd.mSubPicHrdParamsPresentFlag)
            {
                hrd.mCpbSizeDuScale = bitstr.readBits(4);
            }
            hrd.mInitialCpbRemovalDelayLengthMinus1 = bitstr.readBits(5);
            hrd.mAuCpbRemovalDelayLengthMinus1 = bitstr.readBits(5);
            hrd.mDpbOutputDelayLengthMinus1 = bitstr.readBits(5);
        }
    }
    hrd.mFixedPicRateGeneralFlag.resize(maxNumSubLayersMinus1 + 1);
    hrd.mFixedPicRateWithinCvsFlag.resize(maxNumSubLayersMinus1 + 1);
    hrd.mElementalDurationInTcMinus1.resize(maxNumSubLayersMinus1 + 1);
    hrd.mLowDelayHrdFlag.resize(maxNumSubLayersMinus1 + 1);
    hrd.mCpbCntMinus1.resize(maxNumSubLayersMinus1 + 1);
    if (hrd.mNalHrdParameterPresentFlag)
    {
        hrd.mSubLayerNalHrdParams.resize(maxNumSubLayersMinus1 + 1);
    }
    if (hrd.mVclHrdParametersPresentFlag)
    {
        hrd.mSubLayerVclHrdParams.resize(maxNumSubLayersMinus1 + 1);
    }
    for (unsigned int i = 0; i <= maxNumSubLayersMinus1; ++i)
    {
        hrd.mFixedPicRateGeneralFlag[i] = bitstr.readBits(1);
        if (!hrd.mFixedPicRateGeneralFlag[i])
        {
            hrd.mFixedPicRateWithinCvsFlag[i] = bitstr.readBits(1);
        }
        if (hrd.mFixedPicRateWithinCvsFlag[i])
        {
            hrd.mElementalDurationInTcMinus1[i] = bitstr.readExpGolombCode();
        }
        else
        {
            hrd.mLowDelayHrdFlag[i] = bitstr.readBits(1);
        }
        hrd.mCpbCntMinus1[i] = 0;
        if (!hrd.mLowDelayHrdFlag[i])
        {
            hrd.mCpbCntMinus1[i] = bitstr.readExpGolombCode();
        }
        if (hrd.mNalHrdParameterPresentFlag)
        {
            parseSubLayerHrd(bitstr, hrd.mSubLayerNalHrdParams[i], hrd.mCpbCntMinus1[i], hrd.mSubPicHrdParamsPresentFlag);
        }
        if (hrd.mVclHrdParametersPresentFlag)
        {
            parseSubLayerHrd(bitstr, hrd.mSubLayerVclHrdParams[i], hrd.mCpbCntMinus1[i], hrd.mSubPicHrdParamsPresentFlag);
        }
    }

    return 0;
}

int H265Parser::parsePredWeightTable(Parser::BitStream& bitstr, SequenceParameterSet& sps, SliceHeader& slice, PredWeightTable& pwTable)
{
    array<int, 2> weights;
    array<int, 2> offsets;

    pwTable.mLumaLog2WeightDenom = bitstr.readExpGolombCode();
    if (sps.mChromaFormatIdc != 0)
    {
        pwTable.mDeltaChromaLog2WeightDenom = bitstr.readSignedExpGolombCode();
    }
    for (unsigned int i = 0; i <= slice.mNumRefIdxL0ActiveMinus1; ++i)
    {
        pwTable.mLumaWeightL0Flag.push_back(bitstr.readBits(1));
    }
    for (unsigned int i = 0; i <= slice.mNumRefIdxL0ActiveMinus1; ++i)
    {
        pwTable.mChromaWeightL0Flag.push_back((sps.mChromaFormatIdc != 0) ? bitstr.readBits(1) : 0);
    }
    pwTable.mDeltaChromaWeightL0.resize(slice.mNumRefIdxL0ActiveMinus1 + 1);
    pwTable.mDeltaChromaOffsetL0.resize(slice.mNumRefIdxL0ActiveMinus1 + 1);
    for (unsigned int i = 0; i <= slice.mNumRefIdxL0ActiveMinus1; ++i)
    {
        if (pwTable.mLumaWeightL0Flag[i])
        {
            pwTable.mDeltaLumaWeightL0.push_back(bitstr.readSignedExpGolombCode());
            pwTable.mLumaOffsetL0.push_back(bitstr.readSignedExpGolombCode());
        }
        else
        {
            pwTable.mDeltaLumaWeightL0.push_back(1 << pwTable.mLumaLog2WeightDenom);
            pwTable.mLumaOffsetL0.push_back(0);
        }
        if (pwTable.mChromaWeightL0Flag[i])
        {
            for (int j = 0; j < 2; ++j)
            {
                weights[j] = bitstr.readSignedExpGolombCode();
                offsets[j] = bitstr.readSignedExpGolombCode();
            }
        }
        else
        {
            for (int j = 0; j < 2; ++j)
            {
                weights[j] = 1 << pwTable.mDeltaChromaLog2WeightDenom;
                offsets[j] = 0;
            }
        }
        pwTable.mDeltaChromaWeightL0.push_back(weights);
        pwTable.mDeltaChromaOffsetL0.push_back(offsets);
    }

    if (slice.mSliceType == SliceType::B)
    {
        for (unsigned int i = 0; i <= slice.mNumRefIdxL1ActiveMinus1; ++i)
        {
            pwTable.mLumaWeightL1Flag.push_back(bitstr.readBits(1));
        }
        for (unsigned int i = 0; i <= slice.mNumRefIdxL1ActiveMinus1; ++i)
        {
            pwTable.mChromaWeightL1Flag.push_back((sps.mChromaFormatIdc != 0) ? bitstr.readBits(1) : 0);
        }
        for (unsigned int i = 0; i <= slice.mNumRefIdxL1ActiveMinus1; ++i)
        {
            if (pwTable.mLumaWeightL1Flag[i])
            {
                pwTable.mDeltaLumaWeightL1.push_back(bitstr.readSignedExpGolombCode());
                pwTable.mLumaOffsetL1.push_back(bitstr.readSignedExpGolombCode());
            }
            else
            {
                pwTable.mDeltaLumaWeightL1.push_back(1 << pwTable.mLumaLog2WeightDenom);
                pwTable.mLumaOffsetL1.push_back(0);
            }
            if (pwTable.mChromaWeightL1Flag[i])
            {
                for (int j = 0; j < 2; ++j)
                {
                    weights[j] = bitstr.readSignedExpGolombCode();
                    offsets[j] = bitstr.readSignedExpGolombCode();
                }
            }
            else
            {
                for (int j = 0; j < 2; ++j)
                {
                    weights[j] = 1 << pwTable.mDeltaChromaLog2WeightDenom;
                    offsets[j] = 0;
                }
            }
            pwTable.mDeltaChromaWeightL1.push_back(weights);
            pwTable.mDeltaChromaOffsetL1.push_back(offsets);
        }
    }

    return 0;
}

int H265Parser::parseSPS(Parser::BitStream& bitstr, SequenceParameterSet& sps)
{
    // Set default values
    sps.mSeparateColourPlaneFlag = 0;
    sps.mConfWinLeftOffset = 0;
    sps.mConfWinRightOffset = 0;
    sps.mConfWinTopOffset = 0;
    sps.mConfWinBottomOffset = 0;
    sps.mSpsScalingListDataPresentFlag = 0;
    sps.mPcmLoopFilterDisabledFlag = 0;
    sps.mNumLongTermRefPicsSps = 0;

    sps.mVpsId = bitstr.readBits(4);
    sps.mSpsMaxSubLayersMinus1 = bitstr.readBits(3);
    sps.mSpsTemporalIdNestingFlag = bitstr.readBits(1);

    parseProfileTierLevel(bitstr, sps.mProfileTierLevel, sps.mSpsMaxSubLayersMinus1);

    sps.mSpsId = bitstr.readExpGolombCode();
    sps.mChromaFormatIdc = bitstr.readExpGolombCode();
    if (sps.mChromaFormatIdc == 3)
    {
        sps.mSeparateColourPlaneFlag = bitstr.readBits(1);
    }
    sps.mPicWidthInLumaSamples = bitstr.readExpGolombCode();
    sps.mPicHeightInLumaSamples = bitstr.readExpGolombCode();
    sps.mConformanceWindowFlag = bitstr.readBits(1);
    if (sps.mConformanceWindowFlag)
    {
        sps.mConfWinLeftOffset = bitstr.readExpGolombCode();
        sps.mConfWinRightOffset = bitstr.readExpGolombCode();
        sps.mConfWinTopOffset = bitstr.readExpGolombCode();
        sps.mConfWinBottomOffset = bitstr.readExpGolombCode();
    }
    sps.mBitDepthLumaMinus8 = bitstr.readExpGolombCode();
    sps.mBitDepthChromaMinus8 = bitstr.readExpGolombCode();
    sps.mLog2MaxPicOrderCntLsbMinus4 = bitstr.readExpGolombCode();
    sps.mSpsSubLayerOrderingInfoPresentFlag = bitstr.readBits(1);
    // Need to add some dummy data, to make the array indexing work. The same trick is in NEXT decoder.
    // The indexing depends on mSpsSubLayerOrderingInfoPresentFlag and mSpsMaxSubLayersMinus1
    for (unsigned int i = 0; i < (sps.mSpsSubLayerOrderingInfoPresentFlag ? 0 : sps.mSpsMaxSubLayersMinus1); i++)
    {
        sps.mSpsMaxDecPicBufferingMinus1.push_back(0);
        sps.mSpsMaxNumReorderPics.push_back(0);
        sps.mSpsMaxLatencyIncreasePlus1.push_back(0);
    }
    for (unsigned int i = (sps.mSpsSubLayerOrderingInfoPresentFlag ? 0 : sps.mSpsMaxSubLayersMinus1); i <= sps.mSpsMaxSubLayersMinus1; ++i)
    {
        sps.mSpsMaxDecPicBufferingMinus1.push_back(bitstr.readExpGolombCode());
        sps.mSpsMaxNumReorderPics.push_back(bitstr.readExpGolombCode());
        sps.mSpsMaxLatencyIncreasePlus1.push_back(bitstr.readExpGolombCode());
    }
    sps.mLog2MinLumaCodingBlockSizeMinus3 = bitstr.readExpGolombCode();
    sps.mLog2DiffMaxMinLumaCodingBlockSize = bitstr.readExpGolombCode();
    sps.mLog2MinTransformBlockSizeMinus2 = bitstr.readExpGolombCode();
    sps.mLog2DiffMaxMinTransformBlockSize = bitstr.readExpGolombCode();
    sps.mMaxTransformHierarchyDepthInter = bitstr.readExpGolombCode();
    sps.mMaxTransformHierarchyDepthIntra = bitstr.readExpGolombCode();
    sps.mScalingListEenabledFlag = bitstr.readBits(1);
    if (sps.mScalingListEenabledFlag)
    {
        sps.mSpsScalingListDataPresentFlag = bitstr.readBits(1);
        if (sps.mSpsScalingListDataPresentFlag)
        {
            parseScalingListData(bitstr, sps.mScalingListData);
        }
    }
    sps.mAmpEnabledFlag = bitstr.readBits(1);
    sps.mSampleAdaptiveOffsetEnabledFlag = bitstr.readBits(1);
    sps.mPcmEnabledFlag = bitstr.readBits(1);
    if (sps.mPcmEnabledFlag)
    {
        sps.mPcmSampleBitDepthLumaMinus1 = bitstr.readBits(4);
        sps.mPcmSampleBitDepthChromaMinus1 = bitstr.readBits(4);
        sps.mLog2MinPcmLumaCodingBlockSizeMinus3 = bitstr.readExpGolombCode();
        sps.mLog2DiffMaxMinPcmLumaCodingBlockSize = bitstr.readExpGolombCode();
        sps.mPcmLoopFilterDisabledFlag = bitstr.readBits(1);
    }
    sps.mNumShortTermRefPicSets = bitstr.readExpGolombCode();
    sps.mShortTermRefPicSets.resize(sps.mNumShortTermRefPicSets);
    sps.mShortTermRefPicSetsDerived.resize(sps.mNumShortTermRefPicSets);
    for (unsigned int i = 0; i < sps.mNumShortTermRefPicSets; ++i)
    {
        parseShortTermRefPicSet(bitstr, sps.mShortTermRefPicSetsDerived, sps.mShortTermRefPicSets[i], i, sps.mNumShortTermRefPicSets);
        deriveRefPicSetParams(sps.mShortTermRefPicSetsDerived, sps.mShortTermRefPicSets[i], sps.mShortTermRefPicSetsDerived[i], i);
    }
    sps.mLongTermRefPicsPresentFlag = bitstr.readBits(1);
    if (sps.mLongTermRefPicsPresentFlag)
    {
        sps.mNumLongTermRefPicsSps = bitstr.readExpGolombCode();
        for (unsigned int i = 0; i < sps.mNumLongTermRefPicsSps; ++i)
        {
            sps.mLtRefPicPocLsbSps.push_back(bitstr.readBits(sps.mLog2MaxPicOrderCntLsbMinus4 + 4));
            sps.mUsedByCurrPicLtSpsFlag.push_back(bitstr.readBits(1));
        }
    }
    sps.mSpsTemporalMvpEnabledFlag = bitstr.readBits(1);
    sps.mStrongIntraSmoothingEnabledFlag = bitstr.readBits(1);
    setVuiDefaults(sps.mProfileTierLevel, sps.mVuiParameters);
    sps.mVuiParametersPresentFlag = bitstr.readBits(1);
    if (sps.mVuiParametersPresentFlag)
    {
        parseVuiParameters(bitstr, sps.mVuiParameters);
    }
    sps.mSpsExtensionFlag = bitstr.readBits(1);

    return 0;
}


int H265Parser::parsePPS(Parser::BitStream& bitstr, PictureParameterSet& pps)
{
    // Set default values
    pps.mDiffCuQpDeltaDepth = 0;
    pps.mNumTileColumnsMinus1 = 0;
    pps.mNumTileRowsMinus1 = 0;
    pps.mUniformSpacingFlag = 1;
    pps.mLoopFilterAcrossTilesEnabledFlag = 1;
    pps.mDeblockingFilterOverrideEnabledFlag = 0;
    pps.mPpsDeblockingFilterDisabledFlag = 0;
    pps.mPpsBetaOffsetDiv2 = 0;
    pps.mPpsTcOffsetDiv2 = 0;

    pps.mPpsId = bitstr.readExpGolombCode();
    pps.mSpsId = bitstr.readExpGolombCode();
    pps.mDependentSliceSegmentsEnabledFlag = bitstr.readBits(1);
    pps.mOutputFlagPresentFlag = bitstr.readBits(1);
    pps.mNumExtraSliceHeaderBits = bitstr.readBits(3);
    pps.mSignDataHidingEnabledFlag = bitstr.readBits(1);
    pps.mCabacInitPresentFlag = bitstr.readBits(1);
    pps.mNumRefIdxL0DefaultActiveMinus1 = bitstr.readExpGolombCode();
    pps.mNumRefIdxL1DefaultActiveMinus1 = bitstr.readExpGolombCode();
    pps.mInitQpMinus26 = bitstr.readSignedExpGolombCode();
    pps.mConstrainedIntraPredFlag = bitstr.readBits(1);
    pps.mTransformSkipEnabledFlag = bitstr.readBits(1);
    pps.mCuQpDeltaEnabledFlag = bitstr.readBits(1);
    if (pps.mCuQpDeltaEnabledFlag)
    {
        pps.mDiffCuQpDeltaDepth = bitstr.readExpGolombCode();
    }
    pps.mPpsCbQpOffset = bitstr.readSignedExpGolombCode();
    pps.mPpsCrQpOffset = bitstr.readSignedExpGolombCode();
    pps.mPpsSliceChromaQpOffsetsPresentFlag = bitstr.readBits(1);
    pps.mWeightedPredFlag = bitstr.readBits(1);
    pps.mWeightedBipredFlag = bitstr.readBits(1);
    pps.mTransquantBypassEnabledFlag = bitstr.readBits(1);
    pps.mTilesEnabledFlag = bitstr.readBits(1);
    pps.mEntropyCodingSyncEnabledFlag = bitstr.readBits(1);
    if (pps.mTilesEnabledFlag)
    {
        pps.mNumTileColumnsMinus1 = bitstr.readExpGolombCode();
        pps.mNumTileRowsMinus1 = bitstr.readExpGolombCode();
        pps.mUniformSpacingFlag = bitstr.readBits(1);
        if (!pps.mUniformSpacingFlag)
        {
            for (unsigned int i = 0; i < pps.mNumTileColumnsMinus1; ++i)
            {
                pps.mColumnWidthMinus1.push_back(bitstr.readExpGolombCode());
            }
            for (unsigned int i = 0; i < pps.mNumTileRowsMinus1; ++i)
            {
                pps.mRowHeightMinus1.push_back(bitstr.readExpGolombCode());
            }
        }
        pps.mLoopFilterAcrossTilesEnabledFlag = bitstr.readBits(1);
    }
    pps.mPpsLoopFilterAcrossSicesEnabledFlag = bitstr.readBits(1);
    pps.mDeblockingFilterControlPresentFlag = bitstr.readBits(1);
    if (pps.mDeblockingFilterControlPresentFlag)
    {
        pps.mDeblockingFilterOverrideEnabledFlag = bitstr.readBits(1);
        pps.mPpsDeblockingFilterDisabledFlag = bitstr.readBits(1);
        if (!pps.mPpsDeblockingFilterDisabledFlag)
        {
            pps.mPpsBetaOffsetDiv2 = bitstr.readSignedExpGolombCode();
            pps.mPpsTcOffsetDiv2 = bitstr.readSignedExpGolombCode();
        }
    }
    pps.mPpsScalingListDataPresentFlag = bitstr.readBits(1);
    if (pps.mPpsScalingListDataPresentFlag)
    {
        parseScalingListData(bitstr, pps.mScalingListData);
    }
    pps.mListsModificationPresentFlag = bitstr.readBits(1);
    pps.mLog2ParallelMergeLevelMinus2 = bitstr.readExpGolombCode();
    pps.mSliceSegmentHeaderExtensionPresentFlag = bitstr.readBits(1);
    pps.mPpsExtensionFlag = bitstr.readBits(1);

    return 0;
}

int H265Parser::parseSliceHeader(Parser::BitStream& bitstr, SliceHeader& slice, H265NalUnitType naluType, const std::list<SequenceParameterSet*>& aSpsList, const std::list<PictureParameterSet*>& aPpsList)
{
    slice.mNaluType = naluType;
    slice.mDependentSliceSegmentFlag = 0;
    slice.mSliceSegmentAddress = 0;
    slice.mPicOutputFlag = 1;
    slice.mSlicePicOrderCntLsb = 0;
    slice.mNumLongTermSps = 0;
    slice.mNumLongTermPics = 0;
    slice.mSliceTemporalMvpEnabledFlag = 0;
    slice.mSliceSaoLumaFlag = 0;
    slice.mSliceSaoChromaFlag = 0;
    slice.mCabacInitFlag = 0;
    slice.mCollocatedFromL0Flag = 1;
    slice.mSliceQpDelta = 0;
    slice.mSliceCbQpOffset = 0;
    slice.mSliceCrQpOffset = 0;
    slice.mDeblockingFilterOverrideFlag = 0;


    slice.mFirstSliceSegmentInPicFlag = bitstr.readBits(1);
    if (naluType >= H265NalUnitType::CODED_SLICE_BLA_W_LP && naluType <= H265NalUnitType::RESERVED_IRAP_VCL23)
    {
        slice.mNoOutputOfPriorPicsFlag = bitstr.readBits(1);
    }
    slice.mPpsId = bitstr.readExpGolombCode();

    slice.mPps = findPps(slice.mPpsId, aPpsList);
    //std::cout << "Slice Id: " << slice.mPpsId << std::endl;
    assert(slice.mPps != nullptr);
    slice.mSps = findSps(slice.mPps->mSpsId, aSpsList);
    assert(slice.mSps != nullptr);

    SequenceParameterSet& sps = *slice.mSps;
    PictureParameterSet& pps = *slice.mPps;

    slice.mNumRefIdxL0ActiveMinus1 = pps.mNumRefIdxL0DefaultActiveMinus1;
    slice.mNumRefIdxL1ActiveMinus1 = pps.mNumRefIdxL1DefaultActiveMinus1;
    slice.mSliceDeblockingFilterDisabledFlag = pps.mPpsDeblockingFilterDisabledFlag;
    slice.mSliceBetaOffsetDiv2 = pps.mPpsBetaOffsetDiv2;
    slice.mSliceTcOffsetDiv2 = pps.mPpsTcOffsetDiv2;
    slice.mSliceLoopFilterAcrossSlicesEnabledFlag = pps.mPpsLoopFilterAcrossSicesEnabledFlag;

    slice.mDependentSliceSegmentFlag = 0;
    if (!slice.mFirstSliceSegmentInPicFlag)
    {
        if (pps.mDependentSliceSegmentsEnabledFlag)
        {
            slice.mDependentSliceSegmentFlag = bitstr.readBits(1);
        }
        unsigned int minCbLog2SizeY = sps.mLog2MinLumaCodingBlockSizeMinus3 + 3;
        unsigned int ctbLog2SizeY = minCbLog2SizeY + sps.mLog2DiffMaxMinLumaCodingBlockSize;
        unsigned int ctbSizeY = 1 << ctbLog2SizeY;
        unsigned int picWidthInCtbsY = (sps.mPicWidthInLumaSamples + ctbSizeY - 1) / ctbSizeY;
        unsigned int picHeightInCtbsY = (sps.mPicHeightInLumaSamples + ctbSizeY - 1) / ctbSizeY;
        unsigned int picSizeInCtbsY = picWidthInCtbsY * picHeightInCtbsY;
        slice.mSliceSegmentAddress = bitstr.readBits(ceilLog2(picSizeInCtbsY));
    }

    if (!slice.mDependentSliceSegmentFlag)
    {
        for (unsigned int i = 0; i < pps.mNumExtraSliceHeaderBits; ++i)
        {
            bitstr.readBits(1);  // slice_reserved_flag[i]
        }
        slice.mSliceType = SliceType(bitstr.readExpGolombCode());
        if (pps.mOutputFlagPresentFlag)
        {
            slice.mPicOutputFlag = bitstr.readBits(1);
        }
        if (sps.mSeparateColourPlaneFlag)
        {
            slice.mColourPlaneId = bitstr.readBits(2);
        }
        if (naluType != H265NalUnitType::CODED_SLICE_IDR_W_RADL && naluType != H265NalUnitType::CODED_SLICE_IDR_N_LP)
        {
            slice.mSlicePicOrderCntLsb = bitstr.readBits(sps.mLog2MaxPicOrderCntLsbMinus4 + 4);
            slice.mShortTermRefPicSetSpsFlag = bitstr.readBits(1);
            slice.mShortTermRefPicSetIdx = 0;
            if (!slice.mShortTermRefPicSetSpsFlag)
            {
                parseShortTermRefPicSet(bitstr, sps.mShortTermRefPicSetsDerived, slice.mShortTermRefPicSet, sps.mNumShortTermRefPicSets, sps.mNumShortTermRefPicSets);
                deriveRefPicSetParams(sps.mShortTermRefPicSetsDerived, slice.mShortTermRefPicSet, slice.mShortTermRefPicSetDerived, sps.mNumShortTermRefPicSets);
            }
            else if (sps.mNumShortTermRefPicSets > 1)
            {
                slice.mShortTermRefPicSetIdx = bitstr.readBits(ceilLog2(sps.mNumShortTermRefPicSets));
            }
            const unsigned int currStRpsIdx = slice.mShortTermRefPicSetSpsFlag ? slice.mShortTermRefPicSetIdx : sps.mNumShortTermRefPicSets;
            slice.mCurrStRps = (currStRpsIdx == sps.mNumShortTermRefPicSets) ? &slice.mShortTermRefPicSetDerived : &sps.mShortTermRefPicSetsDerived[currStRpsIdx];
            if (sps.mLongTermRefPicsPresentFlag)
            {
                if (sps.mNumLongTermRefPicsSps > 0)
                {
                    slice.mNumLongTermSps = bitstr.readExpGolombCode();
                }
                slice.mNumLongTermPics = bitstr.readExpGolombCode();

                for (unsigned i = 0; i < slice.mNumLongTermSps + slice.mNumLongTermPics; ++i)
                {
                    slice.mLtIdxSps.push_back(0);
                    slice.mPocLsbLtSyntax.push_back(0);
                    slice.mUsedByCurrPicLtFlag.push_back(0);
                    slice.mDeltaPocMsbPresentFlag.push_back(0);
                    slice.mDeltaPocMsbCycleLt.push_back(0);
                    if (i < slice.mNumLongTermSps)
                    {
                        if (sps.mNumLongTermRefPicsSps > 1)
                        {
                            slice.mLtIdxSps[i] = bitstr.readBits(ceilLog2(sps.mNumLongTermRefPicsSps));
                        }
                    }
                    else
                    {
                        slice.mPocLsbLtSyntax[i] = bitstr.readBits(sps.mLog2MaxPicOrderCntLsbMinus4 + 4);
                        slice.mUsedByCurrPicLtFlag[i] = bitstr.readBits(1);
                    }
                    slice.mDeltaPocMsbPresentFlag[i] = bitstr.readBits(1);
                    if (slice.mDeltaPocMsbPresentFlag[i])
                    {
                        slice.mDeltaPocMsbCycleLt[i] = bitstr.readExpGolombCode();
                    }

                    if (i < slice.mNumLongTermSps)
                    {
                        slice.mPocLsbLt.push_back(sps.mLtRefPicPocLsbSps[slice.mLtIdxSps[i]]);
                        slice.mUsedByCurrPicLt.push_back(sps.mUsedByCurrPicLtSpsFlag[slice.mLtIdxSps[i]]);
                    }
                    else
                    {
                        slice.mPocLsbLt.push_back(slice.mPocLsbLtSyntax[i]);
                        slice.mUsedByCurrPicLt.push_back(slice.mUsedByCurrPicLtFlag[i]);
                    }
                }
            }
            if (sps.mSpsTemporalMvpEnabledFlag)
            {
                slice.mSliceTemporalMvpEnabledFlag = bitstr.readBits(1);
            }
        }
        if (sps.mSampleAdaptiveOffsetEnabledFlag)
        {
            slice.mSliceSaoLumaFlag = bitstr.readBits(1);
            slice.mSliceSaoChromaFlag = bitstr.readBits(1);
        }

        if (slice.mSliceType == SliceType::P || slice.mSliceType == SliceType::B)
        {
            slice.mNumPocTotalCurr = 0;
            for (unsigned int i = 0; i < slice.mCurrStRps->mNumNegativePics; ++i)
            {
                if (slice.mCurrStRps->mUsedByCurrPicS0[i])
                {
                    slice.mNumPocTotalCurr++;
                }
            }
            for (unsigned int i = 0; i < slice.mCurrStRps->mNumPositivePics; ++i)
            {
                if (slice.mCurrStRps->mUsedByCurrPicS1[i])
                {
                    slice.mNumPocTotalCurr++;
                }
            }
            for (unsigned int i = 0; i < slice.mNumLongTermSps + slice.mNumLongTermPics; ++i)
            {
                if (slice.mUsedByCurrPicLt[i])
                {
                    slice.mNumPocTotalCurr++;
                }
            }

            slice.mNumRefIdxActiveOverrideFlag = bitstr.readBits(1);
            if (slice.mNumRefIdxActiveOverrideFlag)
            {
                slice.mNumRefIdxL0ActiveMinus1 = bitstr.readExpGolombCode();
                if (slice.mSliceType == SliceType::B)
                {
                    slice.mNumRefIdxL1ActiveMinus1 = bitstr.readExpGolombCode();
                }
            }
            if (pps.mListsModificationPresentFlag && slice.mNumPocTotalCurr > 1)
            {
                parseRefPicListsModification(bitstr, slice, slice.mRefPicListsModification);
            }
            if (slice.mSliceType == SliceType::B)
            {
                slice.mMvdL1ZeroFlag = bitstr.readBits(1);
            }
            if (pps.mCabacInitPresentFlag)
            {
                slice.mCabacInitFlag = bitstr.readBits(1);
            }
            if (slice.mSliceTemporalMvpEnabledFlag)
            {
                if (slice.mSliceType == SliceType::B)
                {
                    slice.mCollocatedFromL0Flag = bitstr.readBits(1);
                }
                if ((slice.mCollocatedFromL0Flag && slice.mNumRefIdxL0ActiveMinus1 > 0) ||
                    (!slice.mCollocatedFromL0Flag && slice.mNumRefIdxL1ActiveMinus1 > 0))
                {
                    slice.mCollocatedRefIdx = bitstr.readExpGolombCode();
                }
            }
            if ((pps.mWeightedPredFlag && slice.mSliceType == SliceType::P) || (pps.mWeightedBipredFlag && slice.mSliceType == SliceType::B))
            {
                parsePredWeightTable(bitstr, sps, slice, slice.mPredWeightTable);
            }
            slice.mFiveMinusMaxNumMergeCand = bitstr.readExpGolombCode();
        }
        slice.mSliceQpDelta = bitstr.readSignedExpGolombCode();
        if (pps.mPpsSliceChromaQpOffsetsPresentFlag)
        {
            slice.mSliceCbQpOffset = bitstr.readSignedExpGolombCode();
            slice.mSliceCrQpOffset = bitstr.readSignedExpGolombCode();
        }
        if (pps.mDeblockingFilterOverrideEnabledFlag)
        {
            slice.mDeblockingFilterOverrideFlag = bitstr.readBits(1);
        }
        if (slice.mDeblockingFilterOverrideFlag)
        {
            slice.mSliceDeblockingFilterDisabledFlag = bitstr.readBits(1);
            if (!slice.mSliceDeblockingFilterDisabledFlag)
            {
                slice.mSliceBetaOffsetDiv2 = bitstr.readSignedExpGolombCode();
                slice.mSliceTcOffsetDiv2 = bitstr.readSignedExpGolombCode();
            }
        }
        if (pps.mPpsLoopFilterAcrossSicesEnabledFlag && (slice.mSliceSaoLumaFlag || slice.mSliceSaoChromaFlag || !slice.mSliceDeblockingFilterDisabledFlag))
        {
            slice.mSliceLoopFilterAcrossSlicesEnabledFlag = bitstr.readBits(1);
        }
    }  // dependent slice segment

    if (pps.mTilesEnabledFlag || pps.mEntropyCodingSyncEnabledFlag)
    {
        slice.mNumEntryPointOffsets = bitstr.readExpGolombCode();
        if (slice.mNumEntryPointOffsets > 0)
        {
            slice.mOffsetLenMinus1 = bitstr.readExpGolombCode();
            for (unsigned int i = 0; i < slice.mNumEntryPointOffsets; ++i)
            {
                slice.mEntryPointOffsetMinus1.push_back(bitstr.readBits(slice.mOffsetLenMinus1 + 1));
            }
        }
    }

    if (pps.mSliceSegmentHeaderExtensionPresentFlag)
    {
        unsigned int len = bitstr.readExpGolombCode();
        for (unsigned int i = 0; i < len; ++i)
        {
            bitstr.readBits(8);
        }
    }

    return 0;
}

// Read next NAL unit in bytestream format, startcode assumed in the beginning 
H265::H265NalUnitType H265Parser::readNextNalUnit(H265InputStream& aInputStream, vector<uint8_t>& nalUnit)
{
    unsigned int numZeros = 0;
    bool startCodeFound = false;
    unsigned int startCodeLen = 0;
    unsigned int nextChar;

    nextChar = aInputStream.getNextByte();
    startCodeLen++;
    while (!aInputStream.eof() && nextChar == 0)
    {
        nalUnit.push_back(nextChar);
        nextChar = aInputStream.getNextByte();
        startCodeLen++;
    }
    nalUnit.push_back((uint8_t)nextChar);

    // copy nal data to output vector while checking for the next start code
    unsigned int readBytes = 0;
    while (!startCodeFound && !aInputStream.eof())
    {
        nextChar = aInputStream.getNextByte();
        if (nextChar == 0)
        {
            numZeros++;
        }
        else if (numZeros >= 2 && nextChar == 1)
        {
            // mp4reader outputs 4-byte start codes, standard hevc streams contain 4-byte start codes for param sets, but 3-byte for pictures
            for (int i = 0; i < numZeros; i++)
            {
                nalUnit.pop_back();
            }
            aInputStream.rewind(numZeros + 1);
            startCodeFound = true;
            break;
        }
        else
        {
            numZeros = 0;
        }
        nalUnit.push_back(nextChar);
        ++readBytes;
    }

    return
        nalUnit.size()
        ? H265NalUnitType((nalUnit[startCodeLen] >> 1) & 0x3f)
        : H265NalUnitType::INVALID;  // NAL unit type
}

H265NalUnitType H265Parser::getH265NalUnitType(const vector<uint8_t>& nalUnit)
{
    int i = 0;
    // skip start code
    while (nalUnit[i] == 0)
    {
        ++i;
    }
    assert(nalUnit[i] == 0x01);
    ++i;

    return H265NalUnitType((nalUnit[i] >> 1) & 0x3f);
}

bool H265Parser::isFirstVclNaluInPic(const vector<uint8_t>& nalUnit)
{
    H265NalUnitType naluType;
    int i = 0;

    // Skip start code
    while (nalUnit[i] == 0)
    {
        ++i;
    }
    assert(nalUnit[i] == 0x01);
    ++i;

    naluType = H265NalUnitType((nalUnit[i] >> 1) & 0x3f);

    if (isVclNaluType(naluType))
    {
        return (nalUnit[i + 2] >> 7) & 1;  // first_slice_segment_in_pic_flag
    }
    else
    {
        return false;
    }
}

void H265Parser::convertToRBSP(const vector<uint8_t>& src, vector<uint8_t>& dest, bool byteStreamMode)
{
    const unsigned int numBytesInNalUnit = src.size();

    // this is a reasonable guess, as the result vector can not be larger than the original
    dest.reserve(numBytesInNalUnit);

    // find start code
    unsigned int i = 0;
    if (byteStreamMode)
    {
        while ((i < (numBytesInNalUnit - 3)) && !(src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 1))
        {
            ++i;
        }

        i += 3;  // skip start code
    }

    // copy NALU header
    static const size_t NALU_HEADER_LENGTH = 2;
    dest.insert(dest.end(), src.cbegin() + i, src.cbegin() + i + NALU_HEADER_LENGTH);
    i += NALU_HEADER_LENGTH;

    // copy rest of the data while removing start code emulation prevention bytes
    enum class State
    {
        COPY_DATA,
        ONE_ZERO_FOUND,
        TWO_ZEROS_FOUND
    };
    State state = State::COPY_DATA;
    size_t copyStartOffset = i;
    for (; i < numBytesInNalUnit; ++i)
    {
        const unsigned int byte = src[i];
        switch (state)
        {
        case State::COPY_DATA:
            if (byte != 0) state = State::COPY_DATA;
            else state = State::ONE_ZERO_FOUND;
            break;

        case State::ONE_ZERO_FOUND:
            if (byte != 0) state = State::COPY_DATA;
            else state = State::TWO_ZEROS_FOUND;
            break;

        case State::TWO_ZEROS_FOUND:
            // sequence of 0x000003 means that 0x03 is the emulation prevention byte
            if (byte == 0x03)
            {
                // skip copying 0x03
                dest.insert(dest.end(), src.cbegin() + copyStartOffset, src.cbegin() + i);
                copyStartOffset = i + 1;
                // continue byte stream copying
                state = State::COPY_DATA;
            }
            else if (byte == 0) state = State::TWO_ZEROS_FOUND;
            else state = State::COPY_DATA;
            break;
        }
    }
    dest.insert(dest.end(), src.cbegin() + copyStartOffset, src.cend());
}

void H265Parser::writeSEINalHeader(Parser::BitStream& bitstr, H265SEIType payloadType, unsigned int payloadSize, int temporalIdPlus1)
{
    // add NAL header
    bitstr.writeBits(0, 1); // forbidden zero bit
    bitstr.writeBits((unsigned int)H265NalUnitType::PREFIX_SEI, 6);
    bitstr.writeBits(0, 6);//nuh_layer_id, shall be 0
    bitstr.writeBits(temporalIdPlus1, 3); // nuh_temporal_id_plus1
    unsigned int type = (unsigned int)payloadType;
    for (; type >= 0xff; type -= 0xff)
    {
        bitstr.write8Bits(0xff);
    }
    bitstr.write8Bits(type);

    for (; payloadSize >= 0xff; payloadSize -= 0xff)
    {
        bitstr.write8Bits(0xff);
    }
    bitstr.write8Bits(payloadSize);
}

std::uint32_t H265Parser::writeEquiProjectionSEINal(Parser::BitStream& bitstr, EquirectangularProjectionSEI& projection, int temporalIdPlus1)
{
    Parser::BitStream payloadStr;
    unsigned int payloadSize = 0;
    payloadStr.writeBits(projection.erpCancel, 1);
    if (!projection.erpCancel)
    {
        payloadStr.writeBits(projection.erpPersistence, 1);
        payloadStr.writeBits(projection.erpGuardBand, 1);
        payloadStr.writeBits(0, 2);// reserved zero 2 bits
        if (projection.erpGuardBand)
        {
            payloadStr.writeBits(projection.erpGuardBandType, 3);
            payloadStr.write8Bits(projection.erpLeftGuardBandWidth);
            payloadStr.write8Bits(projection.erpRightGuardBandWidth);
            payloadSize = 3;
        }
        else
        {
            payloadSize = 1;
        }
    }
    else
    {
        payloadSize = 1;
    }

    writeSEINalHeader(bitstr, H265SEIType::EQUIRECT_PROJECTION, payloadSize, temporalIdPlus1);
    bitstr.append(payloadStr);

    writeRbspTrailingBits(bitstr);

    insertEPB(bitstr.getStorage());
    return bitstr.getSize();
}

std::uint32_t H265Parser::writeCubeProjectionSEINal(Parser::BitStream& bitstr, CubemapProjectionSEI& projection, int temporalIdPlus1)
{
    Parser::BitStream payloadStr;
    payloadStr.writeBits(projection.cmpCancel, 1);
    if (!projection.cmpCancel)
    {
        payloadStr.writeBits(projection.cmpPersistence, 1);
    }

    writeSEINalHeader(bitstr, H265SEIType::CUBEMAP_PROJECTION, 1, temporalIdPlus1);
    bitstr.append(payloadStr);

    writeRbspTrailingBits(bitstr);

    insertEPB(bitstr.getStorage());
    return bitstr.getSize();
}

std::uint32_t H265Parser::writeFramePackingSEINal(Parser::BitStream& bitstr, FramePackingSEI& packing, int temporalIdPlus1)
{
    Parser::BitStream payloadStr;
    unsigned int payloadSize = 0;
    payloadStr.writeBits(packing.fpArrangementId, 32);
    payloadStr.writeBits(packing.fpCancel, 1);
    if (!packing.fpCancel)
    {
        payloadStr.writeBits(packing.fpArrangementType, 7);
        payloadStr.writeBits(packing.quincunxSamplingFlag, 1);
        payloadStr.writeBits(packing.contentInterpretationType, 6);
        payloadStr.writeBits(packing.spatialFlipping, 1);
        payloadStr.writeBits(packing.frame0Flipped, 1);
        payloadStr.writeBits(packing.fieldViews, 1);
        payloadStr.writeBits(packing.currentFrameIsFrame0, 1);
        payloadStr.writeBits(packing.frame0SelfContained, 1);
        payloadStr.writeBits(packing.frame1SelfContained, 1);
        if (!packing.quincunxSamplingFlag && packing.fpArrangementType != 5)
        {
            payloadStr.writeBits(packing.frame0GridX, 4);
            payloadStr.writeBits(packing.frame0GridY, 4);
            payloadStr.writeBits(packing.frame1GridX, 4);
            payloadStr.writeBits(packing.frame1GridY, 4);
        }
        payloadStr.write8Bits(0); //reserved
        payloadStr.writeBits(packing.fpArrangementPersistence, 1);
    }
    payloadStr.writeBits(packing.upsampledAspectRatio, 1);

    writeSEINalHeader(bitstr, H265SEIType::FRAMEPACKING_ARRANGEMENT, 1, temporalIdPlus1);
    bitstr.append(payloadStr);

    writeRbspTrailingBits(bitstr);

    insertEPB(bitstr.getStorage());
    return bitstr.getSize();
}

std::uint32_t H265Parser::writeRwpkSEINal(Parser::BitStream& bitstr, RegionWisePackingSEI& packing, int temporalIdPlus1)
{
    Parser::BitStream payloadStr;
    unsigned int payloadSize = 0;
    payloadStr.writeBits(packing.rwpCancel, 1);
    if (!packing.rwpCancel)
    {
        payloadStr.writeBits(packing.rwpPersistence, 1);
        payloadStr.writeBits(packing.constituentPictureMatching, 1);
        payloadStr.writeBits(0, 5);//reserved_zero_5bits

        payloadStr.write8Bits(packing.regions.size());
        payloadStr.write32Bits(packing.projPictureWidth);
        payloadStr.write32Bits(packing.projPictureHeight);
        payloadStr.write16Bits(packing.packedPictureWidth);
        payloadStr.write16Bits(packing.packedPictureHeight);
        payloadSize = 14;
        for (auto& region : packing.regions)
        {
            payloadStr.writeBits(0, 4); //reserved_zero_4bits
            payloadStr.writeBits(region.rwpTransformType, 3);
            payloadStr.writeBits(region.rwpGuardBand, 1);

            payloadStr.write32Bits(region.projRegionWidth);
            payloadStr.write32Bits(region.projRegionHeight);
            payloadStr.write32Bits(region.projRegionTop);
            payloadStr.write32Bits(region.projRegionLeft);
            payloadStr.write32Bits(region.projRegionWidth);
            payloadStr.write16Bits(region.packedRegionWidth);
            payloadStr.write16Bits(region.packedRegionHeight);
            payloadStr.write16Bits(region.packedRegionTop);
            payloadStr.write16Bits(region.packedRegionLeft);
            payloadSize += 29;
            if (region.rwpGuardBand)
            {
                payloadStr.write8Bits(region.leftGbWidth);
                payloadStr.write8Bits(region.rightGbWidth);
                payloadStr.write8Bits(region.topGbHeight);
                payloadStr.write8Bits(region.bottomGbHeight);

                payloadStr.writeBits(region.gbNotUsedForPredFlag, 1);
                payloadStr.writeBits(region.gbType0, 3);
                payloadStr.writeBits(region.gbType1, 3);
                payloadStr.writeBits(region.gbType2, 3);
                payloadStr.writeBits(region.gbType3, 3);
                payloadStr.writeBits(0, 3); // reserved_zero_3bits
                payloadSize += 6;
            }
        }
    }
    else
    {
        payloadSize = 1;
    }

    writeSEINalHeader(bitstr, H265SEIType::REGIONWISE_PACKING, payloadSize, temporalIdPlus1);
    bitstr.append(payloadStr);

    writeRbspTrailingBits(bitstr);

    insertEPB(bitstr.getStorage());
    return bitstr.getSize();
}

std::uint32_t H265Parser::writeRotationSEINal(Parser::BitStream& bitstr, SphereRotationSEI& rotation, int temporalIdPlus1)
{
    Parser::BitStream payloadStr;
    unsigned int payloadSize = 0;
    payloadStr.writeBits(rotation.sphereRotationCancel, 1);
    if (!rotation.sphereRotationCancel)
    {
        payloadStr.writeBits(rotation.sphereRotationPersistence, 1);
        payloadStr.writeBits(0, 6); // reserved_zero_6bits

        payloadStr.write32Bits(rotation.yawRotation);
        payloadStr.write32Bits(rotation.pitchRotation);
        payloadStr.write32Bits(rotation.rollRotation);
        payloadSize = 13;
    }
    else
    {
        payloadSize = 1;
    }

    writeSEINalHeader(bitstr, H265SEIType::SPHERE_ROTATION, payloadSize, temporalIdPlus1);
    bitstr.append(payloadStr);

    writeRbspTrailingBits(bitstr);

    insertEPB(bitstr.getStorage());
    return bitstr.getSize();
}

void H265Parser::insertEPB(std::vector<uint8_t>& payload)
{
    uint8_t numZeros = 0;
    for (size_t i = 5; i != payload.size(); ++i)
    {
        if (payload.at(i) == 0)
        {
            numZeros += 1;
        }
        else
        {
            numZeros = 0;
        }

        if (numZeros == 2 && payload.at(i + 1) == 0)
        {
            payload.insert(payload.begin() + i + 1, 3);
            numZeros = 0;
        }
    }
}
