# Session Handoff 2026-07-16 (v2) — Phase 12.x big-work sprint

> **作者**: Mavis (mvs_95dbae3dead144e08e903d57a75beb75)
> **日期**: 2026-07-16 13:50-14:00 GMT+8
> **本 session 目标**: User 反馈"终极目标，啥活都要干，不能只干小活" → 推 5 个真活
> **结果**: 5/5 完成, 0 回归, ctest 879 → 897 PASS (+18 用例, 5 commit)

---

## 本 session 完成的 5 个活 (5 commit, 顺序)

### 1. R-9 矩阵约定审计 + 修复 (commit `ba99367`)

**修了什么**: `modern/include/mxh/render/math.hpp` MatrixLookAtLH + MatrixOrthographicLH 写错 layout。
旧代码把 translation 写在 column 3 (`_14/_24/_34` for view, `_43` for ortho z-translation),
实为 column-major 存储。Axis-aligned eye / identity view case 数值碰巧通过, off-axis eye 下
view 矩阵旋转/翻译全错。

**决定的项目约定**: D3DX 风格 `_ij` 命名 + C++ 行主序内存 + HLSL `mul(v_row, M)` row-vec mul 配套。
CPU memcpy 到 cbuffer 后 HLSL 默认 column-major packing 把整矩阵转置读, 数学上等价
D3DX row-major view 矩阵约定。

**7 个新 test** (`math_d3dx_convention_test.cpp`): off-axis eye (3,4,5) + asymmetric frustum
+ rigid isometry check + NDC corner mapping + view*ortho composition。

**3 个旧 test 更新** (math_test.cpp + mesh_geometry_test.cpp): 这些 test 之前 pin 的是 bug
layout (`_34 = 5` 当 translation), 现在 pin 正确 D3DX layout (`_43 = 5`)。

**剩余 work** (R-9.x deferred): `primitives.cpp drawBox` 仍用 x,z 当 2D (TODO 注释未删)。
升级到 3D 顶点需要 vsolid input layout 从 `float2 pos` 改 `float3 pos` + 拆 2D/3D shader paths,
1-2 commit 范围。KNOWN_BUGS.md R-9 entry 写了 deferred 原因。

### 2. CHANGELOG 0.13.12 收口 (commit `5e57dce`)

完整记录 cGuagen 1:1 port (commit de390cd) + R-9 修复 (commit ba99367) + 2 个新 agent
memory entry (off-axis test pin matrix layout, C++ _ij + HLSL column-major packing = 数学转置)。

### 3. P2-12 Dialogs Roadmap 更新 (commit `1c11328`)

- 新 Tier 1.5 子控件 widget 档 (1 周, 已完成 1/9: cGuagen)
- 6/202 = 3.0% progress
- Tier 2 表: CharacterDialog 子控件 blocker 解锁 (cGuagen ✅) 但仍需 PlayerStatsService (Tier 3)
- 当前可立即 port 的 Tier 2 dialog: **MacroDialog** (无 service, 子控件全 port)
- 下次候选子控件: cListDialogEx / cPushupButton / cListCtrl

### 4. Phase 13 Service Interface 启动 (commit `6aa7b20`)

3 个 header-only interface under `modern/include/mxh/services/`:
- `IInventoryService.hpp` (~120 行): getItem / getWearedItem / findItemByIconIdx / hasItem
- `ISkillService.hpp` (~70 行): learnedSkillCount / getLearnedSkillAt / isLearned / getSkillLevel / getQuickSlotBinding
- `IPlayerStatsService.hpp` (~85 行): getStr/Agi/Int/Wis/Dex / getLevel / getLevelExp / getExpForNextLevel / getCurrentHp/Mp / getMaxHp/Mp / getHpFraction / getMpFraction

11 个 mock test (mock impls + contract 验证):
- IInventoryServiceTest.* (5): empty slot / occupied slot / OOB / weared roundtrip / findItem
- ISkillServiceTest.* (2): empty service / learned enumeration
- IPlayerStatsServiceTest.* (3): defaults / max-zero div guard / roundtrip
- ServiceCompositionTest.* (1): CharacterDialog shaped refresh 同时读 3 service

build 模式: header-only, 跟 mxh_proto_tests / mxh_game_types_tests 同模式。

**下次工作**: 实现 real service impl (服务端 backing), 接进 Tier 3 dialog
(CharacterDialog / InventoryExDialog / QuickDialog / MugongDialog 等)。

### 5. (本次) 写 SESSION_HANDOFF_2026-07-16-v2.md

