// ErrorMsg.h — Phase 7.4a server-side stub.
//
// The shared [CC]Header/CommonDefine.h unconditionally does
//     #include "ErrorMsg.h"
// in its Release-only block (line 69), pulling the client's
// [Client]MH/ErrorMsg.h. The client file defines a LOG macro that
// overrides the server-side Console-based logging with a CErrorMsg
// singleton that does not exist in the server builds.
//
// We do NOT want to:
//   - modify [CC]Header/CommonDefine.h (shared with [Client]MH;
//     modification risks breaking the client build), or
//   - add [Client]MH/ to the server's include path (would drag in
//     client-only MFC headers, USINGTON singletons, etc.)
//
// Instead, this stub sits in [Server]Distribute/ which is already
// on the include path via target_include_directories. Because CMake
// passes the target's include dirs before transitive deps, this
// stub wins the #include "ErrorMsg.h" lookup before any other path.
//
// IMPORTANT: this stub does NOT define LOG / LOGEX / LOGFILE /
// OBJECTLOG / LOGMSG. The legacy client ErrorMsg.h defines them as
// `ERRORMSG->PrintError(...)` etc., but the client also assumes a
// 1-argument LOG(a) signature. The server code, in contrast, calls
// `g_Console.LOG(int nLevel, char* fmt, ...)` — a variadic member
// function call. If we define `LOG(a)` as a 1-arg macro here, the
// preprocessor matches it against the variadic member call (MSVC
// C4002 "too many args"), and then complains C2589 / C2059 because
// the expansion garbage doesn't fit. So we leave the LOG family
// undefined. Server code calls `g_Console.LOG(...)` directly as a
// member function, never as a macro — this is the legacy behavior.
//
// If a future change requires the LOG macro for servers, define it
// here as a no-op variadic, e.g.:
//   #define LOG(...)           ((void)0)
//   #define LOGEX(a, flag)     ((void)0)
// … but DO NOT use a 1-arg `LOG(a)` shape — it will shadow
// `CConsole::LOG(int, char*, ...)` and break the build.
#if !defined(AFX_ERRORMSG_H__3D9898D8_59A3_4AAF_81E1_054B04A547AE__INCLUDED_)
#define AFX_ERRORMSG_H__3D9898D8_59A3_4AAF_81E1_054B04A547AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

// ERRORMSG itself — the client uses USINGTON(CErrorMsg) to fetch a
// global CErrorMsg*. Server code never references ERRORMSG directly
// (grep verified across [Server]Distribute / [Server]Agent /
// [Server]Map), so we leave it undefined. If a future server file
// does use it, the compile error will point here and we'll know to
// wire it up properly.
//
// Uncomment if needed:
//   #define ERRORMSG  (g_pServerSystem ? &g_pServerSystem->GetErrorMsg() : NULL)

#endif // !defined(AFX_ERRORMSG_H__3D9898D8_59A3_4AAF_81E1_054B04A547AE__INCLUDED_)