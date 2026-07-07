# [Lib]YHLibrary — 墨香 legacy `YHLibrary.lib` (utility library)
#
# Phase 7.1: convert [Lib]YHLibrary/YHLibrary.vcproj (VS 7.10 / 2003 format)
# into a CMakeLists.txt. Follows the Phase 7.0 POC recipe ([Lib]HSEL).
#
# Build properties extracted from YHLibrary.vcproj:
#   ConfigurationType  = 4  (Static Library)
#   UseOfMFC            = 0  (no MFC)
#   CharacterSet        = 2  (Multibyte / MBCS)
#   PreprocessorDefinitions = WIN32;_WINDOWS (+ _DEBUG or NDEBUG; _LIB in Release)
#   RuntimeLibrary      = 3 (Debug, /MTd) / 2 (Release, /MT)
#   PrecompiledHeader   = stdafx.h
#   Output              = .\YHLibrary.lib (capital Y, no Debug/Release subdir)
#   AdditionalDependencies = IndexGenerator.lib (pre-built)
#
# Source files (from the <Files> block of vcproj, organized by Filter):
#   Network:
#     connection.cpp, hashtable.cpp
#     BaseNetworkInterface.h, connection.h, hashtable.h, DEFINE.H,
#     DESC_BASENETWORK.h
#   MemoryPool:
#     MemoryPool.cpp, MemoryPoolTempl.cpp
#     MemoryPool.h, MemoryPoolTempl.h
#   Etc:
#     HSEL.cpp, HSEL_STREAM.cpp, Strclass.cpp, StrTokenizer.cpp
#     HSEL.h, IndexGenerator.h, StaticString.h, StdAfx.cpp, StdAfx.h,
#     Strclass.h, StrTokenizer.h
#     IndexGenerator.lib   (binary, pre-built; consumed via linker)
#   FileIO:
#     Fileio.cpp, Fileio.h
#   Encryption:
#     Encryptor.cpp, Encryptor.h
#   STL:
#     cConstLinkedList.cpp, cLinkedList.cpp, cLooseLinkedList.cpp, PtrList.cpp
#     cConstLinkedList.h, cLinkedList.h, cLooseLinkedList.h, PtrList.h
#   Top-level:
#     Array.h, Readme.txt, YHLibrary.h
#
# External dependencies:
#   winmm.lib   - HSEL.cpp uses timeGetTime (via mmsystem.h)
#   wsock32.lib - connection.h / connection.cpp use <winsock.h> (WinSock 1.1)
#   IndexGenerator.lib - pre-built; consumed via target_link_libraries
#   user32 / kernel32 / advapi32 / gdi32 / ole32 / oleaut32 — implicit via
#     MSVC default libs (PCH + Win32 baseline).
#
# Phase 7.1 notes relative to the vcproj:
#   - Removed dead `#include <atlbase.h>` from Strclass.cpp (Phase 7.0 POC
#     already documented that the ATL/MFC tooling is unavailable in this
#     environment; the include was never actually referenced — Strclass.cpp
#     has no CString / AtlThrow / ATL symbol usage at all). Tracked in
#     docs/KNOWN_BUGS.md.
#   - YHLibrary keeps its vendored HSEL.cpp (different version from the
#     standalone [Lib]HSEL/HSEL.cpp — 1067-line diff) because the legacy
#     YHLibrary.lib is self-contained (no callers link both .libs).
#     HSEL.h is exposed via YHLibrary.h, so the public ABI of YHLibrary.lib
#     includes CHSEL / CHSEL_STREAM. We do NOT link [Lib]HSEL.lib here.
cmake_minimum_required(VERSION 3.16)
project(YHLibrary CXX)

# HSEL interfaces use __int32 (legacy MSVC extension). The source has
# Korean MBCS comments + string literals, so we deliberately do NOT pass
# /utf-8 here.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # /W3 matches the vcproj WarningLevel=3
    # /FS is for parallel build safety (Ninja + multiple object dirs)
    # /MP enables multi-process compilation
    # /MTd (Debug) / /MT (Release) match RuntimeLibrary=3 / RuntimeLibrary=2
    add_compile_options(/W3 /FS /MP)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/MTd)
    else()
        add_compile_options(/MT)
    endif()
