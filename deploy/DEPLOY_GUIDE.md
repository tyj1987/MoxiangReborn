# 墨香Reborn - 完整部署指南


## Modern local commercial smoke

For a reproducible local three-server run without SQL Server, use the modern launcher:

```powershell
powershell -File deploy\scripts\start_modern.ps1 -Mode start -Locale CHINA
powershell -File deploy\scripts\start_modern.ps1 -Mode status
# run modern/build/tools/MoxianClientE2E/Debug/mxh_client_e2e.exe
powershell -File deploy\scripts\start_modern.ps1 -Mode stop
```

The launcher starts modern Login/Agent/Map on ports `16001/17001/18001`, stores runtime data under `deploy/runtime/modern`, initializes the SQLite demo account on first start, writes per-service logs, records PIDs, performs port health checks, and supports safe stop/restart. Production deployment still requires MSSQL and the production configuration path below.

## 目录

1. [环境要求](#环境要求)
2. [快速部署](#快速部署)
3. [手动部署](#手动部署)
4. [服务端配置](#服务端配置)
5. [客户端配置](#客户端配置)
6. [常见问题](#常见问题)

---

## 环境要求

### 硬件要求

| 组件 | 最低配置 | 推荐配置 |
|------|----------|----------|
| CPU | 双核 2.0GHz | 四核 3.0GHz+ |
| 内存 | 4GB | 8GB+ |
| 硬盘 | 20GB 可用空间 | 50GB+ SSD |
| 网络 | 10Mbps | 100Mbps+ |

### 软件要求

| 软件 | 版本 | 用途 |
|------|------|------|
| Windows | 10/11 或 Server 2016+ | 操作系统 |
| SQL Server | 2019/2022 Express 或更高 | 数据库 |
| Visual Studio | 2019/2022 (可选) | 编译源码 |

---

## 快速部署

### 一键部署（推荐）

```powershell
# 以管理员身份运行 PowerShell
cd "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\deploy\scripts"

# 完整部署
.\deploy_all.ps1

# 或指定服务器IP（局域网部署）
.\deploy_all.ps1 -ServerIP "192.168.1.100"
```

### 分步部署

```powershell
# 1. 安装数据库（需要管理员权限）
.\database\install_database.ps1

# 2. 部署服务端
.\scripts\deploy_server.ps1

# 3. 部署客户端
.\scripts\deploy_client.ps1

# 4. 启动服务
.\scripts\start_all.ps1
```

---

## 手动部署

### 步骤一：安装 SQL Server

1. 下载 SQL Server Express：
   - 访问 https://www.microsoft.com/zh-cn/sql-server/sql-server-downloads
   - 选择 "Express" 版本（免费）

2. 安装 SQL Server：
   - 运行安装程序
   - 选择 "基本" 安装
   - 实例名称：MSSQLSERVER（默认实例）
   - 身份验证模式：混合模式
   - 设置 SA 密码

3. 安装 SQL Server Management Studio (SSMS)：
   - 访问 https://docs.microsoft.com/zh-cn/sql/ssms/download-sql-server-management-studio-ssms

### 步骤二：恢复数据库

```powershell
# 以管理员身份运行
cd "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\deploy\database"
.\install_database.ps1
```

### 步骤三：配置 ODBC 数据源

1. 打开 ODBC 数据源管理器：
   - 运行 `odbcad32.exe`
   - 或在控制面板 → 管理工具 → ODBC 数据源

2. 创建系统 DSN：
   - 选择 "系统 DSN" 选项卡
   - 点击 "添加"
   - 选择 "SQL Server" 驱动
   - 名称：MHCMEMBER
   - 服务器：.\MSSQLSERVER
   - 完成设置

3. 重复创建 MHGAME 和 MHLOG 数据源

### 步骤四：部署服务端

```powershell
cd "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\deploy\scripts"
.\deploy_server.ps1
```

### 步骤五：部署客户端

```powershell
cd "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\deploy\scripts"
.\deploy_client.ps1 -ServerIP "127.0.0.1"
```

### 步骤六：启动服务

```powershell
cd "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\deploy\scripts"
.\start_all.ps1
```

---

## 服务端配置

### 服务器架构

```
┌─────────────────┐
│   客户端        │
└────────┬────────┘
         │
┌────────▼────────┐
│  登录服务器     │ :9000
│  (Distribute)   │
└────────┬────────┘
         │
┌────────▼────────┐
│  代理服务器     │ :10000
│  (Agent)        │
└────────┬────────┘
         │
┌────────▼────────┐
│  地图服务器     │ :10001
│  (Map)          │
└─────────────────┘
```

### 配置文件说明

#### AgentServer.ini

```ini
[Network]
Port=10000              # 监听端口
MaxConnections=2000     # 最大连接数
RecvBufferSize=65536    # 接收缓冲区大小
SendBufferSize=65536    # 发送缓冲区大小

[Database]
DSN=MHCMEMBER          # ODBC 数据源名称
AdminDSN=MHGAME        # 管理员数据源
ThreadCount=2           # 数据库线程数
MaxQueries=512          # 最大查询数

[Server]
ServerSet=1             # 服务器组编号
DistributeIP=127.0.0.1  # 登录服务器IP
DistributePort=9000     # 登录服务器端口
```

#### MapServer.ini

```ini
[Network]
Port=10001
MaxConnections=500
RecvBufferSize=65536
SendBufferSize=65536

[Database]
DSN=MHGAME
AdminDSN=MHGAME
ThreadCount=2
MaxQueries=1024

[Server]
ServerSet=1
MapIndex=1
MapName=首阳
AgentIP=127.0.0.1
AgentPort=10000

[Game]
MaxPlayers=200
MonsterRespawn=300      # 怪物刷新时间（秒）
ItemDropRate=1.0        # 物品掉落率
ExpRate=1.0             # 经验倍率
```

### 端口列表

| 服务 | 端口 | 协议 | 说明 |
|------|------|------|------|
| 登录服务器 | 9000 | TCP | 客户端登录 |
| 代理服务器 | 10000 | TCP | 游戏数据转发 |
| 地图服务器 | 10001 | TCP | 游戏逻辑处理 |
| 监控服务器 | 10002 | TCP | 服务器监控 |

---

## 客户端配置

### 文件说明

| 文件 | 说明 |
|------|------|
| MoxianReborn.exe | 游戏主程序 |
| MHVerInfo.ver | 版本信息文件 |
| MHVerInfo.bin | 服务器列表 |
| Resource\ | 游戏资源文件 |
| Image\ | 图片资源 |
| Sound\ | 音效资源 |
| Data\ | 游戏数据 |
| Ini\ | 配置文件 |

### 修改服务器地址

编辑 `Ini\ServerList.bin`：

```ini
[ServerList]
Count=1
Server1_Name=墨香Reborn
Server1_IP=你的服务器IP
Server1_Port=9000
Server1_Online=0
Server1_Max=1000
Server1_Status=0
```

---

## 常见问题

### Q1: 无法连接到数据库

**症状**：服务器启动失败，提示数据库连接错误

**解决方案**：
1. 检查 SQL Server 服务是否运行：
   ```powershell
   Get-Service -Name "MSSQL*"
   ```
2. 启动服务：
   ```powershell
   net start MSSQLSERVER
   ```
3. 检查 ODBC 数据源是否配置正确

### Q2: 服务器启动后立即退出

**症状**：服务器窗口闪烁后消失

**解决方案**：
1. 检查配置文件是否存在
2. 检查端口是否被占用：
   ```powershell
   netstat -ano | findstr "9000"
   ```
3. 查看日志文件

### Q3: 客户端无法登录

**症状**：客户端提示"无法连接服务器"

**解决方案**：
1. 确保所有服务器已启动
2. 检查防火墙是否放行端口
3. 检查 MHVerInfo.bin 中的服务器地址

### Q4: 游戏资源加载失败

**症状**：游戏画面异常或报错

**解决方案**：
1. 确保 Resource 目录完整
2. 检查文件权限
3. 重新部署客户端

### Q5: 如何修改经验倍率

编辑 `deploy\server\Map\MapServer.ini`：

```ini
[Game]
ExpRate=2.0    # 2倍经验
```

重启地图服务器生效。

---

## 部署检查清单

- [ ] SQL Server 已安装并运行
- [ ] 数据库已恢复（MHCMEMBER, MHGAME, MHLOG）
- [ ] ODBC 数据源已配置
- [ ] 服务端已部署
- [ ] 客户端已部署
- [ ] 防火墙已配置
- [ ] 所有服务已启动
- [ ] 客户端可以登录

---

## 技术支持

如遇问题，请检查：

1. 日志文件位置：
   - 服务端：`deploy\server\Map\Log\`
   - 客户端：`deploy\client\Log\`

2. 常用命令：
   ```powershell
   # 查看服务状态
   Get-Process -Name "DistributeServer","AgentServer","MapServer"
   
   # 停止所有服务
   .\start_all.ps1 -Stop
   
   # 重启服务
   .\start_all.ps1 -Stop
   .\start_all.ps1
   ```
