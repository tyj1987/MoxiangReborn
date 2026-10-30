# 本机验收入口

本目录是保留 WIP 的整合树，当前 HEAD `b751df25`，不能用旧默认脚本隐式启动本机三服或数据库。当前两条客户端的实际启动参数、构建绑定及进程回收规则，分别由对应候选工作树维护：

| 路线 | 本机工作树与验收手册 | PVE 入口 |
|---|---|---|
| 原版 modern DX11 | `E:\Workspaces\moxiang\worktrees\original-integration\docs\HUMAN_ACCEPTANCE.md` | VM112 `192.168.2.244:16001`；Agent/Map 17001/18001 |
| Unity 重制 | `E:\Workspaces\moxiang\worktrees\unity-integration\docs\HUMAN_ACCEPTANCE.md` | VM112 `192.168.2.244:26101`；Agent/Map 27101/28101 |

先进入所选工作树，阅读该线手册并执行计划模式，核对客户端模式、显式远端主机、提交、二进制和资源摘要后再正常启动。两线有各自的日志、运行目录、三服和可写数据库。本机只运行客户端及无需数据库的测试；含 SQLite 的自动化测试也在 PVE 执行。部署说明见 [DEPLOY_GUIDE](../deploy/DEPLOY_GUIDE.md)。

真实操作按登录→建选角→Map10→移动/NPC/任务→战斗→自然掉落→点击拾取→装备→切图→重登记录。每项对应画面、服务端权威事件及适用的数据库结果。截图数量、探针成功和脚本退出码都不能代替全链完成。工具操作与用户本人验收分开记录。

当前通过和失败只采用 [验收矩阵](VERIFICATION_MATRIX.md)、[分线状态](PLAYABLE_STATUS.md) 和 [Unity 状态](UNITY_REMASTER_STATUS.md) 的本轮证据。没有一条线已完成首个可玩验收；旧 G0–G3 PASS、8 帧即通过、loopback 数据库和隐含本机启停说明已退役，旧字节保留在 Git 历史。
