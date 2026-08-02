# Moxian-Reborn 路线图：1:1 完美复现

> **项目代号**：Moxian-Reborn（墨香重生）
> **终极目标**：在现代软硬件环境下 **1:1 完美复现** 2003-2010 韩国 2D MMORPG《墨香》——
> 玩法、数值、协议、资源、UI 全部和原版一致；只在底层换技术栈。
> **本文档替代**：老的 `MODERNIZATION_PLAN.md` / `ROADMAP_2026.md` / `P2-12_DIALOGS_ROADMAP.md` / `AI_TASK_QUEUE.md`。
> **最近一次重置**：2026-07-25（清掉所有历史 session 噪音、重新对齐到终极目标）。
> **最近一次状态刷新**：2026-08-03 — D4.19 ShopItemManager using_items() const accessor (3 new tests: empty after init / all inserted entries returned / delete+release), ShopItemManager 91/91 PASS。

---

## 0. 不可破坏的约束（与原版 1:1 的边界）

这些**绝对不能动**——动了就不是 1:1 复现，是新游戏：

| 约束 | 内容 | 来源 |
|---|---|---|
| 资源格式 | `.bin` / `.pak` / `.bmhm` / `.ttb` / `.chl` / `.chx` / `.chr` / `.mon` / `.bsad` / `.mhs` 二进制结构 | 教程 1-3 + 原码 PackingTool/4DyuchiFileStorage |
| 网络协议 | `[CC]Header/Protocol.h` 96 类 Category + `CommonStruct.h` 网络包结构（含 `#pragma pack(push,1)`） | 教程 2 + 原码 |
| 玩法数值 | 经验曲线、伤害公式、爆率、Boss 刷新、商城、MurimNet PvP | 教程 3 + 原码 |
| HSEL 接口 | `CHSEL::CHSEL / Encode / Decode` 签名必须保持（实现可换） | 客户端依赖 |
| 资源路径 | 客户端启动后从 `MHVerInfo.ver` 读 Distribute 地址；服务器配置 `ServerSet/1/*.txt` | 教程 1 |

**凡是不在这个表里的，都可以动**：编译工具、运行时、UI 控件树实现、数据库驱动、网络底层。

---

## 1. 终极目标的可执行定义

"1:1 完美复现"具体兑现为 3 个可验证目标：

| 目标 | 验证方式 |
|---|---|
| **T1. 资源字节级一致读取** | 任何 `墨香【源码配套资源】/PlayDH/` 下的文件，modern 工具读出来的数据和老客户端读到的一致 |
| **T2. 协议字节级一致收发** | 截一段客户端↔服务端原始网络包，modern 工具的编解码和原码编解码结果完全一致 |
| **T3. 行为 1:1 复现** | 跑同一段操作（登录→进图→打怪→PK→进商城），modern 实现的副作用顺序、数值、UI 状态和原码完全一致 |

3 个 T 全过 = 1:1 复现完成。在此之前说"完成"都是自欺欺人。

---

## 2. 现状盘点（截至 2026-08-01）

| 模块 | 完成度 | 状态 | 验证 |
|---|---|---|---|
| 资源兼容层（`.bin/.pak/.bmhm/.ttb/.chx/.chr/.bsad`） | **100%** | 254 src / 153 test / 2536 tests PASS | T1 部分验证（资源浏览器能解析） |
| UI 控件 1:1 port（dialog + subcontrol） | **115/158 hpp（69 missing，Phase C Batch 1.10 完成 10/10 + Batch 2.1+2.2+2.3+2.4+2.5+2.6+2.7 完成 7 个）** | 老 client 158 dialog / modern 109 hpp（含 subcontrol+core）；Batch 1 已完成 cNumberPadDialog / cPartyWarDialog / cWantedDialog / cMPNoticeDialog / cReviveDialog / cMiniFriendDialog / cGuildInviteDialog / cChatOptionDialog / cStallKindSelectDlg / cDebugDlg / cMPGuageDialog / cMiniNoteDialog / cNoteDialog / cAutoAnswerDlg / cStatusIconDlg / cScreenShotDlg / cOptionDialog / cChannelDialog / cJournalDialog / cHelperSpeechDlg / cCostumeSkinSelectDialog（21 个，331 tests）；Batch 2.1+2.2+2.3+2.4+2.5+2.6+2.7 完成 cMPGuageDialog + cAlertDlg + cChinaAdviceDlg + cLoadingDlg + cKeySettingTipDlg + cIntroReplayDlg + cNameChangeNotifyDlg（7 个，210 tests）；剩 66 个老 dialog 待 1:1 port | T3 1:1 行为锁死 |
| 数据库抽象（MSSQL/SQLite） | **100%** | 两套 adapter + 真实数据 schema | T1 DB 字段级 1:1 |
| 加密（AES-256-GCM + HSEL 接口） | **100%** | OpenSSL EVP，HSEL 签名保留 | T2 协议包加密 1:1 |
| 网络（Asio + IOCP） | **100%** | 跨平台就绪 | T2 协议收发 1:1 |
| 渲染后端（DX11 + IRenderer 抽象） | **100%** | BC1-5 贴图 + Motion Cache | T1 贴图 1:1 |
| 工具链 | **100%** | 12 个工具（资源浏览/打包/GM/地图编辑/补丁/协议文档 + MoxianClientE2E） | 工具全部 build 通过 + E2E 集成 ctest |
| 客户端运行时 | **Phase A + B.2 完成** | CMainGame 1:1 port + 9 state（3 个真接 mxh::net：CLoginState/CCharSelectState/CInGameState） | Phase B.2.1~2.5 E2E 全 PASS（3 state + 50 client test） |
| 服务端运行时 | **3 server E2E 全 PASS** | Phase B: LoginServer + AgentServer + MapServer 3 进程 + Python 模拟 + C++ 状态机双重 E2E | Phase B ✅ |
| 玩法数值 baseline | **D6.1 + T2 item wire 锁死** | 7 OBJECTKIND / 6 MonsterAI / 14B MonsterTotalInfo / 22B ItemBase / 124 槽、2728B ItemTotalInfo / 3775B GameInAck hero payload / 4 ItemEffect 公式 / 3 default MonsterTemplate 全部 1:1 锁定 | T2/T3 wire 与数值回归 baseline |
| D6.2 experience reward | **partial** | CharacterExpPoint.bin + PlayerxMonsterPoint.bin loaders; Distributer reward bands and ability-exp rules | 30 focused tests |
| **SkillList.bin 解析** | ** D1.2 reinforcement | MugongManager.clear() fix (idx_ now resets; pre-fix left stale indices causing vector subscript OOB on find after clear()). 14 new 1:1 lock tests for MugongManager (OwnerIdDefaultZero, SetOwnerIdRoundTrip, ClearResetsSlotsAndIndex regression, RemoveMissingReturnsFalse, FindConstReturnsSameValue, SlotsVectorReturnsAllInInsertionOrder, TotalSpOnEmptyManagerIsZero, MaxSlotConstantIsLegacyHundred, UpdatePreservesPositionInVector) and SkillManager (ReRegisterSameSkillIdxUpdatesInPlace, SkillsVectorPreservesInsertionOrder, EmptyManagerReturnsZeroForAllAccessors, FindReturnsNullForUnknownSkill, Level1AccessorsPullArrayIndexZero). 21 tests total (was 7).
D1.1+D1.2+D1.3 全完成** | SkillInfo 扩到 1:1 legacy SKILLINFO（60+ 字段含 7×[12] 数组）；SkillListParser 解码 MHFile packed-text；SkillManager::init_from_bin() 装真 bin（1817 entries）；端到端 test 锁首行 | 11 SkillListParser test + 13 SkillManager test PASS |
| **BattleFactory 1:1** | **D2 完成** | 14 compute_* 函数（critical/decisive/player-phy/player-attr/player-exp/player-point/phy-defence/received-dmg/monster-phy/monster-attr/titan-phy/titan-attr）+ 13 新 1:1 unit test（attack_calc legacy_* 系列保持 API 兼容） | T2 数值公式回归
| **MurimNet 1:1 wire** | **D5 完成** | Channel/ChannelManager + PlayRoom/PlayRoomManager + MNPlayer/MNPlayerManager + 60 协议代码 (MP_PROTOCOL_MURIMNET) + 9 wire serializer (ChannelList/PlayRoomList/PlayerList/单 item ×3/TeamChange/Chat) + 7 short wire (Byte/Word/Word2/Dword/Dword2-4) + runtime.broadcast_chat sink + MurimNetCrypt (CCrypt 1:1 端口, wrap HselStream) | T2 wire 字节 1:1 锁 |
| **Wire-format 协议覆盖** | **77/81 distinct = 95.1%** | 84 goldens 在 modern/tests/unit/server/golden/；LoginHandler wire 字节层覆盖：81 个 legacy MP_* cat 全部经 M-series 锁定（cat 78-81 是 modern-only 扩展，legacy 无 traffic）；M86-M93 batch 完成 cat=59/60/62/66/74/75/76/77 共 9 个新增 goldens | T2 wire 字节 1:1 锁
| HSEL 硬件狗绕过 | **80%** | stub 已写，但未跑通真实 `.bin` | 待 E2E |
| HackShield 绕过 | **0%** | 卡 R-2 | 阻塞 |
| SQL Server 集成 | **60%** | schema + restore 脚本，但没真启服 | 待 E2E |

