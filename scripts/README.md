# Moxian-Reborn Scripts

PowerShell 脚本用于现代化运维。

## 脚本清单

| 脚本 | 用途 |
|------|------|
| `setup-modern.ps1` | 一次性初始化：检测工具 → CMake → 构建 → 测试 |
| `start-server.ps1` | 启动/停止/重启服务端（替代原 .lnk） |
| `verify-resource-profile.py` | 只读校验 ResourceProfileManifest、关键文件和 SHA-256 |
| `gui-client-smoke.ps1` | 启动三服并验证客户端状态帧、地图依赖和 GUI 运行证据 |
| `run-human-acceptance.ps1` | 原版 DX11 客户端人工验收；默认连接 PVE `192.168.2.244:16001`，不启动本机三服或数据库，不注入凭据或输入 |
| `rog-gate-build.ps1` | ROG 试合入门禁：SHA 或 CMake 生成器变化时干净重建；稀疏检出里若有重解析点则直接拒绝；核对磁盘后再编译并跑 CTest |

原版三服在 PVE 上使用 Login `16001`、Agent `17001`、Map `18001`。PVE 启动 `deploy/scripts/start_modern.ps1` 时须显式设置 `-AdvertisedAgentAddress 192.168.2.244`：LoginAck 使用该通告地址，默认 `127.0.0.1` 会让远程客户端登录后连接自身。Agent 到 Map 的地址由 PVE 内部部署配置决定。

本机客户端可用 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/run-human-acceptance.ps1 -PlanOnly -BuildDirectory modern/build-gui-r13` 核对端点与启动器参数；实际人工采集须指定同时包含 Launcher/Client 的同源构建目录。客户端入口不读取本机数据库配置。
脚本退出码只反映采集流程是否正常结束；`run.json` 的 `acceptanceStatus=not_evaluated` 表示 GUI、服务事件和数据库证据仍待人工审阅，不代表验收通过。截图数量默认为非门禁。

## 用法

```powershell
# 首次构建
.\scripts\setup-modern.ps1

# 跳过测试
.\scripts\setup-modern.ps1 -SkipTests

# 启动服务端
.\scripts\start-server.ps1 -Mode start

# 停止服务端
.\scripts\start-server.ps1 -Mode stop

# 查看状态
.\scripts\start-server.ps1 -Mode status

# 校验正式资源 profile 的关键文件
python .\scripts\verify-resource-profile.py `
  .\reference\manifests\playdh-current.sha256.json `
  --root .\modern\data\PlayDH --profile-id playdh-current
