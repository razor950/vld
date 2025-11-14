////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Detours Implementation WITHOUT Pattern Matching
//
//  This is an EVEN CLEANER approach that eliminates pattern matching entirely
//  by hooking at a different level.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <windows.h>
#include <detours.h>

#pragma comment(lib, "detours.lib")

extern VisualLeakDetector g_vld;

////////////////////////////////////////////////////////////////////////////////
// APPROACH 1: Hook LdrLoadDll Instead of LdrpCallInitRoutine
//
// This is cleaner because LdrLoadDll IS exported and can be found easily.
// We already hook this in VLD, but we can enhance it to call RefreshModules.
////////////////////////////////////////////////////////////////////////////////

// Original LdrLoadDll from ntdll (already hooked by VLD)
typedef NTSTATUS (NTAPI *PLdrLoadDll)(
    IN PWCHAR PathToFile OPTIONAL,
    IN ULONG Flags OPTIONAL,
    IN PUNICODE_STRING ModuleFileName,
    OUT PHANDLE ModuleHandle);

static PLdrLoadDll g_pOriginalLdrLoadDll = NULL;

NTSTATUS NTAPI LdrLoadDll_Hooked(
    IN PWCHAR PathToFile OPTIONAL,
    IN ULONG Flags OPTIONAL,
    IN PUNICODE_STRING ModuleFileName,
    OUT PHANDLE ModuleHandle)
{
    // Refresh modules BEFORE loading the DLL
    // This ensures we catch all allocations
    g_vld.RefreshModules();

    // Call the original LdrLoadDll
    NTSTATUS status = g_pOriginalLdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);

    // Refresh again AFTER loading to catch the new DLL
    if (NT_SUCCESS(status)) {
        g_vld.RefreshModules();
    }

    return status;
}

BOOL InstallLdrLoadDllHook()
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll)
        return FALSE;

    // LdrLoadDll IS exported - no pattern matching needed!
    g_pOriginalLdrLoadDll = (PLdrLoadDll)GetProcAddress(hNtdll, "LdrLoadDll");
    if (!g_pOriginalLdrLoadDll)
        return FALSE;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    LONG error = DetourAttach(&(PVOID&)g_pOriginalLdrLoadDll,
                              (PVOID)LdrLoadDll_Hooked);

    if (error != NO_ERROR) {
        DetourTransactionAbort();
        return FALSE;
    }

    return (DetourTransactionCommit() == NO_ERROR);
}

////////////////////////////////////////////////////////////////////////////////
// APPROACH 2: Use Detours' Built-in Module Enumeration
//
// Instead of trying to intercept LdrpCallInitRoutine, periodically refresh
// the module list. Detours provides DetourEnumerateModules for this.
////////////////////////////////////////////////////////////////////////////////

static HANDLE g_hRefreshThread = NULL;
static BOOL g_bStopRefresh = FALSE;

DWORD WINAPI ModuleRefreshThread(LPVOID lpParameter)
{
    while (!g_bStopRefresh) {
        // Refresh module list periodically
        g_vld.RefreshModules();

        // Sleep for a short time
        Sleep(10); // 10ms - fast enough to catch most DLL loads
    }
    return 0;
}

BOOL StartModuleRefreshThread()
{
    g_bStopRefresh = FALSE;
    g_hRefreshThread = CreateThread(NULL, 0, ModuleRefreshThread, NULL, 0, NULL);
    return (g_hRefreshThread != NULL);
}

VOID StopModuleRefreshThread()
{
    g_bStopRefresh = TRUE;
    if (g_hRefreshThread) {
        WaitForSingleObject(g_hRefreshThread, 1000);
        CloseHandle(g_hRefreshThread);
        g_hRefreshThread = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
// APPROACH 3: Use DLL Notification Callbacks (Windows 8+)
//
// This is the CLEANEST approach - no hooking at all!
// Windows provides LdrRegisterDllNotification for this exact purpose.
////////////////////////////////////////////////////////////////////////////////

typedef VOID (CALLBACK *PLDR_DLL_NOTIFICATION_FUNCTION)(
    ULONG NotificationReason,
    PVOID NotificationData,
    PVOID Context);

typedef NTSTATUS (NTAPI *PLdrRegisterDllNotification)(
    ULONG Flags,
    PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    PVOID Context,
    PVOID *Cookie);

typedef NTSTATUS (NTAPI *PLdrUnregisterDllNotification)(
    PVOID Cookie);

#define LDR_DLL_NOTIFICATION_REASON_LOADED   1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA {
    ULONG Flags;
    PUNICODE_STRING FullDllName;
    PUNICODE_STRING BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

static PVOID g_pDllNotificationCookie = NULL;

VOID CALLBACK DllNotificationCallback(
    ULONG NotificationReason,
    PVOID NotificationData,
    PVOID Context)
{
    if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED) {
        // A DLL was just loaded - refresh our module list
        g_vld.RefreshModules();
    }
}

BOOL RegisterDllNotification()
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll)
        return FALSE;

    // LdrRegisterDllNotification IS exported - no pattern matching!
    PLdrRegisterDllNotification pLdrRegisterDllNotification =
        (PLdrRegisterDllNotification)GetProcAddress(hNtdll, "LdrRegisterDllNotification");

    if (!pLdrRegisterDllNotification)
        return FALSE; // Only available on Windows 8+

    NTSTATUS status = pLdrRegisterDllNotification(
        0,
        DllNotificationCallback,
        NULL,
        &g_pDllNotificationCookie);

    return NT_SUCCESS(status);
}

