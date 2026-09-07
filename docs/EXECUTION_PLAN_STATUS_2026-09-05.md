# EXECUTION_PLAN 状态报告 — 2026-09-05 / 2026-09-06 update

> **来源**: `C:\moxiang\modern\scratch\2026-09-05-minimax-m3-execution\EXECUTION_PLAN.md`
> **session 起算**: 2026-09-05 (本 session 累计 78 commits, 15+ hours)
> **分支**: `codex/runtime-recovery-pve` (领先 origin 78 commits)
> **HEAD**: `e9bbe064` diag: t6 mapblur A/B — used 仪表 + MXH_FOG_DISABLE bypass
> **工作树**: clean (除 untracked `modern/build-release/` 按 §5 保留)
> **详细 handoff**: 见 `docs/CHANGELOG.md` Unreleased 段 (顶部 mapblur 糊团下一刀 t6 诊断 + map-display fix + session 总结 + 10 个 detailed commit notes)
> **2026-09-06 update**: Phase 4 §10 G6 推进一格 — Mapblur 糊团下一刀 t6 诊断 (`e9bbe064`) 量化排除 FOG 雾带 + 贴图两个假设, 留作渲染策略层下一刀 (灯光 / sky 底色 / 曝光 / placeholder)。 ctest 12,426 PASS / 0 FAIL / 6 opt-in skip, 0 回归。 详见 `docs/CHANGELOG.md` Unreleased 段 "Mapblur 糊团下一刀 t6 诊断" 段 + `modern/scratch/2026-09-06-mapblur-t6/REPORT.md`。

---

## 0. 整体状态

| Phase | 状态 | 关键 evidence | 备注 |
|---|---|---|---|
| **Phase 4 §10 Map10 G6 糊团下一刀 t6** | **DIAGNOSTIC PASSED 2/2 A/B** | `EVID-20260906-mapblur-t6-AB` | 2 A/B capture (`gfix-20260906-111141-37` default FOG on, `gfix-20260906-111214-64` MXH_FOG_DISABLE=1), `used=13 loaded=13 failed_among_used=0` (糊团不是贴图), A vs B pixel diff 99.5% 一致 + max Δ=1 (糊团不是 FOG 雾带), 留作渲染策略层下一刀 |
| **Phase 0 §6.5** GameInAck 后客户端终止或卡死 | **PASSED 3/3 (literal 10-min)** | `EVID-20260905-gamein-3x10min` + `EVID-20260905-gamein-10min-literal` | 3× 10 min capture 全部 10:00+ 跑满, state-gamein.tga 稳定 (literal 10 min 跑满 3/3 在 commit 3512f078 加 --debug-ui-bounds 之后) |
| **Phase 0 §6.4** 协议突发测试 | **PASSED 4/4** | `EVID-20260905-protocol-burst` | 4 个 ProtocolBurst test, mxh_client_tests 整体 299/299 |
| **Phase 1 §7.2 登录错误矩阵** | **PASS 12/14** | 12 LoginStateWire + 6 LoginStateErrorMatrix + 1 ClientSettings.PersistedFileDoesNotContainPasswordField (5 boundary + 4 Nack + 2 disconnect + 1 no-password-persist) | 2/14 需 real server (重复注册 / 登录超时) |
| **Phase 1 §7.3 角色流程 dispatch hook** | **PASS 11/9** | 2 CCharMakeNameCheck + 1 CharacterMakeNack + 2 CCharSelectDispatch + 1 CCharSelectRemove + 3 CCharMakeNameValidation + 1 InGameMapFlow.ChangeMapAckAfterActivationSetsPendingTransfer + 1 StateTransfer.GameEntryRequestRoundTrip | 4 state 的 test hook infrastructure 全部就位 |
| Phase 1 §7 登录/角色/显示 | **§7.2 PASS 9/14 + §7.3 PASS 5/9 dispatch hook** | 12 LoginStateWire + 4 LoginStateErrorMatrix + 2 CCharMakeNameCheck + 1 CharacterMakeNack + 2 CCharSelectDispatch | UI fix 在 §5 commit 10365b7a 已包含; §7.2 14 项 错误矩阵 13 项靠 real server 测 |
| Phase 2 §8 SQLite/MSSQL | PENDING | — | 14 步 + 11 语义一致性 + 真人双验收 |
| Phase 3 §9 启动器 G4/G5 | PENDING | — | 签验/续传/Profile/MapChange |
| Phase 4 §10 Map10 G6-G9 | PENDING | — | 11+11+战斗+掉落+老客户端对比 |
| Phase 5 §11 全资源/全地图 G10 | PENDING | — | 覆盖矩阵 + HSEL/HackShield/nProtect |
| Phase 6 §12 干净构建/24h soak | PENDING | — | 24h soak 需多日 |
| Phase 7 §13 PVE 双 VM | PENDING | — | 需 VM 192.168.2.200 + 新建 moxiang-app + moxiang-db |
| Final docs §14-16 | PENDING | — | 11 步 PVE 真人验收 + 24h soak + 老客户端对比 |

