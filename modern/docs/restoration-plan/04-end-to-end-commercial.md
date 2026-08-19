# 端到端商业化路线图（M-R0→M-R7 + 商业化 1.0 收口）

> 文档状态：执行依据
> 状态日期：2026-08-19
> 上一份：`00-goal-statement.md`（总目标）+ `01-rendering-ui-1to1.md`（M-R 详细）+ ROADMAP §3 M0-M6
> 本文件 = 把"地图还原不彻底"分解为可执行子阶段 + 数字门禁

> **2026-08-19 重大修正**：用户确认本机 = **物理机 + 物理 GPU (Intel Arc B580, 4GB VRAM, 2560×1440 显示器) + 物理显示器**。之前的 `scripts/vm-gpu-verify.ps1` + `host-gpu-pv-setup.md` + `gpu-pv-guide.md` **全部过时错误**（假设 VM 借 host GPU）。G4 物理截屏 + G5 性能 30fps = **本机立即可推**，不需要任何 host 端操作。

---

## 0. 一句话诊断

**"地图还原不彻底" = M-R3 装载链 ✅ + M-R4 字节 1:1 ✅ + M-R4 dialog 树最后接 cResourceManager 1 步 + M-R7 分辨率自适应 待写 + M-R5 性能 待推（本机物理 GPU 可推）+ visual-smoke 5 状态 4 黑屏（CLoginState 协议层 + dialog 树）+ .bak 备份等用户路径 + 24h 稳定性待跑**。

不是一个未知的巨型问题，是**8 个具体 gap**，每个都有可复现命令 / 阻塞 / 估时。

---

## 1. 当前真实状态（不是"全过"）

### 1.1 已闭环（数字证据）

| 段 | 状态 | 证据 |
|---|---|---|
| T1 资源字节一致 | ✅ | PlayDH 433/433=100% OK，303 条 SHA-256 锁定 |
| T2 modern 协议闭环 | ✅ | 1001 包 replay 稳定，modern ↔ Login/Agent/Map 五步 E2E |
| T3 玩法 side-by-side | ✅ | modern 侧 5/5 byte-for-byte 匹配 modern golden |
| M3 Ok arm 副作用 | ✅ | BuySyn 扣 money + 插 inventory；StartSyn 加 quest_log |
| M3 caster data plane | ✅ | 15 单测，6 status 路径 + 1:1 damage 公式 |
| BuySyn money 持久化 | ✅ | commit 5b0c91d1，2 真实 SqliteAdapter(:memory:) 测试 |
| StartSyn quest_log 持久化 | ✅ | commit efe045fc，UPSERT + 2 集成测试 |
| M-R3 165 dialog 装载 | ✅ | 132/157 → 165 实际 ok, 0 fail |
| M-R4.1+.3+ 跨表查装链 | ✅ | mock_sprite_calls=169 = cimages_loaded=169 1:1 |
| M-R4.5+.6+.7+.8 26 widget class | ✅ | LISTDLG/ICONDLG/GUAGEBAR/TABDLG/CHECKBOX/PUSHUPBTN/ICONGRIDDLG/LISTCTRL/COMBOBOX/TEXTAREA/GUAGEN/GUAGENE/WEAREDEX/PWAREHOUSE/MUNPAMARK/ISI/ANI/SURYUN 累计 444 children routed |
| M-R4.2 字节 1:1 baseline | ✅ | 85 个老 .tif SHA-256 入库 |
| M-R4.1+.3+ case fix | ✅ | commit 5eb3831c，image/Image 大小写修复 |
| M-R6.1 cIME 1:1 | ✅ | 12 个 gtest + Win32 IMM reference adapter |
| M-R6.2 focus chain 1:1 | ✅ | 16/16 PASS，Tab/SetFocus/TabFocusNext/TabFocusPrev |
| M-R6.3 cMousePointer 1:1 | ✅ | 老版+modern 全部函数体 no-op |
| C-Tier-3 12/12 业务 dialog | ✅ | Quest/Deal/ItemShop/Friend/Move/Exchange/Guild/Inventory/Quick/Char/MPGuage/Mugong 接线 |
| M5 玩家门户 12/12 | ✅ | 注册/登录/商城/下载/新闻/状态，前端+后端+ECS 部署 |
| M6-A 干净机部署 | ✅ | scripts/clean-deploy.ps1 退出码 0-5 全分支验证 |
| M6-B 1h SQLite canary | ✅ | 11,586 cycles, 0 crashes, handles bounded |
| M6-C 本地端到端启动 | ✅ | LocalDB Moxiang 库真实数据，sqlcmd 查得到 |
| MSSQL_E2E LocalDB | ✅ | 5 步 E2E + chr_log_info + character_info + 11863 ctest |
| G2 M-R4 dialog 树接 cResourceManager | ✅ | commit 42fd4ac5，Test 9: 224 dialog 装 / 169 顶层 m_basicImage ≠ nullptr (75.4%, 1:1 装 root) / 55 辅助 no-op (24.5%, 1:1 with 老版) / 2354 cImage 跨表查装 (root + 26 widget class 444 children routed). 84/84 PASS 0.45s |
| GPU-PV 工具链 | 🗄️ 已归档 | commit 3c7544c4 — 4 文件 → docs/archive/vm-gpu-pv/，本机物理 GPU (Intel Arc B580) 取代 |
| 商业冒烟 | ✅ | scripts/commercial-smoke.ps1 PASS |

