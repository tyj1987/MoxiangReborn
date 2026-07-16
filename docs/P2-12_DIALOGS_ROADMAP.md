# P2-12 Dialogs 移植 Roadmap

> **作者**：Mavis  
> **日期**：2026-07-16  
> **状态**：🟡 进行中（6/202 = 3.0%）  
> **关联**：`docs/KNOWN_BUGS.md` R-12、`AI_TASK_QUEUE.md` P2-12

## 背景

legacy `[Client]MH/` 目录下有 **202 个 dialog 文件**（含 1 个备份
`MugongDialog_BACKUP.{h,cpp}`，实际活跃 201 个）。现代 `modern/src/ui/`
只 port 了 **6 个**：`cDialog`（基类）、`cGuildDialog`、`cIconDialog`、
`cListDialog`、`cExitDialog`、`cGuagen`（子控件）。一次推完 196 个
dialog 不现实——这是 Phase 6 时代就遗留的 Phase 12 长尾任务。

本文把 202 个 legacy dialog 按"实现难度 + 依赖复杂度"分 5 档，每档
标 port 优先级 + 估计工作量 + blocker，让后续 session 按矩阵接活。

## 已 Port 列表（6/202 = 3.0%）

| 现代类 | 头文件 | 测试数 | 备注 |
|--------|-------|-------|------|
| `cDialog` | `modern/src/ui/cDialog.{hpp,cpp}` | 8 | 基类，Phase 6.3 |
| `cGuildDialog` | `modern/src/ui/cGuildDialog.{hpp,cpp}` | 13 | Phase 6 早期 |
| `cIconDialog` | `modern/src/ui/cIconDialog.{hpp,cpp}` | ? | Phase 6 |
| `cListDialog` | `modern/src/ui/cListDialog.{hpp,cpp}` | ? | Phase 6 |
| `cExitDialog` | `modern/src/ui/cExitDialog.{hpp,cpp}` | 10 | 2026-07-16 (0.13.10) |
| `cGuagen` | `modern/src/ui/cGuagen.{hpp,cpp}` | 18 | **2026-07-16** (0.13.12) — Tier 2 子控件 |

## 5 档分级

### Tier 1 — Trivial（无外部依赖，1-2 commit/个，~80-200 行）

**特征**：纯 UI 状态机，不读 GameIn/不连网络/不调 NPC 脚本。
基类 cDialog 已能 cover 90%，子类只需 override SetActive/Init
+ 1-2 个 callback。**适合做小活起点**。

| Legacy Dialog | 现代目标 | 工作量 | 优先级 |
|--------------|---------|-------|-------|
| ExitDialog | `cExitDialog` ✅ | 100 行 + 10 test | **已完成** (commit 16ef797) |
| _(已查尽 — 无 trivial 候选)_ | — | — | — |

**重要修正（2026-07-16 探查后）**：原 Tier 1 列表中除 ExitDialog 外
**全部不是 trivial**——legacy 客户端所有 dialog 都深度耦合
global singleton（CHATMGR / OBJECTMGR / GUILDMGR / HERO /
WINDOWMGR / NETWORK / ITEMMGR / HEROID）。经 grep `<1500B` + 检查 cpp
实装后，无 trivial 候选。具体排除原因：

- `HelpDialog` 依赖 cListDialogEx + cPage + cDialogueList + cHyperTextList
  + HelpDicManager（**Tier 2** — 需先 port cListDialogEx）
- `BailDialog` 依赖 cEditBox + cTextArea + CHATMGR + HERO + WINDOWMGR
  + NETWORK（**Tier 3/5** — 需 InventoryService + NetworkService）
- `EventNotifyDialog` 待查（header 274 B，但 cpp 体量可能大）
- `MallNoticeDialog` 待查
- `ChatOptionDialog` 依赖 ChatManager settings（**Tier 5**）
- `NameChangeDialog` 依赖 CHATMGR + GAMEIN + NETWORK（**Tier 5**）
- `PetRevivalDialog` 依赖 OBJECTMGR + ITEMMGR（**Tier 5**）
- `NumberPadDialog` 依赖 WINDOWMGR + cStatic + cComboBox + MT_LOGINDLG
  （**Tier 5**）
- `MNCreateDialog` / `MNFrontDialog` / `MNJoinDialog` 实际是**空壳
  stub**（cpp 全空，WindowIDEnum 没 MN_* 编号）— port 它们毫无
  价值（不会从 dispatcher 触发）

**新策略**：把"trivial"重新定义为 **"无需 modern port 任何
global service 的 dialog"**。符合条件者：
- ✅ cExitDialog（已 port，callback pattern）
- ❌ 其他所有 dialog 至少依赖 1 个 global service

**建议推进**：每次新 session 直接从 **Tier 2** 开始（接受 200-500
行 + 1 个 service interface 注入），不要再在 Tier 1 找 trivial。