endif()

add_library(yhlibrary STATIC
    # Network
    connection.cpp
    hashtable.cpp
    # MemoryPool
    MemoryPool.cpp
    MemoryPoolTempl.cpp
    # Etc (HSEL is vendored here, not linked from [Lib]HSEL)
    HSEL.cpp
    HSEL_STREAM.cpp
    Strclass.cpp
    StrTokenizer.cpp
    StdAfx.cpp
    # FileIO
    Fileio.cpp
    # Encryption
    Encryptor.cpp
    # STL
    cConstLinkedList.cpp
    cLinkedList.cpp
    cLooseLinkedList.cpp
    PtrList.cpp
)

target_include_directories(yhlibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(yhlibrary PUBLIC
    WIN32                  # matches the legacy vcproj
    _WINDOWS               # matches the legacy vcproj (Debug)
    _LIB                   # matches the legacy vcproj (Release)
    _MBCS                  # MBCS character set (TCHAR = char under _MBCS)
    _USRDLL                # matches the legacy vcproj? No — that's for DLLs.
                           # We omit _USRDLL here because YHLibrary is a static lib.
                           # (vcproj Debug had: WIN32;_DEBUG;_WINDOWS  Release: WIN32;NDEBUG;_LIB)
)

# Conditional per-config defines
target_compile_definitions(yhlibrary PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Link winmm.lib (matches `#pragma comment(lib,"winmm.lib")` in HSEL.h).
target_link_libraries(yhlibrary PUBLIC winmm)

# WinSock 1.1 (the legacy `winsock.h` is from WinSock 1.1; modern winsock2.h
# is in ws2_32.lib, but legacy uses wsock32.lib).
target_link_libraries(yhlibrary PUBLIC wsock32)

# IndexGenerator.lib is a pre-built binary shipped alongside the source.
# Pass it directly to the linker so we don't need to build it from source
# (the Phase 7.x plan defers that to a later iteration if needed).
target_link_libraries(yhlibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/IndexGenerator.lib)

# Naming: legacy artifact emitted "YHLibrary.lib" with capital Y/H/L.
# Honor that.
set_target_properties(yhlibrary PROPERTIES OUTPUT_NAME YHLibrary)

# Static library — no DLL defines needed.
# Suppress the warning about MSVC needing /EHsc, /permissive-, etc.
if(MSVC)
    target_compile_options(yhlibrary PRIVATE /permissive-)
endif()

# --- Phase 7.1 smoke test ---------------------------------------------------
# Build a tiny executable that links against yhlibrary and exercises the
# public API surface. Confirms the .lib exports all the symbols the API
# promises. Build with `-DPHASE7_SMOKE=ON` (default ON).
option(PHASE7_SMOKE "Build YHLibrary smoke test executable" ON)
if(PHASE7_SMOKE)
    # The repo path contains Chinese characters, which trips up CMake's
    # get_filename_component() path arithmetic (the `..` normalization
    # drops everything past the first CJK byte).  Hardcode the smoke
    # test path by walking up two parent directories manually via
    # string concatenation, which sidesteps CMake's path parser.
    set(_repo_root "${CMAKE_CURRENT_SOURCE_DIR}/../..")
    set(SMOKE_TEST "${_repo_root}/modern/scripts/yhlibrary_smoke_test.cpp")
    message(STATUS "Phase 7.1 smoke test source: ${SMOKE_TEST}")
    if(NOT EXISTS "${SMOKE_TEST}")
        message(WARNING "Phase 7.1 smoke test not found at ${SMOKE_TEST}; skipping")
    else()
        add_executable(yhlibrary_smoke "${SMOKE_TEST}")
        target_link_libraries(yhlibrary_smoke PRIVATE yhlibrary)
        target_include_directories(yhlibrary_smoke PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
        target_compile_definitions(yhlibrary_smoke PRIVATE WIN32 _WINDOWS _MBCS)
    endif()
endif()