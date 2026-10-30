# 正常出生路线与版本依据

当前下一候选采用明确命名的 `restored-db-map17` 出生配置。现役 Map10 角色和服务保持；Map10 继续作为渲染、实体路由及等级匹配后的玩法对照。此页不是 Map17 建角或可玩验收通过声明。

| 依据 | 已核实行为 | 使用边界 |
|---|---|---|
| 固定旧源码非日版 Agent 建角 | 传入地图96、登录点2096 | 不能直接等同恢复库最终落库值 |
| CT105恢复副本 `mhgame` 与本地旧库PVE恢复副本 `PCDefault_mhgame_20261009` | 两份 `MP_CHARACTER_CreateCharacter` 原字节相同，INSERT明确写地图17、登录点2017；角色表触发器0 | 只读元数据验证；未执行过程或据此创建测试角色 |
| 原版地图登录点 | Map17点位(18000,37200)，无既有InitPoint且切图索引0时，x/z各加[-250,249]随机偏移 | modern目前通用首登(25000,25000)尚需按明确配置校准；保存点优先级另验 |
| Map17准确HFL与33张已解DDS | exact源SHA已保全；33张均64×64 DXT1 | 9个有名纹理缺失；13个使用中的“1”槽显示语义未决，不视为13张已知缺图 |
| Map96本地四份HFL | 均为同一development placeholder，非发布源 | 准确版本及整图覆盖阻塞保留；不能替代原资源，也不是本恢复库Map17出生路线的必经门槛 |

恢复过程还包含初始物品插入、能力经验和活动礼物定制；只采用有版本依据的出生配置，不整体复制这些定制数值。Map10刷怪最低53级、当前目标57级；Lv1角色在此每击1伤不能作为正常新手闭环。Map17怪物类型1–6为低级候选，伤害、升级、自然掉落仍须原公式、权威事件和数据库逐项对照。

两份过程UTF8 SHA256：`ced21d16385d1029ba2fc43852bb639eddf0ef1a16490e6bfacf93e78f7e36bc`。采集v2同时对比SQL Server UTF16LE原字节SHA；v1因截去sqlcmd分段空白，其定义摘要已明确作废。

证据：`audit/2026-10-09-reorg/original-birth-static-contract.json`、`original-creation-procedure-readonly-v2.json`、`original-birth-table-readonly.json`、`unity-candidate-review/map17-source-inventory.json`、`map17-local-provenance-0915.json`。玩法门禁见[VERIFICATION_MATRIX](VERIFICATION_MATRIX.md)。
