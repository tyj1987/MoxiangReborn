# GPU-PV 综合指南 — M-R5/M-R7/M-R4 物理 GPU 截屏解锁

> 写于 2026-08-19,M-R4.8 (ca6b0ee3) 完成后,79/79 PASS,进入 GPU-PV 物理 GPU 测试阶段。

---

## 1. 为什么需要 GPU-PV

Moxiang 1:1 视觉验证分两段:

| 段 | 验证目标 | 工具 | 状态 |
|---|---|---|---|
| 资源 1:1 | 老 sprite byte-compare (SHA-256) | `verify_algorithms.py` 85 个 .tif | ✅ M-R4.2 完成 |
| Dialog 1:1 | 老 .bin → cDialog 装载,跟老 eCTRL_TYPE 1:1 | `cDialogLoader_test` 79/79 | ✅ M-R4.8 完成 |
| 视觉 1:1 | 6 状态 .tga 跟老 client 真实输出比 SSIM ≥ 0.95 | `visual-smoke.ps1` + `visual-compare.py` | ❌ 老 client Win11 崩溃 (SS3DGFunc.dll 0xC0000005) |
| 性能 1:1 | 5 → 30 fps | benchmark | ❌ blocked-on-GPU |
| 分辨率自适应 | 1024×768 ~ 1920×1080 多档 | visual-smoke 多档 | ❌ blocked-on-GPU |

**老 client 永远跑不起来**(2013 编译的 SS3DGFunc.dll + Win11 DX12 不兼容),整体页面 SSIM 比对无参考 baseline。**GPU-PV 解锁的不是 1:1 SSIM,而是**:

1. **现代 modern MoxianClient 自身 6 状态截图作为新 baseline** (物理 GPU 出图,WARP 软渲不可信)
2. **M-R5 性能**(frustum cull / static mesh chunk / HUD instancing)物理 fps 验证
3. **M-R7 分辨率自适应**多档位视觉验证

---

## 2. 当前 VM 端 GPU 状态 (2026-08-19 00:30 CST 快照)

```
PnP 显示设备:
  [软显] Microsoft Hyper-V Video (VMBUS)
  [软显] Microsoft Remote Display Adapter (RDP indirect)

DirectX 12.1 (max feature level 49408)
nvidia-smi: 不存在
AMD Adrenalin: 不存在

Verdict: WARP_ONLY  ← 物理 GPU 测试 blocked
```

`scripts/vm-gpu-verify.ps1` 跑一次,JSON 写到 `modern/docs/restoration-plan/gpu-pv-report.json`。

---

## 3. GPU-PV 配置三步走

### Step 1: 用户在 host 端配 GPU-PV

完整 checklist 看 `scripts/host-gpu-pv-setup.md`(6718 bytes)。

摘要:
1. VM 完整关机 (Shut Down)
2. host 端卸载物理 GPU 驱动 (Device Manager → Uninstall device + 勾 "Attempt to remove")
3. Hyper-V Manager → VM Settings → Physical GPUs → 选 NVIDIA/AMD 独显 → Apply
4. VM 启动
5. VM 内 RDP 进

### Step 2: VM 端自检

```powershell
cd C:\moxiang
.\scripts\vm-gpu-verify.ps1
```

期望:
- `Verdict: PHYSICAL_GPU` (绿)
- `[PCI 物理] NVIDIA GeForce ...` 或 `[PCI 物理] AMD Radeon ...`
- `nvidia-smi: 存在 ✓` (NVIDIA 独显)

JSON 报告: `modern/docs/restoration-plan/gpu-pv-report.json`

### Step 3: 跑 visual-smoke + 比 SSIM

```powershell
cd C:\moxiang
.\scripts\visual-smoke.ps1 -MapNumber 12
```

跑完产物在 `modern/docs/restoration-plan/baseline/{runId}/`,含:
- `modern-connect.tga`
- `modern-login.tga`
- `modern-charselect.tga`
- `modern-charmake.tga`
- `modern-gamein.tga`
- `modern-gamein-terrain.tga`
- `logs/client.out.log` + `client.err.log`
- `state-frames/`

跟 baseline `ca64d007` 比 SSIM(用 `scripts/visual-compare.py`):

```powershell
# 5 状态 × baseline 对比
$states = @('connect','login','charselect','charmake','gamein')
foreach ($s in $states) {
    $a = "modern\docs\restoration-plan\baseline\ca64d007\modern-$s.tga"
    $b = "modern\docs\restoration-plan\baseline\NEW\modern-$s.tga"
    python scripts\visual-compare.py $a $b --threshold 0.95
}
```

