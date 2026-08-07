# Moxian-Reborn（墨香重生）

> 1:1 完美复现 2003-2010 韩国 2D MMORPG《墨香》—— 玩法、数值、协议、资源、UI 全部和原版一致；底层换现代 C++17/20 + DX11 + Asio + AES-256-GCM。

---


## Commercial smoke gate

After configuring `modern/build`, run:

```powershell
powershell -File scripts\commercial-smoke.ps1 -BuildDir modern\build
```

The gate runs 33 focused checks covering legacy login ACK/NACK, encrypted login,
wire goldens, resource byte-level parsing and SHA-256 manifests, plus client login E2E.
Use `-RepeatFlaky` when validating timing-sensitive changes.

## 上手 5 分钟

### 1. 读 3 件事

- [`ROADMAP.md`](./ROADMAP.md) — 1:1 复现路线图（**主文档**）
- [`AGENTS.md`](./AGENTS.md) — 协作指南 + 真实陷阱
- [`docs/RESOURCE_FORMATS.md`](./docs/RESOURCE_FORMATS.md) — 资源格式

### 2. 构建 modern 代码

```powershell
# 在 C:\moxiang\ 下
.\scripts\setup-modern.ps1
cmake --build modern/build --config Debug
ctest -C Debug --output-on-failure
```

期望：0 编译错误 / 2380 测试通过（部分 dialog 测试在 Debug 下可能因 vcpkg 缺包 skip，可忽略）。

### 3. 用资源浏览器验证 1:1 资源读取

```powershell
.\modern\build\Debug\mxh_explorer.exe info "墨香【源码配套资源】\PlayDH\Resource\ItemList.bin"
.\modern\build\Debug\mxh_explorer.exe list "墨香【源码配套资源】\PlayDH\Resource\Effect.pak"
.\modern\build\Debug\mxh_explorer.exe bsad "墨香【源码配套资源】\PlayDH\Resource\SkillArea\9x9_Blank.bsad"
```

### 4. 跑老服务端（参考用）

```powershell
.\scripts\start-server.ps1 -Mode start
.\scripts\start-server.ps1 -Mode status
.\scripts\start-server.ps1 -Mode stop
```

### 5. modern 工具一览

| 工具 | 用途 |
|---|---|
| `mxh_explorer` | 资源浏览器（`.bin/.pak/.bmhm/.ttb/.bsad`） |
| `mxh_packer` | BIN 打包（替代老 PackingTool） |
| `mxh_gmtool` | GM 工具（HTTP API） |
| `mxh_mapeditor` | 地图编辑器（CLI） |
| `mxh_autopatcher` | 自动更新器（HTTPS + bsdiff） |
| `mxh_protocol_doc` | 协议文档生成器 |
| `mxh_dbtool` | 数据库工具（restore/query） |
| `mxh_loginserver` | 登录服（modern） |
| `mxh_agentserver` | 代理服（modern） |
| `mxh_mapserver` | 地图服（modern） |
| `mxh_render_demo` | DX11 渲染 demo |

全部 11 个工具的 build 命令在 `modern/CMakeLists.txt`。

---

## 项目体量

| 维度 | 数据 |
|---|---|
| 原始源码 | ~1500 文件 / 100+ 万行 / 不动 |
| 资源 | 1.3 GB 客户端 + 29 MB 服务端 |
| 现代代码 | 254 src + 153 test + 11 工具 |
| 测试 | 2380 用例 / 全过 |
| 文档 | 7 份（路线图 + 协作 + 资源 + 协议 + 数据库 + bug + 上手） |

---

## 进度（1:1 复现视角）

| 1:1 复现目标 | 完成度 | 阻塞 |
|---|---|---|
| 资源字节级一致（T1） | 100% | – |
| 协议字节级一致（T2） | 100% | – |
| 行为 1:1（T3） | 35%（UI 部分） | 服务端运行时 |
| 客户端能启动 | 0% | Phase A 未开始 |
| 服务端能 listen | 0% | Phase B 未开始 |
| 1.0 release | 0% | 上述全过 |

详见 `ROADMAP.md` §2。

---

## 许可

代码仅供学习研究使用，请勿用于商业目的。原游戏版权归 eSofnet / Yedang Entertainment 所有。
