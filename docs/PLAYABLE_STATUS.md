2026-10-19 | 原版 VM112 资源镜像门禁真实通过：包 `36008e0d51659a7ff423fcc382dd7f73a9ecbad8b43920e0b00635a1528c8681` 镜像后核验 5,297 文件 / 1,691,253,485 字节，并确认 Map10/12/2 精确服务端 AIGroup；回执见 [`original-vm112-resource-mirror-gate-20261019.json`](evidence/original-vm112-resource-mirror-gate-20261019.json)。
2026-10-19 | Map203 新 stage 已使用提交 `16f09967681f3f3c1e944348f2ecd7e8e101dd82` 的启动前资源预检：在任何 `Start-Process` 前明确阻断缺失 `Monster_203.bin`，隔离端口为空且默认端口未触碰。回执见 [`original-map203-runtime-resource-gate-20261019-r2.json`](evidence/original-map203-runtime-resource-gate-20261019-r2.json)。
2026-10-19 | Map203 资源包在独立 VM112 stage 上完成更新后的 Map10 真实回归：当前 E2E 五步全通过，GameIn 收到 228 个怪物，测试账号/角色行和隔离进程均清理，默认端口未触碰。回执见 [`original-pve-map10-updated-resource-playability-20261019.json`](evidence/original-pve-map10-updated-resource-playability-20261019.json)。
2026-10-17 | Map203 更新资源包已通过 VM112 隔离 stage 传输：包 SHA-256 `fa91cf475bd68f10cb6b280cb16c085f74130b0195091773da54eb9689d908b5`，资源 5,297 文件 / 1,691,253,485 字节，预检通过；真实启动在 `Monster_203.bin` 缺失处按保护逻辑阻断，新增只读检查 `modern/tools/audit_map_server_resources.py 203` 同样返回缺失，默认端口未触碰。回执见 [`original-map203-runtime-resource-gate-20261017.json`](evidence/original-map203-runtime-resource-gate-20261017.json)。
2026-10-17 | 当前原版提交 `0eb971250bf8b71d7be040d1333d3fb30be8fede` 已完成出生地图路由修复：注册 Map17 路由时建角写入 Map17，无对应路由时安全回退配置默认地图。`mxh_server_handler_tests` 162/162、商业 smoke 35/35 通过；MSSQL 与真人 GUI 仍显式跳过。证据见 [`original-canonical-birth-map-route-20261017.json`](evidence/original-canonical-birth-map-route-20261017.json)。资源缺口与 1 级首战门禁仍未关闭。

# 可玩状态与分线验收
2026-10-17 | 原版 Map203 精确资源门禁取得实质进展：从与已认证 `203.hfl` 同源的 `D:/MX/client/Package/3DData/Map.pak` 提取并校验 161 个 `tile_201a-*` DDS，源包 SHA-256 `00fab5a3ebae755cda6a500f254d60b21c4d64fb0664744a6de6ad20091f68a3`，全部为 128×128 DXT1；`audit_map_dependencies.py 203` 返回 `missing_texture_count=0`。清单已扩展为 5,297 文件 / 1,691,253,485 字节。回执见 [`original-map203-texture-admission-20261017.json`](evidence/original-map203-texture-admission-20261017.json) 与 [`original-map203-textures.provenance.json`](../reference/manifests/original-map203-textures.provenance.json)。剩余原版精确资源阻塞为 HFL 22、TTB 18、Map17 使用纹理 9。
2026-10-17 | 原版资源准入提交 `647d43bb06633001b8f445bef09df26f65954faa` 重新执行 `commercial-smoke.ps1`：35 项中 34 通过、0 失败、1 项部署 manifest 因构建未携带部署包而按环境跳过；MSSQL 与 GUI 均按显式参数跳过。结构化回执见 [`original-commercial-smoke-20261017-r2.json`](evidence/original-commercial-smoke-20261017-r2.json)，不能据此关闭完整商业、MSSQL 或真人 GUI 门禁。
2026-10-17 | 原版隔离 stage `C:\\Moxiang\\original\\stage-numericcopy-birth-20261017-fixed` 已用真实 MSSQL `MoxiangOriginalNumericCopy_20261009` 完成登录、Map17 建角/重登、任务开始与未完成回退、Map17→Map12 换图持久化、权威战斗→自然掉落→点击拾取→背包移动→重登、4 个 quick-slot 技能效果，以及 Map12 Dealer `53` 的 18 项商店目录、余额扣减、购买物品 `11040` 与重登库存回读。包 SHA-256 为 `7a144af16e79831bfc39c00f28ab3d2501cc9c609bca650ce3a424f6c490ce20`；资源清单 5,136 文件、1,689,474,757 字节。测试结束已恢复角色 Map17、余额 74，删除临时商店物品和技能 fixture。结构化回执见 [`original-pve-fixed-stage-playability-20261017.json`](evidence/original-pve-fixed-stage-playability-20261017.json)。

上述是无头协议验收，不能替代真人 GUI 输入、视觉/音频对照、完整 1:1 资源、全地图经济社交覆盖或 24 小时稳定性。最新本地商业 smoke 为 35/35 通过，`MSSQL_E2E` 与 GUI 仍按显式外部门禁单独记录。