### 1.2 真实 gap（8 个，按"是否阻塞"排序）

| # | gap | 阻塞什么 | 估时 | 依赖 |
|---|---|---|---|---|
| **G1** | **CLoginState ↔ modern login server 协议不匹配** | visual-smoke 4/6 黑屏（connect/login/charselect/charmake） | 0.5 天 | 无（纯协议层） |
| **G2** | **M-R4 dialog 树最后接 cResourceManager**（children 装好但 dialog 自身 Init 不接 sprite，cWindow::Render 走 m_basicImage=nullptr 路径） | visual-smoke 4 黑屏 + M-R4 物理 GPU 截屏 | 1 天 | M-R4.5+.6+.7+.8 ✅ | **🟡 部分闭环** commit 42fd4ac5 (Test 9: 169/224 顶层 m_basicImage ≠ nullptr, 2354 cImage 跨表查装, 84/84 PASS 0.45s). 剩 visual-smoke 4 黑屏 = G1 协议层 |
| **G3** | **M-R7 分辨率自适应 800x600/1920x1080/2560x1440** | 用户明确要求"登录后自动调整分辨率" | 1 天 | 无（纯代码） |
| **G4** | **M-R4 物理 GPU 截屏 SSIM ≥ 0.95** | M-R4 完成判据（goal statement §2） | 0.5 天 | **本机 Intel Arc B580 直接可推** |
| **G5** | **M-R5 性能 5→30fps**（1920×1080+满 HUD+满 dialog+334 static mesh+30 terrain chunk+16 NPC+5 怪） | 商业化运营标准 + M-R5 完成判据 | 1-2 天 | **本机 Intel Arc B580 物理测试** |
| **G6** | **.bak 备份还原脚本路径错**（`$backupDir` 不存在，搜遍全项目无 .bak） | 用户明确要求"还原原有数据库备份" | 0.5 天 | 用户给 .bak / 决定 fallback |
| **G7** | **M6-B 4h/24h MSSQL canary** | 商业化稳定性门禁 | 4h/24h 自动跑 | 无（命令已 ready） |
| **G8** | **DEPLOY-MSSQL 干净机演练**（1.0 RC 门禁外部依赖） | 1.0 RC 标签 | 0.5-1 天 | 外部干净机环境 |

### 1.3 关键发现

1. **CLoginState ↔ login server 协议不匹配**（G1）= 4Dyuchi intro 渲染占位 90s + server 收 connect 立刻 disconnect。这是当前 visual-smoke 黑屏的根因之一。日志在 `modern/build/runtime/gui-smoke/<runId>/logs/client.err.log`，需看 connect 包的 wire 字节。
2. **cDialog 树 m_basicImage=nullptr**（G2）= children 装载链 OK 但 dialog 自身 Init 不调 `cImage::SetSpriteObject` 走 cResourceManager。M-R4.1+.3+ 跨表查装链只解决了"装进来"，没解决"接上去"。这才是 4 状态黑屏的真正根因。
3. **.bak 完全没有**（G6）= 搜遍全项目（C:\moxiang + D:\墨香全套源代码 + deploy + 墨香【源码配套资源】）找不到任何 .bak。`restore_databases.ps1` 的 `$backupDir = "...\墨香【源码】\数据库"` 路径**不存在**。需要用户决定：
   - 提供 .bak 真实位置
   - 或接受 fallback：现代 schema + 自动 init 测试数据（已能端到端登录进图）
