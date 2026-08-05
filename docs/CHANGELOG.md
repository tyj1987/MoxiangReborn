# CHANGELOG - 历史完成项

> 完整 commit 历史: git log --oneline (1 commit = 1 sub-deliverable)。
> 本文件保留 2026-08 重构前 ROADMAP 的关键历史 [x] 项摘要, 用于回溯。

最近重构: 2026-08-06 - 把老 ROADMAP (434 行) 砍成规划文档 (158 行) + 本 CHANGELOG。

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