2026-10-17 | 原版当前 `modern/build` 构建目录定向回归：资源引用与 E2E 目标共 6 项，5 通过、0 失败、1 个历史恢复 fixture 按预期跳过；`MoxianClientE2E` 和 dump CLI 均通过。证据见 [`docs/evidence/original-targeted-regression-20261017.json`](evidence/original-targeted-regression-20261017.json)。该回归不关闭 Map17/Map203 精确纹理及 HFL/TTB 原版资源缺口。


2026-10-17 | 以当前原版提交 `90b190c82701385b4409169c542be2c3f9484659` 重新扫描 C:/Moxiang、C:/backup、C:/CodexData、D:/MX、D:/MoyoI、D:/03_Data、E:/Workspaces/moxiang 和 E:/Workspaces/artifacts：相关 HFL/TTB 文件 2020 个，Map17 `tile_17_mou_a_01..09.dds` 命中 0 个，Map203 的 161 个 `tile_201a-*` 纹理命中 0 个；18 个缺失 HFL/TTB 名称只对应已知占位或 UnityNative 派生副本。没有复制或准入任何候选，原版精确资源门禁仍为 HFL 22、TTB 18、Map17 9、Map203 161。证据见 [`docs/evidence/original-resource-candidate-scan-20261017.json`](evidence/original-resource-candidate-scan-20261017.json)。

## 2026-10-15 当前源码阶段：Map10→Map12 与商店闭环

本次回执绑定原版提交 `f264e8af5afa4f6e5f6636dcb8a5f8d2f38ca469` 与隔离阶段 `C:\Moxiang\original\stage-f264e8af-20261015`，不再把旧 `stage-20261015` 的 `21fe3513` 二进制当作当前源码运行证据。新阶段通过 QGA 传输并校验包 SHA-256 `972550c6eda59e064655b94c6d6d6de4623f408d392b174f65cbff182fdbcdc5`，使用 `playdh-current` 的 5,135 个资源文件。

随后已按精确进程路径停止旧原版 `stage-20261015`，保留其目录作为回滚证据；当前源码阶段已提升到 VM112 默认原版端口 `16001/17001/18001/18002/18003`，Unity 端口与进程未触碰。默认端口切换收据见 [`original-pve-default-current-source-20261015.json`](evidence/original-pve-default-current-source-20261015.json)。

真实 MSSQL PVE 收据完成 Map10 GameIn（228 个怪物）→Map12 ChangeMapAck→Map12 GameIn（18 个 NPC，Dealer `52/53`）→目录 18 项→购买物品 `11040`→断开重登后库存仍为 1，退出码 0。购买前先用受控金钱 fixture 证明余额不足返回 `BuyNack`，再注入 100,000 金钱完成成功路径；地图、金钱与临时购买行随后恢复/删除，隔离端口 `16211/17211/18211/18212/18213` 全部释放。结构化收据见 [`original-pve-current-source-shop-map12-20261015.json`](evidence/original-pve-current-source-shop-map12-20261015.json)、[`original-pve-current-source-mapchange-20261015.json`](evidence/original-pve-current-source-mapchange-20261015.json)，失败分类收据见 [`original-pve-shop-current-source-map12-no-money-20261015.json`](evidence/original-pve-shop-current-source-map12-no-money-20261015.json)。

这关闭了当前源码在 PVE 上的 Map10→Map12 路由、Dealer 发现、商店目录、余额拒绝、购买入包和重登持久化子门禁。Map10 仍没有原始 Dealer 行，不能把 Map12 的商店 NPC 解释为 Map10 1:1 资源完成；真人 GUI、全地图资源、完整经济/社交系统和商业发布门禁仍保持 PARTIAL。

## 2026-10-15 当前源码阶段：Map10 战斗→掉落→拾取→重登闭环复核

同一 VM112 默认端口和当前源码 stage 使用可回滚战斗夹具（level 53、装备 `11000/23000/27000`，临时移除任务行以满足当前权威战斗事务前置）完成 LoginAck、角色选角、GameIn、Map10 目标 `50143` 命中与生命变化、自然掉落 `item=8008`、点击拾取、背包 `MoveAck source=0 target=1`、GameOut 后重登持久化；怪物数量 `228→227`，退出码 0。结构化回执见 [`original-pve-default-current-source-combat-20261015.json`](evidence/original-pve-default-current-source-combat-20261015.json)，完整日志见同名 `-equipped-20261015.log`。

测试后角色 `100017` 已恢复为 level 1、Map10、基础属性 12/12/12/12、生命 125、护盾 245、MP 125、无装备/物品，任务日志 1 行与子目标 8 行均恢复。无装备负对照只收到 Skill 广播而没有权威伤害，见 `original-pve-default-current-source-combat-20261015.log`；因此这次闭环不能升级为 1 级新玩家首战平衡或商业完成。当前源码提交为 `2c33c6adb895853fa676856f22f433f13031354a`，运行包仍绑定 `f264e8af5afa4f6e5f6636dcb8a5f8d2f38ca469` stage。

