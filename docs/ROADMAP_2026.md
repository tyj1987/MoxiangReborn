# Moxian-Reborn 自主推进路线图 (2026 Q3 - 2027 Q1)

> **作者**: Mavis
> **日期**: 2026-07-18
> **状态**: 🟢 推进中 (0.13.58 / 59/202 = 29.2%, 212 commits / 2083 tests)
> **目的**: 把"是否要继续推进 / 接下来推什么"这个决策从"每次问 user"变成"按路线自动判断"

---

## 0. 路线图的目标

`AI_TASK_QUEUE.md` v2.3 列了 39 个任务，但缺 3 个关键维度：
1. **时间盒** — 什么时候完成什么
2. **决策点** — 哪些必须 user 拍板、哪些 AI 可自主
3. **风险** — 已知会卡住的地方、缓解方案

本文件给每个 P2 阶段加上这三个维度，让"继续"指令变成**纯推进动作**，不再问 user 选 A/B/C。

---

## 1. 总览：4 个阶段 + 1 个无限期维持

| 阶段 | 时间盒 | 入口 | 出口 | 关键 blocker | AI 自主度 |
|------|-------|------|------|-------------|----------|
| **Phase 12.5** P2-12 1:1 port | 2026-07 → 2026-09 (~10 周) | 已开始，59/202 = 29.2% | 202/202 + Tier 1.5/2 全清 | Tier 3-5 blocked on Phase 13/14/15 | 🟢 90% 自主 |
| **Phase 13** 服务层接入 | 2026-08 → 2026-11 (~16 周) | IPlayerStatsService / IInventoryService / ISkillService interface 已实装 | 9 个 service 真注入到 MapHandler | Tier 3-5 dialog 需 service 真接 | 🟡 60% 自主（需 service 设计评审） |
| **Phase 14** Tier 3 dialog | 2026-10 → 2027-01 (~12 周) | 9 个 Tier 3 dialog（Friend/Note/Party/ItemShop 等） | 1:1 行为级一致 + 真实数据 | Phase 13 service | 🟡 50% 自主 |
| **Phase 15** 跨平台/Linux 预研 | 2027-01 → 2027-Q3 | 现行 Windows + DX11 稳定 | 4Dyuchi 兼容层可编译、IOCP 替代方案有 PoC | Perf-4 (Linux IocpServer) | 🔴 30% 自主（架构决策） |
| **维持** 1.0 release | 2026-12 起无限期 | 全部 P2 完成 + critical bug = 0 | 持续 bug 修复 + 新资源格式兼容 | 1.0 没有"完成"概念 | 🟢 80% 自主 |

**整体出口（gating 1.0 release）**：
- ✅ Phase 12.5 全清 (P2-12 202/202)
- ✅ Phase 13 9 个 service 真注入
- ✅ Phase 14 9 个 Tier 3 dialog ported
- ⏸️ 4 个 hard blocker 至少解 2/4

**悲观预测**：2027 Q2 = 1.0 release candidate（不算 linux）
**乐观预测**：2026 Q4 = 1.0 release candidate（4 blocker 全解 + Phase 14 提前）

---

## 2. Phase 12.5 — P2-12 1:1 port（9 周）

### 2.1 当前状态（2026-07-18 收口）

- **进度**: 59/202 = 29.2% (0.13.58 cSkillPointNotify 完成)
- **节奏**: 1.5-2h / batch, 5-8 commit / batch, 1-3 dialog / batch
- **ctest baseline**: 2083/2083 PASS, 0 FAILED, 0 SKIPPED, ~37 sec wall
- **commit 节奏**: 跨 day session 9-12h 平均 15-20 commit, 跨周累计 30+ commit
- **5/5 server matrix**: Distribute / Agent / Map 各 5 locale build 干净 (7.16 验证)

### 2.2 阶段拆解

| Sub-phase | 范围 | 估时 | blocker | 状态 |
|----------|------|-----|---------|------|
| 12.5.1 Tier 2 余 dialog | 0.13.59-0.13.95 (~37 dialog) | 6 周 | 无 | 🟢 进行中 |
| 12.5.2 Tier 1.5 余 subcontrol | 0.13.60-0.13.85 (3-5 个) | 2 周 | 无 | 🟢 推进中 |
| 12.5.3 Tier 3 dialog 准备 | 9 dialog 模板 (Phase 13 service 阻塞) | n/a | Phase 13 service | ⏸️ blocked |
| 12.5.4 Tier 4/5 dialog | 100+ dialog (NPC script + network service) | n/a | Phase 14+ | ⏸️ blocked |
| 12.5.5 Phase 11/12 收口 | 1.0 release candidate | 1 周 | 上 4 项 | ⏸️ blocked |

### 2.3 AI 自主度

