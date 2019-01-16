
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
#include "controllerbase.h"

namespace VDD
{

    namespace PipelineModeDefs
    {
        const auto kPipelineModeTypeToName = Utils::reverseMapping(kPipelineModeNameToType);
    }

    std::string nameOfPipelineMode(PipelineMode aOutput)
    {
        return PipelineModeDefs::kPipelineModeTypeToName.at(aOutput);
    }

    std::string filenameComponentForPipelineOutput(PipelineOutput aOutput)
    {
        return PipelineModeDefs::kOutputTypeInfo.at(aOutput).filenameComponent;
    }

    std::string outputNameForPipelineOutput(PipelineOutput aOutput)
    {
        return PipelineModeDefs::kOutputTypeInfo.at(aOutput).outputName;
    }




    ControllerBase::ControllerBase()
	    : mIsStereo(false)
        , mInputVideoMode(VideoInputMode::Mono)
    {

    }

    ControllerBase::~ControllerBase()
    {

    }


}
