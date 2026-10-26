# VM112 r2 stage 运行手册

> **由代码推断、未在机上核实**
>
> 目前没有人持有 VM112（`192.168.2.244`）的 WinRM 会话。`C:\Moxiang\original\stage-ct932-20261024-r2` 是否由该目录里的 `deploy\scripts\start_current_vm112.ps1` 拉起，没有在机器上确认。下文根据本分支这份脚本，以及客户端、启动器和 `map10_smoke` 的参数实现整理。有人在 VM112 上按「计划」一节跑通，并核对五个监听端口之后，删掉本段。

r2 的五个监听端口是 Login `36101`、Agent `37101`、Map10 `38101`、Map12 `38102`、Map2 `38103`。脚本不会写死这五个数。它们必须与该 stage 的 `stage-manifest.json` 里 `listenerPorts.login/agent/map10/map12/map2` 逐项相等，计划阶段才会通过。

这套 stage 使用 CT932 克隆库，数据库 schema 为 4。主机、库名、账号和口令只来自私有配置，下文一律写 `<见私有配置>`。

## 必须使用 r2 目录里的那份脚本

stage 根目录由脚本自身位置决定：

```powershell
$stage = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..')).TrimEnd('\')
```

脚本位于 `<stage>\deploy\scripts\start_current_vm112.ps1`，因此 `$PSScriptRoot\..\..` 就是 stage 根。没有 `-Stage` 参数。在仓库或其他 stage 里执行同一文件，会去读那个位置旁边的 `stage-manifest.json`，不会读到 r2。

在 VM112 上只运行：

`C:\Moxiang\original\stage-ct932-20261024-r2\deploy\scripts\start_current_vm112.ps1`

清单里的 `stage` 字符串只做 `*stage-*` 通配，不与当前目录比较。脚本也不检查本机地址是不是 `192.168.2.244`。目录选错时，计划输出的 `stage` 字段会显示实际读到的根目录。

## 计划

不带 `-Start` 只打印计划并返回。成功时的 JSON 字段为 `mode=plan-only`、`stage`、`sourceHead`、`ports`、`map17Enabled`、`startsProcesses=false`。`ports` 的顺序是 Login、Agent、Map10、Map12、Map2。此模式不连接数据库，不读取 `MXH_DATABASE_CONFIG`，不检查端口是否已在监听，也不看 `run\pids-current.json`。

