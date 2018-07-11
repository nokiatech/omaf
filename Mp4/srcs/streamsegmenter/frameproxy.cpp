
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
#include "api/streamsegmenter/frameproxy.hpp"
#include "api/streamsegmenter/track.hpp"

namespace StreamSegmenter
{
    AcquireFrameData::AcquireFrameData()
    {
        // nothing
    }

    AcquireFrameData::~AcquireFrameData()
    {
        // nothing
    }

    FrameProxy::FrameProxy(std::unique_ptr<AcquireFrameData>&& aAcquire, FrameInfo aFrameInfo)
        : mAcquire(std::move(aAcquire))
        , mFrameInfo(aFrameInfo)
    {
        // nothing
    }

    FrameProxy::FrameProxy(const FrameProxy& aOther)
        : mAcquire(aOther.mAcquire->clone())
        , mFrameInfo(aOther.mFrameInfo)
    {
        // nothing
    }

    FrameProxy::FrameProxy(FrameProxy&& aOther)
        : mAcquire(std::move(aOther.mAcquire))
        , mFrameInfo(std::move(aOther.mFrameInfo))
    {
        assert(aOther.mValid);
        aOther.mValid = false;
    }

    FrameProxy& FrameProxy::operator=(const FrameProxy& aOther)
    {
        mAcquire.reset(aOther.mAcquire->clone());
        mFrameInfo = aOther.mFrameInfo;
        return *this;
    }

    FrameProxy& FrameProxy::operator=(FrameProxy&& aOther)
    {
        assert(aOther.mValid);
        mAcquire      = std::move(aOther.mAcquire);
        mFrameInfo    = std::move(aOther.mFrameInfo);
        aOther.mValid = false;
        return *this;
    }

    FrameProxy::~FrameProxy()
    {
        // nothing
    }

    Frame FrameProxy::operator*() const
    {
        assert(mValid);
        return {mFrameInfo, mAcquire->get()};
    }

    std::unique_ptr<Frame> FrameProxy::operator->() const
    {
        assert(mValid);
        return std::unique_ptr<Frame>{new Frame{mFrameInfo, mAcquire->get()}};
    }

    size_t FrameProxy::getSize() const
    {
        return mAcquire->getSize();
    }

    FrameInfo FrameProxy::getFrameInfo() const
    {
        return mFrameInfo;
    }

    void FrameProxy::setFrameInfo(const FrameInfo& aFrameInfo)
    {
        mFrameInfo = aFrameInfo;
    }
}
