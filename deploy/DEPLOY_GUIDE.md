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

原版打包入口为 `scripts/package-original-vm112-current.py`，安装包使用同源的 `deploy/scripts/start_current_vm112.ps1`；该打包器同时支持旧扁平构建目录和 VS `Release/` 子目录，并在清单中记录源码 SHA、dirty 状态、部署文件、5 个监听端口和 4 个资源指针。打包时通过 `--map-number`（以及可选 `--enable-map17`）要求精确的 `Resource/Server/Monster_*.bin`，缺件会在生成包前失败；启动包装器仍在任何进程启动前复核同一门禁。当前 stage 固定为 `C:\Moxiang\original\stage-20261009`，端口为 Login/Agent/Map10/Map12/Map2 `16001/17001/18001/18002/18003`，数据库为 CT930 `192.168.2.238` 上的 `MoxiangOriginalTest_20261009`。2026-10-11 已生成候选包 `artifacts/original-vm112-r16b-20261011-r3.zip`，对应源码 `671dd94562363583070039aba95450d030358e87`，SHA-256 为 `56f5e4d316dccb692214ffb69692a4f6020cc6d8c0b5667499388141383ca020`，清单 `sourceDirty=false`；它尚未传输或部署到 VM112。Unity使用自己候选工作树的VM112打包和启动入口。启动包装器限定实际主机、stage目录、分线端口和独立数据库，并核对完整文件摘要、canonical资源清单、schema及writer contract。缺失或不匹配即拒绝启动。

通用 `deploy/scripts/start_modern.ps1` 仅供PVE包装器使用；无参数默认只读status，实际start/restart/stop要求VM112主机名及地址。`start -DryRun`只检查启动计划，`restart/stop -DryRun`拒绝执行。它不授予维护权限，也不能替代部署包装器的完整契约验证。

维护由一个协调过的管理拥有者串行执行：先记录两线六服身份及清单、保全目标数据库和旧工件；目标线停写、备份CHECKSUM/VERIFYONLY完成后才替换已审查包。启动和协议/数据库回归后核对未更新线的连续性。失败保持原始回执并按已验证备份回退，不盲目重启全部服务。

VM112当前正常管理使用已验证的加密WinRM身份；QGA仍Stop Pending，停止进一步服务恢复动作。凭据只从私有配置在内存中传递，不放在命令行、Git或日志。数据库测试（包括临时SQLite测试）也在PVE执行；本机共享SQL、其他项目数据库和凭据配置保留。

旧本机一键安装、默认SA/loopback及历史完成声明已从此活动入口移除。旧文可从Git历史和已逐文件验证的私有WIP归档恢复；见 [历史证据索引](../docs/HISTORICAL_EVIDENCE_INDEX.md)。本轮部署成功、GUI通过和完整交付是不同结果，分别在验收矩阵记录。

## 原版 pak 第二层（TYJ-120，当前挂起至 M5）

`deploy/scripts/deploy_client.ps1` 可以在 PlayDH 之外再放一层原版 pak。不传新参数时，脚本仍只做原来的客户端部署。这一层必须和 TYJ-120 同一批合入；当前挂到 M5，在 TYJ-120 合入前不要对验收环境启用。只有原版里有的混淆 `.map` 在 TYJ-119 合入前不得部署。本步骤不复制 `.map`，也不要手工把这类文件放进客户端目录。

脚本常量与 storage 约定一致：

- `$kClientResourceRootRelative = 'data\PlayDH'`：部署目录下的客户端资源根。`FilesystemFileStorage` 目前只扫描这个目录的直接子级 `*.pak`，PlayDH 的 `Map.pak`、`Effect.pak`、`monster.pak`、`npc.pak` 等都在这里，不在 `Resource\` 下。
- `$kOriginalPakSubdir = 'Original'`：与 storage 常量 `kOriginalPakSubdir = "Original"` 同名同大小写。第二层路径是 `<DeployDir>\data\PlayDH\Original`。本分支的 `FilesystemFileStorage` 源码里还没有这个符号；TYJ-120 合入时应使用同一拼写。

```text
<DeployDir>\
  data\PlayDH\                 客户端资源根（第一层，PlayDH）
    Map.pak
    Effect.pak
    monster.pak
    Original\                  第二层，只补 PlayDH 缺的文件
      Map.pak
      Effect.pak
      manifest.json
```

原版包和 PlayDH 包同名，不能放在同一个目录。`-OriginalPakSource` 指向只读源目录（例如 `D:\MX\client\Package\3DData` 这种直接放着 `*.pak` 的目录），脚本只读它的直接子级 `*.pak`，不递归，不复制其他文件，不对源目录做任何写入。

```powershell
.\deploy\scripts\deploy_client.ps1 -OriginalPakSource 'D:\MX\client\Package\3DData' -DryRun
.\deploy\scripts\deploy_client.ps1 -OriginalPakSource 'D:\MX\client\Package\3DData'
```

`-DryRun` 只打印将要复制的文件名、字节数和 SHA-256，不创建、不修改文件。正式执行时，先记录部署目录里 PlayDH 那套 pak 的 SHA-256，复制后再核对一遍不变。源目录不存在、没有 pak、复制失败，或复制后大小 / SHA-256 不一致，都以非零退出。`Original\manifest.json` 只记录每个包的 `fileName`、`bytes`、`sha256`、`sourcePath`。源里已经不再提供、但 `Original\` 里还留着的直接子级 `*.pak` 会删掉；PlayDH 根上的 pak 不动。

回滚只删除部署目录下的 `data\PlayDH\Original`，删除前后核对 PlayDH pak 的 SHA-256，不读取、不修改 `-OriginalPakSource`：

```powershell
.\deploy\scripts\deploy_client.ps1 -RemoveOriginalPaks -DryRun
.\deploy\scripts\deploy_client.ps1 -RemoveOriginalPaks
```

不能同时使用 `-OriginalPakSource` 和 `-RemoveOriginalPaks`。`Original\` 若是目录联接或符号链接，脚本会拒绝复制和删除，避免写到或删到链接目标。

本分支的 `FilesystemFileStorage` 还不会挂 `Original\`。另外，`start_modern.ps1` 的 profile 清单会递归统计资源根里的全部文件；在 TYJ-120 同一批让清单忽略 `Original\` 之前，不要把这一层放进会被整树校验的 PlayDH 目录。用临时目录造假 pak 的检查在 `deploy/scripts/tests/test-original-pak-layer.ps1`。
