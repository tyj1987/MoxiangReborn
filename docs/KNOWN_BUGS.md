2026-10-20 | 当前原版分支已绑定三类新证据：VM112 真实 MSSQL Map17 1 级首发闭环、默认 PVE Map10 Debug 客户端可视窗口捕获，以及 481 项资源/协议/服务器回归全通过。新增回执见 [`original-gui-map10-capture-20261020.json`](evidence/original-gui-map10-capture-20261020.json) 和 [`original-debug-unit-regression-20261020.json`](evidence/original-debug-unit-regression-20261020.json)。这些结果仍不替代真人 GUI 全链、视觉/音频 1:1、Map17 九个准确纹理、全地图覆盖或商业发布验收。

# 当前缺陷与交付阻塞

本页只列本轮仍未闭合的问题。验收状态见 [VERIFICATION_MATRIX.md](VERIFICATION_MATRIX.md)，实施顺序见 [ROADMAP](../ROADMAP.md)。历史原文的恢复位置见 [HISTORICAL_EVIDENCE_INDEX.md](HISTORICAL_EVIDENCE_INDEX.md)；旧截图、探针和通过记录不能代替当前版本验收。

## 原版首个可玩闭环

- **2026-10-20 本机 Map17 首发战斗子门禁通过**：当前提交在本机隔离 SQLite 三服链路以真实 1 级新角色完成 Map17 的攻击、生命变化、自然掉落 `item=50108`、点击拾取、背包移动和重登回读；该结果证明 canonical 出生地图路线比 Map10 测试默认更接近原始首战，但只关闭本机 headless 子门禁。VM112 MSSQL、真人 GUI、Map17 九个准确地表纹理和全量 1:1 资源仍保持开放。回执：`docs/evidence/original-local-map17-starter-combat-20261020.json`。

- **2026-10-14 新增真实门禁证据**：1 级新角色在 Map10 对当前原始怪物只能造成 2–4 点伤害（目标防御 541、生命 6150），攻击命中但无法改变生命，退出码 2；这证明首战地图/等级前置尚未闭合。使用可回滚的合法高等级测试 fixture 在 Map2 已完整通过权威战斗→自然掉落→点击拾取→背包移动→重登回读，不能将 fixture 通过扩展为 1 级 Map10 可玩完成。回执：`docs/evidence/original-pve-combat-20261014.json`。

- **2026-10-15 当前源码复核**：默认 VM112 端口已用带原始首发装备的高等级 fixture 完成 Map10 权威战斗、自然掉落、点击拾取、背包移动和重登回读，回执 `docs/evidence/original-pve-default-current-source-combat-20261015.json`。当前 MapHandler 对带 revision 的玩家会拒绝携带任务状态的战斗事务，因此测试临时移除任务行并在结束后恢复；这不是玩家可见功能完成。无装备负对照只广播技能对象，不产生伤害，说明 1 级首战仍需补齐装备授予、任务耦合和数值平衡后重新验收。

- 屏保和自动熄屏已关闭，锁屏解除；三个旧网络测试防火墙请求已正常取消。当前r12客户端已通过正常GUI登录、建选角和Map10入图；移动/NPC/任务、战斗、自然掉落、点击拾取、装备、切图及重登全链尚未通过。
- 原版r12已完成部署协议回归；其维护时3角色/0物品投影保留为历史快照。后续真实GUI创建MoxR12Gui（100003），非SA读取Map10、四属性12及外观装备行，owned item仍无行；外观记录不证明持有DBID装备。
- 基础属性及持有装备端点计算已恢复部分原始逻辑，完整技能、伤害公式、状态、攻防命中、怪物/Boss 和异常流程仍未完成 1:1 对照。外观图标不代表玩家实际持有装备，手工数据不代表自然掉落和拾取。
- 原 MonsterList 的掉落类别、三张表及最大次数已在本地纯解析提交 `74376789` 中补齐；独立审查无P1/P2，解析3/3、掉落25/25纯测试通过。提交尚未部署或接入死亡路径；现有运行路径按怪物种类选择表并设为100%，不能宣称原概率正确。
- 原 Item 通知是直接入包数组，现代实现却复用为20字节地面物品；现代 Pickup 19–21还与原 Buy 编号冲突。需明确 profile、成对迁移并实际验收，不能局部重编号。详见 [ITEM_PROTOCOL_COMPATIBILITY.md](ITEM_PROTOCOL_COMPATIBILITY.md)。
- 相机点击/宿主接线已在r13由真实GUI和跨客户端Move广播验证。r15b玩家目标路由/换图membership时序、r16死亡移动门禁独立通过但未部署。现有Agent Correction不向发送者回送，原版复活生产链仍未接通；装备MoveAck早于落库、拾取后其他客户端地面移除仍待修。
- r14/e244固定原资源GUI已显示完整名字和动态数值，原颜色实际像素通过；历史黑字/顶部裁切在本次1024×768场景已关闭。完整字体索引/字号、多DPI和极端样式坐标的宿主布局加法仍待验证或修复。2026-10-20 已为 `character_info` 增加可空坐标列，Stop/Warp、退出和断线写入，换图清空旧地图坐标；SQLite 回归已证明重登恢复，VM112 GUI 仍需重新验收。
- 角色选择/创建的三维外观、动作、装备呈现，实体占位回退、全部 InterfaceScript 数据绑定、移动/选中/碰撞/NPC 路径及地图切换仍需逐项验收。
- 启动器更新与回滚、登录错误码/超时重试、DPI/多显示器/Alt-Tab、交换链失败等产品路径缺少当前真实交互证据。

