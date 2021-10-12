
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
#pragma once

#include <vector>
#include <array>
#include <memory>

namespace H265
{
    struct ScalingListData
    {
        std::vector<unsigned int> mScalingListPredModeFlag[4];
        std::vector<unsigned int> mScalingListPredMatrixIdDelta[4];
        std::vector<int> mScalingListDcCoefMinus8[2];
        std::vector<std::vector<unsigned int>> mScalingList[4];
    };
    struct PictureParameterSet
    {
        unsigned int mPpsId;
        unsigned int mSpsId;
        unsigned int mDependentSliceSegmentsEnabledFlag;
        unsigned int mOutputFlagPresentFlag;
        unsigned int mNumExtraSliceHeaderBits;
        unsigned int mSignDataHidingEnabledFlag;
        unsigned int mCabacInitPresentFlag;
        unsigned int mNumRefIdxL0DefaultActiveMinus1;
        unsigned int mNumRefIdxL1DefaultActiveMinus1;
        int mInitQpMinus26;
        unsigned int mConstrainedIntraPredFlag;
        unsigned int mTransformSkipEnabledFlag;
        unsigned int mCuQpDeltaEnabledFlag;
        unsigned int mDiffCuQpDeltaDepth;
        int mPpsCbQpOffset;
        int mPpsCrQpOffset;
        unsigned int mPpsSliceChromaQpOffsetsPresentFlag;
        unsigned int mWeightedPredFlag;
        unsigned int mWeightedBipredFlag;
        unsigned int mTransquantBypassEnabledFlag;
        unsigned int mTilesEnabledFlag;
        unsigned int mEntropyCodingSyncEnabledFlag;

        unsigned int mNumTileColumnsMinus1;
        unsigned int mNumTileRowsMinus1;
        unsigned int mUniformSpacingFlag;
        std::vector<unsigned int> mColumnWidthMinus1;
        std::vector<unsigned int> mRowHeightMinus1;
        unsigned int mLoopFilterAcrossTilesEnabledFlag;

        unsigned int mPpsLoopFilterAcrossSicesEnabledFlag;
        unsigned int mDeblockingFilterControlPresentFlag;
        unsigned int mDeblockingFilterOverrideEnabledFlag;
        unsigned int mPpsDeblockingFilterDisabledFlag;
        int mPpsBetaOffsetDiv2;
        int mPpsTcOffsetDiv2;

        unsigned int mPpsScalingListDataPresentFlag;
        ScalingListData mScalingListData;

        unsigned int mListsModificationPresentFlag;
        unsigned int mLog2ParallelMergeLevelMinus2;
        unsigned int mSliceSegmentHeaderExtensionPresentFlag;
        unsigned int mPpsExtensionFlag;
        unsigned int mPpsExtensionDataFlag;
    };

    enum class H265NalUnitType : uint8_t
    {
        CODED_SLICE_TRAIL_N = 0, // 0
        CODED_SLICE_TRAIL_R,     // 1

        CODED_SLICE_TSA_N,       // 2
        CODED_SLICE_TSA_R,       // 3

        CODED_SLICE_STSA_N,      // 4
        CODED_SLICE_STSA_R,      // 5

        CODED_SLICE_RADL_N,      // 6
        CODED_SLICE_RADL_R,      // 7

        CODED_SLICE_RASL_N,      // 8
        CODED_SLICE_RASL_R,      // 9

        RESERVED_VCL_N10,
        RESERVED_VCL_R11,
        RESERVED_VCL_N12,
        RESERVED_VCL_R13,
        RESERVED_VCL_N14,
        RESERVED_VCL_R15,

        CODED_SLICE_BLA_W_LP,    // 16
        CODED_SLICE_BLA_W_RADL,  // 17
        CODED_SLICE_BLA_N_LP,    // 18
        CODED_SLICE_IDR_W_RADL,  // 19
        CODED_SLICE_IDR_N_LP,    // 20
        CODED_SLICE_CRA,         // 21
        RESERVED_IRAP_VCL22,
        RESERVED_IRAP_VCL23,

