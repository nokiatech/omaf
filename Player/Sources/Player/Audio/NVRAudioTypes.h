
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

OMAF_NS_BEGIN
    namespace AudioReturnValue
    {
        enum Enum
        {
            INVALID = -1,
            
            OK,
            
            // This source has now given all the samples it has, there will be no more.
            END_OF_FILE,
            
            // This source is currently out of samples, but it might get more later
            OUT_OF_SAMPLES,
            
            // There was unrecoverable error while processing the request.
            FAIL,
            
            // Source has not been initialized. Do that first, then try again.
            NOT_INITIALIZED,
            
            COUNT
        };
    }
    
    namespace AudioOutputRange
    {
        // AudioRenderer supports int16 and float32 output. This enum indicates the value range of
        // audio sample amplitude. Use also the correct renderSamples-variant
        enum Enum
        {
            INVALID = -1,
            
            // Output samples are floats, from -1 to +1
            FLOAT_1 = 0,
            
            // Output samples are floats, from -32767f to +32768f   (input is anyway int16 so max for floats is the same)
            FLOAT_16,
            
            // Output samples are integers, from -32767 to 32768
            INT16,
            
            COUNT
        };
    }

    // Playback is enabled for headphones, headphones with head orientation tracking support,
    // and for multichannel loudspeaker setups (2 to N).
    namespace PlaybackMode {
        enum Enum
        {
            INVALID = -1,

            // Output is processed for headphone listening
            HEADPHONES,
            // Output is processed for headphone listening with head-tracking support
            HEADTRACKED_HEADPHONES,
            // Output is processed for a loudspeaker setup (2-8 output channels)
            LOUDSPEAKERS,

            COUNT
        };
    }
OMAF_NS_END