## 2026-10-15 原版 Map10 商店资源缺口回执（历史阻塞仍有效）

在同一 VM112 原版 Map10 真实进入游戏并枚举 NPC 时，没有发现 canonical Dealer，因此 Map10 商店目录请求和购买动作没有发送，进程以退出码 2 结束。回执 [`original-pve-shop-current-20261015.json`](evidence/original-pve-shop-current-20261015.json) 与日志 [`original-pve-shop-current-20261015.log`](evidence/original-pve-shop-current-20261015.log) 保留了这一失败证据；没有发生余额或背包写入。Map12 商店链已在顶部当前源码回执中独立通过。

该结果把商店链明确定位为 Map10 运行时 NPC/资源缺口：需要先恢复与原版一致的 Dealer，再验证目录、扣款、入包和重登持久化，不能把工程级商店单测当作 PVE 商业验收。对工作区内 16 份 `Dealitem.bin` 的字节和解码内容复核显示它们全部为同一 SHA-256，且没有任何 Map10 行；来源审计见 [`original-dealitem-map10-provenance-audit-20261015.json`](evidence/original-dealitem-map10-provenance-audit-20261015.json)。因此当前不能用 Map12 或原创 NPC 冒充原版 Map10 资源。

## 2026-10-15 原版当前提交任务开始与未完成回执

在 VM112 当前原版 stage 使用隔离账号 `origpve15` 进入 Map10，收到 228 个怪物后从 canonical QuestScript 选择任务 1。客户端真实发送任务开始请求并收到 `Quest StartAck`，状态为 `Active - hunt monsters`；随后发送未满足目标的结束请求，收到 `EndNack`，状态恢复为 `Not complete`，退出码 0。结构化回执与完整日志见 [`original-pve-quest-current-20261015.json`](evidence/original-pve-quest-current-20261015.json) 和 [`original-pve-quest-current-20261015.log`](evidence/original-pve-quest-current-20261015.log)。

这关闭了当前原版任务开始、状态同步和未完成结束恢复子门禁；任务目标完成/奖励领取、经济与商店、真人 GUI、完整原始资源及商业发布仍保持 PARTIAL。本回执没有把未完成任务误报为奖励完成。

## 2026-10-15 原版 Map10→Map12 切图持久化回归

当前原版 VM112 stage 使用真实 MSSQL 账号 `origpve15` 完成 Map10 GameIn（228 个怪物）后发送权威 `ChangeMapSyn`，收到 Map12 `ChangeMapAck`，进入 Map12 后读到 228 个怪物和 2 个 NPC；断开并重新 GameIn 仍落在 Map12，退出码 0。测试结束后将该专用角色的持久化地图恢复为 Map10，并通过 CT930 非 SA 查询回读。证据见 [`original-pve-mapchange-current-20261015.json`](evidence/original-pve-mapchange-current-20261015.json) 和 [`original-pve-mapchange-current-20261015.log`](evidence/original-pve-mapchange-current-20261015.log)。

这关闭了当前原版 PVE 的 Map10→Map12 权威路由与重登位置持久化子门禁；完整地图覆盖、真人 GUI、缺失原始资源和商业发布门禁仍保持 PARTIAL。

## 2026-10-15 原版当前提交 Map10 战斗闭环回归

在 VM112 当前原版 stage `C:\Moxiang\original\stage-20261015` 上，使用隔离测试账号 `origcombat15` 和临时等级 53 角色 `Combat161015`，当前 Release `MoxianClientE2E` 真实完成 LoginAck、角色列表/选角、Map10 GameInAck、服务端权威攻击、目标死亡、自然掉落、点击拾取、背包 `MoveAck` 以及断开后的重登持久化。Map10 怪物数量由 `228` 降至 `227`，目标 `50143` 初始生命 `6150`，掉落 `drop=90000,item=50015,count=1`，退出码 0。测试账号、角色、状态、生命、装备、物品、任务和技能临时行已删除；原有角色 `100017` 回读为 level=1、Map10、基础属性 12/12/12/12、生命 125、护盾 245、MP 125、dead=0、revision=1。完整日志和结构化回执见 [`original-pve-combat-current-20261015.log`](evidence/original-pve-combat-current-20261015.log) 与 [`original-pve-combat-current-20261015.json`](evidence/original-pve-combat-current-20261015.json)。

这次回归关闭了当前原版服务包在真实 MSSQL PVE 上的 Map10 权威战斗→自然掉落→点击拾取→背包写入→重登回读子门禁；它仍不等于真人 GUI 验收、1 级新玩家首战平衡、完整原始资源、全地图/经济/社交覆盖或商业发布就绪。

## 2026-10-15 VM112 当前原版运行回执