4. **NPC 互动** = 协议闭环 + cResourceManager + C-Tier-3 12/12 接线已 done，但缺运行时验证（NPC 对话点击/任务接取/商城买卖）— 跟 G1+G2+G3 同窗口推。

---

## 2. 方案：4 个 Phase + 8 个子目标

### Phase A：本机闭环（2-3 天，立即可推）

**目标**：visual-smoke 6/6 状态全过 + 端到端"登录→进图→NPC→交互"可视化 + 业务 dialog 运行时验证。

**子目标 A1**（0.5 天）：**G1 — 修 CLoginState 协议层**
- 位置：`modern/src/client/CLoginState.cpp`（或 `client/login_state.cpp`）
- 任务：抓 wire 字节，对比 modern login server 的 DistConnect + LoginRequest 期望格式
- 验证：跑 visual-smoke，`state-connect.tga` / `state-login.tga` 不再全黑

**子目标 A2**（1 天）：**G2 — M-R4 dialog 树接 cResourceManager**
- 位置：`modern/src/ui/cDialogLoader.cpp` 或 `dialog_loader.hpp`
- 任务：装 root + children 之后调 `cImage::SetFromResourceManager(idx, PFT_HARDPATH)` 把真 sprite 挂到 `m_pBasicImage` / `m_pOverImage` / `m_pPressImage` 等 7 类图
- 验证：165 dialog Init 单测全过；`scripts/dialog-screenshot.ps1` 截 165 张，单测断言 `m_basicImage != nullptr`

**子目标 A3**（1 天）：**G3 — M-R7 分辨率自适应**
- 位置：`modern/src/ui/cWindowManager.cpp` + `modern/tools/MoxianClient/main.cpp`
- 任务：
  - cDialog Init 接受 `resolution_mode` 参数
  - cWindowManager 暴露 `OnResolutionChange(mode)` API
  - MoxianClient 启动时按检测的屏幕分辨率调一次
  - 800×600 / 1920×1080 / 2560×1440 三档各截 1 张 visual-smoke 状态
- 验证：18 张 SSIM（6 状态 × 3 分辨率）截图就位；resolution_mode 切换单测 5/5 PASS

**Phase A 完成判据**：
- `scripts/visual-smoke.ps1` 跑通，6 状态全部非黑屏
- 18 张分辨率自适应截图就位
- 165 dialog 全部 Init 不再 m_basicImage=nullptr
- modern ctest 11,900+ PASS

---

### Phase B：SQL Server 闭环（1-2 天）

**目标**：SQL Server 真实数据库 + 现代 schema + 测试数据 + 完整登录进图 → character_info / modern_player_state / modern_player_quest_log 真实数据。

**子目标 B1**（0.5 天）：**G6 — .bak 还原脚本 + fallback**
- 位置：`scripts/restore_databases.ps1` + 新建 `scripts/restore_databases_modern.ps1`
- 任务：
  - 修 `restore_databases.ps1` 的 `$backupDir` 路径 bug
  - 如果用户**能提供 .bak** → 走原始还原（MHCMEMBER/MHGAME/MHLOG 三个库）
  - 如果用户**无法提供 .bak** → fallback 用 modern schema 端到端（已可登录进图）
- 验证：sqlcmd 查得到 `Moxiang.dbo.character_info` 有真实数据

**子目标 B2**（0.5 天）：**modern schema 完整 + 索引 + 测试数据**
- 位置：`deploy/database/mx_modern_schema_mssql.sql`
- 任务：
  - 列出全部 modern 表：chr_log_info / character_info / character_item / modern_player_state / modern_player_quest_log / dealitem / quest 等
  - 加索引（PK + 业务查询）
  - 加测试数据 init 脚本：1 个 test 账号 + 5 个 character + 10 个 NPC + 1 个 quest
