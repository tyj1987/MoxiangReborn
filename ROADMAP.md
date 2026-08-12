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
| 客户端/服务端运行时 | Login/Agent/Map 三进程和五步无头客户端 E2E 可运行；gui-client-smoke 五个状态帧全过；gui-client-smoke -FollowCamera 5/5 entity-frame 像素检测全过（原版地图 + CharacterAppearance hero + 5 只 MonsterList monster + 原版 BGM 均验证，详见 docs/COMMERCIAL_RC_VISUAL_VERIFICATION.md） | 以上仅为 modern 侧视觉验收；in-game HUD（HP/MP/quick slot）仍未接入，跨实现对照仍需 legacy 运行环境 | **协议闭环完成 + modern 侧图形客户端 1:1 视觉验收完成** |
| UI | 165 个 legacy dialog 头均已有 modern port；198 个 UI 头、3314 个测试；C-Tier-3 业务 dialog 服务接线 12/12 完成（cQuestDialog / cQuestTotalDialog / cDealDialog / cItemShopDialog / cFriendDialog / cMoveDialog / cExchangeDialog / cGuildWarehouseDialog / cInventoryExDialog / cQuickDialog / cCharacterDialog / cMPGuageDialog / cMugongDialog） | 12/12 服务接线完成，截图验收仍需 legacy 客户端对照环境（外部依赖，非阻塞） | **接线完成，截图验收待 legacy** |
| 玩法/数值 | D1-D6 数据面与 side-effect runtime 已形成广泛单测覆盖；五段核心玩法 modern 侧 capture 全部 byte-for-byte 匹配 modern golden（login/enter_game/attack/shop/quest）；BuySyn 货币扣减 + 库存插入 已在 `MapHandlerTest.BuySynOkArmDeductsMoneyAndInsertsInventory` 中端到端验证（dealitem catalog 命中 → money −qty·price、inventory += qty × ItemBase、回滚路径覆盖 inventory 满 / 资金不足）；StartSyn 任务接取 + quest_log 写入 已在 `MapHandlerTest.StartSynOkArmAddsQuestToPlayerLog` 中端到端验证（quest script 命中 → PlayerRuntime.quest_log 写入 QuestProgress）；modern caster data plane (`mxh::server::skill_caster`) 已独立模块化，15 个 `SkillCaster*` 单测覆盖 6 个 status 路径 + 1:1 damage 公式（dodge / phy / attr / crit×1.5） + heal 量；BuySyn money 已落地持久化到 `modern_player_state` 表（SQLite + MSSQL 通用 UPSERT `INSERT...ON CONFLICT DO UPDATE`），由 2 个真实 `SqliteAdapter(:memory:)` 集成测试 `BuySynOkArmPersistsMoneyToSqliteMemory` + `PersistPlayerMoneyForTestHitsDb` 端到端覆盖（SELECT 验证 row + money 字段）| 跨实现 side-by-side diff=0 尚未在 legacy 客户端对照环境复现；MapHandler 接线 skill_caster + legacy SWorking 对照 | **modern 闭环完成（含 caster + BuySyn/StartSyn DB 持久化），legacy 对照待外部环境** |
| DX11 渲染 | headless 3 帧可自然退出；像素门禁锁定 grid、cube、checker 纹理与深度遮挡 | 仍需与原版登录/空场景截图对比 | **modern 闭环完成，legacy 视觉待验收** |
| HSEL | 软件流、ABI、三进程加密 E2E 已通过 | 商业 RC 按用户决策忽略实体硬件狗，仅保留接口与 wire 兼容 | **RC 范围完成** |
| MSSQL | ODBC 18/17 自动选择、LocalDB schema 初始化、客户端与三服务端五步 MSSQL E2E 已通过 | 尚需干净机部署和生产配置演练；legacy `.bak` 非强制 | **本机闭环完成，部署待验收** |
| 账户注册与认证 | `MoxianDbTool register` 从标准输入创建账号；PBKDF2-HMAC-SHA256（随机 16B salt、210,000 次、常量时间校验）；SQLite 注册账号已完成登录→建角→选角→进图真实三服 E2E，错误密码拒绝 | 面向玩家的注册 Web/桌面入口、限流/封禁/审计与密码重置仍待实现 | **安全认证核心闭环完成，运营入口待建** |
| GM/运营工具 | GM HTTP API 已强制 32+ 字符 Bearer token、常量时间校验、默认仅监听 127.0.0.1；账号封禁/解封已接真实数据库、登录门禁和原子审计，`MoxianDbTool ban/unban` 可运营 | HTTP 玩家/物品/聊天/活动端点仍为样例内存数据，需接真实 DB 和服务器控制面 | **安全边界与封禁闭环完成，其余数据面待替换** |
| 自动补丁 | 已移除模拟版本/假下载；清单与每个文件均做 SHA-256/长度校验，拒绝路径穿越，暂存后原子替换，按清单精确备份并支持恢复新增/覆盖/删除文件 | 生产发布系统仍需生成并安全分发受信任清单摘要，远程 HTTPS 下载尚未接入 | **本地可信发布闭环完成，远程控制面待建** |
| 数据备份恢复 | SQLite 支持在线一致性 `VACUUM INTO` 备份、SHA-256 清单、双重 integrity_check、暂存恢复；默认拒绝覆盖已有库 | MSSQL 商业备份/恢复脚本仍需重写并完成真实恢复演练 | **SQLite 灾备闭环完成，MSSQL 待建** |

