# 4DyuchiNET — 墨香 legacy `4DyuchiNET.dll` (IOCP TCP/IP transport +
# inprocess COM server).
#
# Phase 7.2: convert 4DyuchiNET_Latest/I4DyuchiNET.vcproj (VS 7.10 /
# 2003 format) into a CMakeLists.txt. Follows the
# [Lib]BaseNetwork / [Lib]DBThread / 4DyuchiFileStorage COM-DLL
# migration pattern.
#
# Build properties extracted from the vcproj:
#   ConfigurationType  = 2  (Dynamic-Link Library / DLL)
#   UseOfMFC            = 0  (no MFC, no ATL)
#   CharacterSet        = 2  (MBCS — but .cpp uses UNICODE in dllmain.cpp)
#   PreprocessorDefinitions = WIN32;NDEBUG;_WINDOWS;_WIN32_WINNT=0x0500;
#                             _USE_WINSOCK
#   RuntimeLibrary      = 0 (Release, /MT) / 1 (Debug, /MTd)
#   PrecompiledHeader   = stdafx.h  (via 4DyuchiNET_Common/StdAfx.h)
#   ModuleDefinitionFile = I4DyuchiNET.def
#   AdditionalDependencies = odbc32.lib odbccp32.lib ws2_32.lib
#                            (odbc32/odbccp32 appear to be a copy-paste
#                             leftover from another vcproj — NET itself
#                             does not use ODBC. Kept for ABI parity;
#                             see KNOWN_BUGS.md Bug C-21.)
#   BaseAddress          = 0x60000000
#
# Source files (from the <Files> block of vcproj):
#   dllmain.cpp          — DllGetClassObject / DllRegisterServer
#   factory.cpp          — CF4DyuchiNET (IClassFactory)
#   conetwork.cpp        — Co4DyuchiNET (I4DyuchiNET implementation)
#   network.cpp          — CNetwork (one per network instance)
#   connection.cpp       — CConnection (per-socket state)
#   accept.cpp, connect.cpp, pre_connect.cpp,
#   connect_que.cpp, cpio.cpp, log.cpp, exception.cpp
#   confirm.cpp, collect_confirm_packet.cpp
#   create_index.cpp, dynamic_memory.cpp, exception.cpp,
#   mainthread.cpp, net_global.cpp, overlapped.cpp,
#   overlapped_recv.cpp, overlapped_send.cpp, overlapped_udp.cpp,
#   que.cpp, quemanager.cpp, switch_que.cpp, timer.cpp,
#   memory_object.cpp, read_memory_handle.cpp, write_memory_handle.cpp
#   create_index.cpp
#
# Cross-directory headers (relative to project root):
#   ../4DyuchiNET_Common/StdAfx.h          (PCH)
#   ../4DyuchiNET_Common/typedef.h         (callback typedefs)
#   ../4DyuchiNET_Common/net_define.h      (enums + bit masks)
#   ../4DyuchiNET_Common/INetwork_GUID.h   (COM GUIDs)
#   ../4DyuchiNET_Common/INetwork.h        (I4DyuchiNET COM interface)
#   ../4DyuchiNET_Common/icode.h           (ICode interface)
#   ../4DyuchiNET_Common/code_GUID.h       (mirror; see Bug C-20)
#
# External dependencies (vcproj link line):
#   ws2_32.lib      — Winsock 2
#   odbc32.lib      — VC project template leftover (unused; Bug C-21)
#   odbccp32.lib    — VC project template leftover (unused; Bug C-21)
#
# Phase 7.2 changes relative to the vcproj:
#   - Use Visual Studio 17 2022 generator + -A Win32. legacy
#     connection.cpp emits a few MBCS string literals in error-log
#     paths; matching the vcproj's MBSC mode and runtime layout keeps
#     wire-compatible network buffers.
#   - #define _WINSOCKAPI_ before <windows.h> in 4DyuchiNET_Common/StdAfx.h
#     so WinSock 1.x is suppressed. (Matches BaseNetwork / DBThread
#     / FileStorage convention; tracked as convention #2 in
#     PHASE7_MIGRATION_RECIPE.md.)
#   - /Zc:strictStrings- permits the legacy LPTSTR = TCHAR* call
#     style in dllmain.cpp's DllRegisterServer (StringFromGUID2,
#     RegCreateKeyEx). (Matches BaseNetwork Bug C-10.)
#   - 4DyuchiNET_Common/ was missing from the source tree entirely;
#     all 6 shared headers were recreated from the .cpp/.h call sites.
#     See KNOWN_BUGS.md Bug C-19.
#
cmake_minimum_required(VERSION 3.16)
project(I4DyuchiNET CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(_repo_root      "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(_net_common_dir "${_repo_root}/4DyuchiNET_Common")

# VS17 normalizes relative paths in target_link_libraries() in awkward
# ways when the project directory contains CJK bytes (the path
# arithmetic drops CJK bytes during rebase). Convert to absolute up
# front so the build doesn't depend on file(COPY) hacks.
get_filename_component(_net_common_abs "${_net_common_dir}" ABSOLUTE)

add_library(I4DyuchiNET SHARED
    accept.cpp
    collect_confirm_packet.cpp
    confirm.cpp
    connect.cpp
    connect_que.cpp
    connection.cpp
    conetwork.cpp
    cpio.cpp
    create_index.cpp
    dllmain.cpp
    dynamic_memory.cpp
    exception.cpp
    factory.cpp
    log.cpp
    mainthread.cpp
    memory_object.cpp
    net_global.cpp
    network.cpp
    overlapped.cpp
    overlapped_recv.cpp
    overlapped_send.cpp
    overlapped_udp.cpp
    pre_connect.cpp
    que.cpp
    quemanager.cpp
    read_memory_handle.cpp
    switch_que.cpp
    timer.cpp
    write_memory_handle.cpp
)

target_include_directories(I4DyuchiNET PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${_net_common_abs}
)

target_compile_definitions(I4DyuchiNET PUBLIC
    WIN32
    _WINDOWS
    _USRDLL
    _USE_WINSOCK
    _MBCS
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Strictly minimal — the legacy vcproj used RuntimeLibrary="0" (static)
# for Release and "1" (static debug) for Debug. The new generator's
# default (multi-threaded DLL) wouldn't link cleanly against the
# vcproj's ABI; force /MT-/MTd.
if(MSVC)
    target_compile_options(I4DyuchiNET PRIVATE
        /W3 /FS /MP
        # /permissive- would reject the legacy LPVOID/int compares and
        # friend style calls in connection.cpp / network.cpp. Keep the
        # legacy compile permissive.
        /Zc:strictStrings-
        $<$<CONFIG:Debug>:/MTd /Od /RTC1>
        $<$<CONFIG:Release>:/MT /O2 /Ob2 /GF /Gy>
    )
endif()

# Link dependencies from the legacy AdditionalDependencies line.
target_link_libraries(I4DyuchiNET PRIVATE
    ws2_32
    user32
    advapi32
    odbc32      # Bug C-21: vcproj template residue, kept for ABI parity
    odbccp32    # Bug C-21: vcproj template residue, kept for ABI parity
)

# Use the .def file (I4DyuchiNET.def) for explicit symbol exports.
# Legacy COM exports: DllGetClassObject / DllCanUnloadNow /
# DllRegisterServer / DllUnregisterServer.
set_target_properties(I4DyuchiNET PROPERTIES
    OUTPUT_NAME 4DyuchiNET
    WINDOWS_EXPORT_ALL_SYMBOLS FALSE
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/I4DyuchiNET.def\""
)

# Legacy COM pattern: exactly one TU must define INITGUID before
# including INetwork_GUID.h so the GUID constants CLSID_4DyuchiNET
# and IID_4DyuchiNET have storage and get exported as symbols. All
# other TUs get a reference-only declaration via <initguid.h>.
# We pick dllmain.cpp because it owns the COM factory entry points
# (DllGetClassObject / DllRegisterServer) and references both GUIDs.
set_source_files_properties(dllmain.cpp PROPERTIES
    COMPILE_DEFINITIONS "INITGUID=1"
)
