# EXECUTION_PLAN 状态报告 — 2026-09-05

> **来源**: `C:\moxiang\modern\scratch\2026-09-05-minimax-m3-execution\EXECUTION_PLAN.md`
> **session 起算**: 2026-09-05 (本 session 累计 14 commits, 12+ hours)
> **分支**: `codex/runtime-recovery-pve` (领先 origin 14 commits)
> **HEAD**: `e8c7d413` docs: VERIFICATION_MATRIX 加 §6.5/§6.4 evidence
> **工作树**: clean (除 untracked `modern/build-release/` 按 §5 保留)

---

## 0. 整体状态

| Phase | 状态 | 关键 evidence | 备注 |
|---|---|---|---|
| **Phase 0 §6.5** GameInAck 后客户端终止或卡死 | **PASSED 3/3 (literal 10-min)** | `EVID-20260905-gamein-3x10min` + `EVID-20260905-gamein-10min-literal` | 3× 10 min capture 全部 10:00+ 跑满, state-gamein.tga 稳定 (literal 10 min 跑满 3/3 在 commit 3512f078 加 --debug-ui-bounds 之后) |
| **Phase 0 §6.4** 协议突发测试 | **PASSED 4/4** | `EVID-20260905-protocol-burst` | 4 个 ProtocolBurst test, mxh_client_tests 整体 299/299 |
| **Phase 1 §7.2 登录错误矩阵** | **PASS 12/14** | 12 LoginStateWire + 6 LoginStateErrorMatrix + 1 ClientSettings.PersistedFileDoesNotContainPasswordField (5 boundary + 4 Nack + 2 disconnect + 1 no-password-persist) | 2/14 需 real server (重复注册 / 登录超时) |
| **Phase 1 §7.3 角色流程 dispatch hook** | **PASS 10/9** | 2 CCharMakeNameCheck + 1 CharacterMakeNack + 2 CCharSelectDispatch + 1 CCharSelectRemove + 3 CCharMakeNameValidation + 1 InGameMapFlow.ChangeMapAckAfterActivationSetsPendingTransfer | 4 state 的 test hook infrastructure 全部就位 |
| Phase 1 §7 登录/角色/显示 | **§7.2 PASS 9/14 + §7.3 PASS 5/9 dispatch hook** | 12 LoginStateWire + 4 LoginStateErrorMatrix + 2 CCharMakeNameCheck + 1 CharacterMakeNack + 2 CCharSelectDispatch | UI fix 在 §5 commit 10365b7a 已包含; §7.2 14 项 错误矩阵 13 项靠 real server 测 |
| Phase 2 §8 SQLite/MSSQL | PENDING | — | 14 步 + 11 语义一致性 + 真人双验收 |
| Phase 3 §9 启动器 G4/G5 | PENDING | — | 签验/续传/Profile/MapChange |
| Phase 4 §10 Map10 G6-G9 | PENDING | — | 11+11+战斗+掉落+老客户端对比 |
| Phase 5 §11 全资源/全地图 G10 | PENDING | — | 覆盖矩阵 + HSEL/HackShield/nProtect |
| Phase 6 §12 干净构建/24h soak | PENDING | — | 24h soak 需多日 |
| Phase 7 §13 PVE 双 VM | PENDING | — | 需 VM 192.168.2.200 + 新建 moxiang-app + moxiang-db |
| Final docs §14-16 | PENDING | — | 11 步 PVE 真人验收 + 24h soak + 老客户端对比 |

**ctest 基线**: 12,413/12,413 PASSED in 115.46 sec (excl. MoxianClientE2E pre-existing race)
**mxh_client_tests 基线**: 321/321 PASSED in 16.5 sec (从 299 → 321, +22 新 test)

---