        RESERVED_VCL24,
        RESERVED_VCL25,
        RESERVED_VCL26,
        RESERVED_VCL27,
        RESERVED_VCL28,
        RESERVED_VCL29,
        RESERVED_VCL30,
        RESERVED_VCL31,
        VPS,                     // 32
        SPS,                     // 33
        PPS,                     // 34
        ACCESS_UNIT_DELIMITER,   // 35
        EOS,                     // 36
        EOB,                     // 37
        FILLER_DATA,             // 38
        PREFIX_SEI,              // 39
        SUFFIX_SEI,              // 40
        RESERVED_NVCL41,
        RESERVED_NVCL42,
        RESERVED_NVCL43,
        RESERVED_NVCL44,
        RESERVED_NVCL45,
        RESERVED_NVCL46,
        RESERVED_NVCL47,
        UNSPECIFIED_48,
        UNSPECIFIED_49,
        UNSPECIFIED_50,
        UNSPECIFIED_51,
        UNSPECIFIED_52,
        UNSPECIFIED_53,
        UNSPECIFIED_54,
        UNSPECIFIED_55,
        UNSPECIFIED_56,
        UNSPECIFIED_57,
        UNSPECIFIED_58,
        UNSPECIFIED_59,
        UNSPECIFIED_60,
        UNSPECIFIED_61,
        UNSPECIFIED_62,
        UNSPECIFIED_63,
        INVALID
    };
    struct NalUnitHeader
    {
        H265NalUnitType mH265NalUnitType;
        unsigned int mNuhLayerId;
        unsigned int mNuhTemporalIdPlus1;
    };

    enum class SliceType
    {
        B = 0,
        P,
        I
    };

    struct Picture
    {
        H265NalUnitType mH265NalUnitType;
        int mIndex;               // decode order
        int mPoc;                 // display order
        unsigned int mWidth;
        unsigned int mHeight;
        unsigned int mSlicePicOrderCntLsb;
        bool mIsReferecePic;
        bool mIsLongTermRefPic;
        bool mPicOutputFlag;
    };

    struct SubLayerProfileTierLevel
    {
        unsigned int mSubLayerProfileSpace;
        unsigned int mSubLayerTierFlag;
        unsigned int mSubLayerProfileIdc;
        unsigned int mSubLayerProfileCompatibilityFlag[32];
        unsigned int mSubLayerProgressiveSourceFlag;
        unsigned int mSubLayerInterlacedSourceFlag;
        unsigned int mSubLayerNonPackedConstraintFlag;
        unsigned int mSubLayerFrameOnlyConstraintFlag;
        unsigned int mSubLayerLevelIdc;
    };

    struct ProfileTierLevel
    {
        unsigned int mGeneralProfileSpace;
        unsigned int mGeneralTierFlag;
        unsigned int mGeneralProfileIdc;
        unsigned int mGeneralProfileCompatibilityFlag[32];
        unsigned int mGeneralProgressiveSourceFlag;
        unsigned int mGeneralInterlacedSourceFlag;
        unsigned int mGeneralNonPackedConstraintFlag;
        unsigned int mGeneralFrameOnlyConstraintFlag;
        unsigned int mReserved_22bits_1; // for preserving data on read/write cycle
        unsigned int mReserved_22bits_2; // 22 bits
        unsigned int mGeneralLevelIdc;
        std::vector<unsigned int> mSubLayerProfilePresentFlag;
        std::vector<unsigned int> mSubLayerLevelPresentFlag;
        std::vector<SubLayerProfileTierLevel> mSubLayerProfileTierLevels;
    };

