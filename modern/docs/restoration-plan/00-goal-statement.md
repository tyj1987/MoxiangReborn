# Goal Statement：Moxiang 渲染 + UI 视觉 1:1 还原

> 适用范围：`moxiang` 项目
> 任务编号：M-R0 → M-R7
> 文档状态：不可破坏。任何对"完成"的定义有歧义都拿这份文档当裁判
> 关联：[01-rendering-ui-1to1.md](./01-rendering-ui-1to1.md) 详细还原计划

---

## 1. 任务的精确范围

**只做这一件事**：把现代 Moxian 客户端的渲染管线 + UI 资源装载 + 165 个 dialog 视觉还原到和老版（2003-2010 韩国 2D MMORPG《墨香》）**像素级 1:1**。

**精确边界**（出了这个范围不做）：

| 范围 | 在 | 出 |
|---|---|---|
| 渲染管线（DX11） | ✅ 重构、改写、调优 | ❌ 改老 DX8 代码 |
| UI 资源装载（.bin / sprite / hard path） | ✅ 重写 modern 等价物 | ❌ 改老 `墨香【源码】` |
| 165 个 dialog 视觉 1:1 | ✅ 用老 .bin 装载 + 老 sprite | ❌ 用假色块绕开 |
| 性能（5fps → 30fps） | ✅ frustum cull / batch / instancing | ❌ 改玩法逻辑 |
| 协议头 `[CC]Header/Protocol.h` | ❌ 不可改 | ❌ 不可改 |
| 资源文件 `墨香【源码配套资源】/PlayDH/` | ❌ 不可改 | ❌ 不可改 |
| 老源码 `墨香【源码】/[Client]MH/` | ❌ 不可改 | ❌ 不可改（除非修编译 bug） |
| 玩法 / 数值 / 爆率 / Boss 刷新 | ❌ 不可改 | ❌ 不可改 |
| HSEL / HackShield / nProtect 签名 | ❌ 实现可换，签名不可换 | — |

---

## 2. "完美完成" 的不可争议定义

**所有 7 个里程碑全部 go + 全部用 screenshot 锁死**：

| 里程碑 | "完成" 的不可争议判据 |
|---|---|
| M-R0 视觉基线 | `scripts/visual-smoke.ps1` 跑通；6 张 modern 状态截图 + 6 张 legacy 同状态截图就位；每对都算 SSIM；modern/legacy 报告存在 `modern/docs/visual-baseline.md` |
| M-R1 cResourceManager | 7 张 hard path 表装载完成；idx → hard path 100% 命中（0 missed）；单测全过；cResourceManager 单文件 SHA-256 入库 |
| M-R2 sprite atlas | atlas 切分 SHA-256 = 老版 1:1；同 idx 多次查返回同一 sprite 句柄；释放无 SRV 泄漏 |
| M-R3 165 dialog 装载 | MoxianClient 启动时 165 .bin 全部 `parse_interface_script` OK（0 exception、0 null dialog）；dialog 树挂到 cWindowManager 完成 |
| M-R4 165 dialog Init | **每个 dialog 各一张 golden 截图**（165 张），与老版 MoxianClient 同状态对比 **SSIM ≥ 0.95**；任意一张不达 → 整个 M-R4 fail |
| M-R5 性能 5→30fps | 1920×1080 + 满 HUD + 满 dialog 开启 + 334 static mesh + 全 terrain + 16 NPC + 5 怪 → **平均 fps ≥ 30**，最低 ≥ 25；视觉 SSIM ≥ 0.98（优化后不能改画面） |
| M-R6 IME + focus | cIME 输入中文/英文/数字 与老版 SSIM 截图 1:1；focus chain Tab/方向键 1:1；CMousePointer sprite 与老版 1:1 |
| M-R7 分辨率自适应 | 800×600 / 1920×1080 / 2560×1440 三档 × 6 状态 = 18 张 SSIM 全 ≥ 0.95 |

**额外硬指标**：
- ✅ **每个 dialog 的 sprite 必须来自老 .bin 资源**，不是 `CreateSolidSpriteObject(0xAA181010)` 这种假色块
- ✅ **每个 dialog 必须通过 cDialog 树渲染**，不能用 `drawSpriteQuad` 循环绕开
- ✅ **MoxianClient main.cpp 顶部不能再有 "original InterfaceScript art is wired in a later phase" 这种承认**（那是 project 失守的标志）

