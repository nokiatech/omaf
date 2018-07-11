
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
#include "Foundation/NVRDeviceInfo.h"

#include "Foundation/NVRDependencies.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRClock.h"
#include "Foundation/NVRString.h"
#include "Foundation/NVRBase64.h"
#include <stdlib.h>
#include <ctype.h>

#include <sys/system_properties.h>
#include <Foundation/Android/NVRAndroid.h>


OMAF_NS_BEGIN

    static const FixedString32 HUAWEI_MANUFACTURER = "HUAWEI";
    static const FixedString32 GOOGLE_MANUFACTURER = "Google";

    // Used on US Galaxy S7s and MotoZ
    static const FixedString32 SNAPDRAGON_820 = "msm8996";

    // Used on US Galaxy S8
    static const FixedString32 SNAPDRAGON_835 = "msm8998";

    // Used on EU Galaxy S8
    static const FixedString32 EXYNOS_8895 = "universal8895";

    // Used on EU Galaxy S7
    static const FixedString32 EXYNOS_8890 = "universal8890";

    // Hisilicon 3660, used on Huawei Mate 9 Pro
    static const FixedString32 HI_3660 = "hi3660";

    // Google Pixel and Pixel XL have Snapdragon 82x, but the information is not available
    static const size_t GOOGLE_S820_MODEL_COUNT = 2;
    static const FixedString32 GOOGLE_S820_MODELS[GOOGLE_S820_MODEL_COUNT] = {"sailfish", "marlin"};

    namespace DeviceInfo
    {
        static FixedString256 sOSVersion;
        static FixedString256 sOSName;
        static FixedString256 sDeviceModel;
        static FixedString256 sDevicePlatformInfo;
        static FixedString256 sDevicePlatformId;
        static FixedString256 sAppId;
        static FixedString256 sAppBuildNumber;

        void initialize()
        {
            //just pre-fetch everything.
            getOSName();
            getDeviceModel();
            getDevicePlatformInfo();
            getDevicePlatformId();
            getAppId();
        }
        void shutdown()
        {
            //do nothing.
        }
        FixedString256 readProperty(const char_t* property)
        {
            FixedString256 value;
            JNIEnv* jEnv = Android::getJNIEnv();

            jclass newClass = jEnv->FindClass("android/os/Build");

            if (newClass == 0)
            {
                OMAF_ASSERT(false, "Can't find the build class");
            }
            jfieldID fieldID = jEnv->GetStaticFieldID(newClass, property, "Ljava/lang/String;");
            jstring javaString = static_cast<jstring>(jEnv->GetStaticObjectField(newClass, fieldID));

            const char *chars = jEnv->GetStringUTFChars(javaString, JNI_FALSE);
            value = FixedString<256>(chars);
            jEnv->ReleaseStringUTFChars(javaString, chars);
            return value;
        }

        const FixedString256& getOSVersion()
        {
            if (sOSVersion.isEmpty())
            {
                JNIEnv* jEnv = Android::getJNIEnv();

                jclass newClass = jEnv->FindClass("android/os/Build$VERSION");

                if (newClass == 0)
                {
                    OMAF_ASSERT(false, "Can't find the build class");
                }
                jfieldID fieldID = jEnv->GetStaticFieldID(newClass, "RELEASE", "Ljava/lang/String;");
                jstring javaString = static_cast<jstring>(jEnv->GetStaticObjectField(newClass, fieldID));

                const char *chars = jEnv->GetStringUTFChars(javaString, JNI_FALSE);
                sOSVersion = FixedString<256>(chars);
                jEnv->ReleaseStringUTFChars(javaString, chars);
            }
            return sOSVersion;
        }

        const FixedString256& getOSName()
        {
            if (sOSName.isEmpty())
            {
                sOSName = FixedString<256>("Android");
            }
            return sOSName;
        }

        const FixedString256& getDeviceModel()
        {
            if (sDeviceModel.isEmpty())
            {
                FixedString256 manuFacturer = readProperty("MANUFACTURER");
                FixedString256 model = readProperty("MODEL");
                sDeviceModel = manuFacturer;
                sDeviceModel.append(" ");
                sDeviceModel.append(model);
            }
            return sDeviceModel;
        }

        const FixedString256& getDeviceType()
        {
            return getDeviceModel();
        }

        FixedString256 getUniqueId()
        {
            char_t unique[256];

            ::srand((unsigned)Clock::getRandomSeed());
            int32_t r1 = rand();
            int32_t r2 = rand();
            int32_t r3 = rand();
            uint64_t timestamp = Clock::getRandomSeed();// epoch time in microseconds gives further spread to the id

            sprintf(unique, "Android%d%d%d%lld", r1,r2,r3,timestamp);
            String inputString(*MemorySystem::DefaultHeapAllocator(), unique);
            String baseString(*MemorySystem::DefaultHeapAllocator());
            Base64::encode(inputString, baseString);

            return FixedString256(baseString);
        }

        const FixedString256& getAppId()
        {
            if (sAppId.isEmpty())
            {
                jobject activity=Android::getActivity();
                JNIEnv* env=Android::getJNIEnv();
                jclass android_content_Context = env->GetObjectClass(activity);
                jmethodID midGetPackageName = env->GetMethodID(android_content_Context, "getPackageName", "()Ljava/lang/String;");
                jstring packageName= (jstring)env->CallObjectMethod(activity, midGetPackageName);
                const char* appname = env->GetStringUTFChars(packageName, NULL);
                sAppId=appname;
                env->ReleaseStringUTFChars(packageName, appname);
                env->DeleteLocalRef(packageName);
            }
            return sAppId;
        }

        const FixedString256& getAppBuildNumber()
        {
            if(sAppBuildNumber.isEmpty())
            {
                jobject activity=Android::getActivity();
                JNIEnv* env=Android::getJNIEnv();
                jclass android_content_Context = env->GetObjectClass(activity);
                jmethodID getPackageManagerID = env->GetMethodID(android_content_Context, "getPackageManager", "()Landroid/content/pm/PackageManager;");
                jobject packageManager = env->CallObjectMethod(activity, getPackageManagerID);
                jclass packageManagerClass = env->GetObjectClass(packageManager);
                jmethodID getPackageInfoID = env->GetMethodID(packageManagerClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");

                jmethodID midGetPackageName = env->GetMethodID(android_content_Context, "getPackageName", "()Ljava/lang/String;");
                jstring packageName= (jstring)env->CallObjectMethod(activity, midGetPackageName);
                jobject packageInfo = env->CallObjectMethod(packageManager, getPackageInfoID, packageName, 0);
                jclass packageInfoClass = env->GetObjectClass(packageInfo);

                jfieldID fieldId = env->GetFieldID(packageInfoClass, "versionCode", "I");
                int versionCode = env->GetIntField(packageInfo, fieldId);

                env->DeleteLocalRef(packageManager);
                env->DeleteLocalRef(packageInfo);
                sprintf(sAppBuildNumber.getData(), "%d", versionCode);
            }
            return sAppBuildNumber;
        }

        const FixedString256& getDevicePlatformId()
        {
            if (sDevicePlatformId.isEmpty())
            {
                // Huawei writes its chipset info to the HARDWARE field
                // Samsung writes its info to both (but with a different string)
                // QC writes its info to BOARD - except with Google devices where QC info is on ro.board.platform,
                // that is not accessible via public APIs (only via Android.os.SystemProperties)
                // So use HARWARE for Huawei, hardcoded model nicknames for Google, and BOARD for the rest
                if (readProperty("MANUFACTURER").findFirst(HUAWEI_MANUFACTURER) != Npos)
                {
                    sDevicePlatformId = readProperty("HARDWARE");
                }
                else if (readProperty("MANUFACTURER").findFirst(GOOGLE_MANUFACTURER) != Npos)
                {
                    FixedString256 googleModel = readProperty("BOARD");
                    for (size_t i = 0; i < GOOGLE_S820_MODEL_COUNT; i++)
                    {
                        if (googleModel.findFirst(GOOGLE_S820_MODELS[i]) != Npos)
                        {
                            sDevicePlatformId.append(SNAPDRAGON_820);
                            break;
                        }
                    }
                }
                else
                {
                    sDevicePlatformId = readProperty("BOARD");
                }
            }
            return sDevicePlatformId;
        }

        static int32_t getCPUCoreCount() {
            int32_t count = 0;
            FILE *presentFile = fopen("/sys/devices/system/cpu/present", "r");
            if (presentFile != NULL) {
                int pmin = 0;
                int pmax = 0;
                if (fscanf(presentFile, "%d-%d", &pmin, &pmax) == 2) {
                    count = pmax;
                }
                fclose(presentFile);
            }
            return count + 1;
        }

        static int32_t readCpuFreq(bool searchMaxValue) {
            int32_t speed = INT_MIN;

            if (!searchMaxValue)
                speed = INT_MAX;

            for (int i = 0; i < getCPUCoreCount(); i++) {
                char_t cfile[128];
                sprintf(cfile, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
                FILE *freqFile = fopen(cfile, "r");
                if (freqFile != NULL) {
                    int freq = 0;
                    if (fscanf(freqFile, "%d", &freq) == 1) {
                        freq /= 1000;
                        if (searchMaxValue) {
                            if (freq > speed) speed = freq;
                        } else {
                            if (freq < speed) speed = freq;
                        }
                    }
                    fclose(freqFile);
                }
            }
            return speed;
        }

        static int32_t getCPUMinSpeed() {
            return readCpuFreq(false);
        }

        static int32_t getCPUMaxSpeed() {
            return readCpuFreq(true);
        }

        const FixedString256& getDevicePlatformInfo()
        {
            if (sDevicePlatformInfo.isEmpty())
            {
                char_t name[256];
                FixedString256 platform = getDevicePlatformId();
                sprintf(name, "%s, %d cores, %d - %d MHz", platform.getData(), getCPUCoreCount(),
                        getCPUMinSpeed(), getCPUMaxSpeed());
                sDevicePlatformInfo=name;
            }
            return sDevicePlatformInfo;
        }

        bool_t deviceSupports2VideoTracks() {
            const FixedString256& device = getDevicePlatformId();
            if (device.findFirst(SNAPDRAGON_820) != Npos
                || device.findFirst(EXYNOS_8895) != Npos
                || device.findFirst(HI_3660) != Npos
                || device.findFirst(SNAPDRAGON_835) != Npos)
            {
                return true;
            }
            return false;
        }

        bool_t deviceSupportsHEVC()
        {
            return true;
        }

        bool_t deviceCanReconfigureVideoDecoder()
        {
            const FixedString256& device = getDevicePlatformId();
            if (device.findFirst(SNAPDRAGON_820) != Npos)
            {
                const FixedString256& osName = getOSVersion();
                if (osName.findFirst("6") == 0)
                {
                    // Snapdragon 820 on Android 6.0.1 has problems with reusing video decoders
                    return false;
                }
                return true;
            }
            else if (device.findFirst(EXYNOS_8895) != Npos
                    || device.findFirst(HI_3660) != Npos
                    || device.findFirst(SNAPDRAGON_835) != Npos
                    || device.findFirst(EXYNOS_8890) != Npos)
            {
                return true;
            }
            return false;
        }

        LayeredVASTypeSupport::Enum deviceSupportsLayeredVAS()
        {
            const FixedString256& device = getDevicePlatformId();
            if (device.findFirst(EXYNOS_8895) != Npos
                || device.findFirst(HI_3660) != Npos
                || device.findFirst(SNAPDRAGON_835) != Npos)
            {
                return LayeredVASTypeSupport::FULL_STEREO;
            }
            else if (device.findFirst(SNAPDRAGON_820) != Npos
                || device.findFirst(EXYNOS_8890) != Npos)
            {
                return LayeredVASTypeSupport::LIMITED;
            }
            return LayeredVASTypeSupport::NOT_SUPPORTED;
        }

        uint32_t maxLayeredVASTileCount()
        {
            const FixedString256& device = getDevicePlatformId();
            // base layer(s) decoders and streams are on top of this
            if (device.findFirst(EXYNOS_8895) != Npos
                || device.findFirst(HI_3660) != Npos
                || device.findFirst(SNAPDRAGON_835) != Npos)
            {
                return 6;
            }
            else if (device.findFirst(SNAPDRAGON_820) != Npos
                     || device.findFirst(EXYNOS_8890) != Npos)
            {
                return 4;
            }
            else
            {
                return 0;
            }
        }

        uint32_t maxDecodedPixelCountPerSecond()
        {
            const FixedString256& device = getDevicePlatformId();
            if (device.findFirst(EXYNOS_8895) != Npos
                || device.findFirst(HI_3660) != Npos)
            {
                return 4*4096*2048*30;
            }
            else if (device.findFirst(SNAPDRAGON_820) != Npos
                 || device.findFirst(SNAPDRAGON_835) != Npos)
            {
                return 2*4096*2048*30;    //2xUHD @30 fps
            }
            else if (device.findFirst(EXYNOS_8890) != Npos)
            {
                return 1.6f*4096*2048*30;    //2xUHD @~26 fps
            }
            else
            {
                return 4096*2048*30;
            }
        }

        bool_t deviceSupportsSubsegments()
        {
            // Technically, Android would support subsegments, but as subsegments involve heavy index downloading operations, that causes noticeable performance hit
            return false;
        }

        bool_t isCurrentDeviceSupported()
        {
            return true;
        }

    }
OMAF_NS_END