## 资源、地图与音画

- 2026-10-24 更正：HFL `0,4,23,24,32,33,34,94` 与 TTB `0,3,4,23,24,32,33,34,94,198` 未被原版 `.map`/`.stm` 引用，不再记为缺失，也不从文件名规律推断需求。旧文的 “HFL 22 / TTB 18” 含上述编号，下面已不再把它当作余量，也不另造新的扫描合计。Map17 九张纹理在原版 HFL 里是 `tile_17_mou_a_01..09.tga`，不是 `.dds`；连同 `test4` 与 `Monster_203.bin`，归类为“原版引用了但原文件不存在”，只作信息，不是缺失缺陷。旧证据里把 `.dds` 搜索写成结论的，用的是工作区 PlayDH/Map.pak（头 4192 项），不是原版 `D:\MX\client\Package\3DData\Map.pak`（读到 EOF 为 11267 项）；2026-10-21 以 `.tga` 复核原版包和全部磁盘仍无这些文件。
- Map203 客户端侧 161 个 `tile_201a-*` 已从同源原始包恢复并通过 0 缺失依赖审计，但真实 VM112 启动在 `Resource/Server/Monster_203.bin` 处失败；禁止以开发 fallback、空文件或其他地图的 AIGroup 替代。回执：`docs/evidence/original-map203-runtime-resource-gate-20261017.json`；最新启动前预检回执：`docs/evidence/original-map203-runtime-resource-gate-20261019-r2.json`。
- 原资源基线和派生叠加层已经分离；当前源清单精确性检查通过，不等于所有地图可玩。原版线已从原始 `Map.pak` 恢复并验证 `Resource/Map/3.hfl`、`Resource/Map/17.hfl`、`Resource/Map/95.hfl`、`Resource/Map/198.hfl`、`Resource/Map/203.hfl`；Map203 的 161 个准确地表纹理已从同源原始包准入并通过依赖审计。TYJ-159 核对后，原版 `.map`/`.stm` 引用 75 个 HFL、0 个 TTB。其中 10 个 HFL（`01`、`210`、`301`、`302`、`311`、`312`、`341`、`343`、`351`、`353`）PlayDH 没有。TYJ-153 已把它们和本次清单里的其他原版引用条目写入 `reference/manifests/playdh-supplemental-fill.json`，`pakSha256` 为 `1d7a87e00bcb54b7f9d39a719973ebc1efcf7c2505711e7d97d98b0157c5cbd5`（ROG 上两次重建一致，包长 98024087）。补充包字节仍不入库。`44.map` 没有 `HEIGHT_FIELD`，所以不需要 `44.hfl`。Map17 九张 `tile_17_mou_a_01..09.tga`、`test4` 与 `Monster_203.bin` 仍是原版引用了但原文件不存在，只作信息，见上条。准确版本缺失必须保留阻塞，不能用替代美术宣称 1:1。
- 这 10 个 HFL 的字节还没进 PlayDH；已恢复的 Map3/17/95/198/203 不代表 Map10 或全地图缺口为零。对这份参考清单做 `check`，`referencedButNoOriginalFile` 为 4037 条（`originalExists` 为假的引用，再加上 `knownReferencedNoOriginalFile`）。上条已确认的 Map17 九张纹理、`test4` 和 `Monster_203.bin` 仍只作信息。其余条目是 `derive` 只索引 Map.pak 与散文件时没有命中的名字，尚无结论，待 TYJ-165 重新派生；它们既不是缺口，也不是已确认原文件不存在。原创武馆场景不能替代原始地形；全地图目标范围还需按原版配置核验，未证明任何缺项可降级。UnityNative 与原版同名的 10 个 TTB 已明确拒绝进入原版 1:1 线，见 `docs/evidence/original-ttb-isolation-audit-20261015.json`。Unity 重制线的独立基线仍按自身阻塞记录，不能由原版线的恢复自动继承。恢复文件的来源、哈希和验证见 `reference/manifests/95.hfl.provenance.json`、`reference/manifests/3-198-203.hfl.provenance.json`、`reference/manifests/original-map203-textures.provenance.json` 及本轮审计记录。
- 本轮对本机发现的五个 Map.pak 候选（含 D:/MX 与 D:/MoyoI）重新列包并抽取 Map17 HFL；9 个 `tile_17_mou_a_01..09` 均未作为准确纹理条目出现。结果见 `docs/evidence/original-map17-texture-source-search-20261015.json`；在找到准确来源前，Map17 不进入 1:1 视觉发布门禁。
- BMHM/TTB/HFL/STM、天空和环境语义、角色动作、BEFF/BEFL/Effect.pak 触发、WAV/空间音效及全地图视觉音频对照尚未闭合。
- `playdh-current` 的服务器资源编码与旧 `mhfileex-classic-rxmso` 配置不同；小型或非标准条目需要按实际配置验证，不能静默改用旧版解码。历史 Map10→Map12 和 Map101 地形缺失记录只作定位，不沿用其通过或数量结论。
- **R-17**（legacy 2008 PackingMan，SWorking 服务端 `.bin`）仍在默认 `read_mh_bin` 和 `sworking-2008-reference` 上生效：`[uint32 file_size == 文件总长][payload]`，payload 走 positional type 0。这 107 个服务端文件继续走这条路径。安装版客户端容器和 R-17 的首字段相同，不能靠字节启发式拆开。拒绝只在显式 `read_mh_bin(path, "playdh-current")`：文件不少于 20 字节且首个 uint32 等于文件长度时返回含 `obfuscated container` 的错误，不剥离、不解码。库不读环境变量或全局配置。现有调用方（客户端资源、`read_server_mh_bin`、`MhFileReader::init_bin`、`player_monster_point`、`map_handler`）未切换。Refs TYJ-146。
- 掉落表应使用经当前格式校验的入口；历史 `Resource/Server/MonsterDropItemList*.bin` 被识别为 AIGroup 的记录仍需作为防止错误回退的依据，不可仅因字节可读就启用。

