# Moxian-Reborn Scripts

PowerShell 脚本用于现代化运维。

## 脚本清单

| 脚本 | 用途 |
|------|------|
| `setup-modern.ps1` | 一次性初始化：检测工具 → CMake → 构建 → 测试 |
| `start-server.ps1` | 启动/停止/重启服务端（替代原 .lnk） |
| `verify-resources.ps1` | 资源完整性校验（TODO） |
| `convert-resources.ps1` | 资源格式转换（TODO） |

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
```

## 执行策略

如果 PowerShell 阻止脚本运行：

```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```