# 已归档：VM GPU-PV 工具链（2026-08-19）

> **状态**：过时错误，不可用
> **归档原因**：本机 = 物理机 + 物理 GPU（Intel Arc B580 4GB + 2560×1440），不需要任何 VM / Hyper-V GPU-PV（DDA）配置

## 为什么不留

`modern/docs/restoration-plan/04-end-to-end-commercial.md` §0 已明确：
> 之前的 `scripts/vm-gpu-verify.ps1` + `host-gpu-pv-setup.md` + `gpu-pv-guide.md` 全部过时错误（假设 VM 借 host GPU）。

G4 物理截屏 + G5 性能 30fps 都在**本机直接推**，不需要任何 host 端操作。

## 归档内容

| 源文件 | 性质 | 为什么错 |
|---|---|---|
| `scripts/vm-gpu-verify.ps1` | 工具脚本 | VM 内自检，本机是物理机没 VM |
| `scripts/host-gpu-pv-setup.md` | 文档 | Hyper-V GPU-PV / DDA 配置，本机不需要 |
| `modern/docs/restoration-plan/gpu-pv-guide.md` | 文档 | 同上，描述 VM 借 host GPU 方案 |
| `modern/docs/restoration-plan/gpu-pv-report.json` | 报告 | `vm-gpu-verify.ps1` 生成的 VM 报告（最后一次跑） |

## 替代方案

- **物理 GPU 自检**：`scripts/visual-smoke.ps1` 启动 MoxianClient 直接看 DX11 设备 + adapter
- **物理截屏**：`scripts/visual-smoke.ps1` / `scripts/gui-client-smoke.ps1` 直接出 `.tga`（G4 证据）
- **性能基准**：`scripts/commercial-smoke.ps1` (MSSQL_E2E + STATE_FRAMES) + visual-smoke 帧率日志

## 参考

- 详细计划：`modern/docs/restoration-plan/04-end-to-end-commercial.md` §0 + §2 Phase A G4/G5
- 真实 GPU 信息：Windows Device Manager → Display adapters → Intel Arc B580
