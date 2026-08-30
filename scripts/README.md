# Moxian-Reborn Scripts

PowerShell 脚本用于现代化运维。

## 脚本清单

| 脚本 | 用途 |
|------|------|
| `setup-modern.ps1` | 一次性初始化：检测工具 → CMake → 构建 → 测试 |
| `start-server.ps1` | 启动/停止/重启服务端（替代原 .lnk） |
| `verify-resource-profile.py` | 只读校验 ResourceProfileManifest、关键文件和 SHA-256 |
| `gui-client-smoke.ps1` | 启动三服并验证客户端状态帧、地图依赖和 GUI 运行证据 |
| `run-human-acceptance.ps1` | 仅由操作者完成真实鼠标键盘验收；支持 SQLite 或仅经进程环境注入的 MSSQL，不注入凭据或输入 |

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
