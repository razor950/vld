////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Detours-based Implementation Header
//  Drop-in replacement for manual binary patching logic
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef VLD_DETOURS_H
#define VLD_DETOURS_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
// Public API - Drop-in replacement functions
////////////////////////////////////////////////////////////////////////////////

// InstallDetours - Installs the Detours hook for LdrpCallInitRoutine
//
//   Replaces: NtDllPatch() from original implementation
//
//   Return Value:
//     Returns TRUE if the hook was successfully installed, FALSE otherwise.
//
BOOL InstallDetours();

// UninstallDetours - Removes the Detours hook
//
//   Replaces: NtDllRestore() from original implementation
//
//   Return Value:
//     Returns TRUE if the hook was successfully removed, FALSE otherwise.
//
BOOL UninstallDetours();

// DllEntryPoint_Detours - DLL entry point using Detours
//
//   Replaces: DllEntryPoint() from original implementation
//
//   This is the actual DLL entry point that should be used with:
//   #pragma comment(linker, "/entry:DllEntryPoint_Detours")
//
BOOL WINAPI DllEntryPoint_Detours(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

////////////////////////////////////////////////////////////////////////////////
// Migration helper - allows gradual transition
////////////////////////////////////////////////////////////////////////////////

// SetDetoursMode - Enable or disable Detours mode at runtime
//
//   Useful for A/B testing or gradual rollout
//
//   Parameters:
//     bUseDetours - TRUE to use Detours, FALSE to use original implementation
//
void SetDetoursMode(BOOL bUseDetours);

// IsDetoursMode - Check if Detours mode is enabled
//
//   Return Value:
//     Returns TRUE if Detours mode is enabled, FALSE otherwise.
//
BOOL IsDetoursMode();

#ifdef __cplusplus
}
#endif

#endif // VLD_DETOURS_H