VM112 已恢复隔离的原版 modern 三服/多地图阶段：`16001/17001/18001`，以及 Map12/Map2 的 `18002/18003`；Unity 阶段独立使用 `26101/27101/28101`。真实 `MoxianClientE2E --no-spawn` 使用专用账号 `origpve15` 完成 LoginAck、角色列表、建角、重列、选角和 Map10 GameIn，收到 **228 个怪物** 与 NPC `554/572`，玩家生命 `125/125`，退出码 0。当前 stage manifest SHA-256 为 `f44982a74ebba1a11a25545129b9e2a6582b91ba802e610498295f0292e0a77d`，服务包源提交为 `21fe351340d5e0ec240d5aef2c0204ca6db49d5d`；日志 SHA-256 为 `76e0fc6eb7dcad7da3840266d0b563755d12ed302cacf2f000bf01d1a0551888`。结构化回执见 [`original-pve-headless-20261015.json`](evidence/original-pve-headless-20261015.json)。这是原版协议与 GameIn 工程通过，仍需真人 GUI、战斗/掉落/拾取持久化、完整原始资源和商业发布验收。

## 2026-10-15 当前原版资源准入与 PVE 协议回执

本次原版资源准入回执绑定服务包提交 `36e3b33f20518a04edfc44f7c5ccc0017808f02b`；当前分支继续在 GitLab `codex/reorg-original-20261009` 上快进维护。VM112 独立 stage `C:\Moxiang\original\stage-20261015` 已用当前服务二进制和 PlayDH 资源启动五服端口 `16111/17111/18111/18112/18113`；远端核验为 **5,135 文件 / 1,688,281,785 字节**，资源 manifest SHA-256 为 `6b994c106ff068ad03fcbad7b21f28ff78715a8427d0480acb255d598689754e`。`3.hfl`、`95.hfl`、`198.hfl`、`203.hfl` 的远端哈希均与本地准入清单一致；Map.pak 的嵌入资源审计见 [`original-map-pak-resource-audit-20261015.json`](evidence/original-map-pak-resource-audit-20261015.json)。

资源格式回归脚本已适配当前 Resource Explorer 输出格式，并真实核验 7 个 PAK 的条目数、MonsterList/ItemList 元数据及两个代表性条目提取；结构化回执为 [`original-resource-format-verify-20261015.json`](evidence/original-resource-format-verify-20261015.json)。

真实 `MoxianClientE2E --no-spawn` 已通过 LoginAck、AgentConnect、建角、选角、GameInAck 和 Map10 **228 个怪物**；退出码 0，日志回执为 [`original-pve-resource-admission-20261015.json`](evidence/original-pve-resource-admission-20261015.json)，日志 SHA-256 为 `8ce7cdc1a4368efbfa49cee5d44b38c80c7ec42556b4aded944c6ff56cfda3ef`。测试账号密码只在进程内使用，没有写入证据。完成后仅按 stage 路径精确停止五个进程，五个隔离端口均已释放。

UnityNative 中与原版缺失项重名的 10 个 TTB 已逐项拒绝进入原版 1:1 线：它们全部是 `native-authored-development-only`、`releaseReady=false` 的派生数据；原版对应路径仍为空。边界回执为 [`original-ttb-isolation-audit-20261015.json`](evidence/original-ttb-isolation-audit-20261015.json)。这次 PVE 回执只关闭当前资源包的启动与协议/GameIn 子门禁；1 级 Map10 战斗平衡、自然战斗掉落/拾取、完整原始资源覆盖、真人 GUI 和商业发布仍未闭合。


## 2026-10-14 原版 PVE 隔离实例复核

最新复核（绑定原版提交 `f1fa3211e3cd132d09166555b140f0f22d8f458b`）：在 VM112 已保全的 `stage-20261014` 上，通过受控 WinRM 会话启动隔离端口 `16101/17101/18101/18102/18103`，真实 Release `MoxianClientE2E` 以 `mxh_s1010` 完成 LoginAck、Agent、角色列表/重列、角色选择和 Map12 GameInAck，退出码 0；技能 1817、任务 238、物品 9887、NPC 106 均加载成功。结构化回执为 `docs/evidence/original-pve-current-stage-20261014.json`，日志 SHA-256 为 `86DCBC9B3A4E8D82B8BA5B26E996F7F0FA336F3AA63A010B9274017E6B1E73C9`。同次 Map10 门禁没有通过：角色 `100012` 在库中持久化于 Map12，因此没有把 Map12 的成功误报成 Map10 228 怪物闭环。当前 stage 源仍为 `a089ad6b`，尚未部署 `f1fa3211` 服务包；真人 GUI、Map10 fixture、战斗/掉落/拾取/装备/换图重登和完整资源准入仍是开放门禁。

当前原版工作树提交为 `6a56df1c`，本轮 PVE 服务包由其父提交 `a089ad6b22e8dc4626a217108429d612554c6726` 构建，新增的 `MoxianClientE2E` 端口参数（`--login-port/--agent-port/--map-port/--map12-port`）则由当前提交构建并通过默认端口 CTest、非默认 `--dump-cli` 解析和远端运行。Visual Studio Release 全量 CTest 重新运行 **12,574/12,574 通过**，总耗时 72.89 秒；17 项明确因 PVE、MSSQL、旧资源或显式外部门禁跳过，未将 Skip 计为通过。

