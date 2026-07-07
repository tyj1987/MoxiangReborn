# Phase 7 — Legacy MSBuild → CMake Migration Recipe

This document is the field guide for converting the legacy 4Dyuchi-era
`.vcproj` / `.dsp` / `.dsw` MSBuild projects under
`墨香【源码】/` to modern CMake `CMakeLists.txt`.

The first migration (`[Lib]HSEL`) is committed as the worked example;
follow the same shape for the other libs / DLLs / servers.

## 1. Pick a target

Recommended order: pure C++ libs (no MFC, no 3ds Max SDK, no networking)
first; MFC / engine DLLs / game servers later.

| Phase | Target | Notes |
|-------|--------|-------|
| 7.0   | `[Lib]HSEL`          | Worked example — committed first |
| 7.1   | `[Lib]YHLibrary`    | Generic utility lib, no engine deps |
| 7.1   | `[Lib]BaseNetwork`  | Network wrapper, depends on 4DyuchiNET |
| 7.1   | `[Lib]DBThread`     | Database thread pool |
| 7.1   | `[Lib]ZipArchive`   | (Compress on demand) |
| 7.2   | `4Dyuchi*` engines   | FilePack / FileStorage / Geometry / MapEditor / GX_Render |
| 7.3   | `[Server]Distribute` / `Agent` / `Map` / `[Monitoring]Server` | Multi-component server rebuilds |
| 7.4   | `[Client]MH` / `MHAutoPatch` / `Selupdate` | MFC clients (Phase 6 path) |
| 7.5   | `[Tool]*` (PackingMan / Regen / DS_RMTool / AutoPatch / anmexp / maxexp) | Tooling |

## 2. Inventory the legacy `.vcproj`

The `.vcproj` files (VS 2003 era, Version="7.10") carry the following
relevant fields per `<Configuration>`:

| `.vcproj` field                  | CMake equivalent                                    |
|----------------------------------|-----------------------------------------------------|
| `ConfigurationType="4"`          | `add_library(... STATIC)`                           |
| `ConfigurationType="1"`          | `add_executable(...)`                               |
| `ConfigurationType="2"`          | `add_library(... SHARED)` (rare, mostly engines)    |
| `UseOfMFC="0"` (default)         | no extra link libs                                  |
| `UseOfMFC="1"` (auto)            | `target_link_libraries(... MFC.lib)` (see below)     |
| `UseOfMFC="2"`                   | `target_link_libraries(... MfcMfc.lib MfcNmfcd.lib)` (rare) |
| `CharacterSet="2"` (Multibyte)   | `target_compile_definitions(... _MBCS)`; **do NOT pass /utf-8** |
| `CharacterSet="1"` (Unicode)     | `target_compile_definitions(... UNICODE _UNICODE)` |
| `PreprocessorDefinitions`        | `target_compile_definitions(...)` (one per entry, semicolon-list) |
| `RuntimeLibrary="5"` (debug DLL) | `/MDd` or `/MD` via `target_compile_options`        |
| `RuntimeLibrary="4"` (release DLL) | `/MD`                                              |
| `AdditionalDependencies`         | `target_link_libraries(...)`                        |
| `AdditionalIncludeDirectories`   | `target_include_directories(... PUBLIC ...)`        |
| `OutputFile`                     | `set_target_properties(... OUTPUT_NAME ...)`        |
| `PrecompiledHeaderThrough`       | `target_precompile_headers(... PRIVATE <header>)`   |

## 3. MFC leftovers in `StdAfx.h`

Many legacy `.vcproj` files set `UseOfMFC="0"` while their `StdAfx.h`
still pulls in `<afxwin.h>` etc. The MFC include kitchen-sink hides a
handful of transitive includes that real source code uses:

- `<cstdlib>` for `rand` / `srand`
- `<crtdbg.h>` for `_ASSERTE`
- `<windows.h>` (transitively via afxwin but only seen directly once)
- `<mmsystem.h>` (HSEL used it directly)

When stripping MFC out of `StdAfx.h`, add explicit `#include` lines for
the symbols the source actually uses — see `[Lib]HSEL/StdAfx.h` for the
worked example.

