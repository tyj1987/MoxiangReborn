# modern/ 代码量统计

> 维护规则：每次 Phase 收官时跑一次 snapshot，记到下方表格。
> 不要 commit 大文件 diff（git status -s 应该永远 0 行变化），本文件
> 是 metrics 报告，不是 source of truth。

## 统计命令

```powershell
$modern = "<workspace>\modern"
$src = (Get-ChildItem -LiteralPath "$modern\src" -Recurse -File -Include "*.cpp","*.c" -EA SilentlyContinue | Measure-Object).Count
$hdr = (Get-ChildItem -LiteralPath "$modern\include" -Recurse -File -Include "*.hpp","*.h" -EA SilentlyContinue | Measure-Object).Count
$tests = (Get-ChildItem -LiteralPath "$modern\tests" -Recurse -File -Include "*.cpp","*.c" -EA SilentlyContinue | Measure-Object).Count
$srcLines = (Get-ChildItem -LiteralPath "$modern\src" -Recurse -File -Include "*.cpp","*.c" -EA SilentlyContinue | Get-Content -EA SilentlyContinue | Measure-Object -Line).Lines
$hdrLines = (Get-ChildItem -LiteralPath "$modern\include" -Recurse -File -Include "*.hpp","*.h" -EA SilentlyContinue | Get-Content -EA SilentlyContinue | Measure-Object -Line).Lines
$testLines = (Get-ChildItem -LiteralPath "$modern\tests" -Recurse -File -Include "*.cpp","*.c" -EA SilentlyContinue | Get-Content -EA SilentlyContinue | Measure-Object -Line).Lines
[PSCustomObject]@{
  Src = $src; Hdr = $hdr; Tests = $tests;
  SrcLines = $srcLines; HdrLines = $hdrLines; TestLines = $testLines;
  Total = $src+$hdr+$tests; TotalLines = $srcLines+$hdrLines+$testLines
} | Format-List
```

## Snapshot 趋势

| 日期 | Phase | src | hdr | tests | src 行 | hdr 行 | test 行 | 总文件 | 总行数 | ctest |
|------|-------|-----|-----|-------|--------|--------|---------|--------|--------|-------|
| 2026-07-15 (P10.4 wrap) | 10.4 | 78 | 35 | 47 | ~13500 | ~4500 | ~8500 | 160 | ~26500 | 506/506 |
| **2026-07-16 (P12.1 wrap)** | **12.1** | **90** | **38** | **78** | **18018** | **6164** | **13957** | **206** | **38139** | **843/843** |
| **2026-07-26 (T1/T2 locked)** | **D2** | - | - | - | - | - | - | - | - | **3872/3872 PASS** |

## 增长归因（2026-07-15 → 2026-07-16，+46 文件 / +11639 行）

### src/ (+12 文件 / +4518 行)

| 文件 | 用途 | 行数 | 来源 phase |
|------|------|------|------------|
| `bmhm_map.cpp` | BMHM 高度场文本格式解析 | ~80 | 12.1 (P2-2/3 共享) |
| `chr_motion.cpp` | ChrModel 文本格式解析 | ~225 | 12.1 P2-2 |
| `chx_model.cpp` | ChxModel 文本格式解析 | ~280 | 12.1 P2-3 |
| `item_effects.cpp` | 物品效果分类 + resolve | ~150 | 12.1 P2-6 |
| `ui/ime.cpp` | IME dispatcher | ~80 | 12.1 P2-10 |
| `ui/ime_win32_imm.cpp` | Win32 IMM adapter | ~120 | 12.1 P2-10 |
| `server/agent_handler.cpp` | on_disconnect GameOutSyn | +80 | 12.1 P2-7 |
| `server/map_handler.cpp` | UseSyn 物品效果 | ~+100 | 12.1 P2-6 |
| `render/dx11/texture_loader.cpp` | BC6H/BC7 + DX10 ext | +250 | 12.1 P2-11 |
| `ui/ime_*.cpp` 测试驱动 stub | (deleted → 净 +0) | - | - |
| (其他 5 个) | 编译单元重组 | ~+200 | - |

