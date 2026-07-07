
#pragma once

// WinSock 2.0 must come BEFORE any <windows.h> include; <objbase.h>
// (when included transitively via COM headers) pulls in <rpc.h> →
// <windows.h>, which would otherwise drag in <winsock.h> (WinSock 1.1).
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <winsock2.h>

#ifdef _MFC
	#include <afx.h>
	#include <ole2.h>
	#include <initguid.h>

#endif

#ifndef _MFC
	// Modern MSVC doesn't ship <ole2.h> by default; the modern COM
	// equivalent is <objbase.h> (provides IUnknown / IClassFactory).
	#include <objbase.h>
	#include <initguid.h>

	// Windows Header Files:
	#include <windows.h>

	// C RunTime Header Files
	#include <stdlib.h>
	#include <malloc.h>
	#include <memory.h>
	#include <tchar.h>
	#include <stdio.h>
#endif

#define GUID_SIZE 128
#define MAX_STRING_LENGTH 256
typedef void**	PPVOID;

typedef void** PPVOID;

