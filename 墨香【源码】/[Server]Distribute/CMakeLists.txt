# [Server]Distribute — 墨香 legacy `DistributeServer.exe` (login dispatcher)
#
# Phase 7.4a: convert [Server]Distribute/DistributeServer.vcproj (VS 7.10 /
# 2003 format) into a CMakeLists.txt. Follows the Phase 7.0 POC recipe
# ([Lib]HSEL), the Phase 7.1 migrations (YHLibrary / BaseNetwork / DBThread),
# and the Phase 7.2 engine work (4DyuchiNET_Latest).
#
# Build properties extracted from DistributeServer.vcproj:
#   ConfigurationType    = 1 (Application / exe)
#   UseOfMFC             = 0 (no MFC)
#   CharacterSet         = 2 (Multibyte / MBCS)
#   PreprocessorDefinitions (Release):  WIN32;NDEBUG;_WINDOWS
#   PreprocessorDefinitions (Debug KOR):WIN32;_DEBUG;_WINDOWS;_USINGTOOL_;_DISTRIBUTESERVER_;_KOR_LOCAL_
#                                       (and 4 sibling Debug_<LOCALE> configs)
#   RuntimeLibrary       = 4 (Release, /MD) / 3 (Debug, /MDd) per vcproj, but
#                           we use /MT (Release) / /MTd (Debug) to stay ABI-
#                           compatible with YHLibrary.lib / 4DyuchiNET.dll
#                           (all of which were migrated with /MT). A mixed
#                           /MD + /MT link line produces LNK4093 / LNK2005
#                           on the CRT symbols; see PHASE7_MIGRATION_RECIPE.md
#                           §3 "Runtime library unification".
#   PrecompiledHeader    = stdafx.h
#   SubSystem            = 2 (Windows GUI — but no Win32 GUI window; this is
#                             a console-less network service)
#   TargetMachine        = 1 (x86)
#   AdditionalIncludeDirectories:
#       ..\[Lib]YHLibrary
#       .\
#       ..\[CC]ServerModule
#       ..\[CC]Header            (Debug)
#   AdditionalDependencies:
#       odbc32.lib odbccp32.lib ..\[Lib]YHLibrary\YHLibrary.lib   (Release)
#       + MD5.lib Wininet.lib                                    (Debug)
#   Locale-flavored Debug configs: 5 (= one per supported locale).
#
# Source files (from the <Files> block of vcproj, organized by Filter):
#   Server Filter:
#     Server.cpp              (WinMain entry point — calls WinMain in
#                              #elif _DISTRIBUTESERVER_ branch)
#     ServerSystem.cpp        (CServerSystem + CServerTable management)
#   Network Filter:
#     CommonNetworkMsgParser.cpp  (CC base for the network parser)
#     DistributeNetworkMsgParser.cpp  (Distribute-specific messages;
#                                       calls g_Network->Send... etc.)
#   DB Filter:
#     DistributeDBMsgParser.cpp   (login/auth/account-related DB I/O)
#   Helper Filter:
#     BuddyAuth.cpp Crypt.cpp MHFile.cpp MHTimeManager.cpp
#     UserManager.cpp UserTable.cpp
#   PCH:
#     StdAfx.cpp / StdAfx.h
#   Headers-only:
#     ServerSystem.h ServerTable.h UserTable.h UserManager.h
#     BuddyAuth.h Crypt.h MHFile.h MHTimeManager.h
#     MD5Checksum.h MD5ChecksumDefines.h
#     CommonNetworkMsgParser.h DistributeNetworkMsgParser.h
#     DistributeDBMsgParser.h
#
# Cross-directory sources (must be built into the same executable):
#   [CC]ServerModule/ — BootManager, CommonDBMsgParser, Console, DataBase,
#                        MiniDumper, Network, ServerListManager  (7 cpp)
#   [CC]Header/        — vector.cpp, CommonCalcFunc.cpp, CommonGameFunc.cpp,
#                        GameResourceManager.cpp                   (4 cpp)
#
# External dependencies (binary, pre-built by earlier phases):
#   [Lib]YHLibrary/build_yhlibrary/YHLibrary.lib
#       (188,458 bytes — built Phase 7.1; static library of HSEL +
#        memory pool + file I/O + crypto + STL helpers)
#   4DyuchiNET_Latest/build_net/Release/4DyuchiNET.dll + .lib
#       (150,016 bytes DLL + 1,528-byte import lib — built Phase 7.2;
#        COM server providing CLSID_4DyuchiNET)
#   [Server]Distribute/MD5.lib
#       (vendored next to source; legacy hash library used by BuddyAuth)
#   4DyuchiNET_Common/
#       (header-only IID_4DyuchiNET/CLSID_4DyuchiNET defines — no library)
#
# Phase 7.4a changes relative to the vcproj:
#   - StdAfx.h got the Phase 7.1 §3 _WINSOCKAPI_ guard, <winsock2.h> moved
#     before <ole2.h>, and a comment block documenting the per-config locale
#     macro switch (moved out of #ifdefs and into target_compile_definitions).
#     No game-logic changes.
#   - 5 Debug locale variants are exposed as separate CMake targets so a
#     single `cmake --build` can produce all 5 (legacy vcproj required 5
#     separate "Set as Active Project" flips).
#   - Precompiled header is dropped (HSEL POC pattern) — adds a few seconds
#     of build time, removes the need for per-target PCH bookkeeping.
#
# Byte-level parity target:
#   SWorking/DistributeServer.exe = 184,320 bytes (Release build).
#   This CMakeLists targets exactly that configuration (Release|Win32,
#   locale-neutral) and the build script compares the output bytes.
cmake_minimum_required(VERSION 3.16)
project(DistributeServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Per-config options are attached at the target level (generator
    # expressions) so /O2 (Release) and /RTC1 (Debug) never collide.
    # /W3 /FS /MP are config-agnostic so they can stay global.
    #
    # Phase 7.5g: add /source-charset:utf-8 /execution-charset:utf-8 (same
    # rationale as [Server]Map/CMakeLists.txt:96-134). The build host is
    # cp936 (GBK Chinese Windows) — without /execution-charset:utf-8 MSVC
    # would silently turn Korean Hangul codepoints in string literals into
    # '?' placeholders, and the Debug_<LOCALE> configs (KOR/JAPAN/CHINA/
    # HK/TL — all with Korean codepoints in their build error messages)
    # fail to parse [CC]Header/CommonStruct.h. The /source-charset:utf-8
    # half is the actual fix for the parse errors; the /execution-charset
    # half keeps the string-literal bytes intact. Same trade-off analysis
    # as [Server]Map applies here — no _mbs* / isleadbyte / mbstowcs
    # usage in [Server]Distribute so MBCS char* APIs are unaffected.
    add_compile_options(/W3 /FS /MP /source-charset:utf-8 /execution-charset:utf-8)
endif()

# -----------------------------------------------------------------------------
# Source list
# -----------------------------------------------------------------------------
# Cross-directory paths live at the cmake_minimum_required time of the
# `add_subdirectory([CC]ServerModule)` call. CMake's path arithmetic is
# fine here because none of these paths contain CJK characters (the [CC]
# bracket form is handled correctly by cmake_path / get_filename_component).
set(_repo_root     "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(_yh_root       "${_repo_root}/[Lib]YHLibrary")
# Phase 7.4a fix: YHLibrary was rebuilt with VS17/Win32 (was Ninja/x64);
# VS17 multi-config layout puts the output in build_yhlibrary/Release/.
# See modern/scripts/build_yhlibrary.py for the build command.
set(_yh_lib        "${_yh_root}/build_yhlibrary/Release/YHLibrary.lib")
set(_cc_header     "${_repo_root}/[CC]Header")
set(_cc_server     "${_repo_root}/[CC]ServerModule")
set(_net_common    "${_repo_root}/4DyuchiNET_Common")
set(_net_root      "${_repo_root}/4DyuchiNET_Latest")
set(_net_lib_dir   "${_net_root}/build_net/Release")
set(_net_lib       "${_net_lib_dir}/4DyuchiNET.lib")
set(_net_dll       "${_net_lib_dir}/4DyuchiNET.dll")
set(_md5_lib       "${CMAKE_CURRENT_SOURCE_DIR}/MD5.lib")

# -----------------------------------------------------------------------------
# Release target (locale-neutral; matches SWorking/DistributeServer.exe)
# -----------------------------------------------------------------------------
add_executable(DistributeServer
    # [Server]Distribute/ — 13 cpp
    BuddyAuth.cpp
    CommonNetworkMsgParser.cpp
    Crypt.cpp
    DistributeDBMsgParser.cpp
    DistributeNetworkMsgParser.cpp
    MHFile.cpp
    MHTimeManager.cpp
    Server.cpp
    ServerSystem.cpp
    ServerTable.cpp
    StdAfx.cpp
    UserManager.cpp
    UserTable.cpp
    # [CC]ServerModule/ — 7 cpp (cross-directory)
    ${_cc_server}/BootManager.cpp
    ${_cc_server}/CommonDBMsgParser.cpp
    ${_cc_server}/Console.cpp
    ${_cc_server}/DataBase.cpp
    ${_cc_server}/MiniDumper.cpp
    ${_cc_server}/Network.cpp
    ${_cc_server}/ServerListManager.cpp
    # [CC]Header/ — 3 cpp (cross-directory; CommonGameFunc.cpp exposes
    # helpers like SafeStrCpy/ParseRuntimeString).
    ${_cc_header}/CommonCalcFunc.cpp
    ${_cc_header}/CommonGameFunc.cpp
    ${_cc_header}/vector.cpp
    # [CC]Header/TargetList/ — 3 cpp (cross-directory). These were in
    # the original Distribute vcproj's "Common\TargetList" sub-filter but
    # were omitted from the Phase 7.4a initial source list. Without
    # them, MSGBASE-derived structs in [CC]Header/CommonStruct.h that
    # use the TargetList template can't be linked.
    ${_cc_header}/TargetList/TargetData.cpp
    ${_cc_header}/TargetList/TargetList.cpp
    ${_cc_header}/TargetList/TargetListIterator.cpp
)

target_include_directories(DistributeServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${_cc_server}
    ${_cc_header}
    ${_yh_root}
    ${_net_common}
)

if(MSVC)
    target_compile_options(DistributeServer PRIVATE
        # /permissive- is NOT used (Bug C-28 + C-29 in KNOWN_BUGS.md):
        #   - CommonStruct.h:436 reuses `i` declared in a prior for-loop
        #     init (legacy MSVC7 extension, banned under /permissive-).
        #   - TargetListIterator.h:52 declares a qualified member function
        #     inside the class body (legacy MSVC7 extension, banned under
        #     /permissive- as error C4596).
        # Same workaround as DBThread (Bug C-14).
        #
        # /Zc:strictStrings- rejects legacy TCHAR-style calls in DB.h etc.
        # Mirror the Phase 7.1 BaseNetwork / DBThread compromise.
        /Zc:strictStrings-
        # /Zc:forScope- restores the pre-VS2005 for-init variable scope
        # rule (the `int i` declared in `for(int i=...; ...)` leaks into
        # the enclosing function scope). Several legacy sources in
        # [CC]Header/ rely on this — e.g. CommonStruct.h:430 declares
        # `int i` in a for-init, then re-uses `i` at line 436 inside a
        # second for loop in the same function. Tracked in
        # docs/KNOWN_BUGS.md Bug C-28.
        /Zc:forScope-
        # Suppress legacy-source compat diagnostics. These are all
        # accepted by MSVC 6.0/7.0 but rejected (as errors) by VS17:
        #   C4596  illegal qualifier in member declaration
        #          (TargetListIterator.h:52, CommonStruct.h:4713)
        #   C3244  explicit override of base method mismatch
        #          (downstream of C4596 in CommonStruct.h:4713)
        #   C3254  explicit override declared in non-derived class
        #          (downstream of C4596 in CommonStruct.h:4713)
        # See docs/KNOWN_BUGS.md Bug C-29.
        # Use space-separated /wd flags; CMake translates this into
        # MSBuild's <DisableSpecificWarnings> tag (comma-separated
        # values within the tag). Passing `/wd4596,3244,3254` as a
        # single cl.exe flag fails with D8021 "invalid numeric
        # argument"; passing `/wd4596 /wd3244 /wd3254` is fine
        # because CMake routes them into the per-warning tags.
        /wd4596 /wd3244 /wd3254
        # Belt-and-suspenders: also force-include the LOG-undef header
        # so that .cpp files which include stdafx.h but happen to pull
        # in Windows headers via a different path still get the fix.
        # Path goes up 2 parents from [Server]Distribute to the repo
        # root (the legacy tree uses untracked CJK paths; CMAKE_CURRENT_SOURCE_DIR
        # normalizes to forward slashes but the relative-path arithmetic
        # still works). The /undef LOG inside the header itself is
        # tracked in docs/KNOWN_BUGS.md Bug D-1; this /FI is the
        # CMake-side companion.
        /FI ${CMAKE_CURRENT_SOURCE_DIR}/../../modern/scripts/force_undef_legacy_macros.h
        # Per-config runtime + optimization (MSVC rejects /O2 + /RTC1 in
        # the same command line, so these MUST be mutually exclusive).
        $<$<CONFIG:Debug>:/MTd /Od /RTC1>
        $<$<CONFIG:Release>:/MT /O2 /Ob2 /GF /Gy>
    )
endif()

# Preprocessor definitions (Release):
#   vcproj Release|Win32: WIN32;NDEBUG;_WINDOWS
#   vcproj Release had no _MBCS in PreprocessorDefinitions but CharacterSet=2
#   was set on the Configuration, which MSBuild turns into _MBCS. We set
#   it explicitly here.
target_compile_definitions(DistributeServer PRIVATE
    WIN32
    _WINDOWS
    _MBCS
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Linker line (Release):
#   odbc32.lib odbccp32.lib ..\[Lib]YHLibrary\YHLibrary.lib
# Plus:
#   ws2_32          — <winsock2.h> in StdAfx.h
#   wininet         — DBThread legacy (Debug only in legacy, but Distribute
#                     server.cpp's MHFile code uses WinInet for HTTP update)
#   4DyuchiNET.lib  — COM import library (built Phase 7.2)
#   MD5.lib         — vendored; provides MD5 hash for BuddyAuth
# Linker line (Release):
#   odbc32.lib odbccp32.lib ..\[Lib]YHLibrary\YHLibrary.lib
# Plus:
#   ws2_32          — <winsock2.h> in StdAfx.h
#   wininet         — DBThread legacy (Debug only in legacy, but Distribute
#                     server.cpp's MHFile code uses WinInet for HTTP update)
#   4DyuchiNET.lib  — COM import library (built Phase 7.2)
#   MD5.lib         — vendored; provides MD5 hash for BuddyAuth
#
# Phase 7.4a fix: pass the lib DIRECTORIES to target_link_directories()
# and the lib BASENAMES to target_link_libraries(). The alternative of
# passing full absolute paths with `..` and CJK characters in the path
# broke MSBuild's <AdditionalDependencies> encoding in the generated
# .vcxproj, which then made link.exe fail with LNK1181 even though
# the file existed. The basename form sidesteps the encoding issue
# entirely.
target_link_directories(DistributeServer PRIVATE
    "${_yh_root}/build_yhlibrary/Release"
    "${_net_lib_dir}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(DistributeServer PRIVATE
    YHLibrary.lib
    4DyuchiNET.lib
    MD5.lib
    ws2_32
    odbc32
    odbccp32
    wininet
)

# Output naming: legacy produced "DistributeServer.exe" with capital D.
# SubSystem = 2 (Windows GUI) per the legacy vcproj — the entry point
# is WinMain (Server.cpp), not main. Without WIN32 (Windows), the linker
# looks for `_main` and fails with LNK2019.
set_target_properties(DistributeServer PROPERTIES
    OUTPUT_NAME DistributeServer
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    WIN32_EXECUTABLE TRUE
)

# Phase 7.4a sanity: print a STATUS line so the build log records which
# 4DyuchiNET/YHLibrary binaries we linked against.
message(STATUS "Phase 7.4a DistributeServer configuration:")
message(STATUS "  YHLibrary.lib    = ${_yh_lib}")
message(STATUS "  4DyuchiNET.lib   = ${_net_lib}")
message(STATUS "  4DyuchiNET.dll   = ${_net_dll}")
message(STATUS "  MD5.lib          = ${_md5_lib}")

# -----------------------------------------------------------------------------
# Debug locale targets — one per legacy vcproj Debug_<LOCALE> config.
# -----------------------------------------------------------------------------
# Each target adds the same source list + the locale-specific compile
# definitions. The legacy vcproj's Debug_<LOCALE> configs each linked
# Wininet.lib + MD5.lib explicitly — we include those unconditionally
# for Debug to keep parity.
foreach(locale IN ITEMS KOR JAPAN CHINA HK TL)
    set(_defs "")
    set(_name "DistributeServer_Debug_${locale}")
    if(locale STREQUAL "KOR")
        set(_defs "_KOR_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_")
    elseif(locale STREQUAL "JAPAN")
        set(_defs "_JAPAN_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_")
    elseif(locale STREQUAL "CHINA")
        set(_defs "_CHINA_LOCAL_;_TAIWAN_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_")
    elseif(locale STREQUAL "HK")
        set(_defs "_HK_LOCAL_;_TW_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_")
    elseif(locale STREQUAL "TL")
        set(_defs "_TL_LOCAL_;_USINGTOOL_;_DISTRIBUTESERVER_")
    endif()

    add_executable(${_name}
        BuddyAuth.cpp
        CommonNetworkMsgParser.cpp
        Crypt.cpp
        DistributeDBMsgParser.cpp
        DistributeNetworkMsgParser.cpp
        MHFile.cpp
        MHTimeManager.cpp
        Server.cpp
        ServerSystem.cpp
        ServerTable.cpp
        StdAfx.cpp
        UserManager.cpp
        UserTable.cpp
        ${_cc_server}/BootManager.cpp
        ${_cc_server}/CommonDBMsgParser.cpp
        ${_cc_server}/Console.cpp
        ${_cc_server}/DataBase.cpp
        ${_cc_server}/MiniDumper.cpp
        ${_cc_server}/Network.cpp
        ${_cc_server}/ServerListManager.cpp
        ${_cc_header}/CommonCalcFunc.cpp
        ${_cc_header}/CommonGameFunc.cpp
        ${_cc_header}/vector.cpp
        ${_cc_header}/TargetList/TargetData.cpp
        ${_cc_header}/TargetList/TargetList.cpp
        ${_cc_header}/TargetList/TargetListIterator.cpp
    )
    target_include_directories(${_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${_cc_server}
        ${_cc_header}
        ${_yh_root}
        ${_net_common}
    )
    if(MSVC)
        # Same /permissive- drop as the Release target (Bug C-28 + C-29).
        # Same legacy compat flags (incl. D-1 LOG-undef /FI).
        target_compile_options(${_name} PRIVATE
            /Zc:strictStrings-
            /Zc:forScope-
            /wd4596 /wd3244 /wd3254
            /FI ${CMAKE_CURRENT_SOURCE_DIR}/../../modern/scripts/force_undef_legacy_macros.h
            $<$<CONFIG:Debug>:/MTd /Od /RTC1>
            $<$<CONFIG:Release>:/MT /O2 /Ob2 /GF /Gy>
        )
    endif()
    target_compile_definitions(${_name} PRIVATE
        WIN32
        _WINDOWS
        _MBCS
        _DEBUG
        ${_defs}
    )
    target_link_directories(${_name} PRIVATE
        "${_yh_root}/build_yhlibrary/Release"
        "${_net_lib_dir}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(${_name} PRIVATE
        YHLibrary.lib
        4DyuchiNET.lib
        ws2_32
        odbc32
        odbccp32
        wininet
    )
    if(locale STREQUAL "KOR")
        # Phase 7.5i: Bug C-35 KOR LNK1104 mfc71.lib. The legacy MD5.lib
        # (compiled under VS2003) embeds /DEFAULTLIB:"mfc71.lib" /DEFAULTLIB:
        # "mfcs71.lib" and unresolved references to ATL::CStringT<…,StrTraitMFC_DLL<…>>
        # from CMD5Checksum::Final. MSVC14 (build host) has no mfc71.lib and
        # also lacks the "C++ MFC" buildtools component (mfcs140.lib). We
        # therefore drop the legacy MD5.lib and link MD5Checksum_vendor.cpp,
        # an RFC 1321 C++ port with zero MFC dependency. The vendor produces
        # hash digests byte-identical to MD5Checksum.lib (verified against the
        # standard MD5 test vectors), so login equivalence holds: callers
        # compute the hex string and compare against the SQL column.
        target_sources(${_name} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/MD5Checksum_vendor.cpp"
        )
    else()
        target_link_libraries(${_name} PRIVATE MD5.lib)
    endif()
    set_target_properties(${_name} PROPERTIES
        OUTPUT_NAME "DistributeServer_${locale}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        WIN32_EXECUTABLE TRUE
    )
endforeach()