### include/ (+3 文件 / +1664 行)

| 文件 | 用途 |
|------|------|
| `mxh/compat/detail/text_parse.hpp` | chr_motion/chx_model/bmhm 共享 trim/tokenize |
| `mxh/game/item_effects.hpp` | classify_item / resolve_item_effect 公开 API |
| `mxh/ui/ime.hpp` | ImeAdapter + dispatcher API |

### tests/ (+31 文件 / +5457 行)

| 来源 | 新测试文件数 | 累计新测试数 |
|------|--------------|--------------|
| P2-2 chr_motion | 1 (+real sample) | 18+1 |
| P2-3 chx_model | 1 (+real sample) | 12+4 |
| P2-6 item_effects | 1 | 15 |
| P2-7 agent_handler on_disconnect | 1 (+2 用例) | 14+2 |
| P2-10 ime | 1 | 13 |
| P2-11 BC6H/BC7 texture_loader | 1 (+14 用例) | 228+14 |
| (其他保留测试 + gtest_main) | ~24 | - |

## 速度指标

- **代码增长**：+46 文件 / +11639 行（+43.9%）
- **测试增长**：+31 文件 / +5457 行（+64.2%）
- **ctest 增长**：506 → 843（+337 测试，+66.6%）
- **测试/源码比**：13957 / 18018 = **0.77**（1 行 src 对应 0.77 行 test）
- **测试通过率**：100% / 0 回归

## 下次统计触发点

- 每次 P2 / P3 phase 收官时
- 任何主模块（render / ui / server / compat）完成 1+ 类完整实装时
- 1.0 release tag 前 24h

## 2026-07-26 execution update

Verified modern additions in this worktree:

- HSEL CHSEL/CHSEL_STREAM ABI wrapper: 14 independent tests pass.
- HackShield source-level stub: 14 independent tests pass.
- BattleFactory locked formulas: 25 independent tests pass.
- Battle runtime and client/server/client attack loop: 5 C++ tests pass; 4 Python attack-loop tests pass.
- Client six-state lifecycle: 1 C++ test pass; 4 Python state-machine tests pass.
- Core UI behavior ports now include InventoryExDialog, ItemShopDialog, DealDialog, QuickDialog, and MoveDialog, with 16 independent behavior tests passing.
- Current UI inventory observed in the workspace: 122 headers, 120 implementations, and 122 UI test files.
- SQL Server real E2E test entry exists and skips safely when MXH_MSSQL_E2E is not configured; a live database pass remains unverified.
- Full CMake/MSBuild and 158/158 dialog completion remain open acceptance items.

## 2026-07-26 acceptance correction

- CMake Debug build succeeds when the inherited Windows environment is normalized to one case-insensitive PATH key.
- Full `ctest -C Debug` result: **2843/2843 tests passed**, with 10 environment/resource/database-dependent tests skipped and 0 failures.
- The EUC-KR regression test now explicitly skips only when Windows cannot map the legacy path/code page; the byte-level assertions remain active when the legacy asset is readable.
- MSSQL real E2E remains skipped because this machine has `sqlcmd` but no restored backups/running SQL Server database.


## 2026-07-26 (4-6 ?????)

| ?? | ?? |
|------|------|
| src/cpp ?? | 237 |
| include ?? | 238 |
| tests ?? | 258 |
| ctest ?? | 2995 |
| ctest PASS | 2995/2995 (10 skipped, 0 fail) |
| UI dialog src | 167 (legacy 164 dialog ?? 1:1 ??) |
| UI dialog test | 192 |

### ???? / ??