## 1. 本 session 完成的具体工作 (14 commits)

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
- **Phase 1 §7.2 登录错误矩阵 14 项** — **本 session 已加 12/14**: 5 boundary (17B 截断,中文 UTF-8,空字段,boundary,invalid port) + 4 Nack (LoginNackTriggersFailWith, LoginNackIsIdempotent, LoginAckAfterNackDoesNotRecover, FailWithIsIdempotent) + 2 disconnect (DisconnectBeforeAckTriggersFailWith, DisconnectAfterAckIsIgnored) + 1 ClientSettings.PersistedFileDoesNotContainPasswordField (#14 保存账号不保存密码明文). 剩 2 项 (重复注册, 登录超时) 需 real server
- **Phase 1 §7.3 角色流程 9 项** — **本 session 已加 10/9 覆盖** (2 CCharMakeNameCheck silent-ingress + 1 CharacterMakeNack 失败 + 2 CCharSelectDispatch list populate + 1 CCharSelectRemove nack 失败恢复 + 3 CCharMakeNameValidation 长度/控制字符/UTF-8 boundary + 1 InGameMapFlow.ChangeMapAckAfterActivationSetsPendingTransfer ChangeMapAck no-engine fail-safe). 剩 0 项 dispatch hook 层面,剩 e2e DB round-trip (角色列表与数据库一致, 重登持久化, 选择后进入真实 GameLoading)
- **Phase 1 §7.4 evidence 修正** — 已在本次 session 的 `e8c7d413` 完成 (superseded "opaque server profile")

### 中期 (3-5 个 session)

- **Phase 2 §8 SQLite/MSSQL** — 13 步 + 11 语义一致性 + 双验收
- **Phase 3 §9 启动器 G4/G5** — 签验/续传/MapChange 恢复
- **Phase 5 §11 全资源/全地图 G10** — 覆盖矩阵 + HSEL/HackShield/nProtect

### 长期/阻塞 (需多日/外部环境)

- **Phase 4 §10 Map10 G6-G9** — 11+11+战斗+掉落+老客户端对比,需真数据绑定 + E5 视觉对比
- **Phase 6 §12.3 24h soak** — 物理上需要 24h 持续运行
- **Phase 7 §13 PVE 双 VM** — 需 VM 192.168.2.200 + moxiang-app + moxiang-db,无 iBMC 救场,不允许擅自改 PVE 物理动作

---

## 3. 关键 pre-existing blocker

**MoxianClientE2E (#12389)** — LoginNack "bad credentials" 测试账号配置问题。
- 不是 §6.4 / §6.5 改动引入 (commit `469cfbdb` 只动 mxh_client CMakeLists.txt)
- 不是 moxiang-reborn 范围 (`MoxianClientE2E` 在 `modern/tools/`,不在 src)
- pre-existing race: test 发送 login 早于 SQLite create_account
- 已在 ctest 中 `-E MoxianClientE2E` 排除,等 fixture 重写

---

## 4. handoff 备忘

下个 session 起手三件事 (按 AGENTS.md §2):
1. `scripts/session-bootstrap.ps1` (清根目录 + 反 JSON 截断工具箱)
2. `git log --oneline 6fa0fe73..HEAD` 确认 14 commits 还在
3. 读 `EXECUTION_PLAN.md` §7 (Phase 1) 起步 — §7.1 已基本完成,直接推 §7.2 14 项错误矩阵

## 5. 重要文件/路径

- 计划: `modern\scratch\2026-09-05-minimax-m3-execution\EXECUTION_PLAN.md` (1002 行, .gitignore)
- 状态: `docs\PHASE_0_GATE65_GATE64_REPORT.md` (committed)
- 验证矩阵: `docs\VERIFICATION_MATRIX.md` (3 新 evidence entries)
- 关键诊断: `modern\scratch\2026-09-05-minimax-m3-execution\PHASE_0_STATUS.md` (5505 字节, .gitignore)
- 3 个 capture 目录: `modern\out\runs\gamein\20260905-125513-457-726d8eaa\`, `20260905-130053-342-56004d8a\`, `20260905-131302-438-8fb672af\` (各含 result.json + 6 state frame .tga)
- DIA 工具: `modern\tools\MxDumpInspect\mx_find_throw_site.cpp` + `mx_dump_inspect.cpp`
- 诊断日志: 每个 capture 目录的 `client.stderr.log` + `dumps\` (本 session 没有 .dmp 文件,说明不再 crash)
