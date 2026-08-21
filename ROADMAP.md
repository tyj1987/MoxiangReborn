# Moxian-Reborn 路线图：1:1 完美复现

> 状态日期：2026-08-22。可玩性以 [docs/PLAYABLE_STATUS.md](docs/PLAYABLE_STATUS.md) 为准。完成历史见 [docs/CHANGELOG.md](docs/CHANGELOG.md)。本文件只记录目标、当前事实和下一里程碑。单测绿不等于玩家能玩。

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

T1、T2、T3 全部通过并完成商业 RC 打包，才算当前目标完成。legacy 网络互通只作参考，不是发布门禁。

## 2. 当前状态

可玩性细节与代码锚点见 [docs/PLAYABLE_STATUS.md](docs/PLAYABLE_STATUS.md)。下表是 2026-08-22 按 **代码 + 真人路径** 校准后的结论，不是 ctest 计数。

| 领域 | 实际 | 结论 |
|---|---|---|
| T1 资源 | PlayDH 在 `modern/data/PlayDH`；解析/SHA 单测可用 | 资源可读 |
| T2 协议 | Login/Agent/Map 五步 E2E 在 `--auto-*` 和 `mxh_client_e2e` 下通过；PVE VM 100 + MSSQL `192.168.2.203` 已跑通自动化 | 协议闭环 ≠ 可玩 |
| 登录 UI | `g_loginUi` 手写 overlay + `login.dds`，不是 `MT_LOGINDLG` | 看起来正常，不是 1:1 |
| 选角/建角 | 协议有；人类看不见/点不到原版 dialog（P0） | **玩家卡住点** |
| 进图 | 地形/天空/静态物有 DX11 路径；HUD/人物/NPC 不可靠；Map 12 无野外怪（14 字节 stub） | 能进图，不能当游戏 |
| 输入 | WndProc 有转发；不可见 dialog 可吞点击；无视觉时 WASD 像失灵 | 未验收 |
| 单测 | 数量大、大量 PASS。只锁解析/dispatcher/公式 | **禁止当可玩证明** |
| HSEL | 软件流 + 接口签名 | RC 范围保留签名 |
| 部署 | 客户端 `192.168.2.30`；三服 `192.168.2.107`；SQL `192.168.2.203` | 拓扑可用 |

历史 M-R1…M-R7 / M3–M5「GREEN」段落已从本表撤下。装载链与公式单测仍在仓库里，但它们没有让选角按钮出现在玩家屏幕上。


## 3. 当前里程碑

当前只做 **能玩**，顺序锁死。截图必须来自无 `--auto-create` 的真人 `mxh_client`。

### P0：选角 / 建角能看见、能点（进行中）

- 停止启动时把 157 个 InterfaceScript 装进无用 `g_wm`
- 同一 `.tif` 只建一份 GPU sprite
- CharSelect / CharMake load 后激活 root，人类能点创建/进入
- 门禁：本机登录 `192.168.2.107`，选角和建角各一张能看出按钮的截图

### P1：进图 HUD + 键鼠有反馈

- 默认打开主条/快捷栏/小地图；背包/商店/任务保持关
- 未命中或全透明区域不 `consumed` 世界点击
- 门禁：进 Map 12 能看见主 HUD；WASD 有可见位移

### P2：人物与 NPC 可见

- `EntityScene` 失败要计数，不能静默空场景
- 门禁：同一帧指出玩家和至少 1 个 NPC

### P3：打怪图可见怪物

- 不用 Map 12。用非 stub 的 `Monster_*.bin`
- 不改 PlayDH 的 `Monster_12.bin`

### P4：原版登录 dialog + 视觉 1:1（后置）

- 替换 `g_loginUi`；SSIM / 30fps 放这里，不挡 P0–P3

> 以下 M2–M6 是历史单测 / 门户记录，**不是**当前可玩门禁。当前门禁是上面的 P0–P4。

### M2：C-Tier-3 UI 集成 — 接线完成（不等同可玩）

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