VOID UnregisterDllNotification()
{
    if (!g_pDllNotificationCookie)
        return;

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll)
        return;

    PLdrUnregisterDllNotification pLdrUnregisterDllNotification =
        (PLdrUnregisterDllNotification)GetProcAddress(hNtdll, "LdrUnregisterDllNotification");

    if (pLdrUnregisterDllNotification) {
        pLdrUnregisterDllNotification(g_pDllNotificationCookie);
        g_pDllNotificationCookie = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
// RECOMMENDED: Use the DLL Notification approach for Windows 8+,
//              fall back to LdrLoadDll hooking for Windows 7
////////////////////////////////////////////////////////////////////////////////

#define _DECL_DLLMAIN
#include <process.h>
#pragma comment(linker, "/entry:DllEntryPoint_NoPatternMatching")

__declspec(noinline)
BOOL WINAPI DllEntryPoint_NoPatternMatching(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        // Try the cleanest approach first (Windows 8+)
        if (!RegisterDllNotification()) {
            // Fall back to hooking LdrLoadDll (Windows 7)
            if (!InstallLdrLoadDllHook()) {
                // Last resort: start refresh thread
                StartModuleRefreshThread();
            }
        }
    }

    // Call CRT initialization
    if (fdwReason == DLL_PROCESS_ATTACH || fdwReason == DLL_THREAD_ATTACH)
        if (!_CRT_INIT(hinstDLL, fdwReason, lpReserved))
            return FALSE;

    if (fdwReason == DLL_PROCESS_DETACH || fdwReason == DLL_THREAD_DETACH)
        if (!_CRT_INIT(hinstDLL, fdwReason, lpReserved))
            return FALSE;

    if (fdwReason == DLL_PROCESS_DETACH) {
        // Clean up based on what we installed
        UnregisterDllNotification();
        StopModuleRefreshThread();
        // LdrLoadDll hook cleanup would go here if we installed it
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// COMPARISON: Pattern Matching Requirements
////////////////////////////////////////////////////////////////////////////////

/*
┌─────────────────────────────────────────────────────────────────────┐
│ APPROACH               │ Pattern Matching? │ Windows Support       │
├─────────────────────────────────────────────────────────────────────┤
│ Original (Manual)      │ ✗ YES - Extensive │ 7, 8, 10, 11 (fragile)│
│ Detours + LdrpCallInit │ ✗ YES - Minimal   │ 7, 8, 10, 11          │
│ Hook LdrLoadDll        │ ✓ NO              │ 7, 8, 10, 11          │
│ DLL Notifications      │ ✓ NO              │ 8, 10, 11 only        │
│ Refresh Thread         │ ✓ NO              │ All versions          │
└─────────────────────────────────────────────────────────────────────┘

RECOMMENDED STRATEGY:
1. Try LdrRegisterDllNotification (Windows 8+) - ZERO pattern matching
2. Fall back to hooking LdrLoadDll (Windows 7) - ZERO pattern matching
3. Last resort: Refresh thread - ZERO pattern matching

ALL THREE APPROACHES REQUIRE ZERO PATTERN MATCHING!
*/

////////////////////////////////////////////////////////////////////////////////
// EXAMPLE: Complete Integration
////////////////////////////////////////////////////////////////////////////////

/*
PSEUDOCODE showing the best approach:

DllMain() {
    if (DLL_PROCESS_ATTACH) {
        // 1st choice: Official Windows API (Win8+)
        if (LdrRegisterDllNotification available) {
            RegisterDllNotification();
            return; // ✓ Done! Zero pattern matching
        }

        // 2nd choice: Hook exported function (Win7+)
        if (LdrLoadDll found via GetProcAddress) {
            InstallLdrLoadDllHook();
            return; // ✓ Done! Zero pattern matching
        }

        // 3rd choice: Polling (all Windows)
        StartModuleRefreshThread();
        return; // ✓ Done! Zero pattern matching
    }
}

NO PATTERN MATCHING IN ANY CASE!
*/
