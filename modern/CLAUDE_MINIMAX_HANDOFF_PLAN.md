# Claude ↔ MiniMax 接力开发计划

> **目标**：让两个 AI 可以在同一个项目上无缝接力，各自发挥优势。
> **原则**：每个 Phase 有明确的"入口状态"和"出口交付物"，入口=出口，不依赖隐式上下文。

---

## 当前基线（2026-07-07）

```
已完成的 Phase:  0, 1, 2, 5, 7.0, 7.1, 7.2, 7.3(Agent), 7.4a, 7.5
进行中:          无
未开始:          3, 4, 6, 7.5b(Tools), 7.6(MurimNet+Monitoring), 8, 9, 10
```

| 维度 | 状态 |
|------|------|
| 资源兼容层 (Phase 1) | ✅ `.bin` / `.pak` / `.bmhm` / `.ttb` / `.bsad` / `.chx` 全部可读 |
| 数据库层 (Phase 2) | ✅ IDbAdapter + MSSQL + SQLite 双后端 |
| DX11 渲染器 (Phase 5) | ✅ 75/75 接口 + 99/99 测试通过 |
| 遗留库 CMake 迁移 (Phase 7) | ✅ HSEL / YHLibrary / BaseNetwork / DBThread / FileStorage / 4DyuchiNET / Distribute / **Agent** / **Map** |
| 加密替换 (Phase 3) | ❌ 未开始 |
| 网络层现代化 (Phase 4) | ❌ 仅 stub |
| 客户端构建 (Phase 6) | ❌ 未开始 |
| Agent 服务端 (Phase 7.3) | ✅ **刚完成** |
| 工具链迁移 (Phase 7.5b) | ❌ 未开始 |

---

## 接力规则（两个 AI 都遵守）

### 1. 每次开始新工作前
- 读 `CLAUDE_MINIMAX_HANDOFF_PLAN.md`（本文件）确认当前 Phase
- 读 `AGENTS.md` 了解项目约束
- 读对应 Phase 的详细文档（如 `modern/docs/phase7.x_*.md`）
- 检查 `git log --oneline -5` 确认最新提交

### 2. 每次完成工作后
- **必须更新本文件**：勾选完成的子任务、更新状态表
- **必须更新 `MODERNIZATION_PLAN.md`** 第 5 节交付物清单
- **Git commit message 规范**：`Phase X.Y: <what> (<category>)`
- **每个 Phase 必须有至少一个可验证的产物**（编译通过 / 测试通过 / 二进制对比一致）

### 3. 互不破坏的边界
- **`modern/` 目录**：新代码在这里，C++17/20，CMake 构建
- **`墨香【源码】/`**：遗留代码在这里，只能加 `CMakeLists.txt` + 改 `StdAfx.h`（去 MFC），不能改业务逻辑
- **`SWorking/`**：基准二进制，只读，用于字节对比验证
- **协议头文件**：`[CC]Header/Protocol.h` / `CommonStruct.h` / `CommonGameDefine.h` **绝对不改字段**

### 4. 上下文传递格式
在每个 Phase 的出口，写入以下信息到本文件：
```markdown
### Phase X.Y 出口状态
- **最后提交**: `<commit hash>`
- **验证方法**: `<命令或脚本路径>`
- **已知问题**: `<C-N bug IDs 或描述>`
- **给下一个 AI 的备注**: `<任何非显而易见的注意事项>`
```

---

## Phase 优先级排序（推荐接力顺序）

按**依赖关系**和**风险**排序：

```
Phase 7.5 Gate ──→ Phase 7.3 (Agent) ──→ Phase 7.5b (Tools) ──→ Phase 3 (Crypto)
                                                                    │
                                                                    ▼
                                            Phase 8 (集成测试) ←── Phase 4 (Network)
                                                                    │
                                                                    ▼
                                            Phase 6 (客户端)  ←── Phase 9 (跨平台)
```

---

## Phase 3 — 加密层现代化

**优先级**: ⭐⭐⭐ (中)
**依赖**: Phase 7.0 (HSEL.lib 可编译)
**预计工时**: 15-25 小时

### 目标
用 OpenSSL AES-256-GCM 替换 HSEL 加密狗，保持与旧客户端的协议兼容。

