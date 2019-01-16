
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

#include "Platform/OMAFDataTypes.h"
#include "Platform/OMAFCompiler.h"

#include "Foundation/NVRDependencies.h"
#include "Foundation/NVRNonCopyable.h"
#include "Foundation/NVRStorageManager.h"

#if OMAF_ENABLE_LOG

    #define OMAF_LOG_ZONE(name) static const char_t* OMAF_LOG_ZONE_TAG OMAF_UNUSED = "OMAF:" #name;

    #if OMAF_ENABLE_TRACE_LOG

        #define OMAF_LOG_TRACE() OMAF_NS::LogDispatcher::logVerbose(OMAF_LOG_ZONE_TAG, "%s (%s:%d)", OMAF_FUNCTION, OMAF_FILE, OMAF_LINE)

    #else

        #define OMAF_LOG_TRACE()

    #endif
    
    #define OMAF_LOG_V(...) OMAF_NS::LogDispatcher::logVerbose(OMAF_LOG_ZONE_TAG, ## __VA_ARGS__)
    #define OMAF_LOG_D(...) OMAF_NS::LogDispatcher::logDebug(OMAF_LOG_ZONE_TAG, ## __VA_ARGS__)
    #define OMAF_LOG_I(...) OMAF_NS::LogDispatcher::logInfo(OMAF_LOG_ZONE_TAG, ## __VA_ARGS__)
    #define OMAF_LOG_W(...) OMAF_NS::LogDispatcher::logWarning(OMAF_LOG_ZONE_TAG, ## __VA_ARGS__)
    #define OMAF_LOG_E(...) OMAF_NS::LogDispatcher::logError(OMAF_LOG_ZONE_TAG, ## __VA_ARGS__)

    #define OMAF_LOG_V_IF(condition, ...) if (condition) OMAF_LOG_V(## __VA_ARGS__)
    #define OMAF_LOG_D_IF(condition, ...) if (condition) OMAF_LOG_D(## __VA_ARGS__)
    #define OMAF_LOG_I_IF(condition, ...) if (condition) OMAF_LOG_I(## __VA_ARGS__)
    #define OMAF_LOG_W_IF(condition, ...) if (condition) OMAF_LOG_W(## __VA_ARGS__)
    #define OMAF_LOG_E_IF(condition, ...) if (condition) OMAF_LOG_E(## __VA_ARGS__)
    
#else
    
    #define OMAF_LOG_ZONE(name)
    
    #define OMAF_LOG_TRACE()

    #define OMAF_LOG_V(...)
    #define OMAF_LOG_D(...)
    #define OMAF_LOG_I(...)
    #define OMAF_LOG_W(...)
    #define OMAF_LOG_E(...)
    
    #define OMAF_LOG_V_IF(condition, ...)
    #define OMAF_LOG_D_IF(condition, ...)
    #define OMAF_LOG_I_IF(condition, ...)
    #define OMAF_LOG_W_IF(condition, ...)
    #define OMAF_LOG_E_IF(condition, ...)
    
#endif

OMAF_NS_BEGIN

    class Logger
    {
        public:
        
            Logger() {}
            virtual ~Logger() {}
        
            virtual void_t logVerbose(const char_t* zone, const char_t* format, va_list args) = 0;
            virtual void_t logDebug(const char_t* zone, const char_t* format, va_list args) = 0;
            virtual void_t logInfo(const char_t* zone, const char_t* format, va_list args) = 0;
            virtual void_t logWarning(const char_t* zone, const char_t* format, va_list args) = 0;
            virtual void_t logError(const char_t* zone, const char_t* format, va_list args) = 0;

        private:
        
            OMAF_NO_COPY(Logger);
            OMAF_NO_ASSIGN(Logger);
    };
    
    class FileLogger
    : public Logger
    {
        public:
        
            FileLogger(const char_t* filename);
            virtual ~FileLogger();
        
            virtual void_t logVerbose(const char_t* zone, const char_t* format, va_list args);
            virtual void_t logDebug(const char_t* zone, const char_t* format, va_list args);
            virtual void_t logInfo(const char_t* zone, const char_t* format, va_list args);
            virtual void_t logWarning(const char_t* zone, const char_t* format, va_list args);
            virtual void_t logError(const char_t* zone, const char_t* format, va_list args);
        
        private:
        
            StorageManager::FileHandle open(const char_t* mFilename);
            void_t close(StorageManager::FileHandle handle);
        
        private:

            const char_t* mFilename;
            StorageManager::FileHandle mHandle;
    };
    
    class ConsoleLogger
    : public Logger
    {
        public:
        
            ConsoleLogger();
            virtual ~ConsoleLogger();
        
            virtual void_t logVerbose(const char_t* zone, const char_t* format, va_list args);
            virtual void_t logDebug(const char_t* zone, const char_t* format, va_list args);
            virtual void_t logInfo(const char_t* zone, const char_t* format, va_list args);
            virtual void_t logWarning(const char_t* zone, const char_t* format, va_list args);
            virtual void_t logError(const char_t* zone, const char_t* format, va_list args);
    };
    
    namespace LogDispatcher
    {
        namespace LogLevel
        {
            enum Enum
            {
                Verbose = 1,
                Debug = 2,
                Info = 4,
                Warning = 8,
                Error = 16
            };
        }

        void_t initialize(uint8_t aLogMask);
        void_t shutdown();
        
        void_t setLogLevel(uint8_t aLogMask);

        void_t mount(Logger& logger);
        void_t unmount(Logger& logger);
        
        void_t logVerbose(const char_t* zone, const char_t* format, ...);
        void_t logDebug(const char_t* zone, const char_t* format, ...);
        void_t logInfo(const char_t* zone, const char_t* format, ...);
        void_t logWarning(const char_t* zone, const char_t* format, ...);
        void_t logError(const char_t* zone, const char_t* format, ...);
        
        void_t logVerboseV(const char_t* zone, const char_t* format, va_list args);
        void_t logDebugV(const char_t* zone, const char_t* format, va_list args);
        void_t logInfoV(const char_t* zone, const char_t* format, va_list args);
        void_t logWarningV(const char_t* zone, const char_t* format, va_list args);
        void_t logErrorV(const char_t* zone, const char_t* format, va_list args);
    }
OMAF_NS_END
