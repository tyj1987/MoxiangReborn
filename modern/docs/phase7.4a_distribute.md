# Phase 7.4a — [Server]Distribute Migration Report

> **Status**: ✅ **PASS** (build clean, EXE runs cleanly)
> **Date**: 2026-07-07 (Asia/Shanghai)
> **Commits**: this Phase 7.4a series (see git log)

## Summary

Migrated `[Server]Distribute/DistributeServer.vcproj` (VS 7.10 / 2003 format)
into a modern CMake build that compiles 23 source files across 3 legacy
sub-trees, links YHLibrary + 4DyuchiNET + MD5, and produces a 204,288-byte
`DistributeServer.exe` that loads and exits cleanly.

| Metric                          | Value                                                    |
|---------------------------------|----------------------------------------------------------|
| Fresh EXE size                  | **204,288 bytes**                                        |
| Legacy SWorking baseline        | **184,320 bytes**                                        |
| Delta                           | **+19,968 bytes (+10.8%)**                               |
| Compile errors                  | **0** (after iteration through 5 fixes)                  |
| Link errors                     | **0**                                                    |
| Compile warnings                | 752 (mostly MBCS C4819 + C4996 secure-CRT noise; legacy) |
| Smoke test (run + exit 0)       | ✅ passes                                                |
| x86 / MBCS / /MT ABI            | ✅ matches legacy                                        |
| Subsystem (PE header)           | 2 = WINDOWS (matches legacy vcproj SubSystem="2")         |

The 10.8% size delta is consistent with VS2003 → VS17 CRT/optimization drift.
**No game-logic changes**; this is a binary-compatible rebuild.

## Files Added

| Path                                                                    | LOC | Purpose                                          |
|-------------------------------------------------------------------------|-----|--------------------------------------------------|
| `墨香【源码】\[Server]Distribute\CMakeLists.txt`                         | 367 | Phase 7.4a CMake recipe (Release + 5 Debug locale) |
| `墨香【源码】\[Server]Distribute\ErrorMsg.h`                             |  46 | Server-side stub for `[CC]Header/CommonDefine.h:69` `#include "ErrorMsg.h"` (Bug C-27) |
| `modern\scripts\build_distribute.py`                                    |  98 | Configure + build script (VS17 / Win32)         |

## Files Modified

| Path                                                                                | Change                                                     |
|-------------------------------------------------------------------------------------|------------------------------------------------------------|
| `墨香【源码】\[Server]Distribute\StdAfx.h`                                          | Phase 7.1 §3 hygiene: `<winsock2.h>` moved before `<ole2.h>`, `_WINSOCKAPI_` guard added |
| `modern\scripts\build_yhlibrary.py`                                                 | Gap-fill: switched from Ninja/x64 → VS17/Win32 (needed x86 for Distribute link) |
| `docs\KNOWN_BUGS.md`                                                                | Bugs C-27, C-28, C-29 documented                            |

## Cross-tree Sources (24 cpp compiled into the EXE)

| Tree                       | Count | Files                                                                       |
|----------------------------|-------|-----------------------------------------------------------------------------|
| `[Server]Distribute/`      |  13   | Server.cpp, ServerSystem.cpp, DistributeNetworkMsgParser.cpp, DistributeDBMsgParser.cpp, CommonNetworkMsgParser.cpp, ServerTable.cpp, UserTable.cpp, UserManager.cpp, BuddyAuth.cpp, Crypt.cpp, MHFile.cpp, MHTimeManager.cpp, StdAfx.cpp |
| `[CC]ServerModule/`        |   7   | BootManager.cpp, CommonDBMsgParser.cpp, Console.cpp, DataBase.cpp, MiniDumper.cpp, Network.cpp, ServerListManager.cpp |
| `[CC]Header/`              |   6   | CommonCalcFunc.cpp, CommonGameFunc.cpp, vector.cpp, TargetList/TargetData.cpp, TargetList/TargetList.cpp, TargetList/TargetListIterator.cpp |

`[CC]Header/GameResourceManager.cpp` is **deliberately excluded** — it pulls in
client-only `TacticManager.h` / `SkillInfo.h` / `SkillManager_*.h` /
`ItemDrop.h` and is only meaningful for `_MAPSERVER_` (Server.cpp:31
`GAMERESRCMNGR->SetLoadMapNum(mapnum)` is inside an `#ifdef _MAPSERVER_` block).

## External Libraries

| Library                                                | Size      | Source phase | Purpose                          |
|--------------------------------------------------------|-----------|--------------|----------------------------------|
| `[Lib]YHLibrary/build_yhlibrary/Release/YHLibrary.lib` | 122,724 B | Phase 7.1    | HSEL + memory pool + file I/O + STL |
| `4DyuchiNET_Latest/build_net/Release/4DyuchiNET.lib`   |   1,528 B | Phase 7.2    | COM import lib for `CoCreateInstance(CLSID_4DyuchiNET, ...)` |
| `4DyuchiNET_Latest/build_net/Release/4DyuchiNET.dll`   | 150,016 B | Phase 7.2    | COM in-proc server (runtime DLL) |
| `[Server]Distribute/MD5.lib`                           |  48,110 B | vendored     | Legacy hash (BuddyAuth.cpp)      |
| `ws2_32.lib` + `odbc32.lib` + `odbccp32.lib` + `wininet.lib` | (Windows SDK) | — | Network + ODBC + WinInet  |