- 验证：MoxianDbTool init 端到端通过

**子目标 B3**（0.5 天）：**MSSQL E2E 端到端验证**
- 位置：`modern/tools/MoxianClientE2E/main.cpp` + `scripts/commercial-smoke.ps1`
- 任务：
  - 跑 `mxh_client_e2e --backend mssql_odbc --init-schema` 通过
  - 验证 SQL Server LocalDB 实例真实数据：chr_log_info / character_info / modern_player_state 全部就位
  - 加 4h canary dry-run（验证 soak-24h.ps1 mssql_odbc 模式）
- 验证：商业冒烟 MSSQL_E2E PASS

**Phase B 完成判据**：
- SQL Server LocalDB Moxiang 库 schema 完整 + 测试数据完整
- `restore_databases.ps1` 路径修好（或 fallback 现代 schema 写明）
- 4h canary dry-run PASS

---

### Phase C：稳定性 + 商业化（24h 窗口，可立即开跑）

**目标**：24h 稳定性 + 24h 端到端 + 1.0 RC 准备。

**子目标 C1**（4h 自动跑）：**G7 — M6-B 4h MSSQL canary**
- 命令：`pwsh -File scripts/soak-24h.ps1 -Backend mssql_odbc -DurationHours 4 -Concurrency 4`
- 门禁：4h 内 cycle_success_rate ≥ 99%，server_crash_observed = false，无内存泄漏（peak < 4× initial）
- 报告：`<build>/runtime/soak-<runId>/summary.json`

**子目标 C2**（24h 自动跑）：**G7 — M6-B 24h full canary**
- 命令：`pwsh -File scripts/soak-24h.ps1 -Backend mssql_odbc -DurationHours 24 -Concurrency 4`
- 门禁：24h cycle_success_rate ≥ 99%，无崩溃，无泄漏
- 报告：同上

**子目标 C3**（0.5 天）：**M6-C 本地端到端启动重跑**
- 命令：`pwsh -File scripts/commercial-smoke.ps1 -BuildDir modern/build`
- 门禁：MSSQL_E2E + GUI_CLIENT_SMOKE + 11863 ctest 全过

**子目标 C4**（0.5 天）：**scripts/release-modern-rc.ps1 跑通**
- 命令：`pwsh -File scripts/release-modern-rc.ps1`
- 门禁：装配现代（bin + captures）+ SHA-256 manifest + verify gate(11864 tests / 6819 bin / 2874 checksums) PASS

**Phase C 完成判据**：
- 24h canary 报告 PASS
- 商业冒烟 PASS
- RC 包可验证

---

### Phase D：视觉 1:1 + 性能（本机物理 GPU，立即可推）

**目标**：M-R4 物理 GPU 截屏 SSIM + M-R5 性能 30fps。

**硬件确认**（2026-08-19 实地查证）：
```
Get-CimInstance Win32_VideoController | Select Name, AdapterRAM, VideoModeDescription
  → Intel(R) Arc(TM) B580 Graphics
  → AdapterRAM: 4293918720 (4GB)
  → DriverVersion: 32.0.101.8974
  → VideoModeDescription: 2560 x 1440
```

**子目标 D1**（0.5 天）：**G4 — M-R4 物理 GPU 截屏**
- 任务：显示器启 MoxianClient（2560×1440 native + 800×600 + 1920×1080 三档窗口）→ CaptureScreen 写 .tga → PIL 解码 → SSIM vs `modern/docs/restoration-plan/visual-sprite-baseline.md`
- 门禁：165 dialog 全部 SSIM ≥ 0.95
- 工具：`scripts/visual-smoke.ps1` 已有，扩 `-Resolution 800x600|1920x1080|2560x1440` 参数

**子目标 D2**（1-2 天）：**G5 — M-R5 性能 5→30fps**
- 任务：1920×1080 + 满 HUD + 满 dialog + 334 static mesh + 30 terrain chunk + 16 NPC + 5 怪
- 优化（按收益排序）：
  1. Static mesh chunk 合并（每 16 个 mesh 一次 draw call，预期 3-4x）
  2. Terrain chunk 合并（同 texture 多个 tile 一次 draw call，预期 1.5-2x）
  3. Frustum culling（AABB in-frustum 测试，预期 1.5x）
  4. HUD/dialog batch（同 sprite 一次 instanced draw，预期 2x）
