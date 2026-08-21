# 可玩性现状（以代码为准）

> 状态日期：2026-08-22。本文件是日常开发的真理来源。  
> `ROADMAP.md` 只保留目标与宪法；旧 CHANGELOG / restoration-plan 的 GREEN 声明作历史，不证明玩家现在能玩。  
> 禁止用「基本完成」「看着像」「单测全绿」代替本表。

## 0. 玩家现在看到的

| 玩家操作 | 实际 |
|---|---|
| 打开客户端，登录页能打字、能点 Login | **通**。这是 `g_loginUi` 手写 overlay + `login.dds`，不是 `MT_LOGINDLG.bin` |
| 登录后看见选角槽位 / 创建按钮并点进去 | **通**。槽位+Enter/Create 真实 hitbox 单测；无 auto 下 VK_DOWN+Enter 两次进 GameIn。背景 `login.dds` 仍上下颠倒 |
| 登录后看见建角界面并提交角色 | **单测通**（`CharMakeNewDlg.bin` 提交/取消/名字）。已有角色所以无无-auto 建角停留 |
| 进图后默认 HUD，点空地不吞世界点击 | **通**。四默认 HUD 根带非空 `cImage`；I 键开关背包；空隙 `consumed=false`。无 auto GameIn 帧有血条/小地图条 |
| 进图后看见自己 / NPC / 野外怪 | **计数+占位通**。缺 CHX 写入 `placeholders()`（radius>0）。Map 12 仍无野外怪。网格 1:1 未证 |
| WASD | **`step_movement` 单测通** |

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
| 选角 CharSelectDlg | PASS | 两次无 auto：charselect 1323220 vs connect 1229076 | 槽位然后 Enter/Create hitbox；默认无 auto-select |
| 建角 CharMakeNewDlg | PASS | 无无-auto 停留（已有角色） | Submit/Cancel/名字 hitbox |
| 进图地形+HUD | PASS | 两次无 auto GameIn：1508444/1508435 vs 选角 1323220，含血条/右上条 | I 键背包；空隙不 consumed |
| 本机玩家/NPC 网格 | 协议有 | `placeholders()` 带 radius>0，不是只计数 | — |
| 野外怪 Map 12 | — | **资源 stub**：`Monster_12.bin` 14 字节，0 spawn | 不要在这张图验收打怪 |

## 3. 代码锚点（修 bug 从这里进）

| 缺口 | 文件 |
|---|---|
| 手写登录，不是原版 dialog | `modern/tools/MoxianClient/main.cpp` `g_loginUi` |
| 启动全量 `LoadAll`（已停） | `MoxianClient/main.cpp` 改为按状态 on-demand |
| 同一 `1.tif` 复用 sprite | `cDialogLoader.cpp` `g_sprite_by_path` |
| 选角/建角激活 + 点击 | `activateAllLoadedDialogs`；hitbox 测试在 `client_ui_runtime_test.cpp` |
| 进图默认 HUD | `CInGameState` `applyActiveSet(MI_MAINDLG/QI_QUICKDLG/MNM_DIALOG/CG_GUAGEDLG)` |
| 空隙点击穿透 | `DefaultHudActiveAndMissClickIsNotConsumed` |
| 3D 实体失败 | `EntityScene::failedModelCount` / `placeholderCount` |
| GameLoading 空 stub | `GameStateStubs.cpp`；进图仍靠 `main.cpp` 特判 |
| 登录仍是手写 overlay | `g_loginUi`（P4） |

## 4. 当前里程碑（只按这个排期）

1. **P0 选角/建角能看见、能点** — **持**（含槽位然后 Enter；无 auto 两次进 GameIn）
2. **P1 进图默认 HUD + 空隙不吞点击** — **持**（单测 + 无 auto GameIn 帧）
3. **P2 移动步进 + 可绘制占位** — **持**（`step_movement` + `placeholders()`）；CHX 网格 1:1 仍后置
4. **P3 打怪图可见怪物** — 用非 stub 的 `Monster_*.bin`，不改 Map 12
5. **P4 原版登录 dialog + 视觉 1:1** — 替换 `g_loginUi`；SSIM 后置

约束不变：不改 `[CC]Header`、不改 PlayDH 字节、不改数值公式、不改 HSEL 签名。
