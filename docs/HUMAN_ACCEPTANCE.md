# 人工验收手册（Human Acceptance Manual）

> **状态**：vertical slice 阶段。G0-G3 PASS, G4-G9 Partial, G10-G11 Not started。
> 完整跑通下面 6 步相当于在 G4-G9 留 8 张以上 F12 帧 + 1 份 log marker 摘要，
> 不代表项目发布就绪（E5 真实 legacy visual 对比仍缺）。

## 0. 前提

| 条件 | 路径 |
|---|---|
| 客户端构建存在 | `modern\build\tools\MoxianClient\mxh_client.exe` |
| 启动器构建存在 | `modern\build\tools\MoxianLauncher\MoxianLauncher.exe` |
| PVE 三服与数据库就绪 | `192.168.2.244` 的 Login `16001`、Agent `17001`、Map `18001`；由 PVE 管理 |
| 现代 资源 PlayDH 完整 | `modern\data\PlayDH\` (1.56 GB, 4,971 文件) |

客户端或启动器缺失时先构建本机目标；PVE 服务状态须在 PVE 核验。PVE LoginServer 的 `AdvertisedAgentAddress` 必须设为客户端可达的 `192.168.2.244`，否则 LoginAck 会通告默认 loopback 地址。

## 1. 一键入口（最简单路径）

```powershell
pwsh -File scripts\run-human-acceptance.ps1 -PlanOnly
pwsh -File scripts\run-human-acceptance.ps1
```

默认行为：
- Login = `192.168.2.244:16001`；Agent `17001`；Map `18001`
- 资源 profile = `playdh-current`
- 输出到 `modern\out\runs\human\<runId>\evidence\`
- 启动器弹出 → 人工操作 → 按 Enter 收尾；本机不启动三服或数据库
- **最少需要 8 张 F12 帧**，否则脚本报"insufficient evidence"并退出 1

## 2. 8 个强制 checkpoint（按这个顺序按 F12）

1. **login** — 启动器把客户端拉起来后，登录界面渲染完
2. **display-transition** — 提交账号密码后，800×600 切到 1024×768 完成
3. **char-select-or-create** — 选角或建角完成（如果老角色直达，老账号也应该停在选角页）
4. **loading** — 选完角后到进图前 loading 进度条
5. **map10** — 进 Map10 后世界渲染稳定（地形 + 怪物可见）
6. **combat-and-loot** — 攻击一只怪物，看到伤害数字 + 拾取掉落
7. **map-change** — 用菜单或 NPC 切到 Map12（如果可达）
8. **relog** — Alt+F4 退到登录页，再登一次，角色还在 Map10

每步按一次 F12，evidence 目录会出现 `state-<n>.tga` 之类的帧文件。**少 1 个就过不了 8 帧门槛**。

## 3. 8 个 checkpoint 各自的"OK 标志"

肉眼检查项（不需要脚本验证）：

| Checkpoint | 看到什么算 OK |
|---|---|
| login | 登录 UI 正常，账号/密码输入框可点，按"确认"有反应 |
| display-transition | 800×600 切到 1024×768，画面没崩/没黑 |
| char-select-or-create | 至少 1 个角色格子有 3D 模型，建角流程可达 |
| loading | 进度条动，没卡在 0% 或 100% |
| map10 | 地形 + 怪物可见，玩家模型在中央，FPS 不为 0 |
| combat-and-loot | 点怪物有数字飘，怪物死了有掉落物 |
| map-change | 切图过程不停顿，新地图渲染出来 |
| relog | 登出后能重进，角色还停在 Map10 同位置 |

## 4. 收尾

回到启动 `run-human-acceptance.ps1` 的 PowerShell 窗口，按 Enter：
- 脚本只关闭本次识别出的启动器和客户端进程，PVE 三服继续由 PVE 管理
- 写 `run.json` 总结（含每个 F12 帧名、客户端 PIDs、端点与退出码）
- exit 0 仅表示证据采集流程完成且达到最少帧数，`run.json` 写入 `acceptanceStatus=evidence_collected_awaiting_review`；8 个 GUI checkpoint、音频与持久化结果仍须人工逐项审阅。exit 1 表示采集流程未完成，`acceptanceStatus=collection_incomplete`。

## 5. 工具脚本（可单独用）

| 脚本 | 作用 | 何时用 |
|---|---|---|
| `scripts\register-test-account.ps1` | 在跑服务端数据库里建 1 个测试账号 | 想跳过注册直接登录时 |
| `scripts\capture-state-frames.ps1` | 用 auto-login 跑完整 client 流，抓 state-*.tga | 验证客户端基础设施 OK（无人工） |
| `scripts\collect-server-logs.ps1` | 收 `deploy\runtime\modern\logs\` + marker 摘要 | 看服务端协议流转 |
| `scripts\run-human-acceptance.ps1` | 仅启动本机 DX11 客户端并记录人工证据 | 每次完整人工验收 |

## 6. 历史本机工具链烟测（不开 launcher）

```powershell
pwsh -File modern\scratch\2026-08-31-human-acceptance\toolchain-smoke.ps1
```

输出 `TOOLCHAIN_SMOKE_PASS` 即：start server → register → collect logs → stop 全通。
**不验证 client 端**，只验证旧本机服务端/数据库基础设施；不属于本轮 PVE 人工验收路径。

## 7. 已知会失败的（按 plan 接受为"留 G10/G11"）

- 3D 角色预览/视频不完整 → G5 仍 Partial
- 多显示器 / DPI 切换未完整覆盖 → G4 仍 Partial
- 真实 legacy E5 视觉对比缺（拿老客户端跑同 Map10 对比） → 全部 G4-G9 仍 Partial
- Map101 等已知 blocked map 没修 → G10 仍 Not started
- 24h soak + fault injection 没跑 → G11 仍 Not started

任何上面没列的失败 → 写 `docs\KNOWN_BUGS.md` 一条新 bug，不要带病跑 release。
