#pragma once

// Modern MSVC marks sprintf / strcpy / strtok / fopen etc. as deprecated
// and refuses to expose them unless _CRT_SECURE_NO_WARNINGS is defined
// BEFORE <stdio.h> is first included. Legacy DBThread.cpp / CODB.CPP use
// sprintf widely — silence the deprecation so the legacy TUs compile.
// Must be the very first non-comment line so the macro is set before
// anything else transitively pulls in <stdio.h>.
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

// WinSock 2.0 must come BEFORE any <windows.h> include; <objbase.h>
// transitively pulls in <windows.h> via <rpc.h>. <windows.h> would
// otherwise drag in <winsock.h> (WinSock 1.1), and the two are
// mutually exclusive. Same pattern as [Lib]BaseNetwork/stdafx.h
// (Bug C-7).
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <winsock2.h>

// Modern MSVC doesn't ship MFC (no <afx.h>). DBThread uses raw COM
// (IUnknown / IClassFactory / IDBThread) for its DLL boundary. Replace
// <afx.h> with <objbase.h> (the modern COM equivalent).
#include <objbase.h>

// <ole2.h> is the legacy unified OLE/COM header. Modern COM headers
// already cover its surface via <objbase.h>; drop the include.
#include <initguid.h>

#include <windows.h>

// Legacy DBThread.cpp / DB.cpp / CODB.CPP / Voidlist.cpp call sprintf /
// strcpy / strtok / wsprintfA directly without including <cstdio>.
// <windows.h> used to pull <stdio.h> indirectly via the MFC kitchen
// sink; with MFC gone, add <cstdio> explicitly so sprintf resolves.
// Tracked in docs/KNOWN_BUGS.md (Bug C-13).
#include <cstdio>
#include <cstring>

#if !defined(_DLLEXPORT_)

// If _DLLEXPORT_ is NOT defined then the default is to import.
#if defined(__cplusplus)
#define DLLENTRY extern "C" __declspec(dllimport)
#else
#define DLLENTRY extern __declspec(dllimport)
#endif
#define STDENTRY DLLENTRY HRESULT WINAPI
#define STDENTRY_(type) DLLENTRY type WINAPI

// Here is the list of server APIs offered by the DLL (using the
// appropriate entry API declaration macros just #defined above).

STDENTRY DllRegisterServer(void);

STDENTRY DllUnregisterServer(void);

#else  // _DLLEXPORT_

// Else if _DLLEXPORT_ is indeed defined then we've been told to export.
#if defined(__cplusplus)
#define DLLENTRY extern "C" __declspec(dllexport)
#else
#define DLLENTRY __declspec(dllexport)
#endif
#define STDENTRY DLLENTRY HRESULT WINAPI
#define STDENTRY_(type) DLLENTRY type WINAPI

#endif // _DLLEXPORT_


#define GUID_SIZE 128
#define MAX_STRING_LENGTH 256
typedef void**	PPVOID;

