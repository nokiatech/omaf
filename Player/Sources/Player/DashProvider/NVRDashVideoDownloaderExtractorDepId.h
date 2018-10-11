
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
#pragma once

#include "NVREssentials.h"
#include "DashProvider/NVRDashVideoDownloaderExtractor.h"


OMAF_NS_BEGIN

    class DashVideoDownloaderExtractorDepId : public DashVideoDownloaderExtractor
    {

    public:

        DashVideoDownloaderExtractorDepId();
        virtual ~DashVideoDownloaderExtractorDepId();

    public:

        virtual Error::Enum completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo);

    protected:

        virtual void_t checkVASVideoStreams(uint64_t currentPTS);

    };
OMAF_NS_END
