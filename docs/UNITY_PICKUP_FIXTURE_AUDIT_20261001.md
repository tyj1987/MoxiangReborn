# 真实拾取探针战斗输入审查

本报告独立于shader补丁。ROG在0d456d7运行：16次Hit、每次damage=1、目标剩5420生命，玩家累计受伤340后死亡；未进入kill/drop/pickup/relogin。新实现不把这个结果转成通过，也不重复启动已证实不可能完成的输入。

## 数据与代码结论

生产解析器`MonsterCatalog::parse_bin`、`load_ai_group_list_bin(playdh-current)`、`load_skill_list`在云端读取当前原始资源，得到：

| 地图 | 首个object ID | kind | 等级 | 生命 | 防御 | 攻击 |
|---|---:|---:|---:|---:|---:|---:|
| 10 | 50000 | 105 | 53 | 5096 | 451 | 105–131 |
| 10 | 50023 | 73 | 55 | 5436 | 480 | 108–135 |
| 10 | 50074 | 103 | 57 | 5788 | 510 | 112–140 |
| 10 | 50130 | 102 | 59 | 6150 | 541 | 115–144 |
| 10 | 50157 | 104 | 61 | 6523 | 572 | 119–149 |
| 10 | 50227 | 219 | 57 | 435024 | 1854 | 692–868 |

Map2现有五种目标最低等级46、防御325、生命3297，同样不是当前裸角色的低级替代。没有更换地图或生成怪物。

当前测试角色只插入等级48与位置，无属性、装备、已学技能初始化。技能1的额外物理攻击=0、属性攻击=0、耗MP=0。`PlayerCombatStats`默认物攻10、物防5；`MapHandler::handle_userconn`同步level和vitals，但没有把属性/装备计算结果填入这两个战斗字段。`load_player_items`恢复物品，`load_character_equipment`提供外观，不等于同步战斗能力。快捷栏缺省1/2/3/10也不是完整的已学技能初始化。

`skill_caster_calculate_damage`对该目标得到`max(1, 0+10-480)=1`，属性伤害0；1.5倍暴击转整数仍为1。即使120秒内无死亡、无miss，每0.8秒提交一次的乐观上限也只有151伤害，远低于5436。角色生命为48×5+100=340（体魄默认0），吻合ROG实际死亡记录。提高等级不会自动提高目前仍为默认值的物攻；填`modern_character_equipment`外观行也不能解决。

## 本批实现

`--pickup-loop`在建库/注册/启动服务之前生成`pickup-fixture-audit.json`，核验三份资源SHA并明确失败于`combat-fixture-preflight`。已知输入返回`combat-stats-initialization-gap`，文件缺失/变更则返回`fixture-audit-source-mismatch`，要求重新审查而不是沿用静态结论。顶层失败summary引用audit文件；不调用Player、改DB、发奖励或放宽成功条件。**当前模式因此仍被阻塞，不是已修复的可玩闭环。**

这是现有固定测试profile的已知失败保护，不是通用装备模拟器。不会根据未验证的数值自动批准另一个profile；战斗初始化完成后应以新证据更新profile和该保护。

## 合适输入与标准初始化方案

当前代码下没有已验证可用的Map10输入，不能诚实给出一组“穿上即能打赢”的物品ID。建议下一批使用**真实创建并合法成长至与目标相当等级的角色快照**作为隔离输入，而非只写level：

1. 新账号走现有注册与角色创建协议，保留默认属性和起始装备来源；通过已核对的成长/加点规则生成或回放等级55左右的持久属性和已学技能状态。先核对这些字段的服务端恢复链，不能只改显示等级。
2. 装备只能来自当前ItemList中可穿戴且满足等级/属性/职业条件的真实条目。若为了缩短测试准备使用现有`modern_item_grant`管理队列，应仅在战斗前提交有唯一idempotency key的装备请求，由MapServer正常claim，客户端正常穿戴并收到权威库存；不得预写拾取结果或战斗后补库存。具体装备ID须待标准属性恢复和装备计算可验证后选定。
3. 补齐/核验标准属性与装备→既有CharacterCalc/Battle计算→`PlayerInfo.combat`的同步，在登录、穿脱装备及重登时一致；这是当前缺失的生产初始化链，不应在探针中直接设攻击/生命或换任意高伤技能。使用原公式，不更改伤害、掉率、怪物生命或范围校验。
4. 选用角色确实已学且可用的基础技能；从权威角色与目标数据计算保守击杀/存活预算后再固定目标。可优先评估Map10 kind105（level53、HP5096、def451）；当前50023仍可作为同级目标，但尚无可验证的装备/属性组合。预算不是胜利证据，最终仍需真实击杀、自然掉落和全部持久化门槛。
5. 若预先装备由grant进入库存，探针必须先保存权威基线，并把“新增两件掉落、原装备完整保留”与现有“空库存后两件物品”区分；需要同步扩展数据库container映射与验证，不能删除原装备来满足数量断言。

上述初始化链和非空装备基线尚未实现/验收，本批只定位并阻止错误输入，不冒称给出了可运行的训练角色。需要下一开发批次处理，之后再恢复真实闭环验证。shader修复可以独立复测。

## 可重复证据

在仓库根目录，用实际生产解析器编译只读检查工具（输出放仓库外）：

```sh
g++ -std=c++20 -Imodern/include modern/tools/inspect_pickup_fixture.cpp modern/src/monster_catalog.cpp modern/src/mh_file_ex.cpp modern/src/server/ai_group_loader.cpp modern/src/game/skill_list_parser.cpp -o /tmp/inspect_pickup_fixture
/tmp/inspect_pickup_fixture
python3 -m unittest discover -s modern/tests/unit/tools -p 'test_unity*.py'
```

资源SHA：

- MonsterList.bin：`fb7ee93e66ea9321577fe4a5031e98689d346b6fdbd5852e05d69ad7a952eebe`
- SkillList.bin：`9b5d1fac408c610252e419f6c55b12e3bc38f9ed436fdebbdc01ec664da906b7`
- Server/Monster_10.bin：`50033323336100da141443f3b563c62312586d149edf5cfdd78262b26d15cc01`

云端Python工具34/34通过，包含乐观伤害预算、真实来源匹配、来源缺失拒绝；生产解析器检查程序编译运行成功。Unity/Windows新测试尚未运行，ROG的失败结果仍有效。
