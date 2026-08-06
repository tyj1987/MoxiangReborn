# CHANGELOG - 历史完成项

> 完整 commit 历史: git log --oneline (1 commit = 1 sub-deliverable)。
> 本文件保留 2026-08 重构前 ROADMAP 的关键历史 [x] 项摘要, 用于回溯。

最近重构: 2026-08-06 - 把老 ROADMAP (434 行) 砍成规划文档 (158 行) + 本 CHANGELOG。
### D4.27 DiscardAvatarItem data plane (2026-08-06)

- D4.27 - 1:1 port of the data-plane half of legacy CShopItemManager::DiscardAvatarItem(WORD ItemIdx, WORD ItemPos) from [Server]Map/ShopItemManager.cpp.
- Adds modern/include/mxh/server/discard_avatar_item.hpp: AvatarEquipRow struct (Position + Item[24] mask, mirrors legacy AVATARITEM) + kAvatarDefaultFillStart/End constants (12..18 = [Weared_Hair, Weared_Gum)) + free function discard_avatar_item(equip, idx, current_avatar) -> new_avatar. The 4 no-op conditions are: missing equip, position out of range, avatar[pos] != item_idx, position at sentinel.
- Adds modern/src/server/discard_avatar_item.cpp: implementation with the clear-and-default-fill loop matching the legacy collapsed iteration.
- 11 tests in modern/tests/unit/server/discard_avatar_item_test.cpp: 4 no-op conditions + matching-slot clear + default-fill zeros->ones + non-zero masked slots preserved + all masked slots preserved + non-weared slots untouched + Weared_Gwun boundary check + position-at-Max sentinel test.
- See commit a96c6547: server: D4 discard_avatar_item data plane - AvatarEquip clear + default-fill + 11 tests.

### D4.26 IsPetSummonItem/IsTitanCallItem/IsTitanEquipItem + GetItemKindType + playtime_decrement (2026-08-06)

- D4.26 - 1:1 ports of three ItemKind predicates + GetItemKindType + the CheckEndTime PLAYTIME decrement arithmetic, all as pure data-plane free functions.
- Adds modern/include/mxh/server/item_kind_predicates.hpp: is_pet_summon_item (ItemKind == QUEST_PET or SHOP_PET), is_titan_call_item (ItemKind == TITAN_PAPER), is_titan_equip_item (ItemKind & TITAN_EQUIP_UMBRELLA = bit 7), all with nullptr -> false.
- Adds modern/include/mxh/server/get_item_kind_type.hpp: get_item_kind_type(info, kind_out, type_out) - writes (ItemKind, ItemType) on hit, zeros on miss.
- Adds modern/include/mxh/server/playtime_decrement.hpp: playtime_decrement(remtime, last_check, now) -> {new_remaintime, elapsed_clamped_ms} with the 30s clamp + underflow-to-zero arithmetic.
- 22 tests across modern/tests/unit/server/item_kind_predicates_test.cpp (10) + get_item_kind_type_test.cpp (3) + playtime_decrement_test.cpp (9).
- See commits e5b27b1c (item_kind_predicates) + 2dab9b8d (get_item_kind_type) + c26c1d72 (playtime_decrement).

### D4.25 IsDupItem data plane (2026-08-06)

- D4.25 - 1:1 port of legacy CItemManager::IsDupItem(WORD wItemIdx) from [Server]Map/ItemManager.cpp. Pure data plane: returns true iff the inventory is allowed to stack duplicates of the item.
- Adds modern/include/mxh/server/is_dup_item.hpp: legacy item-kind constants (13 always-dup kinds: YOUNGYAK*4 + EXTRA*7 + SHOP_CHARM + SHOP_HERB), Sundries exception (SimMek / CheRyuk / Shout idx), 30-entry Incantation non-dup list (TownMove15 / MemoryMove15 / MemoryMoveExtend* / ShowPyoguk* / Tracking* / Extend* / CharacterSlot*), Skin no-dup (NOMALCLOTHES_SKIN / COSTUME_SKIN), free function is_dup_item + inline wrappers is_rare_option_item (legacy IsRareOptionItem) and is_option_item (legacy IsOptionItem).
- Adds modern/src/server/is_dup_item.cpp: implementation with incantation_is_non_dup helper for the 30-entry switch.
- 19 tests in modern/tests/unit/server/is_dup_item_test.cpp: 4 always-dup kinds + 7 extra kinds + shop charm/herb + Sundries 5 branches + 30-entry Incantation non-dup list + Incantation LimitLevel+SellPrice gate + Skin 3 branches + NullInfo + UnknownKind + 2 precedence tests + 3 IsRareOptionItem/IsOptionItem wrapper tests.
- See commits 535742c2 (16 tests) + 56637dfb (3 wrapper tests).


### D4.24 AddDupParam/DeleteDupParam/IsDupAble data plane (2026-08-06)