### 入口状态
- `[Lib]HSEL/CMakeLists.txt` 已存在，HSEL.lib 可编译 (26,770 bytes)
- `modern/include/mxh/crypto/crypto.hpp` 已声明接口
- `modern/src/crypto/crypto.cpp` 仅有 stub
- HSEL 原算法已逆向：XOR + 位移 + CRC

### 子任务

| # | 任务 | 描述 | 验证 |
|---|------|------|------|
| 3.1 | HSEL 算法完整逆向 | 读 `[Lib]HSEL/HSEL_STREAM.cpp` 全部逻辑，写文档 | 文档 `docs/HSEL_ALGORITHM.md` |
| 3.2 | `mxh::crypto::HselStream` 兼容实现 | C++17 重写，与 HSEL_STREAM 字节级一致 | 单元测试：100+ 随机输入 vs 原 HSEL.lib |
| 3.3 | OpenSSL AES-256-GCM 新通道 | 新 `mxh::crypto::AesStream` 类 | 单元测试：加解密回环 |
| 3.4 | 协议协商层 | 客户端连接时协商加密方式（HSEL 兼容 / AES 新） | 集成测试：新旧客户端同时可登录 |
| 3.5 | 性能对比 | HSEL vs AES 吞吐量 benchmark | benchmark 报告 |

### 出口交付物
- [ ] `modern/src/crypto/hsel_stream.cpp` — 兼容实现
- [ ] `modern/src/crypto/aes_stream.cpp` — 新实现
- [ ] `modern/tests/unit/crypto/` — 完整测试套件
- [ ] `docs/HSEL_ALGORITHM.md` — 算法文档

---

## Phase 4 — 网络层现代化

**优先级**: ⭐⭐⭐⭐ (高)
**依赖**: Phase 7.2 (4DyuchiNET.dll 可编译)
**预计工时**: 30-50 小时

### 目标
用 Boost.Asio 重写 4DyuchiNET（IOCP 网络库），保持 `I4DyuchiNET` 接口签名不变。

### 入口状态
- `4DyuchiNET_Latest/CMakeLists.txt` 已存在，4DyuchiNET.dll 可编译 (150,016 bytes)
- `modern/include/mxh/net/net.hpp` 已声明接口
- `modern/src/net/net.cpp` 仅有 stub
- 已知 Bug: C-19 到 C-26（接口漂移、死代码、odbc 残留等）

### 子任务

| # | 任务 | 描述 | 验证 |
|---|------|------|------|
| 4.1 | I4DyuchiNET 接口文档化 | 读所有虚函数签名，写 IDL 级文档 | `docs/I4DYUCHINET_INTERFACE.md` |
| 4.2 | `mxh::net::TcpServer` | Asio 实现 `IConnectionHandler` + `IAcceptHandler` | 单元测试：多连接 echo 服务 |
| 4.3 | `mxh::net::TcpClient` | Asio 实现客户端连接池 | 单元测试：重连/超时/并发 |
| 4.4 | IOCP 兼容适配层 | 包装 Asio 为 `I4DyuchiNET` 兼容接口 | ABI 对比：导出函数签名一致 |
| 4.5 | 服务端集成 | 替换 Distribute/Agent/Map 中的 4DyuchiNET.dll | 服务端启动冒烟测试 |
| 4.6 | 性能对比 | Asio vs 原 IOCP 吞吐量 benchmark | benchmark 报告 |

### 出口交付物
- [ ] `modern/src/net/tcp_server.cpp` + `tcp_client.cpp`
- [ ] `modern/include/mxh/net/` — 完整公开头文件
- [ ] `modern/tests/unit/net/` — 完整测试套件
- [ ] `docs/I4DYUCHINET_INTERFACE.md` — 接口文档
- [ ] Distribute/Agent 服务端使用新网络层启动成功

---

## Phase 6 — 客户端现代化构建

**优先级**: ⭐⭐⭐⭐⭐ (最高，但最复杂)
**依赖**: Phase 5 (DX11 渲染器), Phase 3 (加密), Phase 4 (网络)
**预计工时**: 80-150 小时

### 目标
让 `[Client]MH/` (~930 文件, ~35-40 万行) 在现代 MSVC 2022 下编译通过并运行。

### 入口状态
- `MHClient-Connect.exe` 存在于 `cworking/`（旧版可运行）
- DX11 渲染器 75/75 接口就绪
- 客户端依赖：MFC + DX8.1 + HSEL + HackShield + nProtect + Miles Sound + FreeImage