**任何一条不达成 = 任务未完成**。不接受"基本完成""大部分完成""视觉差异不大"等模糊声明。

---

## 3. 验证策略：screenshot 锁死一切

### 3.1 工具栈

- `scripts/visual-smoke.ps1`：自动启动 MoxianClient 进 6 状态（登录/选角/char-make/空场/HUD-only/inventory-open），每状态截 1 张
- `scripts/dialog-screenshot.ps1`：对单个 dialog 单独截（OnLoad + SetActive + Render 一帧）
- `scripts/visual-compare.py`：PIL SSIM + 直方图对比
- `modern/tests/visual/*.golden.png`：每个 dialog 一张 golden（165 张），加 6 张全状态 golden

### 3.2 SSIM 阈值（任何不达 = 立即停）

| 元素 | 阈值 | 失败处理 |
|---|---|---|
| 整体页面 | SSIM ≥ 0.92 | 立即停，回查 M-R3 / M-R4 |
| 整体背景（地形+天空） | SSIM ≥ 0.98 | 立即停，回查 M-R3 |
| Static mesh 轮廓 | SSIM ≥ 0.95 | 立即停，回查 M-R5 chunk 合并 |
| Dialog 边框/底色 | SSIM ≥ 0.95 | 立即停，回查 M-R4 该 dialog Init |
| Dialog 文字 | SSIM ≥ 0.85 | 字体 sub-pixel 差异允许 |
| Icon sprite | **SHA-256 完全一致** | 立即停，回查 M-R2 |
| 性能 | avg fps ≥ 30，min fps ≥ 25 | 立即停，回查 M-R5 |

### 3.3 拒绝任何"看着像"

- **不允许**写"视觉效果接近老版"
- **不允许**写"基本能玩"
- **不允许**写"看着像 1:1"
- **不允许**写"N% 完成"（除非附 SSIM 表）

唯一允许的"完成"声明 = **SSIM 表全过 + screenshot 全部存档**。

---

## 4. 与过去的切断：拒绝 121 轮"全过"假断言

**Moxiang Round 11-125 (commit 历史的 121 轮 dialog port) 的方法**：

> 数据/状态机 1:1 = 报"全过" = 报告"完成"

**这种方法是错的**。证据：
- 121 轮"全过"后，codex 实测发现：inventory sprite 看不见、5fps、字体乱码
- `modern/src/compat/` 实际为 0 files（AGENTS.md 说 100% 是假话）
- `cWindow::Render` 永远 no-op（`SetBasicImage` 只在 cWindow 自身定义里出现）
- MoxianClient main.cpp 显式注释"original InterfaceScript art is wired in a later phase"

**新方法**（本 goal statement 强制）：

> **数据 1:1 + 视觉 1:1 + screenshot 锁死 + SSIM 通过 = 才能报"完成"**

**任何 agent（包括 Mavis 后续 session）违反这条 = 任务失败，不允许 commit**。

---

## 5. 执行规范

### 5.1 每个 session 启动必读

新 session 启动时必读（按顺序）：
1. `modern/docs/restoration-plan/00-goal-statement.md`（本文件）
2. `modern/docs/restoration-plan/01-rendering-ui-1to1.md`（详细计划）
3. `modern/docs/visual-baseline.md`（M-R0 产出）
4. 上一 session 的最后一个 commit message
5. `git log --oneline -20`（看历史 commit）

### 5.2 Commit 规范

**禁止**：
- `dialog: Round 122 全过`（Round 模式已废）
- `ui: 1:1 还原完成`（必须附 SSIM）
- `compat: 100% 完成`（必须附 grep / ls 证据）

**必须**：
- 1 commit = 1 个 milestone 子任务（或 1 个 dialog 修）
- commit message 写"做了什么 + 怎么验证 + 验证结果"
- 每个 commit 必跑 build + ctest
- 每个 commit 必跑 visual-smoke（如有视觉变化）
- 每个 commit message 末尾必带链接：`视觉对比：screenshots/m-r4-inventory-before.png vs screenshots/m-r4-inventory-after.png SSIM=0.96`

### 5.3 汇报节奏

每个 milestone 结束时汇报，**必带**：
- 完成了什么（具体到文件 + 行数）
- 怎么验证的（screenshot 路径 + SSIM 数字）
- 还有什么没做（明示，不掩饰）
- 下一步是什么

**禁止**：
- "全过""基本过""应该过了""看着没问题"
- 单独的"覆盖率"汇报（必须配视觉证据）

### 5.4 失败处理