本轮将 Release 包 `original-vm112-release-20261014-r2.zip`（SHA-256 `af2b64960f41ea530a59aa6d30a6ed6bc6eeaa778e95f17e5ce62bd4fb5b0999`）部署到 VM112 的独立 stage `C:\Moxiang\original\stage-20261014`，没有替换旧服务。该 stage 使用 PlayDH 资源 **5,132 文件、1,682,222,623 字节**；补入的 `Resource/Map/95.hfl` SHA-256 为 `fbc336eb9b73a55f50979d4e2678bd760479d69cf49217a5ae268332dbed1746`。manifest SHA-256 为 `d73369562c67e7107211f00fd1f640fbbcb9be32c037fa7876ef6c5ce156e4e6`。

隔离三服实际运行于 `16101/17101/18101`（另起 `18102/18103` 供 Map12/Map2 路由），程序哈希分别为 Login `81355002596f3e93be809ba9150e6b10a9b406f031b20a08ae0f57a3f0b431da`、Agent `03ea891dddf2c75f48af5f8e03d3d8c83b6bb87cc0060ff8b7aae149f4cbba16`、Map `c42dcf2fa468fd8127d8c64b369c9dbf3f981158046dfbae6e6c5de442bdf646`。远端 `MoxianClientE2E --no-spawn` 以真实 GUI 状态类完成 LoginAck、Agent CharacterList、角色重列、CharacterSelect、Map10 GameInAck 和 228 个怪物实例接收，退出码 0；MSSQL 使用 CT930 的 `MoxiangOriginalTest_20261009`，schema/query 只读检查退出码 0，预检读到 13 个角色行。测试结束后仅停止本轮 stage 的精确进程并确认五个隔离端口均无监听；旧 VM100/VM112 服务、Unity 服务和主工作区未触碰。

这关闭了“原版 Release 工程 + PVE MSSQL + 远端协议/GameIn”子门禁，但仍不等于真人 GUI 的自然战斗、掉落点击拾取、装备、切图重登或完整 1:1 资源验收。逐项摘要见 `docs/evidence/original-pve-vm112-20261014.json`。

## 2026-10-14 原版 PVE 真实战斗闭环

本轮先以新建的 1 级角色在 Map10 执行真实战斗门禁，服务器确实收到攻击并扣减护盾，但目标防御 541、角色伤害仅 2–4，目标生命未变化，未产生掉落，退出码 2。该结果确认首个玩家路径存在等级/地图前置缺口，不能通过降低怪物强度或伪造伤害掩盖。

随后在同一隔离 MSSQL 测试库为专用角色 `100013` 建立可回滚的合法高等级测试 fixture，先在原始 Map2、再在目标 Map10 执行真实攻击、死亡、自然掉落、点击拾取、背包移动与重登。两次 LoginAck、角色选择、GameInAck、目标死亡、地面掉落 `item=8008`、拾取、`MoveAck source=0 target=1` 和重登持久化全部通过；Map2 怪物数量由 128 降至 126，Map10 由 228 降至 227，客户端均退出码 0。测试结束后恢复角色、进度、生命值和临时物品行，五个隔离端口均无监听；没有停止旧服务或修改 canonical 资源。完整回执见 `docs/evidence/original-pve-combat-20261014.json`。

这关闭了“原版服务端权威战斗→自然掉落→点击拾取→背包写入→重登回读”的 PVE fixture 子门禁；1 级新玩家在 Map10 的合理首战路线、真人 GUI、装备视觉、切图位置持久化、全地图和商业发布仍保持 PARTIAL。

历史基线（2026-10-10）：当时没有任何一条线通过“首个可玩”验收；历史运行记录已归档。当前结论以本文件顶部的最新回执为准；定义见下方 E0–E5，门禁见 [VERIFICATION_MATRIX.md](VERIFICATION_MATRIX.md)。执行总览见 [重整状态](REORGANIZATION_STATUS.md)，旧状态恢复位置见 [历史证据索引](HISTORICAL_EVIDENCE_INDEX.md)。

## 2026-10-11 最新工程校准

在原版提交 `1b5983d7` 的 VS Release 全量构建后，`ctest --test-dir modern/build-vs-rebuild -C Release --output-on-failure -j 4` 共注册 **12,574** 项，**12,574/12,574 通过**，退出码 0；其中 17 项因外部 MSSQL、PVE、旧资源或显式环境变量门禁而跳过。此前大量 `Not Run` 是未生成 VS 多配置目标，本次已完整构建后消除；回执与原始日志见 `audit/2026-10-09-reorg/original-integration/ctest-full-release-20261011.json` 和 `.log`。这只证明工程测试基线健康，不能替代 PVE 数据库、真实 GUI、完整资源许可和商业发布验收。

## 当前校准（2026-10-11）

当前原版候选分支为 `b762668e`（GitLab `codex/reorg-original-20261009`），服务端单测重新运行 **161/161 通过**，包含自然死亡/掉落/拾取、商店金额持久化、任务和装备重登场景。该结果仍是宿主工程证据；本次端口复核仅确认原版 `16001/17001/18001` 可达，Unity `26101/27101/28101` 不可达，不能把旧文档中的“全部端口可达”继续视为当前事实。PVE 原版修复包 `r16b` 尚未部署，真人 GUI 的自然战斗、掉落拾取、切图和重登闭环仍未通过。

