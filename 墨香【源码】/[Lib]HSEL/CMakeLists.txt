# [Lib]HSEL — 墨香 legacy `libhsel.lib` (encryption library)
#
# Phase 7.0 POC: convert [Lib]HSEL/HSEL.vcproj (VS 7.10 / 2003 format) into
# a CMakeLists.txt. This is the recipe that the remaining [Lib]*/ migrations
# will follow.
#
# Build properties extracted from HSEL.vcproj:
#   ConfigurationType  = 4  (Static Library)
#   UseOfMFC            = 0  (no MFC — the StdAfx.h was a legacy leftover)
#   CharacterSet        = 2  (Multibyte / MBCS, matches modern/ convention)
#   PreprocessorDefinitions = WIN32;_LIB (+ _DEBUG or NDEBUG)
#   PrecompiledHeader   = stdafx.h
#   Output              = .\$(Configuration)\HSEL.lib
#
# Source files (from the <Files> block of vcproj):
#   HSEL.cpp        - 3-line wrapper that includes stdafx.h + HSEL.h
#   HSEL_STREAM.cpp - 33KB implementation
#   StdAfx.cpp      - 1-line PCH wrapper (kept for ABI stability, harmless)
#
# External dependencies:
#   winmm.lib     - HSEL.h declares `#pragma comment(lib,"winmm.lib")`
#   <mmsystem.h>  - HSEL_STREAM.cpp uses timeGetTime via timeapi.h
#
# Phase 7.0 changes relative to the vcproj:
#   - Removed MFC includes from StdAfx.h (vcproj had UseOfMFC="0" but the
#     old stdafx pulled in afxwin.h; new stdafx only depends on Win32).
#   - Switched from precompiled headers to direct inclusion (CMake's
#     target_precompile_headers would also work; we keep things simple).
cmake_minimum_required(VERSION 3.16)
project(HSEL CXX)

# HSEL interfaces use __int32 (legacy MSVC extension). The source has
# Korean MBCS comments + string literals, so we deliberately do NOT pass
# /utf-8 here (that would force UTF-8 parsing and break the MBCS comments).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W3 /FS /MP)
endif()

add_library(hsel STATIC
    HSEL.cpp
    HSEL_STREAM.cpp
    StdAfx.cpp
)

target_include_directories(hsel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(hsel PUBLIC
    _LIB                # matches the legacy vcproj
    WIN32               # matches the legacy vcproj
    _MBCS               # MBCS character set (multi-byte Korean literals)
)

# Link winmm.lib (matches `#pragma comment(lib,"winmm.lib")` in HSEL.h).
target_link_libraries(hsel PUBLIC winmm)

# Naming: legacy artifact emitted "HSEL.lib" with capital H. Honor that.
set_target_properties(hsel PROPERTIES OUTPUT_NAME HSEL)
