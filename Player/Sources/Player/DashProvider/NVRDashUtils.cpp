
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
#include "DashProvider/NVRDashUtils.h"
#include "Foundation/NVRArray.h"
#include "Foundation/NVRFixedString.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRStringUtilities.h"
#include "IMPD.h"
#include "NVRMPDExtension.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashUtils);

static const FixedString16 STATIC_STREAM_TYPE = "static";
static const FixedString16 DYNAMIC_STREAM_TYPE = "dynamic";


DashUtils::DashUtils()
{
}

DashUtils::~DashUtils()
{
}

DashStreamType::Enum DashUtils::getDashStreamType(dash::mpd::IMPD* mpd)
{
    if (STATIC_STREAM_TYPE.compare(mpd->GetType().c_str()) == ComparisonResult::EQUAL)
    {
        return DashStreamType::STATIC;
    }
    else if (DYNAMIC_STREAM_TYPE.compare(mpd->GetType().c_str()) == ComparisonResult::EQUAL)
    {
        return DashStreamType::DYNAMIC;
    }
    else
    {
        OMAF_LOG_W("No information whether the stream is dynamic or static so defaulting to dynamic");
        return DashStreamType::DYNAMIC;
    }
}

DashProfile::Enum DashUtils::getDashProfile(dash::mpd::IMPD* mpd)
{
    const std::vector<std::string>& profiles = mpd->GetProfiles();
    for (int i = 0; i < profiles.size(); i++)
    {
        if (profiles[i] == "urn:mpeg:dash:profile:isoff-live:2011")
        {
            return DashProfile::LIVE;
        }
        else if (profiles[i] == "urn:mpeg:dash:profile:isoff-on-demand:2011")
		{
            return DashProfile::ONDEMAND;
        }
        else if (profiles[i] == "urn:mpeg:dash:profile:isoff-main:2011" ||
                 profiles[i] == "urn:mpeg:dash:profile:full:2011")
        {
            return DashProfile::MAIN_FULL;
        }
        else if (profiles[i] == "urn:mpeg:mpegI:omaf:dash:profile:indexed-isobmff:2020")
        {
			return DashProfile::INDEXED_TILING;
        }
        else
		{
        
			OMAF_LOG_D("Skipping unsupported profile:%s", profiles[i].c_str());
            continue;
        }
    }
    return DashProfile::INVALID;
}

std::vector<dash::mpd::IBaseUrl*> DashUtils::ResolveBaseUrl(DashComponents dashComponents)
{
    std::vector<dash::mpd::IBaseUrl*> urls;
    urls.push_back(dashComponents.mpd->GetMPDPathBaseUrl());
    if (dashComponents.mpd->GetBaseUrls().size() > 0)
    {
        urls.push_back(dashComponents.mpd->GetBaseUrls().at(0));
    }
    if (dashComponents.period->GetBaseURLs().size() > 0)
    {
        urls.push_back(dashComponents.period->GetBaseURLs().at(0));
    }
    if (dashComponents.adaptationSet->GetBaseURLs().size() > 0)
    {
        urls.push_back(dashComponents.adaptationSet->GetBaseURLs().at(0));
    }
    if (dashComponents.representation->GetBaseURLs().size() > 0)
    {
        urls.push_back(dashComponents.representation->GetBaseURLs().at(0));
    }
    return urls;
}

uint32_t DashUtils::elementHasAttribute(const dash::mpd::IMPDElement* element, const char* attribute)
{
    // NOTE: use of uint32_t as return value is a workaround for someone doing something to bool_t. use of bool_t
    // breaks. NOTE2: we need to access the rawattributes for the MPDElement to check if attribute was set or not, this
    // is a MUST for handling attribute inheritance. since libdash does not do that.
    const std::map<std::string, std::string>& attrs = element->GetRawAttributes();
    if (attrs.find(attribute) == attrs.end())
    {
        // not found
        return 0;
    }
    return 1;
}

