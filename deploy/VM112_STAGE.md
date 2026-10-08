# VM112 原版三服 staging（2026-10-09）

仅适用于 `192.168.2.244` 的 `C:\Moxiang\original\stage-20261009`。此目录中的原版 CHINA x86 Debug Login、Agent、Map、dbtool 与 `playdh-current` 清单同源。原资源从 VM112 的 `C:\Moxiang\data\PlayDH` 按清单只读复制到 stage 内的 `modern\data\PlayDH`，原目录不作修改；`start_original_vm112.ps1` 在读取 stage 副本全部 5131 个资源哈希通过后才允许启动。

打包只读取本候选工作树自身的资源，不依赖主 repo 路径。部署清单记录实际 sourceHead 与源码补丁 SHA；提交后重新打包必须产生新的清单与文件哈希，旧 r5 产物仍按其原始清单核验。writer contract 限定此测试库的 schema v1、必要列与资源摘要，不固定到过期 Git HEAD。

默认仅执行只读预检并输出计划：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\Moxiang\original\stage-20261009\deploy\scripts\start_original_vm112.ps1
```

仅在数据库拥有者确认恢复和账户权限后，给当前进程注入受保护的 MSSQL ODBC 配置，再显式传入数据库主机和 `-Start`。配置必须有 `backend=mssql_odbc;host=<DatabaseHost>;...`，且数据库主机不得是本机或 loopback。启动器不会写入或回显该配置，也**不会执行 schema migration**。不得使用本包内保留作参考的 `start_modern.ps1` 启动，它的旧流程会自动执行 `mxh_db_tool migrate`。

三服端口固定默认 Login `16001`、Agent `17001`、Map `18001`，均绑定 `192.168.2.244`；LoginAck 通告 Agent 地址 `192.168.2.244`。此 stage 仅接受 CT930 `192.168.2.238` 的新建隔离库 `MoxiangOriginalTest_20261009`，并要求 `encrypt=yes;trust_server_certificate=no`。`-Start` 前会只读查询库名、schema v1 和三服必需列；任何一项不符都不会创建服务进程。PlanOnly 不连接数据库。PVE VM112 的 CHINA 服务部署与数据库连接需分别验收；stage 和预检成功不等于服务已启动或客户端已登录。
