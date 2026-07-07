# Phase 7.5 — `[Server]Map` CMake Migration

**Status (producer): RECIPE COMMITTED. Build deferred to gate task.**

This document covers the Phase 7.5 migration of
`[Server]Map/MapServer.vcproj` (VS 7.10 / 2003 format) into a modern
`CMakeLists.txt`, mirroring the Phase 7.4a `[Server]Distribute` recipe
(commit `99c1559` + polish `baaeeeb`).

> **Phase 7.5 retry (this commit):** producer is **recipe-only**.
> The actual `cmake --build` / byte-parity verification is owned by the
> downstream **gate task** (`Phase 7.5 Gate` in the plan). This document
> records the recipe, build steps, and known shims — **not** a build
> run. The `MapServer.exe` byte size will be filled in by the gate
> task after the rebuild.

---

## 1. What this commit delivers

| Artifact                                                                | Bytes  | Notes                                         |
|-------------------------------------------------------------------------|--------|-----------------------------------------------|
| `墨香【源码】\[Server]Map\CMakeLists.txt`                                 | ~30 KB | 1 Release target + 5 Debug_<LOCALE> targets  |
| `modern\scripts\build_map.py`                                           | ~3 KB  | Mirror of `build_distribute.py`               |
| `modern\docs\phase7.5_map_migration.md`                                 | this   | Build steps, dependency notes, locale matrix  |
| `modern\PHASE7_MIGRATION_RECIPE.md` (updated §7.5 row)                   | edit   | One-line commit-hash placeholder              |

`MapServer.exe` is **NOT** committed. The Phase 7.5 gate task rebuilds
it from a fresh `build_map/` directory + compares against
`SWorking/MapServer.exe` (2,555,904 bytes baseline).

---

## 2. Build steps (what the gate task will run)

### 2.1 One-time prerequisites

These were built by earlier phases and are linked (not rebuilt) by the
Map CMake target:

- `[Lib]YHLibrary\build_yhlibrary\Release\YHLibrary.lib` — 188,458 bytes
  (Phase 7.1, commit `6f24d4d` + rebuild gap-fill at Phase 7.4a).
- `4DyuchiNET_Latest\build_net\Release\4DyuchiNET.dll` — 150,016 bytes
  + `.lib` 1,528 bytes (Phase 7.2, commit `64da8c9`).
- The legacy MD5 / Wininet libs are **not** needed by Map
  (Distribute-only).

### 2.2 Configure + build (mirror Distribute pattern)

```powershell
# From any working dir; PowerShell native.
python "D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\modern\scripts\build_map.py"
```

The script:

1. Trashes the stale `build_map/` directory and recreates it.
2. Sources `vcvars64.bat` (so cmake picks up MSVC + SDK paths).
3. Runs `cmake -S ...\[Server]Map -B ...\[Server]Map\build_map
   -G "Visual Studio 17 2022" -A Win32`.
4. Runs `cmake --build ...\build_map --config Release
   --target MapServer` (locale-neutral Release target only).
5. Writes `modern\scripts\build_map_full.txt` (sibling log, mirrors
   `build_distribute.py`).
6. Compares produced `MapServer.exe` against
   `墨香【源码】\SWorking\MapServer.exe` and prints byte delta + sha256.

### 2.3 Locale targets (5x Debug_<LOCALE>)

The CMakeLists.txt also declares 5 Debug_<LOCALE> targets mirroring
the legacy vcproj's per-locale configurations:

| CMake target                | Locale macros                                  |
|-----------------------------|------------------------------------------------|
| `MapServer_Debug_KOR`       | `_KOR_LOCAL_`                                  |
| `MapServer_Debug_JAPAN`     | `_JAPAN_LOCAL_`                                |
| `MapServer_Debug_CHINA`     | `_CHINA_LOCAL_;TAIWAN_LOCAL`                   |
| `MapServer_Debug_HK`        | `_HK_LOCAL_;_TW_LOCAL_;_IGNORE_ASSERT_`        |
| `MapServer_Debug_TL`        | `_TL_LOCAL_`                                   |

**Note:** Map does NOT use the `_USINGTOOL_` flag that Distribute
uses — that flag was Distribute-only. Also, Map's Debug_Console (KOR)
uses `_KOR_LOCAL_` (legacy naming) while the gate task may target a
slightly different config if needed.

### 2.4 Build time budget

Per the Phase 7.5 producer scope cut: **the gate task owns the build**.
Local compile for sanity is **not** required. If anyone does run it,
expect:

- **First build**: ~20-30 min for 251 cpp on Win32 (VS17 /MP).
  313-cpp legacy vcproj's Release build took ~25min in CI.
- **Incremental**: <2 min for header-only changes.

---

## 3. Source layout

### 3.1 `[Server]Map/` direct — 188 cpp

Includes the `[Server]Map/Condition/` subdir (battle-state DSL —
ACTION / DATA / SITUATION / RESOURCE / OPDATA + manager glue).