Also: `_ASSERTE` is conditionally defined in `<crtdbg.h>` based on the
build mode. In Release builds the symbol is missing entirely, which
breaks the source. Add a no-op fallback in `StdAfx.h`:

```cpp
#include <crtdbg.h>
#ifndef _ASSERTE
#define _ASSERTE(expr) ((void)0)
#endif
```

## 4. Encoding pitfalls

- **MBCS source + /utf-8 = compile errors** (`warning C4828: 文件包含在
  偏移 0xXXXX 处开始的字符`). Don't pass `/utf-8` for legacy libraries
  whose `.cpp` files contain Korean / Chinese / Japanese comments.
- The `modern/` subdir uses `/utf-8` because every modern file is clean
  ASCII / UTF-8. Do NOT carry this flag into the legacy CMakeLists.
- Set the codepage via `target_compile_definitions(... _MBCS)`.

## 5. Static library "PCH" pattern

Legacy `.vcproj` files use a precompiled header (`stdafx.h` + `StdAfx.cpp`).
For CMake, either:

- Keep the PCH (`target_precompile_headers(hsel PRIVATE <StdAfx.h>)`),
- OR drop the PCH and just compile all `.cpp` files directly. This adds
  a few seconds of build time but removes the dedicated `StdAfx.cpp`.

The HSEL POC went with the latter (simpler).

## 6. The HSEL POC: full diff

Path: `墨香【源码】/[Lib]HSEL/`

Files added: `CMakeLists.txt`
Files modified: `StdAfx.h` (MFC → Win32-only)

Test: `modern/scripts/test_hsel_python.py` (CMake configure + build) and
`modern/scripts/test_hsel_linkage.py` (link smoke test against the
freshly built `HSEL.lib`).

Build command (Ninja via vcvars64):

```
call "C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake -S "<LEGACYROOT>" -B "<BUILDDIR>" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "<BUILDDIR>"
```

The CMakeLists.txt honors vcvars64's MSVC environment; no need to pass
include paths or library directories manually.

## 7. Future work — automatic `/utf-8` for files-with-no-CJK

If a future refactor replaces MBCS comments with UTF-8, you can flip
`add_compile_options(/utf-8)` on **per file** via:

```cmake
set_source_files_properties(
    pch_thread.cpp font_assets.cpp
    PROPERTIES COMPILE_OPTIONS "/utf-8"
)
```

This is the cleanest way to migrate files one at a time rather than
biting the bullet on `/utf-8` globally.

## 8. Per-target status (Phase 7.1 — committed)

The recipe above is the field guide. The status table below records
what actually shipped.

