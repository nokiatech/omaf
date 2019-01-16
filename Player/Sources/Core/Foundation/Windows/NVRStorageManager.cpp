
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
#include "Foundation/NVRStorageManager.h"
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    namespace StorageManager
    {
        FileHandle InvalidFileHandle = NULL;

        PathName gStoragePath;
        bool_t FileExists(const char_t* path)
        {
            PathName filePath = GetFullPath(path);
            struct __stat64 st;
            int result = _stat64(filePath, &st);
            return (result == 0);
        }

        bool_t DirExists(const char_t* path)
        {
            OMAF_ASSERT_NOT_IMPLEMENTED();

            return false;
        }

        PathName GetFullPath(const char_t* path)
        {
            PathName storagePath = gStoragePath;
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
                    handle = fopen(filePath, "rb");
                    break;
                }

                case FileSystem::AccessMode::WRITE:
                {
                    if (createMode == FileSystem::CreateMode::CREATE)
                    {
                        handle = fopen(filePath, "wb");
                    }
                    else if(createMode == FileSystem::CreateMode::APPEND)
                    {
                        handle = fopen(filePath, "ab");
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
                        handle = fopen(filePath, "wb+");
                    }
                    else if(createMode == FileSystem::CreateMode::APPEND)
                    {
                        handle = fopen(filePath, "ab+");
                    }
                    else
                    {
                        OMAF_ASSERT_UNREACHABLE();
                    }

                    break;
                }

                default:
                    break;
            }

            return handle;
        }

        void_t Close(FileHandle handle)
        {
            if(handle != InvalidFileHandle)
            {
                fclose((FILE*)handle);
                handle = InvalidFileHandle;
            }
        }

        int64_t Read(FileHandle handle, void_t* destination, int64_t bytes)
        {
            if(handle != InvalidFileHandle)
            {
                return fread(destination, 1, bytes, (FILE*)handle);
            }

            return 0;
        }

        int64_t Write(FileHandle handle, const void_t* source, int64_t bytes)
        {
            if(handle != InvalidFileHandle)
            {
                return fwrite(source, 1, bytes, (FILE*)handle);
            }

            return 0;
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

        bool_t Seek(FileHandle handle, int64_t offset)
        {
            if(handle != InvalidFileHandle)
            {
                return (0 == _fseeki64((FILE*)handle, offset, SEEK_CUR));//fseek and _fseek64 return 0 on success, all other values mean failure.
            }

            return false;
        }

        int64_t Tell(FileHandle handle)
        {
            if(handle != InvalidFileHandle)
            {
                return _ftelli64((FILE*)handle);
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
                //TODO: should use fstat etc..
                int64_t pos = _ftelli64((FILE*)handle);
                int64_t size = 0;
                _fseeki64((FILE*)handle, 0, SEEK_END);
                size = _ftelli64((FILE*)handle);
                _fseeki64((FILE*)handle, pos, SEEK_SET);
                return size;
            }

            return 0;
        }
    }
OMAF_NS_END