### 2.1 2026-07-26 实时验收快照

- Debug 构建通过；`ctest -C Debug -j 8`：**4016/4016 PASS，10 个环境依赖跳过，0 FAIL**。
- T1 已锁定 PlayDH 下 15 个真实 `.bin` 文件的 SHA-256 与字节大小；资源 smoke 测试优先 PlayDH，禁止静默使用 deploy 变体。
- T2/T3 验证工具已按 Login/Agent/Map 三端口路由，且按 `[length:2][MSGBASE:8][payload]` 修正字节偏移；真实老服 15 包捕获与五段行为 diff 仍待完成。
- 当前状态不是阶段 E 完成：SQL Server 真连接、真实老服协议回放、完整服务端行为 1:1 仍是后续门禁。

---

## 3. 新的阶段路线（**砍掉老 12 阶段噪音**）

老 12 阶段路线图把"现代化"当成目标，**不对**。现代化只是手段，**1:1 复现**才是目标。重新按"距离能玩 1:1 复刻版还有多远"来拆：

### Phase A —— 客户端能跑起来（1-2 周）
**目标**：modern 客户端能启动、能进登录界面、能读真实资源。

- [ ] A1 启动 modern `MoxianClient.exe` 替代 MHClient-Connect.exe
- [ ] A2 接 DX11 renderer 真实输出（现在是 no-op）
- [ ] A3 接 network 层：登录→选服→进图
- [ ] A4 接 main loop：input/tick/render 三线程
- [ ] A5 接 cWindow 主框架：MHClient.cpp → MoxianClient.cpp 1:1 port
- [ ] 验证：截图和原版登录界面像素级一致（允许字体/分辨率缩放差异）

### Phase B —— 服务端能跑起来（2-3 周）
**目标**：3 个 server（Distribute/Agent/Map）能 listen，能用 modern 客户端登录进游戏。

- [ ] B1 MoxianDistributeServer 启动 + listen 400/600
- [ ] B2 MoxianAgentServer 启动 + 接 Distribute
- [ ] B3 MoxianMapServer 启动 + 接 Agent
- [ ] B4 Player/AISystem/Map 1:1 port（核心 5 万行）
- [ ] B5 DBThread + IDbAdapter 真接 MSSQL
- [x] B6.1 HSEL YHLibrary ABI correction | 79 crypto tests
 CHSEL now matches the actual self-contained YHLibrary public header: non-virtual destructor, exactly two const virtual getters, protected version/type fields, and non-virtual CHSEL_STREAM Encrypt/Decrypt/CRC/key methods. R-1 remains open pending real legacy .bin E2E.
- [ ] B6 HSEL stub 完整化（已 80%，补 20%）
- [ ] 验证：登录→选服→进图→看到角色（空场景）

### Phase C —— 客户端 UI 1:1 收口（5-7 session 多批，**已开 Batch 1**）
**目标**：把老 client 158 个 dialog 全部 1:1 port + test。Modern 当前 102 个 hpp（含 subcontrol/core），缺 **87 个老 dialog**。