### 子任务

| # | 任务 | 描述 | 验证 |
|---|------|------|------|
| 6.1 | 客户端依赖清单 | 枚举所有 `#include` / `#pragma comment(lib)` | `docs/CLIENT_DEPENDENCIES.md` |
| 6.2 | MFC 去耦合分析 | 分析 cWindow 自研 UI 框架与 MFC 的边界 | `docs/MFC_DECOUPLING.md` |
| 6.3 | 音频替换 (Miles → OpenAL Soft) | 封装 `mxh::audio::AudioEngine` | 单元测试：播放 wav/mp3 |
| 6.4 | 图片库替换 (FreeImage → DirectXTex/stb) | 封装 `mxh::image::ImageLoader` | 单元测试：TGA/BMP/JPG/DDS 解码 |
| 6.5 | HackShield / nProtect 移除 | 从源码中删除 HS/nP 调用，或 stub 掉 | 编译通过 |
| 6.6 | CMakeLists.txt (客户端) | 930 文件的 CMake 工程，多 locale 配置 | `cmake --build` 0 error |
| 6.7 | DX8 → DX11 渲染接口适配 | 客户端调用从 `ID3D8Device` 切到 `I4DyuchiGXRenderer` | 客户端启动并渲染首帧 |
| 6.8 | 客户端冒烟测试 | 启动→登录→进地图→移动→打怪 全流程 | 自动化测试脚本 |

### 出口交付物
- [ ] `[Client]MH/CMakeLists.txt` — 可编译的客户端工程
- [ ] `modern/src/audio/` — OpenAL 音频后端
- [ ] `modern/src/image/` — 图片解码后端
- [ ] `modern/tests/unit/client/` — 客户端核心逻辑测试
- [ ] `MHClient-Modern.exe` — 可运行的现代化客户端

---

## Phase 7 — 遗留代码 CMake 迁移（剩余部分）

**优先级**: ⭐⭐⭐⭐ (高)
**依赖**: Phase 7 已有成果
**预计工时**: 40-80 小时

### 7.3 — [Server]Agent 迁移

**详见**: `modern/docs/phase7.3_readiness.md` (Agent 章节)

| # | 任务 | 描述 |
|---|------|------|
| 7.3.1 | `[Server]Agent/CMakeLists.txt` | 24 cpp, ~400-600 行 CMake |
| 7.3.2 | `StdAfx.h` MFC 去残留 | 与 Distribute 同模板 |
| 7.3.3 | `build_agent.py` | 构建脚本 |
| 7.3.4 | 冒烟测试 | 27 个协议 category 的链接验证 |
| 7.3.5 | Gate 验证 | 独立重建 + 字节对比 + dumpbin |

### 7.5b — 工具链迁移

| # | 任务 | 描述 |
|---|------|------|
| 7.5b.1 | `[Tool]PackingMan` | BIN 打包工具 CMake |
| 7.5b.2 | `[Tool]Regen` | 怪物重生编辑器 CMake |
| 7.5b.3 | `[Tool]DS_RMTool` | GM 工具 CMake |
| 7.5b.4 | `[Tool]AutoPatchToolWin32` | 自动更新器 CMake |
| 7.5b.5 | `anmexp` / `maxexp` | 3ds Max 导出插件（可跳过） |

### 7.6 — [Server]MurimNet + [Monitoring]Server

| # | 任务 | 描述 |
|---|------|------|
| 7.6.1 | `[Server]MurimNet/CMakeLists.txt` | PvP 服务 CMake |
| 7.6.2 | `[Monitoring]Server/CMakeLists.txt` | 监控服务 CMake |

---

## Phase 8 — 集成测试与回归

**优先级**: ⭐⭐⭐⭐⭐ (最高，在所有模块就绪后)
**依赖**: Phase 3, 4, 6, 7 全部完成
**预计工时**: 40-60 小时

### 目标
端到端验证：现代化客户端 + 现代化服务端 = 完整可玩游戏

### 子任务

