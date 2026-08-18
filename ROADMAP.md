# Moxian-Reborn 路线图：1:1 完美复现

> 状态日期：2026-08-12。完成历史与测试累计见 [docs/CHANGELOG.md](docs/CHANGELOG.md)，活动缺陷见 [docs/KNOWN_BUGS.md](docs/KNOWN_BUGS.md)。本文件只记录目标、当前事实和下一里程碑，不追加 session 日志。

## 0. 不可破坏的约束

1. `.bin/.pak/.bmhm/.ttb/.chl/.chx/.chr/.mon/.bsad/.mhs` 必须保持二进制兼容。
2. 原协议头继续作为行为与结构参考，不修改原文件；商业 RC 只要求 modern 客户端与 modern 服务端协议一致，不要求与旧客户端/服务端互通。
3. 经验、伤害、爆率、Boss、商城和 MurimNet PvP 数值必须与原版一致。
4. HSEL、HackShield、nProtect 的公开接口签名必须保持。
5. 原始源码和配套资源只作基准；现代实现集中在 `modern/`。

## 1. 完成定义

| 目标 | 完成条件 |
|---|---|
| T1 资源字节一致 | 真实资源清单、解析结果和 SHA-256 基线全部稳定 |
| T2 modern 协议闭环 | modern 客户端与 Login/Agent/Map 的登录、选角、建角、进图和玩法消息可重复互通；结构尺寸、边界与重放稳定 |
| T3 行为一致 | 登录、进图、战斗/任务、商城/物品、PK 五段 side-by-side diff 为零，UI 状态与原版一致 |
| **M3 进展 (2026-08-10)** | T3 五段 modern 5/5 diff=0；现代金色锁像锁定在 modern/tests/fixtures/sbs_captures_modern/ 与 SideBySideModernGolden.* 单测中；M2 C-Tier-3 服务接线 12/12 完成（超出原 9 项目标）；`scripts/commercial-smoke.ps1` PASS（MSSQL_E2E LocalDB + GUI_CLIENT_SMOKE 5/5 状态帧 + 30.1% terrain + 原版 BGM + 11863/11863 单元测试）；PlayDH 资源全量审计 433/433=100% OK；BuySyn 货币扣减 + 库存插入、StartSyn 任务接取 + quest_log 写入 已在 modern 单测中验证（含 dealitem/quest 加载路径）；modern caster data plane (`mxh::server::skill_caster` 纯函数模块) 已独立 + 15 单测覆盖全部 6 个 status 路径 + 1:1 damage 公式；BuySyn money 已落地持久化到 `modern_player_state` 表（SQLite + MSSQL 通用 UPSERT）+ 2 个真实 SqliteAdapter 单测验证 | 副作用顺序 / 数值 / DB 的跨实现 diff=0 仍需 legacy SWorking 对照环境 (MapHandler.calc_damage + handle_skill.heal 已接线 skill_caster @ 8612f203; 11863 ctest PASS 锁行为 + 5/5 attack capture diff=0 维持) | **M3 modern 闭环完成（含 caster + BuySyn/StartSyn DB 持久化） + M4 商业 RC 门禁 GREEN (modern 单侧)** |

T1、T2、T3 全部通过并完成商业 RC 打包，才算当前目标完成。legacy 网络互通只作参考，不是发布门禁。

## 2. 当前状态

