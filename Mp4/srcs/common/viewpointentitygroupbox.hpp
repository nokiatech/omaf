
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
#ifndef VIEWPOINTENTITYGROUPBOX_HPP
#define VIEWPOINTENTITYGROUPBOX_HPP

#include "api/isobmff/commontypes.h"
#include "customallocator.hpp"
#include "entitytogroupbox.hpp"

/**
 * ViewpointEntityGroupBox 'vipo' from OMAFv2 specs.
 */
class ViewpointEntityGroupBox : public EntityToGroupBox
{
public:
    ViewpointEntityGroupBox();
    virtual ~ViewpointEntityGroupBox() = default;

    uint32_t getViewpointId() const;
    void setViewpointId(uint32_t);

	String getLabel() const;
    void setLabel(const String&);

    const ISOBMFF::ViewpointPosStruct& getViewpointPos() const;
    void setViewpointPos(const ISOBMFF::ViewpointPosStruct&);

    const ISOBMFF::ViewpointGroupStruct<true>& getViewpointGroup() const;
    void setViewpointGroup(const ISOBMFF::ViewpointGroupStruct<true>& vpGroup);

    const ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct& getViewpointGlobalCoordinateSysRotation() const;

    void
    setViewpointGlobalCoordinateSysRotation(const ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct& globCoordRot);

    ISOBMFF::Optional<ISOBMFF::ViewpointGpsPositionStruct> getViewpointGpsPosition() const;
    void setViewpointGpsPosition(const ISOBMFF::Optional<ISOBMFF::ViewpointGpsPositionStruct>& gpsPos);

    ISOBMFF::Optional<ISOBMFF::ViewpointGeomagneticInfoStruct> getViewpointGeomagneticInfo() const;
    void setViewpointGeomagneticInfo(const ISOBMFF::Optional<ISOBMFF::ViewpointGeomagneticInfoStruct>& geomagInfo);

    ISOBMFF::Optional<ISOBMFF::ViewpointSwitchingListStruct> getViewpointSwitchingList() const;
    void setViewpointSwitchingList(const ISOBMFF::Optional<ISOBMFF::ViewpointSwitchingListStruct>& switchList);

    ISOBMFF::Optional<ISOBMFF::ViewpointLoopingStruct> getViewpointLooping() const;
    void setViewpointLooping(const ISOBMFF::Optional<ISOBMFF::ViewpointLoopingStruct>& looping);

    virtual void writeBox(ISOBMFF::BitStream& bitstream);
    virtual void parseBox(ISOBMFF::BitStream& bitstream);

private:
    uint32_t mViewpointId;
	String mLabel;

    ISOBMFF::ViewpointPosStruct mViewpointPos;
    ISOBMFF::ViewpointGroupStruct<true> mViewpointGroup;
    ISOBMFF::ViewpointGlobalCoordinateSysRotationStruct mViewpointGlobalCoordinateSysRotation;

    ISOBMFF::Optional<ISOBMFF::ViewpointGpsPositionStruct> mViewpointGpsPosition;
    ISOBMFF::Optional<ISOBMFF::ViewpointGeomagneticInfoStruct> mViewpointGeomagneticInfo;
    ISOBMFF::Optional<ISOBMFF::ViewpointSwitchingListStruct> mViewpointSwitchingList;
    ISOBMFF::Optional<ISOBMFF::ViewpointLoopingStruct> mViewpointLooping;
};

#endif /* VIEWPOINTENTITYGROUPBOX_HPP */