- D4.24 - 1:1 port of legacy CShopItemManager::AddDupParam / DeleteDupParam / IsDupAble from [Server]Map/ShopItemManager.cpp:2286-2620. Splits the legacy per-bit OR / XOR-when-both-set / block semantics across 5 dup categories (charm / herb / incantation / sundries / pet_equip) into a pure data plane.
- Adds modern/include/mxh/server/dup_param.hpp: DupCounters struct (5 counter fields) + DupParamIndices struct (5 index fields) + DupParamLookup virtual class (returns 0 by default) + SundrySideEffects struct (set/clear bStreetStall booleans) + 5 inner namespaces (charm_dup / herb_dup / incantation_dup / sundries_dup / pet_equip_dup) with bit constants matching [CC]Header/CommonGameDefine.h:2874-2930 (DONTDUP_* flag values 1:1 with legacy) + 3 free functions (add_dup_param, delete_dup_param, is_dup_able).
- Adds modern/src/server/dup_param.cpp: implementation with apply_or (OR, idempotent for Add) and apply_delete (XOR-when-both-set, clears bit only if both counter and dup_param have it). Sundries street-stall side effect captured in SundrySideEffects struct.
- 33 tests in modern/tests/unit/server/dup_param_test.cpp: 32 named tests covering AddDupParam (NoIndices, CharmBitsOrIntoCounter, CharmOrIsIdempotent, IncantationBitsOrIntoCounter, SundriesBitsOrAndTriggersStreetStall, SundriesBitsWithoutStreetStallHasNoSideEffect, PetEquipBitsOrIntoCounter, AllFiveCategoriesTogether, LookupReturnsZeroIsNoOp) + DeleteDupParam (NoIndices, CharmBitsClearedWhenBothSet, CharmBitsUntouchedWhenCounterEmpty, IncantationBitsClearedWhenBothSet, SundriesClearsAndTriggersClearStreetStall, SundriesNoClearWhenCounterEmpty, PetEquipBitsClearedWhenBothSet, AllFiveCategoriesTogether) + IsDupAble (EmptyCountersAllDupEnabled, CharmOverlapBlocks, CharmNoOverlapAllows, HerbOverlapBlocks, IncantationOverlapBlocks, SundriesOverlapBlocks, PetEquipOverlapBlocks, NoIndicesReturnsTrue, LookupReturnsZeroIsTrue, AnyCategoryOverlapBlocks, NoOverlapReturnsTrueWithAllIndicesSet, DisjointBitsAllAcrossFiveCategoriesReturnsTrue).
- See commit 529e1843: server: D4 AddDupParam/DeleteDupParam/IsDupAble data plane - 5 dup categories + 33 tests.

### D4.23 CalcAvatarOption data plane (2026-08-06)

- D4.23 CalcAvatarOption - 1:1 port of legacy CShopItemManager::CalcAvatarOption() from [Server]Map/ShopItemManager.cpp:2024-2183 (avatar stat accumulator).
- Adds modern/include/mxh/game/avatar_item_option.hpp: AvatarItemOption struct (25 fields, 39 bytes under pack(1), byte-identical to legacy AVATARITEMOPTION) + AvatarSlot enum (eAvatar_Hat..WeeMaxAmgi, Max=24) + EAvatarCount=24 constant.
- Adds modern/include/mxh/server/avatar_calc.hpp: calc_avatar_option(avatar[24], ItemManager&) - walks 24 slots, skips wIconIdx<2 (cosmetic empty/base-skin) and ItemInfo misses, accumulates 25 field deltas with the legacy >0 / ==1 predicates and uint16->uint8 narrow casts.
- 24 tests in modern/tests/unit/game/avatar_calc_test.cpp: layout size + enum indices + empty/cosmetic-base/unknown slots + single-field accumulator + 25-field all-fields pass + multi-slot accumulator + bKyungGong/NaeruykspendbyKG flag-only semantics + LimitSimMek truthy predicate + uint16->uint8 truncation + 24-slot full sweep.
- See commit 7e49a5a5: server: D4.23 CalcAvatarOption data plane + AVATARITEMOPTION struct + 24 tests.

### D4.22 CheckAvatarEndtime data plane (2026-08-06)

- D4.22 CheckAvatarEndtime - 1:1 port of legacy CShopItemManager::CheckAvatarEndtime() from [Server]Map/ShopItemManager.cpp:1142-1180 (avatar-only counterpart to CheckEndTime, runs unconditionally every tick the avatar-event loop calls it).
- Adds modern collect_avatar_realtime_expired(PackedTime now, out) data plane function (header in modern/include/mxh/server/shop_item_manager.hpp, impl in modern/src/server/shop_item_manager.cpp). Predicate is byte-identical to collect_realtime_expired (SellPrice == eShopItemUseParam_Realtime AND curtime > EndTime); the side-effect chain (DiscardItem + SendMsgDwordToPlayer(MP_ITEM_SHOPITEM_USEEND) + ShopItemDeleteToDB + LogItemMoney) is the orchestrator's responsibility and is documented in the implementation comment.
- 8 tests in modern/tests/unit/server/shop_item_manager_test.cpp ShopItemManagerCheckAvatarEndTime suite: NoRows / PlaytimeRowsSkipped / FutureEndTime / EqualEndTime / PastEndTime / MixedRows / DoesNotMutateTable / MatchesCollectRealtimeExpiredExactly.
- See commit 668ec566: server: D4.22 CheckAvatarEndtime data plane + 8 tests (legacy 4-step side-effect chain documented).

