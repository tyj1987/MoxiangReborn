# Moxian-Reborn (墨香 / DarkStory)

> 一个 2003-2010 年韩国 2D MMORPG 的现代化改造项目。
> **目标**：在现代软硬件环境下**完整复刻**游戏（资源、玩法、逻辑 1:1 保留），同时替换掉所有过时的技术栈。

---

## 项目状态

**当前阶段**：Phase 0-1（准备与资源兼容层）

```
✓ 项目分析与计划      MODERNIZATION_PLAN.md
✓ 现代化代码骨架      modern/
✓ 资源兼容层          modern/src/ (.bin / .pak / .bmhm / .ttb / .chx / .chr / .bsad)
✓ 资源浏览器 CLI      modern/tools/MoxianResourceExplorer/
✓ 单元测试            modern/tests/
✓ 启动脚本            scripts/
- 编译验证            待执行
- 真实资源 1:1 读取    待验证
```

---

## 目录速览

| 路径 | 内容 |
|------|------|
| `MODERNIZATION_PLAN.md` | **必读** - 12 阶段路线图 + 关键技术选型 |
| `AGENTS.md` | AI 助手指南（项目结构 + 陷阱清单） |
| `.gitignore` | 过滤 .ncb/.opt/.vsscc 等历史遗留 |
| `modern/` | 现代化 C++17 代码（CMake 工程） |
| `modern/src/` | 资源兼容层实现 |
| `modern/include/mxh/compat/` | 公共头 |
| `modern/tests/` | GoogleTest 单元测试 |
| `modern/tools/MoxianResourceExplorer/` | CLI 资源浏览器 |
| `scripts/` | PowerShell 一键启动 + 初始化 |
| `docs/` | 资源格式、协议、数据库、Bug 文档 |
| `墨香【源码】/` | **原始源码**（保持不变） |
| `墨香【客户端+服务端+工具】/` | **原始部署包**（客户端 1.3GB + 服务端 29MB + 工具） |
| `墨香【教程】/` | 4 篇中文教程（架设/数据库/源码修改） |
| `墨香【源码配套资源】/` | 完整游戏资源（PlayDH\Resource\） |

---

## 快速开始

### 1. 查看计划

```bash
# 阅读核心文档
notepad MODERNIZATION_PLAN.md
notepad AGENTS.md
```

### 2. 构建现代代码（推荐 VS 2022）

```powershell
# 在 PowerShell 中：
cd D:\墨香全套源代码（源码+资源+客户端+服务端+教程）
.\scripts\setup-modern.ps1
```

脚本会自动：
- 检测 VS 2022 / CMake
- 配置 CMake 工程
- 构建 MoxianCompat 静态库
- 构建 MoxianResourceExplorer CLI
- 运行单元测试

### 3. 用资源浏览器查看真实游戏资源

```powershell
# 解密 ItemList.bin 查看元信息
.\modern\build\Release\mxh_explorer.exe info "墨香【源码配套资源】\PlayDH\Resource\ItemList.bin"

# 列出 Effect.pak 中的所有文件
.\modern\build\Release\mxh_explorer.exe list "墨香【源码配套资源】\PlayDH\Resource\Effect.pak"

# 查看 9x9 技能区域
.\modern\build\Release\mxh_explorer.exe bsad "墨香【源码配套资源】\PlayDH\Resource\SkillArea\9x9_Blank.bsad"

# 查看 Map22 地图头
.\modern\build\Release\mxh_explorer.exe map "墨香【源码配套资源】\PlayDH\Resource\Map\Map22.bmhm"
```

### 4. 启动原版服务端

```powershell
# 启动（Monitoring → Distribute → Agent）
.\scripts\start-server.ps1 -Mode start

# 状态
.\scripts\start-server.ps1 -Mode status

# 停止
.\scripts\start-server.ps1 -Mode stop
```

---

## 关键技术决策

| 维度 | 选型 | 理由 |
|------|------|------|
| 编译器 | MSVC 2022 + Clang 17 | 兼容老代码 |
| C++ 标准 | C++17/20 | 现代特性 |
| 构建系统 | CMake 3.25+ | 工业标准 |
| 渲染 | DX11（保留 DX8 接口） | 兼容性 |
| 网络 | Boost.Asio（保留 I4DyuchiNET） | 跨平台 |
| 数据库 | MSSQL（保留 ODBC）+ IDbAdapter | 1:1 兼容 |
| 加密 | OpenSSL AES-256-GCM（替代 HSEL） | 现代安全 |
| 测试 | GoogleTest 1.14 | 工业标准 |

详细见 `MODERNIZATION_PLAN.md` 第 2 节。

---

## 项目代码量

| 维度 | 数据 |
|------|------|
| 客户端代码 | ~930 文件，35-40 万行（保留原样） |
| 服务端代码 | ~412 文件，50-70 万行（保留原样） |
| 引擎代码 | ~5-10 万行（保留原样） |
| **现代代码** | ~1500 行（持续增长） |
| 资源 | 1.3 GB 客户端 + 29 MB 服务端 |

**策略**：**不重写老代码**——只在其外围加抽象层、做兼容、修复 bug、提供新工具。让老代码继续服务 10-20 年。

---

## 重要文档链接

- 📋 [MODERNIZATION_PLAN.md](./MODERNIZATION_PLAN.md) - 完整计划
- 🤖 [AGENTS.md](./AGENTS.md) - AI 协作指南
- 📚 [docs/RESOURCE_FORMATS.md](./docs/RESOURCE_FORMATS.md) - 资源格式逆向文档
- ⚠️ [docs/KNOWN_BUGS.md](./docs/KNOWN_BUGS.md) - 已知 bug 与陷阱

---

## 致谢

- **eSofnet / Yedang Entertainment** —— 墨香原开发者
- **Park Jung Hwan (ilil5@kornet.net)** —— [Tool]Regen/DefineStruct.h 作者
- **"墨香研发"** —— 中文本地化与维护
- **教程作者** —— 4 篇中文搭建文档

---

## 许可

代码仅供学习研究使用，请勿用于商业目的。
原游戏版权归原作者所有。

**Moxian-Reborn** - 让 2003 年的代码在 2026 年依然运行。