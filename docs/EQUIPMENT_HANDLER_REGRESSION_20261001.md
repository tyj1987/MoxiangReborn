# ROG 回归修复：普通耐久与测试属性

起点 `5fd0e0e0b6b9fc02267eb2076abc694d20fb1d18`。
ROG 回报：native/equipment/schema111/111通过；新协议1/2通过，普通发放武器Durability100导致穿戴Nack；旧MoveSyn6/6通过。完整handler出现旧夹具属性缺失失败，随后空optional解引用终止进程。

## 第一笔：逻辑兼容修复

提交 `345d1338e8555e44478de2a47c46c321e793f814`，已推GitHub。

- 原版 `ItemManager::IsOptionItem` 将非零Durability视为非叠加装备选项索引。
- 现代 `make_item` 默认100，正常pending grant与掉落路径也显式100；这不是原版选项索引语义。
- 装备计算只接受既有现代普通值0/100，仍拒绝1、99、101、UINT32_MAX以及稀有/套装。没有改发放物品为0，没有放开所有非零值，也不新增授权入口。
- 未来原版选项导入必须显式区分其语义；原选项ID100不能直接当现代普通耐久100复制入库。本次不是原版强化选项支持。
- 测试穿戴帮助函数改用实际 `make_item` 默认100，包含真实ItemList11000的测试。云端portable **112/112通过**，证据 `/workspace/moxiang-audit-20261001/durability-ctest.log`。

## 第二笔：测试迁移与失败保护

仅测试/文档变更，不改生产属性门禁或数据库默认值：

- 21个旧严格模式测试内23处受控角色创建语句显式写四项12，作为本测试选定输入，不是依等级推算或用户角色回填。新角色默认和真实旧角色导入仍由生产规则约束。
- `EquipmentPersistence.OldUnknownAttributesFailAdmissionWithoutBackfill` 保持逐字不变，继续测NULL、部分导入、显式不同数值导入。
- 所有直接 `player_runtime_snapshot(...)->` 改成保存optional并先 `ASSERT_TRUE`，背包填充循环每轮也检查；同类任务/轻功/复活结果解引用加保护。失败会报告断言，不再沿空值继续运行。
- `ExtendedShopContainersSurviveExitAndFreshHandlerRelogin` 的普通穿戴成功样本改用明确模板777、Durability100、RareIdx0；其他容器仍保留原极值/稀有字段来验证保存。独立新增 `UnsupportedWornOptionsRejectWithoutRewritingItems` 覆盖65/0及100/4选项失败且DB记录保留，不能用正常样本变更掩盖非法边界。
- 抽取当前24条包含明确属性的字面量INSERT，逐条在使用实际schema及新增列的隔离SQLite执行，全部通过；unknown测试与父提交文本一致；直接运行角色快照裸解引用为0。

## 验证限制与后续

云端Linux直接解析handler测试仍被原有 `LoginRateLimiter(Config config = {})` 编译问题阻塞；不把无新增诊断当作完整编译通过。需ROG重建并执行：

1. `EquipmentPersistence.*`，特别是正常管理队列Durability100→穿戴→卸下→重复→重登。
2. 原失败的身份/外观、扩展容器、商城恢复、持久死亡、原地复活测试；随后完整handler。
3. 尚未被原崩溃运行到的商城穿戴分支需关注：例如 `OnlineRealtimeExpirySearchesOriginalWornContainers` 的container1使用商城种类264/计时Durability1，而新普通装备计算器仍明确不支持这类解释。不能把真实商城样本换成普通装备来假装通过，也没有在本次逻辑修复中放开该分支。若ROG确认失败，应按商城字段的原规则另作生产修复。

ROG Python临时目录写入拒绝按环境阻碍记录，未改工具逻辑、系统目录或权限。
两笔均未部署、未执行生产迁移；旧unknown拒绝仍是未发布候选行为，不声称旧角色兼容或全量handler验收完成。

## 后续：限时商城外观入场

ROG 在 `2b7d76c680f6e014f6cc0cbc60546e6c7cd6400a` 回报 native112/112、EquipmentPersistence3/3；完整handler190/192通过、3 disabled。两个失败分别为商城限时穿戴入场，以及StrictGameIn的成功场景仍缺四项基础属性。

本逻辑修复只处理前者：`ShopItemManager::collect_realtime_expired` 使用模板SellPrice=StoredTime及独立Param/BeginTime/Remaintime记录处理恢复/到期，物理ItemBase不应套普通装备耐久/选项解释。仅MAKEUP261、EQUIP264且ItemType11、StoredTime、无任何战斗属性/效果的模板跳过普通装备计算；保留物理字段，由既有商城管理器负责到期。稀有/套装仍先拒绝，普通装备仍只接受0/100，带属性的商城项仍不支持。没有改真实限时handler样本。

新增portable测试验证两类外观重建、重建后的重新载入及移除不改变裸装数值，记录Durability1保留；并覆盖属性、攻击、元素、恢复、技能、稀有/套装及非实时模板拒绝。构建命令：`cmake --build /workspace/moxiang-audit-20261001/build-pickup --parallel 3`；`ctest --test-dir /workspace/moxiang-audit-20261001/build-pickup --output-on-failure`，**114/114通过**。证据：`/workspace/moxiang-audit-20261001/timed-appearance-ctest.log`。这不是Windows完整handler验收；需ROG重跑上述两个失败、EquipmentPersistence及完整handler。
