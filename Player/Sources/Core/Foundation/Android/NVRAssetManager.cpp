
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
#include "Foundation/NVRAssetManager.h"

#include "Foundation/Android/NVRAndroid.h"
#include "Foundation/NVRDependencies.h"
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    namespace AssetManager
    {
        FileHandle InvalidFileHandle = NULL;

        bool_t FileExists(const char_t* path)
        {
            AAssetManager* assetManager = Android::getAssetManager();

            OMAF_ASSERT_NOT_NULL(assetManager);

            AAsset* asset = AAssetManager_open(assetManager, path, AASSET_MODE_BUFFER);

            bool_t fileExists = (asset != InvalidFileHandle);

            if (fileExists)
            {
                AAsset_close(asset);
            }

            return fileExists;
        }

        bool_t DirExists(const char_t* path)
        {
            AAssetManager* assetManager = Android::getAssetManager();

            OMAF_ASSERT_NOT_NULL(assetManager);

            AAssetDir* assetDir = AAssetManager_openDir(assetManager, path);

            bool dirExists = AAssetDir_getNextFileName(assetDir) != NULL;

            AAssetDir_close(assetDir);

            return dirExists;
        }

        PathName GetFullPath(const char_t* filename)
        {
            OMAF_UNUSED_VARIABLE(filename);
            OMAF_ASSERT_NOT_IMPLEMENTED();

            return NULL;
        }

        FileHandle Open(const char_t* filename, FileSystem::AccessMode::Enum mode)
        {
            OMAF_ASSERT(mode == FileSystem::AccessMode::READ, "Only read mode is supported");
            OMAF_UNUSED_VARIABLE(mode);

            AAssetManager* assetManager = Android::getAssetManager();
            OMAF_ASSERT_NOT_NULL(assetManager);

            AAsset* asset = AAssetManager_open(assetManager, filename, AASSET_MODE_BUFFER);

            return asset;
        }

        void_t Close(FileHandle handle)
        {
            if(handle != InvalidFileHandle)
            {
                AAsset_close((AAsset*)handle);
                handle = InvalidFileHandle;
            }
        }

        int64_t Read(FileHandle handle, void_t* destination, int64_t bytes)
        {
            if(handle != InvalidFileHandle)
            {
                return AAsset_read((AAsset*)handle, destination, bytes);
            }

            return 0;
        }

        bool_t Seek(FileHandle handle, int64_t offset)
        {
            if(handle != InvalidFileHandle)
            {
                off64_t result = AAsset_seek64((AAsset*)handle, offset, SEEK_CUR);

                return (result != (off64_t)-1);
            }

            return false;
        }

        int64_t Tell(FileHandle handle)
        {
            if(handle != InvalidFileHandle)
            {
                return AAsset_seek64((AAsset*)handle, 0, SEEK_CUR);
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
                return AAsset_getLength64((AAsset*)handle);
            }

            return 0;
        }
    }
OMAF_NS_END