> **重要**: `ca64d007` 是 WARP 软渲 baseline (4 状态黑屏 + gamein OK),**不能作为物理 GPU 的 SSIM 参考**。
> 物理 GPU 首次跑出 6 状态后,需要**人工肉眼确认** modern MoxianClient 输出符合 1:1 设计意图(对话框位置/HP条/sprite 都对),才能把这次 runId 定为 `physical-gpu-baseline`。
> 之后每次跑跟这个 baseline 比 SSIM ≥ 0.95。

---

## 4. 状态 5 (hud-only) + 状态 6 (inventory) 自动化

当前 `scripts/visual-smoke.ps1` line 139-141 状态 5/6 是 TODO,等 M-R4 cDialog 树完成。

M-R4.8 已 done (79/79 PASS), cDialog 树装载 + sprite hook 都在,**状态 5/6 现在可以做**。

实现方案 (未在 visual-smoke.ps1 落地,等 GPU-PV 配完后一起改):

| 状态 | 触发方式 | 截图 |
|---|---|---|
| 5 (hud-only) | gamein 后停留 5s,自动渲染 HP/MP/QuickSlot | `--save-frame hud.tga` |
| 6 (inventory) | gamein 后 SendInput "I" 键触发 Inventory 对话框 | `--save-frame inv.tga` |

MoxianClient 当前 `--exit-after-gamein` 模式立即退,需要:
- A. 改 MoxianClient `main.cpp` 加 `--state-frames-trigger` flag (gamein 后 N 帧 + 模拟按键)
- B. PowerShell 跑 visual-smoke,外加 `Add-Type SendInput` 注入 I 键

B 简单,但需要 client 不在 gamein 立即退 = 改 visual-smoke 去掉 `--exit-after-gamein`,client 会一直跑到 timeout。

**实际决定**:**这次先不动 visual-smoke.ps1**。等 GPU-PV 配完第一次跑通,5 状态出图 OK,再做状态 5/6 自动化。

---

## 5. M-R5 性能 / M-R7 分辨率 验证路径

M-R5 性能 (5 → 30 fps) 需要:

| 段 | 工具 | 目标 |
|---|---|---|
| GPU frame time | 物理 GPU 内置计时器 (`D3D11_QUERY_TIMESTAMP_DISJOINT` + `D3D11_QUERY_TIMESTAMP`) | baseline 5 fps → 优化后 30 fps |
| CPU frame time | RDTSC + std::chrono | 同上 |
| Draw call 数 | `D3D11DeviceContext::DrawIndexed` 调用次数 | baseline → 优化后 |

modern MoxianClient 当前 renderer:
- `C:\moxiang\modern\src\render\renderer.cpp` 走 DX11 WARP
- 缺 GPU frame time 计时器(只在物理 GPU 上 query 有意义)
- 缺 frustum cull / static mesh chunk / HUD instancing

M-R7 分辨率自适应:
- 加 `--window 1920x1080` flag 到 MoxianClient
- visual-smoke 多档跑 1024×768 / 1280×720 / 1920×1080 / 2560×1440
- 6 状态 × 4 档 = 24 张 .tga 对比
- SSIM 期望各档 ≥ 0.95 (UI 元素位置缩放正确,无锯齿/拉伸)

**两个都 blocked-on-physical-gpu**。配完 GPU-PV 后,代码 + 测试 + 验证同步推。

---

## 6. 文件清单

| 路径 | 大小 | 用途 |
|---|---|---|
| `scripts/vm-gpu-verify.ps1` | 7363 bytes | VM 端 GPU 自检,出 JSON 报告 |
| `scripts/host-gpu-pv-setup.md` | 6718 bytes | host 端操作 checklist (Step 1-4) |
| `modern/docs/restoration-plan/gpu-pv-guide.md` | 本文件 | 综合指南 + 后续工作流 |
| `modern/docs/restoration-plan/gpu-pv-report.json` | 自动生成 | VM 端自检结果,被 visual-smoke 读 |

---

## 7. 用户交互

1. 用户看完 `scripts/host-gpu-pv-setup.md`,在 host 端执行 Step 1-3
2. VM 重启后,跑 `scripts/vm-gpu-verify.ps1` 给我结果
3. 如果 verdict=PHYSICAL_GPU,跑 `scripts/visual-smoke.ps1` 截 6 状态 .tga
4. 肉眼确认 modern 6 状态输出符合 1:1 设计意图,定 baseline
5. 之后每次跑比 SSIM ≥ 0.95

如果 verdict=WARP_ONLY,回到 host 端重检查 (驱动是否真的卸载 / BIOS VT-d 是否开 / VM 是否 Gen 2)。