The Condition subdir is **part of [Server]Map/**, not a separate
cross-dir. The 13 Condition cpp files are listed inline with the
root cpp files (same vcproj pattern).

### 3.2 Cross-directory — 63 cpp (4 dirs)

| Cross-dir                  | cpp count | Notes                                                |
|----------------------------|-----------|------------------------------------------------------|
| `[CC]ServerModule/`        | 7         | Same as Distribute (BootManager, CommonDBMsgParser,  |
|                            |           | Console, DataBase, MiniDumper, Network,              |
|                            |           | ServerListManager)                                   |
| `[CC]Header/`              | 7         | 4 listed + 3 TargetList/                             |
|                            |           | (CommonCalcFunc, CommonGameFunc,                     |
|                            |           | GameResourceManager, vector, TargetData,             |
|                            |           | TargetList, TargetListIterator)                      |
| `[CC]Quest/`               | 21        | All cpp except QuestLimitKind_Time (server-other)    |
| `[CC]BattleSystem/`        | 25        | Root + 6 sub-dirs (GTournament, MunpaField,          |
|                            |           | MurimField, SiegeWar, Suryun, VimuStreet)            |
| `[CC]Suryun/`              | 3         | SuryunDefine + SuryunManager_client +                |
|                            |           | SuryunManager_server                                 |

**Total**: 188 + 7 + 7 + 21 + 25 + 3 = **251 cpp**

### 3.3 Pure-header directories (no source to compile)

- `[CC]Header` — CommonStruct.h, CommonDefine.h, Protocol.h, etc.
- `[Lib]YHLibrary` — base utilities.
- `4DyuchiNET_Common` — CLSID_4DyuchiNET / IID_4DyuchiNET defines.

These are passed via `target_include_directories(... PRIVATE ...)` but
contribute zero cpp files.

---

## 4. Dependency notes

### 4.1 Linker line

```
winmm.lib odbc32.lib odbccp32.lib YHLibrary.lib
4DyuchiNET.lib  ws2_32.lib
```

(Per `MapServer.vcproj` AdditionalDependencies; `MD5.lib` and
`Wininet.lib` are absent because those were Distribute-only.)

### 4.2 Phase 7.4a / 7.5 build flags (carried from Distribute)

These compile options are applied to **all** Map targets (Release +
5 Debug_<LOCALE>):

- `/Zc:strictStrings-` — Distribute compromise for legacy TCHAR calls.
- `/Zc:forScope-` — Distribute compromise for `int i` in for-init.
- `/wd4596 /wd3244 /wd3254` — Suppress legacy MSVC7 diagnostics
  (TargetListIterator.h:52 etc.; Bug C-29).
- `/FI` `modern/scripts/force_undef_legacy_macros.h` — Phase 7.4a
  LOG-macro shim (Bug D-1, commit `baaeeeb`).

Per-config (mutually exclusive — MSVC rejects `/O2` + `/RTC1`):

- Debug: `/MTd /Od /RTC1`
- Release: `/MT /O2 /Ob2 /GF /Gy`

### 4.3 Runtime library unification (CRT)

`/MT` (Release) / `/MTd` (Debug) is mandated to stay ABI-compatible
with `YHLibrary.lib` and `4DyuchiNET.dll` (both built with `/MT` in
Phases 7.1 / 7.2). A mixed `/MD` + `/MT` link line produces LNK4093 /
LNK2005 on the CRT symbols.

### 4.4 Preprocessor macros

| Config                | Defines                                                                |
|-----------------------|------------------------------------------------------------------------|
| Release               | `WIN32;_WINDOWS;_MBCS;_MAPSERVER_;__MAPSERVER__;NDEBUG`                 |
| Debug_Console (KOR)   | + `_DEBUG;_KOR_LOCAL_`                                                 |
| Debug_JAPAN           | + `_DEBUG;_JAPAN_LOCAL_`                                               |
| Debug_CHINA           | + `_DEBUG;_CHINA_LOCAL_;TAIWAN_LOCAL`                                  |
| Debug_HK              | + `_DEBUG;_HK_LOCAL_;_TW_LOCAL_;_IGNORE_ASSERT_`                       |
| Debug_TL              | + `_DEBUG;_TL_LOCAL_`                                                  |

Note: Map's Debug configs do **NOT** carry `_USINGTOOL_` (Distribute-only).
And `Debug_Console` (KOR) uses `_KOR_LOCAL_` not `_KOR_LOCAL_;_USINGTOOL_`.

### 4.5 No DX8 / no client-side DLLs

Map is a **server-side** executable. It does NOT link `d3d8.lib`,
`d3dx8.lib`, `4DyuchiGX_RENDER.dll`, `SS3DGFuncN.lib`, etc. The
gate task verifies this via `dumpbin /imports MapServer.exe` — only
`KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `WS2_32.dll`,
`ADVAPI32.dll`, `ODBC32.dll`, `WINMM.dll`, and `4DyuchiNET.dll`
should appear as dynamic imports.

---

## 5. Source list discrepancies vs Distribute

| Aspect                  | Distribute                                      | Map                                                          |
|-------------------------|-------------------------------------------------|--------------------------------------------------------------|
| cpp count               | 27 (13 direct + 14 cross-dir)                   | 251 (188 direct + 63 cross-dir)                              |
| Link to MD5.lib         | Yes (BuddyAuth)                                 | No                                                           |
| Link to Wininet.lib     | Yes (MHFile HTTP update)                        | No                                                           |
| PrecompiledHeader       | stdafx.h (dropped — HSEL POC pattern)           | stdafx.h (dropped — same pattern)                            |
| Cross-dir complexity    | Just [CC]ServerModule + [CC]Header              | + [CC]Quest (21) + [CC]BattleSystem (25) + [CC]Suryun (3)    |
| SubSystem               | 2 (WinMain / GUI; no real window)               | 2 (same pattern)                                             |
| Runtime library         | /MT (Release) / /MTd (Debug)                    | /MT (Release) / /MTd (Debug) — same                          |
| LOG-undef /FI shim      | Yes (Bug D-1)                                   | Yes (same — Bug D-1)                                         |

---

## 6. Known shims applied (none new vs Distribute)

All Phase 7.4a shims carry forward to Map. No new shims are introduced
in Phase 7.5 — the recipe mirrors Distribute's solution space, and
Map's source diversity ([CC]Quest / [CC]BattleSystem) shares the same
CommonStruct.h / TargetList pattern that Distribute already handles.

- **Bug C-27**: `ErrorMsg.h` stub at `[Server]Map/ErrorMsg.h` (mirrors
  Distribute's stub at the same path). Required because
  `[CC]Header/CommonDefine.h:69` includes `"ErrorMsg.h"` which
  doesn't exist in the server-side tree.
- **Bug C-28**: `/Zc:forScope-` — restores pre-VS2005 for-init scope.
  `CommonStruct.h:430` declares `int i` in a for-init and re-uses it
  in a second for loop at line 436.
- **Bug C-29**: `/wd4596 /wd3244 /wd3254` — suppress illegal qualifier
  in member declaration (TargetListIterator.h:52).
- **Bug D-1**: `force_undef_legacy_macros.h` /FI shim — undoes the
  Windows SDK `LOG(format, ...)` macro that conflicts with Map's
  `g_Console.LOG(level, msg)` member-function call.

If the gate task discovers additional shims needed (e.g. legacy source
typing fixes), they should be recorded in `docs/KNOWN_BUGS.md` per
AGENTS.md "反馈 / Bug" rule.

---

## 7. Verification matrix (gate-task responsibilities)

The Phase 7.5 gate task owns the verification suite. This document
only enumerates them; the gate task records actual numbers in its
deliverable.

| # | Check                          | Method                                | Pass criterion                                       |
|---|--------------------------------|---------------------------------------|------------------------------------------------------|
| 1 | cmake configure                | `cmake -G "VS 17 2022" -A Win32`      | Exit 0; no FATAL_ERROR                                |
| 2 | Release build                  | `cmake --build --target MapServer`    | Exit 0; `MapServer.exe` produced                     |
| 3 | Byte parity vs SWorking        | `len vs 2,555,904`                    | Delta < ±20% (CRT drift expected; not exact)         |
| 4 | dumpbin /imports (no DX8)      | `dumpbin /imports MapServer.exe`      | No `d3d8.dll`, `d3dx8.dll`, `4DyuchiGX_RENDER.dll`   |
| 5 | Smoke run                      | `cmd /c MapServer.exe 0`              | Exit code 0 (no GUI window; console-less)            |
| 6 | git diff sanity                | `git status --short`                  | No tracked source files modified                     |

---

## 8. Files in this commit

```
墨香【源码】\[Server]Map\CMakeLists.txt          (new — 1 Release + 5 Debug_<LOCALE> targets)
modern\scripts\build_map.py                     (new — mirror of build_distribute.py)
modern\docs\phase7.5_map_migration.md           (new — this doc)
modern\PHASE7_MIGRATION_RECIPE.md               (edited — §7.5 row filled in)
```

No legacy source files modified. The `[Server]Map/StdAfx.h` shim is
**not** included in this commit — it was applied + committed by the
earlier producer attempt (the uncommitted shim from attempt #1). If
the gate task surfaces a missing StdAfx.h patch, that goes in a
separate follow-up commit.

---

## 9. Hand-off to gate task

The gate task should:

1. Read `墨香【源码】\[Server]Map\CMakeLists.txt` end-to-end.
2. Trash `墨香【源码】\[Server]Map\build_map/` and run
   `python modern/scripts/build_map.py`.
3. Capture `modern\scripts\build_map_full.txt`.
4. Compare `build_map\Release\MapServer.exe` against
   `SWorking\MapServer.exe` (2,555,904 bytes).
5. Run the 6 verification checks in §7.
6. Write `modern\docs\phase7.5_gate_deliverable.md` with results.
