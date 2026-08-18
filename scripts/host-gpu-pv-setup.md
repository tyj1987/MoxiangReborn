# Host 端 GPU-PV 配置 Checklist (Win11 物理 GPU → Hyper-V VM)

> 用途：把物理 ROG 主机 (192.168.2.30) 上的独立 GPU 通过 **Hyper-V GPU-PV / DDA** 暴露给这台 VM (WINDEV2407EVAL)，让 modern MoxianClient 跑 6 状态物理 GPU 截屏（M-R4 视觉 1:1 验证 + M-R5 性能 + M-R7 分辨率）。
>
> 涉及：Hyper-V Manager + Device Manager + PowerShell。
>
> 耗时：20-40 分钟（含 VM 重启时间）。

---

## 前置确认

VM 端 (本 VM) 必须先**完全关闭** (Shut Down, 不是 Save State / Restart)：

| 步骤 | 动作 |
|---|---|
| 1 | 在本 VM 桌面上 **开始 → 电源 → 关机** |
| 2 | 等到 192.168.2.30 host 上 Hyper-V Manager 显示 VM 是 `Off` 状态 |
| 3 | 准备 host 端 RDP / 物理键鼠操作 |

> GPU-PV 是 PCI passthrough，关机时 host OS 必须不占用 GPU 驱动。一旦配好，**host 桌面的 GPU 加速会临时失效**（登录 RDP / 显示器会改用 WARP / 软显），直到 GPU 重新分配给 host。这是 Win11 GPU-PV 设计的硬约束。

---

## 方案 A：Hyper-V Manager GUI（推荐，最稳）

### Step 1：关 VM + 关 host 显示器独占

在本 VM **完全关机**（power off，不 Save State）。

在 host (192.168.2.30) 上打开 **Hyper-V Manager**：
- 左侧 tree 选 `ROG.52trz.local` → 右侧 actions `Hyper-V Settings...`
- 或者直接选 VM `WINDEV2407EVAL` → 右侧 `Shut Down...`

### Step 2：host 端卸载 GPU 驱动（GPU-PV 要求独占）

`Start → Device Manager` (devmgmt.msc)：
- 展开 `Display adapters`
- 找到 NVIDIA / AMD 独显（**不是** Intel 内显 / Hyper-V Video）
- 右键 → **Uninstall device** → 勾 `Attempt to remove the driver for this device` → `Uninstall`
- 完成后**不要重启**

或者用 PowerShell（admin）：

```powershell
# 列出所有 Display 类设备
Get-PnpDevice -Class Display | Format-Table Status, Class, FriendlyName, InstanceId -AutoSize

# 找非 Hyper-V 的物理 GPU，uninstall
# 替换下面的 InstanceId
Disable-PnpDevice -InstanceId 'PCI\VEN_10DE&DEV_...&SUBSYS_...\4&...' -Confirm:$false
# 完整卸驱动:
pnputil /remove-device 'PCI\VEN_10DE&DEV_...&SUBSYS_...\4&...'
```

### Step 3：Hyper-V Manager 配 GPU-PV

1. `Hyper-V Manager` → 选 `WINDEV2407EVAL` → 右侧 `Settings...`
2. 左侧选 `Physical GPUs`（或 `Add Hardware` → `Physical GPU`）
3. 选中步骤 2 卸载的那个 NVIDIA/AMD GPU
4. `Apply` → `OK`

> 如果 `Physical GPUs` 不可见：
> - VM 必须是 Generation 2（Gen 1 不支持 GPU-PV）
> - VM 必须是 Off 状态
> - host 必须有 **Windows 11 Pro / Enterprise**（Home 不支持 GPU-PV）
> - host BIOS 必须开 **VT-d / IOMMU**

或者用 PowerShell（admin）：

```powershell
# 列出 host 可分配的 GPU
Get-VMHostPartitionableGpu

# 分配给 VM
Add-VMGpuPartitionAdapter -VMName WINDEV2407EVAL -InstancePath "\\?\PCI#VEN_10DE&DEV_...#..."

# 可选：限制分配 VRAM 比例（默认 100%）
Set-VMGpuPartitionAdapter -VMName WINDEV2407EVAL -MinPartitionVRAM 80000000 -MaxPartitionVRAM 80000000 -OptimalPartitionVRAM 80000000
```

### Step 4：VM 启动 + 验证

1. `Hyper-V Manager` → `WINDEV2407EVAL` → `Start`
2. 远程桌面 (RDP) 进 VM (192.168.2.30 改 RDP 进 172.31.157.136，或者 mstsc /v:172.31.157.136)
3. 在 VM 内 PowerShell (admin) 跑:

