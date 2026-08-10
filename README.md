# Moxian-Reborn（墨香重生）

在现代 Windows 环境中 1:1 复现 2003–2010 韩国 MMORPG《墨香》。资源、地图、音乐、视觉、玩法数值和 UI 以原版为准；modern 客户端与 modern 服务端完整互通，不强制兼容旧网络端点。

## 五分钟开始

要求：Visual Studio 2022 C++ 工具链、CMake、Python 3，以及 Windows PowerShell 5.1 或 PowerShell 7。

```powershell
# 在 C:\moxiang 运行。PowerShell 7 可将 powershell 换成 pwsh。
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/session-bootstrap.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/setup-modern.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-modern.ps1 -Config Debug
ctest -C Debug --test-dir modern/build --output-on-failure
python scripts/check-project-governance.py
```

当前 CMake 发现基线为 11,733 项测试。完整状态以 [ROADMAP.md](ROADMAP.md) 为准，数字变化必须由构建目录中的 `ctest -N` 重新生成。

## 常用验证

```powershell
# 资源/协议/登录关键路径
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/commercial-smoke.ps1 -BuildDir modern/build

# DX11 demo；可保存 headless 帧
modern\build\tools\MoxianRenderDemo\Debug\mxh_render_demo.exe --headless --save-frame modern\build\r9-frame.tga --frame-count 3

# 启动 modern 三进程（参数见脚本帮助）
powershell -NoProfile -ExecutionPolicy Bypass -File deploy/scripts/start_modern.ps1 -Mode status
```

商业门禁默认还运行 LocalDB E2E。没有兼容 SQL Server/ODBC 环境时可显式增加 `-SkipMssql`；这只验证 33 项核心门禁，不代表 MSSQL 外部验收完成。

## 项目入口

- [ROADMAP.md](ROADMAP.md)：当前状态、里程碑和完成判据。
- [AGENTS.md](AGENTS.md)：不可破坏约束与协作规范。
- [docs/RESOURCE_FORMATS.md](docs/RESOURCE_FORMATS.md)：资源格式。
- [docs/MoxianProtocolDoc.md](docs/MoxianProtocolDoc.md)：协议说明。
- [docs/DATABASE_SCHEMA.md](docs/DATABASE_SCHEMA.md)：数据库结构。
- [docs/KNOWN_BUGS.md](docs/KNOWN_BUGS.md)：仍在活动的缺陷。

原始源码和配套资源是只读基准；新开发仅进入 `modern/`、`deploy/`、`scripts/` 和治理文档。项目仅供学习研究，原游戏版权归其权利人所有。
