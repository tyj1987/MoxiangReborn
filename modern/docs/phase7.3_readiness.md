# Phase 7.3 Server Migration Readiness

> One-page-per-server diagnostic. Captured at 2026-07-07 (Asia/Shanghai).
> Read-only audit — no legacy source files were modified.

## Summary (3-line)

- **[Server]Distribute**: RISK=**GREEN** — 5.0K LOC, plain x86/MBCS, no MFC, no third-party SDK, no IUnknown surface. Mirrors 4DyuchiNET_Latest recipe almost 1:1.
- **[Server]Agent**: RISK=**YELLOW** — 14K LOC, plain x86/MBCS, no MFC, but ships third-party gameguard SDKs (`ggsrv25`, `AntiCpSvrFunc`) that need vendoring decisions. Soft blocker.
- **[Server]Map**: RISK=**YELLOW** — 100K LOC / 175 cpp files, plain x86/MBCS, no MFC, no third-party SDK beyond what Distribute/Agent already pull in. No build blockers; testing surface is heavy because of the 54 message categories and AI/battle/skill game-logic footprint.

**Recommended migration order**: `[Server]Distribute` → `[Server]Agent` → `[Server]Map`
with reasoning (smallest-GREEN-first; Agent's ggsrv25 SDK is already vendored in repo as `ggsrv25.h` + `ggsrv25.lib` so the only extra work is declaring the include + lib in CMakeLists; Map's risk is *not* build-time but *test-time* — by that point Distribute+Agent will have proven the `[CC]ServerModule`+4DyuchiNET glue, so Map can focus on its own surface).

---

## [Server]Distribute

