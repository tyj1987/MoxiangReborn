# AGENTS.md — Moxian-Reborn AI 协作指南

> 本文件指导 AI Agent（Claude、GPT、Cursor、Mavis 等）在这个项目上工作。请同时阅读 `MODERNIZATION_PLAN.md` 了解整体路线。

## 项目一句话

**墨香（Moxian / DarkStory）**：2003-2010 年韩国 2D MMORPG，使用自研 4Dyuchi 引擎（DX8 + 自研 3D + 自研网络）。目标是**保持游戏内容 1:1 复刻**，把代码迁移到现代软硬件环境。

## 关键约束（绝对不能破坏）

1. **资源格式兼容**：`.bin`（XOR 加密）、`.pak`（4DyuchiFileStorage）、`.bmhm`、`.ttb`、`.chl/.chx/.chr/.mon`、`.bsad` 必须**保持二进制兼容**
2. **网络协议兼容**：`[CC]Header/Protocol.h` 的 96 类 Category + `CommonStruct.h` 的网络包结构**不能改字段**
3. **玩法/数值 1:1**：经验曲线、伤害公式、爆率、Boss 刷新、商城等**不能动**
4. **HSEL/HackShield/nProtect 接口**：可以替换实现，但**接口签名要保持**（便于回退）

## 核心目录速查

| 路径 | 用途 | 关键文件 |
|------|------|---------|
| `墨香【源码】\[Client]MH/` | 客户端代码 | `MHClient.cpp`, `GameIn.cpp`, `ItemManager.cpp` |
| `墨香【源码】\[Server]Agent/` | 代理服务端 | `AgentNetworkMsgParser.cpp` |
| `墨香【源码】\[Server]Distribute/` | 登录服 | `DistributeNetworkMsgParser.cpp` |
| `墨香【源码】\[Server]Map/` | 地图服（最复杂） | `MapServer.cpp`, `Player.cpp`, `AISystem.cpp` |
| `墨香【源码】\[Server]MurimNet/` | PvP | `MurimNetSystem.cpp` |
| `墨香【源码】\[CC]Header/` | 共享协议与结构 | **`Protocol.h`** `CommonStruct.h` `CommonGameDefine.h` |
| `墨香【源码】\[CC]Skill/` | 技能系统（双版本） | `SkillManager_server.cpp`, `SkillObject*` |
| `墨香【源码】\[CC]BattleSystem/` | 战斗系统 | `BattleFactory_Default.cpp` |
| `墨香【源码】\[CC]Quest/` | 任务系统 | `QuestManager.cpp`, `QuestExecute_*` |
| `墨香【源码】\[CC]Ability/` | 能力/状态 | `AbilityManager.cpp` |
| `墨香【源码】\[Lib]BaseNetwork/` | 网络客户端封装 | `BaseNetwork.cpp` |
| `墨香【源码】\[Lib]DBThread/` | 数据库线程池 | `DB.cpp` |
| `墨香【源码】\[Lib]HSEL/` | 加密库 | `HSEL_STREAM.cpp` |
| `墨香【源码】\[Lib]YHLibrary/` | 工具库 | – |
| `墨香【源码】\[Lib]dx81/` | DirectX 8.1 SDK | `d3d8.h`, `d3dx8.h` |
| `墨香【源码】\[Tool]PackingMan/` | BIN 打包工具 | `MHFileEx.cpp`, `PackingTool.cpp` |
| `墨香【源码】\[Tool]Regen/` | 怪物重生编辑器 | `RegenTool.cpp` |
| `墨香【源码】\[Tool]AutoPatchToolWin32/` | 自动更新器 | `AutoPatchToolWin32.cpp` |
| `墨香【源码】\[Tool]DS_RMTool/` | GM 工具 | `RMNetwork.cpp` |
| `墨香【源码】\4DyuchiNET_Latest/` | 网络底层（IOCP） | `connection.cpp`, `I4DyuchiNET.def` |
| `墨香【源码】\4DYUCHIGX_RENDER/` | DX8 渲染 DLL | `CoD3DDevice.cpp` |
| `墨香【源码】\4DyuchiGXGeometry/` | 几何 DLL | `CoGeometry.cpp` |
| `墨香【源码】\4DyuchiFileStorage/` | PAK 格式 DLL | `CoStorage.cpp`, `PackFile.cpp` |
| `墨香【源码】\4DyuchiFilePack/` | PAK 打包工具 | `4DyuchiFilePack.cpp` |
| `墨香【源码】\4DyuchiGXMapEditor/` | 地图编辑器 | `TileSet.cpp` |
| `墨香【源码】\SWorking/` | 已编译服务端目录 | 完整运行实例 |
| `墨香【源码】\cworking/` | 已编译客户端目录 | `MHClient-Connect.exe` |
| `墨香【源码配套资源】\PlayDH\` | 完整游戏资源 | `Resource/`, `Image/`, `Ini/`, `Data/` |
| `墨香【客户端+服务端+工具】` | 已部署客户端/服务端/工具 | `客户端.rar`, `服务端.rar`, `工具.rar`, `DB.zip` |
| `墨香【教程】` | 4 篇中文教程 | `配置服务端.docx` 等 |

## 工作时的常见陷阱

1. **编译时 `LOG` 宏冲突**：游戏自定义 `void LOG(...)` 与系统 `msplog.h` 的 LOG 函数宏冲突。**不要**直接 include Windows 头后还用 LOG；要么 `#undef LOG`，要么把游戏 LOG 改名 `MLOG`。
2. **`wsock32.lib` 未链接**：会报 `LNK2019: closesocket@4 / socket@12` 等 9 处错误。需要在项目链接器加 `WS2_32.lib`。
3. **`d3dx8.lib`** 不存在：DX8.1 SDK 中叫 `d3dx8.lib` 但实际不同版本路径不同。建议在 vcxproj 里直接用 `<PropertyGroup Label="Dx8SdkPaths">` 形式给出。
4. **字符编码**：客户端大量 EUC-KR/CN 注释，UTF-8 编辑器可能乱码。保持原编码别动。
5. **多语言 ifdef**：`_KOR_LOCAL_ / _TW_LOCAL_ / _CHINA_LOCAL_ / _JP_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_ / _NDSC_LOCAL_ / TAIWAN_LOCAL` 散落代码各处。**别急着合并**，先确保每种宏都能编译。
6. **`#pragma pack(push,1)`**：所有网络包结构强制 1 字节对齐——**绝对不要改**，否则协议崩溃。
7. **MHVerInfo.ver**：客户端启动时读这个文件读 Distribute 地址 + 版本号。改服务端要先改这个。
8. **PowerShell 方括号目录名当通配符**：本仓库目录名大量用 `[Lib]` / `[CC]` / `[Server]` 方括号，PowerShell 会把 `[...]` 当作通配符字符类。`Test-Path`/`Get-Item`/`Get-ChildItem` 直接传这种路径会误判（返回 False / 长度 0 / 找不到文件）。**必须**加 `-LiteralPath`（如 `Get-Item -LiteralPath "墨香【源码】\[Lib]BaseNetwork\...\BaseNetwork.dll"`）。`Select-String` / Python `os.path` 不受影响。
9. **git 跟踪范围很窄**：仓库只跟踪约 439 个文件——`modern/`、`docs/`、根级文件、以及每个已迁移遗留目录里的单个 `CMakeLists.txt`。庞大的 `墨香【源码】/...` 遗留源码树是「未跟踪但也没被 .gitignore」的既定状态，`git status` 里成片的 `??` 是正常的，不是本次改动泄漏。验证「git 是否干净」时看的是**暂存/提交**是否有意外新增，而不是这些常驻未跟踪文件。另注意：`.mavis/`、`plan_launch.txt`、`roster.txt` 未被 gitignore，`git add .` 会误收。

