# 可执行的隔离角色准备步骤

本批实现正常账号/角色/装备准备，不宣称已经构建出可战胜Map10怪物的角色。当前装备→战斗攻击恢复缺口和已学技能持久化缺口仍存在；最终`passed=false`、`combatReady=false`、`selectedTarget=null`。只有真实协议和数据库证据一致，才单独标记`preparationPassed=true`。

```text
python modern/tools/unity_three_server_smoke.py --player <本提交构建的DevelopmentPlayer.exe> --prepare-pickup-input
```

该模式与`--pickup-loop`及其他模式互斥，只支持新建SQLite和固定loopback三服。没有数据库路径、账号、装备ID或任意技能覆盖参数；普通Player默认不运行。ROG应在GUI阻碍解除后同步提交重新构建，不执行未提交工作区。

## 真实执行步骤

1. harness建立唯一32位runId目录和`fixture.db`，DbTool migrate/register各限30秒；密码随机生成，正常HSEL鉴权。数据库必须只有本次生成的`ux_<runId前8位>`身份，尚无角色。
2. `prepare-create` Player调用CharacterCreatePanel按钮事件，全部外观选项保持默认，经CharacterMake创建角色并选入Map10。要求收到自己的单一新角色、GameIn、完整库存、地图就绪、正常Disconnected/Idle；SQLite必须由服务端产生level1角色和默认外观装备11000/23000/27000。生成器不插入character_info、成长属性或技能行。
3. 生成器复核所属账号、角色、等级、DB路径、没有既存库存/授予，再向既有`modern_item_grant`队列插入一条pending请求：11000×1，唯一幂等key、创建者、原因齐全。**只写请求，不写`modern_player_item`或claimed状态。**
4. `prepare-equip` Player正常登录，MapServer在GameIn标准路径claim请求并分配真实库存ID；客户端必须收到对应物品，再调用普通`MoveInventoryItem`穿至81号格。要求服务端ItemMoveResponse成功及其后的完整库存，正常退出后最多20秒核验claimed和装备容器1/slot1持久化。
5. `prepare-verify`另起Player再次登录，要求同一个角色、相同持久DBID的11000仍在81号格，正常断开，并再次读取SQLite核对。
6. 记录实际已材质化的怪物objectId/kind，对当前已核Map10六种kind列出HP/防御与不适配原因；无场景证据则明确`scene-target-unavailable`。不会固定选择50023、不会试用任意高伤技能、不会预填击杀或拾取结果。

Map10输入仍不适配不是等待GUI可解决的问题：`PlayerInfo.combat.phy_attack`目前未从角色属性/装备刷新，仍为10。仅装备、显示等级或快捷栏变化不能修复这个数据流。DbTool没有受支持的“完整学技能/升级成长”命令；`modern_player_skill`仅用于快捷栏，并非已学技能授权，因此本批不填它来调用高伤技能。

## 输入来源与适配范围

生产ItemList解析器实读11000：EquipKind1、LimitLevel1、四项属性限制0、MeleeAttack13–17，适合正常新角色装备准备。ItemList SHA：`07d25fb98ee7f02aae3b5950ab4472847742d989775078985576c7c94a3957bd`。准备时连同既有MonsterList/SkillList/Monster_10审计SHA一起核验，变更即要求重新审查。

预期的正常准备参数是新建level1角色和真实初始武器，**不是**虚构的可击杀level53–61怪物角色。现有Unity可用场景和默认战斗字段中没有已核实可战胜的正常目标；未导入新地图/怪物，不降低怪物数值。下一步仍须把标准属性/装备计算接入登录、穿脱和重登的战斗快照，或提供已经接通完整初始化的正常低级场景。该生产数据流缺口没有在测试生成器中绕过。

## 有界执行与产物

- 每服务监听20秒；每Player外层75秒，内部登录30秒、装备15秒、正常断开10秒。战斗未就绪时不发送攻击；既有击杀探针120秒限制未放宽。
- 只停止harness拥有的Popen；没有按名称杀进程。路径必须是本次three-server/runId/fixture.db，拒绝符号链接、外部DB、多身份、多角色、被编辑等级、已有库存或重复grant。
- 输出`pickup-preparation-summary.json`、三个阶段`pickup-report.json`/`pickup-events.jsonl`/`player.log`、总summary和三服日志。setup失败也由顶层失败边界落盘。
- 报告保留参数/来源SHA、CharacterMake结果、grantId、真实角色ID、数据库与协议验证、场景目标、技能验证缺口。密码不入报告。按钮/组件方法自动调用不是真人鼠标操作。

云端Python工具41/41通过，包括正常准备编排但整体不能误报combat成功、只写pending请求、外部/共享/已有数据拒绝、重复grant拒绝、Player超时失败、CLI互斥约束。C#和Windows三服尚未在云端运行，ROG执行前不能把这些单测当作准备链已验收，更不能当作击杀/掉落/拾取通过。
