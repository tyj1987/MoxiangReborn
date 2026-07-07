// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Phase 7.4a: MFC residue removed (none was actually referenced; the legacy
// AFX_STDAFX_H guard + //{{AFX_INSERT_LOCATION}} marker were Visual C++
// wizard residue). <ole2.h> retained because [CC]ServerModule/Network.cpp
// calls CoCreateInstance() and the agent/legacy StdAfx.h pattern uses
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
// Legacy vcproj had these in PreprocessorDefinitions; Phase 7.4a moves them to
// the CMake target so all 5 Debug locale configs (KOR/JP/CN/HK/TL) can build
// from one source tree.
//   _KOR_LOCAL_    default Debug
//   _JAPAN_LOCAL_  Debug_JP
//   _CHINA_LOCAL_ + _TAIWAN_LOCAL_  Debug_CN
//   _HK_LOCAL_ + _TW_LOCAL_         Debug_HK
//   _TL_LOCAL_                      Debug_TL
// Release build uses no locale flag (locale-neutral, matches SWorking baseline).

// _USINGTOOL_ / _DISTRIBUTESERVER_ / _MAPSERVER_ / _AGENTSERVER / _MURIMNET_
// are also set per-config (Debug only in legacy vcproj).

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
// legacy game headers in <yhlibrary.h>). The legacy [Server]Distribute/
// ErrorMsg.h then re-defines it as a 1-arg no-op `LOG(a) ((void)0)`, but
// the Windows 2+arg macro survives the redefine and trips up all
// `g_Console.LOG(level, fmt, ...)` member-function calls (C4002 + C2059).
// Undefine here, AFTER all Windows / game headers, so the legacy 1-arg
// no-op macro is the only LOG visible at the call site. Tracked in
// docs/KNOWN_BUGS.md Bug D-4.
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

// Phase 7.4a fix: the Windows SDK transitively defines a function-like
// `LOG(format, ...)` macro (via <winsock2.h> → <windows.h> or via the
// legacy game headers in <yhlibrary.h>). The legacy [Server]Distribute/
// ErrorMsg.h then re-defines it as a 1-arg no-op `LOG(a) ((void)0)`, but
// the Windows 2+arg macro survives the redefine and trips up all
// `g_Console.LOG(level, fmt, ...)` member-function calls AND the
// `void CConsole::LOG(...)` member-function definition (C4002 + C2059).
// Undefine here, AFTER all Windows / game headers, so the legacy 1-arg
// no-op macro is the only LOG visible at the call site. Tracked in
// docs/KNOWN_BUGS.md Bug D-4.
#if defined(LOG)
#undef LOG
#endif



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
