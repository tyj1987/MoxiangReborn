# 新角色基础属性与普通装备战斗接线

父提交：`52877ba89666fb9670882fd62fc6c5a3a1212f88`。
本批为开发分支代码及隔离验证，不是生产迁移、发布或 ROG 可玩验收。

## 数据兼容策略（部署前必须复核）

- SQLite/MSSQL 为 character_info 新增四个可空列：base_gengol / base_minchub / base_cheryuk / base_simmek，范围0..65535；**无默认值、无旧行 UPDATE**。
- 正常 Agent CharacterMake 在原创建事务内显式写入四项12；失败仍回滚角色与外观装备，不新增管理权限或生产入口。
- NULL、部分属性、类型/范围无效、查询失败均不是已知属性。正式 Map GameIn 记录 `attribute-import-required player=...` 并发送现有 GameInNack，不建立运行角色或保存推测值。选角记录不删除。
- 原因：入场即需要权威生命/防御并可能被怪物攻击，不能让未知角色用默认10/5战斗，也不能将其变为免伤角色。本策略会阻止未导入属性的旧角色入场；**不得直接部署到旧玩家环境**。客户端目前只显示已有入场失败，专用“待导入”界面未新增，明确原因在服务端日志与 NULL 数据状态。
- 原有显式 development fallback 仍仅供旧离线测试使用；本批的新协议回归、准备模式和生产配置不使用它。没有引入新绕过。
- 旧角色必须按实际属性导入；等级不能重建自由加点结果，MXSH character_data 也不含这些原属性。重复迁移保留已导入值，不回填12。

## 来源边界

ROG 回传恢复 `reference/legacy-source/4dddd9a6/[Server]Agent/AgentDBMsgParser.cpp:127-130`：
CheRyuk/SimMek/GenGol/MinChub 均为12，149-155/199-205 传入 JP/其他建角 SP。
报告 SHA256：`2D3CBF4CC23020C3E1864D922F3F4B3F9BFFB5CB2EFF7D74D9B1304FA68C4B29`。
这是恢复快照证据；manifest 的历史提交标记不证明文件属于该提交。
云端没有读取 Windows 恢复文件或独立验证其哈希，且 SP 定义未取得，因此只声称调用方参数已知；本批按父端明确授权将四项12用于现代新角色。

旧字段对应（ROG 回传 MapDBMsgParser.h:704-711 / cpp:2900-2909）：
Gengoal→GenGol、Dex→MinChub、Sta→CheRyuk、Simmak→SimMek。
CharacterManager.cpp:24-67 允许自由加点，Player.cpp:1552-1563 升级只给点；不据等级填值。
基础攻击恢复公式及 CommonCalcFunc 来源限制见 `EQUIPMENT_COMBAT_FORMULA_20261001.md`。

## 本批实现

1. Map 从持久字段加载基础属性，并用于 Actor 与 HERO_TOTALINFO，移除正式路径的四个固定10和伪装备生命100/内力50。
2. `rebuild_equipment_stats` 每次从有效 DBID 穿戴项重建物理攻击区间、防御、四属性加成和装备生命/护盾/内力。外观 icon 不计入拥有装备；基础属性与装备属性分开，卸装不会覆盖持久基础值。
3. 使用 KR/CN ItemApplyRate、臂环百分比、近战根骨/弓暗器敏捷、无武器调用层+5、event hammer 0、原版物理防御公式。当前已恢复 shop/avatar 的基础属性、avatar attack 作为已有输入参与；不伪造缺失的能力、套装等值。
4. 登录装备加载后与合法 MoveSyn 交换后统一重算；失败交换恢复原槽。重复同一 MoveSyn 返回既有 Nack，状态无二次加成。只收缩当前生命到新上限，不借穿戴或登录重算复活/治疗持久死亡角色。
5. Combat 保留 DWORD 攻击 min/max，实际伤害路径按原版包含两端的随机区间取值，不取平均、不把武器值叠加到默认10。未调整当前技能/防御伤害管线、掉落、怪物或技能初始化。现代现有简化技能管线不因此成为完整原版战斗实现。
6. 未支持的强化、稀有、套装、元素项或未知/不匹配穿戴模板明确拒绝该次重算，入场或穿戴失败；不悄悄把这些属性当零。此限制同样需要部署前复核。Japan 基础公式已有独立测试，但装备接线明确仅 KR/CN，未声称 Japan 完整支持。

## 隔离结果与数字

- 四项12、等级1、无装备：攻击16..16、防御8、生命上限125。
- 普通11000（原 ItemList melee13..17）：攻击29..35；卸下回16，重复重算仍29..35。
- 当前 Map10 最低已审目标防御451，攻击上界35仍不能突破最低伤害；本批**不能宣称已经完成击杀/掉落/拾取闭环**。
- 准备模式只读核验新角色四项12，仍走既有 pending grant→MapServer claim→MoveSyn；没有直接写属性或强角色。结果列 expectedStarterStats，明确 runtimeCombatMeasured=false。
- 旧 level48 fixture 明确标为 attribute-import-required；攻击10预算仅保留为历史证据，不再当作当前生产结论。

## 验证与剩余阻碍

```text
cmake --build /workspace/moxiang-audit-20261001/build-pickup --parallel 3
ctest --test-dir /workspace/moxiang-audit-20261001/build-pickup --output-on-failure
python3 -m unittest discover -s modern/tests/unit/tools -p 'test_unity*.py'
```

portable 目标编译真实 PlayerState、装备重算、skill_caster、SQLite adapter 和 schema migration；
验证旧行NULL/已导入值保留、边界拒绝、新角色无装备/装备/重复/卸装/重新构建、护甲生命不治疗、远程/臂环、未知选项不部分发布、真实伤害使用29..35两端。

最终结果：C++ portable **111/111 PASS**（包含原 PlayerState/CharacterCalc/SkillCaster 回归及真实 ItemList11000 解析），Python Unity 工具 **42/42 PASS**。`git diff --check` 通过。
云端证据：`/workspace/moxiang-audit-20261001/equipment-ctest.log`、`equipment-map-syntax.log`、`equipment-agent-syntax.log`、`equipment-handler-test-syntax.log`。

Windows `mxh_server_handler_tests` 新增：

- `EquipmentPersistence.ProtocolCreateEquipUnequipRepeatAndRelogin`：正常 Agent CharacterMake→四项12落库→既有队列领武器→Map MoveSyn→重复Nack无累加→卸装/再装→新Handler重登→卸装再重登。
- `EquipmentPersistence.OldUnknownAttributesFailAdmissionWithoutBackfill`：未知及部分导入拒绝，完整显式导入后允许入场并保留不同于12的值。

这两项未在云端运行。Linux 直接检查 Agent/Map/handler test 遇到原有 LoginRateLimiter 默认参数编译问题；Map 另有 localtime_s 与 Linux long long bind 歧义，不把该检查称为 Windows 构建通过。
旧正式模式测试夹具中未写基础属性的角色现在属于 unknown，会被入场门禁拒绝；这些历史 fixture 与新策略的兼容调整、Windows 全量回归仍需 ROG 处理，不能沿用旧“全绿”结论。

ROG 下一步先运行上述两个新协议测试、schema/装备单测及新角色准备模式，核对编译与正常登录；再核旧 fixture 的显式属性来源，不能靠测试/生产默认回填掩盖问题。旧玩家导入政策和客户端“待导入”提示须在任何部署前确认。技能初始化仍独立处理。
