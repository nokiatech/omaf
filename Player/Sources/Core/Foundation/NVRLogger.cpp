
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
#include "Foundation/NVRLogger.h"

#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRMemorySystem.h"
#include "Foundation/NVRString.h"

OMAF_NS_BEGIN
namespace LogDispatcher
{
    static ConsoleLogger mDefaultConsoleLogger;

#define OMAF_MAX_LOGGERS 32

    typedef FixedArray<Logger*, OMAF_MAX_LOGGERS> Loggers;
    static Loggers mAvailableLoggers;

    static uint8_t mLogMask =
        LogLevel::Verbose | LogLevel::Debug | LogLevel::Info | LogLevel::Warning | LogLevel::Error;

    void_t initialize(uint8_t aLogMask)
    {
        setLogLevel(aLogMask);
        mount(mDefaultConsoleLogger);
    }

    void_t shutdown()
    {
        unmount(mDefaultConsoleLogger);
    }

    void_t setLogLevel(uint8_t aLogMask)
    {
        mLogMask = aLogMask;
    }

    void_t mount(Logger& logger)
    {
        if (!mAvailableLoggers.contains(&logger))
        {
            mAvailableLoggers.add(&logger);
        }
    }

    void_t unmount(Logger& logger)
    {
        if (mAvailableLoggers.contains(&logger))
        {
            mAvailableLoggers.remove(&logger);
        }
    }

    void_t logVerbose(const char_t* zone, const char_t* format, ...)
    {
        if (!(mLogMask & LogLevel::Verbose))
            return;

        va_list args;
        va_start(args, format);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logVerbose(zone, format, args);
        }

        va_end(args);
    }

    void_t logDebug(const char_t* zone, const char_t* format, ...)
    {
        if (!(mLogMask & LogLevel::Debug))
            return;

        va_list args;
        va_start(args, format);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logDebug(zone, format, args);
        }

        va_end(args);
    }

    void_t logInfo(const char_t* zone, const char_t* format, ...)
    {
        if (!(mLogMask & LogLevel::Info))
            return;

        va_list args;
        va_start(args, format);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logInfo(zone, format, args);
        }

        va_end(args);
    }

    void_t logWarning(const char_t* zone, const char_t* format, ...)
    {
        if (!(mLogMask & LogLevel::Warning))
            return;

        va_list args;
        va_start(args, format);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logWarning(zone, format, args);
        }

        va_end(args);
    }

    void_t logError(const char_t* zone, const char_t* format, ...)
    {
        if (!(mLogMask & LogLevel::Error))
            return;

        va_list args;
        va_start(args, format);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logError(zone, format, args);
        }

        va_end(args);
    }

    void_t logVerboseV(const char_t* zone, const char_t* format, va_list args)
    {
        if (!(mLogMask & LogLevel::Verbose))
            return;

        va_list copy;
        va_copy(copy, args);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logVerbose(zone, format, args);
        }

        va_end(args);
    }

    void_t logDebugV(const char_t* zone, const char_t* format, va_list args)
    {
        if (!(mLogMask & LogLevel::Debug))
            return;

        va_list copy;
        va_copy(copy, args);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logDebug(zone, format, args);
        }

        va_end(args);
    }

    void_t logInfoV(const char_t* zone, const char_t* format, va_list args)
    {
        if (!(mLogMask & LogLevel::Info))
            return;

        va_list copy;
        va_copy(copy, args);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logInfo(zone, format, args);
        }

        va_end(args);
    }

    void_t logWarningV(const char_t* zone, const char_t* format, va_list args)
    {
        if (!(mLogMask & LogLevel::Warning))
            return;

        va_list copy;
        va_copy(copy, args);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logWarning(zone, format, args);
        }

        va_end(args);
    }

    void_t logErrorV(const char_t* zone, const char_t* format, va_list args)
    {
        if (!(mLogMask & LogLevel::Error))
            return;

        va_list copy;
        va_copy(copy, args);

        for (size_t i = 0; i < mAvailableLoggers.getSize(); ++i)
        {
            Logger& logger = *mAvailableLoggers[i];
            logger.logError(zone, format, args);
        }

        va_end(args);
    }
}  // namespace LogDispatcher


FileLogger::FileLogger(const char_t* filename)
    : mFilename(filename)
    , mHandle(StorageManager::InvalidFileHandle)
{
}

FileLogger::~FileLogger()
{
    close(mHandle);

    mHandle = StorageManager::InvalidFileHandle;
}

StorageManager::FileHandle FileLogger::open(const char_t* filename)
{
    StorageManager::FileHandle handle = StorageManager::Open(filename, FileSystem::AccessMode::WRITE);

    return handle;
}

void_t FileLogger::close(StorageManager::FileHandle handle)
{
    if (handle != StorageManager::InvalidFileHandle)
    {
        StorageManager::Close(handle);
    }
}

void_t FileLogger::logVerbose(const char_t* zone, const char_t* format, va_list args)
{
    if (mHandle == StorageManager::InvalidFileHandle)
    {
        mHandle = open(mFilename);
    }

    MemoryAllocator& allocator = *MemorySystem::DebugHeapAllocator();

    String messageStr(allocator);
    messageStr.appendFormat("V/%s: ", zone);
    messageStr.appendFormatVar(format, args);
    messageStr.append("\n");

    StorageManager::Write(mHandle, messageStr.getData(), messageStr.getSize() - 1);
}

void_t FileLogger::logDebug(const char_t* zone, const char_t* format, va_list args)
{
    if (mHandle == StorageManager::InvalidFileHandle)
    {
        mHandle = open(mFilename);
    }

    MemoryAllocator& allocator = *MemorySystem::DebugHeapAllocator();

    String messageStr(allocator);
    messageStr.appendFormat("D/%s: ", zone);
    messageStr.appendFormatVar(format, args);
    messageStr.append("\n");

    StorageManager::Write(mHandle, messageStr.getData(), messageStr.getSize() - 1);
}

void_t FileLogger::logInfo(const char_t* zone, const char_t* format, va_list args)
{
    if (mHandle == StorageManager::InvalidFileHandle)
    {
        mHandle = open(mFilename);
    }

    MemoryAllocator& allocator = *MemorySystem::DebugHeapAllocator();

    String messageStr(allocator);
    messageStr.appendFormat("I/%s: ", zone);
    messageStr.appendFormatVar(format, args);
    messageStr.append("\n");

    StorageManager::Write(mHandle, messageStr.getData(), messageStr.getSize() - 1);
}

void_t FileLogger::logWarning(const char_t* zone, const char_t* format, va_list args)
{
    if (mHandle == StorageManager::InvalidFileHandle)
    {
        mHandle = open(mFilename);
    }

    MemoryAllocator& allocator = *MemorySystem::DebugHeapAllocator();

    String messageStr(allocator);
    messageStr.appendFormat("W/%s: ", zone);
    messageStr.appendFormatVar(format, args);
    messageStr.append("\n");

    StorageManager::Write(mHandle, messageStr.getData(), messageStr.getSize() - 1);
}

void_t FileLogger::logError(const char_t* zone, const char_t* format, va_list args)
{
    if (mHandle == StorageManager::InvalidFileHandle)
    {
        mHandle = open(mFilename);
    }

    MemoryAllocator& allocator = *MemorySystem::DebugHeapAllocator();

    String messageStr(allocator);
    messageStr.appendFormat("E/%s: ", zone);
    messageStr.appendFormatVar(format, args);
    messageStr.append("\n");

    StorageManager::Write(mHandle, messageStr.getData(), messageStr.getSize() - 1);
}
OMAF_NS_END
