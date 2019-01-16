
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
#include "Foundation/NVRDeviceInfo.h"

#include "Foundation/NVRDependencies.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRClock.h"
#include "Foundation/NVRString.h"
#include "Foundation/NVRBase64.h"
#include "Graphics/NVRRenderBackend.h"
#include <comdef.h>
#include <stdlib.h>
#include <Wbemidl.h>
#include <string>
#include <cvt/wstring>
#include <codecvt>
#include <VersionHelpers.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "version.lib")

OMAF_NS_BEGIN
    namespace DeviceInfo
    {
        static FixedString256 sOSName;
        static FixedString256 sOSVersion;
        static FixedString256 sDeviceModel;
        static FixedString256 sDeviceType;
        static FixedString256 sDevicePlatformInfo;
        static FixedString256 sDevicePlatformId;
        static FixedString256 sAppId;
        static FixedString256 sAppVersion;

        const LPCTSTR kPCKey = "HARDWARE\\DESCRIPTION\\System\\BIOS\\";
        const LPCTSTR kCpuKey = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\\";
        const int32_t kMaxLength = 2048;
        static const char_t* kUserPath = "Users\\"; // the real path seems to be \\Users\\username\\ even if language is not English, although it appears in Explorer as localized path

        void initialize()
        {
            //just pre-fetch everything.
            getOSName();
            getOSVersion();
            getDeviceModel();
            getDeviceType();
            getDevicePlatformInfo();
            getDevicePlatformId();
            getAppId();
        }
        void shutdown()
        {
            //do nothing.
        }
        
        bool_t readRegistryString(LPCTSTR mainKey, const char_t* key, FixedString2048& value)
        {
            HKEY hKey;
            LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(mainKey), 0, KEY_READ, &hKey);
            if (result != 0)
            {
                return false;
            }
            DWORD bytes = kMaxLength;
            char_t out[kMaxLength];
            DWORD type = REG_SZ;
            result = RegQueryValueEx(hKey, TEXT(key), NULL, &type, (LPBYTE)&out, &bytes);
            if (result == 0)
            {
                value.append(out, (size_t)bytes);
            }
            RegCloseKey(hKey);
            return (0 == result);
        }


        static void readGPUCPUInfo()
        {
            HRESULT hres;

            // Step 1: --------------------------------------------------
            // Initialize COM. ------------------------------------------

            hres = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (FAILED(hres))
            {
                return;
            }

            // Step 2: --------------------------------------------------
            // Set general COM security levels --------------------------

            hres = CoInitializeSecurity(
                NULL,
                -1,                          // COM authentication
                NULL,                        // Authentication services
                NULL,                        // Reserved
                RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
                RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
                NULL,                        // Authentication info
                EOAC_NONE,                   // Additional capabilities 
                NULL                         // Reserved
            );


            if (FAILED(hres))
            {
                CoUninitialize();
                return;
            }

            // Step 3: ---------------------------------------------------
            // Obtain the initial locator to WMI -------------------------

            IWbemLocator *pLoc = NULL;

            hres = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, (LPVOID *)&pLoc);

            if (FAILED(hres))
            {
                CoUninitialize();
                return;
            }

            // Step 4: -----------------------------------------------------
            // Connect to WMI through the IWbemLocator::ConnectServer method

            IWbemServices *pSvc = NULL;

            // Connect to the root\cimv2 namespace with
            // the current user and obtain pointer pSvc
            // to make IWbemServices calls.
            hres = pLoc->ConnectServer(
                _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
                NULL,                    // User name. NULL = current user
                NULL,                    // User password. NULL = current
                0,                       // Locale. NULL indicates current
                NULL,                    // Security flags.
                0,                       // Authority (for example, Kerberos)
                0,                       // Context object 
                &pSvc                    // pointer to IWbemServices proxy
            );

            if (FAILED(hres))
            {
                pLoc->Release();
                CoUninitialize();
                return;
            }

            // Step 5: --------------------------------------------------
            // Set security levels on the proxy -------------------------

            hres = CoSetProxyBlanket(
                pSvc,                        // Indicates the proxy to set
                RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
                RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
                NULL,                        // Server principal name 
                RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
                RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
                NULL,                        // client identity
                EOAC_NONE                    // proxy capabilities 
            );

            if (FAILED(hres))
            {
                pSvc->Release();
                pLoc->Release();
                CoUninitialize();
                return;               // Program has failed.
            }

            // Step 6: --------------------------------------------------
            // Use the IWbemServices pointer to make requests of WMI ----

            // For example, get the name of the operating system
            IEnumWbemClassObject* pEnumerator = NULL;
            hres = pSvc->ExecQuery(
                bstr_t("WQL"),
                bstr_t("SELECT * FROM Win32_") + L"VideoController",
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pEnumerator);

            if (FAILED(hres))
            {
                pSvc->Release();
                pLoc->Release();
                CoUninitialize();
                return;
            }

            // Step 7: -------------------------------------------------
            // Get the data from the query in step 6 -------------------

            IWbemClassObject *pclsObj = NULL;
            ULONG uReturn = 0;

            // we could read e.g. these properties (sample values given):
            //Caption = NVIDIA GeForce GTX 1080
            //Description = NVIDIA GeForce GTX 1080
            //DriverVersion = 21.21.13.7254
            //VideoModeDescription = 2560 x 1440 x 4294967296 colors
            //VideoProcessor = GeForce GTX 1080

            std::wstring key(L"VideoProcessor");
            std::string gpuName;

            while (pEnumerator)
            {
                HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

                if (0 == uReturn)
                {
                    break;
                }

                VARIANT vtProp;

                CIMTYPE type = CIM_ILLEGAL;

                hr = pclsObj->Get(key.c_str(), 0, &vtProp, &type, 0);
                if (hr == S_OK) {
                    if (type == CIM_STRING) {
                        int len = SysStringLen(vtProp.bstrVal);
                        std::wstring ws(vtProp.bstrVal, len);
                        gpuName.assign(ws.begin(), ws.end());
                        break;
                    }
                }
                VariantClear(&vtProp);
                pclsObj->Release();
            }

            FixedString2048 cpu;
            readRegistryString(kCpuKey, "ProcessorNameString", cpu);

            char_t name[256];
            sprintf(name, "%s / %s", gpuName.c_str(), cpu.getData());

            sDevicePlatformId.append(name);

            // Cleanup
            // ========

            pSvc->Release();
            pLoc->Release();
            pEnumerator->Release();
            CoUninitialize();

        }
        const void readOSInfo()
        {
            char_t version[256];

            /*https://msdn.microsoft.com/en-us/library/windows/desktop/ms724429(v=vs.85).aspx
            "To obtain the full version number for the operating system, call the GetFileVersionInfo function on one of the system DLLs, such as Kernel32.dll,
            then call VerQueryValue to obtain the \\StringFileInfo\\<lang><codepage>\\ProductVersion subblock of the file version information.*/
            wchar_t fname[512];
            DWORD versize = 0;
            if (0 != GetSystemDirectoryW(fname, 512))
            {
                wcscat(fname, L"\\ntoskrnl.exe"); //Use the actual kernel version.. hopefully thats the most correct one, since Kernel32.dll,user32.dll etc all give DIFFERENT values. (the UBR is different... ie. the build/patch/revision)
                versize = GetFileVersionInfoSizeW(fname, NULL);
            }
                
            if (0 == versize)                
            {
                //could not get system directory... try this then..
                GetModuleFileNameW(GetModuleHandleW(L"Kernel32.dll"), fname, 512);
                versize = GetFileVersionInfoSizeW(fname, NULL);
            }                
                
            void* verinfo = malloc(versize);
            GetFileVersionInfoW(fname, 0, versize, verinfo);
                
            //Yes, i am using the VS_FIXEFILEINFO instead of the translated string...
            unsigned int ifnolen;
            VS_FIXEDFILEINFO* ifno;
            VerQueryValueW(verinfo, L"\\", (void**)&ifno, &ifnolen);

            //ALSO https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832(v=vs.85).aspx
            //6.0 is vista
            //6.1 is win7
            //6.2 is win8
            //6.3 is win8.1 ... (and 6.1 is also win10. some apis and registry use 6.3 for currentversion...)
            //10.0 is win10 ....
                
                
            //Try to fetch human readable name from registry. (technically undocumented, and MS could move it somewhere else...)
            char prodname[128] = { 0 };
            HKEY hKey;
            DWORD bytes = sizeof(prodname);
            DWORD type = REG_SZ;
            LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);
            if (ERROR_SUCCESS == result)
            {
                result = RegQueryValueExW(hKey, L"ProductName", NULL, &type, (LPBYTE)&prodname, &bytes);
                RegCloseKey(hKey);
                prodname[bytes] = 0;
                stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
                std::string tmp2;                    
                tmp2 = convert.to_bytes((wchar_t*)prodname);
                strcpy(prodname, tmp2.c_str());
            }
            if (ERROR_SUCCESS != result)
            {
                //could not fetch name from registry, so just use plain windows...
                strcpy(prodname, "Windows");
            }

            sprintf(version, "%d.%d.%d.%d %s", ifno->dwProductVersionMS >> 16, ifno->dwProductVersionMS & 0xFFFF, ifno->dwProductVersionLS >> 16, ifno->dwProductVersionLS & 0xFFFF, IsWindowsServer()?"Server":"Workstation");

            free(verinfo);
            sOSName = prodname;
            sOSVersion = version;
        }
        const FixedString256& getOSName()
        {
            if (sOSName.isEmpty())
            {
                readOSInfo();
            }
            return sOSName;
        }
        const FixedString256& getOSVersion()
        {
            if (sOSVersion.isEmpty())
            {
                readOSInfo();
            }
            return sOSVersion;
        }

        const FixedString256& getDeviceModel()
        {
            if (sDeviceModel.isEmpty())
            {
                char_t name[256];

                FixedString2048 manu;
                readRegistryString(kPCKey, "SystemManufacturer", manu);
                FixedString2048 prod;
                readRegistryString(kPCKey, "SystemProductName", prod);
                FixedString2048 cpu;
                readRegistryString(kCpuKey, "ProcessorNameString", cpu);

                if (manu.getLength() > 0 && prod.getLength() > 0)
                {
                    sprintf(name, "%s / %s / %s", manu.getData(), prod.getData(), cpu.getData());
                }
                else
                {
                    sprintf(name, "PC / %s", cpu.getData());
                }
                sDeviceModel=name;
            }
            return sDeviceModel;
        }

        const FixedString256& getDeviceType()
        {
            if (sDeviceType.isEmpty())
            {
                sDeviceType.append("Windows");
            }
            return sDeviceType;
        }

        FixedString256 getUniqueId()
        {
            char_t unique[256];

            ::srand((unsigned)Clock::getRandomSeed());
            int32_t r1 = rand();
            int32_t r2 = rand();
            uint64_t ref = Clock::getSecondsSinceReference();     // on Windows this gives a semi-random value; the time since the PC was booted
            uint64_t timestamp = (uint32_t)Clock::getRandomSeed();// time in microseconds since 1601 gives further spread to the id

            sprintf(unique, "Windows%d%d%lld%lld", r1, r2, ref, timestamp);
            String inputString(*MemorySystem::DefaultHeapAllocator(), unique);
            String baseString(*MemorySystem::DefaultHeapAllocator());
            Base64::encode(inputString, baseString);

            return FixedString256(baseString);
        }

        const FixedString256& getAppBuildNumber()
        {
            if (sAppVersion.isEmpty())
            {
                sAppVersion = "Unknown";
            }
            return sAppVersion;
        }
        const FixedString256& getAppId()
        {
            if (sAppId.isEmpty())
            {
                // there is no package name in Windows desktop apps, so use the execution path instead
                char_t path[2048];
                GetModuleFileName(NULL, path, 2048);
                String pathString(*MemorySystem::DefaultHeapAllocator(), path);
                size_t appNameIndex = pathString.findLast("\\");
                size_t origLength = pathString.getLength();

                // we need to replace '\\' with '.' in the path, since otherwise at least MixPanel rejects the data
                FixedString256 appIdString;
                size_t start = 3; // skip over 'c:\\'
                if (origLength > 256)
                {
                    start = pathString.findFirst("\\", origLength - 256) + 1;
                }
                if (pathString.findFirst(kUserPath) != Npos)
                {
                    // skip over username in the path so we won't log that

                    start += pathString.findFirst(kUserPath, start) + StringLength(kUserPath);
                    // now we are at c:\\Users\\, then skip over the usernam
                    start += pathString.findFirst("\\", start) + 1;
                }
                while (start < appNameIndex)
                {
                    // findFirst returns the relative index to the start index, not the absolute index
                    size_t pathName = pathString.findFirst("\\", start);

                    appIdString.append(pathString.substring(start, pathName));
                    appIdString.append(".");
                    start += pathName + 1;
                }
                appIdString.append(pathString.substring(appNameIndex + 1, origLength - appNameIndex - 5));  // exclude the .exe
                sAppId=appIdString;
            }

            return sAppId;
        }

        const FixedString256& getDevicePlatformId()
        {
            if (sDevicePlatformId.isEmpty())
            {
                readGPUCPUInfo();
            }
            return sDevicePlatformId;
        }

        const FixedString256& getDevicePlatformInfo()
        {
            if (sDevicePlatformInfo.isEmpty())
            {
                sDevicePlatformInfo=getDeviceModel();
            }
            return sDevicePlatformInfo;
        }

        bool_t deviceSupports2VideoTracks()
        {
            return true;
        }

        bool_t deviceSupportsHEVC()
        {
            // TODO: Check if not Win10 and then return false
            return true;
        }

        bool_t deviceCanReconfigureVideoDecoder()
        {
            return true;
        }

        LayeredVASTypeSupport::Enum deviceSupportsLayeredVAS()
        {
            return LayeredVASTypeSupport::FULL_STEREO;
        }

        uint32_t maxLayeredVASTileCount()
        {
            return 12; // would support 6x2 tile grid stereo
        }

        uint32_t maxDecodedPixelCountPerSecond()
        {
            RendererType::Enum renderType = RenderBackend::getRendererType();
            if (renderType == RendererType::D3D11)
            {
                if (getDevicePlatformId().findFirst("GTX 10") != Npos)
                {
                    // at least 6k@60 fps has been decoded on GTX 1080
                    return 6144 * 3072 * 60;
                }
                return 4 * 4096 * 2048 * 30; // 4 x 4k @ 30fps
            }
            else
            {
                return 2 * 4096 * 2048 * 30; // 2 x 4k @ 30fps
            }
        }

        bool_t deviceSupportsSubsegments()
        {
            return true;
        }

        bool_t isCurrentDeviceSupported()
        {
            return IsWindows8OrGreater();// || IsWindows10OrGreater();
        }

    }
OMAF_NS_END
