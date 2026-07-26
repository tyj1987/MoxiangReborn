// hackshield_stub.hpp - Software-only stub for nProtect HackShield 4.0.
//
// 1:1 ABI drop-in for HShield.h (AhnLab HackShield 4.0 SDK, 2006 build).
// The original HShield.h ships with the legacy client and the underlying
// HShield.lib/EHsvc.dll vendor binaries are missing from the workspace.
//
// All stubs return HS_ERR_OK so the modern client boots through the
// HackShield gate without invoking the real anti-cheat engine. This is a
// temporary bridge until someone provides the missing .lib/.dll.
//
// Reference: ?? [Client]MH\HShield.h (lines 200-310 for the API list).
//            ?? [Client]MH\HackShieldManager.cpp (consumer side).

#pragma once

#include <cstdint>

// ============================================================
// Return codes (from HShield.h:75-110)
// ============================================================
namespace mxh::crypto::hackshield {

inline constexpr unsigned int HS_ERR_OK = 0x00000000;

}  // namespace mxh::crypto::hackshield

// ============================================================
// C ABI exports (linker symbols must match HShield.h exactly).
// ============================================================
extern "C" {

using PFN_AhnHS_Callback = int (__stdcall*)(long, long, void*);

int __stdcall _AhnHS_Initialize(const char* szFileName,
                               PFN_AhnHS_Callback pfn_Callback,
                               int nGameCode,
                               const char* szLicenseKey,
                               unsigned int unOption,
                               unsigned int unSHackSensingRatio);

int __stdcall _AhnHS_StartService();
int __stdcall _AhnHS_StopService();
int __stdcall _AhnHS_PauseService(unsigned int unPauseOption);
int __stdcall _AhnHS_ResumeService(unsigned int unResumeOption);
int __stdcall _AhnHS_Uninitialize();

int __stdcall _AhnHS_MakeAckMsg(unsigned char* pbyReqMsg,
                             unsigned char* pbyAckMsg);

int __stdcall _AhnHS_MakeGuidAckMsg(unsigned char* pbyGuidReqMsg,
                                   unsigned char* pbyGuidAckMsg);

int __stdcall _AhnHS_SaveFuncAddress(unsigned int unNumberOfFunc, ...);

int __stdcall _AhnHS_CheckAPIHooked(const char* szModuleName,
                                 const char* szFunctionName,
                                 const char* szSpecificPath);

}  // extern "C"

// ============================================================
// High-level convenience wrappers (from HackShieldManager.h:7-15).
// ============================================================
extern "C" int __stdcall HS_CallbackProc(long lCode, long lParamSize, void* pParam);

// BOOLEAN return aliases used by HackShieldManager.h
inline int __stdcall HS_Init()                  { return _AhnHS_Initialize(nullptr, nullptr, 0, nullptr, 0, 0); }
inline int __stdcall HS_StartService()          { return _AhnHS_StartService(); }
inline int __stdcall HS_StopService()           { return _AhnHS_StopService(); }
inline int __stdcall HS_UnInit()                { return _AhnHS_Uninitialize(); }
inline int __stdcall HS_PauseService()          { return _AhnHS_PauseService(0); }
inline int __stdcall HS_ResumeService()         { return _AhnHS_ResumeService(0); }
inline void HS_SaveFuncAddress()                { (void)_AhnHS_SaveFuncAddress(0); }
