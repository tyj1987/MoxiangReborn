// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Phase 7.0: rewrote from the original MFC-flavored StdAfx.h to a minimal
// Win32-only equivalent. The legacy vcproj had UseOfMFC="0" yet the
// pre-existing StdAfx.h still pulled in <afxwin.h>/<afxext.h>/<afxcmn.h>,
// which is a leftover from a much earlier MFC configuration. None of HSEL's
// actual source uses MFC types — only Win32 + standard CRT types.
//   - rand()           — used by HSEL_STREAM.cpp; was pulling in via afxwin
//   - _ASSERTE (debug) — was via afxwin via crtdbg
// These were transitive include bugs that afxwin's kitchen-sink include
// "fixed". We expose them here explicitly.

#if !defined(AFX_STDAFX_H__440E8C5A_7336_43E0_8719_8FA12F529265__INCLUDED_)
#define AFX_STDAFX_H__440E8C5A_7336_43E0_8719_8FA12F529265__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>      // Win32 base (HWND, DWORD, BYTE, …)
#include <mmsystem.h>     // timeGetTime, joyStick APIs (HSEL_STREAM.cpp needs this)
#include <cstdlib>        // rand() / srand() — used by HSEL_STREAM.cpp
#include <crtdbg.h>       // _ASSERTE — used by HSEL_STREAM.cpp

// Fallback: in case crtdbg.h doesn't define _ASSERTE in this configuration,
// treat it as a no-op (matches legacy vcproj behavior — Release builds with
// WIN32;NDEBUG;_LIB still need _ASSERTE to compile).
#ifndef _ASSERTE
#define _ASSERTE(expr) ((void)0)
#endif

#endif // !defined(AFX_STDAFX_H__440E8C5A_7336_43E0_8719_8FA12F529265__INCLUDED_)
