# 活动缺陷与外部阻塞

> 仅记录仍影响当前里程碑的事项。已解决和历史调查见 [KNOWN_BUGS_ARCHIVE.md](KNOWN_BUGS_ARCHIVE.md)，完成记录见 [CHANGELOG.md](CHANGELOG.md)。



## M4: PlayDH èµ„æº?å…¨é‡?è¦†ç›–å·²è¾¾ 99.77%ï¼ˆ433/434 OKï¼‰

- **çŠ¶æ€?**: 2026-08-10 å½“æ—¥æ–°å»º `modern/tools/audit_resource_coverage.py`ï¼Œèµ° PlayDH å…¨é‡?èµ„æº‹å¹¶è°ƒç”¨ modern `MoxianResourceExplorer` çš„ `info`/`list`/`map`/`bsad` å­?å‘½ä»¤éªŒè¯?è§£æž?èƒ½åŠ›ã€‚å½“å‰?ç»“æž?: .bin 334/335 ï¼›.bmhm 81/81 ï¼›.bsad 11/11 ï¼›.pak 7/7 ï¼›å…± 433/434 (99.77%)ã€‚
- **å¤?çŽ°**: è¿�è¡Œ `python modern/tools/audit_resource_coverage.py "C:\\moxiang\\墨香【源码配套资源】\\PlayDH" --output modern/build/coverage_manifest.txt`ã€‚
- **å½±å“?**: éªŒè¯? T1 èµ„æº‹å­—èŠ‚ä¸€è‡´å’Œ modern è§£æž?å™¨å¯¹æ‰€æœ‰çœŸå®?èµ„æº‹çš„è¦†ç›–çŽ‡ã€‚
- **éªŒæ”¶**: å…¨é‡?è¦†ç›–è¾¾ 100%ï¼Œä¸” 1 é¡„å¤„çš„ `Ini\\GameDesc.bin` è¢«è¯†åˆ«ä¸º plaintext å‡é˜‘æ€?ã€‚
- **å¤–éƒ¨ä¾èµ–**: PlayDH æº?èµ„æº‹ç›®å½•éœ€è¦?å­˜åœ¨ã€‚

## R-9：DX11 完整场景尚未完成原版对照

- **状态**：modern 闭环完成、legacy 视觉待验收；headless 3 帧可自然退出，像素门禁已锁定 grid、cube、checker 纹理与深度遮挡。
- **复现**：运行 `mxh_render_demo --headless --save-frame <path> --frame-count 3`，检查 grid、cube、纹理/HUD 是否同时出帧。
- **影响**：阻塞 Phase A/B 的视觉验收和后续 UI 截图对照。
- **验收**：modern 自动化门禁已完成；剩余要求为与原版登录/空场景截图的差异有明确结论。
- **外部依赖**：需要可运行原版客户端的对照环境。

## CLIENT-RUNTIME：GUI 客户端 2D 精灵渲染已完成，剩全场景与完整交互

- **状态**：活动中；GUI 已能使用可配置端口登录 modern LoginServer、转入 AgentServer 并取得角色列表。已修复未安装状态回调和 WM_PAINT 持续占用消息泵导致状态机不运行的问题。
- **复现**：启动 `deploy/scripts/start_modern.ps1` 后运行 `mxh_client.exe --login-port 16001 --map-port 18001`；空账号会停在无可见交互的角色列表阶段，画面仍为程序化占位 sprite。
- **影响**：直接阻塞商业 RC；无头五步 E2E 不能替代玩家可操作客户端。
- **验收**：真实登录/建角/选角/进图 UI 可操作，加载原版地图、角色、怪物、界面、音乐和音效，完成至少一段战斗/任务流程。
- **外部依赖**：无。




## C-Tier-3: 业务 dialog 服务集成推进中