- [x] **C-Batch-1.1** `cNumberPadDialog` — login PIN 4-digit entry（commit `5790051`）
- [x] **C-Batch-1.2** `cPartyWarDialog` — 战盟战对阵窗口（Show/Hide/NoChange/SetMember/Add/Remove/SetLock/SetUnLock/SetTime，20 tests）
- [x] **C-Batch-1.3** `cWantedDialog` — 通缉列表（SetInfo/AddInfo/InitWanted，18 tests）
- [x] **C-Batch-1.4** `cMPNoticeDialog` — MP 房间提示（Linking + 2 cTextArea，11 tests）
- [x] **C-Batch-1.5** `cReviveDialog` — 复活选项（3 cButton + 城战分支，13 tests）
- [x] **C-Batch-1.6** `cMiniFriendDialog` — 好友添加（4 child + SetActive/SetName，13 tests）
- [x] **C-Batch-1.7** `cGuildInviteDialog` — 帮派邀请（1 cTextArea + SetInfo 双分支 chatmsg 45/1370，14 tests）
- [x] **C-Batch-1.8** `cChatOptionDialog` — 聊天频道开关（12 cCheckBox + CHATMGR->GetOption/SaveUserOption 回调注入，25 tests）
- [x] **C-Batch-1.9** `cStallKindSelectDlg` — 摆摊类型选择（3 cButton + STREETSTALLMGR 回调注入，21 tests）
- [x] **C-Batch-1.10** `cDebugDlg` — 调试类型过滤（6 flags + DebugMsgParser variadic 路由，23 tests）
- [x] **C-Batch-2.32** `cCharChangeDlg` -- 1:1 port of the character-change dialog (used by the 2nd character-change item). Preserves preprocessed WindowIDs 1436-1450 (CHA_CHANGEDLG/NAME/CharMake/CharCancel/SexType/HairType/FaceType/SexType1/SexType2/Height/Width), kHairTypeMax=4/kFaceTypeMax=4 wrap range, chat-message IDs 1180/1181/1182/1183 (gender + hair/face format), kItemTable{Inventory=0,Pyoguk=1,MunpaWarehouse=2,Shop=3} re-enable on SetActive(false) per legacy SetDisableDialog(FALSE,...) 1:1 quirk, the height/width guage formula (h - 0.9) * 5 = SetCurRate, the Process() scale computation (0.9 + rate * 0.2), SetHeroScale(w, h, w) VECTOR3 propagation, and the 1:1 quirk that ChangeSexType is gated by m_bShapeChange but ChangeHairType/ChangeFaceType are NOT (legacy asym). 8 host-injected callbacks (ChatText/setItemTableDisabled/EndObjectState/SetHeroName/SetHeroCharChangeInfo/TriggerCharacterPartChange/SetHeroScale/SendCharacterChange) replace CHATMGR/ITEMMGR/OBJECTSTATEMGR/HERO/APPEARANCEMGR/NETWORK legacy globals. Auxiliary `cGuageBar` 1:1 port (cGuagen subclass) provides the rate-based slider with `InitGuageBar(interval, vertical) / Add(button) / InitValue / SetCurRate / GetCurRate / SetGuageLock / SetAbsXY / ActionEvent (drag stubbed) / Render (delegates to cGuagen + bar button)`. 35 charchangedlg + 10 cGuageBar tests lock IDs/constants, defaults, SetControlsForTest, Linking via findWindowById, SetCharacterInfo Male/Female + null chat + null hero name + null height/width + Apply rate, ChangeSexType toggle + shape-change gate, ChangeHairType/ChangeFaceType wrap (no shape-change gate per legacy), Reset(true) re-publishes current vs Reset(false) restores backup + triggerPartChange, CharacterChangeSyn invokes send + deactivates, SetActive true enables shape controls / SetActive(true)+shapeChange disables, SetActive(false) calls SetItemTableDisabled(false,...) for all 4 tables, Process() applies rate when changed + no-op on shape-change + no-op on null height/width, cGuageBar Initialize round-trip + SetCurRate + SetInterval repositioning + Add(btn) shrinks interval + SetGuageLock + ActionEvent no-op when inactive/locked.
- [x] **C-Batch-2.31** `cPartyCreateDlg` -- 1:1 port of the party creation dialog. Preserves preprocessed WindowIDs 391-399 (PA_CREATE_THEME/MINLEVEL/MAXLEVEL/PUBLIC/PRIVATE/DIVISION/MEMBERNUM/OK/CANCEL), MAX_PARTY_NAME=15 theme length check (chat 1742 on overflow), 1/99 default min/max level, public/private mutual-exclusion OnActionEvent handler, ePartyOpt_Random vs ePartyOpt_Damage division parsing via RESRCMGR->GetMsg(483/484), HERO->GetPartyIdx() != 0 guard, NETWORK->Send(MP_PARTY/MP_PARTY_CREATE_SYN) dispatch via host callback, PARTYMGR->SetIsProcessing state propagation, and SetActive(false) cascading InitOption defaults. Legacy globals (CHATMGR, RESRCMGR, HERO, NETWORK, PARTYMGR, PARTY_ADDOPTION, ePartyOpt_*) are host-injected via ChatMessageFn / ResourceMsgFn / HasPartyFn / CreateSynFn callbacks. 22 tests lock IDs/constants, defaults, SetControlsForTest, InitOption defaults + null tolerance, OnActionEvent public/private/cancel/non-click/unknown, CreatePartySyn dispatches options + long-theme rejection + damage option + unknown division + hasParty guard + send-fail guard + null controls + OK triggers and OK no-deactivate on fail, SetActive false reapplies defaults, Linking resolves all 9 controls + resets processing. -- 1:1 port of the quest tabbed container. Preserves the cTabDialog inheritance, OPT_QUESTDLGICON=78 main-bar callback contract (push-icon + alram-false on activate), RegisterSubDialog routing via dynamic_cast for cWantedDialog/cQuestDialog/cJournalDialog and cPushupButton, curIdx1/curIdx2 auto-increment tab-btn and sheet indices, and the full set of forwarding methods (JournalItemAdd, CompleteQuestDelete, ProcessQuestAdd/Delete, QuestItemAdd/Delete/Update, GetSelectedQuestID, CloseMsgBox, GiveupQuestDelete, QuestListView, JournalView, UpdateSubQuestData) with null-sub-dialog tolerance. 18 tests lock inheritance, ID constants, defaults, sub-dialog injection, RegisterSubDialog routing per type + pushup button + null tolerance, SetActive push-icon + alram + null-callback tolerance, GetSelectedQuestID with/without selection, and full delegation-method null-sub-dialog coverage. -- 1:1 port of the GT battle list panel. Preserves preprocessed WindowID 1390 (GDT_BATTLELIST), m_bPlayOff FALSE default, m_BattleCount/m_nPreSelectedItem = 0/-1 initial state, AddBattleInfo empty-guildName guard, DeleteBattleInfo/DeleteAllBattleInfo/DeleteAddBattleInfo cleanup, RefreshBattleList chat-template formatting (messages 953/954/955 for play-off/group/guild-pair), HandleMouseAction row-click selection capture with active gating + out-of-range guard, EnterBattleOnObserver observer join via selected row, and SetActive(false) cascade DeleteAllBattleInfo. Legacy globals (CHATMGR, HERO, NETWORK, PETMGR, GAMEIN) are host-injected via ChatTextFn / InsertRowFn / ObserverJoinFn callbacks. 22 tests lock IDs/constants, defaults, SetControlsForTest, Linking resolves + state reset, AddBattleInfo empty-name guard, AddBattleInfo stores all, DeleteBattleInfo removes/decrements/missing, DeleteAllBattleInfo / DeleteAddBattleInfo / SetActive false cascade, SetActive true preserves, RefreshBattleList populates + null ctrl tolerance + test-battles path, HandleMouseAction inactive/active/out-of-range/non-click flags, EnterBattleOnObserver uses selected row + null callback tolerance, GroupInitial legacy ABCD? mapping.
- [x] **C-Batch-2.28** `cGuildPlusTimeDialog` -- 1:1 port of the guild plustime purchase dialog. Preserves preprocessed WindowIDs 364/513/516/517 (CMI_CLOSEBTN/GD_PLUSTIMESTART/GD_POINT/GD_PLUSTIMELIST), m_CurrentSelectedItem = -1 initial state, cListDialog::SetShowSelect on Linking, SetGuildPointText comma-thousands formatting, LoadPlustimeList with chat-template formatting (messages 1377-1380 for eGPT_SuRyun/MuGong/Exp/DamageUp), OnActionEvent start/close button dispatch with WE_BTNCLICK gating, and ActionEvent mouse-hit selection capture. Legacy globals (GUILDMGR, CHATMGR) are host-injected via UseGuildPointFn / ChatTextFn / PlustimeCountFn / PlustimeEntryFn callbacks. 18 tests lock IDs/constants, defaults, SetControlsForTest, SetGuildPointText thousands formatting (0/15/1234/1234567), LoadPlustimeList 4 kinds + unknown skip + null tolerance, Linking resolves + state reset + showSelect, HandleMouseAction hit/miss/null/flags, OnActionEvent start dispatch + close deactivates + non-click ignore + unknown id ignore, SetActive propagation.
- [x] **C-Batch-2.27** `cGTScoreInfoDialog` -- 1:1 port of the GT battle score panel. Preserves preprocessed WindowIDs 1326/1394-1399 (GDT_GTIME/GDT_OUTBTN/GDT_GUILDNAME1-2/GDT_GUILDMEMBER1-2), 2-team score slots, 120000 ms entrance countdown, m_bStart battle flag, ShowOutBtn button activation, SetBattleInfo name/score/time routing, and the Process pre/post-start tick deduction + "%02u:%02u" time text. Legacy globals (gTickTime, cStatic::SetStaticText) are host-injected via TickProvider + TextWriter callbacks. 21 tests lock IDs/constants, defaults, SetControlsForTest, Linking + state reset, SetBattleInfo updates, null-control tolerance, team score range checks, StartBattle/EndBattle flag + FightTime reset, ShowOutBtn toggle, Process pre/post-start tick deduction + clamping, custom TickProvider / TextWriter routing, zero-tick tolerance, and accessor coverage.
- [x] **C-Batch-2.26** `cGuildRankDialog` -- 1:1 port of guild-rank assignment UI. Preserves preprocessed WindowIDs 546-551, Dan/Guild mode switching at MAX_GUILD_LEVEL=5, selected-member validation, self/empty rejection message 714, member-name format message 718, and control activation order. 20 tests lock IDs/modes, linking, type/null guards, transitions, idempotence, selection validation, callbacks, and name formatting.
- [x] **C-Batch-2.25** `cMNCreateDialog` -- exact 1:1 port of the legacy MurimNet create-dialog shell. Constructor, destructor, Linking, and OnActionEvent remain intentionally empty. 12 tests lock inheritance, defaults, geometry, state preservation, child ownership, argument tolerance, idempotence, and polymorphic destruction.
- [x] **C-Batch-2.24** `cMNFrontDialog` -- exact 1:1 port of the legacy MurimNet front-dialog shell. Constructor, destructor, Linking, and OnActionEvent remain intentionally empty. 12 tests lock inheritance, defaults, geometry, state preservation, child ownership, argument tolerance, idempotence, and polymorphic destruction.
- [x] **C-Batch-2.23** `cMNJoinDialog` -- exact 1:1 port of the legacy MurimNet join-dialog shell. The constructor, destructor, Linking, and OnActionEvent bodies remain intentionally empty with no added room/password behavior. 12 tests lock inheritance, defaults, geometry, active/disable state, child ownership, argument tolerance, idempotence, polymorphic destruction, and the complete no-side-effect contract.
- [x] **C-Batch-2.22** `cPartyMemberDlg` -- 1:1 port of the six-slot party member panel. Preserves preprocessed WindowID bases 423/429/435/441, requested-vs-actual active state, member visibility gating, online/offline name colors, life/naeryuk gauges, level text, selected-member click routing, push state, and both legacy Render layouts relative to cPartyBtnDlg. 29 tests lock construction, linking, IDs/colors, active-state memory, member refresh branches, null/type guards, clicks, push state, and exact coordinates.
- [x] **C-Batch-2.21** `cPetStateDlg` -- 1:1 port of the legacy pet-state tab dialog. Preserves exact WindowIDs 1466-1484, WT_PUSHUPBUTTON/WT_DIALOG tab routing, top-level and two-sheet Linking, four PETMGR action routes through host callbacks, and the intentionally empty SetBtnClick body. 24 tests lock inheritance, IDs, construction, ownership routing, cross-sheet linking, type/null guards, all action branches, callback isolation, and no-op behavior.
- [x] **C-Batch-2.20** `cMonsterGuageDlg` -- 1:1 port of the monster/character/NPC gauge dialog. Preserves exact WindowIDs 985-996, four gauge modes, life/shield clamp and legacy integer effect-time formula, name/color forwarding, guild/union chat formatting hooks, mode activation transitions, pet-mode behavior, and character Render relative-Y adjustment. 20 tests lock linking, layout, gauge values, clamp/effect rules, formatting, mode transitions, object type, render placement, and null guards.
- [x] **C-Batch-2.19** `cPartyBtnDlg` -- 1:1 port of the party action-panel state machine. Preserves exact WindowIDs 448-456, default option visibility, non-party/master/member branches, ICONCLR usable/disabled colors, ShowOption visibility transitions, and Render member-button offsets of 100/20 pixels. Adds the legacy SetImageRGB compatibility surface to cWindow for button state colors. 16 tests lock inheritance, IDs/colors, linking, all role branches, option toggles, render placement, and missing-control guards.
- [x] **C-Batch-2.18** `cServerListDialog` -- 1:1 port of the legacy server picker. Preserves the 88-byte SEVERLIST layout, exact WindowIDs 1066-1069, WE_ROWCLICK/WE_ROWDBLCLICK values, two-column row loading, available/unavailable colors, selected-row transitions, double-click connect dispatch, and destructor list cleanup. GAMERESRCMNGR/TITLE globals are host-injected; invalid null/negative inputs are guarded. 26 tests lock layout, defaults, linking, loading, bounded names, colors, selection transitions, event priority, callback behavior, and cleanup.
- [x] **C-Batch-2.17** `cGridDialog` -- 1:1 port of the legacy pushup-button grid container. Init delegates base geometry/image/id state, records the source cell pointer and count, clones pushed/passive state into dialog-owned cPushupButton children, and preserves the legacy re-init append behavior. Documented quirks replace unsafe object memcpy with normal construction plus member-wise copy, retain caller ownership of the source array, and omit the removed WT_GRIDDIALOG type tag. 18 tests lock inheritance, construction/destruction, base attributes, source metadata, zero/null handling, child count/type/state cloning, caller ownership, default id, removed type tag, and re-init append semantics.
- [x] **C-Batch-2.16** `cJackpotDialog` -- 1:1 port of documented-but-commented legacy cJackpotDialog (lottery jackpot digit roll animation). Constants (NUMIMAGE_W=8/H=14, BASIC_ANI_TIMELENGTH=2000, BETWEEN_ANI_TIMELENGTH=500, NUM_CHANGE_TIMELENGTH=100, DEFAULT_IMAGE=99, NUM_COUNT=10, CIPHER_NUM=9, MONEY_PER_MON=1), StNumImage + StCipherNum structs, Init/Release/ConvertCipherNum/IsNumChanged/InitForAni/InitForSequenceAni/DoAni/DoSequenceAni/Process all ported. InitNumImage/ReleaseNumImage/Linking/SetNumImagePos/Render are 1:1 quirk stubs (require cImage/cButton/GPU layer). 34 tests lock the numeric contract (constants, struct defaults, ctor invariants, ConvertCipherNum slot assignment + MaxCipher + clamp at >CIPHER_NUM, IsNumChanged increase/decrease flag, InitForAni snapshot, InitForSequenceAni gating, DoAni digit roll mod-10 + settle phase + completion, DoSequenceAni snap when no-anim + roll-and-clamp, Process full loop, 1:1 quirk stubs).
- [x] **C-Batch-2.1** `cMPGuageDialog` — 事件地图定时 + 经验进度条（1 CObjectGuagen + 3 cStatic + CObjectGuagen/CHATMGR 回调注入，29 tests）
  - 14 test: constants, Linking, InitProtectionStr, InsertStr (mask+digit), 4-char cap, OnActionEvent 派发, backspace, WE_CLOSEWINDOW, nGate=3 短路, nGate!=3 允许, unknown button id 容忍, GetProtectionStr 返回 raw digits, SetActive forwarder, buffer cap constant
