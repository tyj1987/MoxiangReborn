# [Server]Map — 墨香 legacy `MapServer.exe` (the per-map game-world server)
#
# Phase 7.5: convert [Server]Map/MapServer.vcproj (VS 7.10 / 2003 format)
# into a CMakeLists.txt. Follows the same recipe shape as Phase 7.4a
# [Server]Distribute (commit 99c1559 + baaeeeb). The Map server is the
# heaviest of the three game servers (Distribute / Agent / Map) — ~70K
# LOC across 313 source files when you include every cross-directory
# file the legacy vcproj's <Files> block pulled in.
#
# Build properties extracted from MapServer.vcproj:
#   ConfigurationType    = 1 (Application / exe)
#   UseOfMFC             = 0 (no MFC)
#   CharacterSet         = 2 (Multibyte / MBCS)
#   PreprocessorDefinitions (Release):   WIN32;NDEBUG;_WINDOWS;_MAPSERVER_;__MAPSERVER__
#   PreprocessorDefinitions (Debug_<LOCALE>): + _FILE_BIN_ + locale macro
#   RuntimeLibrary       = 2 (Release, /MD) / 3 (Debug, /MDd) per vcproj,
#                           but we use /MT (Release) / /MTd (Debug) to stay
#                           ABI-compatible with YHLibrary.lib / 4DyuchiNET.dll
#                           (both built with /MT in Phases 7.1 / 7.2). A
#                           mixed /MD + /MT link line produces LNK4093 /
#                           LNK2005 on the CRT symbols — see
#                           PHASE7_MIGRATION_RECIPE.md §3.
#   PrecompiledHeader    = stdafx.h
#   SubSystem            = 2 (Windows GUI — no real GUI window, the entry
#                             point is WinMain in Server.cpp, mirrors
#                             Distribute's "console-less network service"
#                             pattern)
#   TargetMachine        = 1 (x86)
#   AdditionalIncludeDirectories (Release):
#       ..\[Lib]YHLibrary  .\
#       ..\[CC]ServerModule
#       ..\[CC]Ability  ..\[CC]BattleSystem  ..\[CC]Skill
#   AdditionalDependencies (Release):
#       odbc32.lib odbccp32.lib ..\[Lib]YHLibrary\YHLibrary.lib
#   AdditionalDependencies (Debug_<LOCALE>):
#       + winmm.lib (no MD5.lib / Wininet.lib for Map; those were Distribute
#                    specific for BuddyAuth + MHFile HTTP update)
#   Locale-flavored Debug configs: 5 (= Debug_Console/KOR, Debug_JAPAN,
#                                                  Debug_CHINA, Debug_HK, Debug_TL)
#       Debug_Console: _KOR_LOCAL_   (note: legacy vcproj had no _USINGTOOL_
#                                       for Map — that flag was Distribute-only)
#       Debug_JAPAN : _JAPAN_LOCAL_
#       Debug_CHINA : TAIWAN_LOCAL;_CHINA_LOCAL_
#       Debug_HK    : _HK_LOCAL_;_IGNORE_ASSERT_;_TW_LOCAL_
#       Debug_TL    : _TL_LOCAL_
#
# Phase 7.5 architecture decision:
#   The legacy vcproj's Release config has a fatal bug: [CC]Header/
#   CommonStruct.h:3465-3474 defines `MSG_CHANNEL_INFO` fields
#   `bBattleChannel[]`/`wMoveMapNum`/`dwChangeMapState` ONLY when
#   `_KOR_LOCAL_` is defined, but [Server]Map/ChannelSystem.cpp
#   lines 237/372/377/378/437 access those fields WITHOUT any
#   `#ifdef` guard. The legacy Release build was therefore always
#   broken — SWorking/MapServer.exe was actually built from the
#   Debug_Console (KOR) config. We match the legacy choice and ship
#   the Debug_Console target instead. See modern/docs/
#   phase7.5_map_migration.md for the full diagnostic + Bug C-30.
#
# External dependencies (binary, pre-built by earlier phases):
#   [Lib]YHLibrary/build_yhlibrary/Release/YHLibrary.lib
#       (122,724 bytes — built Phase 7.1; static library of HSEL +
#        memory pool + file I/O + crypto + STL helpers)
#   [Lib]YHLibrary/IndexGenerator.lib  (Phase 7.5 NEW)
#       (3,358 bytes — vendored; provides CIndexGenerator class)
#   4DyuchiNET_Latest/build_net/Release/4DyuchiNET.dll + .lib
#       (150,016 bytes DLL + 1,528-byte import lib — built Phase 7.2;
#        COM server providing CLSID_4DyuchiNET. Map's [CC]ServerModule/
#        Network.cpp:50 calls CoCreateInstance(CLSID_4DyuchiNET) — same
#        wiring as Distribute, no extra glue needed.)
#   4DyuchiGXGFunc/SS3DGFunc.dll + SS3DGFunc.lib  (Phase 7.5 NEW)
#       (122,880 bytes DLL + 48,678-byte import lib — vendored;
#        provides CalcDistance, ICCreate, ICRelease, ICInitialize
#        via naked x86 ASM. Map's [CC]Skill/SkillManager_server.cpp +
#        [CC]BattleSystem/BattleSystem_Server.cpp use these globals.)
#   4DyuchiGXGFunc/ (header-only — global.h, math.h; included by
#                    [CC]BattleSystem/BattleSystem_Server.h and
#                    [CC]Skill/skillmanager_server.h)
#
# Byte-level parity target:
#   SWorking/MapServer.exe = 2,555,904 bytes (legacy Debug_Console build).
#   This CMakeLists targets exactly that configuration (Debug_Console,
#   KOR locale) and the build script compares the output bytes.
#   Exact parity NOT expected — Distribute's parity was +10.8% (CRT
#   drift) and Map is much larger; record the delta.
cmake_minimum_required(VERSION 3.16)
project(MapServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Per-config options are attached at the target level (generator
    # expressions) so /O2 (Release) and /RTC1 (Debug) never collide.
    # /W3 /FS /MP are config-agnostic so they can stay global.
    # /source-charset:utf-8 (Phase 7.5b fix, root cause: mixed-encoding
    # source files). The legacy 墨香 [Server]Map source tree is a MIX
    # of three encodings — none of which match MSBuild's default
    # (system codepage 936 / GBK on the build host):
    #   - 174 files with cp949 (Korean EUC-KR / Wansung) — Korean
    #     comments + Korean string literals (e.g. Party.cpp:132
    #     "파티가 꽉 찾는데 Add하려 했습니다" and FieldBossMonsterManager.cpp:238
    #     "보스소멸" both in cp949 bytes \xba\xb8\xbd\xba\xbc\xd2\xb8\xea)
    #   - 3 files with cp936 (Chinese GBK) — comments with Chinese
    #     characters (Npc.cpp, StreetStall.h, TacticObject.h)
    #   - 1 file (ServerSystem.cpp) with cp1252 / extended ASCII
    # The 2f8b648 attempt at adding /source-charset:utf-8 to fix the
    # cascade only fixed the cases where source bytes happened to
    # decode as UTF-8; the cp949 bytes (Korean Hangul ranges
    # \x81-\xFE lead bytes) decode as INVALID in UTF-8, which MSVC's
    # tokenizer reports as C2001 (newline-in-constant) inside Korean
    # string literals. That cascade becomes C2143/C2144/C2059/C2601/
    # C1075 (siblings of the original C2447 cascade) and 67 total
    # errors at the head of the file stream.
    #
    # The actual fix: convert every non-UTF-8 source file in the build
    # to UTF-8 (lossless for cp949 → UTF-8 since all those bytes are
    # valid in extended Unicode), then /source-charset:utf-8 is the
    # ONLY consistent encoding for the whole TU. The transcoding is
    # done by modern/scripts/convert_map_sources_to_utf8.py (called
    # once during this phase; idempotent on re-run). 174 files were
    # transcoded cp949 → UTF-8 + 1 cp1252 → UTF-8 + 3 cp936 → UTF-8
    # in [Server]Map, plus 100+ cross-directory files in
    # [CC]Ability / [CC]BattleSystem / [CC]Header / [CC]Quest /
    # [CC]ServerModule / [CC]Skill / [CC]Suryun.
    #
    # Note: we keep /execution-charset at the default (system code
    # page / GBK) because the legacy code uses char* string literals
    # and MBCS APIs — flipping execution-charset to UTF-8 would
    # change the encoding of "..." string literals and break the MBCS
    # string handling. /source-charset:utf-8 alone is the surgical
    # fix. See modern/docs/phase7.5b_map_build_fix.md for the full
    # diagnostic.
    add_compile_options(/W3 /FS /MP /source-charset:utf-8)
endif()

# -----------------------------------------------------------------------------
# Source list
# -----------------------------------------------------------------------------
# Cross-directory paths. CMake's path arithmetic is fine here because
# none of these paths contain CJK characters (the [CC] bracket form is
# handled correctly by cmake_path / get_filename_component).
set(_repo_root          "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(_yh_root            "${_repo_root}/[Lib]YHLibrary")
set(_yh_lib             "${_yh_root}/build_yhlibrary/Release/YHLibrary.lib")
set(_cc_header          "${_repo_root}/[CC]Header")
set(_cc_ability         "${_repo_root}/[CC]Ability")
set(_cc_battlesystem    "${_repo_root}/[CC]BattleSystem")
set(_cc_quest           "${_repo_root}/[CC]Quest")
set(_cc_servermodule    "${_repo_root}/[CC]ServerModule")
set(_cc_skill           "${_repo_root}/[CC]Skill")
set(_cc_suryun          "${_repo_root}/[CC]Suryun")
set(_net_common         "${_repo_root}/4DyuchiNET_Common")
set(_net_root           "${_repo_root}/4DyuchiNET_Latest")
set(_net_lib_dir        "${_net_root}/build_net/Release")
set(_net_lib            "${_net_lib_dir}/4DyuchiNET.lib")
set(_net_dll            "${_net_lib_dir}/4DyuchiNET.dll")
set(_gxgfunc            "${_repo_root}/4DyuchiGXGFunc")
# Phase 7.5 fix: vendored [Lib]YHLibrary/IndexGenerator.lib (3358 B)
# provides the CIndexGenerator class implementation. The header
# [Lib]YHLibrary/IndexGenerator.h is in YHLibrary's include path, but
# the implementation lives in a separate static lib that we must link
# explicitly. (The Phase 7.1 YHLibrary build does NOT include
# IndexGenerator.cpp — it stays as a separate vendored lib.)
#
# The IndexGenerator.obj inside this .lib was compiled with an old
# MSVC that hard-coded `-defaultlib:LIBC` (single-threaded legacy CRT
# from MSVC 6.0). Modern MSVC rejects LIBC.lib as missing, so we use
# /NODEFAULTLIB:LIBC (passed via target_link_options() — see below) to
# drop the stale directive. The obj's actual runtime needs
# (memcpy/memset) come from our /MT (libcmt.lib) which is still pulled
# in via the standard msvcrt path.
set(_yh_index_lib       "${_yh_root}/IndexGenerator.lib")
set(_ss3dgfunc_lib      "${_gxgfunc}/SS3DGFunc.lib")

# -----------------------------------------------------------------------------
# Release target (locale-neutral; matches SWorking/MapServer.exe in
# spirit — see the Phase 7.5 architecture decision in the header
# comment for why we build Debug_Console by default)
# -----------------------------------------------------------------------------
add_executable(MapServer
    # [Server]Map/ — 175 cpp directly under the server dir
    AutoNoteManager.cpp
    AutoNoteRoom.cpp
    BobusangManager.cpp
    FieldBossMonster.cpp
    FieldBossMonsterManager.cpp
    FieldSubMonster.cpp
    FortWarManager.cpp
    FortWarWareSlot.cpp
    GameEventManager.cpp
    ItemLimitManager.cpp
    MapItemDrop.cpp
    Pet.cpp
    PetInvenSlot.cpp
    PetManager.cpp
    PetSpeechManager.cpp
    PetWearSlot.cpp
    SurvivalModeManager.cpp
    Titan.cpp
    TitanItemManager.cpp
    TitanManager.cpp
    TitanShopItemSlot.cpp
    TitanWearSlot.cpp
    AIGroupManager.cpp
    AIGroupPrototype.cpp
    AIManager.cpp
    AIParam.cpp
    AISystem.cpp
    AIUniqueGroup.cpp
    AttackCalc.cpp
    AttackManager.cpp
    AuctionContents.cpp
    BattleFactory_Default.cpp
    BossMonster.cpp
    BossMonsterInfo.cpp
    BossMonsterManager.cpp
    BossRewardsManager.cpp
    BossState.cpp
    CQuestBase.cpp
    ChangeItemMgr.cpp
    Channel.cpp
    ChannelSystem.cpp
    CharMove.cpp
    CharacterCalcManager.cpp
    CharacterManager.cpp
    CheckRoutine.cpp
    cJackpotManager.cpp
    cMonsterSpeechManager.cpp
    CommonNetworkMsgParser.cpp
    # [Server]Map/Condition/ — 13 cpp
    Condition/ACTION.cpp
    Condition/ACTION_exe.cpp
    Condition/ActionManager.cpp
    Condition/ConditionFactory.cpp
    Condition/ConditionManager.cpp
    Condition/ConditionNode.cpp
    Condition/ConditionObject.cpp
    Condition/ConditionObjectManager.cpp
    Condition/DATA.cpp
    Condition/OPDATA.cpp
    Condition/RESOURCE.cpp
    Condition/SITUATION.cpp
    Condition/SITUATION_exe.cpp
    DataBlock.cpp
    DataBlockManager.cpp
    DissolutionManager.cpp
    DistributeWay.cpp
    Distribute_Damage.cpp
    Distribute_Random.cpp
    Distributer.cpp
    Economy.cpp
    EventMapMgr.cpp
    ExchangeManager.cpp
    ExchangeRoom.cpp
    FameManager.cpp
    FixedTile.cpp
    FixedTileInfo.cpp
    FriendManager.cpp
    GeneralGridTable.cpp
    Grid.cpp
    GridSystem.cpp
    GridTable.cpp
    GridUnit.cpp
    GroupRegenInfo.cpp
    Guild.cpp
    GuildFieldWarMgr.cpp
    GuildManager.cpp
    GuildMark.cpp
    GuildTournamentMgr.cpp
    GuildUnion.cpp
    GuildUnionManager.cpp
    GuildWarehouse.cpp
    HelpRequestManager.cpp
    InventorySlot.cpp
    ItemContainer.cpp
    ItemDrop.cpp
    ItemManager.cpp
    ItemSlot.cpp
    JournalManager.cpp
    KyungGongInfo.cpp
    KyungGongManager.cpp
    LootingManager.cpp
    LootingRoom.cpp
    MapChange.cpp
    MapDBMsgParser.cpp
    MapNetworkMsgParser.cpp
    MapObject.cpp
    MemoryChecker.cpp
    MHError.cpp
    MHFile.cpp
    MHMap.cpp
    MHTimeManager.cpp
    Monster.cpp
    MsgRouter.cpp
    MugongManager.cpp
    MunpaWareSlot.cpp
    MurimGridTable.cpp
    Npc.cpp
    Object.cpp
    ObjectEvent.cpp
    ObjectFactory.cpp
    ObjectStateManager.cpp
    OptionManager.cpp
    PackedData.cpp
    Party.cpp
    PartyManager.cpp
    PartyWarMgr.cpp
    PathManager.cpp
    PatternNPC.cpp
    PatternObject.cpp
    PeaceWarModManager.cpp
    PKManager.cpp
    Player.cpp
    Purse.cpp
    PyoGukManager.cpp
    PyogukSlot.cpp
    Quest.cpp
    QuestGroup.cpp
    QuestManager.cpp
    QuestMapMgr.cpp
    QuestRegenMgr.cpp
    QuestUpdater.cpp
    QuickManager.cpp
    RarenessManager.cpp
    RecallManager.cpp
    RegenConditionInfo.cpp
    RegenManager.cpp
    RegenNPC.cpp
    RegenPrototype.cpp
    ReinforceManager.cpp
    Server.cpp
    ServerSystem.cpp
    ServerTable.cpp
    ShopInvenSlot.cpp
    ShopItemManager.cpp
    ShopItemSlot.cpp
    SiegeWarMgr.cpp
    SiegeWarProfitMgr.cpp
    SocietyActManager.cpp
    StateMachinen.cpp
    StateNPC.cpp
    StateParam.cpp
    StatisticsManager.cpp
    StatsCalcManager.cpp
    StdAfx.cpp
    StreetStall.cpp
    StreetStallManager.cpp
    STRPath.cpp
    SummonGroup.cpp
    SummonInfo.cpp
    SummonMonster.cpp
    SuryunRegen.cpp
    SuryunRegenGroup.cpp
    SuryunRegenGroupInfo.cpp
    SuryunRegenInfo.cpp
    SuryunRegenManager.cpp
    TacticManager.cpp
    TacticObject.cpp
    TacticStartInfo.cpp
    TaticAbilityInfo.cpp
    Tile.cpp
    TileGroup.cpp
    TileManager.cpp
    UngijosikManager.cpp
    UserTable.cpp
    VimuManager.cpp
    WantedInfo.cpp
    WantedManager.cpp
    WearSlot.cpp
    WeatherManager.cpp
    # [CC]ServerModule/ — 7 cpp (cross-directory; same 7 as Distribute)
    ${_cc_servermodule}/BootManager.cpp
    ${_cc_servermodule}/CommonDBMsgParser.cpp
    ${_cc_servermodule}/Console.cpp
    ${_cc_servermodule}/DataBase.cpp
    ${_cc_servermodule}/MiniDumper.cpp
    ${_cc_servermodule}/Network.cpp
    ${_cc_servermodule}/ServerListManager.cpp
    # [CC]Header/ — 7 cpp (CommonCalcFunc, CommonGameFunc,
    # GameResourceManager, vector + 3 TargetList).
    # GameResourceManager.cpp is INCLUDED here (vs. Distribute which
    # excluded it) because Server.cpp:31 references GAMERESRCMNGR
    # inside `#ifdef _MAPSERVER_`.
    ${_cc_header}/CommonCalcFunc.cpp
    ${_cc_header}/CommonGameFunc.cpp
    ${_cc_header}/GameResourceManager.cpp
    ${_cc_header}/TargetList/TargetData.cpp
    ${_cc_header}/TargetList/TargetList.cpp
    ${_cc_header}/TargetList/TargetListIterator.cpp
    ${_cc_header}/vector.cpp
    # [CC]Quest/ — 21 cpp (Map's quest system glue; QuestLimitKind_Time
    # is excluded — it is referenced only by other servers).
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
    # [CC]BattleSystem/ — 25 cpp (root + 6 sub-dirs). BattleSystem_Client.cpp
    # and Battle_GTournament.cpp include [Client]MH headers inside
    # `#ifndef _MAPSERVER_`, so they compile to empty TUs.
    ${_cc_battlesystem}/Battle.cpp
    ${_cc_battlesystem}/BattleFactory.cpp
    ${_cc_battlesystem}/BattleObject.cpp
    ${_cc_battlesystem}/BattleRect.cpp
    ${_cc_battlesystem}/BattleSystem_Client.cpp
    ${_cc_battlesystem}/BattleSystem_Server.cpp
    ${_cc_battlesystem}/BattleTeam.cpp
    ${_cc_battlesystem}/GTournament/BattleFactory_GTournament.cpp
    ${_cc_battlesystem}/GTournament/BattleTeam_GTournament.cpp
    ${_cc_battlesystem}/GTournament/Battle_GTournament.cpp
    ${_cc_battlesystem}/MunpaField/BattleFactory_MunpaField.cpp
    ${_cc_battlesystem}/MunpaField/BattleTeam_MunpaField.cpp
    ${_cc_battlesystem}/MunpaField/Battle_MunpaField.cpp
    ${_cc_battlesystem}/MurimField/battle_murimfield.cpp
    ${_cc_battlesystem}/MurimField/battlefactory_murimfield.cpp
    ${_cc_battlesystem}/MurimField/battleteam_murimfield.cpp
    ${_cc_battlesystem}/SiegeWar/BattleFactory_SiegeWar.cpp
    ${_cc_battlesystem}/SiegeWar/BattleTeam_SiegeWar.cpp
    ${_cc_battlesystem}/SiegeWar/Battle_SiegeWar.cpp
    ${_cc_battlesystem}/Suryun/BattleFactory_Suryun.cpp
    ${_cc_battlesystem}/Suryun/BattleTeam_Suryun.cpp
    ${_cc_battlesystem}/Suryun/Battle_Suryun.cpp
    ${_cc_battlesystem}/VimuStreet/BattleFactory_VimuStreet.cpp
    ${_cc_battlesystem}/VimuStreet/BattleTeam_VimuStreet.cpp
    ${_cc_battlesystem}/VimuStreet/Battle_VimuStreet.cpp
    # [CC]Ability/ — 14 cpp
    ${_cc_ability}/Ability.cpp
    ${_cc_ability}/AbilityCommonHeader.cpp
    ${_cc_ability}/AbilityGroup.cpp
    ${_cc_ability}/AbilityInfo.cpp
    ${_cc_ability}/AbilityManager.cpp
    ${_cc_ability}/AbilityUpdater.cpp
    ${_cc_ability}/AbilityUpdater_ApplyData.cpp
    ${_cc_ability}/AbilityUpdater_CheckLevelUp.cpp
    ${_cc_ability}/AbilityUpdater_Database.cpp
    ${_cc_ability}/AbilityUse.cpp
    ${_cc_ability}/AbilityUse_Job.cpp
    ${_cc_ability}/AbilityUse_KyungGong.cpp
    ${_cc_ability}/AbilityUse_Pet.cpp
    ${_cc_ability}/AbilityUse_Society.cpp
    # [CC]Suryun/ — 3 cpp (SuryunDefine + SuryunManager_client +
    # SuryunManager_server). client version is wrapped in
    # `#ifdef _MHCLIENT_` and produces an empty TU.
    ${_cc_suryun}/SuryunDefine.cpp
    ${_cc_suryun}/SuryunManager_client.cpp
    ${_cc_suryun}/SuryunManager_server.cpp
    # [CC]Skill/ — 48 cpp (full skill subsystem; client TUs are empty
    # under _MAPSERVER_).
    ${_cc_skill}/AmplifiedPower.cpp
    ${_cc_skill}/DelayGroup.cpp
    ${_cc_skill}/DetectState.cpp
    ${_cc_skill}/SkillArea.cpp
    ${_cc_skill}/SkillAreaData.cpp
    ${_cc_skill}/SkillAreaManager.cpp
    ${_cc_skill}/SkillManager_client.cpp
    ${_cc_skill}/SkillManager_server.cpp
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
)

target_include_directories(MapServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # [Server]Map/ FIRST so the ErrorMsg.h stub wins the
    # #include "ErrorMsg.h" lookup before [CC]Header/CommonDefine.h
    # pulls in the client-side header (Bug C-27 — same as Distribute).
    ${_cc_header}
    ${_cc_servermodule}
    ${_cc_ability}
    ${_cc_battlesystem}
    ${_cc_quest}
    ${_cc_skill}
    ${_cc_suryun}
    ${_yh_root}
    ${_net_common}
    ${_gxgfunc}
)

if(MSVC)
    target_compile_options(MapServer PRIVATE
        # /permissive- is NOT used (Bug C-28 + C-29 in KNOWN_BUGS.md):
        #   - CommonStruct.h:436 reuses `i` declared in a prior for-loop
        #     init (legacy MSVC7 extension, banned under /permissive-).
        #   - TargetListIterator.h:52 declares a qualified member
        #     function inside the class body (legacy MSVC7 extension,
        #     banned under /permissive- as error C4596).
        # Same workaround as DBThread (Bug C-14) and Distribute.
        #
        # /Zc:strictStrings- rejects legacy TCHAR-style calls in
        # [CC]ServerModule/DB.h etc. Mirror the Phase 7.1 BaseNetwork /
        # DBThread compromise.
        /Zc:strictStrings-
        # /Zc:forScope- restores the pre-VS2005 for-init variable
        # scope rule (the `int i` declared in `for(int i=...; ...)`
        # leaks into the enclosing function scope). Several legacy
        # sources in [CC]Header/ rely on this — e.g.
        # CommonStruct.h:430 declares `int i` in a for-init, then
        # re-uses `i` at line 436 inside a second for loop in the
        # same function. Tracked in docs/KNOWN_BUGS.md Bug C-28.
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
        # values within the tag).
        /wd4596 /wd3244 /wd3254
        # Belt-and-suspenders: force-include the LOG-undef header
        # so that .cpp files which include stdafx.h but happen to
        # pull in Windows headers via a different path still get
        # the fix. The /undef LOG inside the header itself is
        # tracked in docs/KNOWN_BUGS.md Bug D-1; this /FI is the
        # CMake-side companion.
        /FI ${CMAKE_CURRENT_SOURCE_DIR}/../../modern/scripts/force_undef_legacy_macros.h
        # Per-config runtime + optimization (MSVC rejects /O2 + /RTC1
        # in the same command line, so these MUST be mutually
        # exclusive).
        $<$<CONFIG:Debug>:/MTd /Od /RTC1>
        $<$<CONFIG:Release>:/MT /O2 /Ob2 /GF /Gy>
    )
endif()

# Preprocessor definitions (Release):
#   vcproj Release|Win32: WIN32;NDEBUG;_WINDOWS;_MAPSERVER_;__MAPSERVER__
#   vcproj Release had no _MBCS in PreprocessorDefinitions but
#   CharacterSet=2 was set on the Configuration, which MSBuild turns
#   into _MBCS. We set it explicitly here.
target_compile_definitions(MapServer PRIVATE
    WIN32
    _WINDOWS
    _MBCS
    _MAPSERVER_
    __MAPSERVER__
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Linker line (Release):
#   odbc32.lib odbccp32.lib ..\[Lib]YHLibrary\YHLibrary.lib
# Plus (Phase 7.4a fix: pass the lib DIRECTORIES to
# target_link_directories() and the lib BASENAMES to
# target_link_libraries()):
#   ws2_32          — <winsock2.h> in StdAfx.h
#   winmm           — Map-specific legacy dep (Debug-only in vcproj but
#                     Distribute's pattern of unconditional-link for
#                     Debug is too narrow; Release vcproj for Map does
#                     NOT list winmm.lib in AdditionalDependencies, so
#                     we link it Debug-only).
#   4DyuchiNET.lib  — COM import library (built Phase 7.2)
#   IndexGenerator.lib — vendored [Lib]YHLibrary/IndexGenerator.lib
#                     provides the CIndexGenerator class (the global
#                     CIndexGenerator g_IndexGenerator and the
#                     CIndexGenerator::ReleaseIndex member). 3358-byte
#                     vendored static lib from [Lib]YHLibrary/.
#   SS3DGFunc.lib   — vendored [4DyuchiGXGFunc/SS3DGFunc.lib]
#                     provides the CalcDistance/ICCreate/ICRelease/
#                     ICAllocIndex/ICFreeIndex/ICInitialize globals
#                     (definitions live in [4DyuchiGXGFunc/SS3DGFunc.dll]
#                     loaded at runtime). 48,678-byte vendored import
#                     lib.
#
# Map does NOT use MD5.lib or Wininet.lib — those were Distribute-only
# (BuddyAuth / MHFile HTTP update).
target_link_directories(MapServer PRIVATE
    "${_yh_root}/build_yhlibrary/Release"
    "${_yh_root}"  # for IndexGenerator.lib (vendored next to source)
    "${_net_lib_dir}"
    "${_gxgfunc}"  # for SS3DGFunc.lib (vendored next to DLL)
)
target_link_libraries(MapServer PRIVATE
    YHLibrary.lib
    IndexGenerator.lib
    4DyuchiNET.lib
    SS3DGFunc.lib
    ws2_32
    odbc32
    odbccp32
    $<$<CONFIG:Debug>:winmm>
)
# Phase 7.5 fix: drop the stale `-defaultlib:LIBC` directive baked
# into the vendored IndexGenerator.obj (see _yh_index_lib comment
# above for the rationale). target_link_options() is the right CMake
# primitive for raw linker flags — target_link_libraries() interprets
# a string starting with `/` as a library file path.
target_link_options(MapServer PRIVATE
    /NODEFAULTLIB:LIBC
)

# Output naming: legacy produced "MapServer.exe" with capital M.
# SubSystem = 2 (Windows GUI) per the legacy vcproj — the entry
# point is WinMain (Server.cpp), not main. Without WIN32 (Windows),
# the linker looks for `_main` and fails with LNK2019.
set_target_properties(MapServer PROPERTIES
    OUTPUT_NAME MapServer
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    WIN32_EXECUTABLE TRUE
)

# Phase 7.5 sanity: print a STATUS line so the build log records which
# 4DyuchiNET/YHLibrary binaries we linked against.
message(STATUS "Phase 7.5 MapServer configuration:")
message(STATUS "  YHLibrary.lib         = ${_yh_lib}")
message(STATUS "  IndexGenerator.lib    = ${_yh_index_lib}")
message(STATUS "  4DyuchiNET.lib        = ${_net_lib}")
message(STATUS "  4DyuchiNET.dll        = ${_net_dll}")
message(STATUS "  SS3DGFunc.lib         = ${_ss3dgfunc_lib}")
message(STATUS "  Sources: 313 cpp (188 [Server]Map + 14 Ability + 25 BattleSystem + 7 Header + 21 Quest + 7 ServerModule + 48 Skill + 3 Suryun)")

# -----------------------------------------------------------------------------
# Debug locale targets — one per legacy vcproj Debug_<LOCALE> config.
# -----------------------------------------------------------------------------
# Map's locale macros (per legacy vcproj):
#   Debug_Console (KOR): _KOR_LOCAL_              (note: NO _USINGTOOL_ —
#                                                   that flag was Distribute-only)
#   Debug_JAPAN        : _JAPAN_LOCAL_
#   Debug_CHINA        : TAIWAN_LOCAL;_CHINA_LOCAL_  (no _USINGTOOL_)
#   Debug_HK           : _HK_LOCAL_;_TW_LOCAL_;_IGNORE_ASSERT_
#   Debug_TL           : _TL_LOCAL_
# The legacy vcproj's Debug_<LOCALE> configs each linked
#   winmm.lib odbc32.lib odbccp32.lib ..\[Lib]YHLibrary\YHLibrary.lib
# We include those unconditionally for Debug to keep parity.
foreach(locale IN ITEMS KOR JAPAN CHINA HK TL)
    set(_defs "")
    set(_name "MapServer_Debug_${locale}")
    if(locale STREQUAL "KOR")
        set(_defs "_KOR_LOCAL_")
    elseif(locale STREQUAL "JAPAN")
        set(_defs "_JAPAN_LOCAL_")
    elseif(locale STREQUAL "CHINA")
        set(_defs "_CHINA_LOCAL_;TAIWAN_LOCAL")
    elseif(locale STREQUAL "HK")
        set(_defs "_HK_LOCAL_;_TW_LOCAL_;_IGNORE_ASSERT_")
    elseif(locale STREQUAL "TL")
        set(_defs "_TL_LOCAL_")
    endif()

    add_executable(${_name}
        AutoNoteManager.cpp
        AutoNoteRoom.cpp
        BobusangManager.cpp
        FieldBossMonster.cpp
        FieldBossMonsterManager.cpp
        FieldSubMonster.cpp
        FortWarManager.cpp
        FortWarWareSlot.cpp
        GameEventManager.cpp
        ItemLimitManager.cpp
        MapItemDrop.cpp
        Pet.cpp
        PetInvenSlot.cpp
        PetManager.cpp
        PetSpeechManager.cpp
        PetWearSlot.cpp
        SurvivalModeManager.cpp
        Titan.cpp
        TitanItemManager.cpp
        TitanManager.cpp
        TitanShopItemSlot.cpp
        TitanWearSlot.cpp
        AIGroupManager.cpp
        AIGroupPrototype.cpp
        AIManager.cpp
        AIParam.cpp
        AISystem.cpp
        AIUniqueGroup.cpp
        AttackCalc.cpp
        AttackManager.cpp
        AuctionContents.cpp
        BattleFactory_Default.cpp
        BossMonster.cpp
        BossMonsterInfo.cpp
        BossMonsterManager.cpp
        BossRewardsManager.cpp
        BossState.cpp
        CQuestBase.cpp
        ChangeItemMgr.cpp
        Channel.cpp
        ChannelSystem.cpp
        CharMove.cpp
        CharacterCalcManager.cpp
        CharacterManager.cpp
        CheckRoutine.cpp
        cJackpotManager.cpp
        cMonsterSpeechManager.cpp
        CommonNetworkMsgParser.cpp
        Condition/ACTION.cpp
        Condition/ACTION_exe.cpp
        Condition/ActionManager.cpp
        Condition/ConditionFactory.cpp
        Condition/ConditionManager.cpp
        Condition/ConditionNode.cpp
        Condition/ConditionObject.cpp
        Condition/ConditionObjectManager.cpp
        Condition/DATA.cpp
        Condition/OPDATA.cpp
        Condition/RESOURCE.cpp
        Condition/SITUATION.cpp
        Condition/SITUATION_exe.cpp
        DataBlock.cpp
        DataBlockManager.cpp
        DissolutionManager.cpp
        DistributeWay.cpp
        Distribute_Damage.cpp
        Distribute_Random.cpp
        Distributer.cpp
        Economy.cpp
        EventMapMgr.cpp
        ExchangeManager.cpp
        ExchangeRoom.cpp
        FameManager.cpp
        FixedTile.cpp
        FixedTileInfo.cpp
        FriendManager.cpp
        GeneralGridTable.cpp
        Grid.cpp
        GridSystem.cpp
        GridTable.cpp
        GridUnit.cpp
        GroupRegenInfo.cpp
        Guild.cpp
        GuildFieldWarMgr.cpp
        GuildManager.cpp
        GuildMark.cpp
        GuildTournamentMgr.cpp
        GuildUnion.cpp
        GuildUnionManager.cpp
        GuildWarehouse.cpp
        HelpRequestManager.cpp
        InventorySlot.cpp
        ItemContainer.cpp
        ItemDrop.cpp
        ItemManager.cpp
        ItemSlot.cpp
        JournalManager.cpp
        KyungGongInfo.cpp
        KyungGongManager.cpp
        LootingManager.cpp
        LootingRoom.cpp
        MapChange.cpp
        MapDBMsgParser.cpp
        MapNetworkMsgParser.cpp
        MapObject.cpp
        MemoryChecker.cpp
        MHError.cpp
        MHFile.cpp
        MHMap.cpp
        MHTimeManager.cpp
        Monster.cpp
        MsgRouter.cpp
        MugongManager.cpp
        MunpaWareSlot.cpp
        MurimGridTable.cpp
        Npc.cpp
        Object.cpp
        ObjectEvent.cpp
        ObjectFactory.cpp
        ObjectStateManager.cpp
        OptionManager.cpp
        PackedData.cpp
        Party.cpp
        PartyManager.cpp
        PartyWarMgr.cpp
        PathManager.cpp
        PatternNPC.cpp
        PatternObject.cpp
        PeaceWarModManager.cpp
        PKManager.cpp
        Player.cpp
        Purse.cpp
        PyoGukManager.cpp
        PyogukSlot.cpp
        Quest.cpp
        QuestGroup.cpp
        QuestManager.cpp
        QuestMapMgr.cpp
        QuestRegenMgr.cpp
        QuestUpdater.cpp
        QuickManager.cpp
        RarenessManager.cpp
        RecallManager.cpp
        RegenConditionInfo.cpp
        RegenManager.cpp
        RegenNPC.cpp
        RegenPrototype.cpp
        ReinforceManager.cpp
        Server.cpp
        ServerSystem.cpp
        ServerTable.cpp
        ShopInvenSlot.cpp
        ShopItemManager.cpp
        ShopItemSlot.cpp
        SiegeWarMgr.cpp
        SiegeWarProfitMgr.cpp
        SocietyActManager.cpp
        StateMachinen.cpp
        StateNPC.cpp
        StateParam.cpp
        StatisticsManager.cpp
        StatsCalcManager.cpp
        StdAfx.cpp
        StreetStall.cpp
        StreetStallManager.cpp
        STRPath.cpp
        SummonGroup.cpp
        SummonInfo.cpp
        SummonMonster.cpp
        SuryunRegen.cpp
        SuryunRegenGroup.cpp
        SuryunRegenGroupInfo.cpp
        SuryunRegenInfo.cpp
        SuryunRegenManager.cpp
        TacticManager.cpp
        TacticObject.cpp
        TacticStartInfo.cpp
        TaticAbilityInfo.cpp
        Tile.cpp
        TileGroup.cpp
        TileManager.cpp
        UngijosikManager.cpp
        UserTable.cpp
        VimuManager.cpp
        WantedInfo.cpp
        WantedManager.cpp
        WearSlot.cpp
        WeatherManager.cpp
        ${_cc_servermodule}/BootManager.cpp
        ${_cc_servermodule}/CommonDBMsgParser.cpp
        ${_cc_servermodule}/Console.cpp
        ${_cc_servermodule}/DataBase.cpp
        ${_cc_servermodule}/MiniDumper.cpp
        ${_cc_servermodule}/Network.cpp
        ${_cc_servermodule}/ServerListManager.cpp
        ${_cc_header}/CommonCalcFunc.cpp
        ${_cc_header}/CommonGameFunc.cpp
        ${_cc_header}/GameResourceManager.cpp
        ${_cc_header}/TargetList/TargetData.cpp
        ${_cc_header}/TargetList/TargetList.cpp
        ${_cc_header}/TargetList/TargetListIterator.cpp
        ${_cc_header}/vector.cpp
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
        ${_cc_battlesystem}/Battle.cpp
        ${_cc_battlesystem}/BattleFactory.cpp
        ${_cc_battlesystem}/BattleObject.cpp
        ${_cc_battlesystem}/BattleRect.cpp
        ${_cc_battlesystem}/BattleSystem_Client.cpp
        ${_cc_battlesystem}/BattleSystem_Server.cpp
        ${_cc_battlesystem}/BattleTeam.cpp
        ${_cc_battlesystem}/GTournament/BattleFactory_GTournament.cpp
        ${_cc_battlesystem}/GTournament/BattleTeam_GTournament.cpp
        ${_cc_battlesystem}/GTournament/Battle_GTournament.cpp
        ${_cc_battlesystem}/MunpaField/BattleFactory_MunpaField.cpp
        ${_cc_battlesystem}/MunpaField/BattleTeam_MunpaField.cpp
        ${_cc_battlesystem}/MunpaField/Battle_MunpaField.cpp
        ${_cc_battlesystem}/MurimField/battle_murimfield.cpp
        ${_cc_battlesystem}/MurimField/battlefactory_murimfield.cpp
        ${_cc_battlesystem}/MurimField/battleteam_murimfield.cpp
        ${_cc_battlesystem}/SiegeWar/BattleFactory_SiegeWar.cpp
        ${_cc_battlesystem}/SiegeWar/BattleTeam_SiegeWar.cpp
        ${_cc_battlesystem}/SiegeWar/Battle_SiegeWar.cpp
        ${_cc_battlesystem}/Suryun/BattleFactory_Suryun.cpp
        ${_cc_battlesystem}/Suryun/BattleTeam_Suryun.cpp
        ${_cc_battlesystem}/Suryun/Battle_Suryun.cpp
        ${_cc_battlesystem}/VimuStreet/BattleFactory_VimuStreet.cpp
        ${_cc_battlesystem}/VimuStreet/BattleTeam_VimuStreet.cpp
        ${_cc_battlesystem}/VimuStreet/Battle_VimuStreet.cpp
        ${_cc_ability}/Ability.cpp
        ${_cc_ability}/AbilityCommonHeader.cpp
        ${_cc_ability}/AbilityGroup.cpp
        ${_cc_ability}/AbilityInfo.cpp
        ${_cc_ability}/AbilityManager.cpp
        ${_cc_ability}/AbilityUpdater.cpp
        ${_cc_ability}/AbilityUpdater_ApplyData.cpp
        ${_cc_ability}/AbilityUpdater_CheckLevelUp.cpp
        ${_cc_ability}/AbilityUpdater_Database.cpp
        ${_cc_ability}/AbilityUse.cpp
        ${_cc_ability}/AbilityUse_Job.cpp
        ${_cc_ability}/AbilityUse_KyungGong.cpp
        ${_cc_ability}/AbilityUse_Pet.cpp
        ${_cc_ability}/AbilityUse_Society.cpp
        ${_cc_suryun}/SuryunDefine.cpp
        ${_cc_suryun}/SuryunManager_client.cpp
        ${_cc_suryun}/SuryunManager_server.cpp
        ${_cc_skill}/AmplifiedPower.cpp
        ${_cc_skill}/DelayGroup.cpp
        ${_cc_skill}/DetectState.cpp
        ${_cc_skill}/SkillArea.cpp
        ${_cc_skill}/SkillAreaData.cpp
        ${_cc_skill}/SkillAreaManager.cpp
        ${_cc_skill}/SkillManager_client.cpp
        ${_cc_skill}/SkillManager_server.cpp
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
    )
    target_include_directories(${_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${_cc_header}
        ${_cc_servermodule}
        ${_cc_ability}
        ${_cc_battlesystem}
        ${_cc_quest}
        ${_cc_skill}
        ${_cc_suryun}
        ${_yh_root}
        ${_net_common}
        ${_gxgfunc}
    )
    if(MSVC)
        # Same /permissive- drop as the Release target (Bug C-28 +
        # C-29). Same legacy compat flags (incl. D-1 LOG-undef /FI).
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
        _MAPSERVER_
        __MAPSERVER__
        _FILE_BIN_
        _DEBUG
        ${_defs}
    )
    target_link_directories(${_name} PRIVATE
        "${_yh_root}/build_yhlibrary/Release"
        "${_yh_root}"  # for IndexGenerator.lib (vendored next to source)
        "${_net_lib_dir}"
        "${_gxgfunc}"  # for SS3DGFunc.lib (same rationale as MapServer above)
    )
    target_link_libraries(${_name} PRIVATE
        YHLibrary.lib
        IndexGenerator.lib
        4DyuchiNET.lib
        SS3DGFunc.lib
        ws2_32
        odbc32
        odbccp32
        winmm
    )
    # Phase 7.5 fix: drop stale `-defaultlib:LIBC` directive (same
    # rationale as the MapServer target above).
    target_link_options(${_name} PRIVATE
        /NODEFAULTLIB:LIBC
    )
    set_target_properties(${_name} PROPERTIES
        OUTPUT_NAME "MapServer_${locale}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        WIN32_EXECUTABLE TRUE
    )
endforeach()