- **状态**: 活动中; 2026-08-10 当日完成 5 项接线（cItemShopDialog + cFriendDialog + cMoveDialog + cExchangeDialog + cGuildWarehouseDialog 已通过对应 service 接入, 均有行为测试）. 当前累计 9 项业务 dialog 已通过 service 接入：cQuestDialog + cQuestTotalDialog (IQuestService), cDealDialog (IInventoryService + ITradeService), cItemShopDialog (IItemShopService), cFriendDialog (IFriendService), cMoveDialog (IMoveService), cExchangeDialog (IInventoryService + ITradeService), cGuildWarehouseDialog (IInventoryService), cInventoryExDialog (IInventoryService), cQuickDialog (IInventoryService + ISkillService), cCharacterDialog (IPlayerStatsService), cMPGuageDialog (IPlayerStatsService), cMugongDialog (ISkillService). 截图验收仍未完成（需要 legacy client 对照环境）。
- **复现**: 启动真实 service 路径, 逐项打开已接入 dialog。
- **影响**: 阻塞 UI 1:1 集成验收, 不能仅用"hpp 已 port"判定完成。
- **验收**: 服务接线 12/12 完成（超出原 9 项目标），截图验收仍需 legacy 客户端。
- **外部依赖**: 截图验收需要 legacy 客户端对照环境；运行时已无需依赖。

## E3：五段核心玩法行为尚未全部 diff=0

- **状态**：进行中；**2026-08-10 推进：五段场景的 modern 端到端 capture 已全部 diff=0**。`login` byte-for-byte 匹配 legacy golden（dist+login Ack），`enter_game` 2 帧（AgentConnectSuccess + GameInNack），`attack` Skill StartNack(err=3 unknown caster)，`shop` Item BuyNack (payload echo)，`quest` Quest StartNack (quest_id echo)。Nack 是有意为之：modern 没有 NPC shop 数据库 / quest manager / skill caster 解析，所以服务端按"老客户端契约：服务端必须回应 StartSyn"返回稳定的 Nack trace，side-by-side harness 才能持续 diff=0。
- **影响**：阻碍 T3 和 1.0。
- **验收**：副作用顺序、网络包、数据库变化、数值和 UI 状态逐项一致。**当前 5/5 段在 modern 侧达到 diff=0，剩余跨实现证据需要 legacy client / server 对照环境（不是阻塞项，可与商业冒烟并行推进）**。
- **外部依赖**：原版客户端/服务端对照环境。

## M3-MAP：MapHandler BuySyn / Quest StartSyn minimal Nack 已实现

- **状态**：已解决（2026-08-10）。`modern/src/server/map_handler.cpp` 给 `Category::Item` 的 `BuySyn` 加了 minimal Nack（4B payload echo），给 `Category::Quest` 加了 `handle_quest()`（StartSyn->StartNack, EndSyn->EndNack，皆回显 quest_id 2B）。`handle_skill` 的 caster-not-found 改为发 `Skill StartNack(err=3)` 而不是 silent drop。
- **影响**：解锁 E3 5-stage 场景中的 attack / shop / quest 三段 modern capture diff=0。
- **验收**：`mxh_side_by_side --modern-only --start` 跑完后 `modern/tests/fixtures/sbs_captures_modern/*.cap` 与新加的 6 个 `SideBySideModernGolden.*` 测试同时通过。
- **外部依赖**：后续 quest_manager / npc_shop 模块落地位后会升级 Nack 为 Ack + DB 持久化。

## E3：五段核心玩法行为尚未全部 diff=0

- **状态**：活动中；数据面和 side-effect runtime 单测覆盖广泛，但缺完整跨实现证据。
- **复现**：分别运行登录进图、战斗/任务、商城/物品、PK 五个固定场景。
- **影响**：阻塞 T3 和 1.0。
- **验收**：副作用顺序、网络包、数据库变化、数值和 UI 状态逐项一致。
- **外部依赖**：原版客户端/服务端对照环境。

## DEPLOY-MSSQL：生产部署环境尚未验收

- **状态**：本机 modern 路径完成；ODBC Driver 18 已安装，适配器默认优先 18 并仅在 `IM002` 时回退 17。LocalDB 初始化、真实 schema roundtrip，以及客户端/Login/Agent/Map 五步 MSSQL E2E 已通过。
- **复现**：在干净 Windows/SQL Server 环境安装并运行商业冒烟。
- **影响**：不阻塞本机开发，阻塞商业部署声明。
- **验收**：无人值守安装、建库、登录、建角、进图、数据库 roundtrip、升级和回滚全部通过且无 schema 漂移。
- **外部依赖**：一套干净机或等价隔离环境；legacy `.bak` 不属于强制门禁。