任何 screenshot 验证失败：
1. **立即停**当前工作
2. 列出失败清单（哪个 dialog / 哪个像素区 / SSIM 数字）
3. 不发新 commit
4. 写一份 `modern/docs/visual-regression.md` 记录失败
5. 回查上一个成功的 commit（`git bisect` 找 regression 引入点）
6. 修完后再发 commit

**禁止**：
- 调高 SSIM 阈值让数字好看
- 用"用户没注意"作为通过理由
- 加 `if (debug) skip_render` 绕开

---

## 6. 跨 session 上下文（防止失忆）

每个新 session 启动时必做：
1. 读本 goal statement（不读不能开工）
2. 读 `git log --oneline -20`
3. 读 `git status` 看 working tree
4. 跑 `scripts/visual-smoke.ps1` 看当前基线
5. 与 `modern/docs/visual-baseline.md` 对比，看是否有 regression

如果 working tree 脏 + 上一 commit 不可复现：
- 立即停
- 不基于脏 working tree 继续
- 写一份 `modern/docs/session-handoff.md` 记录失忆点
- 让用户决定要不要重置

---

## 7. 不可破坏的红线（违反 = 任务失败）

1. ❌ 写"X/Y tests PASS = 1:1 完成" 之类无视觉证据的声明
2. ❌ 绕过 cDialog 树用 `drawSpriteQuad` 循环画 HUD
3. ❌ 用假色块（`CreateSolidSpriteObject(0xAA181010)` 之类）替代真实 sprite
4. ❌ 调高 SSIM 阈值让数字好看
5. ❌ 改 `[CC]Header/Protocol.h`
6. ❌ 改 `墨香【源码】/` 逻辑（仅编译 bug 修可以）
7. ❌ 改 `墨香【源码配套资源】/PlayDH/` 资源文件
8. ❌ 改玩法 / 数值 / 爆率 / Boss 刷新
9. ❌ 把 Round 11-125 的"全过"方法复活
10. ❌ 报告"完成"但没有 screenshot + SSIM 表

**以上任何一条违反，任务即为失败，无论跑了多少 commit**。

---

## 8. 完成 = 一份 SSIM 表 + screenshot 存档

任务完成的**唯一**标识 = 下面这份文件存在且填满：

```
modern/docs/visual-completion-report.md
  - 6 张全状态 SSIM 表（modern vs legacy，每状态 ≥ 0.92）
  - 165 张 dialog 截图 SSIM 表（每 dialog ≥ 0.95）
  - 18 张分辨率自适应 SSIM 表
  - 性能测试日志（avg fps ≥ 30, min fps ≥ 25, 1920×1080 + 满 HUD + 满 dialog）
  - 老版 MoxianClient vs modern MoxianClient 同 5 状态 5 分屏对比图
  - 签名：Mavis + 日期
```

**这份文件不存在 = 任务未完成**，无论 git log 多漂亮、commit message 多自信。

---

## 9. 不要做（防止 scope 漂移）

- 不要改协议头
- 不要改老源码
- 不要改资源
- 不要写"全过"假断言
- 不要绕过 cDialog 树
- 不要加新功能
- 不要重新设计资源格式
- 不要在没 screenshot 的情况下发"完成" commit
- 不要为了"看着像"调高 SSIM 阈值

---

## 10. 用户（脱永军）已认可的原则

- ✅ 接受 7 个里程碑的顺序（M-R0 → M-R7）
- ✅ 接受 ~20.5 天估时
- ✅ 接受 SSIM 阈值（dialog 0.95 / 整体 0.98 / 文字 0.85）
- ✅ 接受"必须 screenshot 锁死"
- ✅ **先做 M-R0**（半天内出视觉基线）

**用户偏好（来自 memory）**：
- 完全自主规划、自主执行，不反复确认
- git 提交去 AI 痕迹（自然 commit message、真实 author、不用 conventional commit 前缀）
- 文档中英双语
- 始终中文交互
- 安全红线：凭据不进 AI 对话

---

## 11. Next Step

**现在立刻开 M-R0**：
1. 写 `scripts/visual-smoke.ps1`
2. 写 `scripts/dialog-screenshot.ps1` 框架
3. 写 `scripts/visual-compare.py`（PIL SSIM）
4. 跑出 6+6 张基线截图（modern + legacy）
5. 产出 `modern/docs/visual-baseline.md`（带 SSIM 表）

**M-R0 不出截图不进入 M-R1**。
