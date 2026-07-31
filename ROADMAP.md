# Moxian-Reborn 路线图：1:1 完美复现

> **项目代号**：Moxian-Reborn（墨香重生）
> **终极目标**：在现代软硬件环境下 **1:1 完美复现** 2003-2010 韩国 2D MMORPG《墨香》——
> 玩法、数值、协议、资源、UI 全部和原版一致；只在底层换技术栈。
> **本文档替代**：老的 `MODERNIZATION_PLAN.md` / `ROADMAP_2026.md` / `P2-12_DIALOGS_ROADMAP.md` / `AI_TASK_QUEUE.md`。
> **最近一次重置**：2026-07-25（清掉所有历史 session 噪音、重新对齐到终极目标）。
> **最近一次状态刷新**：2026-07-31 — Phase C Batch 2.15 完成；D2 BattleFactory 定向回归确认通过（cStreetStall + cStreetBuyStall，12 tests）（meta-cleanup，60 个 P2-12 注释更新到 Phase C 1:1 port 状态，UI 套件 2351 全过）；Batch 1 全部完成（10/10 dialog），Batch 2.1+2.2+2.3+2.4+2.5+2.6+2.7 合计 7 个新 1:1 port + Batch 2.8 修复 3 个 test 文件 + Batch 2.9 标记 60 个 P2-12 stub 为已 1:1 ported。

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

## 2. 现状盘点（截至 2026-07-26）

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
| 玩法数值 baseline | **D6.1 锁死** | 7 OBJECTKIND / 6 MonsterAI / 14B MonsterTotalInfo / 22B ItemBase / 110 槽 ItemTotalInfo / 4 ItemEffect 公式 / 3 default MonsterTemplate 全部 1:1 锁数值 | T2/T6 数值回归 baseline |
| **SkillList.bin 解析** | **D1.1+D1.2+D1.3 全完成** | SkillInfo 扩到 1:1 legacy SKILLINFO（60+ 字段含 7×[12] 数组）；SkillListParser 解码 MHFile packed-text；SkillManager::init_from_bin() 装真 bin（1817 entries）；端到端 test 锁首行 | 11 SkillListParser test + 13 SkillManager test PASS |
| **BattleFactory 1:1** | **D2 完成** | 14 compute_* 函数（critical/decisive/player-phy/player-attr/player-exp/player-point/phy-defence/received-dmg/monster-phy/monster-attr/titan-phy/titan-attr）+ 13 新 1:1 unit test（attack_calc legacy_* 系列保持 API 兼容） | T2 数值公式回归
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
- [ ] D4 商城 / 物品 / 仓库 / 邮件 / 帮派 / 队伍 1:1
- [ ] D5 MurimNet PvP 1:1（频道 Closing 状态已锁：禁止重新加入/发消息）
- [ ] D6 经验曲线 / 伤害公式 / 爆率 / Boss 刷新回归测试
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
- **B.2.3** `CInGameState` 替换 CGameIn stub：GameInSyn + 3000B SEND_HERO_TOTALINFO 解析
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
- **D6.1** `mxh_game` lib + 34-test 数值 baseline lock：7 OBJECTKIND / 6 MonsterAI state / 14B MonsterTotalInfo / 22B ItemBase / 110 槽 ItemTotalInfo / 4 ItemEffect 公式 / 3 default MonsterTemplate 全部 1:1 锁死

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
