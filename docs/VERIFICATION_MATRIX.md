# 验收矩阵

更新：2026-10-20。只记录本轮状态；旧矩阵及 XML 结果已归档，不能继承其中 PASS。总览见 [可玩状态](PLAYABLE_STATUS.md)、[Unity 重制状态](UNITY_REMASTER_STATUS.md) 与 [重整执行状态](REORGANIZATION_STATUS.md)。

## 证据等级

| 等级 | 定义 |
|---|---|
| E0 | 文件/包字节与 SHA-256 已记录 |
| E1 | 格式完整解码、字段覆盖完整 |
| E2 | 运行对象实例化且无缺依赖 |
| E3 | 运行时渲染或播放资源 |
| E4 | 正确玩家动作触发资源及状态变化 |
| E5 | 真实旧版对照与人工审查通过 |

自动测试、脚本 GUI、截图、parser/CTest 总数都不能单独证明 E5 或用户本人验收。E0–E5 是证据等级，不代表功能完成比例。

## 本轮门禁

| 门禁 | 含义 | 状态 | 本轮证据与缺口 |
|---|---|---|---|
| G0 | Git、源码与资源保护 | PARTIAL | WIP、指定资源及归档有恢复证据；隔离副本和遗漏类别尚未全部闭合，不能宣称完整保全。 |
| G1 | Profiles、路径与秘密保护 | PARTIAL | PVE 分线和 GitLab 专用候选通道已核验；干净机器的全路径/秘密审计及 Unity 依赖闭合未完成。 |
| G2 | 干净树、构建与当前文档 | 未通过 | 原版 Unity 工作区仍有保留 WIP；本轮文档更新不等于工作树清洁。不得沿用旧 clean-tree PASS。 |
| G3 | 客户端状态、显示与加载基础 | PARTIAL | Unity原资源隔离后的历史空Library EditMode191/0/13；当前Player42fe79dd/core39836319构建及首次正常GUI入图通过。新呈现诊断与后续源码不继承旧全套测试。 |
| G4 | 启动器、登录与显示转换 | PARTIAL | r14/e244正常Launcher→登录→选角→Map10通过；无旧audit DLL依赖。完整异常流程、DPI/多显示器及真人验收未完成。 |
| G5 | 角色与加载流程 | PARTIAL | 原版GUI建角100003及PVE非SA四属性12读回；owned item无行。Unity当前Player正常选UnityTrail01入Map10并原地复活，UI生命0→37/125；未完成持有装备DBID和完整业务链。 |
| G6 | Map10 世界呈现 | PARTIAL | 原版r14地形、蓝衣长武器角色与怪物可见，死亡后仍站立。Unity当前Player只清楚显示全图地形，角色/怪物不可辨认。两线姿态、遮挡和完整画面尚未验收。 |
| G7 | UI 实时绑定 | PARTIAL | r14固定canonical的名字/等级/HP/盾/内力/地图坐标完整可读，实际像素原色正确；独立parser25/adapter5通过。全部控件、原字体和多DPI尚未通过；极端样式坐标仍有后续布局边界。 |
| G8 | 移动、战斗、掉落与换图 | PARTIAL | 当前提交已在本机 SQLite 与 VM112 隔离 MSSQL 的 canonical Map17 完成 1 级首发权威攻击→死亡→自然掉落→点击拾取→背包移动→重登回读（本次 PVE 216 怪物、5/5、退出码 0）；Map10 1 级首战、真人 GUI、Map17 九个准确纹理、任务奖励、全地图覆盖和 Unity 同等玩法仍未闭合。
| G9 | 特效与音频 | 未运行 | 本轮没有配套 Player、事件轨迹和原版对照证据。 |
| G10 | 全部资源、地图与 UI | 未通过 | 资源依赖及来源仍有缺项；本轮 PVE Map10 没有 canonical Dealer，商店目录/购买/重登无法开始，回执 `docs/evidence/original-pve-shop-current-20261015.json`。工作区发现的 16 份 `Dealitem.bin` 全部同一哈希且解码后 Map10 行数为 0，来源审计 `docs/evidence/original-dealitem-map10-provenance-audit-20261015.json`；未完成全地图/功能覆盖矩阵。 |
| G11 | RC 稳定性 | 未运行 | Linux ctest 覆盖合成目录上的更新/回滚事务（成功替换、坏摘要、`..`/绝对路径、用户数据删除拒绝、清单摘要不符、日志续跑、回滚字节一致、版本比较）。交互验收、Windows HTTPS 下载、干净机器、故障注入和 24 小时 soak 仍未运行。 |

