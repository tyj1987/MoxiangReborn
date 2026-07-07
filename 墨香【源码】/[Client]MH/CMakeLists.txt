# [Client]MH — 墨香 legacy `MHClient-Connect.exe` (game client)
#
# Phase 6: convert [Client]MH/MHClient.vcproj (VS 7.10 / 2003 format)
# into CMakeLists.txt. This is the MAIN game client — the largest single
# target in the repository (460 cpp, ~35-40万 LOC).
#
# KEY FINDING: The client does NOT use MFC (UseOfMFC="0" in vcproj).
# It has a custom Win32 GDI UI framework (cWindow + 40+ widget classes
# in interface/). This means no MFC decoupling is needed — the client
# is pure Win32 + 4Dyuchi engine + HackShield/nProtect stubs.
#
# Build properties from MHClient.vcproj (Semi_ReleaseChina|Win32 config):
#   ConfigurationType    = 1 (Application / exe)
#   UseOfMFC             = 0 (no MFC)
#   CharacterSet         = 2 (MBCS)
#   Include dirs: ., [Lib]YHLibrary, Audio, [CC]BattleSystem, [CC]Skill,
#                  [CC]Ability, [CC]Header
#   Defines: _DEBUG;WIN32;_WINDOWS;USE_WMA;_SEMIRELEASE;TAIWAN_LOCAL
#            (plus _MHCLIENT_, _CRYPTCHECK_, _FILE_BIN_ from StdAfx)
#   RuntimeLibrary       = 3 (/MDd Debug, /MD Release per vcproj)
#                          We use /MT(d) for ABI compat (server convention)
#   PrecompiledHeader    = stdafx.h
#   SubSystem            = 2 (Windows GUI)
#   Output: ..\CWorking\MHClient-Connect.exe
#
# Link libraries (from vcproj AdditionalDependencies):
#   wmstub.lib         — Windows Media stub (intro video playback)
#   ComCtl32.lib       — Win32 Common Controls
#   strmiids.lib       — DirectShow GUIDs (video)
#   vfw32.lib          — Video for Windows (cinematic playback)
#   Audio/SoundLib.lib — Miles Sound System (vendored)
#   imm32.lib          — Input Method Editor
#   odbc32.lib         — ODBC database
#   odbccp32.lib       — ODBC installer
#   Winmm.lib          — Windows Multimedia (timeGetTime)
#   SS3DGFunc.lib      — 4Dyuchi geometry functions (vendored x86 ASM)
#   d3dx8.lib          — DirectX 8.1 helpers  ← REPLACE with Phase 5 DX11
#   yhlibrary.lib      — Legacy utility library (Phase 7.1)
#   IndexGenerator.lib — Vendored YHLibrary index gen
#   wsock32.lib        — Winsock 1.1  ← REPLACE with ws2_32
#   dinput8.lib        — DirectInput 8 (keyboard/mouse)
#
# Phase 6 strategy: FIRST BUILD — minimal viable compilation.
#   1. Skip intro video (stub out wmstub/strmiids/vfw32)
#   2. Skip HackShield/nProtect (don't define _HACK_SHIELD_ / _NPROTECT_)
#   3. Skip Miles Sound (stub out Audio/ files)
#   4. Use FreeImage SDK (vendored headers + DLL exist)
#   5. Use Phase 5 DX11 renderer for d3dx8 replacement
#   6. Use ws2_32.lib instead of wsock32.lib
#   7. Default locale: _KOR_LOCAL_ (like server builds)
#
# Cross-directory sources (from vcproj <Files> block):
#   [CC]Ability/ — 26 files (13 cpp + 13 h)
#   [CC]BattleSystem/ — 30 files (15 cpp + 15 h) + 6 subdirs × 3 cpp each
#   [CC]Header/ — CommonCalcFunc, CommonGameFunc, GameResourceManager,
#                 TargetList (3 cpp), vector
#   [CC]Quest/ — 30 files (15 cpp + 15 h)
#   [CC]Skill/ — 60 files (30 cpp + 30 h)
#   [CC]Suryun/ — 5 files (SuryunDefine + SuryunManager_client)
#   4DyuchiGRX_common/ — headers only (IRenderer.h, IExecutive.h, etc.)
#   4DyuchiGRX_myself97_util/ — MouseLib.cpp + MToolMouseStatus.cpp
#   4DyuchiGXGFunc/ — headers only (global.h, type.h)
#
# This CMakeLists.txt is the RECIPE — actual build verification is
# deferred to a downstream gate task (same pattern as Map/Agent/MurimNet).
cmake_minimum_required(VERSION 3.16)
project(MHClient CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W3 /FS /MP)
endif()

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
set(_repo_root     "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(_yh_root       "${_repo_root}/[Lib]YHLibrary")
set(_yh_lib        "${_yh_root}/build_yhlibrary/Release/YHLibrary.lib")
set(_yh_index_lib  "${_yh_root}/IndexGenerator.lib")
set(_cc_header     "${_repo_root}/[CC]Header")
set(_cc_ability    "${_repo_root}/[CC]Ability")
set(_cc_battle     "${_repo_root}/[CC]BattleSystem")
set(_cc_quest      "${_repo_root}/[CC]Quest")
set(_cc_skill      "${_repo_root}/[CC]Skill")
set(_cc_suryun     "${_repo_root}/[CC]Suryun")
set(_grx_common    "${_repo_root}/4DyuchiGRX_common")
set(_grx_mouse     "${_repo_root}/4DyuchiGRX_myself97_util")
set(_gxgfunc       "${_repo_root}/4DyuchiGXGFunc")
set(_net_common    "${_repo_root}/4DyuchiNET_Common")
set(_net_root      "${_repo_root}/4DyuchiNET_Latest")
set(_net_lib_dir   "${_net_root}/build_net/Release")
set(_net_lib       "${_net_lib_dir}/4DyuchiNET.lib")

# ---------------------------------------------------------------------------
# Client source files (globbing — same approach as Map server)
# ---------------------------------------------------------------------------
file(GLOB _client_root_cpp "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB _audio_cpp       "${CMAKE_CURRENT_SOURCE_DIR}/Audio/*.cpp")
file(GLOB _effect_cpp      "${CMAKE_CURRENT_SOURCE_DIR}/Effect/*.cpp")
file(GLOB _engine_cpp      "${CMAKE_CURRENT_SOURCE_DIR}/Engine/*.cpp")
file(GLOB _input_cpp       "${CMAKE_CURRENT_SOURCE_DIR}/input/*.cpp")
file(GLOB _interface_cpp   "${CMAKE_CURRENT_SOURCE_DIR}/interface/*.cpp")

add_executable(MHClient
    ${_client_root_cpp}
    ${_audio_cpp}
    ${_effect_cpp}
    ${_engine_cpp}
    ${_input_cpp}
    ${_interface_cpp}
    # Cross-directory sources — same set as Map server
    # [CC]Ability/ — 14 cpp
    ${_cc_ability}/Ability.cpp
    ${_cc_ability}/AbilityCommonHeader.cpp
    ${_cc_ability}/AbilityGroup.cpp
    ${_cc_ability}/AbilityIcon.cpp
    ${_cc_ability}/AbilityInfo.cpp
    ${_cc_ability}/AbilityManager.cpp
    ${_cc_ability}/AbilityUpdater.cpp
    ${_cc_ability}/AbilityUpdater_ApplyData.cpp
    ${_cc_ability}/AbilityUpdater_Interface.cpp
    ${_cc_ability}/AbilityUse.cpp
    ${_cc_ability}/AbilityUse_Job.cpp
    ${_cc_ability}/AbilityUse_KyungGong.cpp
    ${_cc_ability}/AbilityUse_Pet.cpp
    ${_cc_ability}/AbilityUse_Society.cpp
    # [CC]BattleSystem/ — 21 cpp (root + 6 sub-dirs × 3)
    ${_cc_battle}/Battle.cpp
    ${_cc_battle}/BattleFactory.cpp
    ${_cc_battle}/BattleObject.cpp
    ${_cc_battle}/BattleRect.cpp
    ${_cc_battle}/BattleSystem_Client.cpp
    ${_cc_battle}/BattleSystem_Server.cpp
    ${_cc_battle}/BattleTeam.cpp
    ${_cc_battle}/GTournament/BattleFactory_GTournament.cpp
    ${_cc_battle}/GTournament/BattleTeam_GTournament.cpp
    ${_cc_battle}/GTournament/Battle_GTournament.cpp
    ${_cc_battle}/MunpaField/BattleFactory_MunpaField.cpp
    ${_cc_battle}/MunpaField/BattleTeam_MunpaField.cpp
    ${_cc_battle}/MunpaField/Battle_MunpaField.cpp
    ${_cc_battle}/MurimField/battle_murimfield.cpp
    ${_cc_battle}/MurimField/battlefactory_murimfield.cpp
    ${_cc_battle}/MurimField/battleteam_murimfield.cpp
    ${_cc_battle}/SiegeWar/BattleFactory_SiegeWar.cpp
    ${_cc_battle}/SiegeWar/BattleTeam_SiegeWar.cpp
    ${_cc_battle}/SiegeWar/Battle_SiegeWar.cpp
    ${_cc_battle}/Suryun/BattleFactory_Suryun.cpp
    ${_cc_battle}/Suryun/BattleTeam_Suryun.cpp
    ${_cc_battle}/Suryun/Battle_Suryun.cpp
    ${_cc_battle}/VimuStreet/BattleFactory_VimuStreet.cpp
    ${_cc_battle}/VimuStreet/BattleTeam_VimuStreet.cpp
    ${_cc_battle}/VimuStreet/Battle_VimuStreet.cpp
    # [CC]Header/ — 7 cpp
    ${_cc_header}/CommonCalcFunc.cpp
    ${_cc_header}/CommonGameFunc.cpp
    ${_cc_header}/GameResourceManager.cpp
    ${_cc_header}/TargetList/TargetData.cpp
    ${_cc_header}/TargetList/TargetList.cpp
    ${_cc_header}/TargetList/TargetListIterator.cpp
    ${_cc_header}/vector.cpp
    # [CC]Quest/ — 16 cpp
    ${_cc_quest}/QuestCondition.cpp
    ${_cc_quest}/QuestEvent.cpp
    ${_cc_quest}/QuestExecute.cpp
    ${_cc_quest}/QuestExecute_Count.cpp
    ${_cc_quest}/QuestExecute_Item.cpp
    ${_cc_quest}/QuestExecute_Quest.cpp
    ${_cc_quest}/QuestExecute_RandomItem.cpp
    ${_cc_quest}/QuestExecute_Time.cpp
    ${_cc_quest}/QuestInfo.cpp
    ${_cc_quest}/QuestLimit.cpp
    ${_cc_quest}/QuestLimitKind.cpp
    ${_cc_quest}/QuestLimitKind_Level.cpp
    ${_cc_quest}/QuestLimitKind_Quest.cpp
    ${_cc_quest}/QuestNpc.cpp
    ${_cc_quest}/QuestNpcData.cpp
    ${_cc_quest}/QuestNpcScript.cpp
    ${_cc_quest}/QuestNpcScriptData.cpp
    ${_cc_quest}/QuestScriptLoader.cpp
    ${_cc_quest}/QuestString.cpp
    ${_cc_quest}/QuestTrigger.cpp
    ${_cc_quest}/SubQuestInfo.cpp
    # [CC]Skill/ — 31 cpp
    ${_cc_skill}/AmplifiedPower.cpp
    ${_cc_skill}/DelayGroup.cpp
    ${_cc_skill}/DetectState.cpp
    ${_cc_skill}/SkillArea.cpp
    ${_cc_skill}/SkillAreaData.cpp
    ${_cc_skill}/SkillAreaManager.cpp
    ${_cc_skill}/SkillManager_client.cpp
    ${_cc_skill}/SkillObjectAttachUnit.cpp
    ${_cc_skill}/SkillObjectAttachUnit_Attack.cpp
    ${_cc_skill}/SkillObjectAttachUnit_CounterAttack.cpp
    ${_cc_skill}/SkillObjectAttachUnit_Defence.cpp
    ${_cc_skill}/SkillObjectAttachUnit_Dodge1.cpp
    ${_cc_skill}/SkillObjectAttachUnit_Immune.cpp
    ${_cc_skill}/SkillObjectAttachUnit_MaxLife.cpp
    ${_cc_skill}/SkillObjectAttachUnit_MoveSpeed.cpp
    ${_cc_skill}/SkillObjectAttachUnit_SkipEffect.cpp
    ${_cc_skill}/SkillObjectAttachUnit_Vampiric.cpp
    ${_cc_skill}/SkillObjectFactory.cpp
    ${_cc_skill}/SkillObjectFirstUnit.cpp
    ${_cc_skill}/SkillObjectFirstUnit_Attack.cpp
    ${_cc_skill}/SkillObjectFirstUnit_Job.cpp
    ${_cc_skill}/SkillObjectFirstUnit_Recover.cpp
    ${_cc_skill}/SkillObjectFirstUnit_SingleSpecialState.cpp
    ${_cc_skill}/SkillObjectSingleUnit.cpp
    ${_cc_skill}/SkillObjectSingleUnit_Attack.cpp
    ${_cc_skill}/SkillObjectSingleUnit_Heal.cpp
    ${_cc_skill}/SkillObjectSingleUnit_Recharge.cpp
    ${_cc_skill}/SkillObjectStateUnit.cpp
    ${_cc_skill}/SkillObjectStateUnit_AmplifiedPower.cpp
    ${_cc_skill}/SkillObjectStateUnit_DummyState.cpp
    ${_cc_skill}/SkillObjectStateUnit_StatusAttach.cpp
    ${_cc_skill}/SkillObjectStateUnit_TieUp.cpp
    ${_cc_skill}/SkillObjectTargetList.cpp
    ${_cc_skill}/SkillObjectTargetList_Area.cpp
    ${_cc_skill}/SkillObjectTargetList_Circle.cpp
    ${_cc_skill}/SkillObjectTargetList_Fixed.cpp
    ${_cc_skill}/SkillObjectTargetList_One.cpp
    ${_cc_skill}/SkillObjectTerminator.cpp
    ${_cc_skill}/SkillObjectTerminator_Time.cpp
    ${_cc_skill}/SkillObject_Mine.cpp
    ${_cc_skill}/SkillObject_client.cpp
    ${_cc_skill}/SpecialState.cpp
    ${_cc_skill}/Status.cpp
    ${_cc_skill}/StunState.cpp
    ${_cc_skill}/ValueInSung.cpp
    ${_cc_skill}/skillinfo.cpp
    ${_cc_skill}/skillobject_server.cpp
    # [CC]Suryun/ — 2 cpp (client only)
    ${_cc_suryun}/SuryunDefine.cpp
    ${_cc_suryun}/SuryunManager_client.cpp
    # 4DyuchiGRX_myself97_util — 2 cpp (mouse input)
    ${_grx_mouse}/MouseLib.cpp
    ${_grx_mouse}/MToolMouseStatus.cpp
)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(MHClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine
    ${CMAKE_CURRENT_SOURCE_DIR}/Effect
    ${CMAKE_CURRENT_SOURCE_DIR}/input
    ${_yh_root}
    ${_cc_header}
    ${_cc_ability}
    ${_cc_battle}
    ${_cc_quest}
    ${_cc_skill}
    ${_cc_suryun}
    ${_grx_common}
    ${_grx_mouse}
    ${_gxgfunc}
    ${_net_common}
)

# ---------------------------------------------------------------------------
# Compile options (Phase 7 server convention)
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(MHClient PRIVATE
        /Zc:strictStrings-
        /Zc:forScope-
        /wd4596 /wd3244 /wd3254
        /FI ${CMAKE_CURRENT_SOURCE_DIR}/../../modern/scripts/force_undef_legacy_macros.h
        $<$<CONFIG:Debug>:/MTd /Od /RTC1>
        $<$<CONFIG:Release>:/MT /O2 /Ob2 /GF /Gy>
    )
endif()

# ---------------------------------------------------------------------------
# Preprocessor definitions
# ---------------------------------------------------------------------------
# _MHCLIENT_ (from StdAfx.h) distinguishes client-specific code paths
# from server builds (which use _DISTRIBUTESERVER_ / _MAPSERVER_ / etc.)
# _KOR_LOCAL_ = default locale (Korea — same as server builds)
# We do NOT define _HACK_SHIELD_ or _NPROTECT_ (stubbed out).
# We do NOT define _SEMIRELEASE (dev build — enables _CHEATENABLE_).
# USE_WMA is from the vcproj Semi_ReleaseChina config; skip for default KOR.
target_compile_definitions(MHClient PRIVATE
    WIN32
    _WINDOWS
    _MBCS
    _MHCLIENT_
    _CRYPTCHECK_
    _FILE_BIN_
    _FOR_MUKHYANG
    _CLIENTVERSIONCHK_
    _KOR_LOCAL_
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
# Phase 6 changes from the legacy vcproj:
#   - Drop wmstub.lib, strmiids.lib, vfw32.lib (video — stubbed)
#   - Drop Audio/SoundLib.lib (Miles Sound — stubbed, see Audio/ stubs)
#   - Drop d3dx8.lib (DX8.1 — replaced by Phase 5 DX11 backend)
#   - Replace wsock32.lib with ws2_32.lib (WinSock 2)
#   - Keep ComCtl32, imm32, odbc32, odbccp32, Winmm, SS3DGFunc,
#     YHLibrary, IndexGenerator, dinput8
target_link_directories(MHClient PRIVATE
    "${_yh_root}/build_yhlibrary/Release"
    "${_yh_root}"
    "${_net_lib_dir}"
    "${_gxgfunc}"
)
target_link_libraries(MHClient PRIVATE
    YHLibrary.lib
    IndexGenerator.lib
    4DyuchiNET.lib
    SS3DGFunc.lib
    ComCtl32
    imm32
    odbc32
    odbccp32
    Winmm
    dinput8
    ws2_32
)
# Drop stale -defaultlib:LIBC from vendored IndexGenerator.lib (Phase 7.5 fix).
target_link_options(MHClient PRIVATE
    /NODEFAULTLIB:LIBC
)

# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------
# Legacy output was CWorking/MHClient-Connect.exe.
# Our modern output is MHClient.exe (or MHClient-Modern.exe).
set_target_properties(MHClient PROPERTIES
    OUTPUT_NAME MHClient-Modern
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    WIN32_EXECUTABLE TRUE
    # Add .rc file for Windows icon/version info
)

# ---------------------------------------------------------------------------
# Sanity
# ---------------------------------------------------------------------------
message(STATUS "Phase 6 MHClient configuration:")
message(STATUS "  YHLibrary.lib      = ${_yh_lib}")
message(STATUS "  IndexGenerator.lib = ${_yh_index_lib}")
message(STATUS "  SS3DGFunc.lib      = ${_gxgfunc}/SS3DGFunc.lib")
message(STATUS "  Sources: ~540 cpp (client~460 + CC~80 + mouse~2)")
message(STATUS "  Locale: KOR (default)")
message(STATUS "  Anti-cheat: STUBBED (no _HACK_SHIELD_ / _NPROTECT_)")
message(STATUS "  Audio: STUBBED (no Miles SoundLib)")
message(STATUS "  Video: STUBBED (no wmstub/strmiids/vfw32)")
message(STATUS "  Renderer: Phase 5 DX11 backend (1:1 I4DyuchiGXRenderer)")
