
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

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRFixedArray.h"


#include <IMPD.h>

#include "Provider/NVRCoreProviderSources.h"

#include "Foundation/NVRArray.h"
#include "Foundation/NVRFixedString.h"

OMAF_NS_BEGIN

    namespace DashStreamType
    {
        enum Enum
        {
            INVALID = -1,

            STATIC,
            DYNAMIC,

            COUNT
        };
    }

    struct DashComponents
    {
        DashComponents() :mpd(OMAF_NULL),period(OMAF_NULL),adaptationSet(OMAF_NULL),representation(OMAF_NULL),segmentTemplate(OMAF_NULL) {}
        dash::mpd::IMPD *mpd;
        dash::mpd::IPeriod *period;
        dash::mpd::IAdaptationSet *adaptationSet;
        dash::mpd::IRepresentation *representation;
        dash::mpd::ISegmentTemplate *segmentTemplate;
        BasicSourceInfo basicSourceInfo;
    };

    struct DashTimelineSegment
    {
        uint64_t segmentStartTime;
        uint64_t segmentDuration;
    };

    class DashUtils
    {
    public:
        DashUtils();
        ~DashUtils();

        static std::vector<dash::mpd::IBaseUrl *> ResolveBaseUrl(DashComponents dashComponents);

        static DashStreamType::Enum getDashStreamType(dash::mpd::IMPD *mpd);

        static void_t parseSegmentStartTimes(Array<DashTimelineSegment>& startTimes, DashComponents dashComponents);


        static uint32_t elementHasAttribute(const dash::mpd::IMPDElement* element, const char* attribute);

        static const std::string& getRawAttribute(const dash::mpd::IMPDElement* element, const char* attribute);
        static const char_t* parseRawAttribute(const dash::mpd::IMPDElement* element, const char* attribute);

        //Following functions handle the segmentTemplate value inheritance...
        static uint64_t getTimescale(DashComponents dashComponents);
        static uint64_t getDuration(DashComponents dashComponents);
        static uint64_t getStartNumber(DashComponents dashComponents);

        static const std::string& getMimeType(DashComponents dashComponents);

        static uint64_t getStreamDurationInMilliSeconds(DashComponents dashComponents);
        static uint32_t getMPDUpdateInMilliseconds(DashComponents dashComponents);
        static uint32_t getPeriodStartTimeMS(DashComponents dashComponents);

    private:
        static uint64_t parseMillisecondsFromString(const std::string&  stringTime);

    };
OMAF_NS_END
