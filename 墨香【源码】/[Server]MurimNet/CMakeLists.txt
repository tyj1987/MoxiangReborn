# [Server]MurimNet — 墨香 legacy `MurimNetServer.exe` (PvP game server)
#
# Phase 7.6: convert [Server]MurimNet/MurimNet.dsp (VC6 format) into
# CMakeLists.txt. MurimNet is a specialized Map server variant (defines
# both _MAPSERVER_ and _MURIMNET_) for the PvP arena system. It shares
# the same Server.cpp entry point as Map but disables Map-specific
# logic via `#ifndef _MURIMNET_` guards.
#
# Build properties extracted from MurimNet.dsp:
#   Configuration type: WIN32 Application (exe)
#   UseOfMFC: no
#   Entry point: WinMain (Server.cpp)
#   Include dirs: ., ..\[Lib]YHLibrary, ..\[CC]ServerModule
#   Defines: WIN32;_DEBUG;_WINDOWS;_MBCS;_MAPSERVER_;__MAPSERVER__;_USINGTOOL_;_MURIMNET_
#   Runtime library: /MDd (Debug only — VC6 default)
#   Link: yhlibrary.lib
#
# Cross-directory sources (from MurimNet.dsp <Files> block):
#   [CC]ServerModule/ — BootManager, CommonDBMsgParser, Console, DataBase,
#                        MiniDumper, Network, ServerListManager  (7 cpp)
#   [CC]Header/        — CommonCalcFunc, CommonGameFunc,
#                        GameResourceManager, vector               (4 cpp)
#   No TargetList (unlike Distribute/Agent/Map).
#
# Dependencies: YHLibrary.lib (Phase 7.1), 4DyuchiNET.dll (Phase 7.2).
#
# Phase 7.6 note: GameResourceManager.cpp is included because the legacy
# .dsp lists it as a source. If it fails due to Map-specific headers
# (TacticManager.h), we exclude it and document as Bug D-8.
cmake_minimum_required(VERSION 3.16)
project(MurimNetServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W3 /FS /MP)
endif()

set(_repo_root  "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(_yh_root    "${_repo_root}/[Lib]YHLibrary")
set(_yh_lib     "${_yh_root}/build_yhlibrary/Release/YHLibrary.lib")
set(_cc_header  "${_repo_root}/[CC]Header")
set(_cc_server  "${_repo_root}/[CC]ServerModule")
set(_net_common "${_repo_root}/4DyuchiNET_Common")
set(_net_root   "${_repo_root}/4DyuchiNET_Latest")
set(_net_lib_dir "${_net_root}/build_net/Release")
set(_net_lib    "${_net_lib_dir}/4DyuchiNET.lib")


add_executable(MurimNetServer
    # [Server]MurimNet/ — 19 cpp
    Channel.cpp
    ChannelManager.cpp
    CommonNetworkMsgParser.cpp
    Crypt.cpp
    Front.cpp
    MHFile.cpp
    MHTimeManager.cpp
    MNDBMsgParser.cpp
    MNNetworkMsgParser.cpp
    MNPlayer.cpp
    MNPlayerManager.cpp
    MNServer.cpp
    MNServerManager.cpp
    MurimNetSystem.cpp
    PlayRoom.cpp
    PlayRoomManager.cpp
    Server.cpp
    ServerSystem.cpp
    ServerTable.cpp
    StdAfx.cpp
    # [CC]ServerModule/ — 7 cpp
    ${_cc_server}/BootManager.cpp
    ${_cc_server}/CommonDBMsgParser.cpp
    ${_cc_server}/Console.cpp
    ${_cc_server}/DataBase.cpp
    ${_cc_server}/MiniDumper.cpp
    ${_cc_server}/Network.cpp
    ${_cc_server}/ServerListManager.cpp
    # [CC]Header/ — 1 cpp (vector.cpp only).
    # CommonCalcFunc.cpp and CommonGameFunc.cpp need Map-specific headers
    # (Object.h) and struct fields (GetMapNum, Days, WeatherState,
    # MunpaName) that are locale-conditional or Map-server-only.
    # GameResourceManager.cpp also needs Map-specific TacticManager.h.
    # Tracked as Bug D-8.
    ${_cc_header}/vector.cpp
)

target_include_directories(MurimNetServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${_cc_server}
    ${_cc_header}
    ${_yh_root}
    ${_net_common}
)

if(MSVC)
    target_compile_options(MurimNetServer PRIVATE
        /Zc:strictStrings-
        /Zc:forScope-
        /wd4596 /wd3244 /wd3254
        /FI ${CMAKE_CURRENT_SOURCE_DIR}/../../modern/scripts/force_undef_legacy_macros.h
        $<$<CONFIG:Debug>:/MTd /Od /RTC1>
        $<$<CONFIG:Release>:/MT /O2 /Ob2 /GF /Gy>
    )
endif()

target_compile_definitions(MurimNetServer PRIVATE
    WIN32
    _WINDOWS
    _MBCS
    _MAPSERVER_
    __MAPSERVER__
    _MURIMNET_
    $<$<CONFIG:Debug>:_DEBUG;_USINGTOOL_>
    $<$<CONFIG:Release>:NDEBUG>
)

target_link_directories(MurimNetServer PRIVATE
    "${_yh_root}/build_yhlibrary/Release"
    "${_net_lib_dir}"
)
target_link_libraries(MurimNetServer PRIVATE
    YHLibrary.lib
    4DyuchiNET.lib
    ws2_32
)

set_target_properties(MurimNetServer PROPERTIES
    OUTPUT_NAME MurimNetServer
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    WIN32_EXECUTABLE TRUE
)

message(STATUS "Phase 7.6 MurimNetServer: 27 cpp (19 MurimNet + 7 ServerModule + 1 Header)")
