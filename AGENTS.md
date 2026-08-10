# AGENTS.md — Mavis 协作指南

> 指导 Mavis（以及任何其他 AI agent）在这个项目上工作。
> **终极目标**：1:1 完美复现 2003-2010 韩国 2D MMORPG《墨香》。详见 `ROADMAP.md`。
> **最近一次重置**：2026-07-25。

---

## 0. 不可破坏的约束

违反任何一条 = 1:1 复现失败。这 4 条是项目宪法：

1. **资源格式二进制兼容** —— `.bin` / `.pak` / `.bmhm` / `.ttb` / `.chl` / `.chx` / `.chr` / `.mon` / `.bsad` 必须能被现代代码读到和老码完全一致的字节
2. **modern 网络闭环稳定** —— 原 `[CC]Header/Protocol.h` 与 `CommonStruct.h` 不修改并继续作为参考；商业 RC 只要求 modern 客户端/服务端互通，不强制新旧互通
3. **玩法/数值 1:1 锁定** —— 经验曲线、伤害公式、爆率、Boss 刷新、商城、MurimNet PvP 不能动
4. **HSEL/HackShield/nProtect 接口签名保持** —— 实现可换，签名不能换

---

## 1. 项目结构（真结构，不是老 session 留下的）

```
C:\moxiang\
├── 墨香【源码】/                  # 原始源码（不动）
│   ├── [Client]MH/                # 客户端 ~930 文件 / 35-40 万行
│   ├── [Server]*/                 # 服务端 3 个进程
│   ├── [CC]Header/                # 协议头（不可改）
│   ├── [Lib]*/                    # 引擎库
│   ├── 4Dyuchi*/                  # 自研 3D 引擎
│   └── [Tool]*/                   # 工具链源码
├── 墨香【源码配套资源】/PlayDH/    # 完整游戏资源（1.3GB）
├── 墨香【教程】/                  # 4 篇中文教程
├── 墨香【客户端+服务端+工具】/     # 已部署包（参考）
├── modern/                        # 现代 C++17/20 重写（**全部在这**）
│   ├── src/                       # 254 个文件
│   │   ├── compat/                # 资源兼容层（100%）
│   │   ├── crypto/                # AES-256-GCM + HSEL
│   │   ├── db/                    # MSSQL/SQLite
│   │   ├── net/                   # Asio + IOCP
│   │   ├── proto/                 # 协议
│   │   ├── render/                # DX11
│   │   ├── ui/                    # 70+ dialog（1:1 port）
│   │   ├── server/                # 服务端核心
│   │   ├── services/              # service interface
│   │   ├── log/                   # MLOG
│   │   ├── memory/                # ObjectPool
│   │   └── monitor/               # perf monitor
│   ├── tests/unit/                # 153 个测试文件 / 2380 用例
│   ├── tools/                     # 11 个现代工具
│   ├── include/mxh/               # 公共头
│   ├── docs/                      # 中间过程文档（逐步清理）
│   └── build/                     # CMake 构建输出
├── deploy/                        # 现代部署（部分）
├── docs/                          # 真实数据文档
│   ├── RESOURCE_FORMATS.md        # 资源格式
│   ├── MoxianProtocolDoc.md       # 协议
│   ├── DATABASE_SCHEMA.md         # 数据库
│   └── KNOWN_BUGS.md              # bug 清单
├── ROADMAP.md                     # 1:1 复现路线图（**主路线**）
├── AGENTS.md                      # 本文件
├── README.md                      # 上手指南
└── scripts/                       # 启动脚本
```

---

## 2. 工作流（每天必读 3 件事）

每次开 Mavis session，**先读**：

0. `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\session-bootstrap.ps1` —— 见 §2.5（PowerShell 7 可将 `powershell` 换成 `pwsh`）
1. `ROADMAP.md` §0-3 —— 当前在哪个里程碑，下一步是什么
2. `AGENTS.md` 本文件 —— 约束 + 陷阱
3. `docs/KNOWN_BUGS.md` —— 待修 bug（挑不会破坏 1:1 的来修）

**然后做**：

- 改 `modern/src/` 或 `modern/tests/` 或 `modern/tools/`
- 写或更新测试
- 跑 `cmake --build modern/build --config Debug` + `ctest -C Debug`
- 仅在可复现证据改变模块状态时更新 `ROADMAP.md` §2；完成明细写入 `docs/CHANGELOG.md`

**不要做**：

