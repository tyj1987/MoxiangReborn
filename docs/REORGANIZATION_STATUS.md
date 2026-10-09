# 重整执行状态

更新：2026-10-09。优先原版 modern DX11；完整计划尚未交付。计划见 [ROADMAP](../ROADMAP.md)，可玩结论见 [PLAYABLE_STATUS.md](PLAYABLE_STATUS.md)，缺陷见 [KNOWN_BUGS.md](KNOWN_BUGS.md)，门禁见 [VERIFICATION_MATRIX.md](VERIFICATION_MATRIX.md)。

## 2026-10-10 工程基线复核

原版独立工作树 `codex/reorg-original-20261009` 已在 `C:\BuildTools` 的 VS2022 x64 工具链下完成全目标 Release 构建；DIA、客户端崩溃转储和最小转储工具均已修正为架构正确的链接/寄存器访问。客户端单元与 dump CLI 定向验收为 **155/155 通过**。本机不持有 VHD 恢复资产的 6 项测试现在明确 Skip，并不会伪装成资源已恢复；HSEL Map round-trip 在缺少 PVE 权威角色/进度/生命值表时 Skip，具备完整 PVE schema 时仍执行真实 GameInAck 断言。对应提交：`7519e1d8`、`e925f4e6`、`ed338bb3`、`2054c7c4`、`65a4654d`。

本次验证没有启动本机三服或数据库，也没有改变 canonical PlayDH、Unity WIP、PVE 数据或远端 refs。随后在同一 Release 构建目录运行全量 CTest：**12551/12551 实际执行用例通过，22 项按缺失 PVE/VHD/资源契约 Skip，另外的禁用项未运行，退出码 0**；日志为 `modern/build-commercial-check-20261010/ctest-r4.log`。该结果只证明本机代码/fixture 回归，不替代 PVE 资源、数据库和真实 GUI 验收。本机 release 资源基线的 45 个 HFL/TTB 阻塞仍有效。

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

原版 canonical PlayDH 基线仍按5131文件、1,681,042,155字节核验；本轮新增的 `Resource/Map/95.hfl` 是独立恢复文件，不回写 canonical hash manifest，来源与哈希记录在同目录 provenance 文件。241份派生文件已移入独立overlay，原资源禁止回写。原版线当前剩余44个地图发布阻塞（26 HFL、18 TTB）；Unity另有精确Effekseer279项和复合来源5135/0验证，其独立基线仍为45个阻塞。

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