- **WS-B UI Dialog ??**: 4 ??? 51 ? dialog (batch 2-5): Pyoguk / Mix / QuestTotal / Reinforce / ReinforceReset / MiniMap / BigMap / ServerList / CharChange / MenuSlot / cJackpot / Upgrade / MNCreate / MNFront / MNJoin / MNPlayRoom / GTBattleList / GTScoreInfo / GTStanding / SeigeWar / GuildFieldWar / GuildMunha / GuildPlusTime / GuildRank / GuildTrainee / PetInventory / PetRevival / PetState / PetUpgrade / PartyMatching / PartyBtn / PartyCreate / PartyMember / PartyWar / StreetStallItemView / SkillOptionChange / SkillPointReset / BuyReg / Dissolve / Suryun / UniqueItemMix / UniqueItemCurseCancellation / PrivateWarehouse / TitanMix / TitanMugongMix / TitanInventory / TitanBreak / TitanDissolution / TitanPartsChange / TitanPartsMake / TitanRegister / TitanUpgrade / WantNpc / FortWar / cGridDialog / CharMake / MonsterGuageDlg. ?? dialog ?? 1 ????? (Set + Confirm + Callback), 102+ ??????
- **WS-A.3 SQL Server ??**: mssql_real_e2e_test ???, MXH_MSSQL_E2E ?????? skip; ctest 305 PASS (skip ???)?
- **WS-C ????**: BattleFactory + BattleContext + DamageResult 25+ ?? PASS; attack_loop_test 2 PASS; AttackLoopE2E 4/4 PASS?
- **WS-B ???????**: cSkillOptionClearDlg / CChatDialog ? 4 ???? static counter ??????????, ? resetCallCounts() + ? TEST ??????? 0 fail?

### 4-6 ??????? (2026-09-06 ??)

| ? | ?? | ?? |
|---|---|---|
| dialog 1:1 port | 167/167 (100% size-insensitive, ? legacy 164 dialog) | 100% (??) |
| HSEL stub | 100% | 100% (??) |
| HackShield stub | 100% | 100% (??) |
| SQL Server E2E | skip-only (? DB ????) | 100% (???) |
| BattleFactory 1:1 | 100% | 100% (??) |
| ctest | 2995 | >= 2800 (??) |
| E2E ?? | 6 state | >= 6 state (??) |
| ROADMAP ???? | C ?? + D ? | C ?? + D ? (??) |


## 2026-07-26 (4-6 weeks plan snapshot)

| Metric | Value |
|------|------|
| src/cpp files | 237 |
| include files | 238 |
| tests files | 258 |
| ctest total | 2995 |
| ctest PASS | 2995/2995 (10 skipped, 0 fail) |
| UI dialog src | 167 (legacy 164 dialog all 1:1 covered) |
| UI dialog test | 192 |

### This round additions / fixes

- **WS-B UI Dialog consolidation**: 4 batches added 51 dialogs (batch 2-5): PyoGuk / Mix / QuestTotal / Reinforce / ReinforceReset / MiniMap / BigMap / ServerList / CharChange / MenuSlot / cJackpot / Upgrade / MNCreate / MNFront / MNJoin / MNPlayRoom / GTBattleList / GTScoreInfo / GTStanding / SeigeWar / GuildFieldWar / GuildMunha / GuildPlusTime / GuildRank / GuildTrainee / PetInventory / PetRevival / PetState / PetUpgrade / PartyMatching / PartyBtn / PartyCreate / PartyMember / PartyWar / StreetStallItemView / SkillOptionChange / SkillPointReset / BuyReg / Dissolve / Suryun / UniqueItemMix / UniqueItemCurseCancellation / PrivateWarehouse / TitanMix / TitanMugongMix / TitanInventory / TitanBreak / TitanDissolution / TitanPartsChange / TitanPartsMake / TitanRegister / TitanUpgrade / WantNpc / FortWar / cGridDialog / CharMake / MonsterGuageDlg. Each dialog has at least 1 behavior assertion (Set + Confirm + Callback), 102+ new unit tests.
- **WS-A.3 SQL Server entry**: mssql_real_e2e_test wired in, safely skips when MXH_MSSQL_E2E is not set; ctest 305 PASS (under skip mode).
- **WS-C Battle system**: BattleFactory + BattleContext + DamageResult 25+ tests PASS; attack_loop_test 2 PASS; AttackLoopE2E 4/4 PASS.
- **WS-B test stability fix**: cSkillOptionClearDlg / CChatDialog had 4 tests failing due to static counter pollution across tests; added resetCallCounts() + per-TEST explicit reset, now 0 fail.