### Tier 1.5 — 子控件 widget（无 service 依赖，作为 Tier 2 dialog 的子组件）

**特征**：本身不是 dialog，而是 dialog 内部用的子控件（progress bar、
list control、edit box、pushup button 等）。Tier 2 dialog 直接需要
这些子控件。**先 port 完子控件再 port dialog**，否则 Tier 2
"无 blocker" 的描述会暴露真实 blocker（0.13.12 教训：cGuagen 缺失
阻塞 CharacterDialog / QuestDialog / MugongDialog 等多个 Tier 2）。

| 子控件 | 现代目标 | 子 dialog 用户 | 状态 |
|--------|---------|---------------|------|
| `cGuagen` | `cGuagen` ✅ | CharacterDialog (HP/MP bar), MonsterGuageDlg, TitanGuageDlg, MainBarDialog, GuageDialog | **2026-07-16 ported** (commit `de390cd`, 18/18 test) |
| `cPushupButton` | `cPushupButton` | OptionDialog, KeySettingTipDlg, MiniNoteDialog | 候选 (Tier 1.5 下一个) |
| `cListCtrl` | `cListCtrl` (确认 modern 端存在) | FriendDialog, NoteDialog, PartyDialog, ChatDialog, QuestDialog | 候选 |
| `cEditBox` | `cEditBox` (确认 modern 端存在) | ChatDialog, NoteDialog, MacroDialog, ChatOptionDialog | 候选 |
| `cListDialogEx` | `cListDialogEx` | HelpDialog, 多个 log-style dialogs | 候选 (Tier 1.5 范围, 无 service 依赖) |
| `cListCtrlEx` | `cListCtrlEx` | InventoryExDialog, QuickDialog (Tier 3 blocked) | Tier 3 范围 |
| `cPage` | `cPage` | HelpDialog (page navigation) | 候选 |
| `cDialogueList` | `cDialogueList` | HelpDialog + Tier 4 NpcScript | 候选 (Tier 4 范围) |
| `cHyperTextList` | `cHyperTextList` | HelpDialog (rich text) | 候选 |

### Tier 2 — UI 状态 + 子控件编排（~200-500 行，2-3 commit/个）

**特征**：dialog 自身无状态，但有 2-5 个 cButton / cListCtrl / cEditBox
子控件需要 wire + 联动。需要先 port 子控件（多数已 port 在 src/ui/
下），主要工作量在 Linking() + callback 编排。

| Legacy Dialog | 现代目标 | 子控件 | 阻塞 |
|--------------|---------|-------|------|
| CharacterDialog | `cCharacterDialog` | cStatic, cButton, cEditBox, cListCtrl, **cGuagen** ✅ | 需 PlayerStatsService (Tier 3) |
| QuestDialog | `cQuestDialog` | cListCtrl, cButton, cStatic | 需 QuestService + 1 子控件 |
| ChatDialog | `cChatDialog` | cEditBox, cButton, cListCtrl | 需 ChatService (Tier 5) |
| DealDialog | `cDealDialog` | cStatic, cButton, cListCtrl | 需 inventory 状态 (Tier 3) |
| ExchangeDialog | `cExchangeDialog` | cStatic, cButton, cListCtrl | 需 inventory 状态 (Tier 3) |
| FriendDialog | `cFriendDialog` | cListCtrl, cButton | 需 network (Tier 5) |
| NoteDialog | `cNoteDialog` | cListCtrl, cEditBox | 需 network (Tier 5) |
| PartyDialog | `cPartyDialog` | cListCtrl, cButton | 需 network (Tier 5) |
| ItemShopDialog | `cItemShopDialog` | cButton, cStatic, cListCtrl | 需 shop data (Tier 3) |
| MacroDialog | `cMacroDialog` | cEditBox, cButton | 无（已 port 子控件） |

**重要更新（2026-07-16）**: 旧 roadmap 说 "无阻塞" 是错的。
CharacterDialog header 包含 cGuagen.h，**0.13.12 cGuagen port 解锁
CharacterDialog 子控件 blocker**。但 CharacterDialog 仍需 PlayerStatsService
（HP/MP/str/agi 等数据从哪读）— Tier 3 阻塞。

**小活建议**:
- 选 1 个**纯 UI 状态机** + **子控件全 port** + **无 service** 的 dialog
- 截至 0.13.12，唯一符合所有条件的是 **MacroDialog**（仅 cEditBox + cButton，OptionManager settings 已经是 local state）
- 第二个候选是 **DealDialog**（Tier 3 blocked after 0.13.12 解锁子控件）

### Tier 3 — 需要 GameIn/Inventory 状态（~500-1000 行，3-5 commit/个）

