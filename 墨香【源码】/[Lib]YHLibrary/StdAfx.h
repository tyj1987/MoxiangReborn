// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__2E18BDB6_4FBE_49A6_B6B5_A64DE5290F40__INCLUDED_)
#define AFX_STDAFX_H__2E18BDB6_4FBE_49A6_B6B5_A64DE5290F40__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <winsock.h>

// TODO: reference additional headers your program requires here
//
// HSEL_STREAM.cpp uses rand/srand (originally pulled in via the
// afxwin.h MFC transitive include in legacy builds — MFC is no longer
// in use, so we add <cstdlib> explicitly).
#include <cstdlib>
#include <crtdbg.h>

// Strclass.cpp / .h use the generic-text mappings _tcslen, _tcschr,
// _tcsinc, _ttoi, _istdigit, _tcsspn, _tcscspn, etc. The legacy build
// got these via <tchar.h> transitively from <afxwin.h>; MFC is no longer
// pulled in, so add <tchar.h> explicitly.
#include <tchar.h>

// Strclass.h declares BSTR AllocSysString() / BSTR SetSysString(BSTR*)
// (guarded by `#ifndef _AFX_NO_BSTR_SUPPORT`). The legacy build got the
// BSTR typedef via the MFC / ATL transitive include chain; add <oaidl.h>
// explicitly here so the declaration type-checks under modern MSVC.
#include <oaidl.h>

// _ASSERTE is conditionally defined by <crtdbg.h>; in Release builds the
// symbol collapses to nothing and the source still references it. Provide
// a no-op fallback so the TU compiles under NDEBUG (matches the Phase 7.0
// POC pattern from [Lib]HSEL/StdAfx.h).
#ifndef _ASSERTE
#define _ASSERTE(expr) ((void)0)
#endif

#include "YHLibrary.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2E18BDB6_4FBE_49A6_B6B5_A64DE5290F40__INCLUDED_)