### build: mxh_compat_tests linker fix (2026-08-06)

- mxh_compat_tests previously failed to link because item_effects.cpp (in mxh_compat) references mxh::game::ItemManager::try_get, which lives in src/game/item_manager.cpp. The test target linked mxh_compat + gtest_main but not mxh_game, leaving 81 tests Not Run.
- Fix: add mxh_game to mxh_compat_tests target_link_libraries in modern/tests/unit/CMakeLists.txt. 1 line change. 81 previously Not-Run compat tests now run + pass.
- See commit f385b8d7: build: fix mxh_compat_tests linker error - link mxh_game (ItemManager::try_get).


### E2 T2 wire golden round-trip coverage (2026-08-06)

- E2 T2 wire golden round-trip - 对所有 84 个 golden .bin 文件各加一个 TEST(WireFormatGolden, RoundTrip_X)，解析字节到 Packet struct，再 wire_bytes() 重编码，断言字节完全相同。 11 个原 wire format 测试 + 84 个 golden round-trip = 95 tests, 95 PASSED, 0 FAILED。 锁住现代 wire encoder 与 legacy [Server]*/4DyuchiNET_Latest 字节级 1:1。
- 加 find_golden_dir() helper (server/golden 等路径候选) + filesystem/fstream/string 头文件。

### E1 T1 parse test subdir expansion (2026-08-06)

- E1 T1 parse test subdir expansion - deploy Server/ + QuestScript/ + PlayDH non-Client (top-level + EffectScript/Map/QuestScript/SkillArea) 全部 read_mh_bin parse ok = true + size match + 256MB 限。
- 总数 89 -> 268 parse test entries across 5 suites (MxhResourceParse 60 + MxhResourceParseClient 29 + MxhResourceParseServer 108 + MxhResourceParseQuestScript 6 + MxhResourceParsePlayDh 65). 7 个 14-byte placeholder stub 被过滤 (header.file_size = 0 会失败 EXPECT_GT)。
- 与 SHA-256 manifest 锁定的 303 records 严格对齐：每个有 SHA-256 记录的 .bin 都有对应 parse test。



---

## 截止 2026-08-06 已完成项

### E1 T1 资源字节级验证 (2026-08-05)

- E1 T1 payload SHA-256 byte-level verification - 3 个 manifest (deploy + PlayDH + PlayDH/Client), 146 records, 117 .bin 文件 SHA-256 + byte-size + header.type 锁定。3/3 VerifyManifest_* PASS。
- E1 T1 mechanical expansion - 89 deploy/Resource .bin 全部 read_mh_bin parse ok = true + size match + 256MB 限。

### Phase D 玩法/数值 1:1

- D1.1 + D1.2 + D1.3 SkillList.bin parser + call-site - SkillInfo 扩到 60+ 字段 1:1 legacy SKILLINFO, SkillListParser 解码 MHFile packed-text, 1817 entries load 成功 0 parse_errors。MapHandler 用真 SkillList.bin 而非 4-skill hardcode。
- D2 BattleFactory 1:1 - 14 compute_* 函数 (critical/decisive/player-phy/player-attr/exp/point/phy-defence/received-dmg/monster-phy/monster-attr/titan-phy/titan-attr) + 13 legacy_* 测试。
- D3.x QuestManager (D3.5-D3.7):
  - D3.5 QuestScriptLine 端到端 parser - 9 event tokens + 6 limit tokens + &Limit + @Event + *Execute layout
  - D3.6 QuestTrigger runtime evaluator - 10 tests, EndQuest fix (args[0] 1-arg subquest 索引)
  - D3.7 QuestScript subquest block parser - 12 tests
  - D3.3 / D3.4 QuestExecute data-plane dispatch (5+4 QuestGroup + 4 dispatch tests)
