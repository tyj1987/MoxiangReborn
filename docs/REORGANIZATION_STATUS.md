# 重整执行状态

## 当前状态校准（2026-10-17）

原版资源与代码验证基线为 `647d43bb06633001b8f445bef09df26f65954faa`。精确资源阻塞以最新回执为准：HFL 22、TTB 18、Map17 使用纹理 9；Map203 的 161 个准确纹理已由同源原始包准入，依赖审计 0 缺失；但真实服务启动又确认 `Resource/Server/Monster_203.bin` 缺失，运行门禁仍阻塞，回执见 `docs/evidence/original-map203-runtime-resource-gate-20261017.json`。本轮还修复了建角出生地图路由和 E2E 初始实体消息竞态；服务处理器 162/162、商业 smoke 35/35 通过。独立 VM112 NumericCopy stage 的真实 MSSQL 玩法回执见 `docs/evidence/original-pve-fixed-stage-playability-20261017.json`。本文以下历史段落不覆盖当前数量和提交绑定。

## 2026-10-15 当前源码阶段回执：原版商店链已在 PVE 闭合

原版分支 `codex/reorg-original-20261009` 当前提交为 `f264e8af5afa4f6e5f6636dcb8a5f8d2f38ca469`。本轮将该提交构建为独立 VM112 stage `C:\Moxiang\original\stage-f264e8af-20261015`，包 SHA-256 为 `972550c6eda59e064655b94c6d6d6de4623f408d392b174f65cbff182fdbcdc5`；旧 stage 和 Unity 服务没有替换。

验证完成后，旧原版 `stage-20261015` 进程已按完整路径停止但目录保留，当前源码 stage 已接管 VM112 原版默认端口 `16001/17001/18001/18002/18003`；Unity `26101/27101/28101/28202/28303/28417` 未触碰。默认端口回归收据为 `docs/evidence/original-pve-default-current-source-20261015.json`。

在 CT930 `MoxiangOriginalTest_20261009` 上真实跑通 Map10→Map12、18 项商店目录、余额不足 `BuyNack`、余额充足 `BuyAck`、入包及重登持久化。临时数据库 fixture 已恢复，购买行已删除，隔离端口已释放。收据见 `docs/evidence/original-pve-current-source-shop-map12-20261015.json` 和 `docs/evidence/original-pve-current-source-mapchange-20261015.json`。

旧顶部记录中绑定 `21fe3513` 的 `stage-20261015` 仅作为历史运行证据保留；本阶段结论以当前源码收据为准。Map10 原始 Dealer 资源仍缺失，不能宣称原版 1:1 商店完成。

## 2026-10-15 当前源码阶段回执：战斗掉落闭环

同一默认 stage 使用可回滚的 level 53 角色和原始首发装备 `11000/23000/27000`，临时移除任务行后在 Map10 真实完成权威攻击、目标死亡、自然掉落 `item=8008`、点击拾取、背包移动和重登回读；怪物数量 `228→227`，退出码 0。测试后角色 `100017` 的 level、Map10、12/12/12/12 属性、生命/护盾/MP、装备、物品、任务日志和子目标均恢复，完整收据见 [`original-pve-default-current-source-combat-20261015.json`](evidence/original-pve-default-current-source-combat-20261015.json)。

无装备负对照只产生技能广播而没有权威伤害；当前 MapHandler 对带任务状态的 revisioned 战斗事务仍会拒绝，因此该结果只关闭高等级夹具子门禁，不代表 1 级新玩家首战、任务耦合、真人 GUI 或商业发布完成。文档路线和当前阻塞已同步到 `ROADMAP.md`、`docs/KNOWN_BUGS.md` 和 `docs/VERIFICATION_MATRIX.md`。

## 2026-10-15 当前执行回执：资源准入与分线隔离

