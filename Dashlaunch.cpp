#include "stdafx.h"

// Excuse the mess. A lot of this would be simplified if launch.xex was updated to not rely on static paths for lhelper

BOOL bDLloaded = FALSE;
#define lhPath_USB "\\Device\\Mass0\\lhelper.xex"
#define lhPath_Mount_USB "Usb:\\lhelper.xex"

#define lhPath_HDD "\\Device\\Harddisk0\\Partition1\\lhelper.xex"
#define lhPath_Mount_HDD "Hdd:\\lhelper.xex"

// Credit to c0z - Some of this was adapted from the Dashlaunch 2.11 source
// Use functions taken from Dashlaunch to load Dashlaunch :-)

DWORD g_status[2]; // 0 = return status, 1 = completion signal
VOID* pLaunchXexData;
DWORD dwLaunchXexSize;

static VOID LoadDLPlugin()
{
	g_status[0] = XexLoadImageFromMemory(pLaunchXexData, dwLaunchXexSize, "launch.xex", XEX_LOADIMG_TYPE_SYSTEM_DLL, 0, NULL); // XEX_LOADIMG_FLAG_DLL
	g_status[1] = 1;
	__dcbst(0, g_status);
	__sync();
	__isync();
}

WCHAR wDLStatusBuf[150];
BOOL bDLisLoaded = FALSE;
INT SysLoadDashlaunch()
{
	ZeroMemory(wDLStatusBuf, sizeof(wDLStatusBuf));
	if (GetModuleHandle("launch.xex")) // Check it hasn't been loaded already
	{
		cprintf("[SysLoadDashlaunch] Already loaded!");
		BOOL blhHDD = FileExists(lhPath_HDD);
		swprintf_s(wDLStatusBuf, L"Dashlaunch: Loaded! %S", blhHDD ? "[HDD mode]" : "[USB mode] DO NOT UNPLUG THE USB!");
		bDLisLoaded = TRUE;
		return 2; // Already Loaded
	}

	cprintf("[SysLoadDashlaunch] Attempting to load launch.xex...");

	// Check if the console has a Hdd attached so we can copy lhelper there.
	// Ideally Hdd would be preferred to allow users to unplug the Usb without disrupting dashlaunch functionality.
	// If not, fallback to using Usb
	BOOL bUseHdd = (XboxHardwareInfo->Flags & 0x20) == 0x20;
	if (!FileExists(bUseHdd ? lhPath_HDD : lhPath_USB))
	{
		cprintf("[SysLoadDashlaunch] Attempting to copy lhelper.xex to %s root...", bUseHdd ? "HDD" : "USB");
		VOID* pLHData;
		DWORD dwLHSize;
		if (!XGetModuleSection(GetModuleHandle(NULL), bUseHdd ? "LHHDD" : "LHUSB", &pLHData, &dwLHSize))
		{
			cprintf("[SysLoadDashlaunch] Failed to load! Failed to get lhelper section data!");
			swprintf_s(wDLStatusBuf, L"Dashlaunch: FAILED! - Failed to write lhelper.xex. Section data error!");
			return 0; // Failed
		}

		if (bUseHdd)
		{
			if (!MountHdd())
			{
				cprintf("[SysLoadDashlaunch] Failed to load! Failed to create Hdd mount point!");
				swprintf_s(wDLStatusBuf, L"Dashlaunch: FAILED! - Failed to write lhelper.xex. Mount error!");
				return 0; // Failed
			}
		}
		else
		{
			if (!MountUsb())
			{
				cprintf("[SysLoadDashlaunch] Failed to load! Failed to create Usb mount point!");
				swprintf_s(wDLStatusBuf, L"Dashlaunch: FAILED! - Failed to write lhelper.xex. Mount error!");
				return 0; // Failed
			}
		}

		// Copy the embedded lhelper.xex to the root of storage device so the embedded launch.xex can use it once loaded
		if (!CWriteFile(bUseHdd ? lhPath_Mount_HDD : lhPath_Mount_USB, pLHData, dwLHSize)) // 7B error when not using a mount point
		{
			cprintf("[SysLoadDashlaunch] Failed to load! Failed to write lhelper.xex to %s root!", bUseHdd ? "HDD" : "USB");
			swprintf_s(wDLStatusBuf, L"Dashlaunch: FAILED! - failed to write lhelper.xex to storage device.");
			return 0; // Failed
		}
		cprintf("[SysLoadDashlaunch] lhelper.xex copied to %s root!", bUseHdd ? "HDD" : "USB");
		Sleep(200);
	}

	if (!XGetModuleSection(GetModuleHandle(NULL), bUseHdd ? "LXHDD" : "LXUSB", &pLaunchXexData, &dwLaunchXexSize))
	{
		cprintf("[SysLoadDashlaunch] Failed to load! Failed to get launch section data!");
		swprintf_s(wDLStatusBuf, L"Dashlaunch: FAILED! - Failed to load launch.xex. Section data error!");
		return 0; // Failed
	}

	HANDLE pthread;
	DWORD pthreadid;
	DWORD sta;
	g_status[0] = 0;
	g_status[1] = 0;
	__dcbst(0, g_status);
	__sync();
	__isync();

	sta = ExCreateThread(&pthread, 0, &pthreadid, (PVOID)XapiThreadStartup, (LPTHREAD_START_ROUTINE)LoadDLPlugin, NULL, 0x2);
	XSetThreadProcessor(pthread, 4);
	SetThreadPriority(pthread, THREAD_PRIORITY_TIME_CRITICAL);
	ResumeThread(pthread);
	CloseHandle(pthread);

	// wait for thread to run it's course
	while (g_status[1] == 0)
	{
		Sleep(100);
	}

	if (!g_status[0])
	{
		cprintf("[SysLoadDashlaunch] Loaded successfully");
		swprintf_s(wDLStatusBuf, L"Dashlaunch: Loaded! %S", bUseHdd ? "[HDD mode]" : "[USB mode] DO NOT UNPLUG THE USB!");
		bDLisLoaded = TRUE;
		return 1; // Success
	}
	else
	{
		cprintf("[SysLoadDashlaunch] Failed! Status: %X", g_status[0]);
		swprintf_s(wDLStatusBuf, L"Dashlaunch: FAILED! Status: %X", g_status[0]);
		return 0; // Failed
	}
}
