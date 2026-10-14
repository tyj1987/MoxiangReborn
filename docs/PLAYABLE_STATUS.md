# 可玩状态与分线验收

## 2026-10-14 原版 PVE 隔离实例复核

当前原版工作树提交为 `a089ad6b22e8dc4626a217108429d612554c6726`。Visual Studio Release 全量 CTest 重新运行 **12,574/12,574 通过**，总耗时 72.89 秒；17 项明确因 PVE、MSSQL、旧资源或显式外部门禁跳过，未将 Skip 计为通过。新增的 `MoxianClientE2E` 端口参数（`--login-port/--agent-port/--map-port/--map12-port`）已在 Release 构建并通过默认端口 CTest、非默认 `--dump-cli` 解析和远端运行。

本轮将 Release 包 `original-vm112-release-20261014-r2.zip`（SHA-256 `af2b64960f41ea530a59aa6d30a6ed6bc6eeaa778e95f17e5ce62bd4fb5b0999`）部署到 VM112 的独立 stage `C:\Moxiang\original\stage-20261014`，没有替换旧服务。该 stage 使用 PlayDH 资源 **5,132 文件、1,682,222,623 字节**；补入的 `Resource/Map/95.hfl` SHA-256 为 `fbc336eb9b73a55f50979d4e2678bd760479d69cf49217a5ae268332dbed1746`。manifest SHA-256 为 `d73369562c67e7107211f00fd1f640fbbcb9be32c037fa7876ef6c5ce156e4e6`。

隔离三服实际运行于 `16101/17101/18101`（另起 `18102/18103` 供 Map12/Map2 路由），程序哈希分别为 Login `81355002596f3e93be809ba9150e6b10a9b406f031b20a08ae0f57a3f0b431da`、Agent `03ea891dddf2c75f48af5f8e03d3d8c83b6bb87cc0060ff8b7aae149f4cbba16`、Map `c42dcf2fa468fd8127d8c64b369c9dbf3f981158046dfbae6e6c5de442bdf646`。远端 `MoxianClientE2E --no-spawn` 以真实 GUI 状态类完成 LoginAck、Agent CharacterList、角色重列、CharacterSelect、Map10 GameInAck 和 228 个怪物实例接收，退出码 0；MSSQL 使用 CT930 的 `MoxiangOriginalTest_20261009`，schema/query 只读检查退出码 0，预检读到 13 个角色行。测试结束后仅停止本轮 stage 的精确进程并确认五个隔离端口均无监听；旧 VM100/VM112 服务、Unity 服务和主工作区未触碰。

这关闭了“原版 Release 工程 + PVE MSSQL + 远端协议/GameIn”子门禁，但仍不等于真人 GUI 的自然战斗、掉落点击拾取、装备、切图重登或完整 1:1 资源验收。逐项摘要见 `docs/evidence/original-pve-vm112-20261014.json`。

更新：2026-10-10。当前没有任何一条线通过“首个可玩”验收。历史运行记录已归档，当前结论只采用本轮可复核证据；定义见下方 E0–E5，门禁见 [VERIFICATION_MATRIX.md](VERIFICATION_MATRIX.md)。执行总览见 [重整状态](REORGANIZATION_STATUS.md)，旧状态恢复位置见 [历史证据索引](HISTORICAL_EVIDENCE_INDEX.md)。

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