- 改 `墨香【源码】/`（除非改老编译 bug，且不会改变行为）
- 改 `[CC]Header/Protocol.h` 或 `CommonStruct.h`
- 改 `墨香【源码配套资源】/PlayDH/`
- 写 P0/P1/P2/P3 任务队列（已被 ROADMAP §3 替代）
- 写 session 交接 log（已废弃）
- 写到根目录的 `.log / .obj / .db` 等临时文件

---

## 3. 真实陷阱清单（不要被老 AGENTS.md 误导）

老 AGENTS.md 列了 10 条"陷阱"，其中有些已修、有些过时。下面是**当前仍然成立**的：

| 陷阱 | 当前状态 | 处理 |
|---|---|---|
| `[...]` 目录名 | 仍存在 | PowerShell 操作必须用 `-LiteralPath` |
| `#pragma pack(push,1)` | 仍存在 | 网络包改字段会崩溃 |
| `LOG` 宏冲突 | **已根治**（2026-07-15） | CConsole::LOG → CConsole::MLOG，新代码用 MLOG |
| `WS2_32.lib` 链接 | **已修** | CMakeLists.txt 已含 |
| `d3dx8.lib` 路径 | **已迁移到 DX11** | 用 modern 渲染，不要再碰 DX8 |
| EUC-KR/CN 注释乱码 | 仍存在 | 保持原编码别动 |
| 多语言 ifdef (`_KOR_LOCAL_` 等) | 仍存在 | 5 种宏都编，确保每种都能 build |
| HSEL 硬件狗 | 80% stub | R-1，阻塞运行时 |
| HackShield 反外挂 | 0% | R-2，阻塞客户端登录 |
| SQL Server 集成 | 60% | 已写 schema + restore，缺端到端验证 |
| **F-1 shell_command JSON 截断** | **每次必踩**（2026-07-30 已根治） | 见 §2.5 + `scripts/no-truncation.ps1`；单行简单 cmdlet，复杂逻辑写到 .ps1 再 `pwsh -File` |
| F-2 根目录 scratch_*.py 污染 | 反复犯 | `scripts/session-bootstrap.ps1` 第 1 步自动清 |

**根目录散落文件陷阱**（最近 session 反复犯）：
- 不要往根目录写 `*.log / *.obj / *.db` / `test_*.txt`
- 临时文件用 `modern/scratch/<source>_<日期>/` 归档
- 子目录根放 `README.md` 索引
- `scripts/session-bootstrap.ps1` 第 1 步会清理根目录散落，**不要绕过它**
- 不要往根目录写 `*.log / *.obj / *.db` / `test_*.txt`
- 临时文件用 `modern/scratch/<source>_<日期>/` 归档
- 子目录根放 `README.md` 索引

---

## 4. 编码风格

**老代码**（`墨香【源码】/`）：匈牙利命名法，不动。
**新代码**（`modern/`）：
- 遵循老代码的 `m_` 成员 / `g_` 全局 / `p` 指针前缀
- C++17 起步；能用 `std::optional / std::string_view / auto / 范围 for` 就用
- 智能指针优先；裸指针只在 C ABI / OS 句柄时用
- 注释中文/英文都行，避免韩文/日文
- 头文件 `#pragma once`
- 类成员大括号同行（C++ 风格，不是老 C 的 Allman）

---

## 5. 提交规范

每次提交必须：

- **1 个 commit = 1 个对话框 / 1 个 bug fix / 1 个工具**，**不要把多个混在一起**
- commit message 用 `类型: 简述` 格式：
  - `ui: cCheckBox 1:1 port + 22 tests`
  - `crypto: HSEL stub 补 20% 绕过点`
  - `bug: R-3 CommonStruct.h 编译错误修`
  - `docs: ROADMAP 重置到 1:1 复现`
- commit 必跑：`cmake --build modern/build --config Debug` 0 错 + `ctest -C Debug` 全过
- 1:1 port 必须有 unit test 锁死（dialog 至少 1 个行为断言）

---

## 6. 与 Mavis 的协作边界

**Mavis 自动可做**：

- 改 `modern/src/` / `modern/tests/` / `modern/tools/` / `modern/include/`
- 写新 dialog / service / 工具
- 跑测试 + 修编译错误
- 重构 modern/ 内部代码（不改接口）

**Mavis 必须问用户**：

- 改老源码（`墨香【源码】/`）的逻辑
- 改协议头（`[CC]Header/Protocol.h`）
- 改资源文件
- 1:1 复现 vs 功能增强 的取舍

**用户拍板**：

- 删文件（破坏性操作，列清单等确认）
- 1.0 release 标记
- 跨平台方向选择

