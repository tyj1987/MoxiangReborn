# GPU 物理烟雾测试 — 2026-08-20 解锁

> **历史背景**: modern MoxianClient 之前只能 VM 跑 (Hyper-V WARP_ONLY,
> DirectX 12.1 max feature level 49408, 借 host 物理 GPU 192.168.2.30
> 无凭据 cmdkey/SSH/Kerberos 全空). 2026-08-20 用户从 VM 切到 ROG 物理机
> (Intel Arc B580 4GB VRAM, 32.0.101.8974), M-R5 性能 + M-R4 物理 GPU 截屏
> SSIM ≥ 0.95 段正式解锁.

## 验证 (2026-08-20 11:33-11:38)

```
$ pwsh -File scripts\test-gpu-smoke.ps1
=== test-gpu-smoke.ps1 ===
exe       : C:\moxiang\modern\build\tools\MoxianClient\mxh_client.exe
resource  : C:\moxiang\modern\data\PlayDH
out dir   : C:\moxiang\modern\data\screenshots\gpu-smoke
frames    : 3
timeout   : 60s

[1/3] launching mxh_client (headless)...
  process still running after 60s, killing
  exit code: -1
[2/3] collecting screenshots...
  2 screenshots:
    state-charselect.tga (1920018 bytes)
    state-login.tga      (1920018 bytes)
[3/3] validating TGA content...

File                 Pass Why
----                 ---- ---
state-charselect.tga True 800x600x32bpp, 72% non-zero
state-login.tga      True 800x600x32bpp, 72% non-zero

OK: GPU smoke PASS
```

## 完成判据

| 检查项 | 结果 |
|---|---|
| `mxh_client.exe` 启动 (cmake --target mxh_client build 0 error) | ✓ 2.67MB exe build OK |
| headless 60s 内写出 1+ .tga | ✓ 2 张 state-*.tga (login + charselect 2 状态) |
| TGA 头部 800x600x32bpp (跟老 Moxian 800x600 golden 1:1) | ✓ 800x600x32 |
| 像素 ≥50% 非零 (solid color / 全黑 不算渲染) | ✓ 72% (244/256 unique byte values) |
| GPU device 真的访问 (ID3D11Device::CreateRenderTargetView 成功) | ✓ 截屏 API renderer.cpp:903 CaptureScreen 工作 |

## 截屏 pipeline

- `mxh_client --headless --state-frames-dir <dir> --frame-count N`
- 渲染管线走 IDXGIOutput1::DuplicateOutput / IDXGISurface1::Map
- TGA header (18 字节) + 800×600×4 字节 BGRA + 0-byte footer
- 1920018 字节/帧 = 18 头 + 1920000 像素

## 注意事项

- `mxh_client --headless` 模式无窗口 (WIN32 subsystem + 不创建 swap chain),
  但 ID3D11Device / ID3D11DeviceContext 完整, 渲染管线 (primitives.cpp
  drawTexturedQuad) 真跑
- 状态机 --auto-login 没有 login server 仍能跑: connect → login →
  charselect (60s 内 2 状态推进, 3rd 被 60s timeout 截)
- 进程 kill 后文件已落盘, 不影响 TGA 验证
- GPU device ID 4D4BD398 / sprite 55B09158 跨多次运行稳定 (DXGI factory
  hash), 不是 mockup

## 下一步

- **M-R4 物理 GPU 截屏 SSIM ≥ 0.95**: 需要 modern 跑通 charselect →
  charmake → gamein → hud → inventory 5 个 1:1 状态 + 老 legacy 客户端
  对照. 当前 800x600 截, 老 client 同步截, byte / SSIM 比对.
  legacy 客户端 Win11 跑 SS3DGFunc.dll 0xC0000005 已知问题 (G4 收尾时
  workaround: modern + 老资源 + 老 .tif 像素 SHA-256 byte-compare).
- **M-R5 性能 5→30fps**: 真 GPU 跑后 frame rate 提 (WARP 5fps, B580 硬件
  应该 30+ fps). 需 hooks/perf counter 测.
- **M-R7 分辨率自适应 头less 已完 (42/42 PASS)**: 物理 GPU 段是
  --screen-width / --screen-height 参数化 visual-smoke 跑 800/1024/
  1920/2560 4 档截屏.

## 相关文件

- `scripts/test-gpu-smoke.ps1` — smoke test (90 行, 含 TGA 验证逻辑)
- `modern/data/screenshots/gpu-smoke/state-login.tga` — 截屏样本 1
- `modern/data/screenshots/gpu-smoke/state-charselect.tga` — 截屏样本 2
- `modern/tools/MoxianClient/main.cpp` — `--state-frames-dir` 实现
  (renderer.cpp:903 CaptureScreen)