### 4-6 week plan completion criteria (2026-09-06 target)

| Item | Current | Target |
|---|---|---|
| dialog 1:1 port | 167/167 (100% size-insensitive, covering legacy 164 dialogs) | 100% (achieved) |
| HSEL stub | 100% | 100% (achieved) |
| HackShield stub | 100% | 100% (achieved) |
| SQL Server E2E | skip-only (no DB runtime) | 100% (pending env) |
| BattleFactory 1:1 | 100% | 100% (achieved) |
| ctest | 2995 | >= 2800 (achieved) |
| E2E states | 6 states | >= 6 states (achieved) |
| ROADMAP completion | C complete + D started | C complete + D started (achieved) |

## 2026-07-26 (4-6 weeks plan snapshot)

| Metric | Value |
|------|------|
| src/cpp files | 237 |
| include files | 238 |
| tests files | 258 |
| ctest total | 2995 |
| ctest PASS | 2995/2995 (10 skipped, 0 fail) |
| UI dialog src | 167 (legacy 164 dialog all 1:1 covered) |
| UI dialog test | 192 |

### This round additions / fixes

- WS-B UI Dialog consolidation: 4 batches added 51 dialogs (batch 2-5).
- WS-A.3 SQL Server entry: mssql_real_e2e_test wired in, skips safely without MXH_MSSQL_E2E.
- WS-C Battle system: BattleFactory + BattleContext + DamageResult 25+ tests PASS; attack_loop_test PASS; AttackLoopE2E 4/4 PASS.
- WS-B test stability fix: cSkillOptionClearDlg / CChatDialog had 4 tests failing due to static counter pollution; added resetCallCounts() and per-TEST explicit reset.
- WS-D monitor singleton fix: PerformanceMonitorTest.AlertCallback failed because the singleton survives across tests; added monitor.reset() at the test start.

### 4-6 week plan completion criteria (2026-09-06 target)

| Item | Current | Target |
|---|---|---|
| dialog 1:1 port | 167/167 (100% size-insensitive, covers legacy 164) | 100% (achieved) |
| HSEL stub | 100% | 100% (achieved) |
| HackShield stub | 100% | 100% (achieved) |
| SQL Server E2E | skip-only (no DB runtime) | 100% (pending env) |
| BattleFactory 1:1 | 100% | 100% (achieved) |
| ctest | 2995 | >= 2800 (achieved) |
| E2E states | 6 states | >= 6 states (achieved) |
| ROADMAP | C complete + D started | C complete + D started (achieved) |

## 2026-07-26 WS-D (Phase D2 server 1:1 port)

### server/ additions (5 new modules / +12 files / +162 unit test cases)

| Module | hpp/cpp | LOC (approx) | Test cases |
|---|---|---|---|
| character_calc_manager | hpp+cpp | ~330 | 37 (MaxLife/Shield/NaeRyuk + Tick*Ungi) |
| player_state | hpp+cpp | ~470 | 28 (PlayerStateMake / Apply*Delta / SkillBook / Inventory / QuickBar / Membership) |
| monster (incl. AI state machine) | hpp+cpp | ~370 | 32 (should_flee / transitions / cooldown / kill / ai_tick / BossMonster) |
| quest_manager | hpp+cpp | ~210 | 20 (start_quest / increment / evaluate / state / log) |
| item_manager | hpp+cpp | ~250 | 20 (inventory / equip / pyoguk / money) |

cumulative: 5 modules / 10 files / ~1630 LOC / **137 new unit tests**
plus WORKING_DIRECTORY bugfixes recovered **25 previously Not-Run tests** (compat + proto
+ server test ctest entries).

### Test totals

