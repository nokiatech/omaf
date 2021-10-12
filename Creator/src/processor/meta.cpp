
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
#include "meta.h"

namespace VDD
{
    std::string to_string(const TrackId& aTrackId)
    {
        return std::to_string(aTrackId.get());
    }

    std::ostream& operator<<(std::ostream& aStream, const CodedFrameMeta& aMeta)
    {
        aStream << "presIndex:" << aMeta.presIndex;
        aStream << "\n" "codingIndex:" << aMeta.codingIndex;
        aStream << "\n" "codingTime:" << aMeta.codingTime;
        aStream << "\n" "presTime:" << aMeta.presTime;
        aStream << "\n" "duration:" << aMeta.duration;
        aStream << "\n" "trackId:" << aMeta.trackId;

        aStream << "\n" "inCodingOrder:" << aMeta.inCodingOrder;

        aStream << "\n" "format:" << aMeta.format;

        // not supported yet
        //std::map<ConfigType, std::vector<std::uint8_t>> decoderConfig;

        aStream << "\n" "width:" << aMeta.width;
        aStream << "\n" "height:" << aMeta.height;

        //aStream << "\n" "channelConfig:" << aMeta.channelConfig;
        aStream << "\n" "samplingFrequency:" << aMeta.samplingFrequency;
        //aStream << "\n" "bitrate:" << aMeta.bitrate;

        aStream << "\n" "type:" << int(aMeta.type);

        //aStream << "\n" "segmenterMeta:" << aMeta.segmenterMeta;

        // applicable in OMAF
        // aStream << "\n" "projection:" << aMeta.projection;
        // aStream << "\n" "regionPacking:" << aMeta.regionPacking;
        // aStream << "\n" "sphericalCoverage:" << aMeta.sphericalCoverage;
        // aStream << "\n" "qualityRankCoverage:" << aMeta.qualityRankCoverage;

        return aStream;
    }

    Meta::Meta()
        : mContentType(ContentType::None)
    {
        // nothing
    }

    Meta::Meta(const Meta& aMeta)
        : mContentType(aMeta.mContentType)
        , mRawFrameMeta(aMeta.mRawFrameMeta)
        , mCodedFrameMeta(aMeta.mCodedFrameMeta)
    {
        for (auto& tag : aMeta.mTags)
        {
            mTags.push_back(std::unique_ptr<MetaTag>(tag->clone()));
        }
    }

    Meta::Meta(Meta&& aMeta)
        : mContentType(std::move(aMeta.mContentType))
        , mRawFrameMeta(std::move(aMeta.mRawFrameMeta))
        , mCodedFrameMeta(std::move(aMeta.mCodedFrameMeta))
        , mTags(std::move(aMeta.mTags))
    {
        // nothing
    }

    Meta::Meta(const RawFrameMeta& aRawFrameMeta, const Meta& aMeta)
        : mContentType(ContentType::RAW)
        , mRawFrameMeta(aRawFrameMeta)
    {
        for (auto& tag : aMeta.mTags)
        {
            mTags.push_back(std::unique_ptr<MetaTag>(tag->clone()));
        }
    }

    Meta::Meta(const CodedFrameMeta& aCodedFrameMeta, const Meta& aMeta)
        : mContentType(ContentType::CODED)
        , mCodedFrameMeta(aCodedFrameMeta)
    {
        for (auto& tag : aMeta.mTags)
        {
            mTags.push_back(std::unique_ptr<MetaTag>(tag->clone()));
        }
    }

    Meta::Meta(const RawFrameMeta& aRawFrameMeta)
        : mContentType(ContentType::RAW)
        , mRawFrameMeta(aRawFrameMeta)
    {
        // nothing
    }

    Meta::Meta(const CodedFrameMeta& aCodedFrameMeta)
        : mContentType(ContentType::CODED)
        , mCodedFrameMeta(aCodedFrameMeta)
    {
        // nothing
    }

    Meta& Meta::operator=(const Meta& aMeta)
    {
        if (this != &aMeta)
        {
            mContentType = aMeta.mContentType;
            mRawFrameMeta = aMeta.mRawFrameMeta;
            mCodedFrameMeta = aMeta.mCodedFrameMeta;
            for (auto& tag : aMeta.mTags)
            {
                mTags.push_back(std::unique_ptr<MetaTag>(tag->clone()));
            }
        }
        return *this;
    }

    Meta& Meta::operator=(Meta&& aMeta)
    {
        if (this != &aMeta)
        {
            mContentType = std::move(aMeta.mContentType);
            mRawFrameMeta = std::move(aMeta.mRawFrameMeta);
            mCodedFrameMeta = std::move(aMeta.mCodedFrameMeta);
            mTags = std::move(aMeta.mTags);
        }
        return *this;
    }

    ContentType Meta::getContentType() const
    {
        return mContentType;
    }

    const RawFrameMeta& Meta::getRawFrameMeta() const
    {
        assert(mContentType == ContentType::RAW);
        return mRawFrameMeta;
    }

    const CodedFrameMeta& Meta::getCodedFrameMeta() const
    {
        assert(mContentType == ContentType::CODED);
        return mCodedFrameMeta;
    }

    CommonFrameMeta Meta::getCommonFrameMeta() const
    {
        // Well this is silly. The correct way would be to have the common fields removed away from
        // mCodedFrameMeta/mRawFrameMeta, or this CommonFrameMeta put inside it. We're a bit closer to this
        // dream now!
        switch (mContentType)
        {
            case ContentType::CODED:
            {
                return { mCodedFrameMeta.presIndex, mCodedFrameMeta.duration, mCodedFrameMeta.width, mCodedFrameMeta.height };
            };
            case ContentType::RAW:
            {
                return { mRawFrameMeta.presIndex, mRawFrameMeta.duration, mRawFrameMeta.width, mRawFrameMeta.height };
            };
            default:
                ;
        }
        return {};
    }
}