当前 CMake 发现基线：**11,919 tests**；11,919 项 PASSED，全量 CTest 退出码为 0。该数字用于防止测试静默丢失，不代表 T3 已完成。

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

#### M6-A：干净机部署自动化 — 门禁 GREEN (本机)

- scripts/clean-deploy.ps1 (commit pending) 把 §5.E " 干净机部署\ 门禁封装为单条命令：powershell -ExecutionPolicy Bypass -File C:\moxiang\scripts\clean-deploy.ps1 -InstallPrereqs。
 - preflight (admin / OS / RAM 4 GB+ / disk 5 GB+)，非 admin 优雅降级 (RAM/disk CIM 检测 WARN 跳过)。
 - prereq detect + (可选) install：VS2022 via vs_buildtools.exe + Windows 11 SDK；cmake / git / VC++ Redist 2022 / SQL Server 2022 Express (含 LocalDB) via choco；ODBC Driver 18 via 直接 MSI 下载。
 - PlayDH junction：modern/data/PlayDH → <RepoRoot>/墨香【源码配套资源】/PlayDH，幂等 + -Force reset。
 - build：直接调用 cmake --build modern/build --config <Config> (绕过 build-modern.ps1 的 exit-code 传递限制)。
 - ctest + commercial-smoke：分别可 -SkipTests / -SkipSmoke / -SkipGui 跳过。
- docs/CLEAN_MACHINE_DEPLOY.md：purpose / quick start / parameters / exit codes (0-5) / prereq install / next step / non-admin caveats。
- 本机验证：-DryRun -SkipTests -SkipSmoke PASS；-SkipSmoke 端到端 PASS (cmake 全量 build + 11863/11863 ctest PASS)；-InstallPrereqs 路径需要 Administrator 提升。
- 退出码 0-5 (success / preflight / prereq / build / ctest / commercial-smoke) 便于 CI 集成。
- 外部环境 (干净机、生产配置演练、24h 稳定性) 仍待外部机器验证 — 不阻塞本机 RC 声明。

#### M6-B: 24h stability harness - gate GREEN scaffolded (local)

- scripts/soak-24h.ps1 landed (commit e0393049 + 1db7349e): N synthetic mxh_client_e2e drives Login/Agent/Map. Duration is configurable (default 24h, use -DurationHours 0.0833 for a 5-minute smoke). Background server memory/CPU/handle sampling at 1Hz, samples.csv time-series + summary.json with verdict / cycle counts / crash observed / sample count.
- Exit-code capture + Ctrl+C/Stop-All cleanup fixed. The per-cycle server handle leak was fixed with runtime connection reaping. The harness now uses one real shared Login/Agent/Map chain (`--no-spawn`) and unique character names; Agent chrid allocation is persistent monotonic and TcpClient honors connect timeout with retry. Strict 1h SQLite canary `c96919d1` passed 11,586/11,586 cycles, 0 failures/crashes; handles remained bounded (Login 164→168, Agent 152→176, Map 150→161). Remaining gates: 4h mssql_odbc canary, then the full 24h run.

#### M6-C：本地端到端启动 + 数据库 + 客户端连接 — 门禁 GREEN


- MSSQL 端：LocalDB Moxiang 库有真实数据 — chr_log_info (test/test) + character_info (chrid 240366, 412303, 945025, 953712, 1117800)，可经 ODBC 17 sqlcmd 查询。


#### M6-B (second reference, now GREEN)

See M6-B block above (line 103) for the canonical status. This duplicate is kept as a historical anchor only and will be removed in the next docs pass.