const std::string& DashUtils::getRawAttribute(const dash::mpd::IMPDElement* element, const char* attribute)
{
    const std::map<std::string, std::string>& attrs = element->GetRawAttributes();
    auto iterator = attrs.find(attribute);
    if (iterator == attrs.end())
    {
        static std::string empty;
        // not found
        return empty;
    }
    return iterator->second;
}

const char_t* DashUtils::parseRawAttribute(const dash::mpd::IMPDElement* element, const char* attribute)
{
    const std::map<std::string, std::string>& attrs = element->GetRawAttributes();
    auto iterator = attrs.find(attribute);
    if (iterator == attrs.end())
    {
        // not found
        return "";
    }
    return iterator->second.c_str();
}

const std::string& DashUtils::getMimeType(DashComponents dashComponents)
{
    if (dashComponents.representation != OMAF_NULL && dashComponents.representation->GetMimeType().length() != 0)
    {
        return dashComponents.representation->GetMimeType();
    }
    else
    {
        return dashComponents.adaptationSet->GetMimeType();
    }
}


uint64_t DashUtils::getTimescale(DashComponents dashComponents)
{
    if (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "timescale"))
    {
        return dashComponents.segmentTemplate->GetTimescale();
    }
    dashComponents.segmentTemplate = dashComponents.adaptationSet->GetSegmentTemplate();
    if ((dashComponents.segmentTemplate) &&
        (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "timescale")))
    {
        return dashComponents.segmentTemplate->GetTimescale();
    }
    dashComponents.segmentTemplate = dashComponents.period->GetSegmentTemplate();
    if ((dashComponents.segmentTemplate) &&
        (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "timescale")))
    {
        return dashComponents.segmentTemplate->GetTimescale();
    }
    return 1;
}

uint64_t DashUtils::getDuration(DashComponents dashComponents)
{
    if (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "duration"))
    {
        return dashComponents.segmentTemplate->GetDuration();
    }
    dashComponents.segmentTemplate = dashComponents.adaptationSet->GetSegmentTemplate();
    if ((dashComponents.segmentTemplate) &&
        (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "duration")))
    {
        return dashComponents.segmentTemplate->GetDuration();
    }
    dashComponents.segmentTemplate = dashComponents.period->GetSegmentTemplate();
    if ((dashComponents.segmentTemplate) &&
        (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "duration")))
    {
        return dashComponents.segmentTemplate->GetDuration();
    }
    return 1;
}

uint64_t DashUtils::getStartNumber(DashComponents dashComponents)
{
    if (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "startNumber"))
    {
        return dashComponents.segmentTemplate->GetStartNumber();
    }
    dashComponents.segmentTemplate = dashComponents.adaptationSet->GetSegmentTemplate();
    if ((dashComponents.segmentTemplate) &&
        (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "startNumber")))
    {
        return dashComponents.segmentTemplate->GetStartNumber();
    }
    dashComponents.segmentTemplate = dashComponents.period->GetSegmentTemplate();
    if ((dashComponents.segmentTemplate) &&
        (DashUtils::elementHasAttribute(dashComponents.segmentTemplate, "startNumber")))
    {
        return dashComponents.segmentTemplate->GetStartNumber();
    }
    return 1;
}

