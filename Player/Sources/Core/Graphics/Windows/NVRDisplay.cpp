
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
#include "Graphics/NVRDisplay.h"

#include <ShellScalingApi.h>

extern "C"
{
    //Export magic flags to enable highperformace displayadapter selection on laptops.
    //http://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    //http://gpuopen.com/amdpowerxpressrequesthighperformance/    
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

OMAF_NS_BEGIN
typedef HRESULT(WINAPI* GetProcessDPIAwareness_t)(_In_opt_ HANDLE hprocess, _Out_ PROCESS_DPI_AWARENESS *value);
typedef HRESULT(WINAPI* GetDpiForMonitor_t)(_In_ HMONITOR hmonitor,_In_ MONITOR_DPI_TYPE dpiType,_Out_ UINT *dpiX,_Out_ UINT *dpiY);

static float32_t getAwareness(GetProcessDPIAwareness_t getProcessDPIAwareness, GetDpiForMonitor_t getDpiForMonitor)
{
    PROCESS_DPI_AWARENESS awareness;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
    HRESULT hr = getProcessDPIAwareness(hProcess, &awareness);

    if (hr == S_OK)
    {
        BOOL isSystemDpiAware = (awareness == PROCESS_SYSTEM_DPI_AWARE);
        //NOTE: this is not actually correct, since it gets the dpi of monitor which draws the point (1,1). multimonitor setups will be wrong!
        POINT point;
        point.x = 1;
        point.y = 1;

        HMONITOR hMonitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);

        UINT dpiX = 0;
        UINT dpiY = 0;

        hr = getDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

        if (hr == S_OK)
        {
            const FLOAT baseDpi = 96.0f;

            return (FLOAT)dpiX / baseDpi;
        }
    }
    return 1.0f;
}

float32_t Display::getDisplayDpiScale()
{
    HMODULE hdll = LoadLibraryW(L"SHCore.dll");
    GetProcessDPIAwareness_t getProcessDPIAwareness = hdll ? (GetProcessDPIAwareness_t)GetProcAddress(hdll, "GetProcessDpiAwareness") : NULL;
    GetDpiForMonitor_t getDpiForMonitor = hdll ? (GetDpiForMonitor_t)GetProcAddress(hdll, "GetDpiForMonitor") : NULL;
    float32_t result = 1.0f;
    if ((getProcessDPIAwareness)&&(getDpiForMonitor))
    {
        result = getAwareness(getProcessDPIAwareness, getDpiForMonitor);
    }
    if (hdll) FreeLibrary(hdll);
    return result;
}

OMAF_NS_END
