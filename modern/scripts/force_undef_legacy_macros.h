// Phase 7.4a: force-undef legacy LOG macro
// -------------------------------------------
// The Windows SDK / Visual C++ CRT headers transitively define a
// function-like macro `LOG(format, ...)` (e.g. via <tchar.h> or the
// Microsoft-specific logging macros). The legacy 墨香 game sources
// also define a macro `LOG(a)` (a no-op stub) in [Server]Distribute/
// ErrorMsg.h:52, and call `g_Console.LOG(level, msg, ...)` as a member
// function on CConsole. The preprocessor sees `LOG(level, msg)` and
// tries to expand it as the system macro, which produces C4002 / C2059
// errors.
//
// This file is force-included via /FI before every TU in the Distribute
// target. It undefines the system LOG macro so the legacy member-function
// call path (CConsole::LOG) compiles cleanly.
//
// Tracked in docs/KNOWN_BUGS.md Bug D-4.
#pragma once
#if defined(LOG)
#undef LOG
#endif