当前提交 `b762668e` 上的 Visual Studio Release CTest `MoxianClientE2E` 重新运行通过 **2/2**（协议 E2E 与 dump CLI，退出码 0）；可执行文件 SHA-256 为 `1b63d4aa0d2612b7ddf5ae75167b9903ad64d897c71d91379d18820ffd777157`，回执见 `audit/2026-10-09-reorg/original-integration/original-e2e-20261011.json`。这仍是本机隔离服务测试，不替代 PVE MSSQL 与真人 GUI 验收。

同一 Release 构建按玩法范围重跑服务端 CTest **125/125 通过**，覆盖路由、Map10 实体、战斗/死亡、自然掉落、近距离拾取、商店、任务、装备和重登；回执为 `audit/2026-10-09-reorg/original-integration/server-playability-ctest-20261011.json`。这显著扩大了工程级闭环证据，但仍不替代 PVE MSSQL 的真实写入和真人 GUI 全链验收。

当前候选包所用的 DB 工具与测试目标已从同一 VS Release 构建生成；`mxh_db_tests.exe` 运行 **35/39 通过、4 跳过、0 失败**。通过项覆盖 SQLite 持久化、DB 线程、schema migration、真实资源解析和 ODBC 负面连接；4 项跳过均明确要求外部 MSSQL 凭据/实例，回执见 `audit/2026-10-09-reorg/original-integration/db-tests-20261011.json`。未把跳过项计为数据库业务验收通过。

本轮最新原版工程基线为提交 `2894804e`：revisioned 玩家使用消耗品时，生命值 CAS、物品扣除、任务进度和背包写入在同一事务中完成；商店购买、丢弃、出售、UseSyn、背包移动/装备栏校验、任务开始持久化、重登恢复和完成奖励防重复领取场景均已启用并通过。击杀任务进度现在会在 revisioned 战斗提交后继续推进，不再因已有任务而拒绝击杀。独立客户端默认端点已统一为 PVE `192.168.2.244`，可通过 `MXH_LOGIN_HOST` 或显式 `--login-host` 覆盖，不再隐式回落 loopback。PVE VM112 的 16001、17001、18001、18002、18003 端口当前可达。上述证据只关闭工程子项，不关闭真人 GUI、完整地图、切图重登或商业发布门禁。

2026-10-10 在重新生成的 Visual Studio 2022 x64 构建目录 `modern/build-vs-rebuild` 中补齐掉落测试目标 `mxh_drop_item_tests`；`DropTableRegistryTest` 与加载器相关测试共 28 项通过（CTest `-I 3092,3119 -C Debug`），`mxh_monster_ext_tests` 直接运行 24/24 通过。此前旧 NMake 构建目录缺少这些可执行文件，不能再将其 `Not Run` 结果视为测试失败。该证据仍属于工程级掉落逻辑验证，尚不替代 PVE 自然击杀掉落、点击拾取和数据库持久化验收。

同一构建目录重新编译 `mxh_client`、`MoxianLauncher` 与 `mxh_client_e2e`；`MoxianClientE2E` 1/1 通过（12.70 秒）。一次按名称筛选的 CTest 因其他目标未生成而出现 `Not Run`，已与实际断言失败区分，不作为回归结论。

随后补建并直接运行 `mxh_server_handler_tests.exe`：161/161 通过，包含自然死亡通知、地面掉落、近距离拾取、写入失败回滚、并发拾取、经验奖励、商店扣款/入包、SQLite 商店价格与金额持久化、任务开始持久化、装备创建/卸下/重登和地图路由场景；当前没有 Disabled 测试。该结果证明当前服务端测试路径一致，但不替代 PVE 数据库和真人 GUI 验收。

## 当前结论

- **原版候选线**：GitLab457/CI746，PVE3565/r12；r14/e244正常Launcher→登录→选角→Map10与HUD字色通过。r12正常建角有DB读回，r13有跨客户端Move广播。本地r15b玩家互见/换图时序、r16死亡移动及fae实际启动wrapper修复独立通过；r16b未部署。复活、NPC/任务、自然战斗、掉落拾取、装备、切图、位置持久化与真人验收仍未闭合。
- **玩法候选及缺口**：`94d6bd0f` 恢复有来源的基础属性和普通装备计算；新角色四属性为12，旧NULL/partial和装备读取失败在创建Actor前拒绝，禁止回填。独立审查、全新x86构建、服务133/游戏251/schema5/守卫4/纯客户端342项通过；真实MSSQL隔离副本两遍迁移、边界与契约负面测试通过，旧行保持原值。后续 `45701197` 只调整逐包日志并重新完成相关构建/回归。完整技能伤害、正常战斗、自然掉落、点击拾取、装备、切图和重登全链尚未通过。
- **Unity候选线**：GitLabad0e096b/CI748；PVE v6b/server84d3a0c5，两轮228/228与既有角色DB回查通过，建角跳过。修复fixture后的handler实际195/195通过，3禁用未运行。Player42fe79dd/core39836319重新构建，真实GUI正常登录、选角、Map10地形及原地复活后HP37/125，窗口正常关闭exit0。角色/怪物在全图概览中不可辨认，正诊断；战斗/拾取与45项地图发布缺口仍未闭合。
- 工具驱动 GUI 或自动化测试只证明对应操作和环境；不等于用户本人验收。两线证据不可互相继承，Unity 测试也不能替代原版对照。