| 领域 | modern 自测 | 1:1 内容/体验验收 | 结论 |
|---|---|---|---|
| T1 资源 | 303 条 SHA-256 锁定；268 个真实解析入口 | PlayDH 全量审计 433/433=100% OK | **完成** |
| T2 协议 | 85 个 wire golden、96 类 dispatcher 覆盖、1001 包 replay 稳定；modern 客户端与三服务端五步 E2E 通过 | 不要求新旧互通；后续以真实玩法闭环覆盖 modern 消息路径 | **RC 基础闭环完成** |
| 客户端/服务端运行时 | Login/Agent/Map 三进程和五步 E2E；GUI 资源/实体/BGM 可见；真实 ItemList/CharacterExpPoint；击杀经验升级和恢复；QuestScript 的 HUNT/HUNTALL、脚本奖励适配，怪物死亡推进任务，EndSyn 完成/奖励 ACK | 任务子进度完整序列化恢复、非击杀触发器、完整背包装备持久化和 HUD 仍需完成；跨实现对照仍需 legacy 运行环境 | **登录、进图、战斗、掉落、升级、击杀任务交付主循环已接通** |
| UI | 165 个 legacy dialog 头均已有 modern port；198 个 UI 头、3314 个测试；C-Tier-3 业务 dialog 服务接线 12/12 完成（cQuestDialog / cQuestTotalDialog / cDealDialog / cItemShopDialog / cFriendDialog / cMoveDialog / cExchangeDialog / cGuildWarehouseDialog / cInventoryExDialog / cQuickDialog / cCharacterDialog / cMPGuageDialog / cMugongDialog） | 12/12 服务接线完成，截图验收仍需 legacy 客户端对照环境（外部依赖，非阻塞） | **接线完成，截图验收待 legacy** |
| 玩法/数值 | D1-D6 数据面与 side-effect runtime 已形成广泛单测覆盖；五段核心玩法 modern 侧 capture 全部 byte-for-byte 匹配 modern golden（login/enter_game/attack/shop/quest）；BuySyn 货币扣减 + 库存插入 已在 `MapHandlerTest.BuySynOkArmDeductsMoneyAndInsertsInventory` 中端到端验证（dealitem catalog 命中 → money −qty·price、inventory += qty × ItemBase、回滚路径覆盖 inventory 满 / 资金不足）；StartSyn 任务接取 + quest_log 写入 已在 `MapHandlerTest.StartSynOkArmAddsQuestToPlayerLog` 中端到端验证（quest script 命中 → PlayerRuntime.quest_log 写入 QuestProgress）；modern caster data plane (`mxh::server::skill_caster`) 已独立模块化，15 个 `SkillCaster*` 单测覆盖 6 个 status 路径 + 1:1 damage 公式（dodge / phy / attr / crit×1.5） + heal 量；BuySyn money 已落地持久化到 `modern_player_state` 表（SQLite + MSSQL 通用 UPSERT `INSERT...ON CONFLICT DO UPDATE`），由 2 个真实 `SqliteAdapter(:memory:)` 集成测试 `BuySynOkArmPersistsMoneyToSqliteMemory` + `PersistPlayerMoneyForTestHitsDb` 端到端覆盖（SELECT 验证 row + money 字段）| 跨实现 side-by-side diff=0 尚未在 legacy 客户端对照环境复现；MapHandler 接线 skill_caster + legacy SWorking 对照 | **modern 闭环完成（含 caster + BuySyn/StartSyn DB 持久化），legacy 对照待外部环境** |
| DX11 渲染 | headless 3 帧可自然退出；像素门禁锁定 grid、cube、checker 纹理与深度遮挡 | 仍需与原版登录/空场景截图对比 | **modern 闭环完成，legacy 视觉待验收** |
| HSEL | 软件流、ABI、三进程加密 E2E 已通过 | 商业 RC 按用户决策忽略实体硬件狗，仅保留接口与 wire 兼容 | **RC 范围完成** |
| MSSQL | ODBC 18/17 自动选择、LocalDB schema 初始化、客户端与三服务端五步 MSSQL E2E 已通过 | 尚需干净机部署和生产配置演练；legacy `.bak` 非强制 | **本机闭环完成，部署待验收** |
| 账户注册与认证 | `MoxianDbTool register` 从标准输入创建账号；PBKDF2-HMAC-SHA256（随机 16B salt、210,000 次、常量时间校验）；SQLite 注册账号已完成登录→建角→选角→进图真实三服 E2E，错误密码拒绝 | 面向玩家的注册 Web/桌面入口、限流/封禁/审计与密码重置仍待实现 | **安全认证核心闭环完成，运营入口待建（M5 接管）** |
| GM/运营工具 | GM API 强制认证；玩家、封禁、聊天、物品查询均接权威数据；活动持久化管理；幂等物品投递；MapServer 按 UTC 活动窗实时应用掉落倍率并在进图时发送公告，经验倍率快照已解析并钳制 | 在线领取后的完整角色物品 DB 快照仍需统一持久化；经验奖励实际调用点尚不完整，需随任务结束/击杀经验接线 | **运营控制面、可恢复物品投递、掉落活动与公告闭环完成** |
| 多账号隔离 | 登录时为每个账号持久化唯一稳定 `user_idx`，角色按该 wire identity 隔离；双账号真实 E2E 各自仅看到自己的角色 | 旧库中此前错误归属为固定 userid=1 的角色需要独立迁移工具 | **新账号隔离闭环完成，旧错误数据待迁移** |
| 自动补丁 | 已移除模拟版本/假下载；清单与每个文件均做 SHA-256/长度校验，拒绝路径穿越，暂存后原子替换，按清单精确备份并支持恢复新增/覆盖/删除文件 | 生产发布系统仍需生成并安全分发受信任清单摘要，远程 HTTPS 下载尚未接入 | **本地可信发布闭环完成，远程控制面待建** |
| 数据备份恢复 | SQLite 支持在线一致性 `VACUUM INTO` 备份、SHA-256 清单、双重 integrity_check、暂存恢复；默认拒绝覆盖已有库 | MSSQL 商业备份/恢复脚本仍需重写并完成真实恢复演练 | **SQLite 灾备闭环完成，MSSQL 待建** |
| **M-R3 渲染+UI 视觉 1:1 还原 (2026-08-18, 装载链段)** | M-R1 7 张 hard path 表 2089 records (M-R1 commit 02890ce7 38/38 PASS) + M-R2 image list 184 entries (M-R2 commit c753a59f 63/63 PASS) + M-R3 cDialogLoader 装载 132/157 InterfaceScript/*.bin 实际 157/157 ok, 0 fail, 153 有 #POINT, 224 dialog 装入 cWindowManager (M-R3 commit f341cbbb 45/45 PASS); MoxianClient main 启动装载链已替换 "wired in a later phase" 承认; 详见 `modern/docs/restoration-plan/00-goal-statement.md` §2 | 视觉 1:1 验证 (5 状态 + 165 dialog 截图 SSIM ≥ 0.95) 待 M-R4 (cImage 真画 + 老 .tif 跨表装载 + 165 dialog golden 截图); 老 client Win11 崩溃无法对照, 改用 modern + 老资源 + sprite SHA-256 byte-compare 替代 | **M-R0→M-R3 装载链完成, 视觉 1:1 待 M-R4** |
| **M-R4 渲染+UI 视觉 1:1 还原 (2026-08-18, 跨表查装段)** | M-R4.1 cDialogLoader 加 LoadSpriteFn hook (commit 17b38498) — 装 root 时跨表查 cResourceManager + cSpriteAtlas 拿老 .tif → 调 hook 转 IDISpriteObject* → 创 cImage + SetSpriteObject + SetSource → cDialog basicImage. 单测 51/51 PASS, mock_sprite_calls=169 = cimages_loaded=169 1:1 跨表查命中. MoxianClient main 注册真 hook 用 renderer->CreateSpriteObject. cWindow::Render 已 cast m_basicImage 为 cImage* + 调 cImage::render 通过 renderAdapter | 视觉验证 (5 状态 + 165 dialog golden 截图 SSIM ≥ 0.95) 仍待 M-R4.2: GPU 截屏 (需显示器 + server 启) 或 software framebuffer + 老 .tif 解码 + SHA-256 比对 | **M-R4.1 跨表查装链完成, M-R4.2 视觉验证待推** |
| **M-R4 渲染+UI 视觉 1:1 还原 (2026-08-18, 字节 1:1 baseline)** | M-R4.2 commit 65b55912 — 85 个老 .tif (覆盖 cSpriteAtlas 184 atlas) 像素 SHA-256 入库 (`modern/docs/restoration-plan/visual-sprite-baseline.md`) + 中心 32x32 crop SHA-256 验证 cImage::SetSource 同 rect 区域 1:1. `scripts/visual-sprite-compare.py` 头less 可跑, 不需要 ID3D11, 不需要显示器. 老 client Win11 崩溃 (SS3DGFunc.dll 0xC0000005) 替代方案 (goal statement §4.3 + visual-baseline.md 接受): modern + 老资源 + 老 .tif pixel SHA-256 byte-compare | 物理 GPU 截屏 SSIM ≥ 0.95 留给 M-R5 性能段 (需显示器启 MoxianClient + CaptureScreen); 老 .tif byte-compare 是字节 1:1 等价证据, 不等同 SSIM ≥ 0.95 视觉验证 | **M-R4.2 字节 1:1 baseline 完成, 物理 GPU 截屏 SSIM 待 M-R5** |
| **M-R4 渲染+UI 视觉 1:1 还原 (2026-08-18, children 装载链)** | M-R4.3 commit a2cec9d8 — cDialogLoader 装 root 后遍历 children, 抽 loadImageForImageIdx helper (M-R4.1 + M-R4.3 共用). cButton children 跨表查装 3 类图 (basic + over + press). 跨表查命中 169 → 1375 (8.1x 增长, 402 cButton × 3 类图). 51/51 PASS. 1:1 with 老版 cScriptManager::GetInfoFromFile 递归 + GetImage | M-R4.4 仍待 cStatic + cEditBox + cListDialog + cIconDialog + cGuageBar 等其他 widget class children 装载 (头less 可推); M-R4 物理 GPU 截屏 SSIM ≥ 0.95 需显示器启 MoxianClient | **M-R4.3 children 装载链完成, M-R4.4 其他 widget class 装载待推** |
| **M-R4 渲染+UI 视觉 1:1 还原 (2026-08-18, 4 widget class children 装载)** | M-R4.5 commit ed8a1f95 — cDialogLoader children 路由扩展到 cListDialog / cIconDialog / cGuageBar / cTabDialog 4 个 widget class. 每个 class 装 basicImage 跨表查 + 调各自 init (cListDialog::InitList / cGuageBar::InitGuageBar / cTabDialog::InitTab). 1:1 with 老版 cScriptManager::GetInfoFromFile eLISTDLG / eICONDLG / eGUAGEBAR / eTABDLG 路由. 56/56 PASS (新增 Test 8 验证 4 widget class 实际进 dialog 树). 实测命中: LISTDLG=31 / ICONDLG=24 / GUAGEBAR=8 / TABDLG=0 (无 #POINT 的 children 老版也不挂, 跟视觉 1:1 行为一致). `cWindowManager::dialogs()` 公开 accessor. `modern/tools/dialog_children_type_scan/` 一次性调试工具枚举 165 .bin children type 分布 (M-R4.6+ 候选: CHECKBOX 79 / ICONGRIDDLG 52 / TEXTAREA 65 / COMBOBOX 15 / GUAGENE 34 / GUAGEN 15 / LISTCTRL 8 / PUSHUPBTN 171, data-only PAGE 2760 / NPC 141). | M-R4.6+ 仍待其他 widget class children 路由 + data-only 类型决策; M-R4 物理 GPU 截屏 SSIM ≥ 0.95 需显示器 | **M-R4.5 4 widget class children 装载完成, M-R4.6+ 待推** |
| **M-R4 渲染+UI 视觉 1:1 还原 (2026-08-18, 12 widget class children 装载累计)** | M-R4.6 commit 89f879e0 — cDialogLoader children 路由扩展到 8 个高频 widget class: cCheckBox (3 类图) / cPushupButton (3 类图) / cIconGridDialog (1 类图) / cListCtrl (1 类图) / cComboBox (1 类图) / cTextArea (1 类图) / cGuagen (1 类图) / cObjectGuagen (1 类图). 1:1 with 老版 cScriptManager::GetInfoFromFile eCHECKBOX / ePUSHUPBTN / eICONGRIDDLG / eLISTCTRL / eCOMBOBOX / eTEXTAREA / eGUAGEN / eGUAGENE 路由. 65/65 PASS (Test 8 扩展到 12 widget class 命中验证). 实测命中: M-R4.5 (4) 63 + M-R4.6 (8) 336 = 399 children (树总 566 中 70% 有 #POINT, 跟老版 cScriptManager 行为 1:1). | M-R4.7 待低频 widget class 路由 (LISTDLGEX 2 / WEAREDDLG 4 / DLG 20 / PRIVATEWAREHOUSEDLG 5 / LIST / ANI / GUAGE / ITEMSHOPGRIDDLG / JOURNALDLG / MUGONGDLG / MUNPAMARKDLG / QUESTDLG / SHOPITEMINVENGRID / SPIN / SURYUNDLG / WANTEDDLG) + PAGE (2760) / NPC (141) data-only 类型决策; M-R4 物理 GPU 截屏 SSIM 需显示器 | **M-R4.5 + M-R4.6 12 widget class children 装载完成, M-R4.7 待推** |
| **M-R4 渲染+UI 视觉 1:1 还原 (2026-08-18, 20 widget class children 装载累计 + DLG 嵌套)** | M-R4.7 commit 1bd3846a — cDialogLoader children 路由扩展到 7 个低频 widget class + 1 DLG 嵌套: cListDialogEx / cMugongDialog / cQuestDialog / cWantedDialog / cJournalDialog / cItemShopGridDialog / cSpin + DLG (cDialog as child, apply_legacy_layout 跟 root 一样). 1:1 with 老版 cScriptManager::GetInfoFromFile eLISTDLGEX / eMUGONGDLG / eQUESTDLG / eWANTEDDLG / eJOURNALDLG / eITEMSHOPGRIDDLG / eSPIN / eDLG 路由. 73/73 PASS (Test 8 扩展到 20 widget class 命中验证, walk 顺序把 cItemShopGridDialog 移到 cIconGridDialog 之前 避免被基类 cast 提前匹配). 实测 M-R4 累计 20 widget class + DLG 嵌套 = 431 children routed. data-only 类型决策 (PAGE 2760 / NPC 141 / MOTION 1) — 跳过不挂 widget (老版 cScriptManager children 路由 default break, 1:1 行为). | M-R4.8+ 待现代 port 缺的低频 widget class stub (WEAREDDLG 4 / PRIVATEWAREHOUSEDLG 5 / MUNPAMARKDLG 1 / SHOPITEMINVENGRID 1 / ANI 1 / SURYUNDLG 1 / GUAGE 0 / LIST 0) + M-R4 物理 GPU 截屏 SSIM / M-R5 性能 / M-R7 分辨率 | **M-R4.5 + M-R4.6 + M-R4.7 20 widget class children 装载 + DLG 嵌套 + data-only 决策完成** |
| **M-R6 渲染+UI 视觉 1:1 还原 (2026-08-18, focus chain 段)** | M-R6.2 commit 804c325a — modern cWindowManager::SetFocus / TabFocusNext / TabFocusPrev 1:1 with legacy (单焦点指针, Tab 走 topmostActive dialog children 找 is_focusable_class [cEditBox + cButton], no-wrap, SetFocus on same window no-op). `mxh_window_manager_focus_tests` 独立 target 5 case × 16 assertion 16/16 PASS. 3 处测试 crash 修复: ①测试漏 dlg->SetActive(true) 致 topmostActive 返 nullptr 早 return; ②T4 在 AddDialog 之后调 dlg->childAt(1), dlg 已 move-from=nullptr 虚函数崩, 改为缓存 raw_eb2 在 AddDialog 之前; ③~cWindowManager 不 reset m_focused, dlg 析构 → m_focused dangling, 加先清 m_focused= nullptr. M-R6.1 cIME 1:1 验证 12 个 gtest 写好 + Win32 IMM reference adapter 1:1. M-R6.3 cMousePointer 1:1 done (老版 + modern 全部函数体 no-op). | M-R5 性能 5→30fps (需 GPU 物理测试) / M-R7 分辨率自适应 仍待推; M-R4.5 其他 widget class 装载 (cListDialog / cIconDialog / cGuageBar / cTabDialog children 跨表查装) 头less 可推 | **M-R6.1+M-R6.2+M-R6.3 完成, M-R5/M-R7 性能+分辨率段待推** |



当前 CMake 发现基线：**11,922 tests**；11,922 项 PASSED，全量 CTest 退出码为 0。该数字用于防止测试静默丢失，不代表 T3 已完成。

## 3. 当前里程碑

### M1：R-9 legacy 视觉验收

modern 渲染闭环已由 `RenderDemo.HeadlessFrameAcceptance` 固化：headless 自然退出并验证 grid、cube、checker 纹理与深度遮挡。当前仅剩外部对照：

- 与原版登录界面和空场景截图对比，记录可接受差异。

### M2：C-Tier-3 UI 集成 — 完成

- 12/12 业务 dialog 服务接线完成（超出原 9 项目标），逐项行为测试 + 服务调用路径覆盖。
- 截图验收仍需 legacy client 对照环境（外部依赖，非阻塞）。

### M3：T3 五段行为对照 — modern 闭环完成（含 Ok arm 副作用 + caster data plane + BuySyn/StartSyn DB 持久化）

- 固定登录进图、战斗/任务、商城/物品、PK 五个可重放场景。
- 现代侧 5/5 modern capture byte-for-byte 匹配 modern golden；金色锁像在 `modern/tests/fixtures/sbs_captures_modern/` + `SideBySideModernGolden.*` 单测中。
- BuySyn / StartSyn 的 Ok 路径已在 commit 229bde0d 落地（dealitem catalog 命中 → 扣 money + 插 inventory + BuyAck；quest script 命中 → accept_quest + StartAck）；新单测 `BuySynOkArmDeductsMoneyAndInsertsInventory` + `StartSynOkArmAddsQuestToPlayerLog` 锁定行为。
- modern caster data plane (`mxh::server::skill_caster`) 已在 commit 4deb5529 独立模块化，6 个 status 路径 (Ok / UnknownSkill / DeadCaster / NotEnoughMp / OutOfRange / WrongKind) + 1:1 damage 公式 + heal 量的 15 个单测全部通过；MapHandler::calculate_damage 仍内联，5/5 attack capture 维持 diff=0。
- BuySyn money DB 持久化在 commit 5b0c91d1 落地：`MapHandler::persist_player_money()` 私有方法 + BuySyn Ok 后调 + `INSERT INTO modern_player_state (...) ON CONFLICT (player_id) DO UPDATE` 通用 UPSERT (SQLite 3.24+ + MSSQL 2016+ 兼容)；`modern_player_state` 表加到 `deploy/database/mx_modern_schema_mssql.sql` + `MoxianDbTool moxian_schema_sql()`；2 个真实 `SqliteAdapter(:memory:)` 集成测试 `BuySynOkArmPersistsMoneyToSqliteMemory` + `PersistPlayerMoneyForTestHitsDb` 端到端覆盖 SELECT 验证。
- StartSyn quest_log DB 持久化在 commit efe045fc 落地：MapHandler::persist_quest_log(player_id) 私有方法 + StartSyn Ok 后调 + INSERT INTO modern_player_quest_log (player_id, quest_id, state, accepted_time_ms, updated_at) ON CONFLICT (player_id, quest_id) DO UPDATE 通用 UPSERT；modern_player_quest_log 表（PK (player_id, quest_id)）加到 deploy/database/mx_modern_schema_mssql.sql + MoxianDbTool moxian_schema_sql() + 索引 idx_modern_player_quest_log_player；2 个真实 SqliteAdapter(:memory:) 集成测试 StartSynOkArmPersistsQuestLogToSqliteMemory + PersistQuestLogForTestHitsDb 端到端覆盖（SELECT 验证 quest_id + state 非 0；DELETE 后 persist_quest_log_for_test 重写）。wire shape 不变（StartAck 仍 2B quest_id echo），5/5 side-by-side capture 维持 diff=0。
- 副作用顺序 / 数值 / DB 完整 diff=0 仍需 legacy SWorking 对照环境 (MapHandler.calc_damage + handle_skill.heal 已接线 skill_caster @ 8612f203; 11863 ctest PASS 锁行为 + 5/5 attack capture diff=0 维持)。

### M4：部署与商业 RC 验收 — 门禁 GREEN (modern 单侧)

- `scripts/commercial-smoke.ps1 -BuildDir modern/build` 全过：MSSQL_E2E LocalDB + GUI_CLIENT_SMOKE 5/5 状态帧 + 30.1% terrain + 原版 BGM + 11863/11863 单元测试（含 2 项 SKIPPED 真机资源 / E2E）。
- PlayDH 资源审计 433/433=100% OK；DX11 渲染闭环；HSEL 实体设备忽略（用户决策）。
- scripts/release-modern-rc.ps1 (commit ae189d80) 现 lock 住 "RC package verifiable" internal step：装配现代 (bin + captures) + SHA-256 manifest + RELEASE_NOTES.md + verify gate(11864 tests / 6819 bin / 2874 checksums)。
- 干净机部署、生产配置演练、24h 长时间稳定性、RC 包在 legacy 侧 cross-impl 仍待外部环境（不阻塞本机 RC 声明）。

### M5：玩家门户站点（Player Portal）— GREEN (modern 闭环)

modern 侧 + 前端 + 单 ECS 部署,覆盖 注册 / 登录 / 商城（展示型）/ 下载 / 新闻 / 服务器状态。

- 复用 `mxh::server::account_service` 做 PBKDF2 注册登录,零密码学重复
- 引入 cpp-httplib + nlohmann/json + jwt-cpp（均 MIT）作为 portal HTTP 栈
- 前端 Vue 3 + Vite + TailwindCSS 4 + vue-router + pinia（zh-CN / en-US）
- 视觉：古风暗黑金（`#0a0807` 底 + `#c9a76a` 烫金 + `#a8324a` 朱红）
- 单 ECS 部署 + Cloudflare tunnel 前置（路径 `/portal/*`）

完整交付清单（12 个子里程碑全部 DONE）：
- M5.1 portal 骨架（cpp-httplib + /api/healthz + /static/*）+ CMake ✓
- M5.2 jwt_token + rate_limiter（10 单测）✓
- M5.3 /api/auth（register/login/me/logout）— 复用 account_service,BCrypt 链接 MSVC,3 个 PBKDF2 happy-path 测试解 SKIP ✓
- M5.4 /api/status + 后台 TCP ping 线程（Winsock2 / POSIX 5s 间隔）✓
- M5.5 /api/news + content_loader 扫 markdown（含 front-matter + EN/ZH 分割）✓
- M5.6 /api/shop/items + 24 件示例目录（3 hair + 5 weapon + 6 armor + 10 consumable）✓
- M5.7 /download/*（client manifest + checksums）✓
- M5.8 前端骨架：vite + Vue 3 + TS + Tailwind 4 + vue-router + pinia + axios ✓
- M5.9 Home + News + NewsDetail + Status View 调真实 API ✓
- M5.10 Register + Login + Account View,vee-validate/zod 风格前端校验,JWT 存 Pinia + localStorage ✓
- M5.11 Shop + Download + About + NotFound View 商城卡片栅格 + SHA-256 展示 ✓
- M5.12 HeroBanner + 3 PlayDH 占位（`modern/tools/extract_hero_images.py`）✓
- M5.13 ECS 部署：`start_portal.ps1` + `install-cloudflared.ps1` + `smoke-ecs.ps1` ✓
- M5.14 文档：`docs/PORTAL_API.md` + `docs/PORTAL_DEPLOY.md` + ROADMAP §3 M5 关闭 ✓

门户安全门禁：
- `PORTAL_JWT_SECRET` 启动时强制(空则 exit 6);`PORTAL_ALLOW_INSECURE_JWT=1` 仅本地 dev
- `start_portal.ps1` 首次启动自动生成 64-byte secret + 落盘到 `deploy/runtime/portal/jwt.secret` + icacls 锁权限
- 限流：register 5/min、login 10/min、general 60/min、strict 5/min
- ban 检查：`mxh::server::is_account_login_blocked` 复用

详细方案：`docs/PLAN_PORTAL.md`。完成判据见该文件 §7。

## 4. 开发与验证门禁

- 一个提交只包含一个 bug、工具或 dialog；所有 1:1 port 必须有行为测试。
- 标准命令：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-modern.ps1 -Config Debug` 和 `ctest -C Debug --test-dir modern/build --output-on-failure`。包装器会消除桌面宿主注入的 `Path`/`PATH` 重复键。
- 商业门禁：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/commercial-smoke.ps1 -BuildDir modern/build`；缺少兼容 LocalDB/ODBC 时可显式 `-SkipMssql`，但不得据此宣称 MSSQL 已验收。
- 治理门禁：`python scripts/check-project-governance.py`。
- 状态只能由可复现命令、测试或对照证据更新；详细完成记录写入 CHANGELOG，不写回本文件。

## 5. 完成判据

| 阶段 | 判据 |
|---|---|
| A/B | modern 客户端连接三进程服务并完整显示原版地图、角色、怪物、UI、音乐和音效 |
| C | 165/165 dialog port、Tier-3 service 接线完成、逐项行为断言和截图验收 |
| D | 五段玩法 side-by-side 的副作用、数值和数据库 diff=0 |
| E | T1、T2、T3 全过，完整构建/测试/商业门禁、干净机部署、稳定性和 RC 包校验通过 |

网络实现允许 modern-only；视觉资源、音频、地图、UI、玩法和数值仍必须以原版为 1:1 基准。未满足表中判据即保持未完成。

### M6：1.0 商业发布就绪

#### M6-A：干净机部署自动化 — 门禁 GREEN (本机) — 2026-08-18 验证

- scripts/clean-deploy.ps1 退出码 0-5 全部分支已在本地验证。
- 文档: docs/CLEAN_MACHINE_DEPLOY.md 含 DryRun + SkipSmoke + InstallPrereqs 路径 + portal smoke 步骤。
- 外部环境 (干净机、生产配置演练) 仍待外部机器验证 — 不阻塞本机 RC 声明。

#### M6-B: 24h stability harness — 1h SQLite canary PASSED, 4h/24h MSSQL PENDING

- scripts/soak-24h.ps1 1h SQLite canary: 11,586 cycles, 0 crashes, handles bounded (Login 164→168, Agent 152→176, Map 150→161)。
- 4h mssql_odbc canary: 文档 + 运行命令齐备 (docs/SOAK/soak-4h-mssql.md),执行待 24h 窗口。
- 24h full canary: 文档齐备 (docs/SOAK/soak-24h-full.md),执行待 24h 窗口。
- 1.0 RC tag 等待 4h/24h canary 落地。

#### M6-C：本地端到端启动 + 数据库 + 客户端连接 — 门禁 GREEN


- MSSQL 端：LocalDB Moxiang 库有真实数据 — chr_log_info (test/test) + character_info (chrid 240366, 412303, 945025, 953712, 1117800)，可经 ODBC 17 sqlcmd 查询。


#### M6-B (second reference, now GREEN)

See M6-B block above (line 103) for the canonical status. This duplicate is kept as a historical anchor only and will be removed in the next docs pass.
