// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Phase 7.6: MFC residue removed. <winsock2.h> moved BEFORE <windows.h>
// per the Phase 7.1 convention. LOG macro undef'd after all game headers
// (Bug D-4 pattern, shared with Distribute/Agent/Map).

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// <winsock2.h> MUST come before any header that transitively pulls
// <windows.h> (otherwise legacy WinSock 1.1 gets dragged in).
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <winsock2.h>

#pragma warning (disable:4786)

#include <windows.h>
#include <ole2.h>
#include <initguid.h>
#include <stdio.h>
#include <assert.h>

#include "DataBase.h"
#include "Console.h"

// Phase 7.6 fix: undef LOG after Windows/game headers (Bug D-4 pattern).
#if defined(LOG)
#undef LOG
#endif

#include <yhlibrary.h>
//---MurimNet
#include "MNDefines.h"

#include "vector.h"
#include "protocol.h"
#include "CommonDefine.h"
#include "CommonGameDefine.h"
#include "ServerGameDefine.h"
#include "CommonGameStruct.h"
#include "CommonStruct.h"
#include "ServerGameStruct.h"
#include "CommonGameFunc.h"


#include "./ServerSystem.h"


#include "ShareDefines.h"
#include "ShareStruct.h"

// Belt-and-suspenders: undef LOG again after all game headers.
#if defined(LOG)
#undef LOG
#endif

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