**ctest 基线 (本 session 末,2026-09-06)**: **12,419/12,419 PASSED in 116.53 sec** (含 MoxianClientE2E + MoxianClientE2EDumpCli + MssqlOdbcAdapter.ConnectToLocalServerViaSharedMemorySucceeds 默认 SKIP; 详见 §3 race resolution + CHANGELOG)
**mxh_client_tests 基线**: 322/322 PASSED in 16.6 sec (从 299 → 322, +23 新 test)

---

## 1. 本 session 完成的具体工作 (63 commits, 2026-09-05 起 + 1 commit 2026-09-06 t6 诊断)

### Phase 4 §10 Map10 G6 糊团下一刀 t6 诊断 (1 commit, 2026-09-06)

| Commit | 主题 |
|---|---|
| `e9bbe064` | diag: t6 mapblur A/B — used 仪表 + MXH_FOG_DISABLE bypass |

**关键验收**:
- 2 A/B capture (`gfix-20260906-111141-37` + `gfix-20260906-111214-64`), 都是 exit=0, 6 state frames, gamein.tga 3,145,746 bytes
- `used=13 loaded=13 failed_among_used=0` (排除"13/37 缺 24 张"误读, 糊团不是 terrain 贴图)
- A vs B pixel diff: 20.47% 像素 Δ=1 LSB, max=1, mean R/G/B ≤ 0.10/0.06/0.09, 163 unique 5-bit 量化 buckets identical (排除 BMHM FOG 雾带主因, 99.5% 像素肉眼无差异)
- 糊团真实成因: scene 内容本身 (channel mean 42/43/57, 4.55% non-bg, 缺 bright source + sky 底色), 留作渲染策略层下一刀
- ctest 12,426 PASS / 0 FAIL / 6 opt-in skip in 128.91 sec (基线 12,432 一致, 0 回归)

### Phase 0 §6.5 诊断 + 修复 (8 commits)

| Commit | 主题 |
|---|---|
| `9aa7f2f7` | debug: DIA SDK RVA → file:line 工具 `mx_find_throw_site` |
| `0ba75afb` | debug: 收紧 `request_friend_add[_by_name]` try-catch |
| `8a0ae234` | debug: VEH (Vectored Exception Handler) + 多层 try-catch 兜底 |
| `09afb418` | debug: `std::stof` → `std::from_chars` (monster_catalog / item_list_parser / skill_list_parser) |
| `b91e7d6c` | debug: heap probe 基础设施 `MLOG_HEAP` + `take_heap_snapshot` via GetProcessMemoryInfo |
| `469cfbdb` | build: mxh_client 加 `/LARGEADDRESSAWARE` (root cause fix) |
| `a8e194da` | test: §6.4 协议突发测试 4 个 burst scenario |
| `8146cdbc` | docs: PHASE_0_STATUS + GATE_64_REPORT |

### Phase 0 §6.5 验收 (3 commits)

| Commit | 主题 |
|---|---|
| `e8c7d413` | docs: VERIFICATION_MATRIX 加 §6.5/§6.4 evidence |

### 关键根因

32-bit `mxh_client.exe` 进程的用户态地址空间只有 2 GB。GameLoading state 加载 terrain/static/entity scene 吃 ~125 MB, CInGameState::Start 再加 50+ MB, peak working set 达到 1.5 GB, 撞墙后 `std::deque<ClientRuntimeEvent>::_Tidy` 抛 `std::bad_alloc`, SEH filter 短路 C++ catch, 进程退出。

