# 活动缺陷与外部阻塞

> 仅记录仍影响当前里程碑的事项。已解决和历史调查见 [KNOWN_BUGS_ARCHIVE.md](KNOWN_BUGS_ARCHIVE.md)，完成记录见 [CHANGELOG.md](CHANGELOG.md)。

## R-9：DX11 完整场景尚未完成原版对照

- **状态**：modern 闭环完成、legacy 视觉待验收；headless 3 帧可自然退出，像素门禁已锁定 grid、cube、checker 纹理与深度遮挡。
- **复现**：运行 `mxh_render_demo --headless --save-frame <path> --frame-count 3`，检查 grid、cube、纹理/HUD 是否同时出帧。
- **影响**：阻塞 Phase A/B 的视觉验收和后续 UI 截图对照。
- **验收**：modern 自动化门禁已完成；剩余要求为与原版登录/空场景截图的差异有明确结论。
- **外部依赖**：需要可运行原版客户端的对照环境。

## C-Tier-3: 业务 dialog 服务集成推进中

- **状态**: 活动中; 12/12 接线完成（超出原 9 项目标）。当前累计：cQuestDialog + cQuestTotalDialog (IQuestService), cDealDialog (IInventoryService + ITradeService), cItemShopDialog (IItemShopService), cFriendDialog (IFriendService), cMoveDialog (IMoveService), cExchangeDialog (IInventoryService + ITradeService), cGuildWarehouseDialog (IInventoryService), cInventoryExDialog (IInventoryService), cQuickDialog (IInventoryService + ISkillService), cCharacterDialog (IPlayerStatsService), cMPGuageDialog (IPlayerStatsService), cMugongDialog (ISkillService)。
- **复现**: 启动真实 service 路径，逐项打开已接入 dialog。
- **影响**: 阻塞 UI 1:1 集成验收，不能仅用"hpp 已 port"判定完成。
- **验收**: 服务接线 12/12 完成（超出原 9 项目标），截图验收仍需 legacy 客户端。
- **外部依赖**: 截图验收需要 legacy 客户端对照环境；运行时已无需依赖。

## E3：五段核心玩法行为尚未全部 diff=0

- **状态**: modern 侧 5/5 已 diff=0；2026-08-10 五段场景 modern capture 全部 byte-for-byte 匹配 modern golden：`login` (dist+login Ack), `enter_game` 2 帧 (AgentConnectSuccess + GameInNack), `attack` (Skill StartAck + SkillObjectAdd + SkillObjectRemove, dev_stub_caster 注入最小 player 触发 Ok 路径), `shop` (Item BuyNack 4B echo), `quest` (Quest StartNack 2B echo). 5/5 capture 中 attack 走 Ok 路径（StartAck 8B），shop / quest 走 Nack 路径（catalog / script 未加载），保持各自 diff=0。BuySyn / StartSyn 的 Ok 路径已在 commit 229bde0d 落地：dealitem catalog 命中 → 扣 money + 插 inventory + BuyAck；quest script 命中 → accept_quest + StartAck。Ok 路径由 `MapHandlerTest.BuySynOkArmDeductsMoneyAndInsertsInventory` + `MapHandlerTest.StartSynOkArmAddsQuestToPlayerLog` 端到端覆盖。modern caster data plane (`mxh::server::skill_caster`) 在 commit 4deb5529 独立模块化，15 个 `SkillCaster*` 单测覆盖 6 个 status 路径 + 1:1 damage 公式（dodge / phy / attr / crit×1.5） + heal 量。Nack 仍是 catalog/script 未加载时的回退路径，与 5/5 modern capture 兼容。
- **影响**：阻碍 T3 和 1.0 跨实现对照。
- **验收**：副作用顺序、网络包、数据库变化、数值和 UI 状态逐项一致。**当前 5/5 段在 modern 侧达到 diff=0; BuySyn/StartSyn 的 Ok 路径已在 modern 单测中验证 (commit 229bde0d). 剩余跨实现证据需要 legacy client / server 对照环境（不是阻塞项，可与商业冒烟并行推进）**。
- **外部依赖**: 原版客户端/服务端对照环境。

## DEPLOY-MSSQL：生产部署环境尚未验收

- **状态**：本机 modern 路径完成；ODBC Driver 18 已安装，适配器默认优先 18 并仅在 `IM002` 时回退 17。LocalDB 初始化、真实 schema roundtrip，以及客户端/Login/Agent/Map 五步 MSSQL E2E 已通过。
- **复现**：在干净 Windows/SQL Server 环境安装并运行商业冒烟。
- **影响**：不阻塞本机开发，阻塞商业部署声明。
- **验收**：无人值守安装、建库、登录、建角、进图、数据库 roundtrip、升级和回滚全部通过且无 schema 漂移。
- **外部依赖**：一套干净机或等价隔离环境；legacy `.bak` 不属于强制门禁。