本次原版资源准入回执绑定服务包提交 `36e3b33f20518a04edfc44f7c5ccc0017808f02b`，GitLab 长期分支为 `codex/reorg-original-20261009`。VM112 的 `stage-20261015` 已在独立端口用当前服务包完成远端资源清单校验和五服启动；PlayDH 5,135 文件、1,688,281,785 字节，资源 manifest SHA-256 `6b994c106ff068ad03fcbad7b21f28ff78715a8427d0480acb255d598689754e`。真实 E2E 完成登录→建角→选角→Map10 GameIn，收到 228 个怪物并退出码 0；回执见 [`original-pve-resource-admission-20261015.json`](evidence/original-pve-resource-admission-20261015.json)。隔离进程已按完整路径回收，旧原版服务、Unity 服务和数据库实例未被停止。

为保持两线严格分离，UnityNative 与原版缺失项同名的 10 个 TTB 已完成哈希、来源和 `releaseReady` 审计，全部拒绝进入原版 1:1 资源目录；详见 [`original-ttb-isolation-audit-20261015.json`](evidence/original-ttb-isolation-audit-20261015.json)。当前仍存在原始 TTB/HFL 缺口和 1 级 Map10 战斗前置问题，不能据此宣称商业完成。


## 2026-10-14 当前执行回执

2026-10-14 当前原版 PVE 隔离复核已重新执行并绑定本分支提交 `f1fa3211e3cd132d09166555b140f0f22d8f458b`：在 VM112 `stage-20261014` 的 `16101/17101/18101/18102/18103` 上，真实 Release `MoxianClientE2E` 完成 LoginAck、Agent 连接、角色列表、角色重列、角色选择和 Map12 GameInAck，退出码 0；读取到 Skill 1817、Quest 238、Item 9887、NPC 106。回执为 [`original-pve-current-stage-20261014.json`](evidence/original-pve-current-stage-20261014.json)，原始日志为 `modern/out/runs/original-pve-current-stage-20261014.log`。本次 Map10 门禁未通过是因为所用角色 `100012` 在数据库中持久化于 Map12，不能把 Map12 通过误报为 Map10 228 怪物闭环；当前 PVE stage 仍来自 `a089ad6b`，尚未部署 `f1fa3211` 服务二进制，发布状态保持 `releaseReady=false`。

原版线已完成一次不影响旧环境的 VM112 隔离复核：当前分支 `codex/reorg-original-20261009` 提交 `6a56df1c`，服务包基线为其父提交 `a089ad6b22e8dc4626a217108429d612554c6726`。Release CTest **12,574/12,574 通过**（17 项外部门禁 Skip）。`MoxianClientE2E` 现支持显式登录、Agent、Map 和 Map12 端口，远端 `--no-spawn` 已在 VM112 的 `16101/17101/18101` 完成 LoginAck→选角→Map10 GameInAck，收到 228 个怪物，退出码 0。

本轮包 `original-vm112-release-20261014-r2.zip` SHA-256 为 `af2b64960f41ea530a59aa6d30a6ed6bc6eeaa778e95f17e5ce62bd4fb5b0999`，stage `C:\Moxiang\original\stage-20261014` 使用 5,132 个 PlayDH 文件和 CT930 的 `MoxiangOriginalTest_20261009`。三服和 Map12/Map2 仅在隔离端口运行，结束后精确停止并确认端口释放；旧服务、Unity 服务和主工作树保留。详细哈希、数据库预检、命令和边界见 [原版 PVE 回执](evidence/original-pve-vm112-20261014.json)。

这一步只提升原版工程/PVE 协议证据等级，不宣称首个可玩或商业完成；真人 GUI、自然掉落拾取、装备/切图/重登、完整原资源覆盖、双线 CI 和 24 小时稳定性仍是后续门禁。

## 2026-10-14 原版战斗闭环回执

在保留上述基础协议证据的前提下，本轮用专用角色 `100013` 在隔离 MSSQL 测试库建立临时高等级 fixture，并在 Map2 与目标 Map10 分别真实执行服务端权威攻击、自然死亡、掉落、点击拾取、背包移动和重登回读。两次运行均退出码 0；临时高等级字段和掉落物品行已恢复到运行前值，stage 进程与端口已清理。对应证据为 [原版 PVE 战斗回执](evidence/original-pve-combat-20261014.json)。

