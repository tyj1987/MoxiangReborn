# 可玩性现状（以代码为准）

> 状态日期：2026-08-22。本文件是日常开发的真理来源。  
> `ROADMAP.md` 只保留目标与宪法；旧 CHANGELOG / restoration-plan 的 GREEN 声明作历史，不证明玩家现在能玩。  
> 禁止用「基本完成」「看着像」「单测全绿」代替本表。

## 0. 玩家现在看到的

| 玩家操作 | 实际 |
|---|---|
| 打开客户端，登录页能打字、能点 Login | **通**。这是 `g_loginUi` 手写 overlay + `login.dds`，不是 `MT_LOGINDLG.bin` |
| 登录后看见选角槽位 / 创建按钮并点进去 | **不通**（P0） |
| 登录后看见建角界面并提交角色 | **不通**（P0）。`--auto-create` 协议能建，人类点不到 |
| 进图后看见主 HUD、快捷栏、小地图 | **不可靠** |
| 进图后看见自己 / NPC / 野外怪 | **不可靠**。Map 12 无野外怪是资源事实 |
| WASD / 鼠标在进图后有可见反馈 | **不可靠**。输入函数在，缺视觉 + 不可见 dialog 可能吞点击 |

自动化 `mxh_client_e2e` 与 `mxh_client --auto-login --auto-create --exit-after-gamein` **不能**当作上表通关。

## 1. 拓扑（已核实，不要再猜）

| 角色 | 地址 | 说明 |
|---|---|---|
| 客户端 | 本机 `192.168.2.30` + Arc B580 | `C:\moxiang\modern\build\tools\MoxianClient\mxh_client.exe` |
| PVE | `192.168.2.200` | 宿主机 |
| 三服 | VM 100 `192.168.2.107` | Login 16001 / Agent 17001 / Map 18001 |
| SQL | LXC 102 `192.168.2.203:1433` | `sa` / 见 `modern/scratch/2026-08-22-live-commercial/sql.env`；库 `Moxiang` |

本机 Clash TUN 会劫持未绑定套接字。客户端与探测必须 `MXH_BIND_IP=192.168.2.30`。

人类验收启动（不要加 `--auto-create` / `--exit-after-gamein`）：

```
mxh_client --login-host 192.168.2.107 --login-port 16001 --resource-root C:\moxiang\modern\data\PlayDH
```

## 2. 三列门禁

每一格必须有命令 + 产物（日志/截图/DB 行）或明确 FAIL。空格 = 未过。

| 场景 | 协议 | 视觉（能看见该有的东西） | 输入（人类能点/能键） |
|---|---|---|---|
| 登录 overlay | PASS（LoginAck） | PASS（手写页） | PASS（仅这页） |
| 选角 CharSelectDlg | PASS（ListAck / SelectSyn，自动化） | 2026-08-22 无 `--auto-create` 停留：右侧槽位面板可见，槽上有 `VisualGui`。背景 `login.dds` 仍上下颠倒；无 3D 选角模型 | 点击路由单测 PASS；真人点进入尚未截到 |
| 建角 CharMakeNewDlg | PASS（MakeSyn，自动化） | 未做无 auto 停留截图 | 点击路由单测 PASS |
| 进图地形 Map 12 | PASS（GameInAck） | 地形有路径 | — |
| 进图 HUD | 脚本已 load | 不可靠 | 不可靠 |
| 本机玩家网格 | GameInAck 有外观字段 | 失败静默 | — |
| NPC | NpcAdd 已广播 | CHX 失败则空 | 点击可能被 HUD 吞 |
| 野外怪 Map 12 | — | **资源 stub**：`Monster_12.bin` 14 字节，0 spawn | 不要在这张图验收打怪 |

## 3. 代码锚点（修 bug 从这里进）

| 缺口 | 文件 |
|---|---|
| 手写登录，不是原版 dialog | `modern/tools/MoxianClient/main.cpp` `g_loginUi` |
| 启动 `LoadAll` 157 个 bin 进无用 `g_wm`，打爆 VRAM | 同上，`cDialogLoader::LoadAll` |
| 同一 `1.tif` 反复 `CreateSpriteObject` | `modern/src/ui/cDialogLoader.cpp` `loadImageForImageIdx` |
| 选角/建角 UI 树 | `CCharSelectState.cpp` / `CCharMake.cpp` + `ClientUiRuntime` |
| `RenderAll` / 点击要求 `cDialog::isActive()` | `cWindowManager.cpp` / `cDialog.hpp` `m_bActive = false` |
| `--auto-create` 才自动选角/建角 | `main.cpp` `set_auto_select_for_test` |
| 进图输入 | `CInGameState::OnKeyEvent` / `OnMouseButton`；点击 `consumed` 会挡住世界 |
| 3D 实体 | `modern/src/render/entity_scene.cpp`；失败只 `MLOG_WARN` |
| GameLoading 空 stub | `GameStateStubs.cpp`；进图靠 `main.cpp` 特判 |

## 4. 当前里程碑（只按这个排期）

1. **P0 选角/建角能看见、能点** — 无 auto 登录，截图里有按钮，鼠标能点创建/进入
2. **P1 进图 HUD + 键鼠有反馈** — 主条可见；WASD 能看出位置/相机变化；点空地不吞后续点击
3. **P2 人物与 NPC 可见** — 同一帧能指出玩家和至少 1 个 NPC
4. **P3 打怪图可见怪物** — 用非 stub 的 `Monster_*.bin`（例如已恢复的 Map 10），不改 Map 12 资源
5. **P4 原版登录 dialog + 视觉 1:1** — 替换 `g_loginUi`；SSIM 放到这之后

约束不变：不改 `[CC]Header`、不改 PlayDH 字节、不改数值公式、不改 HSEL 签名。
