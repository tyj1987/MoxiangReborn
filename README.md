# Moxian-Reborn（墨香重生）

在现代 Windows 环境中 1:1 复现 2003–2010 韩国 MMORPG《墨香》的渲染管线、
UI 布局、玩法数值与网络协议。资源、地图、音乐、视觉以原版为基准；
modern 客户端与 modern 服务端完整互通，不强制兼容旧网络端点。

⚠️ **逆向工程项目，仅供学习研究。** 代码 MIT（见 [LICENSE](LICENSE)），
资源版权属原权利方，**不可商用**（见 [NOTICE.md](NOTICE.md)）。

---

## 五分钟开始

**前置**：Visual Studio 2022 C++ 工具链、CMake ≥ 3.20、Python 3、
Windows PowerShell 5.1 或 PowerShell 7。

**步骤 1 — 克隆 + 初始化**

```powershell
git clone https://github.com/tyj1987/<repo>.git
cd <repo>
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/session-bootstrap.ps1
```

**步骤 2 — 拷贝 1.3GB PlayDH 老资源**（仓库不包含，请从备份档案
`墨香【源码配套资源】/PlayDH/` 复制到 `modern/data/PlayDH/`，详见
[docs/RESOURCE_FORMATS.md](docs/RESOURCE_FORMATS.md)）

```powershell
# 推荐位置：C:\moxiang\modern\data\PlayDH
Copy-Item -LiteralPath '<备份路径>\PlayDH\*' -Destination 'modern\data\PlayDH\' -Recurse -Force
```

**步骤 3 — 构建 + 测试**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/setup-modern.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-modern.ps1 -Config Debug
ctest -C Debug --test-dir modern/build --output-on-failure
python scripts/check-project-governance.py
```

当前 CMake 基线 **158/0 unit tests** 全部通过（M-R3 装载链 + M-R4 跨表
查装 + M-R6 focus chain + M-R2/M-R1 集成），累计 79 + 16 + 63 = 158
个 gtest assertion。完整状态以 [ROADMAP.md](ROADMAP.md) 为准。

---

## 常用验证

```powershell
# 资源/协议/登录关键路径
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/commercial-smoke.ps1 -BuildDir modern/build

# DX11 demo；可保存 headless 帧
modern\build\tools\MoxianRenderDemo\Debug\mxh_render_demo.exe --headless --save-frame modern\build\r9-frame.tga --frame-count 3

# 启动 modern 三进程（参数见脚本帮助）
powershell -NoProfile -ExecutionPolicy Bypass -File deploy/scripts/start_modern.ps1 -Mode status

# 视觉烟雾（5 状态 .tga + 老 sprite SHA-256 比对）
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/visual-smoke.ps1
```

商业门禁默认还运行 LocalDB E2E。没有兼容 SQL Server/ODBC 环境时可显式
增加 `-SkipMssql`；这只验证核心门禁，不代表 MSSQL 外部验收完成。

---

## 物理 GPU 视觉验证（GPU-PV 路径）

M-R4 视觉 1:1 SSIM ≥ 0.95 验证需物理 GPU。在 Win11 + Hyper-V 上：

**步骤 A — VM 端自检**（在你 clone 完仓库后跑）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/vm-gpu-verify.ps1
# 期望：Verdict: PHYSICAL_GPU（绿），JSON 报告写到 modern/docs/restoration-plan/gpu-pv-report.json
```

**步骤 B — 若 verdict=WARP_ONLY**：在 host 端按
[`scripts/host-gpu-pv-setup.md`](scripts/host-gpu-pv-setup.md) 操作
（卸 host 端 GPU 驱动 → Hyper-V Manager 配 GPU-PV → VM 重启），完整
说明见 [`modern/docs/restoration-plan/gpu-pv-guide.md`](modern/docs/restoration-plan/gpu-pv-guide.md)。

**步骤 C — 配完后**：

```powershell
.\scripts\visual-smoke.ps1
python scripts\visual-compare.py modern\docs\restoration-plan\baseline\ca64d007\modern-gamein.tga modern\docs\restoration-plan\baseline\NEW\modern-gamein.tga --threshold 0.95
```

---

## 项目入口

- [ROADMAP.md](ROADMAP.md) — 当前状态、里程碑（M-R0 → M-R8）和完成判据。
- [AGENTS.md](AGENTS.md) — 不可破坏约束与协作规范（4 条项目宪法 + 陷阱清单）。
- [LICENSE](LICENSE) — MIT 协议（仅适用于本仓库代码）。
- [NOTICE.md](NOTICE.md) — 版权与逆向工程声明。
- [docs/RESOURCE_FORMATS.md](docs/RESOURCE_FORMATS.md) — 资源格式。
- [docs/MoxianProtocolDoc.md](docs/MoxianProtocolDoc.md) — 协议说明。
- [docs/DATABASE_SCHEMA.md](docs/DATABASE_SCHEMA.md) — 数据库结构。
- [docs/KNOWN_BUGS.md](docs/KNOWN_BUGS.md) — 仍在活动的缺陷。
- [modern/docs/restoration-plan/](modern/docs/restoration-plan/) — 视觉 1:1
  还原计划与状态文档。

---

## 致谢

- 2003–2010 年原《墨香》研发团队（韩国 Yedang Online / Joyon）
- 早期逆向分析社区对老版 .bin / .pak 格式的解包贡献
- 现代 C++ 社区对 DX11 / Asio / SQLite / nlohmann 项目的开源贡献

原始源码和配套资源是只读基准；新开发仅进入 `modern/`、`deploy/`、
`scripts/` 和治理文档。项目仅供学习研究，原游戏版权归其权利人所有。