## Unity 重制线

- 空 Library 导入、EditMode 和独立 Player 构建已有绑定版本的证据；当前 Player 的角色、怪物可辨性、动作、地面掉落、点击拾取和完整正常 GUI 路径尚未验收。
- 网络线程生命周期修复904已进入PVE v6b；当前两轮严格时限228/228、正常退出和重登/数据库读回通过，建角fixture步骤跳过。Windows发送无进展超时为10秒，同步回调仍可能延长关闭；POSIX编译和运行、故障恢复及GUI仍待验证。
- 原有武馆及刀剑枪等可编辑资产保留。Foundation 样本完整性与游戏接入、画面质量和分发权分别验收；样本许可/权利来源尚未具备完整凭证，不自动纳入 Player 或公开镜像。

## 工程与最终交付

- 两线共享初始化/断开通知排序已在原版3565和Unity84修复并审查，纯网络分别50/52通过。Unity PVE handler原结果193通过/2失败/3禁用；两失败已证为测试包漏MapChange及两处__FILE__绝对路径，39836319及新fixture包待PVE复跑，不能记作生产路由逻辑回归或提前记通过。
- QGA一次停止和精确终止尝试后仍为原PID/Stop Pending，停止后续操作；正常加密WinRM管理已恢复，original owner独占VM112管理。旧只读PID3092终态仍unknown；handler旧193/2/3结果保留，新fixture在PVE实际195/195通过，3禁用，六服/两manifest连续。
- 两个候选分支尚未提升为最终 `main` / `codex/unity-client-remaster`。本机 WIP、历史分支独有成果及隔离副本未完全吸收，禁止提前删分支或重复 checkout。
- GitLab 项目 12 的候选推送和 Linux 治理/资源 CI 已恢复；Windows C++ 与 Unity Editor 构建目前仍是本机证据，尚未成为对应远端 CI 作业。
- 双客户端、断线重连、服务重启、数据库故障恢复、更新回滚、清理后独立恢复及 24 小时稳定性未全部通过。
- 已验证的冗余文件已按清单删除；其余独有源码/资产、本机 SQL 数据及共享实例继续保留，直至对应恢复与业务验证完成。

