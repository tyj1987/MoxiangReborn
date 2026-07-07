# [Lib]DBThread — 墨香 legacy `DBThread.dll` (SQL Server ODBC thread pool
# + COM server)
#
# Phase 7.1: convert [Lib]DBThread/DBThread.vcproj (VS 7.10 / 2003 format)
# into a CMakeLists.txt. Mirrors the [Lib]BaseNetwork migration.
#
# Build properties extracted from DBThread.vcproj:
#   ConfigurationType  = 2  (Dynamic-Link Library / DLL)
#   UseOfMFC            = 0  (no MFC; legacy StdAfx.h still had <afx.h>
#                              which is fixed in Phase 7.1)
#   CharacterSet        = (default = MBCS)
#   PreprocessorDefinitions = WIN32;_WINDOWS (+ _DEBUG or NDEBUG)
#   RuntimeLibrary      = 0 (Release, /MT) / 1 (Debug, /MTd)
#   PrecompiledHeader   = stdafx.h
#   ModuleDefinitionFile = dbthreaddll.def
#   IgnoreDefaultLibraryNames = uafxcwd.lib (Debug only — MFC CRT removal)
#
# Source files (from the <Files> block of vcproj, "Source Files" filter):
#   CODB.CPP, DB.cpp, DBThread.cpp, DBThreadDll.cpp, DBThreadFactory.cpp,
#   MemoryPool.cpp, Voidlist.cpp
#
# Headers:
#   DB.h, DBclass.h, DBThread.h, DBThreadFactory.h, DBThreadInterface.h,
#   dbthread_guid.h, MemoryPool.h, stdafx.h, Voidlist.h
#
# Phase 7.1 changes relative to the vcproj:
#   - Removed `<afx.h>` and `<ole2.h>` from stdafx.h; replaced with
#     `<objbase.h>` (modern COM). Same pattern as [Lib]BaseNetwork (Bug C-7).
#   - WinSock 2.0 forced via `#define _WINSOCKAPI_` before <windows.h>.
#   - /Zc:strictStrings- so the legacy `TCHAR-style` calls with string
#     literals still compile (matches [Lib]BaseNetwork Bug C-9).
#   - **Built as Win32 (x86), not x64.** Legacy DB.cpp uses `SDWORD*` /
#     `long*` for ODBC length parameters; modern ODBC 3.8+ on x64 typedefs
#     `SQLLEN` as `INT64`, so the legacy pointer types no longer match
#     `SQLBindCol(..., SQLLEN*, ...)`. Win32 keeps `SQLLEN = long`, which
#     preserves ABI compatibility. Tracked in docs/KNOWN_BUGS.md Bug C-12.
cmake_minimum_required(VERSION 3.16)
project(DBThread CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Phase 7.1: Win32-only build (Ninja doesn't support -A Win32).
# The build script in modern/scripts/build_dbthread.py uses
# "Visual Studio 17 2022" + "-A Win32" to keep ODBC pointer
# types compatible with legacy source.

add_library(DBThread SHARED
    CODB.CPP
    DB.cpp
    DBThread.cpp
    DBThreadDll.cpp
    DBThreadFactory.cpp
    MemoryPool.cpp
    Voidlist.cpp
)

target_include_directories(DBThread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(DBThread PUBLIC
    WIN32
    _WINDOWS
    _USRDLL
    _MBCS
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Legacy DBThreadDll.cpp #defines _DLLEXPORT_ at the top of the TU to flip
# the DLLENTRY macros in stdafx.h from dllimport to dllexport. That #define
# lives in the source file — do not override it via target_compile_definitions.
# (Pattern copied from [Lib]BaseNetwork Bug C-9.)

if(MSVC)
    target_compile_options(DBThread PRIVATE
        /W3 /FS /MP
        # /permissive- would reject legacy `if (LPVOID > 0)` patterns
        # (DB.cpp:885, DB.cpp:942). Phase 7.1 keeps the legacy source
        # intact and drops /permissive- for this target. Tracked in
        # docs/KNOWN_BUGS.md (Bug C-14).
        /Zc:strictStrings-
        $<$<CONFIG:Debug>:/MTd /Od /RTC1>
        $<$<CONFIG:Release>:/MT /O2 /Ob2 /GF /Gy>
    )
endif()

# ODBC: SQL Server / Sybase / ODBC3 headers (sqlext.h, sql.h) live in the
# Windows SDK's um/ folder. CMake auto-pulls them via the vcvars64 INCLUDE
# env, so no explicit target_include_directories is needed.
#
# The DB library also uses Windows COM helpers (CoTaskMemAlloc etc.) and
# the ODBC user32-driver; link the corresponding default libs.
target_link_libraries(DBThread PRIVATE
    odbc32           # ODBC driver manager
    odbccp32         # ODBC cursor library (legacy dep, kept 1:1)
    user32
    advapi32
)

# Use the .def file (dbthreaddll.def) for explicit symbol exports.
# Legacy exports: DllGetClassObject, DllCanUnloadNow, DllRegisterServer,
# DllUnregisterServer (the standard COM inproc server entrypoints).
set_target_properties(DBThread PROPERTIES
    OUTPUT_NAME DBThread
    WINDOWS_EXPORT_ALL_SYMBOLS FALSE
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/dbthreaddll.def\""
)