- D5 MurimNet 1:1 wire - Channel + PlayRoom + 60 协议代码 + 9 wire serializer + 7 short wire + runtime.broadcast_chat sink + MurimNetCrypt + 139 tests。
- D6.1 数值 baseline - 7 OBJECTKIND / 6 MonsterAI / 14B MonsterTotalInfo / 22B ItemBase / 124 槽 / 2728B ItemTotalInfo / 3775B GameInAck hero payload / 4 ItemEffect 公式 / 3 default MonsterTemplate 全部 1:1 锁死。
- D6.2 Distributer recipient + party-exp pure decisions - legacy null-party tie no-op, 50/50 tie, float allocation, zero-send gate. 10 new tests, 45/45 focused PASS。
- D6.2 FieldBossMonsterManager 1:1 - 19 tests (channel 配置/出怪/重置/单飞/限频)。
- D6.2 ChooseOne tie-break + SetPlusTotalDamage += semantics。
- D6.3 BossMonsterManager 1:1 - 19 tests (register/spawn/erase/damage/live_count)。
- D6.4 cMonsterSpeechManager 1:1 - 15 tests。
- D6.5 ExperienceCurve 1:1 - 15 tests (CharacterExpPoint.bin reader, add_exp 限一次升级)。
- D6.6 state_param + summon_monster + titan_item_manager 1:1 - 31 tests。
- D6.7 distribute_network_msg_parser + common_network_msg_parser 1:1 - 19 tests。
- D6.x ItemList.bin 1:1 parser - ITEM_INFO 77 fields, 56/60 token MHFile packed-text, 9887 rows 0 parse errors。Shared decode_mhfile_text_payload helper。
- R-8 item_effects real lookup via ItemManager - ItemManager.init_from_bin / get / try_get / exists / size。resolve_item_effect_with_manager 读 LifeRecover / LifeRecoverRate / NaeRyukRecover / NaeRyukRecoverRate。12 tests PASS。
- R-8 call-site MapHandler.load_item_list - 3 tests。
- B6.1 HSEL YHLibrary ABI 校正 - 79 crypto tests, non-virtual dtor / 2 const virtual getters / protected version/type fields / non-virtual CHSEL_STREAM。

### R-2 HackShield 路由 + tick

- R-2 AgentHandler HackShield routing - cat==HackShield (67) 通过 mxh::server::parse_hackshield_message, conn_user_levels_ + conn_hs_states_ + hackshield_disconnect_pending_, superuser (>=5) 立即 send_guid_req。10 tests。
- R-2.1 AgentHandler auto-populate user_level from chr_log_info (proto=9) - 3 tests。
- R-2.2 AgentHandler::tick_hackshield() server-side periodic recheck - 4 tests (empty/grace/non-superuser/mixed)。
- bug: Crypt::encrypt_crc/decrypt_crc 返回 0 而非 HselStream CRC char - fixed + 4 tests。

### MurimNet 1:1 关键