- [ ] **C-Batch-1.2 ~ 1.10** NumberPadDialog 之后: MiniNoteDialog, NoteDialog, ScreenShotDlg, StatusIconDlg, OptionDialog, ChannelDialog, JournalDialog, AutoAnswerDlg, HelperSpeechDlg, CostumeSkinSelectDialog, BigMapDlg, MiniMapDlg, ChatDialog, CharacterDialog, CharMakeDialog, CharChangeDlg, MonsterGuageDlg, QuestDialog, QuestTotalDialog, MoveDialog, PyoGukDialog, QuickDialog, FriendDialog, MixDialog, DissolveDlg, DissolutionDialog, BuyRegDialog, DealDialog, ExchangeDialog, MainBarDialog, MenuSlotDialog, ...
- [ ] **C-Batch-2 ~ 5** 主 UI 框架 (MainBarDialog, MenuSlotDialog) + 帮派/商城/泰坦/宠物/战盟/摆摊/强化 等复杂 dialog 100+ 个
- [ ] **C-Tier-3** 9 个等 Phase B service 真接的 dialog（QuestDialog/QuestTotalDialog/DealDialog 等）
- [ ] **C-Tier-4/5** NPC script + network dialog（CharMake/CharChange/CharacterDialog/InventoryEx 等）

**评估**：单 session 平均 port 2-3 个 dialog（hpp+cpp+test+build verify+commit）。87 个 total → **5-7 session**。

**已完成 13/87**（cNumberPadDialog 2026-07-26 commit `5790051`；cPartyWarDialog 2026-07-31；cWantedDialog 2026-07-31；cMPNoticeDialog 2026-07-31；cReviveDialog 2026-07-31；cMiniFriendDialog 2026-07-31；cGuildInviteDialog 2026-07-31；cChatOptionDialog 2026-07-31；cStallKindSelectDlg 2026-07-31；cDebugDlg 2026-07-31；cMPGuageDialog 2026-07-31）。

- [ ] 验证：每个 dialog 用 unit test 锁死 1:1 行为

### Phase D —— 玩法/数值 1:1 锁定（4-6 周）
**目标**：原版所有玩法可玩且数值一致。

- [ ] D1 技能系统 SkillManager 1:1（双版本，client/server）
- [x] D2 战斗系统 BattleFactory_Default 1:1
- [ ] D3 任务系统 QuestManager + QuestExecute_* 1:1
- [x] D3.1 QuestExecute_* script parser | 9 tests lock the 27 commands registered by the Korean legacy QuestScriptLoader, exact eQuestExecute numeric values, quest/subquest context, DWORD argument order, tab/CRLF tokenization, and RANDOMTAKEITEM WORD triples; unsupported STARTQUEST/MINUSCOUNT/CHANGESUBATTR remain rejected like the default loader.
- [x] D3.2 QuestExecute_Count state dispatch | 3 tests lock ADDCOUNT, ADDCOUNTLEVELGAP, and ADDCOUNTMONLEVEL forwarding into QuestGroupState with legacy threshold order, clamping, missing-quest rejection, and explicit UnsupportedContext for weapon-filtered variants until player equipment state is available.
- [x] D3.3 QuestExecute_Quest/Time state dispatch | 5 QuestGroup helper tests + 4 dispatch tests lock StartSub/EndSub active-slot model, EndSub legacy subQuestFlag bit `1 << (31-idx)` plus data/time reset, EndQuest repeat vs final complete, RegistCheckTime day/hour/minute persistence, SaveLoginPoint 2000 offset clamp, and explicit MissingSubquest status; MapChange/RegenMonster/ChangeStage/ChangeSubAttr deliberately return UnsupportedContext.
- [ ] D4 商城 / 物品 / 仓库 / 邮件 / 帮派 / 队伍 1:1
- [x] D4.1 Economy 1:1 lock | 14 tests lock legacy [Server]Map/Economy.h/.cpp
 RegistBaseEconomy matches the packed 16-byte REGIST_BASEECONOMY layout; SetRegistEconomy copies all fields; SetBaseValue safely replaces opaque ITEM_INFO bytes; CalculBase remains the legacy no-op; init/release are idempotent.
- [x] D4.2 MoneyManager MAXMONEY correction + 11 tests lock purse boundaries
 Corrects the fabricated 21,000,000 cap to legacy CommonStruct.h MAXMONEY=0xFFFFFFFF; tests lock zero operations, uint32 saturation without overflow, failed-spend immutability, exact-balance spend, unknown-player lookup, logging, and player isolation.
- [x] D4.3 Purse DWORD/MAXMONEY 1:1 lock | 23 tests lock legacy [Server]Map/Purse.cpp
 Restores MONEYTYPE to 32-bit DWORD and PURSE_UNLIMITED to 0xFFFFFFFF; Addition saturates without wrap, query predicates retain legacy pre-init behavior, Release preserves balances, and corrupt money-above-max state rejects additions.
- [x] D4.4 NoteManager active-inbox limit + 14 tests lock mail state transitions
 Enforces legacy LIMIT_DCOUNT=10 per recipient, lets acknowledgement release capacity, rejects repeated acknowledgement, keeps recipients independent, avoids ID consumption on invalid recipients, and isolates generated ID sequences per manager instance.
- [x] D4.5 ExchangeRoom 1:1 state/money/layout lock | 28 tests
 Restores the 24-byte ITEMBASE exchange proxy layout, global unlock reset of both lock/accept flags, locked-slot add/delete rejection, QuickPosition rejection, and InputMoney replace+available-balance clamp semantics returning the actual stored DWORD.
- [x] D4.6 StreetStall cell/layout/price 1:1 lock | 37 tests
 Restores 24-byte ITEMBASE and 40-byte sCELLINFO layouts, requires owner+stall kind before filling, preserves Sell first-empty behavior, limits Buy slots to 5 with replacement, computes Buy total as money*volume, blocks locked/empty setters, and deduplicates guests.
- [x] D4.7 PyoGukManager page/money/access 1:1 lock | 26 tests
- Locks 5 pages x 30 cells, DWORD money, per-level buy price/max money, first-buy initialization, Japan/HK/Taiwan extra-page gates, warehouse-item/NPC access, deposit NACK clipping, and legacy zero-withdraw no-response behavior.
- [x] D4.8 ITEM_TOTALINFO + GameInAck current-layout 1:1 lock | 9 new tests
 Restores the default legacy 124-slot, 2728-byte item block including pet/titan wear and titan shop slots; centralizes the 3775-byte SEND_HERO_TOTALINFO offsets; embeds the item block directly in GameInAck; rejects the obsolete pre-Titan 3000-byte payload to prevent truncation and server-side stack overwrite.
- [x] D4.9 ItemSlot + InventorySlot core state machine 1:1 lock | 40 tests
 Restores the packed 8-byte SLOTINFO layout and exact UPDATE_BIT / ERROR_ITEM / SLOT_STATE values; absolute-position windows; insert/update/delete validation order; lock-omit, password and DB-index checks; the ignored UB_DBIDX flag; partial writes before duplicate-limit failure; shop-range delete lock bypass; stale ItemParam after delete; locked/reserved empty-slot counting; GetEmptyCell null-output zero-count quirk; and Japan/HK/Taiwan two-given-tab extra-slot sizing.
- [x] D4.10 Pyoguk runtime capacity 150-slot correction | 2 new tests
 Removes the fabricated 80-cell warehouse arrays from PlayerState and ItemContainer, uses shared SLOT_PYOGUK_NUM=5*30 everywhere, proves the fifth page last cell is usable, and locks rejection only after all 150 cells are filled; Player inventory/equipment bounds now also derive from shared legacy constants.