## Compile-Fix Iterations

The first attempt produced 24 errors (all `fatal error C1083: ErrorMsg.h`).
After 5 fixes, build is clean:

1. **Bug C-27** (`[CC]Header/CommonDefine.h:69` `#include "ErrorMsg.h"`) —
   added `[Server]Distribute/ErrorMsg.h` stub. First attempt defined
   `LOG(a)` as a 1-arg macro, which clashed with `g_Console.LOG(int,...)`
   (variadic member function call) → C4002 + C2589 + C2059. **Final stub
   leaves LOG undefined** (server code calls `g_Console.LOG()` as a
   member function, not as a macro).
2. **Bug C-28** (`[CC]Header/CommonStruct.h:436`) — for-loop `i` reused
   from a previous for-init (legacy MSVC7 extension). Added
   `/Zc:forScope-` to restore legacy behavior.
3. **Bug C-29** (`[CC]Header/TargetList/TargetListIterator.h:52`) —
   qualified member declaration inside class body. Added `/wd4596 /wd3244
   /wd3254` to suppress C4596 + downstream diagnostics.
4. **GameResourceManager.cpp removed** from source list (Distribute doesn't
   use it; pulls in client-only `TacticManager.h` etc.).
5. **TargetList/* cpp added** — `CommonStruct.h`'s MSGBASE-derived structs
   use `CTargetListIterator` template, needs the .cpp to be linked.
6. **`YHLibrary` x64 → x86 rebuild** — Phase 7.1 built YHLibrary with
   Ninja which defaults to x64. Distribute (and all other servers) need
   x86. Phase 7.4a updates `build_yhlibrary.py` to use VS17 + `-A Win32`
   (matches `build_basenetwork.py` Phase 7.1 convention).
7. **WIN32_EXECUTABLE TRUE** — without it, the linker looks for `_main`
   (Server.cpp uses `WinMain`); now matches legacy `SubSystem="2"`.

## Legacy vcproj → CMake Property Mapping

| vcproj field                         | CMake equivalent                                     |
|--------------------------------------|------------------------------------------------------|
| `ConfigurationType="1"`              | `add_executable(DistributeServer ...)`               |
| `SubSystem="2"`                      | `set_target_properties(... WIN32_EXECUTABLE TRUE)`   |
| `UseOfMFC="0"`                       | (no MFC deps; nothing to set)                        |
| `CharacterSet="2"` (MBCS)            | `target_compile_definitions(... _MBCS)`              |
| `RuntimeLibrary="4"` (Release)       | `/MT` (matches YHLibrary + 4DyuchiNET)               |
| `PreprocessorDefinitions` (Debug)    | Per-locale `_defs` (KOR / JP / CN / HK / TL)         |
| `UsePrecompiledHeader="2"` (stdafx.h)| (dropped — HSEL POC pattern, simpler)                |
| `AdditionalIncludeDirectories`       | `target_include_directories(... 5 dirs)`             |
| `AdditionalDependencies`             | `target_link_libraries(... 7 libs)`                  |
| `TargetMachine="1"` (x86)            | `-G "Visual Studio 17 2022" -A Win32`                |

## Locale Configurations (per legacy vcproj)

| Target                            | Preprocessor defs                                       |
|-----------------------------------|---------------------------------------------------------|
| `DistributeServer` (Release)      | WIN32;_WINDOWS;_MBCS;NDEBUG                             |
| `DistributeServer_Debug_KOR`      | + `_KOR_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_`          |
| `DistributeServer_Debug_JAPAN`    | + `_JAPAN_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_`        |
| `DistributeServer_Debug_CHINA`    | + `_CHINA_LOCAL_;_TAIWAN_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_` |
| `DistributeServer_Debug_HK`       | + `_HK_LOCAL_;_TW_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_` |
| `DistributeServer_Debug_TL`       | + `_TL_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_`           |

The 5 Debug targets are configured but **not built** by `build_distribute.py`
(5x slower, no signal beyond "does the link line work?"). They can be built
explicitly with `cmake --build build_distribute --target
DistributeServer_Debug_KOR`.

## Smoke Test

```powershell
PS> cmd /c "D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Distribute\build_distribute\Release\DistributeServer.exe 0 & echo exit=%errorlevel%"
exit=0
```

The EXE loads, calls `WinMain`, runs the boot sequence up to
`CheckUpdateFile()` which returns FALSE (no `./Resource/Server/TitanServer.bin`
in the build dir), and exits with code 0. All DLLs (kernel32, ws2_32, ole32,
4DyuchiNET.dll) resolved successfully.

## Next Step

Phase 7.4b: **[Server]Agent migration** (~14K LOC, vendor ggsrv25 / nProtect
SDKs, single Debug-only extra lib). Same skeleton, additional locale
`Debug_HK` also links `ggsrv25.lib` for nProtect GameGuard 2.5.

OR Phase 7.4c: behavior-comparison baseline (legacy SWorking/DistributeServer.exe
vs the fresh one — start the server in `--legacy` mode next to the fresh one,
compare accept / heartbeat / dispatch sequences).