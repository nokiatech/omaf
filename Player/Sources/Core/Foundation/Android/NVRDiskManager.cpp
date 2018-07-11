
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
#include "Foundation/NVRDiskManager.h"

#include "Foundation/Android/NVRAndroid.h"
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    namespace DiskManager
    {
        FileHandle InvalidFileHandle = NULL;
        
        bool_t FileExists(const char_t* path)
        {
            PathName filePath = GetFullPath(path);

            struct stat st;
            int result = stat(filePath, &st);

            // TODO: Make sure it's a FILE and not a directory or something.

            return (result == 0);
        }
        
        bool_t DirExists(const char_t* path)
        {
            OMAF_UNUSED_VARIABLE(path);
            OMAF_ASSERT_NOT_IMPLEMENTED();

            return false;
        }
        
        PathName GetFullPath(const char_t* path)
        {
            PathName storagePath = Android::getExternalStoragePath();
            storagePath.append(path);

            return storagePath;
        }
        
        FileHandle Open(const char_t* filename, FileSystem::AccessMode::Enum mode, FileSystem::CreateMode::Enum createMode)
        {
            FileHandle handle = InvalidFileHandle;

            PathName filePath = GetFullPath(filename);

            switch (mode)
            {
                case FileSystem::AccessMode::READ:
                {
                    int result = open64(filePath, O_RDONLY);
                    if (result == -1)
                    {
                        return InvalidFileHandle;
                    }
                    handle = (FileHandle)(intptr_t)result;
                    break;
                }

                case FileSystem::AccessMode::WRITE:
                {
                    if (createMode == FileSystem::CreateMode::CREATE)
                    {
                        int result = open64(filePath, O_WRONLY | O_CREAT);
                        if (result == -1)
                        {
                            return InvalidFileHandle;
                        }
                        handle = (FileHandle)(intptr_t)result;
                    }
                    else if(createMode == FileSystem::CreateMode::APPEND)
                    {
                        int result = open64(filePath, O_WRONLY | O_APPEND);
                        if (result == -1)
                        {
                            return InvalidFileHandle;
                        }
                        handle = (FileHandle)(intptr_t)result;
                    }
                    else
                    {
                        OMAF_ASSERT_UNREACHABLE();
                    }

                    break;
                }

                case FileSystem::AccessMode::READ_WRITE:
                {
                    if (createMode == FileSystem::CreateMode::CREATE)
                    {
                        int result = open64(filePath, O_RDWR | O_CREAT);
                        if (result == -1)
                        {
                            return InvalidFileHandle;
                        }
                        handle = (FileHandle)(intptr_t)result;
                    }
                    else if(createMode == FileSystem::CreateMode::APPEND)
                    {
                        int result = open64(filePath, O_RDWR | O_APPEND);
                        if (result == -1)
                        {
                            return InvalidFileHandle;
                        }
                        handle = (FileHandle)(intptr_t)result;
                    }
                    else
                    {
                        OMAF_ASSERT_UNREACHABLE();
                    }

                    break;
                }

                default:
                    OMAF_ASSERT_UNREACHABLE();
                    break;
            }

            return handle;
        }
        
        void_t Close(FileHandle handle)
        {
            if(handle != InvalidFileHandle)
            {
                close((intptr_t)handle);
                handle = InvalidFileHandle;
            }
        }

        bool_t Delete(const char_t* filename)
        {
            PathName filePath = GetFullPath(filename);
            int retVal = remove(filePath.getData());

            return (0 == retVal);
        }

        bool_t Rename(const char_t* oldFilename, const char_t* newFilename)
        {
            PathName oldFilePath = GetFullPath(oldFilename);
            PathName newFilePath = GetFullPath(newFilename);

            int retVal = rename(oldFilePath.getData(), newFilePath.getData());

            return (0 == retVal);
        }

        int64_t Read(FileHandle handle, void_t* destination, int64_t bytes)
        {
            if(handle != InvalidFileHandle)
            {
                return read((intptr_t)handle, destination, (size_t)bytes);
            }

            return 0;
        }

        int64_t Write(FileHandle handle, const void_t* source, int64_t bytes)
        {
            if(handle != InvalidFileHandle)
            {
                ssize_t result = write((intptr_t)handle, source, (size_t)bytes);
                fsync((intptr_t)handle); // TODO: Add separate Flush() instead of calling this every time
                return result;
            }

            return 0;
        }

        bool_t Seek(FileHandle handle, int64_t offset)
        {
            if(handle != InvalidFileHandle)
            {
                off64_t result = lseek64((intptr_t)handle, offset, SEEK_CUR);

                return (result != (off64_t)-1);
            }

            return false;
        }
        
        int64_t Tell(FileHandle handle)
        {
            if(handle != InvalidFileHandle)
            {
                return lseek64((intptr_t)handle, 0, SEEK_CUR);
            }

            return (int64_t)-1;
        }
        
        bool_t IsOpen(FileHandle handle)
        {
            if(handle != InvalidFileHandle)
            {
                return true;
            }
            
            return false;
        }
        
        int64_t GetSize(FileHandle handle)
        {
            if(handle != InvalidFileHandle)
            {
                struct stat64 st;
                int result = fstat64((intptr_t)handle, &st);

                OMAF_ASSERT(result == 0, "");

                return st.st_size;
            }

            return 0;
        }
    }
OMAF_NS_END
