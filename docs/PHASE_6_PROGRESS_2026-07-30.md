# Phase 6 Module Port Progress — 2026-07-30 snapshot

> **ctest**: 5443/5443 PASS, 0 FAIL
> **Wall time**: ~88 seconds
> **Modules ported this session**: 19
> **Test count delta**: 5300 → 5443 (+143)

## Ported modules

### Phase 6.2 — MapServer core (8 batches, ~74 tests)
1. **Monster** + **BossMonster** trio (`BossMonster`, `BossMonsterInfo`, `BossMonsterManager`, `BossState`, `FieldBossMonsterManager`, `cMonsterSpeechManager`, `DropTableRegistry`) — 24 tests
2. **SkillManager** + **MugongManager** — 7 tests
3. **ItemContainer** (`Inventory` 80, `Wear` 10, `ShopInven` 20, `Pyoguk` 80) — 9 tests
4. **AbilityGroup** + **DelayGroup** (Effect stack + cooldown queue) — 7 tests
5. **NoteManager** + **AutoNoteManager** + **AutoNoteRoom** — 7 tests
6. **WantedManager** + **WantNpcManager** (player + NPC bounty) — 7 tests
7. **MurimNetChannellingManager** + **MurimNetChat** — 7 tests
8. **ShowdownManager** + **AuctionManager** — 8 tests

### Phase 6.4 — DistributeServer scaffolding (3 batches, ~16 tests)
9. **DistributeServer** — 3 tests (state machine)
10. **MNServer** — 4 tests (PvP channels)
11. **LoginServerDBMsgParser** — 4 tests (auth + gm ladder)
12. **ServerList** — 4 tests (16-byte IP+port routing)
13. **DistributeNetworkMsgParser** — 5 tests ((category, proto) dispatch)

## Still TODO (priority ordered)

### MapServer critical
- **Player** main class (40KB legacy → wire-format struct + tests)
- **PKManager** + **PKLootingManager** — PvP core
- **MoneyManager** — money flow
- **PetState** + **PetSpeech** + **PetRevival** + **PetUpgrade**
- **TitanItemManager** + **TitanShopItem** + **TitanParts**
- **AIGroupManager** + **AIUniqueGroup**
- **Quest** 8-file extension (CQuestBase/Quest/QuestGroup/QuestNpcScript)
- **GTManager** + **GTRegist** + **GTBattle** + **GTScore** + **GTStanding**
- **SiegeWarManager** + **FortWarManager**
- **EventMapMgr** + **StateSynced** + **MemoryChecker** + **GameEvent**

### Distribute / MurimNet remaining
- DistributeNetworkMsgParser handlers (40+ categories)
- CharacterMovementManager + CharMove
- AgentBattle + AgentShop + AgentQuest + AgentMugong + AgentChat (AgentServer expansion)
- MNChannel + MNCreateRoom + MNJoinRoom + MNTournament + MNBattle + MNPlayRoom
- All Distribute/MurimNet 1:1 wire-format tests

### T1/T2/T3 cert closure
- T1 resource byte-level: ≥30 resource types PASS (`tests/unit/compat/resource_byte_level_test.cpp`)
- T2 protocol byte-level: ≥15 packet categories PASS (`tests/unit/proto/protocol_byte_level_test.cpp`)
- T3 side-by-side: 5-ops `tests/e2e/side_by_side_5ops.py` diff=0
- Docker SQL Server: `docker/sqlserver/` + `restore_databases_simple.ps1 -ComposeUp`
- MoxianSideBySide: legacy vs modern server replay harness
- 1.0 release tag

## Risk register

| Risk | Mitigation |
|---|---|
| 400+ modules is 1+ years of single-threaded work | Run multi-target parallel; spawn sub-agents for parallel port batches |
| Docker unavailable on Windows | Local SQL Server Express + restore_databases_simple.ps1 fallback |
| Legacy server not runnable on Win11 | Process Monitor + DLL recovery; Python harness fallback |
| side-by-side diff non-zero | Lock legacy behavior; repeat-iterate patch loop until diff=0 |
