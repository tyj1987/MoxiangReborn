# [Lib]BaseNetwork — 墨香 legacy `BaseNetwork.dll` (IOCP + COM server)
#
# Phase 7.1: convert [Lib]BaseNetwork/BaseNetwork.vcproj (VS 7.10 / 2003
# format) into a CMakeLists.txt. Follows the Phase 7.0 POC recipe
# ([Lib]HSEL) and the Phase 7.1 YHLibrary migration.
#
# Build properties extracted from BaseNetwork.vcproj:
#   ConfigurationType  = 2  (Dynamic-Link Library / DLL)
#   UseOfMFC            = 0  (no MFC; legacy StdAfx.h still had <afx.h>
#                              which is fixed in Phase 7.1)
#   CharacterSet        = (default = MBCS)
#   PreprocessorDefinitions = WIN32;_WINDOWS (+ _DEBUG or NDEBUG)
#   RuntimeLibrary      = 0 (Release, /MT) / 1 (Debug, /MTd)
#   PrecompiledHeader   = stdafx.h
#   ModuleDefinitionFile = BaseNetworkdll.def
#   AdditionalDependencies = ws2_32.lib odbc32.lib odbccp32.lib
#     (note: legacy BaseNetwork uses WinSock 2.0, not WinSock 1.1)
#   IgnoreDefaultLibraryNames = uafxcw.lib / uafxcwd.lib
#     (these are MFC CRT static libs; dropping them is safe once MFC is
#      no longer used in stdafx.h)
#   BaseAddress          = 0x65100000 (Debug only; not honored by CMake)
#
# Source files (from the <Files> block of vcproj, "Source Files" filter):
#   accept.cpp, BaseNetwork.cpp, BaseNetworkDll.cpp,
#   BaseNetworkFactory.cpp, connection.cpp, create_index.cpp,
#   main_msg_que.cpp, msgproc.cpp, network.cpp, overlapped.cpp,
#   overlapped_recv.cpp, overlapped_send.cpp, switch_que.cpp, worker.cpp
#
# Headers:
#   accept.h, basenetwork.h, BaseNetworkFactory.h, BaseNetworkInterface.h,
#   connection.h, create_index.h, define.h, main_msg_que.h, msgproc.h,
#   network.h, overlapped.h, overlapped_recv.h, overlapped_send.h,
#   resource.h, stdafx.h, struct.h, switch_que.h, worker.h
#
# Phase 7.1 changes relative to the vcproj:
#   - Removed `<afx.h>` and `<ole2.h>` from stdafx.h; replaced with
#     `<objbase.h>` (modern COM). Tracked in docs/KNOWN_BUGS.md (Bug C-7).
#   - Switched vcproj IgnoreDefaultLibraryNames=uafxcw*.lib (which removes
#     the MFC CRT from the link line) to a no-op here, because the
#     underlying MFC dependency is gone.
cmake_minimum_required(VERSION 3.16)
project(BaseNetwork CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W3 /FS /MP)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/MTd /Od /RTC1)
    else()
        add_compile_options(/MT /O2 /Ob2 /GF /Gy)
    endif()
endif()

add_library(BaseNetwork SHARED
    accept.cpp
    BaseNetwork.cpp
    BaseNetworkDll.cpp
    BaseNetworkFactory.cpp
    connection.cpp
    create_index.cpp
    main_msg_que.cpp
    msgproc.cpp
    network.cpp
    overlapped.cpp
    overlapped_recv.cpp
    overlapped_send.cpp
    switch_que.cpp
    worker.cpp
)

target_include_directories(BaseNetwork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(BaseNetwork PRIVATE /permissive-)
    # /Zc:strictStrings is on by default under /permissive-, but it
    # rejects the legacy TCHAR-style calls like SetRegKeyValue(TCHAR*,
    # "literal", ...) because the string literal is const TCHAR[] while
    # the function expects a non-const TCHAR*. Restore the legacy
    # implicit conversion (matches what the legacy vcproj compiled).
    target_compile_options(BaseNetwork PRIVATE /Zc:strictStrings-)
endif()

# The legacy BaseNetworkDll.cpp #defines _DLLEXPORT_ at the top of the TU
# to flip the DLLENTRY macros in stdafx.h from dllimport to dllexport.
# That #define is the canonical source of truth for this flag — do not
# override it via target_compile_definitions (which would also force it
# on every other .cpp in the library).

target_compile_definitions(BaseNetwork PUBLIC
    WIN32
    _WINDOWS
    _USRDLL        # legacy vcproj used _USRDLL for the DLL itself
    _MBCS          # MBCS character set
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Legacy BaseNetworkDll.cpp defines _DLLEXPORT_ before including stdafx.h
# to switch the DLLENTRY macros from dllexport to dllimport. That #define
# lives in the source file, so we don't (and shouldn't) re-set it via
# target_compile_definitions — setting it here would also apply to every
# other .cpp in the library.

# Use the .def file (BaseNetworkdll.def) for explicit symbol exports.
# Legacy: exports DllGetClassObject, DllCanUnloadNow, DllRegisterServer,
# DllUnregisterServer (the standard COM inproc server entrypoints).
set_target_properties(BaseNetwork PROPERTIES
    OUTPUT_NAME BaseNetwork
    WINDOWS_EXPORT_ALL_SYMBOLS FALSE
)

target_link_libraries(BaseNetwork PRIVATE
    ws2_32        # WinSock 2.0 (legacy uses winsock2.h)
    odbc32        # legacy link line; not used by BaseNetwork source but
    odbccp32      # preserved to match vcproj's dependency list 1:1
)

# Tell the linker about the .def file via LINK_FLAGS.
set_target_properties(BaseNetwork PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/BaseNetworkdll.def\"")

# --- Phase 7.1 smoke test ---------------------------------------------------
# yhlibrary_smoke already exercises part of the winsock surface via the
# vendored YHLibrary/connection.cpp, so a BaseNetwork-specific smoke test
# would duplicate that work. Add a simple link check (compile + link a
# tiny TU that includes BaseNetwork headers) so the migration is verified.
option(PHASE7_SMOKE "Build BaseNetwork smoke test executable" ON)
if(PHASE7_SMOKE)
    set(_repo_root "${CMAKE_CURRENT_SOURCE_DIR}/../..")
    set(SMOKE_TEST "${_repo_root}/modern/scripts/basenetwork_smoke_test.cpp")
    if(EXISTS "${SMOKE_TEST}")
        add_executable(basenetwork_smoke "${SMOKE_TEST}")
        target_link_libraries(basenetwork_smoke PRIVATE BaseNetwork)
        target_compile_definitions(basenetwork_smoke PRIVATE WIN32 _WINDOWS _MBCS)
    endif()
endif()