#pragma once

// Force WinSock 2.0 mode: <winsock2.h> defines _WINSOCKAPI_ which causes
// <windows.h> to skip the legacy <winsock.h> include. Without this guard,
// any later <windows.h> include (e.g. via objbase.h → RPC → windows.h)
// silently pulls in WinSock 1.1 and the two are mutually exclusive.
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

// WinSock 2.0 must come first so _WINSOCKAPI_ is set before any later
// <windows.h> include.
#include <winsock2.h>

// Modern MSVC doesn't ship MFC (no <afx.h>). BaseNetwork doesn't use MFC
// classes — it uses raw COM (IUnknown / IClassFactory) for the DLL
// boundary. Replace <afx.h> with <objbase.h> (the modern COM equivalent).
#include <objbase.h>

// <ole2.h> is the legacy unified OLE/COM header. Modern COM headers
// already cover its surface via <objbase.h>; drop the include.
#include <initguid.h>

#include <windows.h>

// Legacy network.cpp calls _CrtCheckMemory() unconditionally (no
// _DEBUG guard). The symbol only exists when the CRT debug heap is
// active (i.e. _DEBUG is defined); legacy Release builds silently
// failed to link, so the original vcproj must have always been
// built in Debug mode. Provide a noop fallback under NDEBUG so the
// TU compiles under modern Release builds without altering the
// source. Tracked in docs/KNOWN_BUGS.md (Bug C-8).
#ifdef NDEBUG
#include <crtdbg.h>
#ifndef _CrtCheckMemory
inline int _CrtCheckMemory() { return 1; /* non-zero = OK */ }
#endif
#endif

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

