
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
#pragma once

#include "Math/OMAFMathTypes.h"
#include "Math/OMAFMathConstants.h"
#include "Math/OMAFMathFunctions.h"
#include "Math/OMAFColor3.h"
#include "Math/OMAFColor4.h"
#include "Math/OMAFPoint.h"
#include "Math/OMAFSize.h"
#include "Math/OMAFRect.h"
#include "Math/OMAFVector2.h"
#include "Math/OMAFVector3.h"
#include "Math/OMAFVector4.h"
#include "Math/OMAFQuaternion.h"
#include "Math/OMAFMatrix44.h"
#include "Math/OMAFMatrix33.h"
#include "Math/OMAFAxisAngle.h"

/*
 *
 *  OMAF SDK uses right-handed (RHS) / counter-clockwise (CCW) coordinate system:
 *
 *              (Y)
 *               |
 *               |     (-Z)
 *               |   /
 *               | /
 *  (-X) --------+-------- (X)
 *              /|
 *            /  |
 *         (Z)   |
 *               |
 *              (-Y)
 *
 *
 *  OMAF SDK uses the following directions for positive rotations:
 *
 *  yaw   == look left
 *  pitch == look up
 *  roll  == tilt left
 *
 */