同一轮的 1 级 Map10 运行仍以退出码 2 失败：攻击命中但伤害不足以改变目标生命，未产生掉落。这是首个玩家路线的真实平衡/地图前置缺口，不能把高等级 fixture 的通过扩大解释为 1 级 Map10 首战通过。

历史入口（2026-10-09）：优先原版 modern DX11；当时完整计划尚未交付。当前执行以本文件顶部回执和 [ROADMAP](../ROADMAP.md) 为准；可玩结论见 [PLAYABLE_STATUS.md](PLAYABLE_STATUS.md)，缺陷见 [KNOWN_BUGS.md](KNOWN_BUGS.md)，门禁见 [VERIFICATION_MATRIX.md](VERIFICATION_MATRIX.md)。

当前可恢复性校验（2026-10-11）：原版候选 `435d6f68` 与 Unity 候选 `03691818` 均通过 `git lfs fsck --objects` 和 `git lfs fsck --pointers`；对象与指针完整性回执见 `audit/2026-10-09-reorg/lfs-fsck-20261011.json`。这证明当前 HEAD 的 LFS 对象可读，不等于所有未跟踪 WIP 或 PVE stage 已同步。

## 2026-10-10 工程基线复核

原版独立工作树 `codex/reorg-original-20261009` 已在 `C:\BuildTools` 的 VS2022 x64 工具链下完成全目标 Release 构建；DIA、客户端崩溃转储和最小转储工具均已修正为架构正确的链接/寄存器访问。客户端单元与 dump CLI 定向验收为 **155/155 通过**。本机不持有 VHD 恢复资产的 6 项测试现在明确 Skip，并不会伪装成资源已恢复；HSEL Map round-trip 在缺少 PVE 权威角色/进度/生命值表时 Skip，具备完整 PVE schema 时仍执行真实 GameInAck 断言。对应提交：`7519e1d8`、`e925f4e6`、`ed338bb3`、`2054c7c4`、`65a4654d`。

本次验证没有启动本机三服或数据库，也没有改变 canonical PlayDH、Unity WIP、PVE 数据或远端 refs。随后在同一 Release 构建目录运行全量 CTest：**12551/12551 实际执行用例通过，22 项按缺失 PVE/VHD/资源契约 Skip，另外的禁用项未运行，退出码 0**；日志为 `modern/build-commercial-check-20261010/ctest-r4.log`。该结果只证明本机代码/fixture 回归，不替代 PVE 资源、数据库和真实 GUI 验收。本机 release 资源基线的 45 个 HFL/TTB 阻塞仍有效。

2026-10-14 原版 Release 全量 CTest 在并行模式首次暴露 `MapHandlerTest.LoadItemListPopulatesManagerFromBin` 的固定临时文件竞态；测试辅助函数随后改为使用单调时钟与原子序列生成唯一临时 BIN 路径。修复后 `ctest -C Release -j 8` **12574/12574 实际执行用例通过，16 项按既有 PVE/VHD/资源契约 Skip，退出码 0**；同组 77 个 `MapHandlerTest` 并行复跑全部通过。结构化证据为 [`docs/evidence/original-parallel-ctest-20261014.json`](evidence/original-parallel-ctest-20261014.json)，原始回执为 `modern/out/runs/original-ctest-release-fixed-20261014.log`。这关闭了本机并行测试稳定性缺口，不改变原版 44 个地图资源发布阻塞，也不替代 PVE、数据库和真人 GUI 验收。