- **Path**: `墨香【源码】\[Server]Distribute`
- **vcproj summary**:
  - `ConfigurationType="1"` (exe)
  - `UseOfMFC="0"` (no MFC)
  - `CharacterSet="2"` (MBCS — keep as-is, do **not** pass `/utf-8`)
  - `RuntimeLibrary="4"` release / `"3"` debug (DLL runtime; legacy VC6 style)
  - `AdditionalDependencies`: `odbc32.lib odbccp32.lib [Lib]YHLibrary/YHLibrary.lib` (release), plus `MD5.lib Wininet.lib` (debug)
  - 6 configurations: `Release|Win32`, `Debug|Win32`, `Debug_JAPAN|Win32`, `Debug_CHINA|Win32`, `Debug_HK|Win32`, `Debug_TL|Win32` — i.e. one Debug per locale.
  - Locale flags (`PreprocessorDefinitions`): `_KOR_LOCAL_` (default debug), `_JAPAN_LOCAL_`, `_CHINA_LOCAL_`+`TAIWAN_LOCAL`, `_HK_LOCAL_`+`_TW_LOCAL_`, `_TL_LOCAL_`.
  - No `ModuleDefinitionFile` (it's an exe, no `.def`).
- **Source files**: **13 cpp / 14 h** (rough LOC: 4012 cpp + 943 h = **~4955 LOC total**)
  - `.cpp`: BuddyAuth, CommonNetworkMsgParser, Crypt, DistributeDBMsgParser, DistributeNetworkMsgParser, MHFile, MHTimeManager, Server, ServerSystem, ServerTable, StdAfx, UserManager, UserTable.
  - `.h`: same set + `MD5Checksum.h`, `MD5ChecksumDefines.h`.
  - Has `StdAfx.h` (legacy PCH pattern) but no `dllmain.cpp` (it's an exe, `Server.cpp` is the entrypoint).
  - No IUnknown-derived COM interfaces of its own (CoCreateInstance not called).
- **Blockers**: none. No MFC includes. The only `<ole2.h>` + `<initguid.h>` pair in `StdAfx.h` provides `CoInitialize`/`CoUninitialize` (called once in `ServerSystem.cpp:105`), which is part of plain Win32 SDK.
- **Dependencies**:
  - `[CC]ServerModule/` (BootManager, CommonDBMsgParser, Console, DataBase, iconsole, MiniDumper, Network, ServerListManager) — wired via the `Module` filter in the vcproj.
  - `[CC]Header/` (Protocol.h, CommonDefine, CommonGameDefine, ServerGameDefine, CommonGameStruct, CommonStruct, ServerGameStruct, CommonGameFunc) — included from `StdAfx.h`.
  - `[Lib]YHLibrary/` — via `YHLibrary.lib` in linker + `<yhlibrary.h>` in StdAfx.
  - `MD5.lib` (vendored in the server directory itself; not in `[Lib]`).
  - **No** `[Lib]BaseNetwork` (0 direct includes — confirmed by `findstr`).
  - **No** `4DyuchiNET_Common` direct includes (0 hits for `INetwork.h`) — all network access goes through `g_Network` from `[CC]ServerModule` (56 `g_Network` references in cpp).
- **Network protocol surface**:
  - **61** `MP_xxx_xxx_` references in `.cpp` files; **5** unique categories.
  - Categories: `USERCONN`, `MORNITORMAPSERVER`, `MORNITORTOOL`, `SERVER`, `AUTOPATCH`. I.e. this is the login dispatcher: client logins, server registration/heartbeat, and a Mornitor (GM monitoring) channel.
- **Game logic footprint**: none beyond login dispatch + server table management. No `Player.cpp`, no `Battle.cpp`, no `AISystem.cpp`. Pure session-orchestration server.
- **MFC/ATL blockers**: none — `UseOfMFC="0"` and no `<afx*>` includes anywhere. The `//{{AFX_INSERT_LOCATION}}` template comment + `AFX_STDAFX_H__` guard name are residue from the Visual C++ wizard but harmless — they never reference an actual MFC header.
- **Estimated effort**:
  - Producer-LoC: ~5K LOC across 13 files. ~250-400 lines of CMakeLists (vcproj has 1.8K lines mostly FileConfiguration repetition; the actual signal is the per-file list).
  - Hours: **3-5 hours** including configure+build+smoke link test (matches the 4DyuchiNET_Latest 12-iteration pattern).
  - Parallelism: trivially serial; only 13 source files.
- **Migration recipe delta vs Phase 7.2 NET**:
  - Almost identical to 4DyuchiNET_Latest. Just point `target_sources()` at the 13 `.cpp` files and pull in the `[CC]ServerModule` glob + `[CC]Header` include dir + `[Lib]YHLibrary`.
  - Drop `winmm.lib` (Agent/Map use it, Distribute does not).
  - Link `odbc32.lib odbccp32.lib` (all three need it).
  - Drop `MD5.lib` `Wininet.lib` from the debug-only extra list (Distribute uses them, but they're trivially added; `MD5.lib` ships next to the source).
  - **Locale macros** must be set per-config: 5 debug configurations (`_KOR_LOCAL_`, `_JAPAN_LOCAL_`, `_CHINA_LOCAL_`+`TAIWAN_LOCAL`, `_HK_LOCAL_`+`_TW_LOCAL_`, `_TL_LOCAL_`). Use `target_compile_definitions(distribute_server_debug_kor PRIVATE _KOR_LOCAL_ _DEBUG _USINGTOOL_ _DISTRIBUTESERVER_)` × 5. Or pick one locale (`_KOR_LOCAL_`) as the canonical build and document the rest as `MULTI_LOCALE` semantics.
  - No `target_precompile_headers` needed — drop the legacy PCH pattern (HSEL POC went this way).
- **Risk**: **GREEN**
- **Suggested first-batch commit**:
  1. `modern/docs/phase7.3_readiness.md` (this file).
  2. `[Server]Distribute/CMakeLists.txt` (one target, 13 sources, ~200 lines).
  3. `[Server]Distribute/StdAfx.h` swap: MFC residue → plain Win32 (per Phase 7 §3 — add `<crtdbg.h>` + `_ASSERTE` fallback, ensure `<winsock2.h>` before `<objbase.h>`).
  4. `modern/scripts/build_distribute.py` (mirror of `build_net.py`).
  5. `modern/scripts/distribute_smoke_test.cpp` (link against `[CC]ServerModule` + 4DyuchiNET, instantiate `CNetwork` and `g_Console`, run the boot manager for 1s).

---

## [Server]Agent

- **Path**: `墨香【源码】\[Server]Agent`
- **vcproj summary**:
  - `ConfigurationType="1"` (exe)
  - `UseOfMFC="0"`
  - `CharacterSet="2"` (MBCS)
  - `RuntimeLibrary="2"` release / `"3"` debug (legacy VC6 `2` = static-link release, `3` = static-link debug — i.e. NOT the `4/5` DLL pattern Distribute used; flag this in the CMake recipe).
  - `AdditionalDependencies`: `odbc32.lib odbccp32.lib YHLibrary.lib` (everywhere); Debug_HK also links `ggsrv25.lib`.
  - 6 configurations matching Distribute. Debug_HK adds `_NPROTECT_`.
  - Locale flags: same matrix as Distribute, plus `_NPROTECT_` on `_HK_LOCAL_` debug.
- **Source files**: **24 cpp / 25 h** (rough LOC: 12 085 cpp + 1 815 h = **~13 900 LOC total**)
  - `.cpp`: AgentDBMsgParser, AgentNetworkMsgParser, BobusangManager_Agent, CommonNetworkMsgParser, Crypt, filteringtable, GMToolManager, GMPowerList, HackShieldManager, JackpotManager_Agent, MHFile, MHTimeManager, MsgTable, NProtectManager, PlustimeMgr, PunishManager, Server, ServerSystem, ServerTable, ShoutManager, SkillDalayManager, StdAfx, TrafficLog, UserTable.
  - `.h`: same set + `AntiCpSvrFunc.h`, `ggsrv25.h`.
  - Has `StdAfx.h` (legacy PCH), no `dllmain.cpp` (Server.cpp = entrypoint).
  - No IUnknown-derived COM.
- **Blockers**: **soft** — third-party gameguard SDKs ship in the source tree already (good), but the CMakeLists has to know about them:
  - `ggsrv25.h` + `ggsrv25.lib` — nProtect GameGuard 2.5 SDK header/lib. Already vendored next to Agent source.
  - `AntiCpSvrFunc.h` — anti-cheat server-side functions (header-only, no .lib).
  - The actual wrappers (`HackShieldManager.cpp`, `NProtectManager.cpp`) just call SDK functions; they should compile cleanly once the SDK headers are on the include path and `ggsrv25.lib` is linked. Note the SDK is conditional (`_HK_LOCAL_` only) — so it's only used in the Debug_HK build.
- **Dependencies**:
  - `[CC]ServerModule/` — same as Distribute (BootManager, Console, DataBase, MiniDumper, Network, ServerListManager).
  - `[CC]Header/` — same.
  - `[Lib]YHLibrary/` — same.
  - **No** `[Lib]BaseNetwork`, **no** `4DyuchiNET_Common` direct includes.
- **Network protocol surface**:
  - **540** `MP_xxx_xxx_` references; **27** unique categories.
  - Categories: AUTONOTE, BOBUSANG, CHAT, CHEAT, FORTWAR, FRIEND, GTOURNAMENT, GUILD, GUILDMARK, HACKCHECK, HACKSHIELD, ITEM, ITEMLIMIT, JACKPOT, MORNITORMAPSERVER, MORNITORTOOL, MURIMNET, NOTE, NPROTECT, OPTION, PARTY, SERVER, SIEGEWAR, SKILL, SURVIVAL, USERCONN, WANTED. I.e. Agent is a relay: it forwards most categories between Map and Distribute/Client, with its own logic on chat (CHEAT/MsgTable filtering), guild/party/fortwar/siegewar cross-server coordination, GMTool admin channel, and the HK-locale gameguard anti-cheat callbacks.
  - 405 `g_Network` references in `.cpp` files (vs Distribute's 56 and Map's 170) — Agent is the heaviest direct user of the CNetwork wrapper because of its relay role.
- **Game logic footprint**: thin. Mostly *coordination*: GMToolManager, GMPowerList, PlustimeMgr (boost events), ShoutManager (cross-server broadcast), SkillDalayManager (cooldown ledger), JackpotManager_Agent, BobusangManager_Agent, PunishManager, TrafficLog, MsgTable (chat filter). No `Player.cpp`, no `Battle`, no `AISystem`.
- **MFC/ATL blockers**: none. Same MBCS/ole2/initguid StdAfx pattern as Distribute. `CoInitialize(NULL)` once in `ServerSystem.cpp:94`.
- **Estimated effort**:
  - Producer-LoC: ~14K LOC across 24 files. ~400-600 lines of CMakeLists.
  - Hours: **6-10 hours** (mainly because of the HK-locale SDK conditional + the larger protocol surface to model in the smoke test).
  - Parallelism: 24 files — still trivially serial; no benefit from parallel coders.
- **Migration recipe delta vs Phase 7.2 NET**:
  - Identical structure to Distribute (same wrapper, same `[CC]Header`).
  - `RuntimeLibrary="2"` (static release) vs Distribute's `RuntimeLibrary="4"` (DLL release). CMake `target_compile_options(/MT)` for release, `/MTd` for debug. **Do not** pick `/MD` for Agent — the legacy vcproj explicitly chose static CRT.
  - Add `target_link_libraries(agent_server_debug_hk PRIVATE ggsrv25.lib)` conditional on the HK locale.
  - Add `target_include_directories(agent_server PRIVATE 墨香【源码】\[Server]Agent)` so `ggsrv25.h` + `AntiCpSvrFunc.h` resolve from the agent dir itself.
  - `MSGPROTOCOL::MAX` table in `Protocol.h` will need to be reflected in the smoke test for the 27 categories — bigger fixture than Distribute's.
- **Risk**: **YELLOW** (vendoring decision for nProtect GameGuard SDK is the only soft blocker; if `ggsrv25.lib` ships without symbols or only as a stub for non-HK locales, the link step is still doable but the HK build target becomes a no-op smoke test).
- **Suggested first-batch commit**:
  1. `[Server]Agent/CMakeLists.txt` (1 target, 24 sources, locale-conditional on HK).
  2. `[Server]Agent/StdAfx.h` MFC residue strip (same shape as Distribute).
  3. `modern/scripts/build_agent.py` (mirror of build_net.py; default locale = `_KOR_LOCAL_`).
  4. `modern/scripts/agent_smoke_test.cpp` (verify `g_Network`, `g_Console`, `g_pServerTable` link).
  5. `docs/KNOWN_BUGS.md` entry for the HK-locale ggsrv25 vendoring status.

---

## [Server]Map

- **Path**: `墨香【源码】\[Server]Map`
- **vcproj summary**:
  - `ConfigurationType="1"` (exe)
  - `UseOfMFC="0"`
  - `CharacterSet="2"` (MBCS)
  - `RuntimeLibrary="2"` release / `"3"` debug (static CRT, same as Agent).
  - `EntryPointSymbol="WinMainCRTStartup"` (explicit — Distribute/Agent don't set this).
  - `AdditionalDependencies`: `winmm.lib odbc32.lib odbccp32.lib YHLibrary.lib` (release has only odbc+YHLibrary, debugs add `winmm.lib`).
  - 6 configurations: `Debug_JAPAN`, `Debug_Console` (=`_KOR_LOCAL_`), `Debug_CHINA`, `Release`, `Debug_HK`, `Debug_TL`. Debug_HK sets `ExceptionHandling="TRUE"` (only place in the three servers).
  - **Extra include dirs** that Distribute/Agent do NOT have: `[CC]Ability`, `[CC]BattleSystem`, `[CC]Skill` (these are referenced via the `Module` filter but as include directories).
- **Source files**: **175 cpp / 175 h** (rough LOC: 88 342 cpp + 12 149 h = **~100 491 LOC total**)
  - Core game-logic files present: `Player.cpp`/`Player.h`, `Monster.cpp`, `Npc.cpp`, `Guild.cpp`, `Party.cpp`, `MapObject.cpp`, `ChannelSystem.cpp`, `MapChange.cpp`, `MHMap.cpp`, `MapNetworkMsgParser.cpp`, `MapDBMsgParser.cpp`, `Server.cpp`, `ServerSystem.cpp`, `MsgRouter.cpp`, `MemoryChecker.cpp`, `MHError.cpp`, `OptionManager.cpp`, `MHFile.cpp`, `MHTimeManager.cpp`, `UserTable.cpp`, `ServerTable.cpp`, `StdAfx.cpp`/`StdAfx.h`.
  - AI subsystem: `AISystem.cpp` + `AIManager.cpp` + `AIGroupManager.cpp` + `AIGroupPrototype.cpp` + `AIParam.cpp` + `AIUniqueGroup.cpp` + `AIDefine.h`.
  - Battle: `BattleFactory_Default.cpp` + `AttackManager.cpp` + `AttackCalc.cpp` + `Distribute_Damage.cpp` + `Distribute_Random.cpp`.
  - Item/Skill/Economy: `ItemManager.cpp` + `MugongManager.cpp` + `ShopItemManager.cpp` + `ReinforceManager.cpp` + `ChangeItemMgr.cpp` + `Economy.cpp` + `RarenessManager.cpp` + `KyungGongManager.cpp` + `QuickManager.cpp` + `Titan*` (whole Titan subsystem).
  - Social/war: `GuildManager.cpp` + `GuildUnion*` + `GuildFieldWarMgr` + `GuildTournamentMgr` + `PartyWarMgr` + `SiegeWarMgr` + `FortWarManager` + `FortWarWareSlot` + `PeaceWarModManager`.
  - Quest: `Quest.cpp` + `QuestManager.cpp` + `QuestGroup.cpp` + `QuestMapMgr` + `QuestRegenMgr` + `QuestUpdater` + `CQuestBase` + `Condition/` subdir.
  - Other: `WeatherManager`, `SurvivalModeManager`, `VimuManager`, `WantedManager`, `HelpRequestManager`, `AutoNoteManager`, `AutoNoteRoom`, `AuctionContents`, `LootingManager`, `MapItemDrop`, `Purse`, `TacticManager` + `TacticObject` + `TacticStartInfo` + `TaticAbilityInfo`, `STRPath`, `Summon*`, `SuryunRegen*`, `SocietyActManager`, `JournalManager`, `DissolutionManager`, `ExchangeManager`+`ExchangeRoom`, `StreetStallManager`+`StreetStall`, `cJackpotManager`, `cMonsterSpeechManager`, `Pet*` (PetManager/Pet/PetInvenSlot/PetWearSlot/PetSpeechManager), `CharacterCalcManager`, `StatsCalcManager`, `StateMachinen`/`StateNPC`/`StateParam`, `Object*` (Object/ObjectEvent/ObjectFactory/ObjectStateManager/PatternNPC/PatternObject), `BossMonster` + `BossMonsterManager` + `BossMonsterInfo` + `BossRewardsManager` + `BossState` + `FieldBossMonster` + `FieldBossMonsterManager` + `FieldSubMonster`, `FameManager`, `EventMapMgr`, `GroupRegenInfo`, `MemoryChecker`, `MHError`, `Grid*` (Grid/GridSystem/GridTable/GridUnit/GeneralGridTable/MurimGridTable), `FixedTile`/`FixedTileInfo`/`MHMap`/`Tile`/`TileGroup`/`TileManager`, `Channel`/`ChannelSystem`, `DataBlock`/`DataBlockManager`/`PackedData`, `MapObject`, `Distributer`/`DistributeWay`.
  - Has a `Condition/` subdir — *not* the directory itself listed as a file; contents need to be enumerated before listing in CMake.
  - No `dllmain.cpp` (Server.cpp = entrypoint, `WinMainCRTStartup`).
  - No IUnknown-derived COM.
- **Blockers**: **soft** — `[CC]Ability/`, `[CC]BattleSystem/`, `[CC]Skill/` add three more include directories on top of the standard `[CC]Header/` + `[CC]ServerModule/`. These three modules themselves are not in the migration target list yet, so their headers are read *but not built* — i.e. Map's build needs their `.h` files on the path, but doesn't need to recompile their `.cpp` files. That's fine if the directories are intact.
- **Dependencies**:
  - `[CC]ServerModule/` (same).
  - `[CC]Header/` (same).
  - `[CC]Ability/`, `[CC]BattleSystem/`, `[CC]Skill/` (Map-exclusive).
  - `[Lib]YHLibrary/` (same).
  - **No** `[Lib]BaseNetwork`, **no** `4DyuchiNET_Common` direct includes.
- **Network protocol surface**:
  - **1966** `MP_xxx_xxx_` references; **54** unique categories — by far the largest.
  - Categories: AUTONOTE, BATTLE, BOBUSANG, BOSS, CHAR, CHAT, CHEAT, CHEATE, EXCHANGE, FIELD, FORTWAR, FORTWARINFO, FRIEND, GM, GTOURNAMENT, GUILD, GUILDMARK, HACKCHECK, ITEM, ITEMEXT, ITEMLIMIT, JACKPOT, JOURNAL, KYUNGGONG, MAPITEM, MORNITORMAPSERVER, MOVE, MUGONG, MUNPA, MURIMNET, NOTE, NPC, OPTION, PARTY, PARTYWAR, PET, PETINVEN, PK, PYOGUK, QUEST, QUESTEVENT, QUICK, SERVER, SHOPITEM, SIEGEWAR, SOCIETYACT, STREETSTALL, SUBQUEST, SURVIVAL, SURYUN, TACTIC, TITAN, USERCONN, WANTED. I.e. **every** game-system category lives here.
  - 170 `g_Network` references in `.cpp` files (less than Agent's 405 — Map uses `MsgRouter.cpp` and per-module parsers that hide the CNetwork wrapper from individual game-logic files; that's good design).
- **Game logic footprint**: **100%** of the in-game simulation. Player state, monster AI, NPC dialogue, item/mugong/quest/guild/party/PK/economy all live here. This is the file set that gets unit-tested once the build passes.
- **MFC/ATL blockers**: none. Same MBCS/ole2/initguid StdAfx pattern as the others. `CoInitialize(NULL)` once in `ServerSystem.cpp:177`. Debug_HK turns on `ExceptionHandling="TRUE"` — note this in the CMake config (it's the only config with that flag in the three servers).
- **Estimated effort**:
  - Producer-LoC: ~100K LOC across 175 files. ~600-1000 lines of CMakeLists (mostly the file list).
  - Hours: **15-25 hours** for configure+build+smoke link. The 12-iteration cycle of 4DyuchiNET_Latest took ~30 min each — multiplied by 175 files, expect a couple of header tweaks.
  - Parallelism: 175 files split across 3 coders is **the** opportunity. Bundle by message category (e.g. one worker handles `Battle*` + `Attack*` + `Distribute_Damage/Random`, another handles `Guild*` + `Party*` + `SiegeWar*` + `FortWar*`, third handles `Item*` + `Shop*` + `Titan*` + `Economy*`). But CMake file-listing is the gating step, so coordinate the master list first.
- **Migration recipe delta vs Phase 7.2 NET**:
  - Same wrapper pattern, but `target_sources()` enumerates 175 `.cpp` files.
  - Add 3 extra `target_include_directories(... [CC]Ability [CC]BattleSystem [CC]Skill)`.
  - Add `winmm.lib` to debug builds.
  - Set `EntryPointSymbol WinMainCRTStartup` via `set_target_properties(map_server WIN32_EXECUTABLE FALSE)` (it's a console subsystem but explicitly named entrypoint).
  - Debug_HK target needs `ExceptionHandling` semantics: `/EHa` instead of the default `/EHsc`. Use `set_target_properties(map_server_debug_hk PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreadedDebugDLLExceptionHandling)` or pass `/EHa` via `COMPILE_OPTIONS`.
- **Risk**: **YELLOW** (no build blocker, but 100K LOC means the first successful link is *not* a feature gate — the *behavioral* gate is "does the binary still match the legacy SWorking/`MapServer.exe` byte-for-byte?" which requires a separate behavior-fixture task that comes after this).
- **Suggested first-batch commit**:
  1. `[Server]Map/CMakeLists.txt` (1 target, 175 sources, 6 configs).
  2. `[Server]Map/StdAfx.h` MFC residue strip.
  3. `modern/scripts/build_map.py` (mirror of build_net.py; defaults to `_KOR_LOCAL_` = `Debug_Console`).
  4. `modern/scripts/map_smoke_test.cpp` (verify the three new include dirs, exercise `CNetwork` + `CUserTable` + a stub `CObjectFactory`).
  5. Defer behavior-fixture to Phase 7.4 — Map's protocol surface is too big for a single smoke test.

---

## Cross-cutting observations

- **Shared StdAfx pattern**: all three servers use the *same* legacy `StdAfx.h` template (53 lines, 4-line MACRO guard, MFC residue comment block, identical `#include` order). One patch — `#ifndef _WINSOCKAPI_` guard + `<crtdbg.h>` + `_ASSERTE` fallback + `<objbase.h>` removal — works for all three. Factor this into `modern/legacy_stdafx_patch.h` and `#include` it from each server's `StdAfx.h`.
- **Shared `[CC]ServerModule` glue**: all three servers compile `BootManager.cpp`, `CommonDBMsgParser.cpp`, `Console.cpp`, `DataBase.cpp`, `MiniDumper.cpp`, `Network.cpp`, `ServerListManager.cpp` directly into the exe (visible in each vcproj's `Module` filter). That means the `[CC]ServerModule/` headers are *not* a separate target — they're built inline. For CMake, treat `[CC]ServerModule` as a header-only `INTERFACE` library (`add_library(cc_server_module INTERFACE)`) and let each server's target compile the `.cpp` files itself. This matches the legacy layout and avoids a circular dependency between the server exes and a `cc_server_module` static lib.
- **ODBC linkage**: all three link `odbc32.lib odbccp32.lib`. Distribute+Agent+Map all touch DB. This is a `[Lib]DBThread` **adjacent** dependency — they don't link `DBThread.lib` directly, but they use the same SQL pattern via `[CC]ServerModule/DataBase.cpp` (which is the ODBC wrapper). No blocker.
- **Locale matrix**: all three have the same 5-debug-locale matrix (`_KOR_LOCAL_`, `_JAPAN_LOCAL_`, `_CHINA_LOCAL_`+`TAIWAN_LOCAL`, `_HK_LOCAL_`+`_TW_LOCAL_`, `_TL_LOCAL_`) + 1 release. CMake should pick one (`_KOR_LOCAL_`) as the canonical build and emit the rest as `target_compile_definitions` per-config so they remain buildable on demand.
- **COM initialization**: all three call `CoInitialize(NULL)` once at boot (Distribute:ServerSystem.cpp:105, Agent:94, Map:177). This is plain Win32 COM, **not** MFC. No blocker; but if a future build flags `_WIN32_WINNT >= 0x0600` to drop XP support, `CoInitializeEx` is preferred.
- **MD5**: only Distribute uses `MD5.lib` (vendored in the Distribute dir). Agent/Map don't need it.
- **GameGuard (`ggsrv25`)**: only Agent + Agent's `Debug_HK` config. Vendored as `ggsrv25.h` + `ggsrv25.lib`. Soft blocker — see Agent section.
- **Hardest thing about migrating all three together**: the **shared `StdAfx.h` template**. Because all three servers `#include` the same 14-line block of `[CC]Header/*.h` files *and* a `ServerSystem.h` from a sibling header that's already in the source tree, the legacy build effectively re-uses the same PCH across all three. Modern CMake with `target_precompile_headers` per-target will work, but the master "what goes into the PCH" decision has to be made *once*, not three times. Recommend: write the patched `modern/legacy_stdafx_patch.h` once and `#include` it from each server's own `StdAfx.h`.
- **Second hardest thing**: the **protocol surface test harness**. Distribute=5 categories, Agent=27 categories, Map=54 categories. A smoke test that verifies "the binary links and can boot" needs different fixtures for each. Don't try to write one fixture that works for all three — write three, escalating in coverage.
- **What stays shared across all three**: the `[CC]ServerModule` + `[CC]Header` glue. Any fix that lands in those two directories benefits all three servers simultaneously. So Phase 7.3's strategic bet is: migrate `[CC]ServerModule` and `[CC]Header` into a header-only INTERFACE library *first* (alongside Distribute's CMakeLists), and let Agent/Map inherit it for free.

---

## Appendix — raw audit numbers

| Metric                                      | Distribute | Agent  | Map     |
|---------------------------------------------|-----------:|-------:|--------:|
| `.cpp` files                                 | 13         | 24     | 175     |
| `.h` files                                   | 14         | 25     | 175     |
| cpp LOC                                      | 4 012      | 12 085 | 88 342  |
| h LOC                                        | 943        | 1 815  | 12 149  |
| Total LOC                                    | 4 955      | 13 900 | 100 491 |
| `MP_xxx_xxx_` references in .cpp             | 61         | 540    | 1 966   |
| Unique MP categories                         | 5          | 27     | 54      |
| `g_Network` references in .cpp               | 56         | 405    | 170     |
| `Network.h` references                       | 5          | 13     | 28      |
| `INetwork.h` direct includes                 | 0          | 0*     | 0       |
| `4DyuchiNET` references                      | 0          | 0      | 0       |
| `BaseNetwork.h` direct includes              | 0          | 0      | 0       |
| MFC `<afx*>` includes                         | 0          | 0      | 0       |
| MFC `UseOfMFC`                                | 0          | 0      | 0       |
| IUnknown-derived classes (own code)           | 0          | 0      | 0       |
| `CoInitialize` calls                         | 1          | 1      | 1       |
| `CoCreateInstance` calls                     | 0          | 0      | 0       |
| `DllGetClassObject` / class-factory register  | 0          | 0      | 0       |
| Vendor SDKs in source tree                   | MD5        | ggsrv25, AntiCpSvrFunc | (none)  |

\* Agent has 1 hit for "INetwork" in `ServerSystem.cpp` but it's a string literal in a log message, not an include or symbol reference.

## Appendix — shared headers touched by all three servers

These `[CC]Header/*.h` files are pulled into all three servers' `StdAfx.h`:

`vector.h`, `protocol.h`, `CommonDefine.h`, `CommonGameDefine.h`, `ServerGameDefine.h`, `CommonGameStruct.h`, `CommonStruct.h`, `ServerGameStruct.h`, `CommonGameFunc.h`.

And the `[CC]ServerModule/*.h` pulled in by all three:

`DataBase.h`, `Console.h`, `define.h`, `iconsole.h`, `network_guid.h`, `Network.h`, `Noncopyable.h`, `typedef.h`, `DBThreadInterface.h`.

These two lists form the "minimum viable shared kernel" that all three CMakeLists need to depend on.