    struct ShortTermRefPicSet
    {
        unsigned int mInterRefPicSetPredictionFlag;
        unsigned int mDeltaIdxMinus1;
        unsigned int mDeltaRpsSign;
        unsigned int mAbsDeltaRpsMinus1;
        std::vector<unsigned int> mUsedByCurrPicFlag;
        std::vector<unsigned int> mUseDeltaFlag;
        unsigned int mNumNegativePics;
        unsigned int mNumPositivePics;
        std::vector<unsigned int> mDeltaPocS0Minus1;
        std::vector<unsigned int> mUsedByCurrPicS0Flag;
        std::vector<unsigned int> mDeltaPocS1Minus1;
        std::vector<unsigned int> mUsedByCurrPicS1Flag;
    };

    struct ShortTermRefPicSetDerived
    {
        unsigned int mNumNegativePics;
        unsigned int mNumPositivePics;
        std::vector<unsigned int> mUsedByCurrPicS0;
        std::vector<unsigned int> mUsedByCurrPicS1;
        std::vector<int> mDeltaPocS0;
        std::vector<int> mDeltaPocS1;
        unsigned int mNumDeltaPocs;
    };

    struct RefPicSet
    {
        std::vector<int> mPocStCurrBefore;
        std::vector<int> mPocStCurrAfter;
        std::vector<int> mPocStFoll;
        std::vector<int> mPocLtCurr;
        std::vector<int> mPocLtFoll;
        std::vector<unsigned int> mCurrDeltaPocMsbPresentFlag;
        std::vector<unsigned int> mFollDeltaPocMsbPresentFlag;
        unsigned int mNumPocStCurrBefore;
        unsigned int mNumPocStCurrAfter;
        unsigned int mNumPocStFoll;
        unsigned int mNumPocLtCurr;
        unsigned int mNumPocLtFoll;
        std::vector<Picture*> mRefPicSetStCurrBefore;
        std::vector<Picture*> mRefPicSetStCurrAfter;
        std::vector<Picture*> mRefPicSetStFoll;
        std::vector<Picture*> mRefPicSetLtCurr;
        std::vector<Picture*> mRefPicSetLtFoll;
    };

    struct RefPicListsModification
    {
        unsigned int mRefPicListModificationFlagL0;
        std::vector<unsigned int> mListEntryL0;
        unsigned int mRefPicListModificationFlagL1;
        std::vector<unsigned int> mListEntryL1;
    };

    struct PredWeightTable
    {
        unsigned int mLumaLog2WeightDenom;
        int mDeltaChromaLog2WeightDenom;
        std::vector<unsigned int> mLumaWeightL0Flag;
        std::vector<unsigned int> mChromaWeightL0Flag;
        std::vector<int> mDeltaLumaWeightL0;
        std::vector<int> mLumaOffsetL0;
        std::vector<std::array<int, 2>> mDeltaChromaWeightL0;
        std::vector<std::array<int, 2>> mDeltaChromaOffsetL0;
        std::vector<unsigned int> mLumaWeightL1Flag;
        std::vector<unsigned int> mChromaWeightL1Flag;
        std::vector<int> mDeltaLumaWeightL1;
        std::vector<int> mLumaOffsetL1;
        std::vector<std::array<int, 2>> mDeltaChromaWeightL1;
        std::vector<std::array<int, 2>> mDeltaChromaOffsetL1;

    };

    struct SubLayerHrdParameters
    {
        std::vector<unsigned int> mBitRateValueMinus1;
        std::vector<unsigned int> mCpbSizeValueMinus1;
        std::vector<unsigned int> mCpbSizeDuValueMinus1;
        std::vector<unsigned int> mBitRateDuValueMinus1;
        std::vector<unsigned int> mCbrFlag;
    };