2026-10-14 PVE 只读资源源复核：VM112 挂载的 `PlayDH.iso`（SHA-256 `a0316bcf43b0d456a467efcc14b95061c433a32e6d9ee759c3ec92946f8cf577`）只含 `PLAYDH.TAR/PlayDH/Map.pak`；`Resource.iso`（SHA-256 `e04452c11d8315564e2dc25c826610b0949a6eaa0c24565a93df0ed6a93cd5d0`）含既有 TTB，但缺失的 10 个原版 TTB 与 26 个 HFL 均未出现，PVE `/datapool1`、`/mnt/datapool1`、`/datapool2`、`/backup-pool`、`/vmstore` 文件名扫描也未找到新的 HFL/TTB。审计回执为 `audit/2026-10-09-reorg/cleanup/pve-resource-source-audit-20261014.json`；没有把替代资源提升到原版 1:1 线。

## 工程与远端

| 分线 | 已验证源码 | 本轮验证 | 当前边界 |
|---|---|---|---|
| 原版 | GitLab457/CI746；本地fae0f53d；PVE3565/r12；实际GUI为e244/r14 | r14正常Launcher→登录→选角→Map10和HUD字色通过；r13有跨客户端权威移动证据；r15b玩家路由/换图时序、r16死亡移动和实际启动wrapper已分别修复并独立审查 | 新r16b包仅准备，尚未停服或替换；复活、自然掉落、拾取与首个可玩未通过 |
| Unity | GitLabad0e096b/CI748；本地d9e3d73f；Player42fe79dd/core39836319；PVE v6b/server84d3a0c5 | PVE handler195/195通过，另3禁用；新Player354文件493901073字节，正常GUI登录→选角→Map10地形→原地复活后HP37/125 | 当前全图概览下角色/怪物不可辨认，呈现诊断中；战斗/拾取/完整可玩未通过；05:38 UTC正常关闭Player，exit0 |
| Unity网络 | `84d3a0c5`已部署；39836319修复测试fixture路径 | 纯网络52/52、定向4/4、wire mock20/20；新native core110/110无DB测试通过；PVE两轮228/228 | 协议实体到达不证明Managed呈现或GUI交互；POSIX和故障恢复未验收 |
| Foundation | `ad0e096b`，独立吸收，CI748通过 | 当时13工具/测试、8可编辑输入与原manifest快照，22/22 Git blob原字节；37通过、2跳过；7组35产物可重生成 | 后续7eda接入单测并退役旧CI入口；仅私有审查，许可、画面、地图和Player未验收 |

GitLab 项目12已经验证为私有工程主源。用户授权后，Broker 部署服务器恢复了项目专用 mTLS 身份和普通 Git 推送；令牌留在服务器，通道只允许两个候选分支，不允许 force/delete。2026-10-10 已重新校验四个 GitLab LFS 对象并完成两条长期分支快进：原版 `c26f5760→340b7472`、Unity `5178e733→f1231e26`，dry-run 与实际推送的远端回读均一致；对应 GitLab pipeline **772**、**773** 均 success。GitHub 镜像、长期分支归并和 PR #1 拆分吸收仍未完成；本轮没有删远端分支或正式发布。

CI范围为治理、Python和精确资源校验。Windows C++、Unity Editor/Player目前是本机构建证据，尚未接入远端构建CI。GitLab与GitHub四个LFS对象均已完整独立读回，共1,228,054,889字节，逐对象大小与SHA-256一致；GitHub续传完成时间为2026-10-09 01:12 UTC，证明见`broker-access/dual-remote-lfs-comparison.json`。这只覆盖四个对象，不代表两端refs、WIP、全部资源和配置已经统一。

Unity84本轮有一项执行范围偏差：旧helper在PC误跑含SQLite的处理器194项，不计PVE验收。PVE handler v3实际193通过、2失败、3禁用；两失败均定位到测试包缺MapChange及编译机绝对路径。39836319修复及13项fixture经独立审查后，v6在PVE独立目录实际195/195通过、exit0、3禁用未运行；前后六服身份和两manifest一致，未改业务库。证据`unity-candidate-review/pve-v6b-receipts/vm112-handler-v6-winrm-run-r3-20261009.json`；历史失败及认证前退出回执保留。

## PVE与客户端