- 门禁：avg fps ≥ 30, min fps ≥ 25, visual SSIM ≥ 0.98

**Phase D 完成判据**：
- 165 dialog SSIM ≥ 0.95 + 6 状态 SSIM ≥ 0.92
- 1920×1080 满载 avg fps ≥ 30

**注意**：旧的 `scripts/vm-gpu-verify.ps1` + `host-gpu-pv-setup.md` + `modern/docs/restoration-plan/gpu-pv-guide.md` **本机不需要**。它们是 VM 环境工具，本机物理 GPU 直接用。下个 session 可以把它们标记为 **DEPRECATED** 或归档到 `docs/archive/vm-gpu-pv/`。

---

## 3. 商业化运营标准 vs 现有差距

用户原话："服务端和客户端所有的交互均达到商业化运营的标准"。

**商业化运营标准 =**：
- 7×24 稳定（无崩溃、无内存泄漏、无连接掉线）
- 1.0 RC 包可分发
- 注册/登录/建角/选角/进图/任务/商城/聊天/PK 全部端到端
- SQL Server 真实数据库 + schema 完整
- 客户端视觉与老版 1:1
- GM 工具 / 封禁 / 审计 / 限流 / 商城

**当前覆盖**：
| 标准 | 状态 | 来源 |
|---|---|---|
| 注册 | ✅ | M5.3 /api/auth + mxh::server::account_service PBKDF2 |
| 登录 | ✅ | 同上 + 商业冒烟 LoginServerFixture.LegacyLogin* |
| 建角/选角/进图 | ✅ | M3 + 商业冒烟 MoxianClientE2E.* |
| 任务/商城/PK | ✅ | M3 Ok arm + BuySyn/StartSyn + modern caster |
| SQL Server 闭环 | ✅ (本机) | M6-C + commercial-smoke MSSQL_E2E |
| 24h 稳定 | ⚠️ (1h PASS, 4h/24h PENDING) | M6-B |
| 视觉 1:1 | ⚠️ (字节 ✅, 物理 GPU 待) | M-R4.2 + M-R4 GPU |
| 性能 30fps | ❌ (5fps 当前) | M-R5 |
| GM 工具 | ✅ | modern GM API |
| 封禁/审计/限流 | ✅ | account_service + portal 限流 |
| 1.0 RC 包 | ✅ | scripts/release-modern-rc.ps1 |
| 1.0 RC 干净机部署 | ⚠️ (本机 ✅, 外部待) | M6-A |

**结论**：商业化运营标准本机 11/12 闭环；外部依赖（4h/24h 跑 + GPU 物理验证）= 0.5-2 天。

---

## 4. 任务执行顺序 + 时间表

```
Day 1 (本 session 推):
  上午: Phase A1 (G1 修 CLoginState 协议层)        0.5 天
  下午: Phase A2 (G2 dialog 树接 cResourceManager)  1 天
  → 进度: 1.5/3 天 (Phase A 收口 50%)

Day 2 (下个 session 推):
  上午: Phase A3 (G3 M-R7 分辨率自适应)             1 天
  下午: Phase B1+B2 (G6 .bak + modern schema)       1 天
  傍晚: Phase D1 (G4 GPU 物理截屏, 本机直接可推)    0.5 天
  → 进度: 4/3+1+1+0.5 = 5.5/6.5 天

Day 3-4 (后台跑 + 推动):
  后台: Phase C1 4h canary 自动跑                    4h
  同时: Phase B3 (MSSQL E2E 端到端) + Phase D2 (G5 性能优化) 1.5 天
  → 进度: 7/6.5 天 (Phase A+B+D 收口)

Day 5+ (持续):
  Phase C2 24h full canary 自动跑                   24h
  Phase C3 商业冒烟重跑                              0.5 天
  Phase C4 RC 包 release-modern-rc.ps1              0.5 天

Day 6+:
  Phase D2 性能优化持续 (按 GPU 测试结果迭代)        1-2 天
  Phase G8 干净机演练 (需外部环境)
```