## 推荐工作流（任何修改前）

1. **读相关头文件**：先 grep 找到所有定义/调用点
2. **理解原接口**：用 Read 工具看 1-2 个调用点
3. **写测试**：在 `modern/tests/` 里加单元测试
4. **实现新模块**：用现代 C++ 重写
5. **行为对比**：与原实现对比输出字节级一致
6. **回归**：跑原客户端/服务端，验证 1:1 行为

## 阶段路线

详细见 `MODERNIZATION_PLAN.md` 第 12 阶段路线图。当前应该处于 **Phase 0-1**：

- **Phase 0**：准备与可编译化（基础工程）
- **Phase 1**：资源兼容层（最关键，必须最先做）

## 编码风格

- 遵循项目原有风格（匈牙利命名法：前缀 `m_` 成员、`g_` 全局、`p` 指针）
- C++17 起步；新代码用智能指针、范围 for、`auto`、`std::optional`、`std::string_view`
- 旧代码原样保留（除非是 bug 或必须改）
- 注释：中文/英文都可以，但**避免韩文/日文**（搜索不出来）

## 交付物清单

每次完成工作，**更新 `MODERNIZATION_PLAN.md` 第 5 节交付物**，勾选已完成项。

## 反馈 / Bug

发现原代码 bug 时，**记录但不擅自修改**。在 `docs/KNOWN_BUGS.md` 写明：
- 文件:行号
- 原代码行为
- 期望行为
- 触发条件

除非用户明确指示修复。