| # | 任务 | 描述 |
|---|------|------|
| 8.1 | 服务端集成测试框架 | 一键启动 Distribute + Agent + N×Map |
| 8.2 | 协议回放测试 | 录制旧版客户端-服务端通信，回放对比 |
| 8.3 | 数据库迁移测试 | MSSQL → PostgreSQL 数据完整性 |
| 8.4 | 资源加载全量测试 | 所有 `.bin` / `.pak` / `.bmhm` 全量读取 |
| 8.5 | 性能回归测试 | FPS / 内存 / 网络延迟 vs 旧版 |
| 8.6 | 多 locale 测试 | KOR / CHINA / HK / TL / JAPAN 全 locale |

---

## Phase 9 — 跨平台（可选）

**优先级**: ⭐⭐ (低，Windows 优先)
**依赖**: Phase 8 完成
**预计工时**: 60-120 小时

| # | 任务 | 描述 |
|---|------|------|
| 9.1 | Linux 服务端 | CMake + Clang 编译，去掉 Win32 依赖 |
| 9.2 | SDL3 窗口层 | 替换 Win32 窗口创建 |
| 9.3 | Vulkan 渲染后端 | 替代 DX11 |
| 9.4 | PostgreSQL 默认数据库 | 替代 MSSQL |

---

## Phase 10 — 模型/动画格式现代化

**优先级**: ⭐ (最低)
**依赖**: 无硬依赖
**预计工时**: 30-50 小时

| # | 任务 | 描述 |
|---|------|------|
| 10.1 | `.chl/.chx/.chr/.mon` 格式完整逆向 | 写二进制格式文档 |
| 10.2 | FBX 转换器 | `.chl` → `.fbx` 双向转换 |
| 10.3 | 3ds Max 2024+ 导出插件 | 替代旧版 MAXEXP |

---

## 给 MiniMax 的特别说明

### 你擅长的领域（建议优先分配）
1. **Phase 7.3 (Agent 迁移)**: 纯 CMake 体力活，24 个文件，与已完成的 Distribute 几乎 1:1 模板
2. **Phase 7.5b (工具链迁移)**: 类似体力活，4-5 个小工具
3. **Phase 8 测试编写**: 大量重复测试用例
4. **Phase 3 加密算法**: 算法逆向和单元测试
5. **中文本地化相关**: 理解游戏内的中文注释、locale 宏含义

### 我（Claude）擅长的领域
1. **Phase 4 网络层重构**: 复杂异步架构设计
2. **Phase 6 客户端重构**: 大规模代码分析和重构策略
3. **Phase 9 跨平台**: Clang/Linux/CMake 深度知识
4. **代码审查和 Bug 修复**: KNOWN_BUGS.md 中的深层问题

### 如果你被上下文长度限制
1. 先读本文件 + `AGENTS.md`
2. 只读你要做的 Phase 对应的文档
3. `git log --oneline -3` 确认最后状态
4. 在 `modern/docs/` 下写你的工作记录
5. 完成后一定要更新本文件的 Phase 出口状态

---

## Phase 出口状态记录

### Phase 7.5 (Map) 出口状态 ✅ (2026-07-07 Gate 验证通过)
- **Gate 验证**: ✅ 独立重建成功
- **构建结果**: MapServer_KOR.exe 3,857,920 bytes, **0 errors**, 313 cpp
- **SWorking 基准**: 2,555,904 bytes (+50.9% delta — Debug /MTd vs legacy Release /MD)
- **字节对比**: 不匹配（预期内 — Debug vs Release CRT 不同）
- **冒烟测试**: 未通过（4DyuchiNET.dll + SS3DGFunc.dll 不在 PATH — 服务端二进制预期行为）
- **已知问题**: C-30 (legacy Release 永远编不过), D-5 (Map gate byte parity: Debug vs Release CRT delta)
- **给下一个 AI 的备注**: Map 用 Debug_Console (KOR) 配置编译。字节对比差异大是因为 /MTd + /Od vs 旧版的 Release CRT。如果要做字节级匹配，需要用 Release 配置 + _KOR_LOCAL_ + 修复 ChannelSystem.cpp 的 C-30 bug（bBattleChannel 等字段只在 _KOR_LOCAL_ 下定义）。

### Phase 7.3 (Agent) 出口状态 ✅ (2026-07-07 完成)
- **最后提交**: *(待提交)*
- **构建结果**: AgentServer_KOR.exe 1,493,504 bytes, **0 errors**, 37 cpp
- **SWorking 基准**: 290,816 bytes
- **创建的文件**:
  - `[Server]Agent/CMakeLists.txt` — Release + 5 Debug locale 目标
  - `[Server]Agent/ErrorMsg.h` — 服务端 stub（阻止 CommonDefine.h 引入客户端 ErrorMsg.h）
  - `[Server]Agent/StdAfx.h` — MFC 去残留 + winsock2 前置 + LOG-undef
  - `modern/scripts/build_agent.py` — 构建脚本