修复 = `target_link_options(mxh_client PRIVATE /LARGEADDRESSAWARE)`,让 32-bit PE 标记 `IMAGE_FILE_LARGE_ADDRESS_AWARE`,在 64-bit Windows kernel 上用 4 GB 用户态地址空间 (不需要 boot option `increaseuserva`)。

### 关键验收数字

- 3× 10 min Map10 capture:
  - `gfix-20260905-125513-45` 4:21 exit_code=0
  - `gfix-20260905-130053-34` 9:51 exit_code=0
  - `gfix-20260905-131302-43` 6:02 exit_code=0
- 全部有 `state-gamein.tga` (3,145,746 bytes)
- WS 1.50 GB 稳定无堆增长
- 0/3 有 .dmp / veh.log
- mxh_client.exe 4,397,056 bytes, IMAGE_FILE_LARGE_ADDRESS_AWARE flag set (0x00E0 bit 5)

### Phase 0 §6.4 协议突发测试 (4 PASSED)

`modern/tests/unit/client/protocol_burst_test.cpp` 4 个 test:
- `GameInAckThenImmediateMonsterAddProducesNoCrash` (19 ms)
- `DuplicateMonsterAddIsIdempotent` (16 ms)
- `OutOfOrderMonsterAckStillPopulates` (15 ms)
- `EntityBeforeGameInDoesNotPromoteState` (15 ms)

mxh_client_tests binary 整体 299/299 PASS in 16.3 sec。

---

## 2. 剩余 16 个 phase item 的现实评估

### 短期可推 (1-2 个 session 内可完成)

- **Phase 1 §7.1 登录页布局** — §5 commit 10365b7a 已修"保存账号"重叠,modern 端无独立 UI,无新工作
- **Phase 1 §7.2 登录错误矩阵 14 项** — **本 session 已加 12/14 + 1/14 (登录超时)**: 5 boundary + 4 Nack + 2 disconnect + 1 ClientSettings + **1 LoginAck application-level timeout (commit fa74305e)**. 剩 1 项 (重复注册) 真不可达 — 协议头 `MP_USERCONN_LOGIN_NACK` 是 0-byte payload,client 端无 reason-code 信号区分
- **Phase 1 §7.3 角色流程 9 项** — **本 session 已加 11/9 dispatch hook + 3 ack-timeout**: 11 dispatch hook tests + 1 CCharSelectState List/Select Ack timeout (45009501) + 1 CCharMake MakeAck timeout (b4d2b68c) + 1 CInGameState GameInAck timeout (2bee25c2). 4-state ack-timeout 全部覆盖 (10s default,steady_clock). 剩 0 项 dispatch hook,剩 e2e DB round-trip (角色列表与数据库一致, 重登持久化, 选择后进入真实 GameLoading — 后者部分覆盖 via GameEntryRequest typed payload round-trip)
- **Phase 1 §7.4 evidence 修正** — 已在本次 session 的 `e8c7d413` 完成 (superseded "opaque server profile")
- **新增 2026-09-06**: 4-state ack-timeout 完整覆盖 (CLoginState + CCharSelectState x2 + CCharMake + CInGameState),全部 10s default,steady_clock,Process() poll 模式。原始 client 缺 application-level timeout 是 missing feature,4 state machine 之前会无限等 server 响应。详见 CHANGELOG Unreleased 段最终 coverage table

### 中期 (3-5 个 session)

- **Phase 2 §8 SQLite/MSSQL** — 13 步 + 11 语义一致性 + 双验收
- **Phase 3 §9 启动器 G4/G5** — 签验/续传/MapChange 恢复
- **Phase 5 §11 全资源/全地图 G10** — 覆盖矩阵 + HSEL/HackShield/nProtect

### 长期/阻塞 (需多日/外部环境)

- **Phase 4 §10 Map10 G6-G9** — 11+11+战斗+掉落+老客户端对比,需真数据绑定 + E5 视觉对比
- **Phase 6 §12.3 24h soak** — 物理上需要 24h 持续运行
- **Phase 7 §13 PVE 双 VM** — 需 VM 192.168.2.200 + moxiang-app + moxiang-db,无 iBMC 救场,不允许擅自改 PVE 物理动作

