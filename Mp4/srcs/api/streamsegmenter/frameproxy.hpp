
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
#ifndef STREAMSEGMENTER_FRAMEPROXY_HPP
#define STREAMSEGMENTER_FRAMEPROXY_HPP

#include <memory>
#include "frame.hpp"

namespace StreamSegmenter
{
    class AcquireFrameData
    {
    public:
        AcquireFrameData();
        virtual ~AcquireFrameData();

        AcquireFrameData(const AcquireFrameData& other) = delete;
        void operator=(const AcquireFrameData&) = delete;

        virtual size_t getSize() const = 0;
        virtual FrameData get() const  = 0;

        virtual AcquireFrameData* clone() const = 0;
    };

    class FrameProxy
    {
    public:
        FrameProxy(std::unique_ptr<AcquireFrameData>&& aAcquire, FrameInfo aFrameInfo);
        FrameProxy(const FrameProxy& aOther);
        FrameProxy(FrameProxy&& aOther);
        FrameProxy& operator=(const FrameProxy& aOther);
        FrameProxy& operator=(FrameProxy&& aOther);
        ~FrameProxy();
        Frame operator*() const;
        std::unique_ptr<Frame> operator->() const;
        FrameInfo getFrameInfo() const;
        void setFrameInfo(const FrameInfo& aFrameInfo);
        size_t getSize() const;

    private:
        std::unique_ptr<AcquireFrameData> mAcquire;
        FrameInfo mFrameInfo;

        bool mValid = true;
    };

}  // namespace StreamSegmenter

#endif  // STREAMSEGMENTER_FRAMEPROXY_HPP