- commit 83d53c1b - MurimNetChannel.for_each_channel + PlayRoomManager.for_each_room + SendMsg_ChannelList/PlayRoomList
- commit ce931c51 - wire-byte serializer (MSG_CHANNEL_BASEINFO/MSG_PLAYROOM_BASEINFO/#pragma pack(1) 9 size 锁定)
- commit 845294ec - SendMsg_ChannelList/PlayRoomList 8 tests (size = GetMsgLength, Category=38, Protocol=34/35, title 63 字节)

### Phase C UI 1:1 port - 主要 batches (Batch 1.1-1.10 + Batch 2.1-2.80)

完整 109 dialog 列表 (按 phase commit 索引):
- Batch 1.1 - 1.10 (10 dialog): cNumberPadDialog / cPartyWarDialog / cWantedDialog / cMPNoticeDialog / cReviveDialog / cMiniFriendDialog / cGuildInviteDialog / cChatOptionDialog / cStallKindSelectDlg / cDebugDlg (331 tests)
- Batch 2.1 - 2.7 (7 dialog): cMPGuageDialog / cAlertDlg / cChinaAdviceDlg / cLoadingDlg / cKeySettingTipDlg / cIntroReplayDlg / cNameChangeNotifyDlg (210 tests)
- Batch 2.32 - 2.80: 30+ dialog (cCharChangeDlg, cBailDialog, cChaseInputDialog, cObjectStateManager 等, 每 dialog 10-25 tests)
- 收口 fixes: cChatOption checked event bits, cListDialogEx legacy event bits, cPushupButton sticky click state, cListItem m_maxLine=0 inconsistency。

### Phase B 服务端 E2E

- B.2.1 CLoginState - 38B legacy payload 字节级
- B.2.2 CCharSelectState - 8B ListSyn + 889B ListAck + 自动选第一个 + SelectSyn
- B.2.3 CInGameState - 3775B SEND_HERO_TOTALINFO GameInAck
- B.2.4 state machine 真接 net 事件
- B.2.5 MoxianClientE2E headless tool - CreateProcessW 启 3 server + C++ 状态机跑 Login -> CharSelect -> InGame (60s timeout)

### Phase A 客户端

- f80f6e0 - MoxianClient skeleton (DX11 + WinMain + 4 sprite + cImage <-> IDISpriteObject)
- CMainGame 1:1 port - eGAMESTATE 0..9 byte-for-byte + 9 state stub + CMainTitle (14 字段 1:1 surface + Init 读 MHVerInfo.ver)

---

## 老 ROADMAP 中的 Phase 0-12 噪音

凡旧词提及"Phase 0-12 已完成""35.1% complete""Qoder IDE Quest" 等, 全部废弃。参见 ROADMAP §6。


### D3 quest event dispatcher bridge (2026-08-06)

- D3 runtime bridge - 把 legacy CQuestManager::AddQuestEvent -> CQuestGroup::AddQuestEvent -> sub-condition 匹配 1:1 移植到 modern runtime。新增 mxh::server::QuestEvent {kind,target_id,delta} + dispatch_quest_event(QuestLog&, const QuestEvent&)，按 QuestLog 顺序遍历，只修改 Accepted 状态（与 legacy group 对 terminal quest 的 guard 一致），每个匹配 sub-quest 累加 delta 并 clamp 到 target，返回 std::vector<QuestEventChange> {quest_id, previous_state, state, updated_subs}。
- 6 个新行为锁定测试：DispatchQuestEvent.UpdatesEveryMatchingActiveQuestInLogOrder / CompletesQuestWhenFinalConditionMatches / UpdatesAllMatchingSubsWithinQuest / IgnoresNonMatchingAndTerminalQuests / ZeroDeltaAndNoneKindAreNoOps / DoesNotMutateOtherQuests。mxh_quest_manager_tests: 37 -> 43 tests PASS, 0 regressions; 服务端 ctest 313 项中 2 flaky (LoginServerFixture Weather/GuildFieldWar) 重跑 100% 通过。
- 见 commit 9512a082: server: D3 quest event dispatcher bridge (legacy AddQuestEvent->QuestGroup semantics)。



### D4 use_shop_item_decision data plane (2026-08-06)

- D4.20 use_shop_item_decision - 把 legacy CShopItemManager::UseShopItem 中*实际*落到 SHOPITEMBASE 字节上的部分 1:1 提取为 modern 自由函数：InvalidIcon / ItemInfoMissing / AlreadyInUse 三道 guard + Realtime/Playtime/Continue/SellPrice==0 四个 BeginTime/Remaintime 分支 + ItemKind 到 dup counter (Incantation/Charm/Herb/Sundries/PetEquip) 的路由。
- 新增 inline 常量 SHOP_ITEM_USE_PARAM_REALTIME/PLAYTIME/CONTINUE + LEGACY_SHOP_ITEM_* (257/258/259/260/261/262/263/264/300/310，1:1 引用 [CC]Header/CommonGameDefine.h:698-713)。
- 8 个新行为锁定测试：RejectsEmptyIcon / RejectsMissingItemInfo / RejectsAlreadyInUse / RealtimeBranchEncodesEndTimeInRemaintime / PlaytimeBranchConvertsRarityToMilliseconds / ContinueBranchHasNoExpiry / ZeroSellPriceRoutesSundriesAndNoTimer / RoutesHerbToHerbDup。mxh_shop_item_manager_tests: 91 -> 99 tests PASS, 0 regressions (ctest -R ShopItemManager|UseShopItemDecision 100% 通过)。
- 注释解释：legacy UseShopItem 整段被 `/* ... */` 注释（"임시로 놈음 - 성대"），不可 1:1 复现；本函数只覆盖能落到 SHOPITEMWITHTIME 行的 guard + BeginTime/Remaintime 部分。
- 见 commit dcb05173: server: D4 use_shop_item_decision data plane (legacy UseShopItem guard + BeginTime/Remaintime)。



### D4.21 CheckEndTime realtime branch data plane (2026-08-06)

- D4.21 CheckEndTime realtime branch - 把 legacy CShopItemManager::CheckEndTime 中 Realtime 模式 (`SellPrice == eShopItemUseParam_Realtime`) 的数据面 1:1 提取为 modern 方法: collect_realtime_expired(PackedTime now, out) / consume_realtime_expired(PackedTime now)。predicate 与 legacy `curtime > stTIME(Remaintime)` 严格一致；只扫描 `Param == SHOP_ITEM_PARAM_STORED_TIME` 的行（playtime/continue/one-shot 跳过）。
- 7 个新行为锁定测试: NoRowsNoExpirations / PlaytimeRowsAreSkipped / FutureEndTimeIsNotExpired / EqualEndTimeIsNotExpired / PastEndTimeIsCollected / MixedRowsSelectsOnlyStoredTimeExpired / ConsumeIsIdempotentOnAlreadyExpiredRows。mxh_shop_item_manager_tests: 99 -> 106 tests PASS, 0 regressions。
- 见 commit 3e21470e: server: D4.21 CheckEndTime realtime branch data plane (legacy SellPrice==Realtime sweep)。




### D3.8 quest_group_run_pending (2026-08-06)

- D3.8 quest_group_run_pending - ? legacy CQuestGroup::Process() ???? 1:1 ??? modern ????:?? quest_group_process(state) ? dispatch list,? (quest, event) ??? triggers_by_quest ?,??? quest ?????? trigger (legacy CQuestTrigger::OnQuestEvent ? "first matching condition wins" ??),?? std::vector<QuestGroupApplyResult> {quest_idx, subquest_idx, status, changed}?
- 7 ????????:NoEventsProducesEmptyResult / AppliesFirstMatchingTriggerAndAdvancesSubCount / SkipsQuestWithNoTriggerEntry / FiresFirstConditionMatchAndIgnoresTheRest / FansOutAcrossMultipleQuests / ConditionFailedLeavesQuestUntouched / IdempotentAfterEventsDrained?mxh_quest_group_tests: 23 -> 30 tests PASS, 0 regressions?
- ? commit 9e5ba572: server: D3.8 quest_group_run_pending (legacy CQuestGroup::Process tick link)?



### D3.9 subquest counter access (2026-08-06)

- D3.9 subquest counter access - ? legacy CQuestGroup::GetSubQuestValue + CQuestGroup::ChangeSubQuestValue ???? 1:1 ??? modern ????:get_sub_quest_value(state, questIdx, subIdx) ?? subQuestData[subIdx] ? 0xFFFFFFFFu sentinel(legacy "no such quest" marker),change_sub_quest_value(state, questIdx, subIdx, kind) ?? QUEST_VALUE_ADD/MINUS(legacy eQuestValue_Add/Minus from QuestDefines.h:128-129),Minus clamp ? 0,unknown kind ???
- ?? inline ?? QUEST_VALUE_ADD=0u / QUEST_VALUE_MINUS=1u / QUEST_SUB_QUEST_VALUE_NOT_FOUND=0xFFFFFFFFu?get_quest ? const ??????? lookup?
- 8 ????????:GetMissingQuestReturnsSentinel / GetExistingButUnsetSubQuestReturnsZero / GetReturnsStoredCount / AddIncrementsCount / MinusDecrementsCount / MinusClampsAtZero / UnknownKindIsRejected / MissingQuestReturnsFalse?mxh_quest_group_tests: 30 -> 38 tests PASS, 0 regressions?
- ? commit f5a7d89b: server: D3.9 subquest counter access (legacy GetSubQuestValue + ChangeSubQuestValue data plane)?



### D3.10 quest end lifecycle tests (2026-08-06)

- D3.10 quest end lifecycle tests - ????? quest_group_end_quest / quest_group_end_subquest ? 7 ???????,?? legacy CQuestGroup::EndQuest(questIdx, subQuestIdx) + CQuestGroup::EndSubQuest ??????:missing-quest ?? false?repeat=0 ?? complete / clear active subs / ? subQuestData?repeat=1 ?? quest ??? reset counter?idx==0 ? end_subquest ??? quest->data + quest->time?
- mxh_quest_group_tests: 38 -> 45 tests PASS, 0 regressions?
- ? commit 3616baac: server: D3.10 lock legacy EndQuest + EndSubQuest semantics (7 lifecycle tests)?





### D3.11 weapon-filtered AddCount (2026-08-06)

- D3.11 weapon-filtered AddCount - ? legacy CQuestGroup::AddCountFromWeapon + AddCountFromQWeapon ???? 1:1 ??? modern ????:add_count_from_weapon(state, questIdx, subIdx, max, requiredWeaponKind, playerWeaponKind) ? `playerWeaponKind == requiredWeaponKind` ???? quest_group_add_count;add_count_from_q_weapon(state, ...) ??????? weapon item-idx???????????? false ??? counter(legacy "???" 1:1 quirk)?
- ????????? quest_group_add_count ? max-bound clamp ??,?? + ???????
- 6 ????????:AddCountFromWeaponMismatchedKindDoesNothing / AddCountFromWeaponMatchingKindIncrements / AddCountFromWeaponClampsAtMax / AddCountFromQWeaponMismatchedItemDoesNothing / AddCountFromQWeaponMatchingItemIncrements / AddCountFromWeaponMissingQuestReturnsFalse?mxh_quest_group_tests: 45 -> 51 tests PASS, 0 regressions?
- ? commit 67dcac95: server: D3.11 weapon-filtered AddCount (legacy AddCountFromWeapon + AddCountFromQWeapon data plane)?





### D3.12 weapon-filtered TakeQuestItem (2026-08-06)

- D3.12 weapon-filtered TakeQuestItem - ? legacy CQuestGroup::TakeQuestItemFromWeapon + TakeQuestItemFromQWeapon ???? 1:1 ??? modern ????:take_quest_item_from_weapon(state, quest, sub, item, num, prob, requiredKind, playerKind) ? `playerKind == requiredKind` ?????? quest_group_take_quest_item;take_quest_item_from_q_weapon(state, ...) ????? weapon item-idx????????? false ??? m_QuestItemTable(1:1 quirk)?
- ? D3.11 ? AddCount weapon-filtered ??:????? `*TAKEQUESTITEMFW` / `*TAKEQUESTITEMFQW` QuestScriptLoader execute ??,?? runtime hook (QUESTMGR->TakeQuestItem) ? caller ???
- 5 ????????:TakeQuestItemFromWeaponMismatchedKindDoesNothing / TakeQuestItemFromWeaponMatchingKindInserts (+1 sub increment + m_QuestItemTable ????) / TakeQuestItemFromQWeaponMismatchedItemDoesNothing / TakeQuestItemFromQWeaponMatchingItemInserts (+1 sub increment) / TakeQuestItemFromWeaponMissingQuestStillInsertsItem (legacy "? item table ? ChangeSubQuestValue" 1:1 quirk, add_count return void-cast)?mxh_quest_group_tests: 51 -> 56 tests PASS, 0 regressions?
- ? commit c197573b: server: D3.12 weapon-filtered TakeQuestItem (legacy TakeQuestItemFromWeapon + TakeQuestItemFromQWeapon)?





### D3.13 weapon-filtered execute dispatch (2026-08-06)

- D3.13 weapon-filtered execute dispatch - ? D3.11/12 ? quest_group_add_count_from_weapon / quest_group_add_count_from_q_weapon / quest_group_take_quest_item_from_weapon / quest_group_take_quest_item_from_q_weapon ????? apply_count_execute / apply_item_execute ? QuestScriptLoader dispatch ??:
  - apply_count_execute ?? player_weapon_kind / player_weapon_item ???? 0 ??;AddCountFQW / AddCountFW ?? case ???? UnsupportedContext,????????????(?? 0 ?? "? context" -> gate ???? -> ???? Applied + changed=false,? legacy "missing context ? no-op" ??)?
  - apply_item_execute ??? player_weapon_kind / player_weapon_item;TakeQuestItemFQW / TakeQuestItemFW ?? case ??????,?? GiveItem/TakeItem/GiveMoney/TakeMoney/TakeExp/TakeSExp/RandomTakeItem ??? UnsupportedContext(?? player inventory / ?? / ?? context,??? quest_group ???)?
- ???? MissingQuestAndWeaponContextAreExplicit ??:`*ADDCOUNTFW 1 2 3` ?? 0 context ??? Applied + changed=false;???? "match context ? changed=true" ? sanity check?
- mxh_quest_execute_tests + mxh_quest_group_tests ????(77/77),0 regressions?
- ? commit e6dfaf1a: server: D3.13 extend apply_count_execute / apply_item_execute with player weapon context (D3.11/12 wiring)?



### D4 UseShopItemUpdateToDB data plane (2026-08-06)

- D4 UseShopItemUpdateToDB - 1:1 port of the SQL builders from legacy [Server]Map/MapDBMsgParser.cpp:676-706 (ShopItemUpdatetimeToDB / ShopItemUpdateUseInfoToDB / ShopItemParamUpdateToDB). Splits the legacy sprintf-then-Query monolith into a pure data plane (this header) + a DbThread::execute_async dispatch layer (orchestrator), so the SQL format is testable in isolation and byte-equal to the legacy sprintf output.
- Adds modern/include/mxh/server/shop_item_update_sql.hpp: STORED_SHOPITEM_UPDATETIME / UPDATEUSEINFO / UPDATEPARAM constants matching the legacy MapDBMsgParser.h STORED_SHOPITEM_* macros (verbatim "dbo.MP_SHOPITEM_*") plus three inline SQL builders: build_shop_item_update_time_sql(chid, item, remain_ms) -> "EXEC dbo.MP_SHOPITEM_Updatetime ..." (3 args), build_shop_item_update_use_info_sql(chid, dbidx, param, remain_ms) -> "EXEC dbo.MP_SHOPITEM_UpdateUseInfo ..." (4 args), build_shop_item_update_param_sql(chid, dbidx, param) -> "EXEC dbo.MP_SHOPITEM_UpdateParam ..." (3 args).
- 14 tests in modern/tests/unit/server/shop_item_update_sql_test.cpp: BasicFormat / ZeroArguments / LargeUnsignedArguments / StoredProcedureNameMatchesLegacy / TwoCommasImpliesThreeArguments / HasExecPrefix (for the time_sql builder); BasicFormat / FourArgumentsThreeCommas / ZeroParamAndRemain (for the use_info_sql builder); BasicFormat / ThreeArgumentsTwoCommas (for the param_sql builder); NoHexPrefix / NoLeadingZeros / Deterministic (cross-builder invariants).
- See commit bbebdb18: server: D4 UseShopItemUpdateToDB data plane - 3 SQL builders + 14 tests.
### D4 CalcShopItemOption data plane (2026-08-06)

- D4 CalcShopItemOption - 1:1 port of legacy CShopItemManager::CalcShopItemOption() from [Server]Map/ShopItemManager.cpp:1246-1691 (the shop-item stat accumulator). Splits the legacy monolithic function into a pure data plane (this commit) + an orchestrator half (legacy m_pPlayer->SetExtraSlotCount / STATSMGR->CalcCharStats / etc. hooks stay in the orchestrator).
- Adds modern/include/mxh/game/shop_item_option.hpp: ShopItemOption struct (38 fields, 124 bytes under pack(1), byte-identical to legacy SHOPITEMOPTION) + ESkinItemCount=6 constant. Static-asserts sizeof(ShopItemOption) == 124.
- Adds modern/include/mxh/server/calc_shop_item_option.hpp: inline legacy enum constants (eSHOP_ITEM_CHARM/HERB/INCANTATION/MAKEUP/DECORATION/SUNDRIES + eIncantation_SkPointRedist/StatePoint/InvenExtend/PyogukExtend/MugongExtend/CharacterSlot/MixUp) + CalcShopItemOptionInfo view struct (24 fields subset of legacy ITEM_INFO) + CalcShopItemOptionEnv base class (override event_rate_active for plustime gates) + CalcShopItemOptionStatus enum (Ok / InvalidIcon / NullStats / ItemInfoMissing) + CalcShopItemOptionSideEffects struct (new_protect_item_idx + 4 locale-bounded expansion flags) + free function calc_shop_item_option(stats, w_idx, b_add, param, info, env, current_protect_item_idx, out_side_effects).
- Adds modern/src/server/calc_shop_item_option.cpp: full implementation of the 4 branches (Incantation / Charm / Herb / Sundries) with the +/- calc = (bAdd ? 1 : -1) accumulator semantics, the clamp-to-zero underflow patterns, the locale-bounded incantation side-effect flags, and the plustime gates (legacy gEventRate / gEventRateFile replaced by env.event_rate_active).
- 57 tests in modern/tests/unit/server/calc_shop_item_option_test.cpp: layout size + Avatar/SkinItem array sizes + 4 early returns + 12 incantation branches (MixUp add/remove/clamp, GenGol/StatePoint, Life/SkillPoint, CheRyuk/ProtectCount add/remove, LimitJob/EquipLevelFree add/remove, InvenExtend side-effect) + 22 charm branches (GenGol/MinChub/Cheryuk/SimMek + Life/Shield/NeagongDamage/WoigongDamage + NaeRyuk/AddSung + LimitJob/LimitGender/LimitLevel/LimitGenGol/LimitMinChub + plustime gating on LimitCheRyuk/LimitSimMek/ItemGrade + non-plustime branches + RangeType/Plus_MugongIdx/Plus_Value/AllPlus_Kind + RangeAttackMin/RangeAttackMax/CriticalPercent/PhyDef + NaeRyukRecover + AttrFire street-stall decoration) + 4 herb branches (Life/Shield/Naeryuk with zero-clamp semantics) + 2 sundries branches (HK_LOCAL bStreetStall).
- See commit 68061fe2: server: D4 CalcShopItemOption data plane - SHOPITEMOPTION struct + 57 tests.
### T2 wire-format coverage 100% (2026-08-06)

- T2 protocol byte-level compat was at 95.1% (77/81 categories) due to a hardcoded kTotalCategories=81 in wire_format_coverage_test.cpp. The legacy Protocol.h has 77 real categories (MP_SERVER=1..MP_FORTWAR=77, MP_MAX=78), so the test was reporting 4 "missing" categories 78-81 that don't exist in the protocol.
- Fix: kTotalCategories = 77 (matches the actual legacy count). Coverage is now 100% (77/77). All 84 wire-format goldens (parse + re-encode byte-equal) cover every real category.
- See commit 8b17de1d: test: wire_format_coverage kTotalCategories 81 -> 77.
### D4 UpdateLogoutToDB data plane (2026-08-06)

- D4 UpdateLogoutToDB - 1:1 port of legacy CShopItemManager::UpdateLogoutToDB() from [Server]Map/ShopItemManager.cpp:1195-1238. Splits the per-row iteration into a pure data plane (this commit) + an orchestrator half (the legacy function calls ShopItemUpdatetimeToDB / g_DB.Query via g_pServerMsg).
- Adds modern/include/mxh/server/update_logout_to_db.hpp: UpdateLogoutToDBInfo struct (sell_price / item_kind / melee_attack_min -- 3-field subset of legacy ITEM_INFO) + UpdateLogoutRowDecision struct (Action enum: Persist / Skip / Drop + new_remaintime + new_last_check) + free function update_logout_to_db_decision(current_remaintime, last_check_time, g_cur_time, info, env). Reuses CalcShopItemOptionEnv for the plustime gate.
- Adds modern/src/server/update_logout_to_db.cpp: 1:1 implementation of the per-row legacy logic -- drop non-PLAYTIME rows, plustime gate (Charm + MeleeAttackMin + env.event_rate_active), clamp checktime to 30000 ms, decrement Remaintime with underflow-clamp-to-zero.
- 12 tests in modern/tests/unit/server/update_logout_to_db_test.cpp: RealTimeItemIsDropped / PlayTimeDecrementsByElapsedTime / RemaintimeUnderflowClampsToZero / PlustimeActiveDecrementsNormally / PlustimeInactiveSkipsRemaintimeUpdate / PlustimeRemaintimeZeroAlwaysDrops / PlustimeDisabledWhenMeleeAttackMinZero / CheckTimeCapClampsToThirtySeconds / ClockSkewYieldsZeroChecktime / PlustimeRemaintimeZeroWithActiveRateDrops + 2 integration tests.
- See commit cb30fba6: server: D4 UpdateLogoutToDB data plane - per-row PLAYTIME decrement + plustime gate + 12 tests.