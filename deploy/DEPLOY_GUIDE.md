# Moxiang 测试部署入口

当前只部署 PVE 测试环境。本机运行客户端、编译及无需数据库的测试。开发计划见 [ROADMAP](../ROADMAP.md)，已部署版本、资源摘要与验证结果见 [重整状态](../docs/REORGANIZATION_STATUS.md)。本页不提供正式发布流程。

| 分线 | 客户端工作树 | VM112 Login / Agent / Map | 数据库 |
|---|---|---|---|
| 原版 DX11 | `worktrees/original-integration` | `192.168.2.244:16001 / 17001 / 18001` | CT930 `192.168.2.238` 上的原版独立测试库 |
| Unity | `worktrees/unity-integration` | `192.168.2.244:26101 / 27101 / 28101` | CT930 上的 Unity 独立测试库 |

工作树相对项目容器 `E:\Workspaces\moxiang`。主 `repo` 仍保留待分类 WIP；不得直接把它当作已验收部署源。VM100仅用于旧环境对照，CT105原件保留，不参与当前部署或数据库迁移。

## PC 客户端

在相应候选工作树阅读 [人工验收入口](../docs/HUMAN_ACCEPTANCE.md)。两线参数和构建目录不同：原版使用当前已绑定构建中的 Launcher/Client，Unity使用当前已绑定 Player/原生插件。先运行各自的计划模式，核对明确远端、分线端口、资源和二进制SHA后再启动客户端。

不运行本机数据库安装、旧一键部署、SQLite三服启动或VM100迁移脚本。原资源保持 `playdh-current`，Unity派生资源及原创美术独立存放。

## PVE 服务与数据库

原版打包入口为 `scripts/package-original-vm112-current.py`，安装包使用 `deploy/scripts/start_original_vm112.ps1`；该打包器同时支持旧扁平构建目录和 VS `Release/` 子目录，并在清单中记录源码 SHA、dirty 状态、11 个部署文件和 4 个资源指针。2026-10-11 已生成候选包 `artifacts/original-vm112-r16b-20261011-r2.zip`，SHA-256 为 `a6cc416acb34ee1357f86683f1b0fc8bf6da768377e7d4dd615420c45935582e`，清单 `sourceDirty=false`；它尚未传输或部署到 VM112。Unity使用自己候选工作树的VM112打包和启动入口。启动包装器限定实际主机、stage目录、分线端口和独立数据库，并核对完整文件摘要、canonical资源清单、schema及writer contract。缺失或不匹配即拒绝启动。

通用 `deploy/scripts/start_modern.ps1` 仅供PVE包装器使用；无参数默认只读status，实际start/restart/stop要求VM112主机名及地址。`start -DryRun`只检查启动计划，`restart/stop -DryRun`拒绝执行。它不授予维护权限，也不能替代部署包装器的完整契约验证。

维护由一个协调过的管理拥有者串行执行：先记录两线六服身份及清单、保全目标数据库和旧工件；目标线停写、备份CHECKSUM/VERIFYONLY完成后才替换已审查包。启动和协议/数据库回归后核对未更新线的连续性。失败保持原始回执并按已验证备份回退，不盲目重启全部服务。

VM112当前正常管理使用已验证的加密WinRM身份；QGA仍Stop Pending，停止进一步服务恢复动作。凭据只从私有配置在内存中传递，不放在命令行、Git或日志。数据库测试（包括临时SQLite测试）也在PVE执行；本机共享SQL、其他项目数据库和凭据配置保留。

旧本机一键安装、默认SA/loopback及历史完成声明已从此活动入口移除。旧文可从Git历史和已逐文件验证的私有WIP归档恢复；见 [历史证据索引](../docs/HISTORICAL_EVIDENCE_INDEX.md)。本轮部署成功、GUI通过和完整交付是不同结果，分别在验收矩阵记录。
