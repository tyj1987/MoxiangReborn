// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Phase 7.5: MFC residue removed (none was actually referenced; the legacy
// AFX_STDAFX_H guard + //{{AFX_INSERT_LOCATION}} marker were Visual C++
// wizard residue). <ole2.h> retained because [CC]ServerModule/Network.cpp
// calls CoCreateInstance() and the legacy server StdAfx.h pattern uses
// <ole2.h>+<initguid.h> as the COM entry point. <winsock2.h> moved BEFORE
// <ole2.h> per the Phase 7.1 convention (Phase 7.1 §3 rule 2: <ole2.h>
// transitively pulls <rpc.h>→<windows.h>, which silently includes the
// legacy <winsock.h> if _WINSOCKAPI_ isn't yet defined).
//
// The shape of this file is shared across Distribute / Agent / Map — see
// modern/docs/phase7.4a_distribute.md for the shared-template rationale.

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT	0x0500
#define _CRYPTCHECK_

// <winsock2.h> MUST come before any header that transitively pulls
// <windows.h> (otherwise the legacy WinSock 1.1 <winsock.h> gets dragged
// in and we get winsock.h/winsock2.h symbol clashes).
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <winsock2.h>

// Locale switches — set by CMakeLists per-configuration (target_compile_definitions).
// Legacy vcproj had these in PreprocessorDefinitions; Phase 7.5 moves them to
// the CMake target so all 5 Debug locale configs (Console/KOR, JAPAN, CHINA,
// HK, TL) can build from one source tree.
//   _KOR_LOCAL_                         default Debug
//   _JAPAN_LOCAL_                       Debug_JAPAN
//   _CHINA_LOCAL_ + TAIWAN_LOCAL        Debug_CHINA
//   _HK_LOCAL_ + _TW_LOCAL_ + _IGNORE_ASSERT_   Debug_HK
//   _TL_LOCAL_                          Debug_TL
// Release build uses no locale flag (locale-neutral, matches SWorking baseline).
//   _MAPSERVER_  __MAPSERVER_           always on (the legacy vcproj had
//                                        these in every config)
//
// Note: legacy vcproj had NO `_USINGTOOL_` flag for Map (that was
// Distribute-only for the RM tool linkage). Map uses _FILE_BIN_ on
// Debug configs only.

//#define TAIWAN_LOCAL //pjslocal

//#define	_FILE_BIN_
#pragma warning(disable : 4786)

#include <windows.h>
#include <ole2.h>
#include <initguid.h>
#include <stdio.h>
#include <assert.h>
// TODO: reference additional headers your program requires here

#include "DataBase.h"
#include "Console.h"

// Phase 7.4a fix: the Windows SDK transitively defines a function-like
// `LOG(format, ...)` macro (via <winsock2.h> → <windows.h> or via the
// legacy game headers in <yhlibrary.h>). The legacy [Server]Map/ does
// not define a 1-arg `LOG(a)` macro itself, but the Windows 2+arg
// macro could still trip up g_Console.LOG(level, fmt, ...) member-function
// calls (C4002 + C2059) if a future header pulls in <tchar.h> or
// similar. Undefine here, AFTER all Windows / game headers, so any
// legacy 1-arg no-op macro is the only LOG visible at the call site.
// The /FI belt-and-suspenders shim in modern/scripts/force_undef_legacy_macros.h
// repeats this undef for .cpp files that bypass stdafx.h. Tracked in
// docs/KNOWN_BUGS.md Bug D-1.
#if defined(LOG)
#undef LOG
#endif

#include <yhlibrary.h>
//#include "CommonHeader.h"
#include "..\[CC]Header\vector.h"
#include "..\[CC]Header\protocol.h"
#include "..\[CC]Header\CommonDefine.h"
#include "..\[CC]Header\CommonGameDefine.h"
#include "..\[CC]Header\ServerGameDefine.h"
#include "..\[CC]Header\CommonGameStruct.h"
#include "..\[CC]Header\CommonStruct.h"
#include "..\[CC]Header\ServerGameStruct.h"
#include "..\[CC]Header\CommonGameFunc.h"


#include ".\ServerSystem.h"



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
