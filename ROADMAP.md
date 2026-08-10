# Moxian-Reborn 路线图：1:1 完美复现

> 状态日期：2026-08-09。完成历史与测试累计见 [docs/CHANGELOG.md](docs/CHANGELOG.md)，活动缺陷见 [docs/KNOWN_BUGS.md](docs/KNOWN_BUGS.md)。本文件只记录目标、当前事实和下一里程碑，不追加 session 日志。

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
| **M3 进展 (2026-08-10)** | T3 五段 modern 5/5 diff=0；现代金色锁像锁定在 modern/tests/fixtures/sbs_captures_modern/ 与 SideBySideModernGolden.* 单测中 | 副作用顺序 / 数值 / DB 这 3 项需 modern manager (caster / npc_shop / quest_manager) 落地后才能达到 diff=0；跨实现证据仍需 legacy SWorking 运行环境 | **M3 modern 闭环完成** |

T1、T2、T3 全部通过并完成商业 RC 打包，才算当前目标完成。legacy 网络互通只作参考，不是发布门禁。

## 2. 当前状态

| 领域 | modern 自测 | 1:1 内容/体验验收 | 结论 |
|---|---|---|---|
| T1 资源 | 303 条 SHA-256 锁定；268 个真实解析入口 | 基准来自原始 PlayDH/deploy | **完成** |
| T2 协议 | 85 个 wire golden、96 类 dispatcher 覆盖、1001 包 replay 稳定；modern 客户端与三服务端五步 E2E 通过 | 不要求新旧互通；后续以真实玩法闭环覆盖 modern 消息路径 | **RC 基础闭环完成** |
| 客户端/服务端运行时 | Login/Agent/Map 三进程和五步无头客户端 E2E 可运行；GUI 客户端已修复端口配置、WM_PAINT 饥饿和状态回调，可登录并取得角色列表 | GUI 仍使用占位 sprite/file storage，空账号缺少建角交互，尚未显示真实地图与完整 UI | **协议闭环完成，图形客户端未完成** |
| UI | 165 个 legacy dialog 头均已有 modern port；198 个 UI 头、3314 个测试；Quest/Deal/仓库已有真实 service wiring | 9 个业务 Tier-3 dialog（历史重点为 Quest/Deal 类）仍需完整 service 接线和截图对比；当前 3 个核心业务 dialog 有 service 行为验收 | **port 完成，业务集成待验收** |
| 玩法/数值 | D1-D6 数据面与 side-effect runtime 已形成广泛单测覆盖 | 五段核心玩法 side-by-side 尚未全部 diff=0 | **部分完成** |
| DX11 渲染 | headless 3 帧可自然退出；像素门禁锁定 grid、cube、checker 纹理与深度遮挡 | 仍需与原版登录/空场景截图对比 | **modern 闭环完成，legacy 视觉待验收** |
| HSEL | 软件流、ABI、三进程加密 E2E 已通过 | 商业 RC 按用户决策忽略实体硬件狗，仅保留接口与 wire 兼容 | **RC 范围完成** |
| MSSQL | ODBC 18/17 自动选择、LocalDB schema 初始化、客户端与三服务端五步 E2E 已通过 | 尚需干净机部署和生产配置演练；legacy `.bak` 非强制 | **本机闭环完成，部署待验收** |

当前 CMake 发现基线：**11,749 tests**；11,748 项基线全量 CTest 退出码为 0，新增 GUI 状态回调回归测试也已通过。该数字用于防止测试静默丢失，不代表 T3 已完成。

## 3. 当前里程碑

### M1：R-9 legacy 视觉验收

modern 渲染闭环已由 `RenderDemo.HeadlessFrameAcceptance` 固化：headless 自然退出并验证 grid、cube、checker 纹理与深度遮挡。当前仅剩外部对照：

- 与原版登录界面和空场景截图对比，记录可接受差异。

### M2：C-Tier-3 UI 集成

- 从 legacy 对照表确认 9 个业务 dialog（历史候选包括 QuestDialog、QuestTotalDialog、DealDialog），再接入对应 quest/trade/inventory service；不将无关面板接线计入完成数。
- 每个 dialog 保持至少一个行为断言，并完成原版/modern 截图对照。

### M3：T3 五段行为对照

- 固定登录进图、战斗/任务、商城/物品、PK 五个可重放场景。
- 比较副作用顺序、网络包、数据库变化、数值与 UI 状态；差异必须归零。

### M4：部署与商业 RC 验收

- 在干净 Windows 环境完成 ODBC、数据库、三服务端、客户端和工具的一键安装/启动/卸载演练。
- 完成资源全量覆盖清单、音乐/音效播放、地图巡检、长时间稳定性、崩溃诊断和 RC 校验和。
- legacy SWorking 抓包、legacy `.bak` 与实体 HSEL 狗均不纳入商业 RC 门禁。

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
