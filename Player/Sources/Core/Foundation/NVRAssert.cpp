
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
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRStringUtilities.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(Assertion);

namespace Assertion
{
    BreakType::Enum defaultHandler(const char_t* condition, const char_t* msg, const char_t* file, const int32_t line)
    {
        OMAF_LOG_D("%s(%d): Assert failure: '%s', msg: %s", file, line, condition, msg);
        
        return BreakType::HALT;
    }
    
    Handler& getAssertHandlerInstance()
    {
        static Handler handler = &defaultHandler;
        
        return handler;
    }
    
    Handler getHandler()
    {
        return getAssertHandlerInstance();
    }
    
    void_t setHandler(Handler handler)
    {
        getAssertHandlerInstance() = handler;
    }
    
    BreakType::Enum reportFailure(const char_t* condition, const char_t* file, const int32_t line, const char_t* msg, ...)
    {
        const char_t* message = OMAF_NULL;
        
        if (msg != OMAF_NULL)
        {
            const uint32_t bufferSize = 2048;
            char_t messageBuffer[bufferSize];

            va_list args;
            va_start(args, msg);
            StringFormatVar(messageBuffer, bufferSize, msg, args);
            va_end(args);
            
            message = messageBuffer;
        }
        
        return getAssertHandlerInstance()(condition, message, file, line);
    }
}

OMAF_NS_END