原先五对 UI 头文件差异已经按候选源码校准，不再保留“等待用户决定”的历史阻塞。缺陷只在对应提交、资源、环境和真实行为证据齐全后关闭。


## 2026-10-15 任务状态与战斗

- 已修复 `MapHandler` 对带任务状态的 revisioned 技能错误拒绝：`3106ec4efbd6d82eb0f4c1cc60bf53d29f5817cb` 的单测覆盖 MP、怪物死亡和击杀任务进度。PVE 受控 fixture 实际观察到任务公告、两次权威击杀及角色/物品事件，随后恢复数据库基线。
- E2E 战斗循环仍会在击杀后继续选择刷新目标，本轮没有进入最终拾取/重登断言；该客户端循环问题与 1 级首战数值/装备路线分开跟踪。回执：`docs/evidence/original-pve-questfix-combat-20261015.json`。
- E2E 战斗探针已将战斗窗口限制为 `min(max(timeout,30)*3,600)` 秒，避免稀有掉落或缺失掉落时无限选择刷新目标；Release 客户端重新通过旧 Map10 登录/GameIn 烟测。完整回执中的 harness 字段记录该边界。

### 2026-10-15 真实新角色首战复核

使用全新 PVE 账号通过真实 LoginAck、空角色列表、CharacterMake、选角和 Map10 GameIn。角色创建正确持久化原始首发装备 `11000/23000/27000`，GameIn 读回 1 级、生命 `131/131`。当前测试默认把新角色送入 Map10；对原始目标 `50143`（生命 `6150`）连续攻击只产生 `2–4` 点护盾伤害，目标生命始终 `6150`，没有掉落或拾取事件。该结果证明角色创建与首发装备写入可用，但首战地图/等级前置尚未按原始规范闭合。测试账号、角色、装备、物品、状态和生命行已在 CT930 事务清理，复核账号/角色/identity 行均为 0。

回执：[`original-pve-fresh-starter-combat-20261015.json`](evidence/original-pve-fresh-starter-combat-20261015.json)，日志：[`original-pve-fresh-starter-combat-20261015.log`](evidence/original-pve-fresh-starter-combat-20261015.log)。下一步只核对原始启动地图与等级路线；不通过降低原始怪物防御、伪造伤害或替代资源来“修复”1:1。

### 2026-10-15 原始出生路线静态契约

对恢复的特定历史数据库版本，`dbo.MP_CHARACTER_CreateCharacter` 的字节核验定义显式写入 `CHARACTER_MAP=17`、`character_loginpoint=2017`；对应 `LoginPoint.bin` 行为 Map17 兰州登录点。当前 VM112 的 Map10 是测试部署参数，不是已证明的原始出生路线。准确 `17.hfl` 已从原版 `Map.pak` 恢复，但 9 个使用纹理仍缺失，Map17 新角色运行验收暂不能执行；不得把 Unity/占位纹理纳入原版 1:1。

静态回执：[`original-birth-route-static-20261015.json`](evidence/original-birth-route-static-20261015.json)。

### 2026-10-28 默认 StartArea Refs TYJ-156

恢复的数据库过程被认为把出生地图写成 17，但 `CHARACTER_MAP=17` 不能追溯到一份原版安装文件。在出现这样的原件之前，默认 StartArea 保持不变：Agent 省略 `--start-area` 时使用 `default_map_num`（`modern/tools/MoxianAgentServer/main.cpp` 里的默认值是 12，调用方可以用 `--default-map` 覆盖）。显式把 StartArea 配成 17 且该地图未注册时，启动失败并写明地图号；建角拒绝未注册的 StartArea，不会再静默落到 Map10。已经保存在 Map10 的角色仍进入 Map10。

### 2026-10-28 原版建角起始地图 17 证据 Refs TYJ-156

原版角色创建的起始地图是 17。证据：originalFile `D:\MX\database\mhgame.bak`（2014-01-13 备份，SoftwareVersion 9.0.5000），sha256 `7773fa1da919d2f8b5f18019dbc6e55efd64025edd817fddb923f08e3535126b`，存储过程 `MP_CHARACTER_CreateCharacter` 向 `TB_CHARACTER.CHARACTER_MAP` 写入字面量地图 17。旁证仅：`D:\MX\src\database\MHGAME.bak`（2009-12-07，SoftwareVersion 9.0.4035），sha256 `bcdf836239b9e4f029efb337a262be668c52719692b6846c0a79c54f08570df3`，过程名相同；该备份里 `CHARACTER_MAP` 来自过程参数（调用方传入），因此不能确定地图号。未还原，也不再追查。以上由 ROG 上的只读字节搜索和 `RESTORE HEADERONLY`/`FILELISTONLY` 确定，没有执行还原。过程体留在 ROG，不在本页复述。