- [x] D4.11 ItemManager MAXMONEY clipping correction | 2 new tests
- [x] D4.12 ShopItemManager 1:1 lock | 25 new tests
- [x] D4.13 ShopItemManager tick timer + expire scan | 13 new tests
- [x] D4.14 ShopItemManager UsedShopItem data plane | 11 new tests
- [x] D4.15 ShopItemManager SEND_SHOPITEM_USEDINFO wire bytes | 7 new tests
- [x] D4.16 ShopItemManager SEND_SHOPITEM_INFO + SEND_MOVEDATA_INFO wire | 10 new tests
- [x] D4.17 ShopItemManager AddMovePoint ValidCount gate | 11 new tests
- [x] D4.18 ShopItemManager legacy data-plane helpers (GetUsingItemInfo / AddShopItemUse / ReNameMovePoint / ReleseMovePoint / GetSavedMPNum) | 11 new tests
- [x] D4.19 ShopItemManager using_items() const accessor 1:1 lock | 3 new tests
 Locks the const accessor used by legacy SendShopItemUseInfo serialization hook. using_items() returns the underlying unordered_map<uint64_t, UsingShopItemEntry>& by const reference (1:1 with legacy m_UsingItemTable.GetData()). Tests verify: empty after init, returns all inserted entries with correct key->entry mapping (ItemIdx->UsingShopItemEntry), reflects delete_using_item (count drops by 1) and release() (empty). ShopItemManager 91/91 PASS.
 Locks the remaining pure-data legacy CShopItemManager query/mutate helpers without touching ITEMMGR/AbilityManager/ShopItemOption. find_using_item_by_icon_idx_mutable(icon_idx) returns a non-const UsingShopItemEntry* (legacy GetUsingItemInfo(ItemIdx)) or nullptr. add_shop_item_use(shop_item, now_ms) inserts a pre-built ShopItemBase keyed by wIconIdx, sets LastCheckTime=now_ms (legacy gCurTime), rejects zero icon and duplicate keys -- preserves every ShopItemBase field byte-identically. rename_move_point(db_idx, new_name) copies up to MAX_SAVEDMOVE_NAME-1=20 bytes into the row Name field with NUL terminator (legacy ReNameMovePoint). release_move_points() clears only the move-point table -- the using-items table and dup counters are untouched (legacy ReleseMovePoint typo preserved in comment). get_saved_mp_num() is an alias for move_point_count (legacy m_MovePointTable.GetDataNum). ShopItemManager 88/88 PASS. The full legacy UsedShopItem/UseShopItem/CalcShopItemOption hooks (SkillPoint / StatePoint / StatePoint_30 ShopItemOption) remain in a follow-up commit.
 Locks legacy CShopItemManager::AddMovePoint capacity gate: when any of the 4 MemoryMove incantation items (eIncantation_MemoryMoveExtend=55365 / eIncantation_MemoryMoveExtend7=55390 / eIncantation_MemoryMove2=55371 / eIncantation_MemoryMoveExtend30=58010) is currently in the using-items table, capacity doubles from MAX_MOVEDATA_PER_PAGE=10 to MAX_MOVEDATA_PER_PAGE*MAX_MOVEPOINT_PAGE=20. add_move_point now rejects inserts that would push past the dynamic capacity (>= gate matches legacy). move_point_capacity() and is_memory_move_extend_icon() are public for testability and follow-on dispatch logic.
 Locks the remaining two legacy ShopItemManager wire functions 1:1 with CommonStruct.h. SEND_SHOPITEM_INFO: 8-byte MsgHeader + 2-byte ItemCount + 150*22-byte ItemBase = 3310 bytes max (legacy SLOT_SHOPITEM_NUM=TAB_SHOPITEM_NUM* TABCELL_SHOPITEM_NUM=5*30). SEND_MOVEDATA_INFO: 8 MsgHeader + 1 bInited + 2 Count + 20*31 MoveData = 631 bytes (legacy MAX_MOVEDATA_PERPAGE*MAX_MOVEPOINT_PAGE=10*2). serialize_move_points(category, protocol, b_inited) and serialize_move_points_headerless(b_inited) match legacy GetSize() trim exactly. Together with D4.15 this completes the ShopItemManager wire layer.
 Locks legacy CShopItemManager::SendShopItemUseInfo wire format 1:1 with CommonStruct.h SEND_SHOPITEM_USEDINFO: 8-byte MsgHeader + 2-byte ItemCount + 100*34-byte ShopItemBase = 3410 max bytes, trimmed exactly as legacy GetSize() does. serialize_using_items(category, protocol) builds the byte vector with category/protocol filled in; serialize_using_items_headerless() leaves them zeroed. Tests verify struct size, helper size formula, empty/1/3 item trim, headerless zero-fill, and post-release empty frame.
 Locks legacy CShopItemManager::UsedShopItem data plane without the ITEMMGR/AbilityManager/ShopItemOption coupling: used_shop_item(item_base, param, begin_time, remain_ms, now_ms) inserts a UsingShopItemEntry keyed by item_base.wIconIdx (1:1 with legacy m_UsingItemTable.Add(ShopItem, wIconIdx)), rejects zero icon and duplicate icons, sets LastCheckTime=now_ms (legacy gCurTime). Adds has_using_item_by_icon_idx/find_using_item_by_icon_idx O(1) lookups (legacy GetData) and is_using_item_active(now_ms) which is true iff (LastCheckTime+Remaintime) > now_ms. Delete-then-reinsert replaces the row (matches legacy m_UsingItemTable.Remove + Add sequence). ShopItemAllUseInfoUpdateToDB and the eIncantation_SkPointRedist/eIncantation_StatePoint ShopItemOption adjustments remain a follow-up commit.
 Locks the legacy CheckEndTime timing gate (CHECK_INTERVAL_MS=30000, UPDATE_INTERVAL_MS=600000) without the player/item-manager coupling: tick(delta_ms) increments m_Checktime and m_Updatetime using DWORD arithmetic, returns true when m_Updatetime wraps past 600000 (legacy DB-flush trigger), check_due() exposes the 30s throttle, collect_expired(now_ms, out) appends item indices whose (LastCheckTime + Remaintime) deadline has passed, tick_and_collect_expired(delta_ms, now_ms, out) is the convenience wrapper that gates on check_due(). UseShopItem/CheckEndTime full legacy path (ITEMMGR/AbilityManager/ShopItemOption hooks) remains a follow-up.
 Ports the legacy [Server]Map/ShopItemManager.h data plane: SHOPITEMBASE/SHOPITEMWITHTIME/MOVEDATA packed structs under #pragma pack(1) (34/38/31 bytes), the five dup counters (Incantation/Charm/Herb/Sundries/PetEquip), the m_ProtectItemIdx, the m_Checktime/m_Updatetime timers, and CRUD on the using-item table (legacy UsingItemPool 50/10) and move-point table (legacy MovePointPool 50/10, MovePointTable 30). Init zeroes all state and stores the opaque player pointer; release clears state and nulls the pointer; add_* rejects 0 keys and duplicates. UseShopItem/CheckEndTime/CalcShopItemOption and the legacy AbilityManager/StatsCalcManager/EventMapMgr/etc. hooks remain in a follow-up commit.
 Removes the fabricated 2,000,000,000 cap and reuses MXH_PLAYER_MAX_MONEY=0xFFFFFFFF; add_money now mirrors CPurse::Addition by clipping to remaining room instead of rejecting the whole addition, accepts 2,000,000,001, and cannot wrap at UINT32_MAX.