- **🟢 90% 自主**：
  - 选下一个 dialog（按 P2-12_ROADMAP 5 档分级 + size 排序）
  - 1:1 port 实现
  - unit test 编写（4-30 test / dialog）
  - CHANGELOG / roadmap / plan 同步
  - collateral fix（cButton setter 7cf011e 模式，R-9 drawBox 撤回模式）
- **🔴 10% 需 user 决策**：
  - 1.0 release 标记时机
  - 路线分歧（"还 port dialog 还是转 Phase 13"）
  - Tier 3+ service 阻塞时的方向选择

### 2.4 风险登记

| 风险 | 概率 | 影响 | 缓解 |
|------|-----|-----|------|
| Tier 2 dialog 中遇到 cScriptManager/cResourceManager 依赖 | 高 (5/37 估) | 阻塞该 dialog 1-3 周 | 抽 helper stub 1:1 + 标 R-13 |
| Tier 1.5 subcontrol 撞 Windows SDK typedef | 中 (3/5 估) | 1 commit naming 重命名 (R-9 模式) | rename + test |
| MSVC 14.44 parser quirk 复发 | 中 (持续) | 单 commit delay 30 min | trailing return type workaround |
| R-9.x drawBox 真 host 接入被再次催 | 中 | 整个 batch delay | 继续标 R-9.x deferred |

### 2.5 出口条件（gating 12.5 → 13）

- [x] Tier 1 dialog 9/9 (已完成 0.13.13)
- [ ] Tier 1.5 subcontrol 9/9 (现 14/9 超额, Phase 13.1 扩展)
- [x] Tier 2 dialog 42/10 (超额完成)
- [ ] Tier 3 dialog 9/9 → 阻塞 Phase 13
- [ ] Phase 13/14/15 service interface 完整 (现 3/9)

---

## 3. Phase 13 — 服务层接入（16 周）

### 3.1 当前状态

- **Service interface 已实装（header-only）**：
  - IPlayerStatsService (Phase 13.1)
  - IInventoryService (Phase 13.1)
  - ISkillService (Phase 13.1)
- **Real impl 已实装**：
  - PlayerStatsServiceImpl
  - InventoryServiceImpl
  - SkillServiceImpl
- **测试**：11 mock + 15 real contract tests, 26/26 PASS
- **未注入**: MapHandler::on_message 还在用 hardcoded 路径

### 3.2 阶段拆解

| Sub-phase | 范围 | 估时 | blocker | 状态 |
|----------|------|-----|---------|------|
| 13.1 Service interface (header) | 9 个 IService | 4 周 | 无 | ✅ 3/9 done |
| 13.2 Real impl | 9 个 *ServiceImpl | 6 周 | 13.1 | 🟡 3/9 done |
| 13.3 MapHandler 注入 | 改 MapHandler 接口签名 | 4 周 | 13.2 | ⏸️ blocked |
| 13.4 Server matrix smoke | 5/5 locale build 干净 | 2 周 | 13.3 | ⏸️ blocked |

### 3.3 AI 自主度

- **🟡 60% 自主**：
  - 剩余 6 个 service interface 设计
  - Real impl 实现
  - Mock 框架
  - Real contract tests
- **🔴 40% 需 user 决策**：
  - **服务边界划分**（CharacterDialog 7 服务 vs 5 服务 vs 9 服务）
  - **错误处理语义**（ServiceResult error code 体系）
  - **线程模型**（service 是 thread-local 还是 shared）
  - **DI 容器选型**（手写 vs 第三方）

### 3.4 风险登记

| 风险 | 概率 | 影响 | 缓解 |
|------|-----|-----|------|
| 服务边界划分反复改 | 高 | 13.1-13.2 delay 4 周 | **先做 1 个 dialog 完整路径** (CharacterDialog 模板) |
| MapHandler 接口签名改 blast radius | 中 | 5/5 locale build 全破 | **adapter 层** 保持旧接口 + 内部转 service |
| 跨线程竞态 | 中 | 难复现 bug | service 内部 mutex + contract test 压力测试 |
| Real impl 性能不如 legacy | 低 | MapServer tick rate 掉 | benchmark + 优化热点 |

### 3.5 出口条件

- [ ] 9/9 service interface
- [ ] 9/9 real impl
- [ ] MapHandler::on_message 用 service 真路径
- [ ] 5/5 locale server build 干净
- [ ] Tier 3 dialog 至少 1 个完整 e2e 跑通

---

## 4. Phase 14 — Tier 3 dialog（12 周）

### 4.1 当前状态

