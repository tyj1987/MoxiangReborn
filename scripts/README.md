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

原版三服在 PVE 上使用 Login `16001`、Agent `17001`、Map `18001`。PVE 启动 `deploy/scripts/start_modern.ps1` 时须显式设置 `-AdvertisedAgentAddress 192.168.2.244`：LoginAck 使用该通告地址，默认 `127.0.0.1` 会让远程客户端登录后连接自身。Agent 到 Map 的地址由 PVE 内部部署配置决定。

本机客户端可用 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/run-human-acceptance.ps1 -PlanOnly` 核对端点与启动器参数；实际人工验收去掉 `-PlanOnly`。客户端入口不读取本机数据库配置。
脚本退出码只反映截图采集是否达到最少数量；`run.json` 的 `acceptanceStatus=evidence_collected_awaiting_review` 表示 GUI、音频和持久化 checkpoint 仍待人工审阅，不代表验收通过。

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

## 执行策略

如果 PowerShell 阻止脚本运行：

```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```