- [x] D5 MurimNet 1:1 lock | 139 tests (11 modules: agent_murimnet + channel + channelling_manager + crypt + player + play_room + play_room_manager + protocol + runtime + wire + wire_serializer) | runtime connect/reconnect/disconnect/transport-disconnect/team_change/broadcast_chat/channel-mode/captain-start/start-ack/start-nack/channel-room-state/closing/duplicate-id-skip/eLOCATION_-order all locked; channel/playroom wire-byte serializers (mnh_build_channel_list_wire/mnh_build_playroom_list_wire/mnh_build_player_list_wire/mnh_build_channel_info_wire/mnh_build_playroom_info_wire/mnh_build_player_info_wire/mnh_build_chat_wire/mnh_build_team_change_wire/mnh_build_byte/word/word2/dword/dword2/dword3/dword4_wire) match legacy GetMsgLength() = MSGBASE + 4 + count*BaseInfo; 60 protocol codes locked in MurimNetProtocol enum; MHTimeManager (10 tests) replaces gCurTime/MHTIMEMGR TODOs（频道状态已锁：Closing 禁止重新加入/发消息，非成员禁止频道注入消息，销毁频道同步清理历史，频道 ID 溢出跳过 0/重复值；PlayRoom + PlayRoomManager + MNPlayer/MNPlayerManager 已移植（房间状态、索引、玩家连接字段与生命周期）；Agent parser 未知协议显式丢弃；runtime connect/reconnect/disconnect/game-logout 已接房间与玩家管理器，开局 transport 断线保留玩家并允许重连；队长开局请求与 ACK/NACK 状态已锁；大厅 Channel/ChannelManager 核心已移植并接入 runtime 频道进出MnPlayerLocation 拆出 Channel 与 PlayRoom（Channel=1, PlayRoom=2）匹配老 eLOCATION_ 顺序；MnChannelMode 与 runtime.select_channel_mode 已落地，状态码新增 InvalidChannelMode=10，要求进频道才能选 ID/Channel/PlayRoom 三种视图runtime.team_change 已落地，对齐 MP_MURIMNET_PR_TEAMCHANGE_SYN：检查玩家所在房间 -> 检查 room 启动 -> 调用 play_room.team_change，错误返 Ok/CapacityFull、RoomStarted、NotInRoom、RoomNotFound、PlayerNotFoundMurimNet 协些协些 wire-byte 协计协义：以 MP_PROTOCOL_MURIMNET 60 个协计代码建立枚体型 enum 迁出， agent_murimnet 类体原来 升到：changetomurimnet_syn=0/connect_syn=6/reconnect_syn=9/disconnect_ack=13/pr_teamchange_syn=21/chnl_modechange=32/chat_all=48/notifytomn_player_logout=58通道，房间以 for_each_member 接口，另 runtime.broadcast_chat 根据玩家位置（转发 Channel/PlayRoom/NotInRoom），对齐 MP_MURIMNET_CHAT_ALL、 SendMsgToAll 调用 sink回调MHTimeManager 已落地（启动、 init）可接 now_ms 参数， 清除 timeGetTime ） + TICK_PER_DAY/HOUR/MINUTE 1:1 锁赭 MhDate， MhTime 分裂 getter ）， 解  modern 代码 gCurTime/MHTIMEMGR TODO）MurimNetChannel.for_each_channel + MurimNetPlayRoomManager.for_each_room ???,?? SendMsg_ChannelList/SendMsg_PlayRoomList ????(commit 83d53c1b);?? wire-byte ???(MSG_CHANNEL_BASEINFO/MSG_PLAYROOM_BASEINFO)???wire-byte ???(MnhWireBase/MnhMsgChannelBaseInfoList/MnhMsgPlayRoomBaseInfoList/MnhMsgTeamChange/MnhMsgPlayerBaseInfo[|List])? #pragma pack(1) ?? 1:1 ????(commit ce931c51,9 size ????);SendMsg_ChannelList / SendMsg_PlayRoomList ??????(commit 845294ec,8 tests:size ?? GetMsgLength?Category=38?Protocol=34/35?title ?? 63 ??+???);channel/playroom ? kind/max_observers/max_players_per_team/money_for_play/map_num ?? getter;?? MnPlayerBaseInfo ??? / Chat_All wire / Crypt stub ???
- [x] D6.2 FieldBossMonsterManager 1:1 lock | 19 tests lock legacy [Server]Map/FieldBossMonster.h
 FieldBossMonsterManager: configure_channel() auto-resizes channel vector
 (default-initialized entries disabled), tick() iterates channels in order,
 fires at most one per call (1:1 with legacy reak), skips monster_kind=0
 (disabled) / active=1 (alive) / respawn_interval_ms=0 (no-respawn) / now_ms<
 next_spawn_ms (not yet) / template-not-found, sets active=1 + assigns
 current_object_id + reschedules next_spawn_ms=now+interval, returns true.
 Tests: DefaultManagerHasZeroChannels, ConfigureChannelStoresAllFields,
 ConfigureChannelAutoResizes, ChannelOutOfRangeReturnsNull,
 TickWithNoEnabledChannelsReturnsFalse, TickBeforeNextSpawnMsReturnsFalse,
 TickAtNextSpawnMsFires, TickAfterSpawnReschedules, TickSkipsAliveChannel,
 TickSkipsDisabledChannel, TickSkipsChannelWithZeroRespawnInterval,
 TickSkipsWhenTemplateMissing, TickWithEmptyTemplatePoolSkipsSpawn,
 TickUpdatesLastSpawnObjectId, TickOneSpawnPerCallEvenWithMultipleChannels,
 TickRespawnsAfterActiveCleared, SpawnedChannelRemembersPosition,
 MultipleChannelsWithDifferentIntervals, FieldBossChannelDefaultZero.
- [x] D6.2 experience reward wire serialization | 3 new tests (29 distributer tests total) lock legacy MSG_EXPPOINT (17 bytes: MSGBASE + INT64 + BYTE) and MSG_ABILLITY_EXPPOINT (13 bytes: MSGBASE + DWORD + BYTE), including little-endian fields, ACK protocol bytes 13/29, custom header values, and signed INT64 preservation.
- [x] D6.3 BossMonsterManager 1:1 lock | 19 tests lock legacy [Server]Map/BossMonsterManager.h CBossMonsterManager
 BossMonsterManager: register_info() upserts per-MonsterKind into infos_ map, info_for() returns pointer or nullptr. spawn() looks up info; returns 0 when missing (legacy guard against spawning without static info), otherwise calls create_boss_from_template with x/y/z/map and stores by object_id. Same object_id respawn replaces (1:1 with legacy). damage() forwards to apply_boss_damage on the live instance; returns Sealed for unknown oid. erase() returns bool, idempotent. live_count() tracks map size.
 Tests: DefaultManagerIsEmpty, FindUnknownOidReturnsNull, RegisterInfoStoresData, InfoForUnknownKindReturnsNull, RegisterMultipleKinds, RegisterInfoOverwritesByKind, SpawnWithoutInfoReturnsZero, SpawnCreatesLiveInstance, SpawnMultipleDistinctOids, SpawnSameOidReplaces, InfoKeepsRewardsIntact, FindConstAndNonConstReturnSamePointer, DamageUnknownOidReturnsSealed, DamageKnownOidReturnsValidPhase, EraseUnknownOidReturnsFalse, EraseKnownOidReturnsTrueAndRemoves, EraseOneKeepsOthers, EraseThenSpawnReusesOid, LiveCountTracksSpawnAndErase.
- [x] D6.4 cMonsterSpeechManager 1:1 lock | 15 tests lock legacy [Server]Map/cMonsterSpeechManager.h
 cMonsterSpeechManager: register_speech() appends to flat vector (1:1 with legacy, no dedup). find(kind, trigger) returns first match or nullptr. death_speech(kind) is a convenience for find(kind, 2). Trigger codes are 0=on-spawn, 1=on-HP-50%, 2=on-die. MAX_SPEECH_PER_MONSTER=8
 Tests: DefaultManagerIsEmpty, RegisterIncreasesSize, FindReturnsRegisteredEntry, FindByTriggerDistinguishesSpawnHalfHpDeath, FindDifferentiatesMultipleKinds, FindReturnsFirstMatchForDuplicateKey, FindReturnsNullForUnknownKind, FindReturnsNullForUnknownTrigger, DeathSpeechReturnsTriggerTwo, DeathSpeechReturnsNullWhenMissing, FindAndDeathAgreeOnSameEntry, SpeechIdPreserved, LongTextFitsBuffer, MultipleKindsIndependent, MaxSpeechPerMonsterConstantIsEight.
- [x] D6.5 ExperienceCurve 1:1 lock | 15 new tests (5 -> 20 total) lock legacy CharacterExpPoint.bin reader
 ExperienceCurve::load_from_bin delegates to compat::read_mh_bin and parses the text payload with parse_entries() (1:1, sequential level/exp rows, exactly MAX_CHARACTER_LEVEL_NUM=121 entries). load_from_text is the in-memory entry point. max_exp_point(level) returns required_exp_[level-1] (or [0] for level=0 legacy quirk). add_exp(state, amount) increments by at most one level per call (1:1 with legacy, callers loop). At MAX level add_exp is a no-op. New tests cover: per-level max_exp_point consistency, below/exact/one-below threshold transitions, level-0 quirk, enormous amount cap, zero-amount no-op, max-level no-op, tab/CRLF whitespace tolerance, rejects empty / non-numeric / extra-row (silently truncated at 121).
- [x] D6.6 state_param + summon_monster + titan_item_manager 1:1 lock | 31 new tests on Phase D subsystems (8+5+10 -> 16+14+24)
 state_param_set_state transitions stateOld <- previous stateCur, sets stateNew = stateCur = dwState, bStateUpdate = true. state_param_init resets all 6 numeric fields + flag. state_param_elapsed clamps below stateStartTime to 0. summon_monster (legacy CSummonMonster typo m_SummmonerID preserved) tracks summoner/regen/die times; age_ms returns now-regen clamped below 0. titan_item_manager covers titan upgrade (consume money+materials with empty-stack cleanup), titan break (per-roll weighted selection with rate subtract), titan grade cap (TitanGrade stops at MAX_TITANGRADE), and titan_material_count (sums matches with UINT32_MAX saturation).
- [x] D6.7 distribute_network_msg_parser + common_network_msg_parser 1:1 lock | 19 new tests on Phase D T2 protocol-dispatch coverage (5+5 -> 13+16)
 DistributeNetworkMsgParser: register_handler upserts handler per (cat,proto) pair; dispatch invokes only the matching handler and passes through session_id/protocol/object_id/category/payload verbatim. CommonParserTables: 96-slot fixed array (mp_max=96, 1:1 with legacy MH protocol table), set_server/user replace by index, invoke_server/user routes independently (same idx different parser), OOB indices rejected without throwing, empty slot is a no-op.
- [x] D6.8 PlayerMonsterPoint 1:1 lock | 9 new tests (4 -> 13 total) lock level-gap lookup and parser behavior
 PlayerMonsterPointTable tests cover level boundaries and midpoints, invalid level/gap exceptions, get_player_point gap clamping, max-level zero progression, non-numeric rejection, valid zero-gap column mapping, tab/CRLF whitespace tolerance, and just-below-max behavior.