    struct HrdParameters
    {
        unsigned int mNalHrdParameterPresentFlag;
        unsigned int mVclHrdParametersPresentFlag;
        unsigned int mSubPicHrdParamsPresentFlag;
        unsigned int mTickDivisorMinus2;
        unsigned int mDuCpbRemovalDelayIncrementLengthMinus1;
        unsigned int mSubPicCpbParamsInPicTimingSeiFlag;
        unsigned int mDpbOutputDelayDuLengthMinus1;
        unsigned int mBitRateScale;
        unsigned int mCpbSizeScale;
        unsigned int mCpbSizeDuScale;
        unsigned int mInitialCpbRemovalDelayLengthMinus1;
        unsigned int mAuCpbRemovalDelayLengthMinus1;
        unsigned int mDpbOutputDelayLengthMinus1;
        std::vector<unsigned int> mFixedPicRateGeneralFlag;
        std::vector<unsigned int> mFixedPicRateWithinCvsFlag;
        std::vector<unsigned int> mElementalDurationInTcMinus1;
        std::vector<unsigned int> mLowDelayHrdFlag;
        std::vector<unsigned int> mCpbCntMinus1;
        std::vector<SubLayerHrdParameters> mSubLayerNalHrdParams;
        std::vector<SubLayerHrdParameters> mSubLayerVclHrdParams;
    };

    struct VuiParameters
    {
        unsigned int mAspectRatioInfoPresentFlag;
        unsigned int mAspectRatioIdc;
        unsigned int mSarWidth;
        unsigned int mSarHeight;
        unsigned int mOverscanInfoPresentFlag;
        unsigned int mOverscanAppropriateFlag;
        unsigned int mVideoSignalTypePresentFlag;
        unsigned int mVideoFormat;
        unsigned int mVideoFullRangeFlag;
        unsigned int mColourDescriptionPresentFlag;
        unsigned int mCcolourPrimaries;
        unsigned int mTransferCharacteristics;
        unsigned int mMatrixCoeffs;
        unsigned int mChromaLocInfoPresentFlag;
        unsigned int mChromaSampleLocTypeTopField;
        unsigned int mChromaSampleLocTypeBottomField;
        unsigned int mNeutralChromaIndicationFlag;
        unsigned int mFieldSeqFlag;
        unsigned int mFrameFieldInfoPresentFlag;
        unsigned int mDefaultDisplayWindowFlag;
        unsigned int mDefDispWinLeftOffset;
        unsigned int mDefDispWinRightOffset;
        unsigned int mDefDispWinTopOffset;
        unsigned int mDefDispWinBottomOffset;
        unsigned int mVuiTimingInfoPresentFlag;
        unsigned int mVuiNumUnitsInTick;
        unsigned int mVuiTimeScale;
        unsigned int mVuiPocProportionalToTimingFlag;
        unsigned int mVuiNumTicksPocDiffOneMinus1;
        unsigned int mVuiHrdParametersPresentFlag;
        HrdParameters mHrdParameters;
        unsigned int mBitstreamRestrictionFlag;
        unsigned int mTilesFixedStructureFlag;
        unsigned int mMotionVectorsOverPicBoundariesFlag;
        unsigned int mRestrictedRefPicListsFlag;
        unsigned int mMinSpatialSegmentationIdc;
        unsigned int mMaxBytesPerPicDenom;
        unsigned int mMaxBitsPerMinCuDenom;
        unsigned int mLog2MaxMvLengthHorizontal;
        unsigned int mLog2MaxMvLengthVertical;
    };

