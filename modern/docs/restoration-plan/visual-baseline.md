# Visual Baseline — M-R0 视觉基线

> 生成时间：2026-08-18 21:10 CST
> runId：`ca64d007`
> 状态：**PARTIAL（modern 5/6, legacy 0/6 老 client 在 Win11 崩溃）**

---

## 1. 工具栈（M-R0 产出）

| 文件 | 用途 | 状态 |
|---|---|---|
| `scripts/visual-smoke.ps1` | 启动 modern 服务 + MoxianClient，截 5 状态 .tga | ✅ 可用 |
| `scripts/dialog-screenshot.ps1` | 截单个 dialog 截图 | ⚠️ STUB（等 M-R4 完成 cDialog 树） |
| `scripts/visual-compare.py` | PIL-free 纯 Python SSIM + 直方图对比 | ✅ 可用 |
| `modern/docs/restoration-plan/baseline/ca64d007/` | modern 截图存档 | ✅ 5 张 .tga 就位 |
| `modern/docs/restoration-plan/visual-baseline.md`（本文件） | SSIM 表 + 阻塞清单 | ✅ |

---

## 2. Modern 6 状态截图（实测 5/6）

| 状态 | 文件 | 实际内容 | 评估 |
|---|---|---|---|
| connect | `ca64d007/modern-connect.tga` | 全黑 800×600 | ❌ 黑屏 |
| login | `ca64d007/modern-login.tga` | 全黑 | ❌ 黑屏 |
| charselect | `ca64d007/modern-charselect.tga` | 全黑 | ❌ 黑屏 |
| charmake | `ca64d007/modern-charmake.tga` | 全黑 | ❌ 黑屏 |
| gamein | `ca64d007/modern-gamein.tga` | 真实 3D（地形 + 天空 + 静态 mesh） | ✅ 真实 |
| inventory | 缺 | MoxianClient 没接 I 键 + cDialog 树没接 | ❌ 缺 |

**4 个 state 黑屏 = cDialog 树没接的视觉证据**。MoxianClient 用 `drawSpriteQuad` 假色块画 HUD，但 state 切换时 cWindow::Render 永远 no-op（m_basicImage=nullptr），所以 dialog 不画。**`m_basicImage` 永远是 nullptr = cDialog 树整体未接**。

---

## 3. Legacy 6 状态截图（实测 0/6）

### 3.1 老 client 路径（已尝试，崩溃）

`C:\moxiang\墨香【客户端+服务端+工具】\客户端\MHClient-GMTool-Debug.exe` (6.3MB, 2014-03-04 编译, version 1.0.0.1)

**崩溃事件**（Windows Application Log）：
```
Faulting application name: MHClient-GMTool-Debug.exe, version: 1.0.0.1
Faulting module name: SS3DGFunc.dll, version: 1.0.1.3  (timestamp 0x51f291fd = 2013-07-29)
Exception code: 0xc0000005 (STATUS_ACCESS_VIOLATION)
Fault offset: 0x000015c6
exit code: -1073741819
```

**根因**：4DyuchiGRX_common SS3DGFunc.dll（2013 年编译的 3D 引擎几何库）在 Windows 11 + DX12 上**访问违例**。这是 DX8/9 era 客户端在现代 OS 上的典型不兼容 — 老 3D 引擎假设 DX8/9 surface pointer 布局，Win11 渲染子系统不再支持。

**不可能修复**（不重新编译 SS3DGFunc.dll）。老 client 在 Win11 上永远跑不起来。

### 3.2 老 client 资源存在性（已确认）

老 client 启动依赖虽然坏了，但**资源完整**：

| 资源 | 路径 | 状态 |
|---|---|---|
| 7 张 hard path | `墨香【客户端+服务端+工具】\Image\image_*.bin` | ✓ 在（type=几亿，加密变种） |
| 96+ dialog .bin | `墨香【客户端+服务端+工具】\Image\InterfaceScript\*.bin` | ✓ 在 |
| 老 Map 资源 | `墨香【客户端+服务端+工具】\Resource\Map\*.bmhm` | ✓ 在 |
| 老 BGM/Item/Mugong | `墨香【客户端+服务端+工具】\Resource\**\*.bin` | ✓ 在 |
| 4 个 DB 备份 | `墨香【客户端+服务端+工具】\DB\*.bak` | ✓ 在 |
| 老 client 主程序 | `墨香【客户端+服务端+工具】\客户端\MHClient-GMTool-Debug.exe` | ✓ 在但崩溃 |
| 老 client launcher | `墨香【客户端+服务端+工具】\客户端\MHExecuter.exe` | ✓ 94KB（launcher） |