**特征**：dialog 通过 `GAMEIN->GetInventoryDialog()` 之类全局指针
读游戏状态。Modern port 需要：
1. 把 GameIn 拆成可注入的 service interface
2. dialog 通过 service interface 读状态而非全局指针
3. mock service 给单测用

| Legacy Dialog | 阻塞 |
|--------------|------|
| InventoryExDialog | 需 InventoryService 接口 |
| QuickDialog | 需 InventoryService + SkillService |
| MugongDialog | 需 SkillService + InventoryService |
| CharacterDialog | 需 PlayerStatsService |
| ItemShopDialog | 需 ShopService + InventoryService |
| GuildWarehouseDialog | 需 GuildService + InventoryService |
| PrivateWarehouseDialog | 需 InventoryService |
| MixDialog | 需 ItemService + InventoryService |
| BuyRegDialog | 需 ShopService + InventoryService |

**架构阻塞**：需要先做 InventoryService / SkillService / PlayerStatsService
等 5-8 个 service interface（每个 ~200 行 + mock + test）。这是
"现代服务化" 的分水岭工作，建议**单独开一个 phase**（Phase 13？），
不在 P2-12 内做。

### Tier 4 — 需要 NPC 脚本驱动（~800-2000 行）

**特征**：dialog 状态机由 NPC 对话脚本驱动，每次 NPC talk 会重写
dialog 内容 + 选项按钮。

| Legacy Dialog | 阻塞 |
|--------------|------|
| NpcScriptDialog | 需 NpcScriptEngine（未 port） |
| QuestDialog | 部分功能依赖 NpcScriptEngine |
| HelpDialog 部分页 | 依赖 quest data |

**架构阻塞**：NpcScriptEngine 是 2008-era 的自研脚本系统（5-6 个
class），完整 port 等价于重写一个小的 game scripting language。
**Phase 13+ 长尾**。

### Tier 5 — 网络协议驱动（~1000-3000 行）

**特征**：dialog 状态与网络包强耦合，server push / client ack 一来
一回，dialog 立刻 rebuild。

| Legacy Dialog | 阻塞 |
|--------------|------|
| GuildDialog | 已 port（1:1），但需 GuildService（Tier 3 阻塞） |
| GuildCreateDialog | 需 GuildService + network |
| GTBattleListDialog | 需 GTService（guild tournament 协议） |
| GTScoreInfoDialog | 需 GTService |
| FortWarDialog | 需 GuildWarService |
| SeigeWarDialog | 需 GuildWarService + MapService |
| MurimNet 系列（5 个） | 需 MurimNetService（PvP 协议） |
| GuildTraineeDialog | 需 GuildService |
| GuildFieldWarDialog | 需 GuildWarService |
| PartyWarDialog | 需 PartyService |
| GuildRankDialog | 需 GuildService |
| GuildMarkDialog | 需 GuildService + texture 协议 |
| GuildMunhaDialog | 需 GuildService |
| GuildNickNameDialog | 需 GuildService |
| GuildPlusTimeDialog | 需 GuildService |
| GuildLevelUpDialog | 需 GuildService |
| GuildJoinDialog | 需 GuildService |
| GuildInviteDialog | 需 GuildService |
| GuildInvitationKindSelectionDialog | 需 GuildService |
| ChangeJobDialog | 需 JobService + network |
| ChannelDialog | 需 ChannelService |
| ServerListDialog | 需 LoginService |
| CostumeSkinSelectDialog | 需 ItemService + texture |
| SkinSelectDialog | 需 ItemService + texture |
| PetUpgradeDialog | 需 PetService + network |
| PetWearedExDialog | 需 PetService + inventory |
| CharacterDialog 部分页 | 需 QuestService + PlayerStatsService |
| ChatDialog | 需 ChatService + network |
| MiniNoteDialog | 需 NoteService |
| MiniFriendDialog | 需 FriendService |
| ShoutDialog | 需 ChatService |
| ShoutchatDialog | 需 ChatService |
| SurvivalCountDialog | 需 GuildWarService + TimerService |
| WantNpcDialog | 需 WantService + network |
| WantRegistDialog | 需 WantService + network |
| WantedDialog | 需 WantService + network |
| DissolutionDialog | 需 GuildService + ItemService |
| PKLootingDialog | 需 InventoryService + LootService |
| PointSaveDialog | 需 PointService + network |
| PyoGukDialog | 需 PyoGukService + ItemService |
| MoveDialog | 需 MapService + network |
| ReviveDialog | 需 PlayerService + MapService |
| SealDialog | 需 ItemService + PlayerService |
| ItemShopGridDialog | 需 ShopService + InventoryService |
| MNFrontDialog | 需 MurimNetService |
| MNCreateDialog | 需 MurimNetService |
| MNJoinDialog | 需 MurimNetService |
| MNChannelDialog | 需 MurimNetService |
| MNPlayRoomDialog | 需 MurimNetService |
| MPGuageDialog | 需 PlayerService（实时刷新） |
| MPMissionDialog | 需 MissionService |
| MPNoticeDialog | 需 MissionService |
| MPRegistDialog | 需 MissionService + network |
| GuageDialog | 需 PlayerService（实时刷新） |
| GTRegistDialog | 需 GTService + network |
| GTRegistcancelDialog | 需 GTService + network |
| GTStandingDialog | 需 GTService |
| cDialogueList | 需 NpcScriptEngine（Tier 4） |
| ChaseDialog | 需 PlayerService（实时刷新） |
| ChaseinputDialog | 需 PlayerService |
| cJackpotDialog | 需 ItemService + network |
| cListDialogEx | 需 Service（泛型 list 增强） |
| JournalDialog | 需 JournalService + InventoryService |
| MugongSuryunDialog | 需 SkillService + InventoryService |
| SuryunDialog | 需 SkillService + InventoryService |
| HelpDialog 部分页 | 需 HelpService + 网络下载 |
| SOSDialog | 需 PlayerService + network |
| WearedExDialog | 需 EquipmentService + InventoryService |
| RareCreateDialog | 需 ItemService + InventoryService |
| MainDialog | 需 GameMainService（**最复杂**） |
| MainBarDialog | 需 GameMainService（**最复杂**） |
| MugongDialog | 需 SkillService + InventoryService + QuickBar |
| QuestTotalDialog | 需 QuestService |
| OptionDialog | 需 OptionService（settings） |

