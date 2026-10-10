# VM112 原版三服 staging（current）

仅适用于 `192.168.2.244` 的当前候选 stage。此目录中的原版 CHINA x86 Debug Login、Agent、Map、dbtool 与 `playdh-current` 清单同源。原资源从 VM112 的 `C:\Moxiang\data\PlayDH` 按清单只读复制到 stage 内的 `modern\data\PlayDH`，原目录不作修改；当前启动器在读取 stage 副本全部 5132 个资源哈希通过后才允许启动。`Resource/Map/95.hfl` 是已核对的补齐文件，不能从清单中省略。

打包只读取本候选工作树自身的资源，不依赖主 repo 路径。部署清单记录实际 sourceHead 与源码补丁 SHA；提交后重新打包必须产生新的清单与文件哈希，旧 r5 产物仍按其原始清单核验。writer contract 限定此测试库的 schema v1、必要列、四个基础属性列的类型/空值/约束及资源摘要，不固定到过期 Git HEAD。

默认仅执行只读预检并输出计划：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\Moxiang\original\stage-20261009\deploy\scripts\start_original_vm112.ps1
```

仅在数据库拥有者确认恢复和账户权限后，给当前进程注入受保护的 MSSQL ODBC 配置，再显式传入数据库主机和 `-Start`。配置必须有 `backend=mssql_odbc;host=<DatabaseHost>;...`，且数据库主机不得是本机或 loopback。启动器不会写入或回显该配置，也**不会执行 schema migration**。不得使用本包内保留作参考的 `start_modern.ps1` 启动，它的旧流程会自动执行 `mxh_db_tool migrate`。

三服端口固定默认 Login `16001`、Agent `17001`、Map10 `18001`，均绑定 `192.168.2.244`；Agent 路由 Map12 `18002`、Map2 `18003`。LoginAck 通告 Agent 地址 `192.168.2.244`。此 stage 仅接受 CT930 `192.168.2.238` 的新建隔离库 `MoxiangOriginalTest_20261009`，并要求 `encrypt=yes;trust_server_certificate=no`。`-Start` 前会只读查询库名、schema v1 和三服必需列；任何一项不符都不会创建服务进程。PlanOnly 不连接数据库。PVE VM112 的 CHINA 服务部署与数据库连接需分别验收；stage 和预检成功不等于服务已启动或客户端已登录。Map12 若使用旧兼容 stage，必须在运行收据中标注其来源，不能宣称四个服务二进制同源。

数值候选要求 `character_info.base_gengol/base_minchub/base_cheryuk/base_simmek` 四列均为 nullable `INT`、无 `DEFAULT`，各自有启用且可信的 `0..65535` CHECK。schema version 仍是 1，因此只看 version=1 不能证明已迁移。迁移前应备份隔离测试库并记录四列/旧行状态；停住建角与三服后，只对 `MoxiangOriginalTest_20261009` 执行 dbtool migrate，重复迁移应保留旧角色 `NULL` 与已导入值。新 Agent 与 Map 必须在同一维护窗口切换；旧 Agent 写出的 `NULL` 角色会被新 Map 拒绝入图。回滚程序时保留新增列和原数据，不执行 `DROP COLUMN` 或旧行回填。旧角色属性未知时返回 `GameInNack`，须经独立、可审计的原值导入后才可进入地图。