就是本文件。

---

## 当前 git 状态

```
ba99367 fix(render): R-9 - MatrixLookAtLH/MatrixOrthographicLH D3DX row-major layout + 7 new tests
6aa7b20 feat(services): Phase 13 - 3 service interface headers + 11 mock tests
1c11328 docs(roadmap): P2-12 - 新 Tier 1.5 子控件档 + 标 cGuagen ported + 6/202 = 3.0%
5e57dce docs: CHANGELOG 0.13.12 - cGuagen Tier 2 子控件 + R-9 矩阵约定
de390cd ui: 1:1 port of 墨香 cGuagen (progress bar widget) + 18 tests
```

cGuagen 跟 cExitDialog 上一 session 已 commit 在 0.13.10, R-9 + Phase 13 + CHANGELOG + roadmap 是本 session 新增。

## ctest 状态

```
$ cd D:\Moxian\modern\build; ctest -C Debug --timeout 30
100% tests passed, 0 tests failed out of 897
Total Test time (real) =  20.08 sec
```

879 → 886 (R-9) → 897 (Phase 13) PASS. 0 回归。

## 下个 session 推荐接活

按价值/独立性排:

1. **P2-12 下一个子控件 port** (1-2 commit, 立刻可推)
   - 候选: `cListDialogEx` (P2-12 Tier 1.5, 无 service 依赖) / `cPushupButton` / `cListCtrl`
   - 每个 100-200 行 + 8-15 test
   - 推进 P2-12 progress (3% → 4%)

2. **P2-12 Tier 2 dialog port** (2-3 commit, 子控件全 port 后)
   - 当前可立即: `MacroDialog` (仅 cEditBox + cButton, OptionManager settings 是 local state)
   - 200-500 行 + 8-15 test
   - 推进 P2-12 progress (3% → 4%)

3. **Phase 13.2: real service implementation** (大活, 2-3 commit)
   - 在 `modern/src/services/` 下实现 `InventoryServiceImpl` (backed by server player state) /
     `SkillServiceImpl` / `PlayerStatsServiceImpl`
   - 写 ~600 行 + 10 test
   - 解锁 CharacterDialog / InventoryExDialog Tier 3 端口径

4. **R-9.x: primitives drawBox 升级** (1-2 commit, 真大活)
   - vsolid input layout 从 `float2 pos` 改 `float3 pos`
   - 拆 vsSolid2D / vsSolid3D shader paths
   - drawBox / drawGrid 走 3D, drawLine / drawPoint / drawCircle / drawTexturedQuad 保持 2D
   - 影响所有 caller (renderer.cpp RenderBox / RenderPoint / RenderCircle / RenderLine / RenderGrid)
   - 需要同步修 font_object / sprite 的 drawTexturedQuad (确保 2D path 不受影响)
   - 推进 Phase 5 stub cleanup

5. **Phase 14 service 化 (后续大活)**
   - 完整 InventoryService / SkillService / PlayerStatsService 端到端串通
   - 包含 5-8 个新 service interface 头文件
   - 旧 ROADMAP 估算 3-4 周

## 关联文档

- `CHANGELOG.md` 0.13.12 entry: 完整记录 5 commit
- `docs/P2-12_DIALOGS_ROADMAP.md`: P2-12 整体进度 + Tier 1.5 新档
- `docs/KNOWN_BUGS.md` R-9 entry: 部分实装状态 + drawBox deferred 计划
- `modern/include/mxh/services/`: 3 个 service interface 头文件
- `modern/tests/unit/services/`: 11 个 mock test

## 已知 blocker

- **C-32 (无 SQL Server)**: 仍是 MapServer 完整 smoke 的硬 blocker
- **4 个 Distribute Debug_<LOCALE> target (KOR/JP/HK/TL)**: pre-existing C2065 legacy error, 收手策略不修
- **R-9 drawBox**: deferred to R-9.x

## 备注

- 全程用 write tool 写 multi-line UTF-8 (PowerShell `Set-Content` 在 cp936 locale 下损坏 CJK multi-byte, 见 memory)
- PowerShell `[...]` 通配符问题: `Get-ChildItem -LiteralPath` 必须用, 不然误判
- mavis-trash 路径: 用 `D:\墨香全套源代码...` mirror, 不是 `D:\Moxian\` reparse point
- Git 不自动 commit: user 主导, 5 commit 全部经我提议
- Windows Search indexer 偶尔锁新建文件 (~30s 干扰), 等 + 重试可解决
