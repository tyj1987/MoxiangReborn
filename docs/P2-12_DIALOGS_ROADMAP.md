# P2-12 Dialogs 移植 Roadmap

> **作者**：Mavis  
> **日期**：2026-07-16  
> **状态**：🟡 进行中（5/202 = 2.5%）  
> **关联**：`docs/KNOWN_BUGS.md` R-12、`AI_TASK_QUEUE.md` P2-12

## 背景

legacy `[Client]MH/` 目录下有 **202 个 dialog 文件**（含 1 个备份
`MugongDialog_BACKUP.{h,cpp}`，实际活跃 201 个）。现代 `modern/src/ui/`
只 port 了 **5 个**：`cDialog`（基类）、`cGuildDialog`、`cIconDialog`、
`cListDialog`、`cExitDialog`。一次推完 197 个 dialog 不现实——这
是 Phase 6 时代就遗留的 Phase 12 长尾任务。

本文把 202 个 legacy dialog 按"实现难度 + 依赖复杂度"分 5 档，每档
标 port 优先级 + 估计工作量 + blocker，让后续 session 按矩阵接活。

## 已 Port 列表（5/202 = 2.5%）

| 现代类 | 头文件 | 测试数 | 备注 |
|--------|-------|-------|------|
| `cDialog` | `modern/src/ui/cDialog.{hpp,cpp}` | 8 | 基类，Phase 6.3 |
| `cGuildDialog` | `modern/src/ui/cGuildDialog.{hpp,cpp}` | 13 | Phase 6 早期 |
| `cIconDialog` | `modern/src/ui/cIconDialog.{hpp,cpp}` | ? | Phase 6 |
| `cListDialog` | `modern/src/ui/cListDialog.{hpp,cpp}` | ? | Phase 6 |
| `cExitDialog` | `modern/src/ui/cExitDialog.{hpp,cpp}` | 10 | **2026-07-16**（本 session） |

## 5 档分级

### Tier 1 — Trivial（无外部依赖，1-2 commit/个，~80-200 行）

**特征**：纯 UI 状态机，不读 GameIn/不连网络/不调 NPC 脚本。
基类 cDialog 已能 cover 90%，子类只需 override SetActive/Init
+ 1-2 个 callback。**适合做小活起点**。

| Legacy Dialog | 现代目标 | 工作量 | 优先级 |
|--------------|---------|-------|-------|
| ExitDialog | `cExitDialog` ✅ | 100 行 + 10 test | **已完成** |
| HelpDialog | `cHelpDialog` | 80 行 + 6 test | P2-12a |
| BailDialog | `cBailDialog` | 120 行 + 8 test | P2-12b |
| EventNotifyDialog | `cEventNotifyDialog` | 150 行 + 8 test | P2-12c |
| MallNoticeDialog | `cMallNoticeDialog` | 100 行 + 6 test | P2-12d |
| ChatOptionDialog | `cChatOptionDialog` | 180 行 + 10 test | P2-12e |
| NameChangeDialog | `cNameChangeDialog` | 150 行 + 8 test | P2-12f |
| PetRevivalDialog | `cPetRevivalDialog` | 200 行 + 10 test | P2-12g |
| NumberPadDialog | `cNumberPadDialog` | 100 行 + 6 test | P2-12h |

**小活建议**：每次新 session 推 1-2 个，连续 3-4 session 把 Tier 1 扫完。

### Tier 2 — UI 状态 + 子控件编排（~200-500 行，2-3 commit/个）

**特征**：dialog 自身无状态，但有 2-5 个 cButton / cListCtrl / cEditBox
子控件需要 wire + 联动。需要先 port 子控件（多数已 port 在 src/ui/
下），主要工作量在 Linking() + callback 编排。

| Legacy Dialog | 现代目标 | 子控件 | 阻塞 |
|--------------|---------|-------|------|
| CharacterDialog | `cCharacterDialog` | cStatic, cButton, cEditBox, cListCtrl | 无 |
| QuestDialog | `cQuestDialog` | cListCtrl, cButton, cStatic | 无 |
| ChatDialog | `cChatDialog` | cEditBox, cButton, cListCtrl | 无 |
| DealDialog | `cDealDialog` | cStatic, cButton, cListCtrl | 需 inventory 状态 |
| ExchangeDialog | `cExchangeDialog` | cStatic, cButton, cListCtrl | 需 inventory 状态 |
| FriendDialog | `cFriendDialog` | cListCtrl, cButton | 需 network |
| NoteDialog | `cNoteDialog` | cListCtrl, cEditBox | 需 network |
| PartyDialog | `cPartyDialog` | cListCtrl, cButton | 需 network |
| ItemShopDialog | `cItemShopDialog` | cButton, cStatic, cListCtrl | 需 shop data |
| MacroDialog | `cMacroDialog` | cEditBox, cButton | 无 |

**小活建议**：选 1 个 + 先确认子控件都 port 了（cStatic / cButton /
cEditBox / cListCtrl / cPushupButton 多数已存在）。

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
| Tier 1 | 9 | 80-200 行 + 测试 | 1-2 周（每次小活 1-2 个） |
| Tier 2 | 10 | 200-500 行 + 测试 | 2-3 周 |
| Tier 3 | 9 | 500-1000 行（需 service interface） | 3-4 周（依赖 Phase 13 service 化） |
| Tier 4 | 3 | 800-2000 行（需 NpcScriptEngine） | 2-3 周（依赖 Phase 14 script port） |
| Tier 5 | 100+ | 1000-3000 行（需 15-20 service） | 8-12 周（依赖 Phase 15 service + network） |
| **总计** | **131+** | — | **16-22 周** ≈ 4-5 个月 |

加上 base **5/202** = **2.5%**（当前）→ 100% ≈ 4-5 个月全职工作量。
**这是真的"长尾"，不靠 24 小时 AI 接力推不完。**

## 建议推进节奏

1. **每次新 session** 推 1-2 个 **Tier 1** dialog（小活，1-2 commit）
2. **每 3-5 session** 评估一次 Tier 2（需要先确认子控件齐全）
3. **不开 Phase 13** 不动 Tier 3-5（架构阻塞）
4. **每 10 session** 更新一次本 roadmap（重新评估 Tier / 优先级）

## 关联文档

- `docs/KNOWN_BUGS.md` R-12（roadmap + SetActive polymorphic bug）
- `AI_TASK_QUEUE.md` P2-12（每次完成一个 dialog 在此打勾）
- `MODERNIZATION_PLAN.md` Phase 6（dialogs 整体状态）