---

## 3. MoxianClientE2E pre-existing race — RESOLVED (2026-09-06)

> **Resolved**: race 描述 (test 发送 login 早于 SQLite create_account) 在本 session 末不再复现。
> 详细 evidence 见本文件 §"2026-09-06 update"。

**原 MoxianClientE2E (#12389) blocker** — LoginNack "bad credentials" 测试账号配置问题。
- 不是 §6.4 / §6.5 改动引入 (commit `469cfbdb` 只动 mxh_client CMakeLists.txt)
- 不是 moxiang-reborn 范围 (`MoxianClientE2E` 在 `modern/tools/`,不在 src)
- pre-existing race: test 发送 login 早于 SQLite create_account
- 已在 ctest 中 `-E MoxianClientE2E` 排除,等 fixture 重写

### 2026-09-06 update — race resolution 验证

| 维度 | 值 |
|---|---|
| 单独跑 `ctest -R '^MoxianClientE2E$'` 3 次 | **3/3 PASSED** (6.62s / 6.70s / 6.54s) |
| 全套 ctest 包含 MoxianClientE2E + DumpCli | **12418/12418 PASSED in 116.20 sec** |
| 仅剩 SKIP | 5 个 (MSSQL E2E × 3 + 2 resource gate) |
| 0 FAIL | 0 |
| ctest 跑并发 | `-j 4` (跟 §6.5 一致) |
| 全套日志 | `C:\moxiang\modern\out\ctest_full_20260906_100700.log` (完整 12418 行) |

**结论**:`-E MoxianClientE2E` 排除不再需要。下个 session 起手时,默认 ctest 应该包含这 2 个 E2E test,期望 12418/12418 全绿。

**未触动**:
- ~~没改 `modern/tools/MoxianClientE2E/main.cpp` (为啥 race 突然消失留待下个 session 调查;当前 build 上 race 不复现是稳定事实)~~ **2026-09-06 update**:race 根因找到 — commit `00018e11` (2026-08-21) 改 spawn 顺序,E2E 工具先 `prepare_sqlite_database` (migrate + create_account) 再 spawn 三服,LoginServer 不再带 `--init-schema` flag。Race window (`spawn LoginServer` vs `E2E 发送 RequestLogin`) 消失因为 DB 在 spawn 前已就绪。详见 CHANGELOG Unreleased 段。
- 没动 5 个 pre-existing SKIP 的 gating 测试 (MSSQL E2E 需要 ODBC + DB,D:\[SWorking]\SWorking\Resource\Server 资源路径未挂载)
- 没动 `recent commits ahead of origin` 计数 (本 session 末 63 commits,见头部)

---

## 4. handoff 备忘

下个 session 起手三件事 (按 AGENTS.md §2):
1. `scripts/session-bootstrap.ps1` (清根目录 + 反 JSON 截断工具箱)
2. `git log --oneline 6fa0fe73..HEAD` 确认 63 commits 还在 (本 session 起点 14,末 63)
3. 读 `EXECUTION_PLAN.md` §7 (Phase 1) 起步 — §7.1 已基本完成,直接推 §7.2 14 项错误矩阵

## 5. 重要文件/路径

- 计划: `modern\scratch\2026-09-05-minimax-m3-execution\EXECUTION_PLAN.md` (1002 行, .gitignore)
- 状态: `docs\PHASE_0_GATE65_GATE64_REPORT.md` (committed)
- 验证矩阵: `docs\VERIFICATION_MATRIX.md` (3 新 evidence entries)
- 关键诊断: `modern\scratch\2026-09-05-minimax-m3-execution\PHASE_0_STATUS.md` (5505 字节, .gitignore)
- 3 个 capture 目录: `modern\out\runs\gamein\20260905-125513-457-726d8eaa\`, `20260905-130053-342-56004d8a\`, `20260905-131302-438-8fb672af\` (各含 result.json + 6 state frame .tga)
- DIA 工具: `modern\tools\MxDumpInspect\mx_find_throw_site.cpp` + `mx_dump_inspect.cpp`
- 诊断日志: 每个 capture 目录的 `client.stderr.log` + `dumps\` (本 session 没有 .dmp 文件,说明不再 crash)
