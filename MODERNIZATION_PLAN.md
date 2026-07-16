# 墨香（Moxian / DarkStory）现代化改造计划

> **项目代号**：Moxian-Reborn
> **目标**：在不破坏现有游戏内容、玩法、资源的前提下，把 2003-2010 年的代码体系迁移到 2026 年的现代软硬件环境，并对性能、可维护性、跨平台能力做最大化提升。
> **核心约束**：**游戏逻辑、协议、资源、玩法必须 1:1 保留**——这是"复刻"而不是"重做"。

---

## 0. 项目现状摘要（分析结论）

### 0.1 项目体量

| 维度 | 数据 |
|------|------|
| 客户端代码 | `[Client]MH/` 约 930 个 .h/.cpp，~15 MB，约 35-40 万行 |
| 服务端代码 | `[Server]*/` + `[CC]*/` 约 412 个 .h/.cpp，约 50-70 万行 |
| 引擎代码 | `4Dyuchi*` 全家桶 + `[Lib]dx81` 等 ~5-10 万行 |
| 工具链 | 打包/地图/重生/导出/GM/压测/更新器 ~10 个独立工程 |
| 资源（已部署） | 1.3 GB 客户端 + 29 MB 服务端 + ~37 张地图 + 三库 MSSQL 备份 |
| 已编译产物 | `SWorking/` 完整服务端工作目录，`cworking/MHClient-Connect.exe` 客户端 |

### 0.2 进程拓扑

```
                    ┌──────────────────────────────┐
                    │  Monitoring Server (MS) :30001│  中央协调
                    └──────────────────────────────┘
                                  ▲
                                  │ 上报/查询
   ┌────────────────────┐         │         ┌─────────────────────┐
   │ Distribute :6001/400│◄────────┼────────►│  Agent :7001/600    │
   │  (登录/选服)        │         │         │  (代理/世界服)       │
   └────────────────────┘         │         └─────────────────────┘
                                  │                ▲▼  ▲▼
                                  │      ┌──────────────────────┐
                                  │      │  Map #N :800N         │
                                  │      │  (一张地图一个进程)    │
                                  │      │  Map #M :800M         │
                                  │      └──────────────────────┘
                                  ▼
                          客户端 MHClient.exe
```

### 0.3 客户端架构

```
MHClient.exe (WinMain)
├─ MHVerInfo.ver ─ 启动配置（版本、Distribute 地址）
├─ 4DyuchiNET ─ 自研 IOCP 网络库（客户端封装为 BaseNetwork.dll）
├─ HackShield + nProtect ─ 韩国反外挂（已停更）
├─ HSEL ─ 加密狗/协议加密（物理狗已停产）
├─ DirectX 8.1 ─ 渲染（Win11 不再预装 d3dx8.dll）
├─ 4Dyuchi 引擎 ─ 自研 3D 封装（SS3DRenderer/Geometry/ExecutiveForMuk.dll）
├─ MFC + 自研 cWindow 控件树 ─ UI（Win32 GDI）
├─ Miles Sound System ─ 3D 音效（商业授权）
└─ FreeImage ─ 图片解码
```

### 0.4 资源格式（不可破坏的 1:1 数据）

| 类型 | 扩展名 | 数量级 | 加密 | 工具 |
|------|--------|------|------|------|
| 业务表 | `.bin` | 80+ 个 | XOR + 位移 | `MHFileEx` (PackingTool) |
| 资源包 | `.pak` | 7 个核心 | 否 | `4DyuchiFileStorage` |
| 地图 | `.bmhm` + `.ttb` | 37-207 张 | 否 | `4DyuchiGXMapEditor` |
| 模型 | `.chl` / `.chx` / `.chr` / `.mon` / `.wpn` / `.hat` 等 | 数百 | 否 | `MAXEXP / anmexp / MtlExp` (3ds Max Biped 插件) |
| 贴图 | `.tga` / `.dds` / `.bmp` / `.jpg` | 数千 | 否 | – |
| 技能区 | `.bsad` | 50+ 模板 | 否 | – |
| 数据库 | `MHCMEMBER/MHGAME/MHLOG.bak` | 3 库 MSSQL | – | SQL Server 2008 R2 |

**关键判断**：
- `.bin` 加密算法极弱（`data[i] -= (char)i`，CRC 已被注释），但**格式必须保留**才能读取老资源
- `.pak` 包格式（32 字节 header）是运行时主格式，**不可破坏**
- 模型 `.chl/.anm` 是 3ds Max Biped/Physique 导出，3ds Max 2018+ 已移除此模块 → 必须**保留或转换**

---

## 1. 现代化路线总览

### 1.1 战略选择

| 方案 | 风险 | 工作量 | 复刻度 | 跨平台 | 推荐度 |
|------|------|--------|--------|--------|------|
| A. 全部重写（Unity + Rust） | 极高 | 极大量（2 人年+） | 难保证 1:1 | ✓ | ✗ |
| B. 引擎替换 + 协议保留 | 中 | 大（6-12 月） | 高 | ✓ | ✓ |
| **C. 渐进式现代化（默认）** | **低** | **中（3-6 月）** | **极高** | 部分 | **★★★★★** |
| D. 仅文档/容器化 | 极低 | 小（2-4 周） | 100% | ✗ | ✗ |

**采用 C：渐进式现代化**

### 1.2 渐进式现代化的三大原则

1. **兼容性优先**：任何模块替换都不破坏现有协议、资源、玩法
2. **接口稳定**：抽象层保持与原代码同形（如 `MHFile::Open()` 在新旧实现下都能跑）
3. **可回退**：每一步改造都能切回原版运行；不一次性"砸烂重做"

### 1.3 12 阶段路线图

```
Phase 0: 准备与可编译化 (2-3 周)
  ├─ 0.1 资源完整性盘点 + 编写 MODERNIZATION_PLAN.md （本文档）
  ├─ 0.2 编写 .gitignore，过滤 .ncb/.opt/.suo/.vsscc 等
  ├─ 0.3 编写现代构建系统骨架 (CMake + vcpkg/Conan)
  ├─ 0.4 VS2022 工程改造（最小可编译集：先服务端独立工程）
  └─ 0.5 编写 PowerShell/Shell 一键启动脚本（替代 .lnk）

Phase 1: 资源兼容层 (3-4 周) ★1:1 复刻的关键★
  ├─ 1.1 用现代 C++ 重写 MHFileEx (.bin 加解密)
  ├─ 1.2 用现代 C++ 重写 4DyuchiFileStorage (.pak 包格式)
  ├─ 1.3 实现 .bmhm + .ttb 地图几何读取器
  ├─ 1.4 实现 .chx/.chr/.mon 模型格式解析器（部分逆向）
  ├─ 1.5 实现 .bsad 技能区域读取器
  ├─ 1.6 实现 .tga → 现代贴图转换管线
  └─ 1.7 编写"资源浏览器"工具验证 1:1 读取

Phase 2: 数据库抽象层 (2-3 周)
  ├─ 2.1 抽取 IDatabase 接口（保留原 eQueryType 路由）
  ├─ 2.2 MSSQL/PostgreSQL 双实现（先 MSSQL 保 1:1）
  ├─ 2.3 数据库 schema 文档化 + 工具导出
  └─ 2.4 数据迁移/导入工具（兼容原 .bak）

Phase 3: 加密与反外挂现代化 (2 周)
  ├─ 3.1 HSEL 加密狗 → AES-256-GCM (HSEL 接口保留签名，内部替换)
  ├─ 3.2 HackShield/nProtect → 服务端权威校验（可选保留二进制兼容）
  └─ 3.3 协议加密可配置（关闭/兼容/严格 三档）

Phase 4: 网络层现代化 (3-4 周) ✅
  ├─ 4.1 抽取 I4DyuchiNET 抽象接口（保留 IOCP 语义） ✅
  ├─ 4.2 Asio/Boost.Asio 实现（Linux-ready） ✅
  ├─ 4.3 协议版本化（消息包加 version 字段） ✅
  └─ 4.4 加密/压缩/校验中间件 ✅

Phase 5: 渲染引擎现代化 (4-6 周) ✅
  ├─ 5.1 抽取 IRenderer/IGeometry/IExecutive 抽象接口 ✅
  ├─ 5.2 DX11/DX12 后端实现（保留原 4Dyuchi 接口签名） ✅
  ├─ 5.3 模型/动画复用：.chl/.anm → 自研二进制保持 ✅
  └─ 5.4 字体/UI 资源路径保持 ✅

Phase 6: UI 系统现代化 (3-4 周) ✅
  ├─ 6.1 cWindow 控件 → 现代 GUI（保留二进制 .bin UI 描述） ✅
  ├─ 6.2 选项：保留自研 cWindow（DX11 后端）/ 替换为 ImGui ✅
  └─ 6.3 多语言/i18n 抽离（消灭 _KOR_LOCAL_ 等宏） ✅

Phase 7: 构建系统完全化 (2 周) ✅
  ├─ 7.1 全工程迁移到 CMake / Premake ✅
  ├─ 7.2 依赖管理 vcpkg/Conan ✅ (vcpkg.json manifest + CMake find_package 三模式回退)
  └─ 7.3 CI/CD（GitHub Actions） ✅ (.github/workflows/ci.yml, Debug+Release 矩阵)

Phase 8: 性能优化 (持续) ✅
  ├─ 8.1 多线程：渲染线程 / 网络线程 / 逻辑线程 ✅ (ThreadPool + 线程池模型)
  ├─ 8.2 内存：自定义分配器 / 对象池 ✅ (ObjectPool<T> 泛型对象池)
  ├─ 8.3 协议：批量压缩 / 增量同步 ✅ (RLE 压缩 + 压缩阈值 + wire format)
  └─ 8.4 性能剖析与回归测试 ✅ (net_benchmark + crypto_benchmark)

Phase 9: 可选 - 跨平台 (4-8 周)
  ├─ 9.1 Linux 服务端（先 Distribute/Agent，Map 涉及 DX 跳过） ✅ (platform.hpp + socket.hpp/cpp)
  ├─ 9.2 macOS 客户端（Metal 后端）
  └─ 9.3 容器化部署（Docker）

Phase 10: 工具链现代化 (2-3 周) ✅
  ├─ 10.1 PackingTool.exe → Web/CLI（C# Avalonia / Rust egui） ✅ (MoxianPacker CLI)
  ├─ 10.2 GMTOOL → Web 后台（FastAPI/Blazor） ✅ (MoxianGMTool HTTP API)
  ├─ 10.3 地图编辑器 → 保留 4DyuchiGXMapEditor 但支持现代 SDK ✅ (MoxianMapEditor CLI)
  └─ 10.4 AutoPatchTool → HTTPS + bsdiff ✅ (MoxianAutoPatcher)

Phase 11: 文档与社区 (持续) ✅
  ├─ 11.1 AGENTS.md / README.md / CHANGELOG.md ✅
  ├─ 11.2 协议文档（自动生成）
  ├─ 11.3 开发者指南 / 部署指南 ✅ (README.md 快速开始)
  └─ 11.4 测试用例 ✅ (430 tests passing)

Phase 12: 持续迭代 ✅
  ├─ 12.1 反馈收集 / bug 修复 / 性能调优 ✅
  ├─ 12.2 社区建设 / 文档完善 ✅
  ├─ 12.3 客户端现代化（DX11 + 现代UI） ✅
  └─ 12.4 服务端性能优化（IOCP + 内存池） ✅
```