| Target                    | Status        | Generator / arch | Build cmd                                          | Artifact                                              | Known issues                                                                                  |
|---------------------------|---------------|------------------|----------------------------------------------------|-------------------------------------------------------|-----------------------------------------------------------------------------------------------|
| `[Lib]HSEL`               | **DONE** (7.0)| Ninja / x64      | `modern/scripts/build_hsel_python.py` + link test | `HSEL.lib` 26,770 bytes                              | none                                                                                          |
| `[Lib]YHLibrary`          | **DONE** (7.1)| Ninja / x64      | `modern/scripts/build_yhlibrary.py`                | `YHLibrary.lib` 188,458 bytes                        | C-4 (atlbase.h removed), C-5/C-6 (vendored HSEL), LNK4006 duplicates                      |
| `[Lib]BaseNetwork` †      | **DONE** (7.1)| VS17 / x86       | `modern/scripts/build_basenetwork.py`              | `BaseNetwork.dll` 99,840 bytes                       | C-7/C-8/C-9/C-10/C-11                                                                         |
| `[Lib]DBThread`           | **DONE** (7.1)| VS17 / x86       | `modern/scripts/build_dbthread.py`                 | `DBThread.dll` 140,800 bytes                         | C-12 (SQLLEN on x64), C-13 (cstdio), C-14 (permissive-)                                      |
| `[Lib]ZipArchive`         | **SKIPPED**   | n/a              | n/a                                                | (use prebuilt `ZipArchive_Debug.lib`)                | Depends on MFC + zlib; not installed in this BuildTools env.                                   |
| `4DyuchiFileStorage`      | **DONE** (7.2)| VS17 / x86       | `modern/scripts/build_filestorage.py`              | `4DyuchiFileStorage.dll` 165,376 bytes              | C-15 (SS3DGFuncN pragma), C-16 (common header MFC), C-17 (WORD/WCHAR), C-18 (CJK path)   |
| `4DyuchiFilePack`         | **SKIPPED**   | n/a              | n/a                                                | (use prebuilt 4DyuchiFilePack.exe if needed)         | MFC dialog tool (UseOfMFC="2"). MFC unavailable.                                                |
| `4DYUCHIGXEXECUTIVE`      | TODO          | n/a              | n/a                                                | n/a                                                   | Same COM DLL pattern as FileStorage; depends on 4DyuchiGXGeometry + SS3DGFuncN.lib.            |
| `4DyuchiGXGeometry`       | DEFERRED      | n/a              | n/a                                                | Already modernized: see `modern/src/render/dx11/`   | DX11 reimpl in modern/ subsumes the legacy DX8 source. Skip legacy port unless needed.       |
| `4DYUCHIGX_RENDER`        | DEFERRED      | n/a              | n/a                                                | Already modernized: see `modern/src/render/dx11/`   | Same — DX11 reimpl covers CoD3DDevice / D3DResourceManager / HField / Font / Mesh / Texture.  |
| `4DyuchiGXMapEditor`     | SKIPPED       | n/a              | n/a                                                | n/a                                                   | MFC editor; MFC unavailable.                                                                  |
| `4DyuchiNET_Latest`       | **DONE** (7.2)| VS17 / x86       | `modern/scripts/build_net.py`                      | `4DyuchiNET.dll` 150,016 bytes                       | C-19..C-26 (see `docs/KNOWN_BUGS.md`) — interface drift from `[CC]ServerModule/inetwork.h`; legacy Code_GUI mirror; odbc link leftover; lost constants; dead PauseTimer impl; missing `PPVOID` typedef; CUSTOM_EVENT/EVENTCALLBACK field-type swap (layout byte-identical). |
| `[Server]Distribute` ‡     | **DONE** (7.4a)| VS17 / x86      | `modern/scripts/build_distribute.py`               | `DistributeServer.exe` 204,800 bytes                 | C-27/C-28/C-29 (compile: ErrorMsg stub, /Zc:forScope-, /wd4596+3244+3254) + D-1/D-2/D-3 (polish: LOG-undef shim, YHLibrary x64→x86 rebuild, Debug locale mfc71.lib out-of-scope). |

† **`[Lib]BaseNetwork` — CMakeLists commit provenance & Phase 7 gate (2026-07-07).**
The `墨香【源码】/[Lib]BaseNetwork/CMakeLists.txt` (139 lines) was committed
in Phase 7.1 at **`eb58017`** (`Phase 7.1: legacy [Lib]BaseNetwork migration
(.vcproj -> CMakeLists.txt)`). The Phase 7.1 gap-fill at **`6eeb815`**
(`... build script -> VS17/Win32 + sibling _full.txt log`) switched
`build_basenetwork.py` from Ninja to the VS17 2022 + `-A Win32` convention and
added the sibling `build_basenetwork_full.txt` log — it did **not** touch the
committed `CMakeLists.txt` or any legacy source. The Phase 7 integration gate
independently rebuilt from a fresh build dir and confirmed: **0 errors / 17
warnings** (14× C4819 MBCS codepage + 2× C4996 `inet_addr` + 1× tool noise),
`BaseNetwork.dll` = **99,840 bytes** (exact parity with the Phase 7.1 baseline),
and all 4 COM exports present (`DllCanUnloadNow`, `DllGetClassObject`,
`DllRegisterServer`, `DllUnregisterServer`). No new C-N bug surfaced.