```powershell
cd C:\moxiang
powershell -ExecutionPolicy Bypass -File scripts\vm-gpu-verify.ps1
```

**期望输出**:
- `Verdict: PHYSICAL_GPU`（绿）
- `nvidia-smi: 存在 ✓` （如果有 NVIDIA 独显）
- PnP 列表里出现 `[PCI 物理] NVIDIA GeForce ...` 或 `[PCI 物理] AMD Radeon ...`

如果仍报 `WARP_ONLY`：
- 回到 Step 2，确认 host 端 GPU 驱动已卸载
- 确认 VM 是 Gen 2
- 确认 host BIOS VT-d 开启
- host `Get-VMGpuPartitionAdapter -VMName WINDEV2407EVAL` 看是否真的分配成功

---

## 方案 B：DDA (Discrete Device Assignment) — 整卡独占

适用：需要把整张 GPU 给 VM 独占（性能极限，比 GPU-PV 强 10-20%）

⚠️ DDA 配错会蓝屏 host，**先备份 host 关键数据**。

```powershell
# host 端 (admin)
$gpu = Get-PnpDevice -Class Display | Where-Object { $_.InstanceId -like 'PCI*' }
Dismount-VMHostAssignableDevice -Force -DevicePath $gpu.InstanceId
Add-VMAssignableDevice -VMName WINDEV2407EVAL -DevicePath $gpu.InstanceId -LocationPath "..." 
```

DDA 配完 VM 启动后，host 上对应物理显示器会**全黑**（GPU 完全给 VM），必须 RDP 进 VM。

---

## 方案 C：SR-IOV（如果 host GPU 支持）

**NVIDIA Tesla / Quadro / RTX Server 系列**、**AMD Radeon Pro** 才支持 SR-IOV。消费级 GeForce / Radeon 不行。

```powershell
Get-VMHostPartitionableGpu | Where-Object { $_.PartitionVRAM -gt 0 } | Format-List Name,PartitionVRAM,PartitionCount
```

如果输出非空，按方案 A 走即可（SR-IOV 自动启用）。

---

## 配完后回 VM 自检

VM 内 PowerShell 跑：

```powershell
cd C:\moxiang
.\scripts\vm-gpu-verify.ps1
```

期望看到 `PHYSICAL_GPU` 绿色 verdict，JSON 报告写到 `modern/docs/restoration-plan/gpu-pv-report.json`。

报告存好后,通知 agent：

> "GPU-PV 配完，VM 自检 verdict=PHYSICAL_GPU"

我会立刻跑 `scripts\visual-smoke.ps1` 截 6 状态 .tga，然后用 `scripts\visual-compare.py` 跟 baseline `ca64d007` 比 SSIM。

---

## 撤销 (回退 host 端)

如果 VM 用完后想 host 重新用物理 GPU：

```powershell
# host 端 (admin)
Remove-VMGpuPartitionAdapter -VMName WINDEV2407EVAL

# 让 host 重新识别物理 GPU
# Scan for hardware changes in Device Manager → 装 NVIDIA/AMD 驱动
# 或:
pnputil /scan-devices
```

host 端装回 GPU 驱动 → 桌面恢复 GPU 加速。

---

## 风险与边界

| 风险 | 影响 | 缓解 |
|---|---|---|
| host 端 GPU 驱动卸载后 host 桌面变软显 | host 用 RDP 体验降级 | 配完 VM 后 VM 拥有 GPU，host 走 RDP 看 VM 即可 |
| GPU 资源被 VM 占满 | host 端无 GPU 加速 | 接受，配完撤销即可 |
| 物理 GPU 不支持 GPU-PV | 分配失败 | 换 DDA 或换 GPU |
| host 端是 Win11 Home | 根本不支持 GPU-PV | 必须 Pro / Enterprise / Education |
| VM 端 RDP 显示可能仍走 RemoteFX | 截屏拿到 RDP 二次合成图 | 用 `psr /start` 或禁用 RDP 视频压缩 |

---

## 参考

- MS Learn: GPU-PV (Hyper-V): https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/plan/plan-for-gpu-acceleration
- MS Learn: DDA: https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/manage/manage-discrete-device-assignment
- `nvidia-smi` (VM 内): `nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv`
- agent 配套: `scripts/vm-gpu-verify.ps1` + `modern/docs/restoration-plan/gpu-pv-guide.md`