- CT105已快照并克隆到不自启的CT930；只修复副本文件属主。原有三个数据库CHECKDB通过，CT105原件和VM100旧环境保留。
- CT930为192.168.2.238，VM112为192.168.2.244。两线各自使用非SA writer、独立数据库和三服；VM112使用ODBC18验证SQL证书，资源和writer contract不匹配时拒绝启动。
- **原版r12已部署并回归**：服务器源码`3565db96`，15文件和5131资源精确验证，manifest `80bd7d6e6ed5646255b6b8e3b8dfa5aadb771e4514d47a754b679d7b87772302`，三服路径/程序SHA/启动时间/端口归属一致。停写后的独立COPY_ONLY+CHECKSUM备份及VERIFYONLY通过，仅替换三EXE和manifest，未改schema或回填属性。正常协议选择r11创建的100002时，Map10生命125/125、四属性12；旧100000/100001均GameInNack。CT930非SA读回3角色、0物品，与维护前完整角色/物品投影相同。两次过早观察失败已保留并用独立状态收敛；滞留只读QGA诊断也已精确退役。证据为`original-integration/numeric-r11/numeric-r12/r12-runtime-protocol-db-final.md`，不计GUI/自然掉落验收。
- 当前本地原版GUI客户端为e244/r14，SHA-256 `d1beffa1fe63a02304776264ddab3071f0d383af1adba49cf13ab88e4992cea5`，Launcher `ef21ef997b2bf09a96705d3ca6e08690f83e57c1dc010d4dcbe93b81bbbbe228`。实际进程从Windows系统目录加载CRT，没有依赖旧audit DLL路径。本轮已正常关闭两窗口，退出码均0；旧r12/r13构建和证据分别封存。
- 本地已提交原Item profile/codec `d7995380`，Python 2/2、codec 6/6、protocol 31/31及独立审查通过；仅提供独立解析/编码，不改变运行分发、现有modern编号或PVE死亡路径。
- 原版数值迁移已先在独立MSSQL副本通过两次迁移、契约与边界/负面验证；原两角色旧字段哈希不变，四个新增属性仍为NULL，不补默认值。新角色和实际持有装备必须从正常业务路径验证，外观图标不能证明所有权。
- **Unity v6b已部署并回归**：包绑定`69f98655`、服务器`84d3a0c5`，ZIP SHA-256 `adb2d5167c78db703f7b26f67dfcdadd3d4cf30731d52d5d61daed78b68ae14f`，stage manifest SHA-256 `1d7e63779e577f69eeecc094781079118317daa9dddebd00bf61f6a0c35d0380`。PVE三服使用26101/27101/28101；两轮严格2秒门槛均228/228，GameInSyn至末条怪物分别336/190ms、GameInAck至末条分别96/67ms，均正常退出并完成重登。已有fixture使建角跳过；CT930只读DB回查到100000/UnityTrail01。DBTool只读确认schemaVersion 1及writer contract，无本轮迁移。原版服务连续性保持。
- **当前原版GUI为PARTIAL**：r12正常GUI创建100003并有非SA数据库读回；r13第二客户端收到100003移动广播；r14名字/等级/HP/盾/内力/地图坐标完整可读，原色RGB208/234/245的标题/坐标ROI为88/70像素，旧倒置色0。r14在HP0后仍预测移动；6c37e5d6死亡移动门禁已独立12服务器+62客户端测试通过，尚未部署和复验GUI。原Agent旧Correction分发不向发送者回送，不能宣称纠正包已到达死亡客户端。证据`gui-original/r14-gui-current.md`。
- 本机七个旧SQL库的备份已在PVE独立恢复、VERIFYONLY和CHECKDB通过；旧业务读写尚未验证，本机原数据及共享SQL实例保留。

## 资源与独有成果