‡ **`[Server]Distribute` — Phase 7.4a migration + polish + gate (2026-07-07).**
The `墨香【源码】/[Server]Distribute/CMakeLists.txt` (367 lines) was committed
in Phase 7.4a at **`99c1559`** (`Phase 7.4a: legacy [Server]Distribute migration
(.vcproj -> CMakeLists.txt)`). The Phase 7.4a polish at **`baaeeeb`**
(`Phase 7.4a polish: LOG-undef shim + KNOWN_BUGS D-1..D-3 + AGENTS #8/#9`)
added the LOG-undef `force_undef_legacy_macros.h` shim, three new bug
entries (D-1 LOG macro / D-2 YHLibrary x64→x86 / D-3 mfc71.lib out-of-scope),
and two new AGENTS.md traps (#8 PowerShell 方括号通配符 / #9 git 跟踪范围很窄);
it did **not** touch the committed `CMakeLists.txt` or any legacy source.
The Phase 7.4a integration gate independently rebuilt from a fresh build dir
(trashed `build_distribute/`) and confirmed: **0 errors / 752 warnings**
(mostly MBCS C4819 codepage + C4996 secure-CRT noise from legacy sources),
`DistributeServer.exe` = **204,800 bytes** (vs SWorking baseline 184,320 bytes,
+11.1% delta consistent with VS2003→VS17 CRT drift; /MT static link means
YHLibrary / 4DyuchiNET / MD5 / odbc32 / odbccp32 / wininet are all merged into
the EXE — only `WS2_32.dll` + `KERNEL32.dll` show as dynamic imports), and
smoke test `cmd /c "...DistributeServer.exe 0"` exits with **code 0**. The
gate commit (this file's update) is added at the end of this batch; see
`docs/phase7-gate/deliverable.md` for the full evidence trail.

### New conventions learned

These rules surfaced while migrating BaseNetwork / DBThread and are
**not** in the Phase 7.0 POC recipe:

1. **COM DLLs** (`ConfigurationType="2"`) need `add_library(... SHARED)`
   plus an explicit `LINK_FLAGS "/DEF:\"...\path\to\foo.def\""` to
   surface the standard COM entrypoints (DllGetClassObject, etc.).
   Do NOT use `target_sources(... foo.def)` — CMake rejects it for
   `LANGUAGE NONE`.

2. **`<winsock2.h>` must come before `<objbase.h>`** because `<objbase.h>`
   pulls in `<rpc.h>` → `<windows.h>`, which silently includes the
   legacy `<winsock.h>` if `_WINSOCKAPI_` isn't yet defined. Put
   `#ifndef _WINSOCKAPI_ #define _WINSOCKAPI_ #endif` at the very top
   of `stdafx.h` before any other include. (Pattern repeated across
   BaseNetwork / DBThread.)

3. **x86 only** — BaseNetwork / DBThread use `__asm` / `__declspec(naked)`
   or `SDWORD*` for ODBC length params. These are x86-only constructs;
   we use the Visual Studio 17 2022 generator with `-A Win32` so the
   Ninja + x64 default doesn't apply.

4. **`/Zc:strictStrings-`** is required for any DLL whose StdAfx.h
   pulls `<tchar.h>` + uses `#define UNICODE` (BaseNetworkDll.cpp,
   DBThreadDll.cpp). The legacy `TCHAR-style` API takes
   `LPTSTR` (non-const TCHAR*) but call sites pass string literals
   (const TCHAR[]).

5. **`/permissive-` must be dropped** for DBThread (Bug C-14) because
   legacy source uses `if (LPVOID > 0)` to test "non-null". For other
   targets (HSEL, YHLibrary, BaseNetwork) `/permissive-` is safe.

6. **`<cstdio>` / `<cstring>` / `<winsock2.h>` / `<oaidl.h>` / `<tchar.h>`**
   often need to be added to stdafx.h because the MFC transitive
   include chain used to drag them in. After MFC removal, add the
   explicit includes.

### Bug tracking

All known issues are recorded in `docs/KNOWN_BUGS.md` under
"Bug C-N" headings. Bug IDs introduced in Phase 7.1:

- C-4 to C-6: YHLibrary
- C-7 to C-11: BaseNetwork
- C-12 to C-14: DBThread