- [x] D6.1 DropTableRegistry 1:1 lock | 31 tests lock legacy [Server]Map/ItemDrop.h
 CItemDrop: add() appends to flat vector (no dedup, 1:1 with legacy), find() returns
 first match by (monster_kind, drop_id), roll() uses rng_value % total_ratio with
 per-entry interval pickup (entry 0 covers [0, r0), entry 1 covers [r0, r0+r1), ...),
 total==0 returns 0 (div-by-zero guard), total==0 explicit single entry returns 0,
 fallback returns entries.back().item_id for u32 overflow, large rng (0xFFFFFFFF)
 honored via modulo, MAX_DROP_PER_MONSTER=10 const, MAX_DROP_ITEM_PERCENT=10000
 basis points 1:1. Tests: EmptyRegistryFindReturnsNull, EmptyRegistryRollReturnsZero,
 AddIncrementsSize, AddAllowsDuplicateTablesByValue (no dedup), FindReturnsNullFor*
 (Kind/DropId/Both), FindFirstMatchWins, RollSingleEntryWithFullRatio,
 RollTwoEntriesWithRatioBoundary (off-by-one 69/70), RollUsesModuloArithmeticWhenRngExceedsTotal,
 RollTotalZeroReturnsZero, RollTotalZeroExplicitSingleEntry, RollUsesLastEntryAsFallback,
 RollThreeEntriesDistribution, RollAsymmetricRatios, RollSingleEntryZeroRatio,
 RollIndependentForDifferentKind/DropId, RollPreservesInsertionOrderAcrossTables,
 RollReturnsZeroForMissingKind/DropId/Both, LargeRngValueModuloTotal, LargeRatiosRespectBoundary,
 FindReturnsConstPointer, RollIsIdempotent, DropTableDefaultsZeroEntry, DropItemEntryDefaultsAreOne,
 MaxDropPerMonsterConstantIsLegacyTen, AddDoesNotDeduplicateAcrossKind.
D6 经验曲线 / 伤害公式 / 爆率 / Boss 刷新回归测试
- [ ] 验证：side-by-side 跑原版和 modern，截日志 diff = 0

### Phase E —— 1:1 复现 E2E 验证（2 周）
**目标**：3 个 T 全过。

- [ ] E1 T1 资源：跑 80+ `.bin` 全部解析，老客户端 + modern diff = 0
- [ ] E2 T2 协议：截 1000+ 包，modern 编解码和老码编解码 diff = 0
- [ ] E3 T3 行为：录 5 段操作（登录/打怪/PK/商城/任务），modern 回放输出和老码 diff = 0
- [ ] E4 1.0 release tag

### Phase F —— 维护期（无限期）
- [ ] F1 社区 bug 反馈 → 修
- [ ] F2 资源补丁兼容（新 `.bin` 格式）
- [ ] F3 Linux 客户端 / 跨平台（可选）

---

## 4. 当前在做

### ✅ Phase A 已完（commit `f80f6e0`）
- MoxianClient 骨架（DX11 + WinMain + 4 sprite + cImage↔IDISpriteObject）
- CMainGame 1:1 port：eGAMESTATE 0..9 byte-for-byte + state machine
- 9 个 state stub + CMainTitle（14 字段 1:1 surface + Init 读 MHVerInfo.ver）
- 25 client unit tests + ctest 全过（2405 total）

### ✅ Phase B 已完（commits `632e815` `c726127` `4bda372` `d176f32` `ec0c11f`）
- **B.2.1** `CLoginState` 替换 CConnecting stub：RequestLogin 38B legacy payload 字节级
- **B.2.2** `CCharSelectState` 替换 CCharSelect stub：CharacterListSyn 8B + ListAck 889B + 自动选第一个 + CharacterSelectSyn
- **B.2.3** `CInGameState` 替换 CGameIn stub：GameInSyn + 当前默认分支 3775B SEND_HERO_TOTALINFO 解析（含 2728B / 124 槽 ITEM_TOTALINFO）
- **B.2.4** 状态机真接 net 事件（host 监视 state-change 上升沿调 Start）
- **B.2.5** `MoxianClientE2E` headless tool：CreateProcessW 启 3 server 子进程 + 真 C++ 状态机跑 Login→CharSelect→InGame 全流程。集成 ctest（60s timeout，标签 client/e2e/server）
- **3 server 端到端 + Python 模拟 + C++ 真状态机** 三层 E2E 全 PASS
- client test 25→50；e2e tool 是 ctest 的一员

### ✅ Phase C 进展
- 102 hpp / 1721 ui test — modern UI 100% 覆盖（每个 dialog 都有 unit test 锁 1:1 行为）
- Bug fix：cListItem m_maxLine=0 inconsistency（commit `c79231c`）
- Bug fix：cPushupButton sticky click state（commit `1406096`）
- 新 1:1 lock：cObject 9 tests / cListItem + ComboItem 15 tests / cPushupButton 2 new tests
- legacy dialog 缺项 port 评估：跨 GameResourceManager/MainTitle/ChatManager/ItemManager 依赖大，**defer 到 Phase D 跟 subsystem 一起做**

### ✅ Phase D 起步（commit `fe74077`）
- **D6.1** `mxh_game` lib 数值 baseline lock：7 OBJECTKIND / 6 MonsterAI state / 14B MonsterTotalInfo / 22B ItemBase / 124 槽、2728B ItemTotalInfo / 4 ItemEffect 公式 / 3 default MonsterTemplate 全部 1:1 锁死

### 下一步
- D1 SkillManager 1:1 port — **D1.1 (placeholder 表) + D1.2 (lookup class) 已完，剩 D1.3 (SkillList.bin 1:1 parser) 是 1-2 周大活，defer**
- D2 BattleFactory / D3 QuestManager / D5 MurimNet — 各 1-2 周
- D4 商城/物品/仓库/邮件/帮派/队伍 — 3-4 周
- Phase B/C 残：HSEL 补完、HackShield 绕、SQL Server 真启、MSSQL 端到端、Render 真正显示角色

### D1.3 (SkillList.bin parser) — 评估后 defer

**为什么 defer**:
1. **Bin 格式复杂**: legacy `CSkillInfo::InitSkillInfo` 读 ~50+ 字段（SkillIdx, SkillName string, TooltipIdx, RestrictLevel, LowImage, HighImage, SkillKind, WeaponKind, SkillRange, TargetKind, TargetRange, ... Duration, Interval, ... 12-element NeedExp array, 12-element NeedNaeRyuk array, Attrib, ... ~6 AdditiveAttr cases with 12-element attribute arrays, etc.）。完整 1:1 1 个 skill 解析 200+ bytes bin，subset port 不"1:1"。
2. **struct 1:1 不一致**: ~~D1.3 已扩 `SkillInfo` 到 full legacy `SKILLINFO` 60+ 字段含 7×[12] 数组（D1.3.1 ✅）~~。
3. **MapHandler 已有 hardcoded** (`init_skill_table` line 1319+) 4 个 skill（BasicSlash, FireBolt, Heal, Whirlwind）— D1.3 改用 `add_simple_skill` helper project `SkillInfoSimple` → `SkillInfo` 1:1 struct（D1.3.3 ✅，向后兼容 + 留 hook 给 `init_from_bin`）。
4. **Bin 文件大**: `SWorking\Resource\SkillList.bin` 769KB，1817 entries × 150 token/行（`{150: 1817}` 分布）— D1.3.2 parser 一气呵成。

**D1.3 完成**（2026-07-26 commit `2228226`）:
- **D1.3.1** ✅ 扩 `mxh::game::SkillInfo` 到完整 legacy `SKILLINFO` 60+ 字段（PascalCase，1:1 名字），7 段 12-元素数组（NeedExp/NeedNaeRyuk/CounterDodgeRate/FirstRecoverLife/.../ContinueAttAttackRate/AmplifiedPowerPhy/.../VampiricLife/.../UpPhyDefence/.../DownPhyDefence/.../SkillAdditionalTime/UpAttAttack/DamageRate/AttackRate/UpCriticalRate/AttackLifeRate/AttackShieldRate/AttackSuccessRate/VampiricReverseLife/VampiricReverseNaeryuk/AttackPhyLastUp/AttackAttLastUp）。Fix 1:1 byte 布局，BYTE/WORD/DWORD 类型都保留。加 2nd-class 字段（SkipEffect/SpecialState/ChangeKind/AddDegree/SafeRange/LinkSkillIdx）。提供 `to_simple()` view 给运行时 combat 路径。
- **D1.3.2** ✅ `SkillListParser` 1:1 port 老 `[CC]Skill\skillinfo.cpp::InitSkillInfo` (L65-258) + `[Client]MH\MHFile.cpp::OpenBin`/`CheckCRC` (L102-201)。Decode `m_pData[i] -= i; if (i%type==0) m_pData[i] -= type`。`FindEffectNum` 走 map-server 分支（0 if "0", else 1）。CRLF split → tab+space tokenize → 6 AdditiveAttr segment (1 disc + 12 values) → SKILLINFO 字段。Header check + per-row try-catch 累加 parse_errors。
- **D1.3.3** ✅ `SkillManager::init_from_bin(path, *out_errors)` — 调 `load_skill_list` → clear → add 全部 entries。IO error throw `std::runtime_error`；per-row error 累加到 `out_errors` 不 throw。`MapHandler::init_skill_table` 改用 `add_simple_skill` helper 投影 `SkillInfoSimple` → `SkillInfo` 1:1 struct，留 hook 给将来 `init_from_bin`。
- **D1.3.4** ✅ 11 个 `SkillListParser` test: decode roundtrip (zero/ascii), parse_skill_row (minimal, with effects, wrong token count, 5 AdditiveAttr cases), **2 端到端 test load 真 SWorking\Resource\SkillList.bin 验证 first entry 1:1 字段** (SkillIdx=1, SkillKind=0, SkillRange=230, WeaponKind=1, ComboNum=1, EffectStart=0, EffectUse=1, LinkSkillIdx=10001, etc.)。