- **构建时修复的三个问题**:
  1. 移除 GameResourceManager.cpp（Agent 的 Server.cpp 用 `#ifdef _MAPSERVER_` 保护了它，且它引入 Map 专属的 TacticManager.h）
  2. 添加 `_AGENTSERVER` 定义（无尾部下划线 — UserTable.cpp:9 用的这个变体，而 vcproj 只定义了 `_AGENTSERVER_`）
  3. 链接 SS3DGFunc.lib + IndexGenerator.lib（MsgTable.cpp 使用了 ICCreate/ICRelease 等符号）
- **已知问题**: D-6 (ggsrv25.lib 缺失 — HK locale 链接会失败), D-7 (SS3DGFunc.lib + IndexGenerator.lib 是链接时发现的未声明依赖)
- **给下一个 AI 的备注**: Agent 是 Distribute 的姊妹模板，两者共享相同的 [CC]ServerModule + [CC]Header 内核。任何对 Distribute StdAfx.h 的修复也应应用到 Agent。

### Phase 7.4a (Distribute) 出口状态
- **最后提交**: `17d76b4` (gate passed)
- **验证通过**: 0 errors, DistributeServer.exe 204,800 bytes, 启动冒烟 exit code 0
- **已知问题**: D-1 (LOG-undef shim), D-2 (YHLibrary x64→x86), D-3 (mfc71.lib out-of-scope)
- **给下一个 AI 的备注**: Distribute 是后续所有服务端的模板。新服务端 CMakeLists 直接复制 Distribute 的结构。

### Phase 5 (DX11 渲染器) 出口状态
- **最后提交**: `713ba3f` (Phase 5.8 完成)
- **测试**: 99/99 PASS
- **已知限制**: 7 个 deferred stubs (HeightField full / motion cache / render target pool / error texture / BC real compression / specular / per-geometry light)
- **给下一个 AI 的备注**: 接口已完整（75 个方法），客户端接入时直接调用即可。stubs 不影响基本渲染。

### Phase 7.5b (工具链) 出口状态 ❌ (全部跳过 — MFC)
- **PackingMan**: SKIP — UseOfMFC="2" (full MFC)
- **Regen**: SKIP — UseOfMFC="2" (full MFC)
- **DS_RMTool**: SKIP — UseOfMFC="1" (shared MFC)
- **AutoPatchToolWin32**: SKIP — No MFC, but depends on MFC-based ZipArchive.lib
- **给下一个 AI 的备注**: 所有 4 个工具均因 MFC 或 MFC 传递依赖而跳过。ZipArchive 有预编译版本可用于完整 VS 环境。

### Phase 3.2 (HSEL 流密码) 出口状态 ✅ (2026-07-07)
- **测试**: **32/32 PASS** (MsvcRand + KeyGen + Stream + RoundTrip×19 + Stress)
- **算法**: MSVC6 LCG PRNG → 12×int32 key gen → block swap → 4 DES types (XOR/ADD/SUB/MIXED) → CRC → key schedule
- **源文件**: hsel_stream.hpp + hsel_stream.cpp (800 lines) + 32 tests
- **给下一个 AI 的备注**: HSEL 已完整逆向实现，测试覆盖全部类型×大小组合。如需与旧版 HSEL.lib 字节级兼容，用 `MsvcRand`（MSVC6 LCG 再现）；否则直接用 `mxh::crypto::Aes256GcmCipher`。

---

## 快速启动命令（备忘）

```powershell
# 现代代码构建
cmd /c "call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && cmake -S modern -B modern/build -G 'Visual Studio 17 2022' -A x64 && cmake --build modern/build --config Debug"

# 现代代码测试
cmd /c "call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && ctest --test-dir modern/build/tests -C Debug --output-on-failure"

# 遗留库构建 (以 Distribute 为例)
python "D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\modern\scripts\build_distribute.py"

# 查看已迁移目标状态
grep "DONE\|SKIPPED\|DEFERRED\|TODO" "D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\modern\PHASE7_MIGRATION_RECIPE.md"
```
