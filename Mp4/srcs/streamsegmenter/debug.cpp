
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
#include "debug.hpp"

#include <sstream>

#include "../api/streamsegmenter/rational.hpp"
#include "utils.hpp"

namespace StreamSegmenter
{
    namespace Debug
    {
        namespace
        {
            template <typename T>
            std::string q(T x)
            {
                std::ostringstream s;
                s << "\"" << x << "\"";
                return s.str();
            }

            template <typename Rational>
            std::string rat(Rational x)
            {
                std::ostringstream st;
                st << "\"" << x << "=" << x.asDouble() << "\"";
                return st.str();
            }
        }

        std::string dumpSegmentStructure(const std::list<StreamSegmenter::Segmenter::Segments>& aSegments)
        {
            using namespace StreamSegmenter;
            std::ostringstream st;
            Utils::TrueOnce firstSegment;
            st << "{ \"dump\": ";
            st << "[";
            for (const Segmenter::Segments& segments : aSegments)
            {
                st << (firstSegment() ? "" : ", ");
                st << "[";
                Utils::TrueOnce firstSubsegment;
                for (const Segmenter::Segment& segment : segments)
                {
                    st << (firstSubsegment() ? "" : ", ");
                    st << "{";
                    st << "\"t0\": " << rat(segment.t0);
                    st << ", \"duration\": " << rat(segment.duration);
                    st << ", \"sequenceId\": " << segment.sequenceId;
                    st << ", \"tracks\": [";
                    Utils::TrueOnce firstTrack;
                    for (const auto& trackSegment : segment.tracks)
                    {
                        auto trackId                                    = trackSegment.first;
                        const Segmenter::TrackOfSegment& trackOfSegment = trackSegment.second;
                        st << (firstTrack() ? "" : ", ");
                        st << "{\"trackId\": " << trackId;
                        st << ", \"t0\": " << rat(trackOfSegment.trackInfo.t0);
                        st << ", \"timescale\": " << q(trackOfSegment.trackInfo.trackMeta.timescale);
                        st << ", \"frames\": [";
                        Utils::TrueOnce firstFrame;
                        for (const FrameProxy& frame : trackOfSegment.frames)
                        {
                            const FrameInfo frameInfo = frame.getFrameInfo();
                            st << (firstFrame() ? "" : ", ");
                            st << "{";
                            st << "\"cts\": [";
                            Utils::TrueOnce firstCts;
                            for (auto cts : frameInfo.cts)
                            {
                                st << (firstCts() ? "" : ", ") << rat(cts);
                            }
                            st << "]";
                            st << ", \"dts\": " << (frameInfo.dts ? rat(*frameInfo.dts) : std::string("null"));
                            st << ", \"duration\": " << rat(frameInfo.duration);
                            st << ", \"isIDR\": " << (frameInfo.isIDR ? "true" : "false");
                            st << ", \"contents\": \"";
                            auto data = *frame;
                            for (auto x : data.data)
                            {
                                st << char(x);
                            }
                            st << "\"";
                            st << "}";
                        }
                        st << "]";
                        st << "}";
                    }
                    st << "]";
                    st << "}";
                }
                st << "]";
            }
            st << "]";
            st << "}";
            return st.str();
        }
    }
}