**测试**: 42/42 game tests pass, **2536/2536 全 ctest pass**（8 skipped，0 fail）。真 bin 1817 entries 全部 load 成功，0 parse_errors。

**Trivia**:
- skill_list_parser_test 必须用 `LR"(...)"` 路径 + ASCII temp copy (kTempSkillList)：MSVC `std::ifstream` ctor 对 CJK 路径会跑 `MultiByteToWideChar` → throw `system_error(1113)`。
- `e.what()` 自身会 throw（system_category::message(1113) → MultiByteToWideChar 失败 → recursive system_error）— test catch 块只 print `e.code().value()`，不 print `e.what()`。
- MapHandler 用 `auto simple = mxh::game::to_simple(*skill);` 在 hot path 一次投影。

下一波: D1.3 完工，**Phase D2 BattleFactory / D3 QuestManager / D4 商城物品 / D5 MurimNet** 任选，或回头推 Phase C 残（HSEL 补完 / HackShield 绕 / Render 真实输出）。

---

## 5. 完成判据（**唯一**，**不再含糊**）

| 阶段 | 完成判据 |
|---|---|
| A | 截图对比原版登录界面，差异 ≤ 字体抗锯齿级别 |
| B | modern 客户端登录 + 看到空场景，日志和原版对齐 |
| C | P2-12 202/202 + 每个 dialog 有 ≥1 行为断言 test |
| D | side-by-side 跑原版/现代版，5 段操作 diff = 0 |
| E | T1+T2+T3 全过，1.0 tag |

**没达 = 没完。** 任何"基本完成""大部分完成"都不算。

---

## 6. 已废弃的概念（防止旧词干扰新方案）

| 旧词 | 为什么废弃 |
|---|---|
| "现代化" | 不是目标，1:1 复现才是 |
| "兼容性优先" | 模糊；改成"协议/资源/数值 1:1 锁定" |
| "12 阶段路线图" | 把手段当目标，误导 |
| "Qoder IDE / QoderWork / QoderWake" | 我们用 Mavis，这套三产品不适用 |
| "AI 接力" / "shift log" | 噪音，没有可执行价值 |
| "P0/P1/P2/P3 任务队列" | 旧框架，按 1:1 阶段重排 |
| "modern 进度 35.1%" | 单独看无意义，要看对应 T 的覆盖 |
| "Phase 12 已完成" | 误导，Phase 0-12 全部标完成但 T1/T2/T3 一个没过 |

---

## 7. 文档体系（替代老的散乱文档）

| 新文档 | 内容 | 替代什么 |
|---|---|---|
| `ROADMAP.md`（本文件） | 1:1 复现路线图 + 现状 + 完成判据 | MODERNIZATION_PLAN.md / ROADMAP_2026.md / P2-12_DIALOGS_ROADMAP.md / AI_TASK_QUEUE.md |
| `AGENTS.md` | Mavis 协作指南（项目结构 + 真陷阱） | 老 AGENTS.md + AI_WORKFLOW_GUIDE.md |
| `README.md` | 极简上手（5 分钟跑起来） | 老 README.md |
| `CHANGELOG.md` | 仅版本号 + 阶段完成（每阶段 1 行） | 老 3091 行 changelog |
| `docs/RESOURCE_FORMATS.md` | 资源格式 | 保留 |
| `docs/MoxianProtocolDoc.md` | 协议 | 保留 |
| `docs/DATABASE_SCHEMA.md` | 数据库 | 保留 |
| `docs/KNOWN_BUGS.md` | Bug 清单 | 保留 |

---

**下一步**：清理老的 14 份干扰性文档（见 `CLEANUP_PLAN.md` 清单），重写 `AGENTS.md` + `README.md`。
- [x] **C-Batch-2.33** cGuageBar -- completes the missing unit-test coverage for the already-ported cGuagen slider subclass. 26 tests lock default state, interval/orientation initialization and clamping, legacy value-to-track repositioning, rate semantics, horizontal/vertical button sizing, null/duplicate Add behavior, absolute-position propagation, lock color, inactive/locked ActionEvent, and Render smoke behavior.
- [x] **P-Batch-1** Category wire compatibility correction -- replaced the drifted modern category ordering with explicit legacy MP_CATEGORY bytes 1-78; HackShield=67, GTournament=60, NProtect=69, and added complete category-table regression coverage plus corrected GTournament constants.
- [x] **R-2** AgentHandler HackShield routing -- routes cat==HackShield (67) messages through the HackShieldManager state machine (mxh::server::parse_hackshield_message). Per-connection state (UserLevel, m_bHSCheck, dwConnectionIndex) tracked in conn_user_levels_ + conn_hs_states_; disconnect pending queue in hackshield_disconnect_pending_. handle_legacy_character_select now queries chr_log_info.userlevel and triggers send_guid_req for superusers (eUSERLEVEL_SUPERUSER >= 5) immediately after CharacterSelectAck (1:1 with legacy AgentDBMsgParser.cpp:500-502). register_session(id, user_id, char_id, map_num, user_level=0) accepts user_level with default 0 for source compat. on_disconnect clears user_level + hs state + disconnect-pending entry. 10 new tests cover: superuser/non-superuser gate, all 5 protocols (GuidReq, GuidAck, Req x1, Req x2, Ack, Disconnect), state machine transitions, on_disconnect cleanup, conn-id isolation. Pure data-plane: vendor AntiCpSvr calls replaced with make_*_succeeded=true / analyze_succeeded=true defaults so tests can drive without the proprietary library.
- [x] **R-2.1** AgentHandler auto-populates user_level from chr_log_info on CharacterListSyn (proto=9) so the HackShield gate sees the right threshold without requiring a manual register_session call. Adds MockDbAdapter::userlevel_to_return injection field (0xFF = no injection, default). 3 new tests cover: superuser GuidAck -> Req reply, non-superuser GuidAck -> dropped, missing userlevel -> defaults to 0.
- [x] **bug**: Crypt::encrypt_crc / decrypt_crc always returned 0 instead of the underlying HselStream CRC char (legacy CCrypt::GetEnCRCConvertChar/GetDeCRCConvertChar semantics). Fixed by delegating to en_stream_.get_crc_char() / de_stream_.get_crc_char() with the same uninitialized-returns-0 early-out. Adds 4 new tests (EncryptCrcIsZeroWhenUninitialized, EncryptCrcAfterInitBeforeDataIsZero, EncryptCrcIsNonZeroAfterEncryptingData, EncryptCrcIsDeterministicAfterSameDataEncrypt) so the regression cannot recur. Crypt tests now 7/7 PASS.
- [x] **R-2.2** AgentHandler::tick_hackshield() server-side periodic recheck -- walks every connection with active HackShield state and runs send_hackshield_req() through the same state machine the client-driven Req handler does. Legacy CHackShieldManager called this every ~30s. State machine: m_bHSCheck==0 idle -> send Req + transition to waiting; m_bHSCheck==1 waiting -> Disconnect (timeout); m_bHSCheck==2 grace -> waiting (no send). Non-superusers are filtered before the loop so the state machine no-ops are skipped. Returns the number of Send/Disconnect actions so tests assert progress without inspecting reply buffers. Snapshot-and-process-then-persist pattern avoids re-entering user_mu_ from the reply_ callback. 4 new tests cover: empty handler is zero, grace superuser noop then disconnect, non-superuser no-op, mixed superusers each get one disconnect action after two ticks. AgentHandlerHackShieldTest now 17/17 PASS (was 13).
- [x] D3.3 QuestExecute_Quest/Time state dispatch | 5 QuestGroup helper tests + 4 dispatch tests lock StartSub/EndSub active-slot model, EndSub legacy subQuestFlag bit `1 << (31-idx)` plus data/time reset, EndQuest repeat vs final complete, RegistCheckTime day/hour/minute persistence, SaveLoginPoint 2000 offset clamp, and explicit MissingSubquest status; MapChange/RegenMonster/ChangeStage/ChangeSubAttr deliberately return UnsupportedContext.
- [x] D3.4 QuestExecute_Item data-plane dispatch | 3 new tests lock GiveQuestItem removing rows from m_QuestItemTable, TakeQuestItem inserting {questIdx,itemIdx,itemNum} plus subquest count and probability gate, TakeMoneyPerCount erasing the row when an amount is returned, and explicit UnsupportedContext for GiveItem/TakeItem/GiveMoney/TakeMoney/TakeExp/TakeSExp/Weapon-filtered RandomTakeItem until player inventory/network contexts are supplied.
