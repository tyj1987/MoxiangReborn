# 装备战斗属性：恢复公式与持久化阻碍

后续状态：已获新建参数四项12的恢复证据并开展普通装备接线，见 `EQUIPMENT_COMBAT_HYDRATION_20261001.md`。下文保留本报告提交时的边界。

本轮父基线：`1c04c25043414baa47588e1f32dc64e55eba090e`。
本次仅加入未接入生产的基础攻击纯函数及 golden 测试；**装备/卸装/重登战斗属性修复尚未完成**。

## 来源边界

ROG 回传恢复文件位置：
`E:\Workspaces\moxiang\repo\reference\legacy-source\4dddd9a6\[CC]Header\CommonCalcFunc.cpp`。
回传 SHA256：`338FC7BBB57113C5044A7B6CD41F1B78B7A4A9537EB4B9BBC783235A64E04D11`。
manifest `legacy-source-4dddd9a6.json` 标记来源为
`4dddd9a6f609f2c0d4c961ea0a159aff4fd449b9`，但该 CPP 不在此提交中，是恢复源码。
云端按本次回传的公式转录，没有直接读取该 Windows 文件或独立验证其 SHA256，不能称为该 Git 提交的可验证原始 blob。

云端完整阅读了仓库 `墨香【源码】/[Server]Map/Player.cpp:3145-3295`；
`git hash-object` 返回 `9644287a3a1d269392847d8b8f449360f43e3175`，与 ROG 一致。
同时阅读 `StatsCalcManager.cpp` 的装备汇总与角色基础属性计算、
`ServerSystem.cpp:1066` 的 HERO_TOTALINFO 初始化入口、
`MapDBMsgParser.cpp:2160` 的 CharacterHeroInfoUpdate 持久化调用。

## 已锁定的算术

`modern/include/mxh/game/base_attack_power.hpp` 将 WORD stat 和 weapon 转 double：

```text
KR/CN = DWORD((Weapon * ((Stat+200)/200) * ((Stat+1000)/500) + Stat)
             * 0.74 + min(Stat-12,25))
Japan = DWORD(Weapon + Stat + Stat/3)
```

近战取总 GenGol，远程取总 MinChub。纯函数不增加徒手5、Japan level*4，也不把10加到结果。
这些是 Player.cpp 调用层职责；原版近战/远程分别传入相应 min/max 端点。
KR/CN 的臂环倍率在函数返回值与能力攻击相加后应用；Japan 在调用前调整 Weapon，再加 level*4 和能力攻击。
能力、头像、套装、强化、稀有、属性伤害没有在此纯函数中伪造或宣称已接入。

golden 示例（**stat=10 只是测试输入，不是已经确认的新建默认值**）：

| Stat | Weapon | KR/CN |
|---:|---:|---:|
| 10 | 5 | 13 |
| 10 | 13 | 25 |
| 10 | 17 | 32 |
| 11 | 13 | 27 |
| 12 | 13 | 29 |
| 36 | 13 | 74 |
| 37 | 13 | 76 |
| 38 | 13 | 76 |
| 200 | 100 | 528 |
| 65535 | 65535 | 2121103139 |

不模拟负浮点转 DWORD 的编译器行为。计算结果小于0或超出 DWORD 时返回空 optional；
这是显式输入边界，不声称为原版溢出兼容。尤其 stat=0、weapon=5 的公式结果为负，
不能直接把现代当前未初始化的零属性送入生产公式。纯函数目前没有生产调用点。

## 尚缺的持久化证据

现代 `schema_migration.cpp:28-54` 的 character_info / modern_player_state，
对应 MSSQL 创建/升级语句及 `deploy/database/mx_modern_schema_mssql.sql`，
均没有四项基础属性列。`character_data` 虽存在，但现代 Agent 建角没有写入，Map 没有解码其 HERO_TOTALINFO 的路径。

- `agent_handler.cpp:1674` 正常 CharacterMake 只写外观、等级1、地图等字段；随后写外观装备图标表。
- `map_handler.cpp:load_char_data` 只读取身份、外观、等级、经验和货币。
- `map_handler.cpp:313-316` 回包写四个常量10；`:2494` 仅给 spawn.base.level 赋值。
- `PlayerState` 四项属性当前来自零初始化 CalcBaseStats。这不能当作原版数值来源。
- 原版 ServerSystem 把已取得的 HERO_TOTALINFO 传给玩家，并未提供建角默认值。
- 原版 CharacterHeroInfoUpdate 证明应持久保存哪些属性，却没有证明现代旧账号如何回填或新建默认值是多少。

按本次“若缺持久字段先报告、不猜”要求，尚未创建默认10的迁移、改建角默认值、
或把装备攻击接入错误的零基础属性。需父端补充原版正常建角存储过程/初始化数值，
以及已有角色可用的属性迁移来源。若不存在旧属性，必须明确恢复政策，不能静默填0或10。

## 下一批最小接线与验证

1. 依据确认的建角/恢复规则新增 SQLite 与 MSSQL 属性持久化；建角、加载、保存使用同一来源，GameIn 回包不再固定四个10。
2. 从有效 DBID 穿戴项重新构建普通装备加成，保留原版臂环、等级折减与武器种类分支；外观 icon 不能当作拥有物品。
3. 将 min/max 派生攻击保留到实际战斗抽样接口，不能任意取最小值或平均值。统一登录、穿戴、卸下重建，避免重复请求累计加成。
4. 增加无装备→装备→卸下、重复 MoveSyn、保存/重登一致性回归；未接入系统独立列明。
5. Windows 三服和 Unity 由 ROG 验证；技能初始化独立处理。生产属性接线改变后再重审旧 pickup fixture 的攻击10预算。

## 本轮验证

```text
/tmp/moxiang-audit-tools/cmake/data/bin/cmake --build /workspace/moxiang-audit-20261001/build-pickup --parallel 2
/tmp/moxiang-audit-tools/cmake/data/bin/ctest --test-dir /workspace/moxiang-audit-20261001/build-pickup --output-on-failure
17/17 PASS（既有12项 + 新增5项公式测试）
```

新增测试覆盖 double 中间值、一次截断、11/12 和36/37/38边界、WORD最大值、Japan分支、负结果拒绝。
Linux portable 构建通过；不代表 Windows 全量构建、装备协议回归、Unity 或真人验收通过。
准备模式未修改：继续使用既有 modern_item_grant pending 队列，由 MapServer claim，未新增生产授权入口。