原版 canonical PlayDH 当前按 **5,135 文件、1,688,281,785 字节**核验；本轮已将来源核验通过的 `Resource/Map/3.hfl`、`95.hfl`、`198.hfl`、`203.hfl` 纳入独立恢复资源，并同步更新 canonical hash manifest。Map.pak 只读审计显示 3.hfl 与 198.hfl 也以相同字节嵌入，其他缺失 HFL/TTB 不在该包内，详见 [`original-map-pak-resource-audit-20261015.json`](evidence/original-map-pak-resource-audit-20261015.json)。241份派生文件已移入独立overlay，原资源禁止回写。原版线当前剩余41个文件级地图发布阻塞（23 HFL、18 TTB），Map203 另有161个准确纹理依赖待恢复；Unity另有精确Effekseer279项和复合来源5135/0验证，其独立基线仍为45个阻塞。

原树初始182修改、241删除、1499未跟踪和空暂存区已保全，没有整体提交或覆盖。29个初始refs、指定WIP和资产在私有PVE恢复验证；范围不包含所有ignored和全部DB。04:13 UTC主树去向表为1118项候选同路径同字节、558项待整合、20项新增量和243项原有删除；delta-v4的20文件/158772原字节已独立恢复，ZIP SHA `fc9bd1091e61fbb4afd9b1d1bd7e4f626e6977669ecadae7879666fe46959147`。Unity当时4746项Git WIP中4737项命中已验证私有包，另9项/457585字节在delta独立恢复（SHA `dc5d391ab4be52997210fbf66cd93ea2b9457b573cc9c17db30f6f74b37eddc3`）。后续构建与文档增量分别记录；字节恢复不等于已整合或验收。

现有武馆、刀剑枪、可编辑源与工具保留。Foundation独立落位于Unity的`artsource/ProceduralStudies/moxiang-reart-v1`，未写入原资源或Player。Unity/legacy隔离副本仍有独有派生资产、源码、Prefab和meta，继续保留。许可顺序与记录见 [ASSET_LICENSE_LEDGER.md](ASSET_LICENSE_LEDGER.md)。

## 本机清理

19个唯一项目已盘点；Project Control的21条记录包含ChatLink与Woai各两种checkout角色，并非21个独立项目。两个已退役项目在两份登记中均明确记录本地不存在及私有恢复位置。

| 已删除范围 | 逻辑字节 |
|---|---:|
| 已恢复验证的源码、recovery、reference及重复资源 | 11,022,791,534 |
| 本轮生成的旧bundle与Unity输入传输包 | 3,055,565,963 |
| 下载缓存、旧日志、测试与状态输出 | 287,791,246 |
| 合计 | **14,366,148,743** |

这是删除文件的逻辑大小，包含本轮临时产物，不能视作磁盘净增可用空间。Open WebUI、textbook-parser本地源码及各自兼容入口已退役；Docker/WSL、现用工具、其他项目和唯一资产继续保留。Junction入口单独处理，没有跟随重复删除。

清理汇总：`audit/2026-10-09-reorg/pc-inventory/CURRENT_CLEANUP_RESULT.md`及同名JSON；总删除逻辑大小14,366,148,743字节，其中源码/恢复/重复项11,022,791,534字节、生成物3,055,565,963字节、缓存/日志/测试/状态287,791,246字节。已删除5454份精确重复overlay/canonical资源和9973份legacy deploy/client文件；32份未覆盖数据文件及6个数据库/sidecar文件仍保留，其他独有文件与目录继续保留。逐项恢复证明、回读和删除回执见清理账本；数据库字节级归档不代表数据库一致性验证。

## 剩余交付

原版PVE仍为r12/3565；本地r15b修复玩家互见/换图响应时序，r16拒绝死亡角色移动，fae0f53d修复实际VM112 wrapper持有Process对象清理及持久化进程身份，均经独立审查。r16b新包与维护脚本正封存，旧r16包只传入run目录、未解压。Unity handler195/195已在PVE通过，新Player已完成首次真实GUI；继续定位实体不可辨认。正常WinRM可用且由original owner独占VM112管理；QGA仍Stop Pending，停止进一步操作，不为其重启VM/DB/六服。