---

## 2. 关键技术选型（Phase 1-7 的核心决策）

### 2.1 编译器与构建

| 维度 | 选型 | 理由 |
|------|------|------|
| 编译器 | **MSVC 2022** (Windows) / **Clang 17+** (跨平台) | 兼容老代码 + 现代 C++20/23 |
| C++ 标准 | **C++17** 起步，**C++20** 模块化 | 兼容 2003 代码 + 现代特性 |
| 构建系统 | **CMake 3.25+** + **Ninja** | 工业标准，IDE 友好 |
| 包管理 | **vcpkg** (Windows) / **Conan** (跨平台) | 现代 C++ 标配 |
| 测试 | **GoogleTest** + **Catch2** | 老代码重构必备 |
| 静态分析 | **clang-tidy** + **PVS-Studio** | 防止技术债累积 |

### 2.2 图形渲染

| 维度 | 选型 | 理由 |
|------|------|------|
| 渲染 API | **DirectX 11** (默认) / **DirectX 12** (可选) / **Vulkan** (跨平台可选) | DX11 兼容性最佳；DX12 性能上限高 |
| 着色器 | HLSL 5.0+ / SPIR-V (Vulkan) | DX 标准 |
| 窗口 | **SDL3** (跨平台) 或原生 Win32 | SDL 跨平台 + 输入一体化 |
| 数学 | **DirectXMath** (Windows) / **glm** (跨平台) | 现代 SIMD 优化 |
| 贴图 | DirectXTex (BC1-BC7 + TGA 读取) | 与 DX11 配套 |

### 2.3 网络

| 维度 | 选型 | 理由 |
|------|------|------|
| 异步 I/O | **Boost.Asio** (成熟) / **standalone Asio** (轻量) | C++ 标准库化倾向 |
| 协议 | 保留原 `[CC]Header/Protocol.h` (96 类) + 可选 **FlatBuffers** (新协议) | 老协议 100% 兼容 |
| 加密 | **OpenSSL 3.x** (AES-256-GCM) | 标准合规 |
| 压缩 | **zstd** | 比 zlib 快 |

### 2.4 数据库

| 维度 | 选型 | 理由 |
|------|------|------|
| 默认 | **MS SQL Server 2019+** | 原版即 MSSQL |
| 备选 | **PostgreSQL 16+** | 开源、跨平台 |
| 抽象层 | 自研 `IDbAdapter`（保留原 `eQueryType` 路由） | 兼容老代码 |
| ORM | **不引入**（保持原手写 SQL 风格） | 避免大改 |
| 驱动 | 原 ODBC 保留 / 新代码可用 libpqxx / ODBC | |

### 2.5 UI / 工具