**架构阻塞**：需要 ~15-20 个 service interface + 完整 network
packet 解码（[CC]Header/Protocol.h 的 96 类 Category）+ 实时刷新
timer 机制。这是 Phase 14+ 范畴，**预计 4-6 周工作量**，本 roadmap
只标 TODO 不做。

## 文件归档 / 清理建议

| Legacy 文件 | 状态 | 建议 |
|------------|------|------|
| `MugongDialog_BACKUP.{h,cpp}` | 备份 | 记录在 R-12，本 session 不删 |
| `cDialogueList.{h,cpp}` | 看起来是 Tier 4 残留 | 暂留，port NpcScriptEngine 时一起处理 |

## 整体估算

| 档 | 数量 | 工作量 | 累计估算 |
|----|------|-------|---------|
| Tier 1.5 子控件 | 9+ | 100-200 行 + 测试 | 1 周（每次小活 1-2 个, 已完成 1/9: cGuagen） |
| Tier 1 (legacy "trivial") | 1 | 100 行 + 测试 | ✅ 已完成 (cExitDialog) |
| Tier 2 | 10 | 200-500 行 + 测试 | 2-3 周 |
| Tier 3 | 9 | 500-1000 行（需 service interface） | 3-4 周（依赖 Phase 13 service 化） |
| Tier 4 | 3 | 800-2000 行（需 NpcScriptEngine） | 2-3 周（依赖 Phase 14 script port） |
| Tier 5 | 100+ | 1000-3000 行（需 15-20 service） | 8-12 周（依赖 Phase 15 service + network） |
| **总计** | **131+** | — | **16-22 周** ≈ 4-5 个月 |

加上 base **6/202** = **3.0%**（当前）→ 100% ≈ 4-5 个月全职工作量。
**这是真的"长尾"，不靠 24 小时 AI 接力推不完。**

## 建议推进节奏

1. **每次新 session** 推 1-2 个 **Tier 1.5 子控件** widget（小活，1-2 commit）。
   下次 session 候选: cListDialogEx / cPushupButton / cListCtrl（确认 modern 存在）
2. **每 3-5 session** 评估一次 Tier 2（需要先确认子控件齐全 + 无 service 阻塞）
   - 当前可立即 port 的 Tier 2 dialog: **MacroDialog**（无 service, 子控件全 port）
3. **不开 Phase 13** 不动 Tier 3-5（架构阻塞）
4. **每 10 session** 更新一次本 roadmap（重新评估 Tier / 优先级）

## 0.13.12 更新摘要

- ✅ 新 ported: cGuagen (18 test, Tier 1.5 子控件) — 解锁 CharacterDialog 等多个
  Tier 2 dialog 的子控件 blocker
- ✅ R-9 矩阵约定审计完成: 7 个新 test pin D3DX row-major layout, 3 个旧 test 更新
- 📊 Progress: 5/202 = 2.5% → 6/202 = 3.0% (Tier 1.5 子控件算"组件 port",
  不算严格 dialog port, 但属于"无 service 阻塞的 ui 元素")

## 关联文档

- `docs/KNOWN_BUGS.md` R-12（roadmap + SetActive polymorphic bug）
- `AI_TASK_QUEUE.md` P2-12（每次完成一个 dialog 在此打勾）
- `MODERNIZATION_PLAN.md` Phase 6（dialogs 整体状态）