先把 `-DatabaseHost`、`-DatabaseName` 设成私有配置里的值。省略 `-DatabaseName` 时，脚本仍会带上参数默认值，并与清单 `writerContract.database` 比较；不一致会在进入计划输出之前失败。r2 不要省略这两个参数。不要传 `-EnableMap17`。`-AdvertisedAgentAddress` 保持默认 `192.168.2.244`。

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\Moxiang\original\stage-ct932-20261024-r2\deploy\scripts\start_current_vm112.ps1 -LoginPort 36101 -AgentPort 37101 -MapPort 38101 -Map12Port 38102 -Map2Port 38103 -DatabaseHost <见私有配置> -DatabaseName <见私有配置>
```

端口参数先经过 `ValidateRange(1, 65535)`。绑定失败时脚本正文不执行。绑定成功后、打印计划之前，按下面的顺序检查，任何一条失败都抛错并退出：

1. `<stage>\stage-manifest.json` 必须存在，否则 `stage-manifest.json is missing`。
2. `schema` 必须是整数 `2`，且 `stage` 字符串匹配 `*stage-*`，否则 `current stage manifest identity mismatch`。这里的 `schema` 是清单格式，不是数据库 schema。
3. 清单上的 writer contract 必须同时满足，否则 `writer contract mismatch`：
   - `-DatabaseHost` 非空白，且等于 `writerContract.databaseHost`
   - `-DatabaseName` 等于 `writerContract.database`
   - `writerContract.schemaVersion` 等于 `4`
   - `writerContract.resourceProfile` 等于 `playdh-current`
4. `-DatabaseHost` 必须能解析为 IPv4，且不是 loopback、不是 `0.0.0.0`，否则 `An explicit non-loopback IPv4 database host is required.`
5. `-AdvertisedAgentAddress` 必须是 `192.168.2.244`，否则 `VM112 binding mismatch`。这个参数只做拒绝检查；进程参数里的绑定地址和 Agent 地址是脚本写死的 `192.168.2.244`。
6. Login、Agent、Map10、Map12、Map2 五个端口必须互不相同，否则 `Login, Agent and Map ports must be distinct.`
7. 清单 `listenerPorts` 必须与参数相等：`login`=`-LoginPort`，`agent`=`-AgentPort`，`map10`=`-MapPort`，`map12`=`-Map12Port`，`map2`=`-Map2Port`，否则 `stage listener port contract mismatch`。
8. `<stage>\reference\manifests\playdh-current.sha256.json` 必须存在，否则 `resource manifest missing`。
9. `<stage>\modern\data\PlayDH` 下递归得到的文件个数、字节总和必须等于该资源清单的 `fileCount`、`byteCount`，否则 `resource manifest count/size mismatch`。这一步不计算每个资源文件的 SHA-256。
10. 清单 `files` 的每一项必须存在，长度等于 `bytes`，SHA-256 与 `sha256` 一致（大小写不敏感），否则 `missing stage file` 或 `stage hash mismatch`。
11. `<stage>\modern\data\PlayDH\Resource\Server` 下必须有本次地图的 AIGroup 文件：默认 `-MapNumber 10` 时为 `Monster_10.bin`，另外固定要求 `Monster_12.bin` 和 `Monster_02.bin`（地图号小于 100 时后缀是两位）。缺文件则 `missing server AIGroup resource for map ...`。只检查存在，不检查哈希。

`-Start` 也会先跑完这 11 步。上面任何一步失败，都不会创建进程。

## 启动

计划 JSON 的 `stage` 确认为 `C:\Moxiang\original\stage-ct932-20261024-r2`，且 `ports` 为 `36101,37101,38101,38102,38103` 之后，再追加 `-Start`。脚本只用 `[Environment]::GetEnvironmentVariable(-DatabaseConfigEnv, 'Process')` 读取当前进程环境。`-DatabaseConfigEnv` 的默认名字是 `MXH_DATABASE_CONFIG`。在 PowerShell 里先给这个进程变量赋值，紧接着启动脚本；子进程会继承当前环境块。只写入用户级或机器级变量、而当前进程里没有这份值时，脚本读不到。不要把配置写进命令行参数、仓库或日志。脚本匹配配置后不会回显它。

```powershell
$env:MXH_DATABASE_CONFIG = '<见私有配置>'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\Moxiang\original\stage-ct932-20261024-r2\deploy\scripts\start_current_vm112.ps1 -Start -LoginPort 36101 -AgentPort 37101 -MapPort 38101 -Map12Port 38102 -Map2Port 38103 -DatabaseHost <见私有配置> -DatabaseName <见私有配置>
```

`-Start` 在计划检查之后继续做这些事：

1. `<stage>\run\pids-current.json` 已存在则抛 `current stage PID file already exists`，不启动。计划模式不看这个文件。
2. 进程环境变量为空则抛 `protected database config missing or mismatched`。
3. 配置串按分号拆开，下列键必须各出现一次，值大小写不敏感地相等，否则抛出缺项、重复或不匹配：
   - `backend` = `mssql_odbc`
   - `host` = `-DatabaseHost`
   - `database` = `-DatabaseName`
   - `encrypt` = `yes`
   - `trust_server_certificate` = `no`
4. 清单 `writerContract.databaseCertificateName` 为空时，配置里不能出现 `hostname_in_certificate`。非空时，该键必须恰好一次，且等于清单里的证书名。
5. `<stage>\modern\build\tools\MoxianDbTool\mxh_db_tool.exe` 和 `<stage>\deploy\scripts\assert_original_writer_contract.ps1` 都必须存在。
6. 调用该 guard，传入同一个 `-DatabaseName` 和 `-DatabaseConfigEnv`。guard 的库名参数只接受 `^[A-Za-z_][A-Za-z0-9_]*$`。它执行一条只读查询：当前库名等于 `-DatabaseName`，`dbo.modern_schema_version` 的最大 `version` 为 4，并核对登录表、角色表、玩家状态（含四项进度列）、生命值表和四项基础属性列所需的类型、空值、主键与 CHECK。查询结果必须是 `contract_ok` / `1` / `(1 rows)`。失败时抛错，且不会创建服务进程。guard 不建库、不迁移。
7. 五个端口都不能已有 `Listen` 连接，否则 `port already listening`。
8. 创建 `<stage>\run\logs-current`，把 `<stage>\runtime` 加到进程 `PATH` 前面。
9. 按 map10、map12、map2、agent、login 的顺序 `Start-Process`。工作目录是各自 exe 所在目录，标准输出和错误分别写入 `logs-current\<name>.out.log` 与 `.err.log`。每个进程启动后等待 500 毫秒，若已退出则整批失败。
10. 每成功一个进程，就把已启动列表重写到 `run\pids-current.json`。每项有 `name`、`pid`、`exe`、`port`、`sha256`、`startUtc`。`sha256` 是当时 exe 的 SHA-256 小写十六进制。`startUtc` 写在 500 毫秒等待和这次 SHA-256 之后，不是进程创建时间本身。
11. 标准输出再打印一次 JSON：`stage`、`started`、`state`。

进程参数把五个服务都绑在 `192.168.2.244`。Map10 使用 `-MapNumber`（默认 10）和 `-MapPort`；Map12、Map2 的地图号固定为 12 和 2。Agent 注册 `192.168.2.244:<MapPort>`，以及 `12=`、`2=` 两条分线，`--default-map` 为 `-MapNumber`。Login 和 Agent 的参数带 `--legacy`。五个进程都不带 `--use-hsel`。Map 程序自身默认 legacy 打开、HSEL 关闭。数据库后端是 `mssql_odbc`，配置通过 `--db-env` 指向 `-DatabaseConfigEnv`。

任一步失败时，脚本只对**这一次已经启动的 PID** 执行 `Stop-Process -Id -Force`，并删除 `pids-current.json`。它不按进程名结束进程。

## 数据库

只使用这三个名字：

| 名字 | 作用 |
|---|---|
| `-DatabaseConfigEnv` | 进程环境变量的名字，默认 `MXH_DATABASE_CONFIG` |
| `-DatabaseHost` | 与清单 `writerContract.databaseHost` 比较，并写入配置键 `host` |
| `-DatabaseName` | 与清单 `writerContract.database` 比较，并写入配置键 `database`；也传给 writer-contract guard |

三者的值都是 `<见私有配置>`。不要把主机、库名、账号或口令写入本仓库、命令行历史或日志。脚本不迁移 schema，也不提供停库或切库开关。

不要连接、迁移或停止 CT930。r2 的主机和库名以 r2 清单的 writer contract 为准。

## 健康检查

先确认五个端口都在监听，并且监听进程就是 r2 的 PID 文件所记录的进程：

- `36101` 对应 `login`
- `37101` 对应 `agent`
- `38101` 对应 `map10`
- `38102` 对应 `map12`
- `38103` 对应 `map2`

`OwningProcess` 等于该项 `pid`，进程映像路径等于该项 `exe`，且路径位于 `C:\Moxiang\original\stage-ct932-20261024-r2\` 之下。只读核对。这一步不结束任何进程。

登录和 `GameInAck` 用已经在跑的服务做 headless 检查。必须带 `--no-spawn`，避免工具再拉起本地三服或初始化 schema。

```text
mxh_client_e2e --no-spawn --login-host 192.168.2.244 --login-port 36101 --agent-port 37101 --map-port 38101 --account <见私有配置> --password-env <口令环境变量名>
```

这四个端点参数在 `modern/tools/MoxianClientE2E/main.cpp` 里都有解析。`--login-host` 默认是 `192.168.2.244`，也可用环境变量 `MXH_LOGIN_HOST`，显式参数优先。`--login-port`、`--agent-port`、`--map-port` 的默认值是 `16001`、`17001`、`18001`。

`--no-spawn` 时，工具连到 `--login-host`:`--login-port`，收到 LoginAck 后把 Agent 端口改成 `--agent-port` 再连 Agent。`GameIn` 走这条 Agent 会话，地图号默认 `--map-number 10`。`--map-port` 在 `--no-spawn` 下不负责拨号，只参与端口互异检查；真正把 Map10 指到 `38101` 的是 r2 Agent 启动时的 `--map-server`。成功时日志包含 `LoginAck received` 和 `GameInAck received`，进程退出码为 0。

工具在拨号前会从当前目录和 Map 可执行文件的上级目录寻找 `modern\data\PlayDH` 或 `data\PlayDH`。找不到时打印 `unable to locate canonical PlayDH root for MapServer` 并以退出码 3 结束，此时还没有连接 r2。账号下已有角色时，工具跳过建角；没有角色时会向当前服务所连的库插入角色。只对 r2 做这次检查。

账号用 `--account`。口令放在 `--password-env` 所指的环境变量里，不要使用 `--password` 把口令放进命令行。省略这两项时，工具使用编译进去的默认测试账号，该账号不一定存在于 r2 的克隆库。不要加 `--init-schema`。不要加 `--use-hsel`：按本脚本启动的服务没有打开 HSEL。

`MXH_MAP10_SMOKE_HOST`、`MXH_MAP10_SMOKE_LOGIN_PORT`、`MXH_MAP10_SMOKE_AGENT_PORT`、`MXH_MAP10_SMOKE_MAP_PORT` 在本仓库中不存在。`modern/tests/unit/client/map10_smoke.cpp` 只读取 `MXH_MAP10_SMOKE` 和 `MXH_MAP10_SMOKE_BUDGET`，另可用 `MXH_CLIENT_E2E_BIN` 指定二进制。打开 `MXH_MAP10_SMOKE` 时，测试固定执行 `mxh_client_e2e --map-number 10 --timeout 30 --no-spawn`，不传主机和端口，因此落到默认的 `16001/17001/18001`。不能用它作为 r2 的健康检查。

图形客户端和启动器的参数不一样：

- `MoxianLauncher` 解析 `--login-host`、`--login-port`、`--agent-port`、`--map-port`，默认同样是 `192.168.2.244` 与 `16001/17001/18001`。它会把四个参数原样转给 `mxh_client`。
- `mxh_client` 只解析 `--login-host`、`--login-port`、`--map-port`。`--agent-port` 会被忽略。`--map-port` 只写入启动日志，连接路径不用它。图形客户端的 Agent 地址和端口来自 LoginAck；Login 由 r2 的 Login 进程以 `--agent-addr 192.168.2.244 --agent-port 37101` 通告。进图走 Agent 会话。
- 图形客户端要指向 r2 时，使用 `--login-host 192.168.2.244 --login-port 36101`。`--agent-port 37101 --map-port 38101` 可以放在启动器上，但当前 `mxh_client` 不根据它们改连接。

## 回滚

只停止 r2 的 `C:\Moxiang\original\stage-ct932-20261024-r2\run\pids-current.json` 里记录的 PID。对每一项先核对，再 `Stop-Process -Id`：

- 进程映像路径等于该项 `exe`，并且位于 r2 stage 目录内。
- 进程创建时间早于该项 `startUtc`。`startUtc` 写在 500 毫秒等待和对 exe 做 SHA-256 之后，间隔通常是数百毫秒到数秒。创建时间晚于 `startUtc`，或间隔达到分钟，说明 PID 已复用或文件已过期，不要结束该 PID。
- 需要时再比较 exe 的 SHA-256 与该项 `sha256`。

禁止按进程名结束 `mxh_login_server.exe`、`mxh_agent_server_CHINA.exe` 或 `mxh_map_server_CHINA.exe`。这些名字在其他 stage 上同时存在。

不要触碰：

- r18：`C:\Moxiang\original\stage-r18-20261009`，由 `deploy/scripts/start_original_vm112.ps1` 固定使用 `36001/37001/38001`。该脚本拒绝其他端口。
- 默认 current 端口 `16001/17001/18001/18002/18003`。本分支脚本的默认值就是这五个端口；`deploy/DEPLOY_GUIDE.md` 与 `deploy/VM112_STAGE.md` 把对应目录写成 `C:\Moxiang\original\stage-20261009`。
- CT930。不要连接、迁移或停止这套库。r2 的失败回滚只结束上面核对过的 PID。

客户端回滚是去掉为 r2 加上的端口参数。去掉之后，启动器、`mxh_client` 和 `mxh_client_e2e` 回到代码里的默认端点 `192.168.2.244:16001`，E2E 和启动器的 Agent/Map 默认端口是 `17001/18001`。这会重新指向默认端口上的服务。回滚客户端时不要去停默认 stage。

## 和口头推断不一致的代码事实

- 本分支没有 `stage-3f355242-20261015` 这个路径。默认端口 `16001/17001/18001/18002/18003` 在文档和 `start_current_vm112.ps1` 参数默认值里对应 `C:\Moxiang\original\stage-20261009`。
- 计划阶段核对的是清单字段和资源计数。只读 SQL writer contract、配置环境变量和“端口未被占用”只在 `-Start` 之后执行。
- 计划输出的键是 `mode`，值是 `plan-only`，另有 `startsProcesses=false`。它不检查本机是不是 VM112。
- 上述四个 `MXH_MAP10_SMOKE_*` 主机/端口变量不存在。现有 smoke 测试打的是默认端口。
- `mxh_client` 没有 `--agent-port`。`--map-port` 不参与进图。四个参数同时生效的是 `mxh_client_e2e` 和启动器；E2E 在 `--no-spawn` 下不用 `--map-port` 拨 Map。
- 当前启动脚本不给服务加 `--use-hsel`。仓库里 2026-10-24 的 CT932 证据命令带过客户端 `--use-hsel`，与这份脚本不一致，不能据此认为正在运行的 r2 开了 HSEL。
