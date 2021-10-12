
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
#include "dash.h"

#include "codecinfo.h"
#include "common.h"
#include "configreader.h"
#include "controllerops.h"
#include "log/log.h"

namespace VDD
{
    namespace
    {
        const std::map<std::string, DashProfile> kNameToDashProfile{
            {"live", DashProfile::Live}, {"on_demand", DashProfile::OnDemand}};

        const std::function<DashProfile(const ConfigValue&)> readDashProfile =
            readMapping("DashProfile", kNameToDashProfile);

        const std::map<std::string, OutputMode> kNameToOutputMode{{"none", OutputMode::None},
                                                                  {"omafv1", OutputMode::OMAFV1},
                                                                  {"omafv2", OutputMode::OMAFV2}};

        const std::function<OutputMode(const ConfigValue&)> readOutputMode =
            readMapping("OutputMode", kNameToOutputMode);
    }  // namespace

    Optional<StreamSegmenter::MPDTree::OmafProjectionFormat>
    mpdProjectionFormatOfOmafProjectionType(Optional<OmafProjectionType> aProjection)
    {
        if (aProjection)
        {
            switch (*aProjection)
            {
            case VDD::OmafProjectionType::Cubemap:
            {
                StreamSegmenter::MPDTree::OmafProjectionFormat p{};
                p.projectionType.clear();
                p.projectionType.push_back(StreamSegmenter::MPDTree::OmafProjectionType::Cubemap);
                return p;
            }
            case VDD::OmafProjectionType::Equirectangular:
            {
                StreamSegmenter::MPDTree::OmafProjectionFormat p{};
                p.projectionType.clear();
                p.projectionType.push_back(
                    StreamSegmenter::MPDTree::OmafProjectionType::Equirectangular);
                return p;
            }
            }
        }
        else
        {
            return {};
        }
        assert(false);
        return {};
    }

    Dash::Dash(std::shared_ptr<Log> aLog, const ConfigValue& aDashConfig,
               ControllerOps& aControllerOps)
        : mOps(aControllerOps)
        , mLog(aLog)
        , mDashConfig(aDashConfig)
        , mOverrideTotalDuration(VDD::readOptional(readSegmentDuration)(
              mDashConfig.tryTraverse(configPathOfString("mpd.total_duration"))))
        , mCompactNumbering(optionWithDefault(mDashConfig, "compact_numbering", readBool, true))
        , mBaseDirectory(optionWithDefault(mDashConfig, "output_directory_base", readString, ""))
        , mDashProfile(optionWithDefault(mDashConfig, "profile", readDashProfile, DashProfile::Live))
        , mOutputMode(optionWithDefault(mDashConfig, "output_mode", readOutputMode, OutputMode::OMAFV1))
    {
        if (mCompactNumbering)
        {
            mNextId = 1;
        }
        else
        {
            mNextId = 1000;
        }
        assert(mDashConfig);
    }

    std::string Dash::getBaseName() const
    {
        assert(mBaseName);
        return *mBaseName;
    }

    void Dash::setBaseName(std::string aBaseName)
    {
        mBaseName = aBaseName;
    }

    Dash::~Dash() = default;

    std::string Dash::getBaseDirectory() const
    {
        return mBaseDirectory;
    }

    void Dash::hookSegmentSaverSignal(const DashSegmenterConfig& aDashOutput, AsyncProcessor* aNode)
    {
        // optional because currently we're not using views (omafvi)
        Optional<ViewId> viewId = aDashOutput.viewId;
        auto& segmentSaverSignals = mSegmentSavedSignals[viewId];
        if (!segmentSaverSignals.count(aDashOutput.segmenterName))
        {
            SegmentSaverSignal signal;
            CombineNode::Config config{};
            config.labelPrefix = "SegmentSaverHook";
            signal.combineSignals = Utils::make_unique<CombineNode>(mOps.getGraph(), config);
            segmentSaverSignals.insert(std::make_pair(aDashOutput.segmenterName, std::move(signal)));
        }
        auto& savedSignal = segmentSaverSignals.at(aDashOutput.segmenterName);

        size_t sinkIndex = savedSignal.sinkCount++;
        mSinkIndexToRepresentationId[viewId][sinkIndex] = aDashOutput.representationId;

        connect(*aNode, *mOps.configureForGraph(savedSignal.combineSignals->getSink(sinkIndex))).ASYNC_HERE;
    }

    ConfigValue Dash::dashConfigFor(std::string aDashName) const
    {
        return mDashConfig[aDashName];
    }

    StreamId Dash::newId(IdKind aKind)
    {
        if (!mCompactNumbering && Optional<IdKind>(aKind) != mPrevIdKind)
        {
            mPrevIdKind = aKind;
            mNextId = mNextId.get() + 1000 - mNextId.get() % 1000;
        }
        return mNextId++;
    }

    void Dash::createHevcCodecsString(const std::vector<uint8_t>& aSps, std::string& aCodec)
    {
        HevcCodecInfo hevcCodecInfo = getHevcCodecInfo(aSps);
        aCodec += getCodecInfoString(hevcCodecInfo);
    }

    void Dash::createAvcCodecsString(const std::vector<uint8_t>& aSps, std::string& aCodec)
    {
        static constexpr char hex[] = "0123456789ABCDEF";
        size_t i = 0;
        if (aSps[0] == 0 && aSps[1] == 0 && aSps[2] == 0 && aSps[3] == 1)
        {
            i += 4;
        }
        i++;
        // skip start code (4 bytes) + first byte (forbidden_zero_bit + nal_ref_idc + nal_unit_type)
        // no EPB can take place here, as profile_idc > 0 and level_idc > 0
        for (int j = 0; j < 3; ++i, j++)
        {// copy profile_idc, constraint_set_flags and level_idc (3 bytes in total)
            aCodec.push_back(hex[aSps.at(i) / 16]);
            aCodec.push_back(hex[aSps.at(i) % 16]);
        }
    }

    const ConfigValue& Dash::getConfig() const
    {
        return mDashConfig;
    }

    /** @brief Retrieve the curren opreating mode set by the "mode" field */
    OutputMode Dash::getOutputMode() const
    {
        return mOutputMode;
    }

    StreamSegmenter::Segmenter::ImdaId Dash::newImdaId(PipelineOutput /*aOutput*/)
    {
        std::unique_lock<std::mutex> lock(mImdaIdMutex);
        assert(false);
        return mImdaId++;
    }

    DashProfile Dash::getProfile() const
    {
        return mDashProfile;
    }

    std::string Dash::getDefaultPeriodId() const
    {
        return "1";
    }
}  // namespace VDD