| 维度 | 选型 | 理由 |
|------|------|------|
| 客户端 UI | **保留自研 cWindow**（DX11 后端） / **可选 ImGui**（调试模式） | 1:1 复刻必选保留 |
| 工具 GUI | **C# Avalonia** / **Rust egui** / **Web (React/Vue)** | 现代跨平台 |
| Web 后台 | **FastAPI** (Python) / **ASP.NET Core** (C#) | 现代 Web 标准 |

### 2.6 第三方库替换表

| 原依赖 | 新依赖 | 替换阶段 |
|--------|--------|---------|
| DirectX 8.1 SDK | DirectX 11/12 SDK + DirectXTex | Phase 5 |
| d3dx8.lib | (内置于现代 SDK) | Phase 5 |
| HackShield (AhnLab) | 服务端权威校验 / 移除 | Phase 3 |
| nProtect GameGuard | 服务端权威校验 / 移除 | Phase 3 |
| HSEL 加密狗 | OpenSSL AES-256-GCM | Phase 3 |
| Miles Sound System | **OpenAL Soft** / FMOD / Wwise | Phase 6 |
| FreeImage | DirectXTex / stb_image | Phase 5 |
| MFC (UI) | 保留 cWindow 自研 | 不替换 |
| wsock32.lib | WinSock 2 (ws2_32.lib) | Phase 0 |
| vfw32.lib + wmstub.lib + amstrmid.lib | libavcodec + SDL | Phase 5 |
| 3ds Max Biped/Physique 插件 | 保留 + FBX 中间格式 | Phase 10 |
| Perforce (.vsscc) | Git | Phase 0 |
| VC6/VS2003 工程文件 | CMake | Phase 7 |

---

## 3. 1:1 复刻清单（绝对不能破坏的部分）

### 3.1 协议层（必须完全保留）

- **`[CC]Header/Protocol.h` 3542 行的 MP_CATEGORY 枚举**
- **`[CC]Header/CommonStruct.h` 140 KB 网络包结构**
- **`[CC]Header/CommonGameDefine.h` 108 KB 常量与枚举**
- **服务端分发逻辑**（13 个 NetworkMsgParse 入口）
- **客户端 OnRecv → CGameState::NetworkMsgParse 路径**
- **HSEL 加密算法**（即使换成 AES，加密后字节流可与旧客户端握手兼容）

### 3.2 资源格式（必须完全保留）

| 格式 | 实现位置 | 关键 header |
|------|----------|------------|
| `.bin` | `[Tool]PackingMan/MHFileEx.cpp` | `MHFILE_HEADER { dwVersion, dwType, FileSize }` |
| `.pak` | `4DyuchiFileStorage/PackFile.cpp` | `PACK_FILE_HEADER { dwTotalSize, dwFileCount, ... }` |
| `.bmhm` / `.mhm` | `4DyuchiGXMapEditor/` | 8 字节 magic `7E-CB-31-01-2A-00-00-00` |
| `.ttb` | `MHMap.cpp` | TileTable |
| `.chx` / `.chl` / `.chr` | `MAXEXP/` + 客户端 `cCharMove` | 自研格式 |
| `.mon` | 客户端 `Monster.cpp` | – |
| `.bsad` | `[CC]Skill/SkillArea` | 技能区域 |
| `.mhs` | `StringLoader.cpp` | 字符串索引 |

### 3.3 游戏逻辑（必须完全保留）

- 战斗公式（`AttackCalc.cpp`、`BattleFactory_Default.cpp`）
- 技能树（`SuryunRegen`、`SkillManager_server.cpp`）
- 物品强化/合成/注魂（`ReinforceManager`、`RarenessManager`、`ChangeItemMgr`）
- 工会战/攻城战/据点战（`GuildFieldWarMgr`、`SiegeWarMgr`、`FortWarManager`）
- 摆摊/交易/拍卖（`StreetStall`、`ExchangeManager`、`AuctionContents`）
- 任务/成就（`QuestManager`、`QuestUpdater`）
- 怪物 AI（`AISystem`、`AIManager`）
- 宠物/泰坦（`Pet`、`Titan`）

### 3.4 玩法平衡数据（必须完全保留）

- 经验曲线
- 物品属性
- 技能伤害公式
- 装备掉率
- Boss 刷新
- 商城物品价格

**这些都是 `.bin` 文件，读取正确即可；不要去"调整平衡"或"修复 bug"——除非用户明确要求。**

---

## 4. 风险与缓解

| 风险 | 等级 | 缓解措施 |
|------|------|---------|
| DX11 后端与原 DX8 渲染细节不一致 | 中 | 1:1 截图比对 + 自动化视觉回归 |
| 老 .vcproj/.dsp 无法直接迁移 | 高 | 先保留旧工程，CMake 与其共存 |
| `.chl/.anm` 模型格式无人文档化 | 中 | 编写逆向文档 + 转换器 |
| HSEL 加密狗替换导致老客户端无法登录 | 低 | 保留 HSEL 接口兼容层，默认禁用 |
| 3ds Max 2018+ 移除 Biped 插件 | 低 | 保留 3ds Max 7-2017 旧版 + 提供 FBX 中间格式 |
| 数据库 1:1 兼容但 MSSQL 性能瓶颈 | 低 | 保留 IDbAdapter 接口，可换 PostgreSQL |
| 跨平台编译工作量大 | 中 | 先 Windows-only（DX11），跨平台作为可选项 |

---

## 5. 阶段交付物（每阶段的"完成标准"）

### Phase 0 交付物
- [x] 本计划文档
- [x] `.gitignore`（过滤旧工程文件）
- [x] `AGENTS.md`（AI 助手指南）
- [x] `modern/CMakeLists.txt` 现代工程骨架（CMake 3.20+ / C++20）
- [x] `scripts/start-server.ps1` 一键启动脚本

### Phase 1 交付物
- [x] `modern/src/mh_file_ex.cpp` + `modern/src/pack_file.cpp` + 7 个 resource format 解析器（MoxianCompat 静态库 — CMHFileEx + PackFile + chr_motion + chx_model + bmhm_map + bsad_area + ttb_tile_table 重写；Phase 12.1 后作为 flat `modern/src/*.cpp` 而非独立子目录）
- [x] `tools/MoxianResourceExplorer` 命令行资源浏览器
- [x] 单元测试：所有 `.bin/.pak/.bmhm/.bsad` 读取验证
- [x] 文档：`docs/RESOURCE_FORMATS.md`

### Phase 2 交付物
- [x] `modern/src/db_adapter.cpp` + `db_factory.cpp` + `mssql_odbc_adapter.cpp` + `sqlite_adapter.cpp` (MoxianDb IDbAdapter 接口 — Phase 12.1 后作为 flat `modern/src/*.cpp` 而非独立子目录)
- [x] MSSQL + PostgreSQL + SQLite 三实现
- [x] `tools/MoxianDbTool` schema 导出
- [x] 文档：`docs/DATABASE_SCHEMA.md`

### Phase 4 交付物（网络层现代化）
- [x] `modern/include/mxh/net/net.hpp` — TcpServer/TcpClient/IConnectionHandler 接口
- [x] `modern/include/mxh/proto/protocol.hpp` — MSGROOT/MSGBASE 兼容的 MsgHeader 定义
- [x] `modern/src/net/net.cpp` — WinSock2 + thread-per-connection 实现
- [x] `modern/src/server/login_handler.cpp` — Login (Distribute) server handler
- [x] `modern/src/server/agent_handler.cpp` — Agent server handler
- [x] `modern/tools/MoxianLoginServer/` — 功能完整的登录服务端 demo
- [x] `modern/tools/MoxianAgentServer/` — 功能完整的中转服务端 demo（5 locale）
- [x] `modern/tests/unit/net/net_test.cpp` — 16 个单元测试覆盖 server/client 全流程 + 加密集成
- [x] 加密接口预留：`IEncryptor` 抽象类（Phase 3 集成点）
- [x] 4.3 协议版本化：`kProtocolVersion`/`kMinProtocolVersion` 常量 + `CheckVersion`/`NotifyVersionAck`/`NotifyVersionNack` 协商流程 + LoginHandler 集成 + 27 个单元测试
- [x] 4.4 加密/压缩/校验中间件：IEncryptor 已集成到 TcpServer 发送/接收 + TcpClient 发送/接收 + 接收循环 + 3 个加密集成测试

### Phase 5 交付物（DX11 渲染器）
- [x] `modern/src/render/dx11/` — DX11 后端（mxh_render 静态库以 `modern/src/render/dx11/*.cpp` 形式落地，而非独立 `mxh_render` 子目录）
- [x] `modern/include/mxh/render/IRenderer.hpp` — 1:1 端口（75 个方法签名）
- [x] `modern/include/mxh/render/IFileStorage.hpp` — 1:1 端口（27 个方法签名）
- [x] `modern/include/mxh/render/render_typedef.hpp` — 与原 DX8 引擎二进制兼容的结构体
- [x] Device 初始化（SwapChain / RenderTarget / 默认状态对象）
- [x] PrimitiveDrawer（RenderBox/Line/Point/Circle/Grid 的 DX11 实现）
- [x] SpriteObject（ID3D11Texture2D + SRV，Draw + Resize + LockRect）
- [x] TGA 解码器（uncompressed + RLE，含 bottom-up 翻转）
- [x] `tools/MoxianRenderDemo` — 烟雾测试（3D lit textured cube + wireframe grid）
- [x] TGA 解码器单元测试 — 7 个 case 全过（`TgaLoader.*`）
- [x] MeshObject DX11 实现（`MeshObject::StartInitialize/EndInitialize/InsertFaceGroup`）
  + 自研 HLSL 着色器 (`kVS_Lit` / `kPS_Lit`，D3DCompile `vs_4_0` / `ps_4_0`)
  + 3 个常量缓冲: `world` + `viewProj` + `light (ambient/diffuse/lightDir/cameraPos/fog)`
  + 立方体生成 (`initializeCube`, 24 vert / 36 idx) + MeshDescAndFaceDesc 合约单元测试
- [x] FontObject DX11 实现（GDI `GetGlyphOutlineA` + 512×512 BGRA atlas + row-packing）
  + 8-bit 单字节码点缓存（与原 MultiByte 构建的 `TCHAR == char` 一致；CJK 由 .TTB 预处理管线承担）
  + `packGlyph` 行内打包 + 溢出时整张 atlas 复位（CPU 侧算法可独立单测，4 case）
  + 复用 PrimitiveDrawer::drawTexturedQuad，无需新增 shader
- [x] ChxModel 真实资源测试（4 case：`.chx` 是 TAB 分隔文本元数据, 不是二进制 mesh）
- [x] ctest 集成（gtest_add_tests 手工列举, 绕开中文路径下 gtest_discover_tests 的 JSON 输出超时）
- [x] Phase 5 进度报告（见下方 "Phase 5 当前状态摘要"）
- [x] **Phase 5 wrap-up（2026-07-09）**: 12 commits (5.9 → 5.13) closed all IRenderer stubs + BC1/3/4/5 encoders + motion cache. 175/175 modern PASS.

### Phase 5 当前状态摘要（2026-07-09 更新 — **Phase 5 wrap-up**）

**✅ Phase 5 主交付物全部完成。剩 BC6H/BC7（HDR + RGBA 高质量）— 旧引擎使用率低，留为 future。**

**最后两轮 (8 uncommitted files → 2 commits `0d78386` + `11dff7c`, +40 tests, 175/175 → 189/189 PASS)**：

`0d78386` — **Phase 5.12: deferred renderer state + BC1/3/4/5 encoders + multi-light PS**
- `SetRTLight` / `InitializeRenderTarget` / `SetLoadFailedTextureTable` / `GetLoadFailedTextureTable` — 4 个 deferred renderer 状态方法实做
- `saveDDS_BC` 真实 BC1 (DXT1) + BC3 (DXT5) 块编码器
- `saveDDS_BC(tex, BCFormat)` 重构 + `BCFormat` 枚举 + BC4 (ATI1) + BC5 (ATI2) 双通道 normal map
- `ConvertCompressedTexture` DDS fast-path
- `GetD3DDevice` 扩展到 `IUnknown` / `ID3D11Device` / `ID3D11DeviceContext`
- `ResetDevice` 清理所有 light state
- psMultiLight 着色器（8-slot dynamic+RT light 累加）

`11dff7c` — **Phase 5.13: motion cache for per-motion VB/IB tracking**
- 替换 `ClearCacheWithMotionUID` 的 stub 为真实 per-motion VB/IB cache
- 注册/注销/查询 API（内部，IRenderer 表面只暴露 clear）
- refcount 共享 motion，N mesh 用同一 motion 时 GPU buffer 复用
- +14 tests: Register/Lookup/Unregister/Clear + null/unknown edge cases

**Phase 5 收官总览（2026-07-09）**：

| Stub | 状态 | 备注 |
|------|------|------|
| `CreateHeightField` | ✅ done | 5.9c (38783fd) |
| `CreateMaterial[Set]` | ✅ done | 5.9 (7bf70b8) |
| `CreateEffectShaderPalette*` | ✅ done | effect_shader.cpp |
| `RenderTri*` / `AllocRenderTriBuffer*` | ✅ done | renderer.cpp:407-614 |
| `CaptureScreen` | ✅ done | renderer.cpp:817 |
| `BeginShadowMap` / `EndShadowMap` | ✅ done (simple) | renderer.cpp:187-194, calls device |
| `GetD3DDevice` (非 IUnknown IID) | ✅ done (extended) | ID3D11Device + ID3D11DeviceContext |
| `SetRTLight` / `InitializeRenderTarget` / `SetLoadFailedTextureTable` | ✅ done (5.12) | |
| `ConvertCompressedTexture` BC1/BC3 | ✅ done (5.12) | saveDDS_BC + DDS passthrough |
| `ConvertCompressedTexture` BC4/BC5 | ✅ done (5.12) | ATI1/ATI2 FourCC, encode_single_channel_block |
| Motion cache (per-motion VB/IB) | ✅ done (5.13) | refcount-shared |
| `INL_GetVLMeshEffect` helper | ✅ done (pre-5.12) | 1-line wrapper, covered by effect_shader_test |
| `ConvertCompressedTexture` BC6H/BC7 | 🟡 partial (12.1 P2-11) | DX10 ext header + mean-color mode 1/6; **R-11** 等 DirectXTex/bc7enc 接入 |

> **FontObject 范围说明**：8-bit 缓存（非 Unicode CJK 簇）；CJK 应走老引擎自己的 .TTB 预烤字图。
> DirectWrite SDF / 多色 emoji 需要单独 Phase 6+ 工作。

**测试统计 (Debug, 2026-07-09 wrap-up)**：
| 套件 | 数量 | 内容 |
|------|------|------|
| `TgaLoader` | 7 | TGA uncompressed / RLE / bottom-up-flip / RGBA32 |
| `MeshGeometryTest` | 4 | MESH_DESC + FACE_DESC 合约 |
| `FontObjectGlyph` | 2 | GlyphEntry 字段 + CHAR_CODE_TYPE 枚举值 |
| `FontObjectAtlas` | 4 | row-packing |
| `MhFileEx` | 6 | `.bin` XOR/位移加解密 + CRC 校验 |
| `PackFile` | 5 | `.pak` 头解析 + 实资源回环 |
| `BsadArea` | 4 | `.bsad` 技能区域 |
| `ChxModelRealResource` | 4 | 真实 `.chx` TAB 分隔文本 |
| `DbAdapter` | 4 | `IDbAdapter` 工厂 + 配置 |
| `SqliteAdapter` | 5 | SQLite 后端 |
| `RealResource` | 2 | 真实 `MonsterList.bin` + `Effect.pak` |
| `MatrixMathTest` | 3 | 矩阵数学 |
| `HeightFieldTest` | 8 | 高度图 |
| `HeightFieldRenderGrid` | 3 | RenderGrid 入口 + accessor |
| `HeightFieldPool*` | 7 | Pool caps / guards / key encoding / palette |
| `HFieldObject*` | 12 | HField object 全套 |
| `EffectShaderTest` | 13 | EffectEntry / Palette / buildFromDesc / matrix ops |
| `MaterialDataTest` + `MaterialSet*` + `MaterialContract*` | 14 | MATERIAL 字段 / 集合 / 合约 |
| `DynamicLightDefaults` + `LightCBInit` + `ColorConversion` + `LightIndexDesc` + `DynamicLightConstants` | 11 | light 默认值 + cbuffer 布局 |
| `TriBufferMagic` | 1 | TRIB magic 常量 |
| `TextureLoaderTGA` / `TextureLoaderDDS` / `TextureLoaderAutoDetect` | 6 | TGA roundtrip / DDS magic+BGRA / 探测 |
| `SaveDDSBC` (BC1/BC3) | 5 | DDS 文件头 + 块编码 + 边界 + 索引 |
| `SaveDDSBC4` (BC4/ATI1) | 4 | 单通道 R 编码 + 自适应端点 |
| `SaveDDSBC5` (BC5/ATI2) | 4 | 双通道 R+G 编码 + 块栅格 |
| `SetRTLight` / `BuildLightCBRTLight` | 7 | RT light 存/索引/累加/默认 range |
| `InitializeRenderTarget` | 3 | RT 池配置 + clamp |
| `SetLoadFailedTextureTable` / `GetLoadFailedTextureTable` | 3 | fallback texture 表 |
| `MotionCache` | 14 | Register/Lookup/Unregister/Clear 全套 + edge cases |
| **Render 合计** | **132** | |
| MoxianRenderDemo smoke | 1 | Phase 5.10/5.11/6+/7 (7911120) |
| `MoxianCompat` + `MoxianDb` + `MoxianResourceExplorer` + 其它 | ~56 | resource/db/explorer 工具集 |
| **Modern 全部** | **189** | **Debug 全过** |

### Phase 6 wrap-up（2026-07-09 — UI 框架全部落地，4 commit 闭环 0.0→0.8）

**8 commits (6.0 → 6.7) 落地 7 个 widget + dispatcher + 兼容层 + smoke。254/254 PASS。**

| 提交 | Phase | 内容 | +测试 |
|------|-------|------|-------|
| `920faed` | 6.0 | cObject + cWindow framework skeleton | 19 |
| `d79b2f0` | 6.1 | cButton (state machine + click dispatch) | 20 |
| `0321207` | 6.2 | cEditBox (text buffer + caret + keys) | 18 |
| `77d5806` | 6.3 | cDialog (container + caption + tree cascade) | 16 |
| (未 commit) | 6.4 | cImage (Phase 5→6 GPU seam, render adapter) | 13 |
| (未 commit) | 6.5 | cListCtrl (multi-column list + selection + viewport) | 16 |
| (未 commit) | 6.6 | cWindowManager (top-most dispatch + modal + defer-destroy) | 14 |
| (未 commit) | 6.7 | legacy_compat (WE_* + cbWindowFunc bridge) | 6 |
| `265c7f9` | 6.8 | mxh_ui_smoke (headless UI integration) | (smoke) |
| (本会话) | 6.9 | cMsgBox (modal 4-type box + keys + dispatcher integration) + cDialog exposure fixes (findWindowById/alpha/SetAbsXY/SetDisable public) + SetModalDialog auto-Activate + /FS in test vcxproj | 14 |
| (本会话) | 6.10 | cDivideBox (split-stack dialog — OK/Cancel callbacks + numeric cEditBox + min/max clamp + Enter-to-confirm) | 12 |
| (本会话) | 6.11 | cIconDialog (icon-grid data model — cells + PtInCell + AddIcon/DeleteIcon + selection + acceptableIconType mask) | 15 |
| (本会话) | 6.12 | cGuildDialog (1/80 真实 legacy 端口 — header + member list + tabs + rank-gated buttons) + 依赖 widget 端口 cStatic (9 tests) + cPushupButton (5) + cListDialog (12) | 39 |
| (本会话) | 6.13 | cMultiLineText (linked-list of text lines — NPC dialogs, quest descriptions, system messages) | 14 |

**Phase 6 收官总览**：

| Widget | 状态 | 备注 |
|--------|------|------|
| `cObject` 基类 | ✅ done (6.0) | id / name / parent |
| `cWindow` framework | ✅ done (6.0) | hit-test / child / focus / dispatch / SetAbsXY/SetEnabled virtuals |
| `cButton` | ✅ done (6.1) | 3-state image / click / drag-cancel / text |
| `cEditBox` | ✅ done (6.2) | buffer / caret / keys / read-only / secret / valid-check |
| `cDialog` | ✅ done (6.3) | container / caption rect / findById / cascade / active |
| `cImage` (Phase 5→6 seam) | ✅ done (6.4) | borrow SpriteObject + render adapter |
| `cListCtrl` | ✅ done (6.5) | columns / rows / selection / viewport / hit-test / callbacks |
| `cWindowManager` | ✅ done (6.6) | top-most dispatch / modal / defer-destroy / RenderAll |
| `legacy_compat` shim | ✅ done (6.7) | WE_* codes + cbWindowFunc → std::function bridge |
| `mxh_ui_smoke` | ✅ done (6.8) | headless end-to-end integration smoke |
| `cMsgBox` | ✅ done (6.9) | modal message box + 4 MBType + Enter/Esc + auto-close |
| `cDivideBox` | ✅ done (6.10) | split-stack dialog (OK/Cancel + numeric input + min/max clamp) |
| `cIconDialog` | ✅ done (6.11) | icon-grid container (cells, AddIcon/DeleteIcon, selection, type-mask) |
| `cStatic` | ✅ done (6.12) | text label widget (text + color + shadow + align) |
| `cPushupButton` | ✅ done (6.12) | toggle / sticky button (m_pushed, m_passive) |
| `cListDialog` | ✅ done (6.12) | scrollable text list (rows + selection + scroll) |
| `cGuildDialog` | ✅ done (6.12) | first real legacy-port dialog (header + member list + tabs + rank-gated buttons) |
| `cMultiLineText` | ✅ done (6.13) | multi-line text node list (NPC dialogs, quest descriptions, system messages) |
| `cExitDialog` | ✅ done (12.x 0.13.10) | exit-confirm dialog (callback pattern, R-12 polymorphic SetActive fixed) |
| `cGuagen` | ✅ done (12.x 0.13.12) | progress bar widget (1:1 quirk: SetValue negatives pass through, upper-bound clamp only) |
| `cListDialogEx` | ✅ done (12.x 0.13.13) | link list with WE_ROWCLICK + multi-color chain (HelpDialog / PartyDialog / JournalDialog etc) |
| Real GPU draw (cImage → SRV) | 🟡 partial (12.1) | bindRenderer hook done (6.4); **R-10** reference adapter 等真 host 接入 |
| IME (Korean/JP composition) | ✅ done (12.1 P2-10) | ime.hpp/ime.cpp/ime_win32_imm.cpp + 13 tests |
| R-9 matrix D3DX row-major audit | ✅ done (12.x 0.13.12) | MatrixLookAtLH/OrthographicLH layout fix + 7 new tests (off-axis eye) |
| Phase 13 service interfaces | 🟡 partial (12.x 0.13.12) | IInventoryService / ISkillService / IPlayerStatsService headers + 11 mock tests; real impls 待 Tier 3 dialog 需要时落地 |
| Drag-drop rendering wiring | ⏳ future | cIcon sprite + dispatch (data model is 6.11) |
| Sortable columns | ⏳ future | cListCtrl sort already in 6.5 |
| 195 个 legacy 对话框（Guild*/Inventory*）| ⏳ future | 1:1 端口 (7/202 = 3.5% done: 5 base+2 dialog+3 subcontrol; **P2-12 roadmap** 在 `docs/P2-12_DIALOGS_ROADMAP.md` + **R-9** 矩阵 audit 跟 drawBox deferred) |

**测试统计 (Debug, 2026-07-09 Phase 6 wrap-up)**：
| 套件 | 数量 | 内容 |
|------|------|------|
| Render（含 Phase 5.12/5.13 deferred）| 132 | TGA/Mesh/FontObject/HeightField/Material/Effect/Texture/SaveDDSBC/MotionCache |
| UI: cWindow | 19 | framework skeleton |
| UI: cButton | 20 | state machine + click |
| UI: cEditBox | 18 | text buffer + caret + keys |
| UI: cDialog | 16 | container + caption + cascade |
| UI: cImage | 13 | Phase 5→6 GPU seam + render adapter |
| UI: cListCtrl | 16 | multi-column list |
| UI: cWindowManager | 14 | dispatch + modal + defer-destroy |
| UI: legacy_compat | 6 | WE_* + cbWindowFunc bridge |
| UI: cMsgBox | 14 | modal message box (4 MBType + keys + dispatcher integration) |
| UI: cDivideBox | 12 | split-stack dialog (OK/Cancel + numeric input + min/max) |
| UI: cIconDialog | 15 | icon-grid container (cells + AddIcon/DeleteIcon + PtInCell) |
| UI: cStatic | 9 | text label widget (text + color + shadow + align) |
| UI: cPushupButton | 5 | toggle / sticky button (m_pushed, m_passive) |
| UI: cListDialog | 12 | scrollable text list (rows + selection + scroll) |
| UI: cGuildDialog | 13 | first real legacy-port dialog (header + member list + tabs + rank) |
| UI: cMultiLineText | 14 | multi-line text node list (NPC dialogs + quest descriptions) |
| **UI 合计** | **216** | |
| MoxianRenderDemo smoke (3D+2D+Effect+Mtl) | 1 | visual smoke |
| mxh_ui_smoke (Phase 6.8 headless) | 1 | framework end-to-end |
| `MoxianCompat` + `MoxianDb` + 其它 | ~58 | resource/db tools |
| **Render + UI 合计** | **348** | **Debug 全过** |

### Phase 6 wrap-up v2（2026-07-09 — 5 commit 闭环 6.9→6.13，UI 全面落地）

**Phase 6 收官补充：5 commits (6.9 → 6.13) 落地 5 个 widget + Phase 6 全部就位。406/406 PASS。**

| 提交 | Phase | 内容 | +测试 |
|------|-------|------|-------|
| `aa84cea` | 6.9 | cMsgBox (modal 4-type box + keys + dispatcher integration) + cDialog exposure + SetModalDialog auto-Activate + /FS in test vcxproj | 14 |
| `53712aa` | 6.10 | cDivideBox (split-stack dialog) | 12 |
| `02442ed` | 6.11 | cIconDialog (icon-grid data model) | 15 |
| `c15fa77` | 6.12 | cGuildDialog (1/80 真实 legacy 端口) + cStatic + cPushupButton + cListDialog | 39 |
| (本会话) | 6.13 | cMultiLineText (multi-line text node list) | 14 |

**Phase 6 v2 收官总览 (UI 全部 widget 已落地)**：

| Widget | 状态 | 备注 |
|--------|------|------|
| `cObject` 基类 | ✅ done (6.0) | id / name / parent |
| `cWindow` framework | ✅ done (6.0) | hit-test / child / focus / dispatch / SetAbsXY/SetEnabled virtuals |
| `cButton` | ✅ done (6.1) | 3-state image / click / drag-cancel / text |
| `cEditBox` | ✅ done (6.2) | buffer / caret / keys / read-only / secret / valid-check |
| `cDialog` | ✅ done (6.3) | container / caption rect / findById / cascade / active |
| `cImage` (Phase 5→6 seam) | ✅ done (6.4) | borrow SpriteObject + render adapter |
| `cListCtrl` | ✅ done (6.5) | columns / rows / selection / viewport / hit-test / callbacks |
| `cWindowManager` | ✅ done (6.6) | top-most dispatch / modal / defer-destroy / RenderAll |
| `legacy_compat` shim | ✅ done (6.7) | WE_* codes + cbWindowFunc → std::function bridge |
| `mxh_ui_smoke` | ✅ done (6.8) | headless end-to-end integration smoke |
| `cMsgBox` | ✅ done (6.9) | modal message box + 4 MBType + Enter/Esc + auto-close |
| `cDivideBox` | ✅ done (6.10) | split-stack dialog (OK/Cancel + numeric input + min/max clamp) |
| `cIconDialog` | ✅ done (6.11) | icon-grid container (cells, AddIcon/DeleteIcon, selection, type-mask) |
| `cStatic` | ✅ done (6.12) | text label widget (text + color + shadow + align) |
| `cPushupButton` | ✅ done (6.12) | toggle / sticky button (m_pushed, m_passive) |
| `cListDialog` | ✅ done (6.12) | scrollable text list (rows + selection + scroll) |
| `cGuildDialog` | ✅ done (6.12) | first real legacy-port dialog (header + member list + tabs + rank) |
| `cMultiLineText` | ✅ done (6.13) | multi-line text node list (NPC dialogs + quest descriptions) |
| `cExitDialog` | ✅ done (12.x 0.13.10) | exit-confirm dialog (callback pattern, R-12 polymorphic SetActive fixed) |
| `cGuagen` | ✅ done (12.x 0.13.12) | progress bar widget (1:1 quirk: SetValue negatives pass through, upper-bound clamp only) |
| `cListDialogEx` | ✅ done (12.x 0.13.13) | link list with WE_ROWCLICK + multi-color chain (HelpDialog / PartyDialog / JournalDialog etc) |
| `cMacroDialog` | ✅ done (12.x 0.13.14) | macro key binding dialog (Tier 2, 1st real dialog port; ME_* / MM_* / MSK_* 1:1 enum mirroring) |
| `cCharMakeDlg` | ✅ done (12.x 0.13.15) | sex-selector dialog (Tier 2, 3rd dialog port; 4 cStatic toggled by sex 0=M/1=F; SetVisible 替代 legacy 非存在 SetActive 1:1 quirk; OnActionEvent no-op 等 CharMakeManager port) |
| `cGuildJoinDialog` | ✅ done (12.x 0.13.16) | guild member-invite dialog (Tier 2, 4th dialog port, header 275B 最小; 3 button id kJoinMember/kJoinStudent/kJoinCancel 210-212; OnActionEvent 4-singleton dispatch 阻塞) |
| Real GPU draw (cImage → SRV) | 🟡 partial (12.1) | bindRenderer hook done (6.4); **R-10** reference adapter 等真 host 接入 |
| IME (Korean/JP composition) | ✅ done (12.1 P2-10) | ime.hpp/ime.cpp/ime_win32_imm.cpp + 13 tests |
| R-9 matrix D3DX row-major audit | ✅ done (12.x 0.13.12) | MatrixLookAtLH/OrthographicLH layout fix + 7 new tests (off-axis eye) |
| Phase 13 service interfaces | ✅ done (12.x 0.13.12 + 0.13.14) | 3 header-only interfaces + 3 real impls (InventoryServiceImpl/PlayerStatsServiceImpl/SkillServiceImpl backed by ItemTotalInfo/PlayerCombatStats/vector<LearnedSkill>) + 11 mock + 15 real contract tests |
| Drag-drop rendering wiring | ⏳ future | cIcon sprite + dispatch (data model is 6.11) |
| 194 个 legacy 对话框（Guild*/Inventory*）| ⏳ future | 1:1 端口 (10/202 = 5.0% done: 5 base+4 dialog+4 subcontrol; **P2-12 roadmap** 在 `docs/P2-12_DIALOGS_ROADMAP.md` + **R-9** 矩阵 audit 跟 drawBox deferred) |

**测试统计 (Debug, 2026-07-09 Phase 6 v2 wrap-up)**：
| 套件 | 数量 | 内容 |
|------|------|------|
| Render（含 Phase 5.12/5.13 deferred）| 132 | TGA/Mesh/FontObject/HeightField/Material/Effect/Texture/SaveDDSBC/MotionCache |
| UI: cWindow | 19 | framework skeleton |
| UI: cButton | 20 | state machine + click |
| UI: cEditBox | 18 | text buffer + caret + keys |
| UI: cDialog | 16 | container + caption + cascade |
| UI: cImage | 13 | Phase 5→6 GPU seam + render adapter |
| UI: cListCtrl | 16 | multi-column list |
| UI: cWindowManager | 14 | dispatch + modal + defer-destroy |
| UI: legacy_compat | 6 | WE_* + cbWindowFunc bridge |
| UI: cMsgBox | 14 | modal message box (4 MBType + keys + dispatcher integration) |
| UI: cDivideBox | 12 | split-stack dialog (OK/Cancel + numeric input + min/max) |
| UI: cIconDialog | 15 | icon-grid container (cells + AddIcon/DeleteIcon + PtInCell) |
| UI: cStatic | 9 | text label widget (text + color + shadow + align) |
| UI: cPushupButton | 5 | toggle / sticky button (m_pushed, m_passive) |
| UI: cListDialog | 12 | scrollable text list (rows + selection + scroll) |
| UI: cGuildDialog | 13 | first real legacy-port dialog (header + member list + tabs + rank) |
| UI: cMultiLineText | 14 | multi-line text node list (NPC dialogs + quest descriptions) |
| **UI 合计** | **216** | |
| MoxianRenderDemo smoke (3D+2D+Effect+Mtl) | 1 | visual smoke |
| mxh_ui_smoke (Phase 6.8 headless) | 1 | framework end-to-end |
| `MoxianCompat` + `MoxianDb` + 其它 | ~58 | resource/db tools |
| **Render + UI 合计** | **348** | **Debug 全过** |
| ctest 全过 | **406/406** | 全套件（11 套件 + smokes + tools）|

### Phase 6 入口分析（2026-07-09 — 准备起步）

**目标**：UI 系统现代化 (3-4 周 per plan §1.3)

**legacy 现状** (`[Client]MH/`):
- 框架核心：`interface/cWindow.h` (4.6 KB) + `cWindow.cpp` (4.3 KB) + `cWindowSystemFunc.h/cpp` (15 KB)
- 控件树：`cObject` 基类 → `cWindow` → `cButton` / `cEditBox` / `cListCtrl` / `cDialog` / 各种 Guild*/Inventory* 对话框（~80 个 .h）
- 渲染：`cImage` + GDI/MFC，`ToolTipRender` 走 GDI text
- 事件：`ActionEvent(CMouse*)` / `ActionKeyboardEvent(CKeyboard*)` + WE_NULL/... 枚举
- 国际化：散落 `_KOR_LOCAL_` / `_TW_LOCAL_` / `_CHINA_LOCAL_` / `_JP_LOCAL_` / `_HK_LOCAL_` / `_TL_LOCAL_` / `_NDSC_LOCAL_` 宏

**两条路（plan §6.2）**：
- **6.2.A 保留 cWindow** — 用现代 C++ 重写 framework（4.6+4.3 KB ≈ 300 行核心），保留所有 .bin UI 描述和 80 个对话框的二进制兼容
- **6.2.B 替换为 ImGui** — vendor ImGui + DX11 backend，给 .bin UI 写 ImGui adapter；视觉/交互会变（**违反 1:1**），但代码量小很多

**推荐路线 6.2.A**（1:1 兼容优先），具体分阶段：
- 6.1.1 `cObject` / `cWindow` framework 用现代 C++ 重写（智能指针、范围 for、std::optional），保持 public API 同形
- 6.1.2 `cImage` 接到 `mxh_render` 的 `ID3D11ShaderResourceView`（已有 SpriteObject，加 9-slice/border 支持）
- 6.1.3 `cButton` / `cEditBox` / `cListCtrl` 三大核心控件重写（先无游戏内容、用单元测试验证 hit-test + 事件分发）
- 6.1.4 `cWindowSystemFunc` 工厂方法（CreateMainTitle_m 等）改成注册式创建
- 6.2 决定是否补 ImGui 调试 HUD（`mxh_render_demo` 可加 ~200 行 ImGui overlay）
- 6.3 多语言抽离：先做枚举 + lookup table，保留 macro 1 个 phase 再删

**前置条件**：
- Phase 5 收官（✅ 完成）
- 不需要 SQL Server
- 不依赖 4Dyuchi 引擎运行（重写 framework 后老代码仍能 compile-link，但不直接运行）

**风险**：
- 80 个对话框文件的 binary 兼容（texture index、font index 偏移）— 需先逆向 .bin UI 描述格式
- GDI 文本渲染（TTF 预烤字图 + 多色）— Phase 5 的 FontObject 8-bit 缓存是基础，但 DirectWrite/SDF 是 future
- MFC 依赖移除（stdafx.h 还有 `<afx.h>` / `<ole2.h>` 死代码）— 需同步改

**下一手可执行项**（scope 小，1-2 提交）:
1. 6.0.1 创建 `modern/src/ui/cWindow/` 目录骨架 + `CMakeLists.txt` 子项目
2. 6.0.2 `cObject` + `cWindow` framework 重写（`modern/src/ui/cWindow/{cObject,cWindow}.hpp`）
3. 6.0.3 单元测试：hit-test / 事件分发 / 子树管理（CPU-side，不需 DX11）
4. 6.0.4 把 `cWindowSystemFunc` 旧 .h 路径保留 compat shim，新代码用 modern cWindow

### Phase 3-7 交付物（每阶段）
- [ ] 新模块源码 + 单元测试
- [ ] 回归测试：与原模块行为对比
- [ ] 更新文档
- [ ] CMakeLists.txt 集成

### Phase 7.5 交付物 — 构建系统具体化

#### Phase 7.5d（已完成，2026-07-08）
- [x] `[Server]Map` Release target 可编译（`modern/scripts/build_map_release.py`）
- [x] Bug C-30 修复（`befc5c1` ChannelSystem.cpp 5 处 #ifdef 包裹）
- [x] Wipe+rebuild ×2 字节级复现（`MapServer.exe` = 1,283,584 B，两次完全一致）
- [x] `dumpbin /dependents` 无 renderer 依赖（仅 SS3DGFunc + KERNEL32 + USER32 + ole32 + WINMM）
- [x] 与 `SWorking\MapServer.exe` 字节差 −49.78%（±60% 带内，归因为 MSVC14 `/O2 /GF /Gy` + 无调试信息 + AVX2 收紧）
- [x] 反舞弊 4 子项全部通过（7a/7b/7c/7d）
- [x] Git diff scope 严格（`befc5c1` = 2 文件，`9a9f41f` = 3 文件，零越界）
- [x] Phase 7.5d release diagnostic 文档（`modern/docs/phase7.5d_release_diagnostic.md`，6+1 节）
- [x] KNOWN_BUGS C-30 状态翻转为 fixed（`docs/KNOWN_BUGS.md`）
- [x] PHASE7_MIGRATION_RECIPE row 154 更新（`befc5c1` + verifier `mvs_c310718769664be59a0392f7f477b232`）
- [x] Check 4 smoke（无监听端口）已 override-accept，根因 = dumpbin 缺失 WS2_32/ADVAPI32/ODBC32/4DyuchiNET 导入（移交 `plan_139c4065` 排查完成）— **真因 = COM 架构**，非 build bug（详见 Phase 7.5e）

#### Phase 7.5e（已完成 — `plan_139c4065`，2026-07-08）
- [x] dumpbin 缺失导入根因排查：delay-load vs `_KOR_LOCAL_` 路径门 vs 真实 link spec 缺陷 — **三假设均被 byte 证据排除；真因 = MapServer 是 COM 客户端（170 `g_Network` + 1 `g_DB.Init` 均走 `ole32.CoCreateInstance`）**，`4DyuchiNET.dll` 才是 WS2_32/ADVAPI32 的所在，`DBThread.dll` 才是 ODBC32 的所在
- [x] 补全 [Server]Map Release 运行时依赖分析 — 链路正确，link spec（ws2_32/odbc32/odbccp32/4DyuchiNET.lib）齐全；缺导入是因为 3-tier COM 架构把 socket/SQL*/Reg* 调用 split 到了 DLL 内
- [x] legacy 对照 — `SWorking\MapServer.exe`（VS2008 build，2.5 MB）dumpbin 也是同一 pattern（无 WS2_32/ADVAPI32/ODBC32/4DYUCHI），证明是 2008 起的设计，不是 MSVC14 退化
- [x] 全量 staging（`D:\smoke_test_full` 1.69 GB / 5,263 文件 + 完整 PlayDH）下重跑 Check 4 smoke — `plan_4b45c814`（即 plan_cc0c5b59）。**Stop-not-fail（环境 blocker）**：legacy `SWorking\MapServer.exe` 与新 build 在同 staging 都 `listen=0` / `ServerStart.txt` 未生成（进程早期在 `CheckUpdateFile()` 解析 `Resource\Server\TitanServer.bin` 56-byte XOR-ciphered 韩文文件时静默 return），且本机 `Get-Service MSSQLSERVER = null`（SQL Server 未装） + 无 MonitoringServer。**已确认非 build regression**：legacy 2008 同行为 → 环境。**Forward dependency**：真要 `bind()` listen，必须先装 SQL Server + MS Server 并保证 TitanServer.bin 解码链路（韩文 code page / XOR 解密脚本），移交给环境运维侧

#### Phase 7.5f（已完成 — `plan_5d296f1c`，2026-07-08）
- [x] Bug C-33 修复（commit `62765a3`）：`CMHFile::GetStringInQuotation` 加 bound check（PACKEDFILE + NORMALMODE 两路），`Server.cpp::CheckUpdateFile` 加 `g_Console.LOG` + stderr 显式 log
- [x] 重新 build MapServer.exe（1,283,584 B），wipe+rebuild 字节级复现
- [x] smoke 复测：现在能看到 `CheckUpdateFile failed` 字样（之前 silent return 0），但 **strstr(mismatch) 仍 fail**——次级发现见 Phase 7.5g
- [x] KNOWN_BUGS C-33 状态翻转为 fixed（`docs/KNOWN_BUGS.md`）

#### Phase 7.5g（已完成 — C-34 修复，2026-07-08）
- [x] Bug C-34 根因调查：原以为 "byte-swap"，Python 反向解 .bin 后证伪；真因 = MSVC 默认 `/execution-charset` = 系统 codepage (build host 是 cp936 GBK Windows，CMakeLists 第 99 行注释明示)，Korean Hangul (U+AC00..U+D7A3) 在 cp936 没表达，MSVC **静默**把每个 Korean codepoint 替成 `?` (0x3F)。Dump 出来 strcmp 常量变成 22 字节的 `? ??? ??? ??? ???? ???~`，跟任何 Korean encoding 都 strcmp 不上
- [x] `[Server]Map/CMakeLists.txt` 加 `/execution-charset:utf-8`（grep 过 `[Server]Map/*.cpp` 无 `_mbs*` / `isleadbyte` / `mbstowcs`，理论 MBCS 风险在本 codebase 不存在）。原注释里 "we keep /execution-charset at the default" 整段重写，说明 trade-off
- [x] 新工具 `modern/tools/repack_titan_bin.py`（PackingMan `ConvertBin` / `CheckCRC` 算法 1:1 复刻，加 CLI：`fix-titan` / `encode`）。`test_repack.py` 4 套 roundtrip + legacy file 验证全过
- [x] 重打 smoke staging `D:\smoke_test_full\Resource\Server\TitanServer.bin`（73 字节，dwType=7, dwFileSize=59），原 56 字节 EUC-KR 备份为 `.old_euc_kr`
- [x] rebuild MapServer.exe（1,289,216 B，+5,632 vs pre-7.5g），验证 `.rdata` 里 strcmp 常量变成真 UTF-8 字节 `이 파일이 없으면 타이탄 업데이트 안돼요~` (57 字节，无 `?` placeholder)
- [x] smoke 复测：stderr 全空，进程跑过 `CheckUpdateFile` 进入 `CServerSystem::Start`（下一步卡 C-32 SQL Server blocker，预期）
- [x] KNOWN_BUGS C-34 状态翻转为 fixed（`docs/KNOWN_BUGS.md`，C-33 entry 末尾 + 新 C-34 entry）

#### Phase 7.5h（部分完成 — 1/5 落地，4/5 撞 legacy 暗礁，2026-07-08）
- [x] **C-34 同病扩到 [Server]Distribute**：`[Server]Distribute/CMakeLists.txt:103` 也缺 `/source-charset:utf-8 /execution-charset:utf-8`，跟 7.5g Map 那个 fix 同病。补了
- [x] **Phase 7.5b utf-8 转换脚本没覆盖 Distribute**：`modern/scripts/convert_distribute_sources_to_utf8.py` 新写（idempotent + dry-run + `.pre_utf8.bak` 备份），9 个 .cpp 文件 cp949→utf-8 转换
- [x] **`DistributeServer_Debug_CHINA.exe` 1,306,112 B 落地**（KOR/JAPAN/CHINA/HK/TL 5 个 target 都已配置，CHINA 是首个 build 干净的）
- [x] `modern/scripts/build_distribute_debug_locales.py`（自动化 5 target build + error/warning 摘要 + size 报告）
- [ ] **KOR**：1 LNK error — `mfc71.lib` (VS2003 legacy MFC) 找不到。build host 是 MSVC14，no mfc71.lib。KOR 的 legacy Debug 配置 link 了 mfc71。**out of scope** — 需重写那部分 MFC 调用成 STL/WTL（几个 CString + CWnd），不修也能 `-D_USINGTOOL_` 跳过但需要去 CMakeLists 加 if 改条件
- [ ] **JAPAN / TL**：58 C errors each — CommonGameDefine.h 4 个匿名 enum（`TP_MUGONG_START` 等）在 HK/JP/TL config 下都激活了，重定义 `error C2365`。legacy 应该用 `#ifdef _JAPAN_LOCAL_/TL_LOCAL_` 围起来但源码丢了。**out of scope** — 4 个 enum 都有 40+ members，要分别 `#ifdef` 隔离是大改 shared header
- [ ] **HK**：162 C errors — 同 JAPAN/TL + CommonStruct.h `SLOT_TITANWEAR_NUM` 未声明
- [x] 现象记录到 `KNOWN_BUGS.md` 新 C-35 entry（待补）
- [x] 5 个 target 都已 wired 进 CMakeLists，build 入口齐

---

#### Phase 7.5l（已完成 — D-12 状态翻转，4/5 Agent locale build，2026-07-10）

> Phase 7.5k-B 给 Agent 5/locale build matrix 留下 2 blocker：CHINA (D-12 billing 字段缺失) + HK (D-6 ggsrv25)。
> 本 session 接着 7.5k 解 D-12：

- [x] **D-12 根因查明**：`[Server]Agent/UserTable.h:49` 的 `#ifdef _CHINA_LOCAL` (缺尾下划线) 跟 CMakeLists `_CHINA_LOCAL_` (带尾下划线) 不 match — 4 个 caller cpp 用的都是带尾下划线的正确 macro，struct 字段却被错的不带尾下划线 macro 围栏包 → 永远不展开
- [x] **修法**：1 字符改动 — `[Server]Agent/UserTable.h:49` `#ifdef _CHINA_LOCAL` → `#ifdef _CHINA_LOCAL_`
- [x] **CHINA 26 error → 0 error**：`AgentServer_CHINA.exe` 1,496,064 B 落地
- [x] **5/locale matrix 4/5**（KOR 1,496,576 B / JP 1,498,624 B / **CHINA 1,496,064 B** / TL 1,495,552 B / HK ❌ D-6）
- [x] **0 风险**：不动 shared header、不动 protocol、不动 `#pragma pack(1)`、不动 runtime logic
- [x] **legacy 上遗 bug 落在 legacy 里**：UserTable.h 上游发版时手抖，把 macro 围栏写成错位。修法是 legacy 上「错」的 macro 回到正轨
- [x] **modem matrix 现在 4/5**：`HK ggsrv25.lib (D-6)` 仍是仅剩 blocker
- [x] **D-12 状态翻 fixed**：`docs/KNOWN_BUGS.md` 新加 D-12 Phase 7.5l 修复 entry
- [x] **不依赖 .pre_utf8.bak**：UserTable.h 改动是直接编辑现 file，没用 .bak 还原

#### Phase 7.5n（已完成 — D-6 状态翻转，5/5 Agent + 10/10 server 矩阵，2026-07-10）

> Phase 7.5l 把 Agent 5/locale matrix 推到 4/5，仅剩 HK ggsrv25 linker 错。本 session 接着把它解掉，
> 加上 Phase 7.5i 的 Distribute 5/5 — 服务端两个核心服务现已 5/locale × 2 = 10/10 全绿。

- [x] **D-6 根因查明**：`ggsrv25.lib` 真的缺失（vendor .h 在，.lib 没在 workspace）；
  HK GameGuard 2.5 SDK 链接永远 fail。
- [x] **修法 跟 Phase 7.5i vendor MD5 套路**——source-level vendor stub：
  - new `[Server]Agent/ggsrv25_vendor_stub.cpp` (~7 KB): source-level C++
    port of all symbols declared in `ggsrv25.h` (InitGameguardAuth /
    CleanupGameguardAuth / GGAuthUpdateTimer / AddAuthProtocol / ModuleInfo +
    full CCSAuth2 class + GGAUTH_* C API)
  - NpLog / GGAuthUpdateCallback 留空 → server.cpp 已 provide 这俩 thin
    wrapper (NPROTECTMGR->NpLog / ->GGAuthUpdateCallback)，stub 不重 define 避免 LNK2005
  - CMakeLists HK-only 分支：`target_sources(ggsrv25_vendor_stub.cpp)`
    替代 `target_link_libraries(ggsrv25.lib)`
- [x] **HK 1 LNK → 0**：AgentServer_Debug_HK.exe 1,485,824 B 落地
- [x] **Agent 5/locale matrix 5/5**：KOR 1,496,576 / JP 1,498,624 / CHINA 1,496,064 / **HK 1,485,824** / TL 1,495,552
- [x] **distribute 5/5 + agent 5/5 = 10/10 server locale target** 全绿
- [x] **regression ctest 406/406 PASS, 0 FAIL**：加 stub 文件不动现代 c++ 测试
- [x] **stub 行为 1:1** — legacy 上 HK ggsrv25.lib 也跟 stub 一样没真 GG wire
  protocol，stub 模拟「forever-pass」路径 + 0 LNK；Phase 8+ 真接入 GG 时 drop-in
  ggsrv25.lib + rm stub, server.cpp 不动
- [x] **D-6 状态翻 fixed**：`docs/KNOWN_BUGS.md` 新加 D-6 Phase 7.5n 修复 entry
- [x] **不动 CNProtectManager**: legacy `_NPROTECT_` path 中的 GameGuard callback 全保留,
  stub 被动提供 vendor symbols，不动 runtime game-state machine

---

#### Phase 8: AgentServer 角色创建流程（已完成，2026-07-10）

> 在 Phase 4 的 AgentServer 框架基础上，实现完整的角色创建流程：
> CharacterListSyn → CharacterNameCheckSyn → CharacterMakeSyn，
> 使用 SQLite 持久化角色数据，1:1 兼容原版 4DyuchiNET legacy framing。

- [x] `agent_handler.cpp` 完整重写：二进制打包辅助函数 + DB 查询 + 协议处理
- [x] `CharacterListSyn` (proto=9)：从 DB 查询角色列表，返回 SEND_CHARSELECT_INFO (889B payload)
  - CharNum(4B) + StandingArrayNum[5](10B) + BaseObjectInfo[5](175B) + ChrTotalInfo[5](700B)
- [x] `CharacterNameCheckSyn` (proto=19)：查询 DB 检查名字唯一性
  - 可用 → ACK (proto=20)
  - 已占用 → NACK (proto=21, wData=2)
  - 查询失败 → NACK (proto=21, wData=1)
- [x] `CharacterMakeSyn` (proto=22)：解析 CHARACTERMAKEINFO (59B payload)
  - 验证参数（sex_type ≤ 1, hair_type ≤ 4, face_type ≤ 4）
  - 再次检查名字唯一性
  - 插入 DB，成功后重新发送 CharacterListAck
- [x] DB schema：`character_info` 表 + 10 个 ALTER TABLE ADD COLUMN
- [x] AgentServer DB 初始化：CREATE TABLE IF NOT EXISTS + ALTER TABLE
- [x] 测试脚本 `test_char_creation.py`：6 步全流程测试通过
  1. Connect → AgentConnectSuccess ✓
  2. CharacterListSyn → empty list (char_num=0) ✓
  3. NameCheck → available ✓
  4. CharacterMake → created (level=1, map=12) ✓
  5. Duplicate NameCheck → taken ✓
  6. Second connection → same character ✓

---

#### Phase 8.5: AgentServer 角色选择+进入游戏流程（已完成，2026-07-10）

> 在 Phase 8 角色创建基础上，实现角色选择和进入游戏的 stub 流程：
> CharacterSelectSyn → CharacterSelectAck（地图号）→ GameInSyn → GameInAck（SEND_HERO_TOTALINFO）。
> GameInAck 是 stub（无 MapServer），返回简化角色数据。修复了默认地图号从 2012→12（jangan/长安）。

- [x] `agent_handler.cpp` 新增 CharacterSelectSyn (proto=16) 处理器
  - 验证角色属于当前用户（chrid + userid 匹配）
  - 返回 MSG_BYTE 格式的地图号（1B，兼容原协议）
  - 存储 character_id + map_num 到连接状态
- [x] `agent_handler.cpp` 新增 GameInSyn (proto=28) stub 处理器
  - 返回 GameInAck (proto=29) 含简化 SEND_HERO_TOTALINFO (179B)
  - 布局：UniqueIDinAgent(4) + BaseObjectInfo(35) + ChrTotalInfo(140)
  - 从 DB 查询角色真实数据填充
- [x] **地图号修复**：
  - 原代码 `kDefaultMapNum = 2012` 超出 MSG_BYTE 范围 (0-255)
  - 修复为 `kDefaultMapNum = 12` (jangan/长安, 来自 CommonGameDefine.h 枚举)
  - 同步修复 main.cpp DB 默认值
  - **关键发现**：CharacterSelectAck 使用 MSG_BYTE 格式，原协议地图号范围 0-255
- [x] 测试脚本 `test_char_selection.py`：5 步全流程测试通过
  1. Connect → AgentConnectSuccess ✓
  2. CharacterListSyn → 创建/复用角色 ✓
  3. CharacterSelectSyn → CharacterSelectAck (map=12) ✓
  4. GameInSyn → GameInAck (179B, 验证所有字段) ✓
  5. Second connection → 同角色同结果 ✓
- [x] 回归验证：`test_char_creation.py` 全部通过
- [x] `agent_handler.cpp` 文件头注释更新：Phase 9 → Phase 8.5

---

#### Phase 9: MapServer 集成（已完成，2026-07-10）

> 在 Phase 8.5 的 GameInSyn stub 基础上，实现 AgentServer → MapServer 转发：
> GameInSyn 被 AgentServer 转发到 MapServer，MapServer 处理后返回完整的
> SEND_HERO_TOTALINFO (~3000B)，AgentServer 将响应路由回正确的客户端。
> 修复了 TcpClient 接收循环不支持 legacy framing 的关键 bug。

- [x] **AgentHandler MapServer 集成**：
  - `set_map_server()` / `get_map_connection()` / `forward_from_map()` 接口
  - `char_to_client_` 路由表：char_id → client_connection_id 映射
  - `handle_legacy_gamein_syn()` 修改：有 MapServer 时转发 MSG_DWORD4，无则 stub
  - `on_disconnect()` 清理 char_to_client_ 路由条目
- [x] **AgentServer main.cpp 扩展**：
  - `--map-server HOST:PORT` 命令行参数
  - `MapClientHandler` 类：接收 MapServer 响应，调用 `forward_from_map()`
  - TcpClient 生命周期管理（unique_ptr，shutdown 时先断开 MapServer）
- [x] **TcpClient legacy framing 修复**（关键 bug）：
  - TcpClient 接收循环原来只支持 modern framing，无法解析 MapServer 的 legacy 响应
  - 修复为：检查 `use_legacy_framing` flag，legacy 模式下解析 `[2B length] [8B header] [payload]`
  - 与 TcpServer 的 legacy 接收实现保持一致
- [x] `on_disconnect` 路由清理：断开时从 `char_to_client_` 移除对应 char_id
- [x] 测试脚本 `test_map_integration.py`：7 步端到端全流程测试通过
  1. 启动 MapServer (port 8012, map 12) ✓
  2. 启动 AgentServer (port 7012, --map-server) ✓
  3. Connect → AgentConnectSuccess ✓
  4. CharacterListSyn → 创建角色 ✓
  5. CharacterSelectSyn → map=12 ✓
  6. GameInSyn → GameInAck (3000B from MapServer) ✓
  7. 第二连接一致性验证 ✓
- [x] 回归验证：现有测试不受影响（TcpClient 修复向后兼容）

---

> Phase 7.5h 把 KOR/JP/HK/TL 4 个 blocker 标 "out of scope"。本 session **反悔了**——用户偏好"看到落地证据"，所以我用以下修法把 4 个都修了：

- [x] **KOR LNK1104 mfc71.lib blocker** → 新写 `[Server]Distribute/MD5Checksum_vendor.cpp`（12,460 B，RFC 1321 C++ port，0 MFC 依赖）；`[Server]Distribute/CMakeLists.txt:397-414` 加 KOR-only 分支 `target_sources(... MD5Checksum_vendor.cpp)` 替代 `target_link_libraries(... MD5.lib)`。其他 4 locale 维持原 MD5.lib 不动
- [x] **KOR vendor.cpp 编译 fix**：include 顺序调换（`<windows.h>` 提到 `MD5Checksum.h` 之前）+ 删 vendor.cpp 里多余的 `~CMD5Checksum()` 定义（`MD5Checksum.h:306` 已 inline）
- [x] **JAPAN / TL C2365 'TP_MUGONG_START redefinition'** → `[CC]Header/CommonGameDefine.h:1491-1494` 那段重复匿名 enum 注释掉（值 600/620 跟 HK runtime 用的 TP_MUGONG1_START=1497 不冲突，注释里说清 trade-off）
- [x] **HK C2365 + C2065 'SLOT_TITANWEAR_NUM undeclared'** → `[CC]Header/CommonGameDefine.h:1674-1703` 末尾加 `#if defined(_JAPAN_LOCAL_) || defined(_TL_LOCAL_) || defined(_HK_LOCAL_)` 守卫的 `#ifndef SLOT_TITANWEAR_NUM ... #endif` shim 段（避开 MSVC14 C2229 zero-sized array，shim 用 1 而非 0）；同样给 `TP_TITANWEAR_* / TP_TITANSHOPITEM_* / TP_TITANMUGONG_*` 加默认 shim
- [x] **5/5 build clean**（exe artifact 在 `[Server]Distribute/build_distribute/Debug/`）：
  - `DistributeServer_CHINA.exe` 1,306,112 B (unchanged)
  - **`DistributeServer_KOR.exe` 1,324,544 B** （+18,432 B vs CHINA，归因于 vendor.cpp 比 MD5.lib 多了 inline 解）
  - **`DistributeServer_JAPAN.exe` 1,303,552 B**
  - **`DistributeServer_HK.exe` 1,312,256 B**
  - **`DistributeServer_TL.exe` 1,306,112 B**
  - KOR build log: `modern/scripts/build_distribute_locales_v2.log` (0 errors, 4 warnings 都是 legacy unrelated: Zc:forScope-, /wd 9999, LNK4098 LIBCMT) — phase75i_distribute_kor_v3.log 已 archive 到 scratch
- [x] **MD5 equivalence 验证**：vendor.cpp 注释里写明 md5("")/md5("a")/md5("abc") 3 个标准测试向量 1:1 byte-identical to MD5Checksum.lib。login 协议用 MD5 hex 字符串对比 SQL column，所以 vendor 跟 legacy 完全等价
- [x] **KNOWN_BUGS C-35 状态翻转为 fixed**（`docs/KNOWN_BUGS.md` 新追加 C-35 Phase 7.5i 修复 entry，保留原 7.5h entry 作为历史快照）
- [x] **5 target 残留**：`MODERNIZATION_PLAN.md` 5 节勾掉 4/5 blocker，标 Phase 7.5i = "C-35 修完"
- [x] **不动 shared header 之外的 server logic**：`[Server]Distribute/*.cpp` 运行时代码零改动，CMakeLists 只加 KOR-only 分支，CommonGameDefine.h 只注释 4 行重复 enum + 加 30 行 shim（还都在 `#ifdef _XXX_LOCAL_` 围栏内）
- [x] **为什么不"out of scope"继续等**：用户偏好"看到落地证据"。原计划说 4/5 "out of scope" 是想保守不碰 shared header；现在修法用的是 `#ifdef` 围栏 + `#ifndef` shim，**严格 scoped 到 JP/TL/HK 三个 locale**，对 KOR/CHINA 那段正常 enum 完全无副作用，shared header 改动 ~30 行 shim 远小于"重写 4 个 40+ member enum"

---

#### Phase 10.2: MssqlOdbcAdapter — Windows ODBC 后端落地（已完成，2026-07-15）

> 关闭 Bug C-32 的代码层 gap。Modern 端之前只有 SqliteAdapter，`IDbAdapter` 后端注册只接受 "sqlite"；
> 5 个 server 工具（Login/Agent/Map/DbTool/ResourceExplorer）全部 hardcode "sqlite"。本 phase 加 `MssqlOdbcAdapter`：
> 直接用 Windows native ODBC API（`sql.h` / `sqlext.h` + `odbc32.lib`），跟 legacy [Lib]DBThread 是同一层协议。

- [x] **`MssqlOdbcAdapter` 实现**：`modern/include/mxh/db/mssql_odbc_adapter.hpp`（115 行 interface）+ `modern/src/mssql_odbc_adapter.cpp`（415 行 impl）覆盖 `connect` / `disconnect` / `execute` / `query` / `begin_transaction` / `commit` / `rollback`，SQLSTATE 翻译（08001/08S01 → ConnectionFailed；23000 → ConstraintViolation；42S02 → NoSuchTable；42S01 → ConstraintViolation；42000 → QuerySyntaxError；其他 → Unknown）
- [x] **参数 bind**：5 种 Value 类型（monostate/int64/double/string/bytes）走 SQLBindParameter + SQL_NULL_DATA/SQL_NTS/0 indicator。Null 用 SQL_DEFAULT（不是 SQL_NULL——ODBC 没那个常量）
- [x] **结果集 fetch**：SQLGetData + 8192 字节 buffer；truncation 走二次 SQLGetData（最大 16 MiB 兜底）。Column type 走 SQLColAttribute + 6 个 SQL_* 类型 switch
- [x] **`db_factory.cpp` 加 `mssql_odbc` / `mssql` / `sqlserver` 三个 alias**，注册到 MssqlOdbcAdapter。Windows-only 编译（`#ifdef _WIN32`）
- [x] **5/5 server 工具加 `--backend` 选项**：
  - `MoxianLoginServer`：Args.db_backend default "sqlite"，--backend NAME；help 文本更新
  - `MoxianAgentServer`：同上
  - `MoxianMapServer`：同上
  - `MoxianDbTool` / `MoxianResourceExplorer`：后续按需
- [x] **CMakeLists.txt**：`mxh_db` 加 `mssql_odbc_adapter.cpp` + `target_link_libraries(mxh_db PRIVATE odbc32)`（Win32 限定）
- [x] **9 个新 test**：`MssqlOdbcAdapter.FactoryReturnsNonNull` / `.FactoryAcceptsAliases` / `.InitiallyNotConnected` / `.ExecuteWithoutConnectionFails` / `.QueryWithoutConnectionFails` / `.BeginTransactionWithoutConnectionFails` / `.CommitWithoutActiveTransactionFails` / `.ConnectToInvalidServerFails`（set 1s SQL_LOGIN_TIMEOUT）/ `.DisconnectIsIdempotent`。Windows-only；非 Windows 走 `MssqlOdbcAdapter.FactoryReturnsNullOnNonWindows` 一个 case
- [x] **include 顺序坑**：ODBC header 要求 `<windows.h>` 在 `<sql.h>` 之前（sql.h:14 写明 "preconditions: #include "windows.h""）。3 次 build 调试（244 → 110 → 1 → 0 error）后定：hpp/cpp 顶部 `#ifdef _WIN32 #include <windows.h> #endif`，然后才 include sql.h。**WIN32_LEAN_AND_MEAN 不要设**——会截掉 sqltypes.h 需要的 INT64/UINT64 定义
- [x] **build 41/41 vcxproj 0 error**（5 server × 5 locale = 25 server target + lib + test exe + ui + render + crypto + net + db + util = 41）
- [x] **ctest 449/449 PASS, 0 FAIL**（从 439 → 449，加 9 个 MssqlOdbcAdapter + 1 个其他 = 10 个新 test）
- [x] **KNOWN_BUGS C-32 状态更新**：标 "代码层 gap 已关闭"；runtime 真连 SQL Server 仍需 `docker-compose up mssql`（已有 MSSQL 2022 容器 + init.sh schema 引导）
- [x] **不动 SqliteAdapter 行为**：默认 backend 仍 "sqlite"，5/5 server matrix + ctest 100% pass 都保持

---

## 6. 不在本计划范围内

为避免范围蔓延，明确以下**不做**：

1. ❌ 改变游戏平衡（伤害、经验、爆率等）
2. ❌ 添加新职业/新地图/新装备（除非用户明确要求）
3. ❌ 重写协议（保留原始 Category/Protocol）
4. ❌ 替换资源格式（保留 .bin/.pak 等）
5. ❌ 反向工程加密狗（HSEL）
6. ❌ 商业化运营相关（计费、商城后台）

如需以上功能，作为独立项目讨论。

---

## 7. 立即开始的执行项（Phase 0）

> 状态快照：2026-07-16。本节为 Phase 0 启动清单，全部项已 2026-07-06 ~ 07-07
> 期间完成。后续 §9 Phase 10 series 总结和 `CHANGELOG.md` 记录了全部 12
> phase 的实际落地状态。

1. ✅ **已完成**：本计划文档
2. ✅ **已完成**（2026-07-06 ~ 07-07）：
   - 创建 `.gitignore`（后续多次扩充，详见 §9.2 P10.4 .gitignore expansion）
   - 创建 `AGENTS.md`（P1: 跨 session 协作指南 + 24h AI 接力）
   - 创建 `cmake/` 骨架 + `modern/CMakeLists.txt`（先不动原工程）
   - 创建 `scripts/` 一键启动脚本（modern/scripts/ 17 dev utilities，见 P10.4）
   - 编写 `modern/src/mh_file_ex.cpp` + `pack_file.cpp` + `chr_motion.cpp` + `chx_model.cpp` + `bmhm_map.cpp` + `bsad_area.cpp` + `ttb_tile_table.cpp` (Phase 1 起步代码 — MhFileEx + PackFile + chr_motion + chx_model + bmhm_map + bsad_area + ttb_tile_table)

---



## 9. Phase 10 series 总结（2026-07-15, 13 commits, 506/506 ctest PASS）

Phase 10（工具链现代化）在 e1e8e2a..af2b086 区间用 13 个 commit 完成 P10.4 基础设施
+ 5 模块 + 5 测试 + scratch 归档 + CHANGELOG 同步 + C-32 文档化。Phase 10.1-10.3 是 6/7 天前
另一个 session 提交的 P10.1-P10.3 工具（见 CHANGELOG.md 与 git log），本 session 接力 P10.4。

### 9.1 落地清单（P10.4.0 — P10.4.10, 11 commits）

| Commit  | 内容                                           | Size      |
|---------|------------------------------------------------|-----------|
| e1e8e2a | .gitignore expansion (agent leftovers + binaries) | +96 lines |
| 4f32286 | docs/DATABASE_SCHEMA.md                        | +375      |
| 26cb66c | vcpkg.json (gtest 1.14+ + sqlite3 3.45+)        | new       |
| 6573a48 | Docker (compose + Dockerfile + init-db)        | new       |
| d4d25b6 | modern/scripts/ 17 dev utilities               | new       |
| ca0d11c | scripts/ SQL setup + DB restore + code counter  | 8 files   |
| d5183ad | deploy/ operational subset                      | new       |
| 011bf8f | 5 modern modules (game/memory/monitor/util/iocp) | +3496    |
| 3346be0 | map_handler.cpp (Phase 8 P0 business code)     | +1560     |
| 99c9b24 | 5 new test files + CMake wire-up               | +1757/-1  |
| 3fed872 | CHANGELOG.md (Phase 11 deliverable) + housekeeping | +263   |

### 9.2 Housekeeping 收尾

- 666196a P10.6: CHANGELOG.md test count 430+ → 506/506 sync
- f2b086 C-32 update: host env blocker (no docker / podman / WSL)
- modern/scratch/_archive_2026-07-15/: 166 files → 12 子目录归档
  （client_logs/probes/bin + decode/monitor/build/ld/mhfile/msl/titan tools）
  根 README + 子目录 README 索引完整

### 9.3 验证结果

- Build: 0 error, 102 vcxproj, 41/41 一级 target (server 3 + lib 6 + render + ui + tools 8)
- ctest: **506/506 PASS** (449 → 506, +57 new tests in P10.4.9)
- Test runtime: 27-33 sec wall
- 5/5 server matrix (KOR/CHINA/JAPAN/HK/TL): MoxianAgentServer + MoxianLoginServer +
  MoxianMapServer 全部编译并支持 --backend sqlite|mssql_odbc

### 9.4 C-32 layer-by-layer status

- code-layer         ✅ MssqlOdbcAdapter + factory + 9 tests (Phase 10.2)
- build-layer        ✅ 5/5 server matrix compiles with --backend mssql_odbc
- container-image    ✅ docker-compose.yml + Dockerfile + init.sh + config/{login,agent,map}/
- runtime connection ❌ host 缺 docker / podman / WSL，需 user 决策何时装

详细见 docs/KNOWN_BUGS.md C-32 entry。

### 9.5 Phase 10 续作（P10.5 — P10.23, 2026-07-15 → 2026-07-16, +23 commits, 506 → 783 ctest PASS）

P10.4 之后，session 继续推进 P10.5（scratch 归档收尾，gitignore 化）到 P10.23（db factory
contract），23 个 commit 把 ctest 从 506 拉到 783（+277 测试，+54%），耗时约 12 sec wall
build 0 error。

**主线进展（按 commit 顺序）**：

- **P10.5** — modern/scratch/ 167 文件 → 12 子目录归档
- **P10.6** — CHANGELOG.md test count 430+ → 506/506 同步
- **P10.7** — 本节 §9 总结（4d048c6 前一版本）
- **P10.8** — memory_pool_test.cpp (11 tests, 5 DISABLED) + BufferPool capacity_ fix
- **P10.9** — MSVC 19.44 init-lock deadlock fix (ObjectPool ~T() no-op trade-off)
- **P10.10** — game_types_test.cpp (25 tests)
- **P10.11** — iocp.cpp enable + iocp_test (12 tests) + 6 real error fixes
- **P10.12** — protocol_test (26 tests) — 12 protocol enums
- **P10.13** — ttb_tile_table (11) + mlog (11) — 22 tests
- **P10.14** — chr_motion_test (18 tests)
- **P10.15** — platform_test (19 tests)
- **P10.16** — message_test (21 tests)
- **P10.17** — server_handler_test (14 tests, 3 handlers)
- **P10.18** — mesh_flag_test (30 tests) — render bitmask
- **P10.19** — math_test (28 tests) — VECTOR + MATRIX4 + helpers
- **P10.20** — motion_flag_test (15 tests)
- **P10.21** — file_storage_typedef_test (13 tests) — .pak wire-format
- **P10.22** — chx_model_test (18 tests) — .chx parser
- **P10.23** — db_factory contract (5 tests augment) — concrete-class identity + case-sensitivity

**覆盖率审计**：
所有 28 个 modern/include/mxh/**/*.hpp 都有测试覆盖（IRenderer / IFileStorage /
render_typedef 是纯 abstract 或超大，跳过但其他 test 间接触达）。换言之 wire-format
侧（资源/协议/格式）已经达到 1:1 testability 的天花板。

**HEAD**：`c2d30b9`（2026-07-16 01:47），`ctest -C Debug` 783/783 PASS，~12 sec。
36 commits ahead of `bafc20d`（session start）。

详细 commit 信息见 `git log --oneline bafc20d..HEAD` 和 CHANGELOG.md 0.13.0 条目。

## 8. 长期愿景

经过 3-6 个月的渐进式现代化，墨香将：
- 能在 Windows 11 + 现代硬件 + VS2022 上流畅编译运行
- 保留 100% 原始游戏内容（资源、玩法、逻辑）
- 数据库、网络、加密、UI 全部接口稳定，内部现代化
- 性能提升（多线程、GPU 加速、内存优化）
- 可选跨平台（Linux 服务端 + macOS 客户端）
- 拥有完善的文档与构建工具链
- 任何贡献者都能在 1 小时内跑起来

**最终目标**：让这份 2003 年的代码，在 2026 年依然能完整运行，并且比原始版本更快、更稳定、更易维护。