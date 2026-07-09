# [Server]Agent — 墨香 legacy `AgentServer.exe` (game proxy / relay server)
#
# Phase 7.3: convert [Server]Agent/AgentServer.vcproj (VS 7.10 / 2003 format)
# into a CMakeLists.txt. Follows the Phase 7.4a [Server]Distribute recipe
# (commit 99c1559 + baaeeeb) — same shape, same shared [CC]ServerModule +
# [CC]Header kernel, same locale matrix.
#
# Build properties extracted from AgentServer.vcproj:
#   ConfigurationType    = 1 (Application / exe)
#   UseOfMFC             = 0 (no MFC)
#   CharacterSet         = 2 (Multibyte / MBCS)
#   PreprocessorDefinitions (Release):  WIN32;NDEBUG;_WINDOWS
#   PreprocessorDefinitions (Debug KOR):WIN32;_DEBUG;_WINDOWS;_USINGTOOL_;_AGENTSERVER;_FILE_BIN_;_KOR_LOCAL_
#                                       (and 4 sibling Debug_<LOCALE> configs)
#   RuntimeLibrary       = 2 (Release, /MD) / 3 (Debug, /MDd) per vcproj, but
#                           we use /MT (Release) / /MTd (Debug) to stay ABI-
#                           compatible with YHLibrary.lib / 4DyuchiNET.dll
#                           (all of which were migrated with /MT). A mixed
#                           /MD + /MT link line produces LNK4093 / LNK2005
#                           on the CRT symbols; see PHASE7_MIGRATION_RECIPE.md
#                           §3 "Runtime library unification".
#   PrecompiledHeader    = stdafx.h
#   SubSystem            = 2 (Windows GUI — no real GUI window, console-less
#                             network service)
#   TargetMachine        = 1 (x86)
#   AdditionalIncludeDirectories:
#       .\
#       ..\[Lib]YHLibrary
#       ..\[CC]Header
#       ..\[CC]ServerModule
#   AdditionalDependencies (Release):
#       odbc32.lib odbccp32.lib ..\[Lib]YHLibrary\YHLibrary.lib
#   AdditionalDependencies (Debug_HK, additional):
#       ggsrvlib25.lib  (nProtect GameGuard 2.5 SDK — vendored header exists
#                        but the .lib is MISSING from the repo; HK link will
#                        fail until the lib is restored. See Bug D-6.)
#   Locale-flavored Debug configs: 5 (= Debug|Win32 = KOR, Debug_JAPAN,
#                                                  Debug_CHINA, Debug_HK,
#                                                  Debug_TL)
#       Debug (KOR): _KOR_LOCAL_
#       Debug_JAPAN : _JAPAN_LOCAL_
#       Debug_CHINA : TAIWAN_LOCAL;_CHINA_LOCAL_
#       Debug_HK    : _HK_LOCAL_;_IGNORE_ASSERT_;_TW_LOCAL_;_NPROTECT_
#       Debug_TL    : _TL_LOCAL_
#
# Source files (from the <Files> block of vcproj, organized by Filter):
#   Server Filter:
#     Server.cpp              (WinMain entry point)
#     ServerSystem.cpp        (CServerSystem + CServerTable management)
#   Network Filter:
#     AgentNetworkMsgParser.cpp   (27 protocol categories — relay/coordination)
#     CommonNetworkMsgParser.cpp  (CC base for the network parser)
#   DB Filter:
#     AgentDBMsgParser.cpp    (account/char/guild/party DB I/O)
#   Helper Filter:
#     Crypt.cpp MHFile.cpp MHTimeManager.cpp
#     UserTable.cpp ServerTable.cpp
#     BobusangManager_Agent.cpp JackpotManager_Agent.cpp
#     ShoutManager.cpp SkillDalayManager.cpp
#     PunishManager.cpp PlustimeMgr.cpp TrafficLog.cpp
#     GMToolManager.cpp GMPowerList.cpp
#     HackShieldManager.cpp NProtectManager.cpp
#     MsgTable.cpp filteringtable.cpp
#   PCH:
#     StdAfx.cpp / StdAfx.h
#   Headers-only:
#     ggsrv25.h AntiCpSvrFunc.h (nProtect SDK headers, HK locale only)
#
# Cross-directory sources (must be built into the same executable):
#   [CC]ServerModule/ — BootManager, CommonDBMsgParser, Console, DataBase,
#                        MiniDumper, Network, ServerListManager  (7 cpp)
#   [CC]Header/        — vector.cpp, CommonCalcFunc.cpp, CommonGameFunc.cpp
#                                                                    (3 cpp)
#                        GameResourceManager.cpp is EXCLUDED: Agent's Server.cpp
#                        guards it behind #ifdef _MAPSERVER_, and it pulls in
#                        Map-specific headers (TacticManager.h etc.) that don't
#                        exist in the Agent include path.
#   [CC]Header/TargetList/ — TargetData.cpp, TargetList.cpp, TargetListIterator.cpp
#                        (3 cpp — needed because CommonStruct.h's MSGBASE-
#                        derived structs use CTargetList template instantiations)
#
# External dependencies (binary, pre-built by earlier phases):
#   [Lib]YHLibrary/build_yhlibrary/Release/YHLibrary.lib
#       (122,724 bytes — built Phase 7.1; static library of HSEL +
#        memory pool + file I/O + crypto + STL helpers)
#   4DyuchiNET_Latest/build_net/Release/4DyuchiNET.dll + .lib
#       (150,016 bytes DLL + 1,528-byte import lib — built Phase 7.2;
#        COM server providing CLSID_4DyuchiNET)
#   4DyuchiNET_Common/
#       (header-only IID_4DyuchiNET/CLSID_4DyuchiNET defines — no library)
#
# Phase 7.3 changes relative to the vcproj:
#   - StdAfx.h got the Phase 7.1 §3 _WINSOCKAPI_ guard, <winsock2.h> moved
#     before <windows.h>, locale switch documentation added, and LOG macro
#     undef'd after all game headers (Bug D-4 pattern).
#   - ErrorMsg.h stub created (mirrors Distribute's — prevents the client-side
#     ErrorMsg.h from being pulled in via [CC]Header/CommonDefine.h:69).
#   - ggsrv25.lib is linked for Debug_HK only; if the vendored lib is
#     missing from the repo (current status), the HK target will fail at
#     link time (Bug D-6). Release + other 4 Debug targets are unaffected.
#   - 5 Debug locale variants are exposed as separate CMake targets.
#   - Precompiled header is dropped (HSEL POC pattern).
#
# Byte-level parity target:
#   SWorking/AgentServer.exe — TBD bytes (legacy Debug build).
#   This CMakeLists targets the Debug (KOR) configuration as the canonical
#   build; the build script compares output bytes.
cmake_minimum_required(VERSION 3.16)
project(AgentServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Per-config options are attached at the target level (generator
    # expressions) so /O2 (Release) and /RTC1 (Debug) never collide.
    # /W3 /FS /MP are config-agnostic so they can stay global.
    #
    # Phase 7.5i: add /source-charset:utf-8 /execution-charset:utf-8 (same
    # rationale as [Server]Map/CMakeLists.txt:96-134 and
    # [Server]Distribute/CMakeLists.txt:104-115). The build host is cp936
    # (GBK Chinese Windows) — without /execution-charset:utf-8 MSVC would
    # silently turn Korean Hangul codepoints in string literals into '?'
    # placeholders, and the Debug_<LOCALE> configs (KOR/JAPAN/CHINA/HK/TL —
    # all with Korean codepoints in their build error messages) fail to parse
    # [CC]Header/CommonStruct.h. The /source-charset:utf-8 half is the actual
    # fix for the parse errors; the /execution-charset half keeps the
    # string-literal bytes intact. No _mbs* / isleadbyte / mbstowcs usage in
    # [Server]Agent so MBCS char* APIs are unaffected.
    add_compile_options(/W3 /FS /MP /source-charset:utf-8 /execution-charset:utf-8)
endif()

# -----------------------------------------------------------------------------
# Source list
# -----------------------------------------------------------------------------
set(_repo_root     "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(_yh_root       "${_repo_root}/[Lib]YHLibrary")
# YHLibrary was rebuilt with VS17/Win32 in Phase 7.1; VS17 multi-config
# layout puts the output in build_yhlibrary/Release/.
set(_yh_lib        "${_yh_root}/build_yhlibrary/Release/YHLibrary.lib")
set(_cc_header     "${_repo_root}/[CC]Header")
set(_cc_server     "${_repo_root}/[CC]ServerModule")
set(_net_common    "${_repo_root}/4DyuchiNET_Common")
set(_net_root      "${_repo_root}/4DyuchiNET_Latest")
set(_net_lib_dir   "${_net_root}/build_net/Release")
set(_net_lib       "${_net_lib_dir}/4DyuchiNET.lib")
set(_net_dll       "${_net_lib_dir}/4DyuchiNET.dll")
# Phase 7.3 fix: MsgTable.cpp uses ICCreate/ICRelease/ICAllocIndex/ICFreeIndex/
# ICInitialize from SS3DGFunc (naked x86 ASM DLL, vendored at 4DyuchiGXGFunc/).
# Agent also uses CIndexGenerator (vendored IndexGenerator.lib from YHLibrary).
# Both deps were discovered at link time — the Agent vcproj's Release config did
# NOT list them in AdditionalDependencies, but the legacy build shipped a working
# AgentServer.exe which means the old linker resolved them through a different
# path (possibly a .lib search dir or an MSVC auto-link pragma). Tracked as Bug D-7.
set(_gxgfunc        "${_repo_root}/4DyuchiGXGFunc")
set(_ss3dgfunc_lib  "${_gxgfunc}/SS3DGFunc.lib")
set(_yh_index_lib   "${_yh_root}/IndexGenerator.lib")
# ggsrv25.lib — nProtect GameGuard 2.5 SDK. Vendored in Agent directory
# for the HK locale (Debug_HK only). Header (ggsrv25.h) exists; the .lib
# is MISSING from the repo. HK link step will fail until ggsrv25.lib is
# restored. Tracked as Bug D-6.
set(_ggsrv_lib     "${CMAKE_CURRENT_SOURCE_DIR}/ggsrv25.lib")

# -----------------------------------------------------------------------------
# Release target (locale-neutral; matches SWorking/AgentServer.exe in spirit)
# -----------------------------------------------------------------------------
add_executable(AgentServer
    # [Server]Agent/ — 24 cpp
    AgentDBMsgParser.cpp
    AgentNetworkMsgParser.cpp
    BobusangManager_Agent.cpp
    CommonNetworkMsgParser.cpp
    Crypt.cpp
    filteringtable.cpp
    GMPowerList.cpp
    GMToolManager.cpp
    HackShieldManager.cpp
    JackpotManager_Agent.cpp
    MHFile.cpp
    MHTimeManager.cpp
    MsgTable.cpp
    NProtectManager.cpp
    PlustimeMgr.cpp
    PunishManager.cpp
    Server.cpp
    ServerSystem.cpp
    ServerTable.cpp
    ShoutManager.cpp
    SkillDalayManager.cpp
    StdAfx.cpp
    TrafficLog.cpp
    UserTable.cpp
    # [CC]ServerModule/ — 7 cpp (cross-directory)
    ${_cc_server}/BootManager.cpp
    ${_cc_server}/CommonDBMsgParser.cpp
    ${_cc_server}/Console.cpp
    ${_cc_server}/DataBase.cpp
    ${_cc_server}/MiniDumper.cpp
    ${_cc_server}/Network.cpp
    ${_cc_server}/ServerListManager.cpp
    # [CC]Header/ — 3 cpp (Agent does NOT include GameResourceManager.cpp
    # because Server.cpp guards the include + GAMERESRCMNGR call behind
    # #ifdef _MAPSERVER_, and GameResourceManager.cpp pulls in Map-specific
    # headers like TacticManager.h that don't exist in the Agent/CC path).
    ${_cc_header}/CommonCalcFunc.cpp
    ${_cc_header}/CommonGameFunc.cpp
    ${_cc_header}/vector.cpp
    # [CC]Header/TargetList/ — 3 cpp (MSGBASE-derived structs in
    # CommonStruct.h use CTargetList template instantiations).
    ${_cc_header}/TargetList/TargetData.cpp
    ${_cc_header}/TargetList/TargetList.cpp
    ${_cc_header}/TargetList/TargetListIterator.cpp
)

target_include_directories(AgentServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${_cc_server}
    ${_cc_header}
    ${_yh_root}
    ${_net_common}
    ${_gxgfunc}
)

if(MSVC)
    target_compile_options(AgentServer PRIVATE
        # /permissive- is NOT used (Bug C-28 + C-29 in KNOWN_BUGS.md):
        #   - CommonStruct.h:436 reuses `i` declared in a prior for-loop
        #     init (legacy MSVC7 extension, banned under /permissive-).
        #   - TargetListIterator.h:52 declares a qualified member function
        #     inside the class body (legacy MSVC7 extension, banned under
        #     /permissive- as error C4596).
        # Same workaround as DBThread (Bug C-14) and Distribute.
        #
        # /Zc:strictStrings- rejects legacy TCHAR-style calls in DB.h etc.
        # Mirror the Phase 7.1 BaseNetwork / DBThread compromise.
        /Zc:strictStrings-
        # /Zc:forScope- restores the pre-VS2005 for-init variable scope
        # rule. Several legacy sources in [CC]Header/ rely on this.
        # See docs/KNOWN_BUGS.md Bug C-28.
        /Zc:forScope-
        # Suppress legacy-source compat diagnostics (Bug C-29).
        /wd4596 /wd3244 /wd3254
        # Belt-and-suspenders: force-include the LOG-undef header
        # so that .cpp files which include stdafx.h but happen to pull
        # in Windows headers via a different path still get the fix.
        # Tracked in docs/KNOWN_BUGS.md Bug D-1.
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
target_compile_definitions(AgentServer PRIVATE
    WIN32
    _WINDOWS
    _MBCS
    _AGENTSERVER
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Linker line (Release):
#   odbc32.lib odbccp32.lib ..\[Lib]YHLibrary\YHLibrary.lib
# Plus:
#   ws2_32          — <winsock2.h> in StdAfx.h
#   4DyuchiNET.lib  — COM import library (built Phase 7.2)
# Agent does NOT use MD5.lib or Wininet.lib (Distribute-only).
target_link_directories(AgentServer PRIVATE
    "${_yh_root}/build_yhlibrary/Release"
    "${_yh_root}"  # for IndexGenerator.lib (vendored next to source)
    "${_net_lib_dir}"
    "${_gxgfunc}"  # for SS3DGFunc.lib (vendored next to DLL)
)
target_link_libraries(AgentServer PRIVATE
    YHLibrary.lib
    IndexGenerator.lib
    4DyuchiNET.lib
    SS3DGFunc.lib
    ws2_32
    odbc32
    odbccp32
)
# Phase 7.3 fix: drop the stale `-defaultlib:LIBC` directive baked into
# the vendored IndexGenerator.obj (same as Map, Phase 7.5).
target_link_options(AgentServer PRIVATE
    /NODEFAULTLIB:LIBC
)

# Output naming: legacy produced "AgentServer.exe" with capital A.
# SubSystem = 2 (Windows GUI) per the legacy vcproj — the entry point
# is WinMain (Server.cpp), not main. Without WIN32 (Windows), the linker
# looks for `_main` and fails with LNK2019.
set_target_properties(AgentServer PROPERTIES
    OUTPUT_NAME AgentServer
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    WIN32_EXECUTABLE TRUE
)

# Phase 7.3 sanity: print a STATUS line so the build log records which
# 4DyuchiNET/YHLibrary binaries we linked against.
message(STATUS "Phase 7.3 AgentServer configuration:")
message(STATUS "  YHLibrary.lib    = ${_yh_lib}")
message(STATUS "  4DyuchiNET.lib   = ${_net_lib}")
message(STATUS "  4DyuchiNET.dll   = ${_net_dll}")
message(STATUS "  Sources: 37 cpp (24 Agent + 7 ServerModule + 3 Header + 3 TargetList)")

# -----------------------------------------------------------------------------
# Debug locale targets — one per legacy vcproj Debug_<LOCALE> config.
# -----------------------------------------------------------------------------
# Agent's locale macros (per legacy vcproj):
#   Debug (KOR):    _KOR_LOCAL_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_
#   Debug_JAPAN:    _JAPAN_LOCAL_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_
#   Debug_CHINA:    TAIWAN_LOCAL;_CHINA_LOCAL_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_
#   Debug_HK:       _HK_LOCAL_;_IGNORE_ASSERT_;_TW_LOCAL_;_NPROTECT_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_
#   Debug_TL:       _TL_LOCAL_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_
# All Debug configs link the same libs as Release (odbc32, odbccp32, YHLibrary,
# 4DyuchiNET, ws2_32). Debug_HK additionally links ggsrv25.lib.
foreach(locale IN ITEMS KOR JAPAN CHINA HK TL)
    set(_defs "")
    set(_name "AgentServer_Debug_${locale}")
    if(locale STREQUAL "KOR")
        set(_defs "_KOR_LOCAL_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_")
    elseif(locale STREQUAL "JAPAN")
        set(_defs "_JAPAN_LOCAL_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_")
    elseif(locale STREQUAL "CHINA")
        set(_defs "_CHINA_LOCAL_;TAIWAN_LOCAL;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_")
    elseif(locale STREQUAL "HK")
        set(_defs "_HK_LOCAL_;_IGNORE_ASSERT_;_TW_LOCAL_;_NPROTECT_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_")
    elseif(locale STREQUAL "TL")
        set(_defs "_TL_LOCAL_;_USINGTOOL_;_AGENTSERVER_;_FILE_BIN_")
    endif()

    add_executable(${_name}
        AgentDBMsgParser.cpp
        AgentNetworkMsgParser.cpp
        BobusangManager_Agent.cpp
        CommonNetworkMsgParser.cpp
        Crypt.cpp
        filteringtable.cpp
        GMPowerList.cpp
        GMToolManager.cpp
        HackShieldManager.cpp
        JackpotManager_Agent.cpp
        MHFile.cpp
        MHTimeManager.cpp
        MsgTable.cpp
        NProtectManager.cpp
        PlustimeMgr.cpp
        PunishManager.cpp
        Server.cpp
        ServerSystem.cpp
        ServerTable.cpp
        ShoutManager.cpp
        SkillDalayManager.cpp
        StdAfx.cpp
        TrafficLog.cpp
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
        ${_gxgfunc}
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
        _AGENTSERVER
        _DEBUG
        ${_defs}
    )
    target_link_directories(${_name} PRIVATE
        "${_yh_root}/build_yhlibrary/Release"
        "${_yh_root}"  # for IndexGenerator.lib (vendored next to source)
        "${_net_lib_dir}"
        "${_gxgfunc}"  # for SS3DGFunc.lib (vendored next to DLL)
    )
    target_link_libraries(${_name} PRIVATE
        YHLibrary.lib
        IndexGenerator.lib
        4DyuchiNET.lib
        SS3DGFunc.lib
        ws2_32
        odbc32
        odbccp32
    )
    # Phase 7.3 fix: drop stale `-defaultlib:LIBC` directive (same as Map, Phase 7.5).
    target_link_options(${_name} PRIVATE
        /NODEFAULTLIB:LIBC
    )
    # Debug_HK additionally provides nProtect GameGuard 2.5 SDK symbols.
    # The vendored header (ggsrv25.h) exists; the .lib is MISSING from the
    # repo (Bug D-6). Phase 7.5n ships a source-level C++ stub at
    # ggsrv25_vendor_stub.cpp that implements all the symbols declared in
    # ggsrv25.h (InitGameguardAuth / GGAuthUpdateTimer / CleanupGameguardAuth
    # / ... + CCSAuth2 class + GGAUTH_* C API) as empty no-ops that return
    # success-equivalent values. Linking this source-instead-of-lib means
    # the HK target builds clean, the legacy CNProtectManager init path
    # succeeds, and the gameplay layer (which does not depend on nProtect
    # GameGuard) keeps working 1:1.  Real GGAuth 2.5 wire-protocol support
    # is deferred to a future phase (8+) once upstream ggsrv25.lib is
    # restored. KOR/JP/CHINA/TL targets do not define `_NPROTECT_` so they
    # never reference any of these symbols and the stub is silent.
    if(locale STREQUAL "HK")
        target_sources(${_name} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/ggsrv25_vendor_stub.cpp"
        )
        message(STATUS "  Debug_HK: ggsrv25_vendor_stub.cpp linked instead of missing ggsrv25.lib (Bug D-6 mitigated Phase 7.5n)")
    endif()
    set_target_properties(${_name} PROPERTIES
        OUTPUT_NAME "AgentServer_${locale}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        WIN32_EXECUTABLE TRUE
    )
endforeach()
