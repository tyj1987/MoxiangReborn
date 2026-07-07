# 4DyuchiFileStorage — 墨香 legacy `4DyuchiFileStorage.dll`
# (PAK pack/unpack + COM inproc server)
#
# Phase 7.2: convert 4DyuchiFileStorage/4DyuchiFileStorage.vcproj
# (VS 7.10 / 2003 format) into a CMakeLists.txt. Follows the
# [Lib]BaseNetwork / [Lib]DBThread COM-DLL migration pattern.
#
# Build properties extracted from the vcproj:
#   ConfigurationType  = 2  (Dynamic-Link Library / DLL)
#   UseOfMFC            = 0  (no MFC)
#   CharacterSet        = 2  (MBCS)
#   PreprocessorDefinitions = WIN32;_WINDOWS;_USRDLL;
#                             MY4DYUCHIFILESTORAGE_EXPORTS
#   RuntimeLibrary      = 0 (Release, /MT) / 1 (Debug, /MTd)
#   PrecompiledHeader   = stdafx.h  (via 4DyuchiGRX_common/StdAfx.h)
#   ModuleDefinitionFile = 4DyuchiFileStorage.def
#   OutputFile           = ../../4DyuchiDLL_MUK/4Dyuchigx_Release/
#                            4DyuchiFileStorage.dll
#   BaseAddress          = 0x67000000
#
# Source files (from the <Files> block of vcproj, "Source Files" filter):
#   4DyuchiFileStorage.def (linker manifest — DllGetClassObject etc.)
#   CoStorage.cpp, dllmain.cpp, factory.cpp, PackFile.cpp, Script1.rc
#
# Cross-directory headers (relative to project root):
#   ..\4DyuchiGRX_common\FileStorage_typedef.h
#   ..\4DyuchiGRX_common\IFileStorage.h
#   ..\4DyuchiGRX_common\IFileStorage_GUID.h
#   ..\4DyuchiGXGFunc\global.h
#   ..\4DyuchiGRX_common\typedef.h
#
# External dependencies:
#   SS3DGFuncN.lib  — legacy comment in dllmain.cpp. The file is
#                     missing from the source tree (only SS3DGFunc.lib
#                     exists alongside SS3DGFuncN.dll). Link against
#                     SS3DGFunc.lib instead — the symbols it imports
#                     (__imp__* BSAE / IC / DPCQ family) match the
#                     SS3DGFunc.dll exports exactly. Tracked in
#                     docs/KNOWN_BUGS.md (Bug C-15).
#
# Phase 7.2 changes relative to the vcproj:
#   - Use Visual Studio 17 2022 generator + -A Win32 (legacy CoStorage.cpp
#     uses __asm {} blocks; x64 MSVC does not support inline asm. Same
#     rationale as [Lib]BaseNetwork Bug C-10).
#   - Replace <ole2.h> + <initguid.h> via 4DyuchiGRX_common/StdAfx.h.
#     The legacy common header has MFC and non-MFC branches; we
#     patch out the MFC branch (Bug C-16).
cmake_minimum_required(VERSION 3.16)
project(FileStorage CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(_repo_root "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(_grx_common "${_repo_root}/4DyuchiGRX_common")
set(_gxgfunc    "${_repo_root}/4DyuchiGXGFunc")

# Resolve to absolute paths. Visual Studio 17 generator normalizes
# relative paths from target_link_libraries(...) in awkward ways when
# the project directory contains CJK characters (it tries to make
# every dependency path relative to the build dir, but the path
# arithmetic drops CJK bytes). Use absolute paths to bypass the
# normalization.
get_filename_component(_grx_common "${_grx_common}" ABSOLUTE)
get_filename_component(_gxgfunc    "${_gxgfunc}"    ABSOLUTE)

add_library(FileStorage SHARED
    CoStorage.cpp
    dllmain.cpp
    factory.cpp
    PackFile.cpp
)

target_include_directories(FileStorage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${_grx_common}
    ${_gxgfunc}
)

target_compile_definitions(FileStorage PUBLIC
    WIN32
    _WINDOWS
    _USRDLL
    _MBCS
    MY4DYUCHIFILESTORAGE_EXPORTS
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Legacy dllmain.cpp #defines _DLLEXPORT_ at the top of the TU to flip
# the DLLENTRY macros in 4DyuchiGRX_common/StdAfx.h from dllimport to
# dllexport. That #define lives in the source file — do not override
# it via target_compile_definitions. (Same pattern as [Lib]BaseNetwork
# Bug C-9.)

if(MSVC)
    target_compile_options(FileStorage PRIVATE
        /W3 /FS /MP
        # /permissive- would reject legacy WORD* -> LPOLESTR (wchar_t*)
        # casts in dllmain.cpp:319/408 (StringFromGUID2). Phase 7.2 keeps
        # the legacy source intact and drops /permissive- for this
        # target. Tracked in docs/KNOWN_BUGS.md (Bug C-17).
        /Zc:strictStrings-
        $<$<CONFIG:Debug>:/MTd /Od /RTC1>
        $<$<CONFIG:Release>:/MT /O2 /Ob2 /GF /Gy>
    )
endif()

# Copy SS3DGFunc.lib into the build dir so the link line can reference
# it as a bare relative path. The Visual Studio 17 generator corrupts
# absolute paths containing CJK bytes when it rebases them into
# AdditionalDependencies; copying to a build-dir-relative location
# sidesteps that bug. CMake's configure_file / file(COPY) don't survive
# the path-arithmetic corruption either, so use the build script
# (modern/scripts/build_filestorage.py) to drop a copy into BD/Release/
# before invoking cmake --build. Then explicitly add Release/ as a
# LIBPATH so the linker resolves the bare name from there.
target_link_libraries(FileStorage PRIVATE "SS3DGFunc.lib")
target_link_directories(FileStorage PRIVATE
    $<TARGET_FILE_DIR:FileStorage>
)

# Use the .def file (4DyuchiFileStorage.def) for explicit symbol exports.
# Legacy: DllGetClassObject / DllCanUnloadNow / DllRegisterServer /
# DllUnregisterServer (standard COM inproc server entrypoints).
set_target_properties(FileStorage PROPERTIES
    OUTPUT_NAME 4DyuchiFileStorage
    WINDOWS_EXPORT_ALL_SYMBOLS FALSE
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/4DyuchiFileStorage.def\" /NODEFAULTLIB:SS3DGFuncN.lib"
)