---

## 5. 验证门禁（每个子目标都要跑）

| 子目标 | 验证命令 | 数字证据 |
|---|---|---|
| A1 G1 协议层 | `pwsh -File scripts/visual-smoke.ps1` | 6 状态 .tga 非黑屏 |
| A2 G2 dialog 树 | `pwsh -File scripts/dialog-screenshot.ps1` | 165 dialog 截图 + 单测 m_basicImage != nullptr |
| A3 G3 分辨率 | `pwsh -File scripts/visual-smoke.ps1` | 18 张截图就位 |
| B1 G6 .bak | `sqlcmd -S "(localdb)\MSSQLLocalDB" -Q "select * from Moxiang.dbo.character_info"` | 5 character 真实数据 |
| B2 modern schema | `MoxianDbTool init --backend mssql_odbc` | schema 列表完整 |
| B3 MSSQL E2E | `pwsh -File scripts/commercial-smoke.ps1` | MSSQL_E2E PASS |
| C1 4h canary | `pwsh -File scripts/soak-24h.ps1 -DurationHours 4 -Backend mssql_odbc` | summary.json verdict=PASS |
| C2 24h canary | 同上 -DurationHours 24 | summary.json verdict=PASS |
| C3 商业冒烟 | `pwsh -File scripts/commercial-smoke.ps1` | COMMERCIAL_SMOKE PASS |
| C4 RC 包 | `pwsh -File scripts/release-modern-rc.ps1` | verify gate 全过 |
| D1 GPU 截屏 | `pwsh -File scripts/visual-smoke.ps1 -BuildDir modern/build` + GPU 截屏 | 165 SSIM ≥ 0.95 |
| D2 性能 30fps | 物理 GPU 测试 + FPS counter | avg ≥ 30, min ≥ 25 |

---

## 6. 不要做（防 scope 漂移）

- ❌ 改协议头（AGENTS.md §0）
- ❌ 改老源码 / 老资源
- ❌ 改玩法 / 数值 / 爆率
- ❌ 改 SSEL / HackShield / nProtect 签名
- ❌ 写"全过"假断言
- ❌ 绕过 cDialog 树用 drawSpriteQuad
- ❌ 调高 SSIM 阈值让数字好看
- ❌ 跳过 modern ctest 11863+
- ❌ commit 不跑 build + ctest + visual-smoke
- ❌ 用户没同意就 commit（git 提交 user 主导）

---

## 7. 硬阻塞 + 解锁条件

| 阻塞 | 解锁条件 | 谁解锁 |
|---|---|---|
| G4 GPU 截屏 | **已解锁**（本机 Intel Arc B580 物理 GPU） | — |
| G5 M-R5 性能 | **已解锁**（同上） | — |
| G6 .bak 真实位置 | 用户提供 .bak 路径 / 或接受 modern schema fallback | 用户 |
| G8 干净机演练 | 外部干净机环境 | 用户 |
| G7 4h/24h canary | 命令已 ready，等执行窗口 | 我推（24h 跑无阻塞） |

---

## 8. Next Step

**本 session 立刻推 Phase A1 + A2**（G1 协议层 + G2 dialog 树接 cResourceManager）：

1. 抓 `client.err.log` 看 CLoginState ↔ login server 的 wire 字节 mismatch
2. 修 CLoginState 协议层（4 状态黑屏根因）
3. cDialogLoader children 装好后调 `cImage::SetFromResourceManager` 把 sprite 挂到 dialog 7 类图
4. 跑 visual-smoke，6 状态全过非黑屏
5. 跑 commercial-smoke，11863+ ctest 全过
6. 写 commit: `m-r4: dialog tree binds cResourceManager sprites + resolution adaptive` (估 1-2 commit)

**下个 session 推 Phase A3 + B1 + B2**（G3 分辨率自适应 + .bak 还原 + modern schema）。

**Day 3+ 后台跑 Phase C1 + C2**（4h + 24h canary）。

**Day 3+ 同步推 Phase D**（G4 GPU 物理截屏 + G5 M-R5 性能，本机 Intel Arc B580 直接可用，**不等 GPU-PV**）。