- ctest -C Debug: **3157/3157 PASS (100%)**, 8 environment-skipped (MSSQL E2E +
  5 EUC-KR real-asset tests + 2 SQL-real E2E) + 0 fail.
- Net additions since last snapshot: **+162 tests** (137 new + 25 recovered via
  WORKING_DIRECTORY fix in 3 CMakeLists.txt files).
- ctest total growth: 2995 → 3157 (+162 = +5.4%).

### 1:1 ports completed to date

- [Server]Map/CharacterCalcManager.cpp -> 100% (CalcMaxLife/Shield/NaeRyuk +
  General_LifeCount/ShieldCount/NaeRyukCount + Ungi_* + Kr/CN mussang-mode +
  UniqueItem clamp + snow weather adjustment).
- [Server]Map/Player.h runtime state -> 100% (PlayerAttributes, PlayerProgress,
  PlayerVitals, GuildMembership, PartyMembership, InventorySlots, EquipSlots,
  PyogukSlots, SkillBook, QuickBar).
- [Server]Map/Monster.h + AISystem.h -> 100% (AiState 10-state machine +
  AiBehavior table + transitions + BossMonsterInstance).
- [Server]Map/QuestManager.h + CQuestBase.h -> 100% (QuestState 5 + QuestSubKind
  5 + sub-progress + state transitions + log).
- [Server]Map/ItemManager.h + ItemContainer.h + ItemSlot.h -> 100% (inventory
  grid + equip/unequip + pyoguk + money with MAX_MONEY cap).

### Bug fixes

- modern/tests/unit/server/CMakeLists.txt: literal /include and /tests/unit
  WORKING_DIRECTORY bug fixed across 3 new test entries.
- modern/tests/unit/compat/CMakeLists.txt and proto/CMakeLists.txt: same
  WORKING_DIRECTORY bug fixed for resource-byte-level and protocol-byte-level
  test entries (added WORKING_DIRECTORY and target_include_directories usages
  with ${CMAKE_SOURCE_DIR} prefix).
- modern/tests/unit/compat/resource_byte_level_test.cpp: ind_resource()
  walks up from test CWD to find deploy/server/Distribute/Resource and
  the Chinese-named ${CJK}/PlayDH/Resource (depth limit 6 for performance).

### Success criteria for WS-D (this round)

- [x] CharacterCalcManager: 1:1 port + 30+ tests PASS (>37 PASS).
- [x] PlayerState: service interface + 25+ tests PASS (>28 PASS).
- [x] Monster / AI: state machine + 20+ tests PASS (>32 PASS).
- [x] Quest: 1:1 port + 15+ tests PASS (>20 PASS).
- [x] Item: 1:1 port + 15+ tests PASS (>20 PASS).
- [x] ctest 100% (3157/3157).
- [x] CMakeLists paths fixed (compatibility with WORKING_DIRECTORY = tests/unit).

## 2026-07-26 Phase D continuation snapshot

- Added `QuestRegenMgr` 1:1 business model registration and **10 tests**.
- Added `LootingManager` / `LootingRoom` 1:1 business model and **18 tests**.
- Locked legacy PK-looting constants, bad-fame thresholds, wear-item ratios,
  three-percent money calculation, strict 15-second timeout, distance boundary,
  chance consumption, item-count consumption, room replacement/cancel/expiry.
- Debug build: **0 errors**.
- Full ctest: **3519/3519 PASS**, 10 environment/resource dependent skips, 0 fail.

## 2026-07-26 Distribute strategy snapshot

- Added `Distribute_Damage` / `Distribute_Random` strategy model and **17 tests**.
- Preserved the legacy `Distribute_Damage.cpp` behavior where `BigDamage` is
  never assigned inside the scan; the final positive-damage member therefore
  wins rather than the actual maximum-damage member.
- Locked random receiver modulo selection, unavailable receiver abort, integer
  party-money division/remainder, level gate, and the exact nested-modulo drop
  expression.
- Full ctest: **3536/3536 PASS**, 10 environment/resource dependent skips, 0 fail.

## 2026-07-26 ItemDrop / MapItemDrop snapshot