## E0–E5 证据等级

| 等级 | 含义 | 本身不能证明 |
|---|---|---|
| E0 | 文件或包字节已记录并有 SHA-256 | 内容语义正确 |
| E1 | 格式已解码且字段覆盖完整 | 运行时可用 |
| E2 | 运行时对象已实例化且无缺失依赖 | 正确渲染或交互 |
| E3 | 运行时确实渲染或播放该资源 | 正确玩家动作及状态变化 |
| E4 | 正确玩家动作触发资源及状态变化 | 与真实旧版一致且经人工审查 |
| E5 | 真实旧版对照通过并完成人工审查 | RC 稳定性或发布准备 |

## 首个可玩验收边界

原版线至少需要在受控 PVE 环境中完成登录、角色创建/选择、进入 Map10、可辨认地形与角色、HUD、移动、战斗和掉落闭环，并完成真人可见检查；同时保留服务端权威和数据库结果。Unity 线需在独立 PVE Unity 实例和闭合资源依赖下完成相同核心闭环，并单独通过 Unity 画面与交互审查。无缺资源、无替身占位、无协议/玩法语义漂移。单项测试、GUI 脚本或宿主 CTest 均不关闭整体验收。

任何 G 门禁 PASS 都须在矩阵中绑定本轮提交/工作树、命令或操作、结果和证据文件；缺少任一关键证据时保持 PARTIAL、未通过或未运行。
## 2026-10-14 当前提交 PVE 可玩子门禁

原版当前提交 `3bf8192065635394893d2df0b7a847aada201d53` 已在 VM112 隔离 stage 通过真实 Release 协议验收：Map10 专用测试角色收到 **228 个怪物**，Map12 既有角色完成 GameInAck，登录、Agent、角色列表、建角/选角和资源加载均通过。Map10/Map12 日志及包、服务哈希见 [`original-pve-f1fa-stage-20261014.json`](evidence/original-pve-f1fa-stage-20261014.json)。专用账号/角色已清理，旧 stage、默认端口和旧服务已恢复。

这只是 PVE 协议与初始实体门禁；真人 GUI 全链、权威战斗、自然掉落、点击拾取、装备、切图、重登持久化和完整原版资源仍未闭合，`releaseReady=false`。

## 2026-10-14 当前提交战斗验收

当前原版服务包在 PVE Map2 通过真实战斗、掉落、点击拾取、背包移动和重登持久化；Map10 已证明权威攻击与生命变化，但本次没有自然掉落，仍保持失败状态。专用 fixture 已删除，证据见 [`original-pve-f1fa-combat-20261014.json`](evidence/original-pve-f1fa-combat-20261014.json)。
## 2026-10-15 当前校准

本轮状态以以下提交和证据为准，旧段落仅作历史定位：

- 原版工作树 `codex/reorg-original-20261009` 当前提交 `ffa9e97bc341e477259ff55840237e4af8b32ceb`，此前 `f1fcd852` 已修复 CTest 客户端资源工作目录；完整 Release CTest 为 **12,574/12,574 通过、0 失败、14 显式跳过**，回执见 [`original-ctest-current-20261015.json`](evidence/original-ctest-current-20261015.json)。
- 原版 VM112 隔离 stage `stage-20261015` 使用当前构建、PlayDH **5,135 文件 / 1,688,281,785 字节**，并纳入来源核验通过的 `Resource/Map/3.hfl`、`95.hfl`、`198.hfl`、`203.hfl`。真实远端流程完成登录、建角、GameIn、Map10 的 228 个怪物和服务器权威技能事件；装备攻击范围为 29–35，Map10 目标防御 541，1 级角色每击 2–3 点且生命不变。这是合法的等级/地图前置门槛，未伪造伤害或掉落；证据见 [`original-pve-starter-equipment-regression-20261015.json`](evidence/original-pve-starter-equipment-regression-20261015.json) 和 [`original-resource-admission-20261015.json`](evidence/original-resource-admission-20261015.json)。现有高等级 fixture 已通过 Map10 战斗、自然掉落、点击拾取和重登持久化，首个玩家路径仍需按原始等级路线补齐。
- Unity 工作树 `codex/reorg-unity-20261009` 当前提交 `ffac1084c415e31c9cd533dc1caf107ec9480248`；EditMode **279 总计 / 266 通过 / 0 失败 / 13 跳过**，商业 smoke **35/35 通过**，回执见 [`unity-commercial-regression-20261015.json`](../unity-integration/docs/evidence/unity-commercial-regression-20261015.json)。三组公开资源已经隔离接入并通过哈希/许可检查，但仍是 `releaseReady=false`。
- 两线仍保持独立工作树、构建目录和资源目录；旧 stage、旧服务和原版 PlayDH 未被替换。当前未闭合门禁：原版缺失的 TTB/HFL 资源、首个玩家完整 GUI 战斗路线、全地图/经济/社交覆盖、Unity PVE MSSQL 与人工 GUI/最终美术验收，以及双线稳定性和清理回归。