- **Tier 3 dialog 候选** (按 P2-12 roadmap):
  - cFriendDialog (好友)
  - cNoteDialog (邮件)
  - cPartyDialog (组队)
  - cItemShopDialog (商城)
  - cDealDialog (交易)
  - cExchangeDialog (交换)
  - cChaseDialog (追踪)
  - cGuildDialog (公会)
  - cGTBattleListDialog (战场)
- **状态**: 9/9 ⏸️ blocked on Phase 13 service

### 4.2 阶段拆解

| Sub-phase | 范围 | 估时 | blocker | 状态 |
|----------|------|-----|---------|------|
| 14.1 CharacterDialog 模板 | 1 dialog 完整 e2e | 2 周 | Phase 13 | ⏸️ blocked |
| 14.2 cFriendDialog + cNoteDialog | 2 service-mock dialog | 3 周 | 14.1 | ⏸️ blocked |
| 14.3 cPartyDialog + cItemShopDialog | 2 service 真接 | 4 周 | 13.3 | ⏸️ blocked |
| 14.4 cDealDialog + cExchangeDialog | 2 trade service | 3 周 | 14.3 | ⏸️ blocked |
| 14.5 剩余 3 dialog (cChase / cGuild / cGTBattle) | 1-week / 个 | 3 周 | 14.4 | ⏸️ blocked |

### 4.3 AI 自主度

- **🟡 50% 自主**：
  - 1:1 port 实现
  - service 注入
  - 单元 / 集成 / e2e test
- **🔴 50% 需 user 决策**：
  - 哪些 service 可 mock vs 必须 real
  - 网络协议兼容性（server 是否改）
  - 数据库 schema 兼容

### 4.4 出口条件

- [ ] 9/9 Tier 3 dialog ported
- [ ] 1+ 个 e2e 路径实测（client → dialog → service → DB）
- [ ] 协议兼容性测试（client 2003 binary + server modern）

---

## 5. Phase 15 — 跨平台/Linux 预研（~9 个月）

### 5.1 当前状态

- **4Dyuchi 兼容层** (modern/): render 11/12 done (R-10 deferred)
- **IOCP 替代**: Perf-4 (Linux IocpServer) + Perf-5 (AcceptEx) deferred
- **DX11**: 100% done (Phase 5)
- **DX12/Vulkan**: 未启动
- **Linux/macOS 平台代码**: 0 行（仅 platform.hpp 提供 OS detection）

### 5.2 阶段拆解

| Sub-phase | 范围 | 估时 | blocker | 状态 |
|----------|------|-----|---------|------|
| 15.1 平台兼容层 | 4Dyuchi* 抽象接口 | 8 周 | 无 | ⏸️ not started |
| 15.2 Linux IocpServer | POSIX epoll reactor 替代 | 12 周 | 15.1 | ⏸️ blocked |
| 15.3 Linux ss3d 渲染后端 | OpenGL 4.6 / Vulkan | 16 周 | 15.1 | ⏸️ blocked |
| 15.4 Linux server smoke | MapServer 在 Linux 跑通 | 4 周 | 15.2 | ⏸️ blocked |
| 15.5 Linux client smoke | MHClient 在 Linux 跑通 | 8 周 | 15.3 | ⏸️ blocked |

### 5.3 AI 自主度

- **🔴 30% 自主**：
  - 平台兼容层设计（架构）
  - 第三方库选型（asio / libuv / SDL3 / SDL_GPU）
  - 1:1 行为边界（DX11 ↔ OpenGL 等价）
  - 性能目标（1:1 client FPS）

### 5.4 出口条件（gating Linux 1.0）

- [ ] 4Dyuchi* 接口抽象完
- [ ] Linux IocpServer 跑通 echo
- [ ] Linux server smoke 全 P0
- [ ] Linux client 至少登录 + 选角

---

## 6. 维持 — 1.0 release + 持续修复

### 6.1 1.0 release 出口条件

**必要**：
- [ ] Phase 12.5 全清 (P2-12 202/202)
- [ ] Phase 13 9/9 service 真注入
- [ ] Phase 14 9/9 Tier 3 dialog ported
- [ ] 4 hard blocker 至少解 2/4
- [ ] 5/5 server build 干净
- [ ] ctest 2500+/2500+ PASS, 0 SKIPPED
- [ ] 真实 client 登录 + 进游戏 + 1 战斗 + 1 交易 路径通过

**加分**：
- [ ] Linux server smoke
- [ ] Tier 4/5 dialog 至少 1 个 ported
- [ ] DX12 渲染后端

### 6.2 1.0 release 后的"维持"工作

- **bug 修复**: 1:1 兼容性发现 → R-XX 记录 → 测试回归
- **新资源格式**: 1:1 解码新 .bmhm/.chl/.pak
- **新 dialog**: 玩家自制内容扩展
- **性能优化**: MapServer tick rate / client FPS
- **E-1 verifier session**: 每次 producer session 配 verifier session