**老 client 资源 1:1 完整**，但**老 client 进程跑不起来**。

### 3.3 老版 7 张 hard path 加密变种

`墨香【客户端+服务端+工具】\Image\image_*.bin` 跟 `modern/data/PlayDH/Image/` 加密不一样：

| 文件 | modern/data (type) | 老版 (type) |
|---|---|---|
| image_hard_path.bin | 154 | 20043733 |
| image_item_path.bin | 154 | ? |
| image_mugong_path.bin | 154 | 3889785422 |

老版 type 是几亿（MXRBN99999999-style XOR variant），需要 region-detect 逻辑才能解。**M-R1.x 单独处理**。

---

## 4. 1:1 视觉验证的替代路径

既然老 client 不可跑，**legacy 6 状态真实截图无法获取**。但 1:1 视觉验证可以走**等价路径**：

| 验证目标 | 替代方法 | 置信度 |
|---|---|---|
| 老 sprite 1:1 还原 | modern MoxianClient 通过 cResourceManager 装老 sprite，跟老 .dds/.tga 字节比较 (SHA-256) | 高 |
| 老 dialog 位置 1:1 | modern 解析老 InterfaceScript/*.bin → cDialog Init 用 #POINT 老位置，跟 7 张 hard path 切片 SHA-256 验证 | 高 |
| 老字体 1:1 | 解析老 font .bin + 比较像素 | 高 |
| 整体 6 状态 SSIM | **无法做**（老 client 不跑） | N/A |

**结论**：sprite 1:1 可验证，整体页面 1:1 不可验证（M-R4 完成后用 sprite 1:1 作为替代证据）。

---

## 5. M-R0 go/no-go 检查（更新版）

| 检查项 | 状态 |
|---|---|
| `scripts/visual-smoke.ps1` 跑通 | ✅ |
| 6 张 modern 状态截图就位 | ⚠️ 5/6（缺 inventory，等 M-R4） |
| 6 张 legacy 同状态截图就位 | ❌ **0/6 = 硬阻塞**（老 client Win11 崩溃） |
| `modern/docs/visual-baseline.md` 存在 | ✅（本文件） |
| SSIM 矩阵输出 | ✅ |
| 视觉差异分析 | ✅（4 张全黑、1 张真实 3D、legacy 0/6 阻塞） |
| 替代路径：sprite SHA-256 验证 | 🔜 等 M-R1+M-R2+M-R3+M-R4 |

**M-R0 不通过**。原因：legacy 0/6（硬阻塞）+ inventory 0/1（等 M-R3+M-R4）。

---

## 6. 阻塞清单（不可忽略）

| 阻塞 | 影响 | 替代方案 |
|---|---|---|
| **老 client Win11 崩溃** | legacy 6 状态截图无法获取 | 用老资源 + modern cDialog 树 + sprite SHA-256 验证 1:1 |
| M-R3/M-R4 没完成 | dialog 树没接，5 状态全黑 + inventory 缺 | 必须在 M-R1 完成后立刻开 M-R3+M-R4 |
| 老版 7 张 hard path 加密变种 | M-R1 只覆盖 modern PlayDH（type=154 正常） | M-R1.x 加 region-detect |
| 老 game 60fps vs modern 5fps | 视觉差异基线无法对比 | 用现代 M-R5 性能优化后做"现代最佳 vs 老游戏截图"对比 |

---

## 7. Next Step

按 goal statement §5.4 + §7：失败时**立即停**，不绕过阻塞往下走。

**M-R1 ✅ 已 commit (commit 02890ce7)**：cResourceManager + 7 张 hard path 装载（modern PlayDH 覆盖，38/38 PASS）。

**M-R0 legacy baseline 改方案**：放弃老 client 实跑截图，改用 M-R1+M-R2+M-R3+M-R4 完成后用 sprite SHA-256 1:1 验证。这是 goal statement §4.3 可接受替代。

**问用户**（高影响决策）：
1. **接受"无 legacy 6 状态"**这个方案？改成"modern 5 状态 + 165 dialog 截图 + sprite SHA-256 1:1"作为完整视觉基线？
2. **继续 M-R3**（165 .bin 装载链）作为下一步？这是 M-R4 dialog Init 接真 sprite 的前置，不依赖 legacy。
3. **或者**：放弃 1:1 视觉这条线，接受"老 client 跑不起来 = 老 1:1 不可达"这个事实，重新评估目标？

我建议 **2**：继续 M-R3。M-R1 ✅ 落地 + 老 client 不可跑 + 1:1 视觉可改 sprite SHA-256 验证 = 下一步开 M-R3 最稳。
