
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

#include "DashProvider/NVRDashAdaptationSet.h"


OMAF_NS_BEGIN

    class DashAdaptationSetSubPicture : public DashAdaptationSet
    {
    public:
        DashAdaptationSetSubPicture(DashAdaptationSetObserver& observer);

        virtual ~DashAdaptationSetSubPicture();

        static bool_t isSubPicture(DashComponents aDashComponents);

    public: // from DashAdaptationSet
        virtual AdaptationSetType::Enum getType() const;

    protected: // from DashAdaptationSet
        virtual DashRepresentation* createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth);
        virtual bool_t parseVideoProperties(DashComponents& aNextComponents);
        virtual void_t parseVideoViewport(DashComponents& aNextComponents);
        virtual void_t doSwitchRepresentation();

    };

    typedef FixedArray<DashAdaptationSetSubPicture*, MAX_ADAPTATION_SET_COUNT> TileAdaptationSets;


OMAF_NS_END