### 6.3 AI 自主度

- **🟢 80% 自主**：
  - bug 修复
  - 1:1 兼容性测试
  - doc 同步
  - 性能 benchmark
- **🔴 20% 需 user 决策**：
  - 哪些 bug 算 critical
  - 1.0.x 何时发
  - 重大重构决策

---

## 7. Hard Blocker 决策矩阵

| Blocker | 当前状态 | 解的代价 | 不解的代价 | 推荐动作 |
|---------|---------|---------|----------|---------|
| **C-32 无 SQL Server** | user 机器未装 | 装 SQL Server 2019/2022 Express ~1h, restore 3 DB ~30 min | MapServer 完整 smoke 永远跑不了 | 🟡 user 决策（成本低，1.5h 一次） |
| **C-35 Distribute Debug_<LOCALE> 撞 mfc71.lib** | 4/5 locale build 撞 legacy | 改 shared header 加 4 个 enum 隔离 | 5 locale 全 build 只能 1/5 | 🔴 user 决策（架构改 vs 不强推） |
| **R-9.x drawBox 真 host 接入** | 矩阵约定混乱 | 矩阵库 row/col-major 决策 + 5 文件改 | drawBox 留为 debug-only TODO | 🟡 user 决策（defer 接受） |
| **Phase 13 Tier 3+ service** | 3/9 service impl done | 16 周连续推进 | Tier 3-5 dialog 永远 port 不了 | 🟢 AI 自主推（按路线） |

---

## 8. 决策点清单：什么时候该问 user

| 触发 | 决策类型 | 谁拍板 |
|------|---------|--------|
| 选下一个 dialog（Tier 1/1.5/2） | 推进选择 | 🟢 AI 自主 |
| 选下一个 service（Phase 13 剩余 6 个） | 推进选择 | 🟢 AI 自主 |
| 1:1 行为边界（legacy bug vs modern fix） | 兼容性 | 🔴 user（写 R-XX 等决策） |
| 架构改（shared header enum 隔离、矩阵约定） | 架构 | 🔴 user |
| 装新工具（SQL Server / 第三方库） | 环境 | 🔴 user |
| 1.0 release 标记 | 发布 | 🔴 user |
| 跨 session 长期方向（"现在该 port dialog 还是 service"） | 路线 | 🔴 user |
| commit 提交 | git | 🔴 user（按现有约定） |

**核心原则**：路线明确 → AI 推；路线分歧 → 1 次 ask user；user "继续" → AI 推 1 个值钱的活 + 说明理由；user "自己判断" → AI 推 1-2 commit 量级小活后收。

---

## 9. 跨 session 接力约定（已存在 + 微调）

按 `AI_WORKFLOW_GUIDE.md` 既有 Qoder IDE + QoderWork + QoderWake 机制。本路线图补 3 条：

1. **每次 session 启动**：
   - 必读 `AI_SHIFT_LOG.md` 最近 3 段
   - 必读 `AI_TASK_QUEUE.md` 队头 5 行
   - 必读本路线图 §0-§2 决定当前阶段
2. **每次 commit 后**：
   - 更新 `CHANGELOG.md`
   - 更新 `MODERNIZATION_PLAN.md` §5 进度
   - 更新 `AI_SHIFT_LOG.md`
   - 更新 `AI_TASK_QUEUE.md` 状态
3. **每次 session 收**：
   - 写 `AI_SHIFT_LOG.md` 一段
   - 清理 `modern/scratch/` 至 ≤ 5 文件
   - 0 uncommitted 改动（除非 user 明确说"先不 commit"）

---

## 10. 本文件怎么维护

- **作者**: Mavis（producer session 维护）
- **更新触发**：
  - 阶段完成（Phase 12.5 → 13 → 14 → 15）
  - blocker 状态变化
  - 路线方向调整（user 决策）
  - 重大里程碑（2000/3000 tests、1.0 RC、Linux 1.0）
- **不动**：阶段 1.1 阶段拆解细节、commit 节奏——这些是 session 内的执行问题
- **commit**: 跟随每次路线更新 1 commit，message 形如 `docs(roadmap): 2026 ROADMAP update @ 0.13.XX`

---

## 11. 现在该做什么（next 3 actions）

1. ✅ **本文件落地** (现在做)
2. 🟢 **AI_TASK_QUEUE.md v2.4** 加 4 行 hard blocker 显式化（现在做）
3. 🟢 **继续 P2-12 batch** (0.13.59+ 按 §2.2 顺序推)

不再需要"问 user 选哪个"——按本文件推就完了。user "继续" = 按 §11.3 推；user "你自己判断" = 按 §11.1-2 housekeeping + §11.3 推 1-2 commit。