```

## ROG 试合入门禁 (`rog-gate-build.ps1`)

Windows 构建机（ROG）上手动把 mainline 和 PR head 试合入后，用这个脚本编译并跑 CTest。构建目录如果还留着上一份 trial-merge SHA，或者 `CMakeCache.txt` 里的 `CMAKE_GENERATOR` 和这次不一致，测试会打到旧二进制。现网门禁目录（例如 `D:\build-mxr\pr20`）是 **x86（Hostx86/x86）+ NMake Makefiles**，不是 Ninja。

本脚本不改 `build-modern.bat` / `build-modern.ps1`。那条路径仍用 Ninja，给仓库里的常规构建用。

门禁行为：

- 在任何 git 操作、编译或删除之前（和 10 GB 磁盘下限同一处）检查 SourceDir。稀疏检出已启用，并且工作区里有 junction、symlink 或其他重解析点时，打印稀疏状态和这些路径，退出码 1。没有跳过开关。只有稀疏检出，或只有重解析点，允许继续。稀疏判定满足任一即可：`git config --get core.sparseCheckout` 为 true，`git sparse-checkout list` 列出模式，或 git 目录里的 `info/sparse-checkout` 存在且仍有模式。`.git` 是文件时按 `gitdir:` 解析，不跟随重解析点。扫描工作区时遇到重解析点就记下路径并停止下探。原因是 ROG 上 git 2.55 在稀疏 worktree 里 checkout 会穿过 junction，删掉另一个目录里的文件。
- `git -C SourceDir rev-parse HEAD` 必须等于 `-ExpectedSha`（完整 40 位）。不等就非 0 退出。
- `-BuildDir` 解析后必须严格位于允许根之下（默认 `D:\build-mxr`，可用 `-AllowedBuildRoot` 改）。不能等于根本身，也不能是或位于 `D:\MX`。允许根不能是盘符根。
- 删除前检查 BuildDir 自身、它的祖先，以及目录里的每一项。出现 junction、symlink 或其他重解析点就拒绝并退出，不跟随、不透过它删除。删除函数本身遇到重解析点也会停。
- `BuildDir\rog-gate.sha` 缺失或和 ExpectedSha 不同，或者已有 `CMakeCache.txt` 的 `CMAKE_GENERATOR` 与 `-Generator` 不同，就整目录干净重建。印记只在本次构建成功之后写入。配置或编译失败则非 0 退出，不跑测试，也不写印记。
- 跑 CTest 之前检查 BuildDir 里的测试可执行文件：至少 `*_tests.exe`，再加上 CTest `add_test` 登记且落在 BuildDir 里的 `.exe`。干净重建时，任一测试 exe 的修改时间早于本次构建开始就拒绝测试。印记已经对上本次 SHA 的增量构建允许旧时间戳。
- CTest 命令是 `ctest --test-dir <BuildDir> -C <Config> --output-on-failure`，`-CtestArgs` 原样接在后面，退出码原样返回。`-SkipTests` 只编译并写印记。
- 磁盘预检在删除、配置和编译之前。去重后检查 BuildDir 所在盘、SourceDir 所在盘，以及 **E:**。10 GB（1024^3 字节）是硬下限。`-MinFreeGB` 和每一项 `-DriveMinFreeGB` 只能把阈值调高；低于 10 会立刻打印错误并以退出码 1 结束，不做后续检查。没单独覆盖的盘使用 `-MinFreeGB`（默认 10）。任一盘低于其阈值就非 0 退出，并打印每块盘的剩余空间和允许根下最大的顶层目录。统计大小时不跟随重解析点。

`-Generator` 默认 `NMake Makefiles`，也可以设为 `Ninja`。两种都先调用 `C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86`。Ninja 再传入与 `build-modern.bat` 相同的 `CMAKE_MAKE_PROGRAM`。`-WhatIf` 和 `-DryRun` 只做检查并打印将执行的动作。

已有的 `D:\build-mxr\prNN` 目录第一次用这个脚本时还没有 `rog-gate.sha`，会按干净重建删掉。这是预期行为。

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\rog-gate-build.ps1 -SelfTest

powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\rog-gate-build.ps1 `
  -SourceDir E:\work\trial-merge `
  -BuildDir D:\build-mxr\pr20 `
  -ExpectedSha <40位SHA> `
  -PlayDhRoot E:\work\trial-merge\modern\data\PlayDH `
  -Config Debug

powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\rog-gate-build.ps1 `
  -SourceDir E:\work\trial-merge `
  -BuildDir D:\build-mxr\pr20 `
  -ExpectedSha <40位SHA> `
  -PlayDhRoot E:\work\trial-merge\modern\data\PlayDH `
  -Generator Ninja `
  -DriveMinFreeGB E=20 `
  -WhatIf
```

ROG 验收必须传 `-PlayDhRoot`，值为这次门禁 worktree 自己导出的 PlayDH 目录（`<SourceDir>\modern\data\PlayDH`）。门禁主流程不读取已有的 `MXH_PLAYDH_ROOT`；子进程会被改写成这个显式路径。另一份 worktree 或 `D:\MX` 上残留的变量不能静默继承。

`-SelfTest` 不需要编译器。它检查路径安全、生成器判定、盘符去重和剩余空间阈值。

## 执行策略

如果 PowerShell 阻止脚本运行：

```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```