    struct SequenceParameterSet
    {
        unsigned int mVpsId;
        unsigned int mSpsMaxSubLayersMinus1;
        unsigned int mSpsTemporalIdNestingFlag;
        ProfileTierLevel mProfileTierLevel;
        unsigned int mSpsId;
        unsigned int mChromaFormatIdc;
        unsigned int mSeparateColourPlaneFlag;
        unsigned int mPicWidthInLumaSamples;
        unsigned int mPicHeightInLumaSamples;
        unsigned int mConformanceWindowFlag;
        unsigned int mConfWinLeftOffset;
        unsigned int mConfWinRightOffset;
        unsigned int mConfWinTopOffset;
        unsigned int mConfWinBottomOffset;
        unsigned int mBitDepthLumaMinus8;
        unsigned int mBitDepthChromaMinus8;
        unsigned int mLog2MaxPicOrderCntLsbMinus4;
        unsigned int mSpsSubLayerOrderingInfoPresentFlag;
        std::vector<unsigned int> mSpsMaxDecPicBufferingMinus1;
        std::vector<unsigned int> mSpsMaxNumReorderPics;
        std::vector<unsigned int> mSpsMaxLatencyIncreasePlus1;
        unsigned int mLog2MinLumaCodingBlockSizeMinus3;
        unsigned int mLog2DiffMaxMinLumaCodingBlockSize;
        unsigned int mLog2MinTransformBlockSizeMinus2;
        unsigned int mLog2DiffMaxMinTransformBlockSize;
        unsigned int mMaxTransformHierarchyDepthInter;
        unsigned int mMaxTransformHierarchyDepthIntra;
        unsigned int mScalingListEenabledFlag;
        unsigned int mSpsScalingListDataPresentFlag;
        ScalingListData mScalingListData;
        unsigned int mAmpEnabledFlag;
        unsigned int mSampleAdaptiveOffsetEnabledFlag;
        unsigned int mPcmEnabledFlag;
        unsigned int mPcmSampleBitDepthLumaMinus1;
        unsigned int mPcmSampleBitDepthChromaMinus1;
        unsigned int mLog2MinPcmLumaCodingBlockSizeMinus3;
        unsigned int mLog2DiffMaxMinPcmLumaCodingBlockSize;
        unsigned int mPcmLoopFilterDisabledFlag;
        unsigned int mNumShortTermRefPicSets;
        std::vector<ShortTermRefPicSet> mShortTermRefPicSets;
        std::vector<ShortTermRefPicSetDerived> mShortTermRefPicSetsDerived;
        unsigned int mLongTermRefPicsPresentFlag;
        unsigned int mNumLongTermRefPicsSps;
        std::vector<unsigned int> mLtRefPicPocLsbSps;
        std::vector<unsigned int> mUsedByCurrPicLtSpsFlag;
        unsigned int mSpsTemporalMvpEnabledFlag;
        unsigned int mStrongIntraSmoothingEnabledFlag;
        unsigned int mVuiParametersPresentFlag;
        VuiParameters mVuiParameters;
        unsigned int mSpsExtensionFlag;
    };

    struct SliceHeader
    {
        H265NalUnitType mNaluType;
        unsigned int mFirstSliceSegmentInPicFlag;
        unsigned int mNoOutputOfPriorPicsFlag;
        unsigned int mPpsId;
        unsigned int mDependentSliceSegmentFlag;
        unsigned int mSliceSegmentAddress;
        SliceType mSliceType;
        unsigned int mPicOutputFlag;
        unsigned int mColourPlaneId;
        unsigned int mSlicePicOrderCntLsb;
        unsigned int mShortTermRefPicSetSpsFlag;
        ShortTermRefPicSet mShortTermRefPicSet;
        ShortTermRefPicSetDerived mShortTermRefPicSetDerived;
        unsigned int mShortTermRefPicSetIdx;
        const ShortTermRefPicSetDerived* mCurrStRps;
        unsigned int mNumPocTotalCurr;

        unsigned int mNumLongTermSps;
        unsigned int mNumLongTermPics;

        std::vector<unsigned int> mLtIdxSps;
        std::vector<unsigned int> mPocLsbLtSyntax;
        std::vector<unsigned int> mUsedByCurrPicLtFlag;
        std::vector<unsigned int> mDeltaPocMsbPresentFlag;
        std::vector<unsigned int> mDeltaPocMsbCycleLt;

        unsigned int mSliceTemporalMvpEnabledFlag;