TYJ-156 仍开放。Map17 这一半的结论：代码默认是 12（`modern/tools/MoxianAgentServer/main.cpp:58`）；部署配置传入 10（`deploy/scripts/start_original_vm112.ps1:10` 的 `$MapNumber = 10` 在 `:137` 作为 `--default-map` 传给 Agent；同样 `deploy/scripts/start_current_vm112.ps1:14` 与 `:108`，`scripts/gui-client-smoke.ps1:67` 与 `scripts/gui-client-smoke-ephemeral.ps1:507`，`deploy/scripts/start_modern.ps1:12` 与 `:373`）。StartArea 没有改成 17。这是已知偏差：原版建角由存储过程 `MP_CHARACTER_CreateCharacter` 写入起始地图 17，文件 `D:\MX\database\mhgame.bak`，sha256 前缀 `7773fa1d…126b`。原因：2026-10-28 对完整原版客户端根（全部 pak 与散文件，经资源引用门禁的 exists 索引）直接查找，`tile_17_mou_a_01..09` 的 `.tga` 与 `.dds` 合计 18 项命中为 0。不允许替代美术，因此 Map17 排除在 1:1 GUI 门禁之外，并且只保持可选（`start_original_vm112 -EnableMap17` 默认关闭），记在 TYJ-166。若以后找到原版 `tile_17_mou_a_*` 纹理，再重新评估是否把默认改为 17。TYJ-156 的另一半仍未验收：新角色在 Map10 的首战造成有效伤害并击杀一只怪物。

仓库内没有找到 `MP_CHARACTER_CreateCharacter` 的调用方；现代建角不调用该过程，客户端把 `params.start_area` 写入建角包偏移 25（`modern/src/client/CCharMake.cpp:234`；CHINA 目录唯一 StartArea 选项值为 17，`modern/tests/unit/client/char_make_options_test.cpp:66`），`handle_legacy_character_make` 读取该字节（`modern/src/server/agent_handler.cpp:1642`）但写入 `character_info.start_area` 与 `map_num` 的是配置 StartArea（`--start-area`，否则 `default_map_num`；`modern/src/server/agent_handler.cpp:1643`、`1763`、`1774`、`1777`、`1809`）。

### 2026-10-15 Map17 原始 HFL 真实恢复

复核发现当前原版 `modern/data/PlayDH/Map.pak` 的第 240 项实际包含 `17.hfl`。已从该原版包提取并按 SHA-256 `727b706892bb276bfec418de21ca4c38bafd2533222a0741b6dddd55256cab8a` 写入 `Resource/Map/17.hfl`，同步更新 canonical hash manifest。HFL 解析通过，使用的 42 个纹理中仍有 9 个缺失；只恢复了原始高度场，没有引入 Unity 派生或替代纹理。

回执：[`original-map17-hfl-admission-20261015.json`](evidence/original-map17-hfl-admission-20261015.json)，来源清单：[`17.hfl.provenance.json`](../reference/manifests/17.hfl.provenance.json)。Map17 仍未达到可玩/视觉发布条件。

### 2026-10-28 Map17 九张地表无原件 Refs TYJ-166

Map17 引用 9 张地表 `tile_17_mou_a_01..09.tga`，没有原始来源。对完整原版客户端（全部 pak 与散文件）直接查找这 9 个 `.tga` 和同名 `.dds`，18 个名字的命中数是 0。不制作替代美术。Map17 不进入 1:1 GUI 门禁。在地图 17 上，新角色出生即可见这 9 张缺失地表。本地核验出生地图 17 使用 `-MapNumber 17`。部署脚本默认地图保持 10。`derive` 会在存在性索引上查找这些名字；其中任何一个出现时 `derive` 失败且不写清单，这个问题重新打开。

### 2026-10-28 LocalDB 孤儿服务端占住输出管道 Refs TYJ-163

LocalDB 路径已去掉按映像名清理。`scripts/commercial-smoke.ps1:1046` 调用 `Invoke-CommercialSmokeOwnedExecutable`，只记录本次启动的 `mxh_client_e2e` 以及已经进入 owned 进程树的后代，并只停止这些 PID。触发条件：E2E 先退出，留下一个没有记入该进程树的孤儿服务端。影响：该孤儿继续占着输出管道，脚本一直等管道关闭而挂起。当前缓解：无；只做 owned PID 清理。ROG 没有 LocalDB，实机验证记在 TYJ-162，在装有 LocalDB 的干净机器上做。