## 分线局部证据（不升级整体验收）

| 分线 | 当前可陈述事实 | 不代表 |
|---|---|---|
| 原版候选 | 远端457/CI746；PVE3565/r12；r14/e244真实HUD显示/颜色局部通过，parser25/adapter5独立通过；r13有GUI跨客户端Move广播，100003建角有DB读回。 | 双客户端外观、完整GUI链、死亡/复活、装备DBID所有权、完整数值、自然掉落/拾取、位置持久化及RC。 |
| Unity候选 | ad0e096b/CI748；v6b两轮228/228、既有DB读回；39836319 handler在PVE195/195、3禁用，新native110/110；Player42fe79dd正常GUI入Map10、地形显示、原地复活生命更新。 | PC误跑SQLite不计PVE验收；Windows/Unity构建CI、可辨认角色/实体、战斗/拾取、许可和45个地图缺口未闭合。 |

## 下一轮关闭条件

G0/G1 需完成隔离副本、忽略文件、派生资源和依赖去向的逐项清单及可恢复验证。G2 需按已保全的 WIP 范围整理后复查树与干净构建。G3–G9 分别按原版/Unity 候选提交和本轮日志重跑，并完成 PVE 三服/独立 DB 闭环、GUI 人工审查及原版对照。G10 需无未解释的资源、地图和 UI 缺口；G11 需干净机器、故障/回滚验证与 24 小时运行。每门禁单独记录 SHA、环境、命令、结果和证据路径。

### 2026-10-15 增量回执

G8 仍为 PARTIAL。`3106ec4efbd6d82eb0f4c1cc60bf53d29f5817cb` 修复了预载任务状态导致 revisioned 战斗被拒绝的问题；VM112 受控 fixture 在任务行保留的前提下完成 Map10 GameIn 和两次权威击杀，CT930 数据库及旧 Map10 服务均已恢复。E2E 最终拾取/重登断言未完成，回执为 [`original-pve-questfix-combat-20261015.json`](evidence/original-pve-questfix-combat-20261015.json)，不升级为完整玩法通过。

| G9 | 真实全新账号建角与首发装备持久化 | PASS | 2026-10-15 | CharacterMake/GameIn 读回装备与生命；清理后账号/角色/identity 为 0 | Map10 1级首战仅护盾伤害，原始起始地图路线待核对 |

| G10 | 原始出生路线静态契约 | PASS (静态) | 2026-10-15 | 恢复库过程明确 Map17/2017，LoginPoint 版本绑定已记录；隔离 PVE 建角/GameIn 已通过 | 移动、玩法、GUI 和持久化尚未闭合 |
| G11 | Map17 原始 HFL 恢复 | PASS (资源) | 2026-10-15 | 从原版 Map.pak entry 240 提取，SHA-256 校验，HFL 解析通过；隔离服务真实加载并完成 GameIn | 9 个使用纹理缺失；视觉 1:1 尚未通过 |
| G12 | Map17 纹理来源复核 | PASS (审计) | 2026-10-15 | 五个本机 Map.pak 候选重新列包，9 个 `tile_17_mou_a_01..09` 均未找到准确条目 | 仍需准确原包或合规公开来源；不得以重命名/替代纹理关闭原版门禁 |

### 2026-10-15 Map17 隔离 PVE 协议增量

`docs/evidence/original-pve-map17-e2e-20261015.json` 记录了 VM112 临时端口 `16117/17117/18117` 的真实 `MoxianClientE2E --no-spawn` 结果：LoginAck、空角色列表、CharacterMake、重列、Map17 CharacterSelect 和 Map17 GameInAck 全部通过，玩家生命 `131/131`。探针账号及角色相关行已清理为 0，临时端口已释放，原版和 Unity 默认服务端口保持监听。该证据只关闭 Map17 协议/出生子门禁，不升级 G8/G10 为完整玩法或视觉通过。