全地图/功能、完整技能数值与视听对照、隔离副本独有内容、旧库业务恢复、Windows/Unity构建CI、干净恢复、双客户端、故障恢复、更新回滚及24小时稳定性尚未闭合。达到相应门槛后才提升长期分支、删旧分支和退役剩余副本。

证据根：`E:\Workspaces\moxiang\audit\2026-10-09-reorg`；私有恢复根：PVE `/vmstore/ci-backups/moxiang-reorg/20261009/`。关键入口为`original-integration/numeric-r11/`、`unity-candidate-review/`、`broker-access/`、`pc-inventory/`与`isolated-review/`。旧活动文档和测试流水的精确恢复位置见 [HISTORICAL_EVIDENCE_INDEX.md](HISTORICAL_EVIDENCE_INDEX.md)。
## 2026-10-14 当前提交 PVE 隔离回执

当前原版提交 `3bf8192065635394893d2df0b7a847aada201d53` 已封装为干净服务包（SHA-256 `491110912a2e6007e888da9a3f0f9f5f6a602f381265f4410d1acd9df0560a9d`），在 VM112 `stage-20261014` 的隔离端口 `16101/17101/18101/18102/18103` 启动五个服务。真实 Release 客户端以专用测试账号完成 Map10 登录、建角、选角、GameInAck，并收到 **228 个怪物**；同一当前包以 `mxh_s1010` 完成 Map12 GameInAck。Map10/Map12 原始日志哈希分别为 `2a58b26f2266d175ada73f9cbf7df9ad9c815bdcb53ef3dfe8db3ca568b9e093`、`682518513405f723f9ba0eebec2a06f53971998b445e9fa9043aec97993e13c8`，结构化回执见 [`original-pve-f1fa-stage-20261014.json`](evidence/original-pve-f1fa-stage-20261014.json)。

专用 Map10 账号和角色在验收后已删除；隔离进程、临时包、展开目录和备份已清理，旧 stage 已恢复到 `a089ad6b22e8dc4626a217108429d612554c6726`，默认端口与旧服务未触碰。该回执关闭“当前提交 + PVE MSSQL + Map10/Map12 GameIn”子门禁，仍不等于商业发布：真人 GUI 战斗、自然掉落/拾取/装备/切图重登、缺失原版资源、全地图玩法与稳定性门禁仍开放。

## 2026-10-14 当前提交战斗门禁回执

当前提交 `3d30042b` 的服务包在 VM112 隔离环境执行了真实战斗 fixture。Map2 完成服务端权威命中、生命变化、自然掉落 `item=50009`、点击拾取、背包 `MoveAck` 和重登持久化，怪物数量 `128→126`，退出码 0。Map10 同样完成权威命中和生命变化，但本次验收窗口未观察到自然物品掉落，因此没有把 Map10 战斗闭环误报为通过。完整回执见 [`original-pve-f1fa-combat-20261014.json`](evidence/original-pve-f1fa-combat-20261014.json)。

战斗测试账号、角色、状态、装备和物品已逐项删除，数据库回查账号行数为 0；隔离服务已停止，stage 恢复为 `a089ad6b`。当前关闭的是“当前提交 + PVE Map2 战斗/掉落/拾取/重登”子门禁，Map10 掉落、真人 GUI 和商业发布门禁仍开放。

## 2026-10-15 任务状态战斗修复

原版候选新增提交 `3106ec4efbd6d82eb0f4c1cc60bf53d29f5817cb`。它删除了 revisioned 技能对非空任务日志的错误前置拒绝，并以单测和 VM112 受控 MSSQL fixture 验证任务行保留时的 Map10 权威击杀。临时 fixture、服务任务和旧 Map10 已按原路径恢复；CT930 回读为 level 1、四属性 12、HP 125、装备 0。结构化回执：[`original-pve-questfix-combat-20261015.json`](evidence/original-pve-questfix-combat-20261015.json)。

本回执只关闭任务状态与 revisioned 战斗兼容性子门禁。由于 E2E 循环未完成最终拾取/重登断言，原版首个可玩版本、资源完整性、Unity 线和最终清理仍保持未闭合。