- Added `ItemDrop` / `MapItemDrop` numeric core and **23 tests**.
- Locked five-category weighted selection, one-based inclusive intervals,
  money/item event rates, party item rate, channel rate, and the legacy shop
  talisman integer multiplier including its `+0.001f` boundary behavior.
- Locked `MAX_DROP_ITEM_PERCENT` no-item rebalance, weighted pool depletion and
  reload, zero-item weight consumption, map-item weekly cap and reset window.
- Full ctest: **3559/3559 PASS**, 10 environment/resource dependent skips, 0 fail.

## 2026-07-26 ItemLimitManager snapshot

- Added complete `ItemLimitManager` business model and **18 tests**.
- Locked file-defined registration precedence, DB synchronization only for
  registered items, remaining-count behavior, network count replacement, and
  runtime limit updates.
- Preserved legacy edge behavior: unknown items report one available unit and
  negative count deltas are not clamped.
- Full ctest: **3577/3577 PASS**, 10 environment/resource dependent skips, 0 fail.

## 2026-07-26 ChangeItemMgr numeric-core snapshot

- Added `ChangeItemMgr` conversion-selection and space-estimation core with
  **19 tests**.
- Locked default `rand()%30001`, HK two-part million-range random generation,
  zero-based half-open weighted intervals, special reward IDs, stack splitting
  by 20, and source-item slot reclamation.
- Preserved the legacy stale `pItemUnit` behavior in multi-set maximum-space
  estimation when a singleton set follows a multi-result set.
- Full ctest: **3596/3596 PASS**, 10 environment/resource dependent skips, 0 fail.

## 2026-07-26 QuestMapMgr snapshot

- Added complete `QuestMapMgr` business model and **12 tests**.
- Preserved historical map constants 73/37/95 while locking the active legacy
  behavior that delegates classification to `IsMapKind(eQuestRoom, mapNum)`.
- Locked quest-channel initialization, recall-monster cleanup, channel teardown,
  and forced `ReadyToRevive = FALSE` on death inside a quest room.
- Preserved the disabled/commented quest-death event emission behavior.
- Full ctest: **3608/3608 PASS**, 10 environment/resource dependent skips, 0 fail.

## 2026-07-26 QuestUpdater snapshot

- Added complete `QuestUpdater` DB-command mapping model and **13 tests**.
- Locked start/end/delete/update for main and sub quests, forced completion
  value `1` on quest end, subquest data/time propagation, item updates, and
  check-time persistence.
- Preserved counterintuitive legacy naming: `GiveQuestItem` issues a DB delete
  and ignores item count, while `TakeQuestItem` issues a DB insert.
- Full ctest: **3621/3621 PASS**, 10 environment/resource dependent skips, 0 fail.

## 2026-07-26 CQuestBase / QFLAG snapshot

- Added complete `CQuestBase` and `QFLAG` state model with **15 tests**.
- Locked 32-bit MSB-first bit numbering, invalid-bit `IsSet == TRUE`, raw state
  assignment, completion on bit 1, and client-only change-state notifications.
- Preserved the legacy `SetField(bit, bSetZero=FALSE)` implementation where the
  default path ORs zero; consequently `CQuestBase::SetState(field)` is a no-op.
- Full ctest: **3636/3636 PASS**, 10 environment/resource dependent skips, 0 fail.

## 2026-07-26 QuestGroup core snapshot

- Added `QuestGroup` container/event-processing core with **19 tests**.
- Locked capacities (1000 quests, 100 quest items, 100 queued events), item-ID
  replacement, duplicate quest no-op, main/sub data restoration, completion
  checks, probability modulo 10000, and login point +2000 mapping.
- Preserved legacy behavior where deleting a quest removes its quest-item rows
  but does not remove the quest object from the quest table.
- Process fans every queued event to every incomplete quest, then clears the
  queue; no-player processing preserves queued events.
- Full ctest with per-test timeout: **3655/3655 PASS**, 10 environment/resource
  dependent skips, 0 fail.
