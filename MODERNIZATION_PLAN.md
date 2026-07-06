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

Phase 4: 网络层现代化 (3-4 周)
  ├─ 4.1 抽取 I4DyuchiNET 抽象接口（保留 IOCP 语义）
  ├─ 4.2 Asio/Boost.Asio 实现（Linux-ready）
  ├─ 4.3 协议版本化（消息包加 version 字段）
  └─ 4.4 加密/压缩/校验中间件

Phase 5: 渲染引擎现代化 (4-6 周)
  ├─ 5.1 抽取 IRenderer/IGeometry/IExecutive 抽象接口
  ├─ 5.2 DX11/DX12 后端实现（保留原 4Dyuchi 接口签名）
  ├─ 5.3 模型/动画复用：.chl/.anm → 自研二进制保持
  └─ 5.4 字体/UI 资源路径保持

Phase 6: UI 系统现代化 (3-4 周)
  ├─ 6.1 cWindow 控件 → 现代 GUI（保留二进制 .bin UI 描述）
  ├─ 6.2 选项：保留自研 cWindow（DX11 后端）/ 替换为 ImGui
  └─ 6.3 多语言/i18n 抽离（消灭 _KOR_LOCAL_ 等宏）

Phase 7: 构建系统完全化 (2 周)
  ├─ 7.1 全工程迁移到 CMake / Premake
  ├─ 7.2 依赖管理 vcpkg/Conan
  └─ 7.3 CI/CD（GitHub Actions / GitLab CI）

Phase 8: 性能优化 (持续)
  ├─ 8.1 多线程：渲染线程 / 网络线程 / 逻辑线程
  ├─ 8.2 内存：自定义分配器 / 对象池
  ├─ 8.3 协议：批量压缩 / 增量同步
  └─ 8.4 性能剖析与回归测试

Phase 9: 可选 - 跨平台 (4-8 周)
  ├─ 9.1 Linux 服务端（先 Distribute/Agent，Map 涉及 DX 跳过）
  ├─ 9.2 macOS 客户端（Metal 后端）
  └─ 9.3 容器化部署（Docker）

Phase 10: 工具链现代化 (2-3 周)
  ├─ 10.1 PackingTool.exe → Web/CLI（C# Avalonia / Rust egui）
  ├─ 10.2 GMTOOL → Web 后台（FastAPI/Blazor）
  ├─ 10.3 地图编辑器 → 保留 4DyuchiGXMapEditor 但支持现代 SDK
  └─ 10.4 AutoPatchTool → HTTPS + bsdiff

Phase 11: 文档与社区 (持续)
  ├─ 11.1 AGENTS.md / README.md / CHANGELOG.md
  ├─ 11.2 协议文档（自动生成）
  ├─ 11.3 开发者指南 / 部署指南
  └─ 11.4 测试用例

Phase 12: 持续迭代
  └─ 12.1 反馈收集 / bug 修复 / 性能调优
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
- [ ] `.gitignore`（过滤旧工程文件）
- [ ] `AGENTS.md`（AI 助手指南）
- [ ] `cmake_minimum.txt` 现代工程骨架
- [ ] `scripts/start-server.ps1` 一键启动脚本

### Phase 1 交付物
- [ ] `modern/MoxianCompat` 静态库（CMHFileEx + PackFile 重写）
- [ ] `tools/MoxianResourceExplorer` 命令行资源浏览器
- [ ] 单元测试：所有 `.bin/.pak/.bmhm/.bsad` 读取验证
- [ ] 文档：`docs/RESOURCE_FORMATS.md`

### Phase 2 交付物
- [ ] `modern/MoxianDb` IDbAdapter 接口
- [ ] MSSQL + PostgreSQL 双实现
- [ ] `tools/MoxianSchemaExporter` schema 导出
- [ ] 文档：`docs/DATABASE_SCHEMA.md`

### Phase 5 交付物（DX11 渲染器）
- [x] `modern/src/render/mxh_render` 静态库 — DX11 后端
- [x] `modern/include/mxh/render/IRenderer.hpp` — 1:1 端口（75 个方法签名）
- [x] `modern/include/mxh/render/IFileStorage.hpp` — 1:1 端口（27 个方法签名）
- [x] `modern/include/mxh/render/render_typedef.hpp` — 与原 DX8 引擎二进制兼容的结构体
- [x] Device 初始化（SwapChain / RenderTarget / 默认状态对象）
- [x] PrimitiveDrawer（RenderBox/Line/Point/Circle 的 DX11 实现）
- [x] SpriteObject（ID3D11Texture2D + SRV，Draw + Resize + LockRect）
- [x] TGA 解码器（uncompressed + RLE，含 bottom-up 翻转）
- [x] `tools/MoxianRenderDemo` — 烟雾测试（窗口 + 清屏 + RenderBox）
- [x] TGA 解码器单元测试 — 7 个 case（truncated/24bpp-topdown/24bpp-bottomup-flip/32bpp-alpha/auto-detect/invalid-type/RLE-pack-unpack）全过
- [ ] Phase 5 进度报告（剩余 stub 清单：MeshObject / Font / HeightField / Effect 等）
- [ ] HLSL shader 编译 + cache 策略

### Phase 3-7 交付物（每阶段）
- [ ] 新模块源码 + 单元测试
- [ ] 回归测试：与原模块行为对比
- [ ] 更新文档
- [ ] CMakeLists.txt 集成

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

1. ✅ **已完成**：本计划文档
2. ⏳ **进行中**：
   - 创建 `.gitignore`
   - 创建 `AGENTS.md`
   - 创建 `cmake/` 骨架（先不动原工程）
   - 创建 `scripts/` 一键启动脚本
   - 编写 `modern/MoxianCompat` Phase 1 起步代码

---

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