        unsigned int mSliceSaoLumaFlag;
        unsigned int mSliceSaoChromaFlag;
        unsigned int mNumRefIdxActiveOverrideFlag;
        unsigned int mNumRefIdxL0ActiveMinus1;
        unsigned int mNumRefIdxL1ActiveMinus1;
        RefPicListsModification mRefPicListsModification;
        unsigned int mMvdL1ZeroFlag;
        unsigned int mCabacInitFlag;
        unsigned int mCollocatedFromL0Flag;
        unsigned int mCollocatedRefIdx;
        PredWeightTable mPredWeightTable;
        unsigned int mFiveMinusMaxNumMergeCand;
        int mSliceQpDelta;
        int mSliceCbQpOffset;
        int mSliceCrQpOffset;
        unsigned int mDeblockingFilterOverrideFlag;
        unsigned int mSliceDeblockingFilterDisabledFlag;
        int mSliceBetaOffsetDiv2;
        int mSliceTcOffsetDiv2;
        unsigned int mSliceLoopFilterAcrossSlicesEnabledFlag;

        unsigned int mNumEntryPointOffsets;
        unsigned int mOffsetLenMinus1;
        std::vector<unsigned int> mEntryPointOffsetMinus1;

        std::vector<unsigned int> mPocLsbLt;
        std::vector<unsigned int> mUsedByCurrPicLt;

        const SequenceParameterSet* mSps;
        const PictureParameterSet*  mPps;
    };
    struct sliceHeaderOffset
    {
        unsigned int byteOffset;
        unsigned int bitOffset;
    };
    enum class H265SEIType : unsigned int
    {
        FRAMEPACKING_ARRANGEMENT = 45,
        EQUIRECT_PROJECTION = 150,
        CUBEMAP_PROJECTION,
        SPHERE_ROTATION,
        REGIONWISE_PACKING
    };
    struct EquirectangularProjectionSEI
    {
        bool erpCancel;
        bool erpPersistence;
        bool erpGuardBand;
        std::uint8_t erpGuardBandType;
        std::uint8_t erpLeftGuardBandWidth;
        std::uint8_t erpRightGuardBandWidth;
    };
    struct CubemapProjectionSEI
    {
        bool cmpCancel;
        bool cmpPersistence;
    };
    struct FramePackingSEI
    {
        std::uint32_t fpArrangementId;
        bool fpCancel;
        std::uint8_t fpArrangementType;
        bool quincunxSamplingFlag;
        std::uint8_t contentInterpretationType;
        bool spatialFlipping;
        bool frame0Flipped;
        bool fieldViews;
        bool currentFrameIsFrame0;
        bool frame0SelfContained;
        bool frame1SelfContained;
        std::uint8_t frame0GridX;
        std::uint8_t frame0GridY;
        std::uint8_t frame1GridX;
        std::uint8_t frame1GridY;
        bool fpArrangementPersistence;
        bool upsampledAspectRatio;
    };
    struct SphereRotationSEI
    {
        bool sphereRotationCancel;
        bool sphereRotationPersistence;
        std::int32_t yawRotation;
        std::int32_t pitchRotation;
        std::int32_t rollRotation;
    };
    struct RegionStruct
    {
        std::uint8_t rwpReserved4Bits;
        std::uint8_t rwpTransformType;
        bool rwpGuardBand;
        std::uint32_t projRegionWidth;
        std::uint32_t projRegionHeight;
        std::uint32_t projRegionTop;
        std::uint32_t projRegionLeft;
        std::uint16_t packedRegionWidth;
        std::uint16_t packedRegionHeight;
        std::uint16_t packedRegionTop;
        std::uint16_t packedRegionLeft;

        std::uint8_t leftGbWidth;
        std::uint8_t rightGbWidth;
        std::uint8_t topGbHeight;
        std::uint8_t bottomGbHeight;
        bool gbNotUsedForPredFlag;
        uint8_t gbType0;
        uint8_t gbType1;
        uint8_t gbType2;
        uint8_t gbType3;
        //+  reserved 3 bits
    };
    struct RegionWisePackingSEI
    {
        bool rwpCancel;
        bool rwpPersistence;
        bool constituentPictureMatching;
        std::uint32_t projPictureWidth;
        std::uint32_t projPictureHeight;
        std::uint16_t packedPictureWidth;
        std::uint16_t packedPictureHeight;
        std::vector<RegionStruct> regions;
    };

}
