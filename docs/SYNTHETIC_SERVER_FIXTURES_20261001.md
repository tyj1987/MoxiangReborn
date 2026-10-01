# 三项Handler历史资源依赖的隔离

本批隔离测试资源依赖，不修改生产游戏数据或解析器。ROG在补入经SHA核对的旧文件后报告原Handler173/173通过；不带未追踪文件时170/173。该运行结果来自协作端，不是云端亲测。

## SHA校验与测试注入修正

ROG在`8910dcf265c7b2d81c0f28cf89b0789380961b41`报告Windows构建通过、默认Handler179/181，登录点和惩罚表两项失败：合成文件在生产加载器的固定SHA校验处被正确拒绝，尚未进入解码器。这两项失败不能作为验收通过。

修正采用仅在Handler测试翻译单元定义的`MapHandlerResourceTestPeer`，把真实解码器产生的类型化表交给Handler的私有安装方法。默认合成回归通过该入口检查行为；可选原资源参考测试仍调用生产文件加载器。安装方法不是公开API，并有编译期访问性断言；没有环境变量、运行时开关或任意文件加载的生产入口。

生产加载器的profile检查、旧表清除、文件读取/长度检查、四个固定SHA及摘要比较均保留，仅将校验后的赋值封装为私有方法。新增`ProductionResourceLoadersRejectSyntheticHashesAndClearStaleTables`，对current和legacy的登录点/惩罚表分别验证：相同合成内容可经测试入口安装，但生产文件入口必须拒绝并清除旧表。该Handler回归需ROG运行，云端12项可移植测试不包含它。

## 默认回归与参考测试

| 原测试 | 默认回归输入 | 显式参考测试 |
|---|---|---|
| LoginPointRequestAppliesPenaltyAndRestoresOnRelogin | 原创登录点文本，分别封装为current和MHFile；仍检查实际Handler复活、惩罚、重复/错误请求、持久化重登 | MapHandlerReferenceTest.DISABLED_LoginPointRequestAppliesPenaltyAndRestoresOnRelogin |
| RecoveredMonster10BinSpawnsAllGroups | 改名SyntheticMonster10BinSpawnsAllGroups；原创3组×76个出生定义，保留228实例/广播/客户端解码断言 | MapHandlerReferenceTest.DISABLED_RecoveredMonster10BinSpawnsAllGroups |
| ExpPenaltyLoadRequiresMatchingProfileAndClearsStaleTable | 原创4行惩罚表，current与MHFile双封装，保留错误profile、丢失文件和旧表清除断言 | MapHandlerReferenceTest.DISABLED_ExpPenaltyLoadRequiresMatchingProfileAndClearsStaleTable |

默认与参考测试调用同一个检查函数，原来的67、15、12条ASSERT/EXPECT表达式逐条保留。参考测试是显式禁用的可选数据验证，不是缺文件时无条件skip，也不计入通过；显式运行时缺文件仍断言失败。真实资源的意义仍是原数据兼容性，合成通过不能替代它。

生成器在`modern/tests/unit/server/synthetic_server_resources.hpp`，只向自建唯一临时目录写文件并自动清理；不复制旧资源或写入PlayDH/reference。文本与数值为测试原创：登录点包含多点优先级、0/65535坐标边界与有效的排除地图58；惩罚表含零值和缺失等级；怪物组具有不同kind/位置与228个唯一源ID。固定格式变换按现有读取器的容器契约编写，非生产资源生成器。

## 验证

Linux实际构建并运行：此前5项PickupWire、3项ItemId，加4项SyntheticServerResources，共12/12通过。新夹具测试调用真实login/penalty解码器与`load_ai_group_list_bin`，覆盖两profile、CRC损坏、长度损坏、错误profile、坐标边界、缺失等级回退，以及全部228个出生定义。治理和diff检查通过。完整Handler仍依赖Windows目标，云端未宣称运行通过。

```sh
cmake -S modern/unity-core/tests/portable -B modern/out/pickup-wire-tests
cmake --build modern/out/pickup-wire-tests
ctest --test-dir modern/out/pickup-wire-tests --output-on-failure
```

Windows先在不补入旧reference文件的隔离checkout构建并运行默认Handler：

```text
mxh_server_handler_tests --gtest_filter=MapHandlerTest.LoginPointRequestAppliesPenaltyAndRestoresOnRelogin:MapHandlerTest.SyntheticMonster10BinSpawnsAllGroups:MapHandlerTest.ExpPenaltyLoadRequiresMatchingProfileAndClearsStaleTable:MapHandlerTest.ProductionResourceLoadersRejectSyntheticHashesAndClearStaleTables:SyntheticServerResources.*
mxh_server_handler_tests
```

有经核对的只读原资料时，单独运行参考组（使用实际构建输出路径）：

```text
mxh_server_handler_tests --gtest_also_run_disabled_tests --gtest_filter=MapHandlerReferenceTest.*
```

参考组仍读取`reference/legacy-source/4dddd9a6/SWorking/Resource/Server/{LoginPoint,Monster_10,ExpPenalty}.bin`，不授权本脚本下载或补入这些资产。

本批只处理ROG定位的三项Handler失败。其他目标已有的真实资源测试（如`game/exp_penalty_test.cpp`的全等级历史对照、`ai_group_loader_test.cpp`的真实114组/空Map12检查）未在本批改写，不能据此声称全仓历史资源依赖已消除。默认Handler的登录复活测试仍使用已跟踪的CharacterExpPoint/MapKindInfo/current ExpPenalty；源资产与ROG的差异仍需按既有基线说明记录。
