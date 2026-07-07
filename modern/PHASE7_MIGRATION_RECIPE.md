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