---

## 7. 常见问题

**Q：老 MODERNIZATION_PLAN.md / CHANGELOG.md / AI_*.md 还在，能参考吗？**
A：能读，不要被"Phase 12 已完成"误导。那是 35% 完成的过度表述。看 `ROADMAP.md` §2 现状表和 §5 完成判据。

**Q：能直接用 `SWorking/` 启动老服吗？**
A：能，那里有完整编译产物。`scripts/start-server.ps1` 还在。

**Q：modern 代码需要和老客户端/老服务端互通吗？**
A：商业 RC 不强制新旧互通；modern 客户端必须和 modern Login/Agent/Map 完整互通。原协议仍用于结构与行为参考，资源、视觉、音频、地图、玩法和数值继续要求 1:1。

**Q：HSEL 硬件狗真没了怎么办？**
A：用 `modern/src/crypto/` 的 stub 跑，能进登录但有些功能受限。要真上线得补完 stub（参 ROADMAP §2 R-1）。

**Q：怎么贡献？**
A：挑 `ROADMAP.md` §3 当前阶段的一个 task → 改 modern/ → 测试过 → 提交。PR 不需要（单分支 + commit history 足够）。

## 2.5 Session Bootstrap（每次开 session **第一件事**）

> **2026-07-30 根因事故**：session `019f9c8e` 和 `019fb254` 先后在 4 天 / 30 分钟内因为同一个 JSON 截断 bug 死亡（详见 §3 陷阱 F-1）。两个 session 都留下未提交的工作和根目录 `scratch_*.py` 污染。修复 = 改 agent 行为，不动模型。

**每个新 Mavis session 在读 ROADMAP / AGENTS / KNOWN_BUGS 之前，先跑**：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File C:\moxiang\scripts\session-bootstrap.ps1
```

默认只审计并列出污染数量，不静默删除。查看 `docs/CLEANUP_MANIFEST.md` 并确认目标后，才可显式增加 `-CleanKnownArtifacts`。

脚本会做 4 件事，**全部通过才能继续**：

1. **清根目录散落** —— 删除 `C:\moxiang\scratch_*.py`、临时 `*.log / *.obj / *.db`、空 `Testing\Temporary\`。
2. **检查 working tree** —— `git status --short` 非空时输出警告（不是阻断，让你知道有没有上一 session 留下的工作要 commit）。
3. **加载反 JSON 截断工具箱** —— 输出 `scripts/no-truncation.ps1` 的存在性，并把它的使用提示打印到屏幕上。
4. **打印 1 行确认** —— `AGENTS.md ✓ | scratch clean ✓ | bootstrap loaded ✓ | ready`。

如果第 1 或第 3 步失败，**停下来修，不要带着 broken state 进 session**。

### 反 JSON 截断硬规则（agent 必须遵守）

`MiniMax-M3` 在生成 `shell_command` 工具调用时，arguments 字符串经常被截断
（不发闭合 `"` / `}`），CLI 端 `EOF while parsing a string at column N` 报错，
且**没有自动重试 / 降级**。一旦触发必死。

**禁止**：

- 在一条 `shell_command` 里写多语句 PowerShell（`$x = ...; for (...) { ... }`）
- 在一条 `shell_command` 里用 `\"` / `$()` / backtick `` ` `` 转义
- 在一条 `shell_command` 里嵌入 here-string `@"..."@`
- 把 CJK 路径直接拼到 PowerShell 表达式里（用 `-LiteralPath` 也救不了，但能降概率）

**必须**：

- 每条 `shell_command` 是**单行简单命令**（一个 cmdlet 或一个 `|` 管道）
- 复杂逻辑 → 先 `apply_patch` 写一个 `.ps1` 或 `.py` 文件到 `modern\scratch\<date>-<topic>\`，再 `pwsh -File` / `python` 调用
- 读文件 → 用 **Read 工具**，不要 `Get-Content` 链
- 写文件 → 用 **apply_patch 工具**，不要 sed / 流式 PowerShell
- 跑测试 → `ctest -C Debug --test-dir modern\build`，不要 subprocess hack
- 跑 build → `cmake --build modern/build --config Debug`，不要手动调 MSBuild

`scripts/no-truncation.ps1` 提供 4 个安全 wrapper（`Get-FileLines`、`Read-JsonObject`、`Test-PathSafe`、`Format-TestOutput`），任何涉及文件读取 / JSON 解析 / 路径判断的操作**优先调用它们**而不是裸 shell_command。