void_t DashUtils::parseSegmentStartTimes(Array<DashTimelineSegment>& startTimes, DashComponents dashComponents)
{
    const dash::mpd::ISegmentTimeline* timeline = dashComponents.segmentTemplate->GetSegmentTimeline();
    const std::vector<dash::mpd::ITimeline*>& line = timeline->GetTimelines();
    uint64_t time = 0;
    uint64_t lasttime = 0;
    for (std::vector<dash::mpd::ITimeline*>::const_iterator s = line.begin(); s != line.end(); s++)
    {
        const dash::mpd::ITimeline* S = (*s);
        // OMAF_LOG_D("Entry: %lld %lld %lld", S->GetStartTime(), S->GetDuration(), S->GetRepeatCount());
        uint64_t newTime = S->GetStartTime();
        if (newTime)
        {
            time = newTime;
        }

        uint64_t duration = S->GetDuration();
        int64_t reps = S->GetRepeatCount();
        if (reps < 0)
        {
            // the duration indicated in @d attribute repeats until the start of the next S element, the end of the
            // Period or until the next MPD update. ) which libdash apparently cant handle either..
            OMAF_LOG_D("parseSegmentStartTimes negative repeat count!");
            OMAF_ASSERT_NOT_IMPLEMENTED();
            break;
        }
        bool add = true;
        // check if this 'S' needs to be added.
        uint64_t nt = time + (reps + 1) * duration;  // End of current 'S'
        if (startTimes.isEmpty() == false)
        {
            lasttime = startTimes.back().segmentStartTime;  // end of our current timeline.
            add = (nt >= lasttime);                         // add if the end of this 'S' is after our current timeline.
        }
        else
        {
            // if startTimes is empty we always add the first one.
            startTimes.add({time, duration});
            lasttime = time;
            time += duration;
            reps--;  // because we did one "repeat" already.
        }

        if (add)
        {
            for (int64_t rep = 0; rep <= reps; rep++)
            {
                if (time > lasttime)
                {
                    startTimes.add({time, duration});
                    lasttime = time;
                }
                time += duration;
            }
        }
        else
        {
            time += (reps + 1) * duration;
        }
    }

    /*
            int64_t scl= DashUtils::getTimescale(dashComponents);
            int64_t mPresentationTimeOffset = dashComponents.segmentTemplate->GetPresentationTimeOffset();
            int64_t st=startTimes[0].segmentStartTime;
            int64_t et=startTimes.back().segmentStartTime;
            OMAF_LOG_D("First segment starttime: %lld duration: %lld  offset: %lld",  st,
       startTimes[0].segmentDuration,mPresentationTimeOffset); OMAF_LOG_D("Last segment starttime: %lld duration: %lld
       offset: %lld", et, startTimes.back().segmentDuration, mPresentationTimeOffset);
            */
}

uint64_t DashUtils::parseMillisecondsFromString(const std::string& stringTime)
{
    if (stringTime.empty())
    {
        return 0;
    }
    size_t ptLocation = stringTime.find("PT");
    size_t fieldStart = ptLocation + 2;
    float32_t totalTimeInMS = 0;
    if (ptLocation != std::string::npos)
    {
        size_t hourLocation = stringTime.find("H");
        if (hourLocation != std::string::npos)
        {
            std::string hour = stringTime.substr(fieldStart, hourLocation - fieldStart);
            fieldStart = hourLocation + 1;
            float32_t hours = stof(hour);
            totalTimeInMS += hours * 60 * 60 * 1000;
        }
        size_t minuteLocation = stringTime.find("M");
        if (minuteLocation != std::string::npos)
        {
            std::string minute = stringTime.substr(fieldStart, minuteLocation - fieldStart);
            fieldStart = minuteLocation + 1;
            float32_t minutes = stof(minute);
            totalTimeInMS += minutes * 60 * 1000;
        }
        size_t secondLocation = stringTime.find("S");
        if (secondLocation != std::string::npos)
        {
            std::string second = stringTime.substr(fieldStart, secondLocation - fieldStart);
            float32_t seconds = stof(second);
            totalTimeInMS += seconds * 1000;
        }
    }
    return (uint64_t) totalTimeInMS;
}

uint64_t DashUtils::getStreamDurationInMilliSeconds(DashComponents dashComponents)
{
    if (!dashComponents.mpd->GetMediaPresentationDuration().empty())
    {
        return parseMillisecondsFromString(dashComponents.mpd->GetMediaPresentationDuration());
    }
    else if (!dashComponents.period->GetDuration().empty())
    {
        return parseMillisecondsFromString(dashComponents.period->GetDuration());
    }
    else
    {
        OMAF_ASSERT_UNREACHABLE();
        return 0;
    }
}

uint32_t DashUtils::getMPDUpdateInMilliseconds(DashComponents dashComponents)
{
    return (uint32_t) parseMillisecondsFromString(dashComponents.mpd->GetMinimumUpdatePeriod());
}
uint32_t DashUtils::getPeriodStartTimeMS(DashComponents dashComponents)
{
    return (uint32_t) parseMillisecondsFromString(dashComponents.period->GetStart());
}

OMAF_NS_END