任何完成声明必须绑定当前提交、资源摘要、实际运行证据和人工验收；历史通过记录不自动继承。

## 2026-10-15 任务状态战斗修复复核

原版提交 `3106ec4efbd6d82eb0f4c1cc60bf53d29f5817cb` 已修复 revisioned 战斗在存在任务状态时被提前拒绝的问题。VM112 的受控高等级 fixture 保留任务行，真实客户端收到任务公告、Map10 228 个怪物，并完成目标 `50143`、`50099` 的服务器权威击杀；角色事件和物品/奖励事件均有日志证据。测试结束后 CT930 恢复角色、生命、状态和装备，旧 Map10 服务复位后再次通过登录/选角/GameIn 烟测。回执：[`original-pve-questfix-combat-20261015.json`](evidence/original-pve-questfix-combat-20261015.json)。

当前仍是 `PARTIAL`：本轮 E2E 循环没有到达最终点击拾取/重登断言，且 fixture 仍为高等级和首发装备。不能据此宣称 1 级新玩家路线、自然掉落持久化、完整 GUI 或商业完成。

### 2026-10-15 新账号首战门禁

全新账号已完成真实建角与原始首发装备写入，但当前 `--default-map 10` 仅能在 Map10 对目标 `50143` 造成护盾伤害，无法改变目标生命；这不是装备保存失败，而是测试默认与原始首战路线尚未证明一致。回执 `original-pve-fresh-starter-combat-20261015.json`。清理已验证，无临时账号残留。

### 2026-10-15 Map17 HFL 资源收口

原版 `Map.pak` 实际包含并已恢复准确 `17.hfl`；当前资源阻塞从 23 个 HFL 收窄到 22 个 HFL，Map17 进入九个已使用纹理待恢复状态。Map17 运行闭环仍未执行。

### 2026-10-15 Map17 隔离出生协议通过

在 VM112 原版 stage `stage-f264e8af-20261015` 上，临时端口 `16117/17117/18117` 使用真实 Release `MoxianClientE2E --no-spawn` 完成 LoginAck、空角色列表、CharacterMake、重列、Map17 选角和 GameInAck；随后用 `16118/17118/18118/18119` 完成 Map17 任务 1 的 StartAck/未完成 EndNack、Map17→Map12 的 ChangeMapAck、Map12 GameInAck 和新 GameIn 重登路由。玩家 `100018` 为 1 级、生命 `131/131`、Map17 首次进入成功，Map12 读回包含 216 个怪物和 31 个 NPC。两个探针账号、角色、状态、生命、装备、物品、任务和技能行均已清理为 0，临时端口已释放，`16001/17001/18001/18002/18003` 与 Unity `26101/27101/28101` 保持不变。结构化回执见 [`original-pve-map17-e2e-20261015.json`](evidence/original-pve-map17-e2e-20261015.json)。

这只关闭了 Map17 的登录、建角、选角、首次 GameIn、任务开始/未完成拒绝以及 Map17→Map12 路由子门禁；九个准确地形纹理、移动、战斗、自然掉落、拾取、装备、完整重登持久化、真人 GUI 和视觉 1:1 仍未闭合。

## 2026-10-17 原版工程基线复核（当前整合线）

当前 `codex/reorg-original-20261009` 工作树保持干净；在 Visual Studio 2022 x64 Release 支持路径重新构建并运行 `mxh_client_tests`，**355 项中 349 通过、0 失败、6 项明确跳过，退出码 0**。跳过项仅依赖外部 PlayDH、PVE 三服或显式 Map10 smoke 环境变量；没有把 Skip 计为通过。可执行文件、日志哈希与命令见 [`modern-client-x64-regression-20261017.json`](evidence/modern-client-x64-regression-20261017.json)。

旧 `modern-targeted-tests-20261017.json` 记录的 `0xC0000374` 来自旧 Win32/NMake 构建路径；当前项目文档和构建入口要求 VS2022 x64，当前支持路径不再复现该崩溃。旧记录保留作历史诊断证据，不作为当前客户端门禁结论。

同一 x64 Release 构建已补齐 `mxh_client`、Launcher、三服、数据库工具、资源浏览器和 E2E 工具；本地 SQLite 三服链 `MoxianClientE2E` **1/1 通过**。PVE 人工入口的 PlanOnly 检查确认只连接 `192.168.2.244:16001/17001/18001`，不会启动本机服务或数据库；完整人工 GUI 仍需操作者在实际桌面完成，回执见 [`modern-client-x64-build-20261017.json`](evidence/modern-client-x64-build-20261017.json)。

同一 source-bound x64 Release 目录随后完成全目标编译和完整 CTest：**12,577/12,577 实际执行用例通过、0 失败、26 项明确 Skip，退出码 0**，总耗时 130.79 秒。Skip 仅对应 PVE/MSSQL、恢复运行时资源、真实特效资源和显式 Map10 smoke 门禁；回执与构建日志哈希见 [`modern-full-x64-release-20261017.json`](evidence/modern-full-x64-release-20261017.json)。




