# Unity 复活迁移证据与未完成边界

本记录依据当前原始源码只读检查，不表示复活已实现。死亡事件和输入限制不能替代复活。

## 已核实入口

- 原始 `[Server]Map/MapNetworkMsgParser.cpp:3836` 的 MP_REVIVEMsgParser 分别调用 RevivePresentSpot、ReviveLogIn、ReviveVillage；事件地图入口直接返回。
- Category::CharRevive 为32，原地/登录点/村庄请求分别为0/3/6；modern/include/mxh/server/agent_charrevive.hpp已有这些常量，但转发分类器不证明Map运行时实现。
- `[Server]Map/Player.cpp:3867` 从服务器 SendMsg 发出 UserConn ReadyToRevive(77)，不是客户端请求。其前置检查包含准备状态、搜身状态和SOS状态。
- CharacterRevive(44)使用MOVE_POS，发送角色ID和压缩坐标。Unity实现前必须与该结构的实际布局核对，不能复用无关快照布局。

## 原地复活规则

`[Server]Map/Player.cpp:2162` 起：必须处于死亡状态；被搜身、正在退出分别拒绝。
末尾2335行恢复最大生命的30%，2336行清空内力。中间逻辑涉及地图类型、战斗频道、
等级至少5、GFW死亡标志、韩服条件编译及保护道具。普通未保护金钱分支扣6%，经验损失
使用等级经验与条件倍率计算；不能将这一分支常量推广为所有模式通用规则。
保护次数、道具结束、经验下降和宠物亲密度也有副作用。

比武复活是另一条路径：2136行ReviveAfterVimu，在当前位置结束死亡状态、清除谋杀索引、
恢复最大生命30%；不能用它替代普通地图复活惩罚。

## 实施要求

先完整提取三条复活规则和依赖数据，明确当前modern缺失状态；在服务端事务内应用消耗、
经验和位置，失败保持死亡状态。随后接入Agent路由、C ABI命令和事件、Unity选择UI。
覆盖重复请求、存活角色、其他连接伪造、换图、退出、数据库写入失败、保护道具及特殊地图。
SQLite/MSSQL各自完成死亡→选择→复活→重登验证，再验收动画和真人操作。
未核实的复活路径不得以免费满血重置替代，也不得声明完成。

## 登录点与村庄补充核验

- Player.cpp:2348 ReviveLogIn：死亡/搜身/退出检查同原地；GetRegenPoint使用角色登录点和换图点，攻城按队伍改用1103/1102，最终采用RandPos。宠物同步传送，向队伍发复活坐标。
- 登录点结束死亡状态后调用ReviveLogInPenelty；CHINA攻城分支跳过该调用，KOR有行动惩罚分支，因此不能无条件套用普通恢复。普通惩罚末尾2593行恢复最大生命和护盾30%、内力归零。登录点无敌30秒，HK为60秒。
- Player.cpp:4414 ReviveVillage：CHINA条件编译跳过该惩罚块，其他分支涉及保护道具和等级；普通未保护金钱扣4%、等级经验扣1%。最终发送SiegeWar ReturnToMap(GetReturnMapNum())并RemovePlayer，不是当前位置复活。
- 原地恢复也包括最大护盾30%（2337行），此前记录只有生命和内力，现补齐。

## 必须接入的经验惩罚资源

原始[CC]Header/GameResourceManager.cpp:4107从Resource/Server/ExpPenalty.bin加载；
每条读取等级整数、fNow浮点、fSave浮点，按等级建立表。
Player.cpp:2303原地使用fNow，2557登录点读取同一表。缺失等级时原地有3.0默认百分比，
仍须核对登录点及特殊分支，不能硬编码所有等级3%。

当前实际文件为modern/data/PlayDH/Resource/Server/ExpPenalty.bin，SHA-256：
`7ABCC8211E0BFFA58CE7658883C79BF040E748A42568701B773E4ED3F0809419`。
本轮搜索modern/src和modern/include未找到对应LevPenalty解析实现；下一步需使用现有
CMHFile兼容解析层导入此表，验证完整条目、精度与原版默认行为，再接服务端复活事务。
原资源未修改。本轮为源码与资源核验，未新增运行时复活功能或执行新的行为测试。

## 实际容器验证：不能直接复用SkillList读取器

当前ExpPenalty.bin长1025字节，开头uint32为1025，与总长度相同；若误按普通MHFile
12字节头解释，type为3452828107、payload长度为3504600459，与实际长度不符。
因此不能直接复用SkillList的头部和位移解码算法。

modern/src/mh_file_ex.cpp的read_mh_bin会将这种长度前缀当作旧2008格式处理，
但代码明确说明长度标记也用于当前opaque配置。read_server_mh_bin(playdh-current)
调用的decode_opaque_server_payload目前限定20字节配置头和AIGroup的$Group 1文本，
并不证明支持ExpPenalty。复活惩罚接入前须取得该表的可靠解码证据；不能把容器读取
成功、任意可打印文本或旧配置的表当作当前配置数值。此次未创建一个输出猜测数值的
运行时解析器；原始SHA-256保持前述值。

后续候选恢复取得进展：modern/tools/audit_exp_penalty.py固定上述源SHA，跳过24字节
容器头后，以数字、点和空白字符为允许集合求解8字节重复XOR；每个位置恰有唯一候选。
解码得到连续1–99级、每行level/fNow/fSave三列，48级为2.4/1.9，99级为1.7/1.4。
工具拒绝未审阅源摘要、非唯一解、列数错误、等级不连续和非法百分比。
候选明文SHA为079b66fcee10abd4aa3620716099e5abcce6bc6ce3058d4cc6c92a9430536d4b。
实际运行通过99条与48级字段断言，输出保存至
modern/scratch/2026-09-14-map10-entities/exp-penalty-audit.json。
这解决了候选恢复的可重复性；仍需独立原始解码器或可信运行数值对照后接入运行时，
不能把字符约束唯一解本身当成原版行为验收。服务器尚未读取该候选。

独立对照现已完成：reference/legacy-source/4dddd9a6/SWorking/Resource/Server/ExpPenalty.bin
为1015字节原版MHFile，type=450、payload=1001，头尾CRC均108且计算值108。
其SHA为f39c4d312788fbb66f7eecf798e33bb82a9e2f5a647b31e300cf2db19d396a19。
按原始CMHFile位置减法解码，明文SHA与当前opaque恢复结果完全相同（不是仅数值相似），
99行逐项一致。audit_exp_penalty.py新增--reference校验源摘要、长度、两处CRC及明文SHA，
实际运行状态为plaintext-verified-against-legacy-resource。此前“待独立对照”的解码缺口
已解决；复活服务端规则、事务和UI仍未实现，不能把表验证当作玩法完成。

C++数据解析已新增于modern/include/mxh/game/exp_penalty.hpp：接收明确配置解码后的
文本，返回等级到float百分比映射；拒绝截断、重复等级、非有限数值、尾随非法字符及
超界百分比，缺失等级保持缺失供调用方执行原版分支。真实当前资源99级测试以及
异常输入测试通过，完整mxh_game_tests为272/272 PASS。容器解码在本轮真实资源测试中
使用已独立核实的变换；生产加载入口和复活事务尚未接入，不能声称服务端已使用此表。

C++容器入口decode_exp_penalty现显式接收PlayDhCurrent或LegacyMhFile配置：当前格式
使用已核验的固定变换，历史格式验证实际长度与头尾CRC，不自动切换配置。
四项ExpPenalty测试PASS，覆盖两份真实资源全部99级逐值一致、配置不匹配、截断、
CRC损坏及文本异常。发布资产仍必须另按清单SHA验证；opaque格式自身无已核实CRC，
结构解析成功不等于完整性验证。MapServer生产装载与复活事务尚未接入。

MapServer装载现已接入：配置resource_root时要求Resource/Server/ExpPenalty.bin，
load_exp_penalty显式选择playdh-current或sworking-2008-reference，校验各自上述固定
SHA-256后解码，失败清空表并由main终止启动。未指定resource_root的既有测试路径不变。
CHINA MapServer构建通过，真实SQLite三服Player死亡回归运行
624c8447f6a1478786c7f0965c888d1a PASS，证明实际资源可通过新增启动校验。
表已保存到MapHandler但尚未用于复活扣值；复活事务与UI仍未完成。

MapHandler装载回归已覆盖当前/历史两份真实文件成功查询、当前文件配历史配置失败、
未知配置失败、路径不可读取失败，并逐次确认失败后不保留上一份表。
完整server_handler测试161/161 PASS。此项不替代对篡改文件的进程级启动失败验证，
也不证明经验、金钱或保护道具事务已经实现。

ExperienceCurve新增reduce_exp，按Player.cpp:1828复制有效范围内的降级算法：
损失恰等于当前经验时不降级；跨级使用前一级阈值减1再扣余量；起始1级损失封底为0。
对会造成等级下溢或超出原版应急循环范围的异常输入抛错，状态按值计算不发生部分提交。
经验曲线21项测试PASS，包括边界和超界拒绝。此函数尚未连接角色属性重算、数据库提交、
恢复标记或网络通知，不能把纯数值测试当作完整复活降级验收。

Player::revive原简化逻辑（半血、零护盾、保留内力）已改为普通复活恢复规则：
生命/护盾分别按max*0.3的double乘法后截断，内力归零，仍仅接受Dead生命周期。
14/14 Player测试通过，覆盖341生命上限→102、103护盾上限→30、内力57→0，
以及重复复活不改变状态。该方法尚无生产调用者；不用于比武/攻城特殊恢复，
调用方仍必须实现并提交惩罚、位置、持久化和生命周期衔接，完整复活尚未交付。

怪物致死生命周期已衔接：此前攻击直接修改vitals为0但actor仍Active；现在同一伤害
结算锁内调用mark_dead_if_zero_life，仅把Active且零生命角色转为Dead，不重复计算伤害。
怪物致死用例断言Dead生命周期，Player用例覆盖存活拒绝、重复通知及Dead→revive。
server_handler161/161、Player15/15 PASS。此项修复怪物致死路径；复活请求事务、
网络通知及其他死亡来源仍须逐项接入验证。

退出审查：Player::begin_logout允许Dead，新增测试确认Dead→LoggingOut不恢复生命、
不能复活或重复退出。materialize_player_position_locked在死亡时停止运动，位置保存
不要求Active。persist_player_exit当前直接调用物品、金钱、任务、位置、商城保存，
没有直接写入生命/护盾/内力；GameIn先initialize并activate角色。必须继续核对角色表
现有字段与原版登录死亡处理，验证重登是否绕过死亡惩罚，不能仅接复活按钮。
本轮Player16/16测试通过，未完成死亡重登持久化验收。

原版死亡退出结算已核实：ServerSystem.cpp:1496在RemovePlayer阶段检查死亡，
普通战斗且IsPenaltyByDie、或攻城时调用ReviveLogInPenelty，其余调用ReviveAfterVimu(false)；
KOR另有行动惩罚分支。之后1524附近调用CharacterHeroInfoUpdate/CharacterTotalInfoUpdate。
因此正确迁移不能只在客户端点击复活后才扣经验，必须覆盖死亡断线/退出。

modern现状：schema_migration.cpp的character_info已有character_data BLOB（MSSQL对应
需继续核验），modern_player_state只有money/level/exp等；load_char_data当前SELECT未取
character_data，CharData生命/内力仍有默认值。退出事务未见生命字段写入，存在重登恢复
丢失的实现风险，尚未以真实重登测试验证。不得直接覆盖未知character_data内容；先检查
既有使用方/存量布局，确定兼容载荷方案，再在同一事务中结算死亡退出和保存恢复状态。
本轮没有修改数据库结构或生产数据，也未宣称这一风险已经修复。

character_data已有MXSH商城记录，现扩展v2在原商城记录末尾增加life/shield/naeryuk三项
u32；v1与NULL仍可读取，未知载荷仍拒绝。save_modern_shop_state默认保留已有生命字段，
显式传入时更新，继续校验角色归属、原始BLOB及乐观并发条件，不改表结构。
SQLite数据库回归8/8 PASS，覆盖新字段往返、商城修改保留生命状态、旧快照写入失败。
尚未接入MapHandler保存/恢复生命、未做MSSQL实际验证；v2写入后的回退客户端须支持v2，
旧版只支持v1，发布回滚必须配套，不能直接切回旧二进制。

MapHandler现于退出既有事务内保存v2生命字段，商城条目为空也保存；入场在装备/商城
上限重算后恢复生命，按新上限截取（处理离线增益到期），零生命转Dead。
既有server_handler161项PASS，新增真实SQLite零生命入场专项PASS，生命0、内力7、
Dead生命周期一致。死亡退出的原版惩罚事务仍未实现，因此此处保留死亡只是防止静默
恢复的中间状态，不是最终原版死亡退出行为；MSSQL、实际重登与复活UI仍待验收。

SQLite处理器完整保存链专项已追加：入场后把内力7改为5、保持零生命，执行真实
on_disconnect保存路径，重新读取DB确认life=0/naeryuk=5，再用新连接GameIn确认
零生命、内力5和Dead生命周期。完整server_handler162/162 PASS。
这是SQLite处理器入场/退出证据，不是独立Player双端流程或MSSQL验收；普通死亡
退出应结算登录点惩罚的最终行为仍未完成。

MSSQL实际探测：当前进程无MXH_MSSQL_E2E配置，本机MSSQLSERVER与SQLEXPRESS服务
Running。专用mxh_test库用Windows集成身份连接localhost时TCP超时；lpc:localhost、
port=0共享内存可到达实例，但ODBC18报证书链不受信任。尚未进入三服Player测试，
未放宽证书验证，不能标记MSSQL通过。需复用可信实例连接配置或修复本机证书信任链，
不应因SQL服务Running就推断数据库验收可用。CHINA MapServer本轮重建成功。

SQLite退出写入失败专项通过：在character_data更新处注入触发器失败，GameOut返回Nack、
保留运行时，数据库金钱和位置回滚至原值且v2生命记录未部分写入；移除注入后重试
GameOut成功，保存life=0/naeryuk=5和新金钱/位置。验证复用了已有晚期退出回滚测试，
本轮该专项1/1 PASS；不代表原版死亡惩罚事务或MSSQL故障注入已完成。

已新增unprotected_revive_loss计算原版普通未保护分支：原地6%金钱、登录点/村庄4%；
原地/登录点经验读等级表并保持float倍率运算，缺级分别默认3%/2%；攻城登录点1%，
村庄使用double 1%。损失向整数截断。原始参考CommonStruct.h定义EXPTYPE为INT64，
超过其范围的输入拒绝。ExpPenalty五项测试PASS，覆盖48级、缺级默认、位置模式与超界。
该计算不决定等级/地图/战斗豁免，也不消耗保护道具；调用方必须先处理这些原版条件。
尚未接入扣款事务，不能宣称角色已被正确扣除复活损失。

plan_revive_protection新增纯计算：有效综合保护优先消耗一次，最后一次标记结束，
不同时消耗独立金钱/经验保护；综合次数有值但使用记录无效时按原版ProtectAll_UseFail
转入独立保护分支。负次数拒绝。七项ExpPenalty测试PASS。返回消耗计划不修改商城或DB，
仅应在适用惩罚的分支调用；真实消耗、结束通知和持久化仍待事务接入。

prepare_revive_shop已将消耗计划映射为商城使用记录副本：综合保护剩余次数更新parameter，
最后一次移除使用记录；独立金钱/经验保护ID55311/55312（原CommonGameDefine.h核实）
按计划移除，其余使用记录和skin保持。拒绝重复物品ID，输入不变。
LegacyShopAppearance九项测试PASS。尚未提交此候选到真实复活事务，也未发布结束通知；
该方法不是独立完成复活的入口。

prepare_ordinary_revive_candidate现把已选普通惩罚分支的损失、保护记录、经验降级、
金钱、上限重算和30%恢复合成副本，在线Player保持不变；拒绝非Dead及当前32位经验
字段无法表示的结果。Player17项测试PASS，覆盖组合结果和原状态不变。
调用方仍须做等级/地图豁免、特殊战斗、位置、恢复标志、商城管理器同步、事务及消息；
候选函数不是生产复活入口，低等级测试只是计算组合夹具，不意味着低等级应扣费。

commit_revive_state新增独立事务提交辅助函数：以旧等级、经验、金钱及原商城BLOB
作并发比较，在同一事务更新进度、角色等级、保护记录及生命状态。提交或回滚结果
不明确时返回Uncertain，调用方必须隔离会话，不能直接重试扣除。
本轮修复字符串bind与std::bind的重载冲突，mxh_db_tests目标构建成功；
LegacyShopAppearance全部10项PASS，新增SQLite触发器注入最后BLOB写入失败，
验证前面等级/经验/金钱更新回滚，并验证成功后旧快照重放被拒绝。
尚未接入生产复活请求；MSSQL事务路径及提交结果不明确的故障注入未验收。

补充提交回应丢失验证：SQLite真实COMMIT成功后，适配器分别返回IoError和抛异常；
两种情况下辅助函数均返回Uncertain。重新读取数据库确认经验、金钱和生命状态已经
落盘，旧快照再次提交被拒绝。LegacyShopAppearance现11项全部PASS，目标构建成功。
此证据仅覆盖SQLite持久化加适配器故障注入；尚不证明MSSQL网络故障或在线会话隔离。

原地复活候选现接入来源明确的策略判断：事件地图先忽略；非死亡、搜身、退出按原顺序
拒绝。特殊地图、战斗频道且对应死亡、低于5级、GFW死亡以及韩服首次行动惩罚分支
不扣损失、不消耗保护记录；普通分支沿用已恢复的惩罚表。上下文等级/死亡状态与Player
不一致时拒绝生成候选。策略输入必须来自服务端，当前尚无生产上下文组装调用。
韩服设置行动惩罚和记录日志、死亡标志清理及宠物副作用仍需在实际入口实施。

保护消耗候选补齐按原Player.cpp顺序的MSG_DWORD通知：综合保护结束时先UseEnd(106)，
再ProtectAll(150)及剩余次数；独立保护分别MoneyProtect(109)/ExpProtect(110)，值为
对应道具ID。modern协议枚举补齐这些已有线上值，原始协议头未改。未使用经验保护
分支记录reduce_pet_friendship，即使经验损失取整为零也需调用宠物处理；豁免分支不调用。
这些仅为事务成功后的待执行结果，未发送网络消息或实际扣减宠物亲密度。

宠物处理计算已补入pet_manager.hpp：apply_master_revive_pet_loss消费已由装备/规则
计算的有符号m_iFriendshipReduceAmount，只修改当前召唤宠物。无召唤或零变化不处理；
事件宠物仅返回主人死亡日志意图。普通宠物返回信息通知，亲密度严格下溢时死亡并返回
5000ms释放延迟意图，恰好零保持存活，与原Pet.cpp:494一致。
mxh_pet_manager_tests构建成功，64项PASS。调用方须对事务副本调用并在提交后分发
通知/日志/延迟释放；当前仍未接入复活生产入口，不能作为在线宠物验收证据。

prepare_present_revive_candidate现在要求传入PetManagerState，并返回同一候选中的角色、
商城记录、宠物状态副本和宠物通知意图；只在未受经验保护的扣损失分支应用宠物变化。
该分支发现召唤ID无对应记录或重复记录时拒绝候选。角色和宠物原状态均不变。
20项Player测试PASS，新增组合验证未保护死亡、经验保护、地图豁免和召唤身份异常。
当前DB复活辅助函数仍仅持久化角色与商城，尚未包含宠物，不能直接用于提交此完整候选。

持久化扩展进展：MXSH v3在现有生命字段后增加宠物记录，使用显式固定宽度字段编码
当前modern PetTotalInfo的全部字段。拒绝重复/零召唤道具ID、非法布尔值、截断及超限
记录；4096是解码容量上限，不是玩法宠物数量规则。v1/v2仍可读取，普通商城保存
自动保留已存宠物。复活事务辅助函数已接受宠物记录与其他状态同次写入BLOB。
SQLite资源往返及逐字节截断测试通过，LegacyShopAppearance共12项PASS。
尚未连接Map入场恢复、退出保存或复活运行时转换；旧二进制不能读v3，回滚须匹配数据。

Map入场和退出现已接入宠物记录：入场从同一事务快照恢复所有已保存的modern宠物字段，
退出在既有事务内把在线宠物记录与角色生命/商城一起保存；无宠物且旧数据不含宠物时
保留v2格式。恢复持有记录不会伪造已召唤的世界实体，召唤实体及规则加载仍待接入。
真实SQLite的GameIn→断线保存→再次GameIn用例验证宠物数量、亲密度、体力与等级保持。
本次handler测试记录在modern/scratch/2026-09-14-map10-entities/pet-persistence-handler-tests.log。

复活事务故障用例扩展为真实v3旧记录：最后BLOB写入被SQLite触发器拒绝时，
角色等级/经验/金钱回滚，旧生命和宠物数据逐字节不变；解除故障后同一候选成功提交，
宠物亲密度/存活状态更新，体力保持，旧快照重放仍拒绝。12项数据库相关测试PASS。
这证明辅助事务的原子性，不代表生产复活入口或MSSQL验收完成。

地图属性来源核验：当前MapKindInfo.bin为原版MHFile，1650字节，SHA256
9af05426844b08bc8c624b04da2413a7ef23ec5025a66af0f7679b0c378f55f8，双CRC验证通过。
新增map_kind.hpp按原LoadMapKindInfo解析地图ID、名称和标志；Map10为MAPVIEW(64)，
不在原地复活特殊地图掩码。保留原RUNNING=129的枚举值。
真实资源含历史源码未定义的INSDUNGEON，相关地图保留为unresolved_maps；运行时必须
使用resolved_flags，不能读取已知位后默认为普通地图。缺失地图同样返回未解析。
当前资源和参考资源哈希/内容不同，未用历史表覆盖。MapKind与ExpPenalty共9项PASS。
尚未接入MapHandler的配置加载及复活入口；INSDUNGEON语义仍需进一步核验。

MapHandler现已加载按profile固定SHA256的MapKindInfo，双CRC/大小检查后提供
resolved_map_kind；失败重载清除旧规则。MapServer资源启动流程要求该表通过校验。
CHINA MapServer与handler目标构建成功，163项handler测试PASS。实际Unity Player+
SQLite三服死亡冒烟通过，runId=d5aa9948d3e94497a6012820431e990f；使用夹具角色，
不是人工验收，也没有验证完整复活。未知地图语义及复活入口仍未完成。

后续验证：两套现有源码按INSDUNGEON/eInsDungeon/eInsDun/eInstanceDungeon检索均无
定义，保持未知语义，不据此猜测标志。MapKind真实资源测试已改为直接调用生产
decode_map_kind，覆盖头字段、两个CRC及内容损坏和所有截断长度，9项相关测试PASS。
KOR/CHINA/JAPAN/HK/TL五个MapServer目标构建成功；该证据是编译验证，不证明五区
运行时玩法一致或复活流程完成。

客户端保护通知桥接：ABI升至1.14，新增SHOP_PROTECTION(39)，reserved0保留109/110/150，
argument0为独立保护ID或综合剩余次数。原生核心校验归属、长度、精确道具ID和0..127
次数，经会话/地图事件队列交给Unity；通知状态显示对应中文反馈并拒绝旧代次/重复事件。
x64原生ctest通过，DLL已stage；Unity Editor ShopItemNoticeStateTests 5项PASS，证据
protection-tests.xml。当前独立Player尚未重建，服务端真实复活尚未触发这些通知。

独立Player现已重建至ABI 1.14，protection-player-build.log记录MXH_PLAYER_BUILD_OK。
该Player运行真实SQLite三服战斗/死亡冒烟通过，runId=631bb5b759b44c198856a4c25c81d6b5。
此运行仍使用夹具角色、无人工验收；证明新二进制的既有链路未回归，不证明真实复活
保护消费通知或完整复活流程已经完成。

保护通知异常验证：真实socket夹具注入其他角色ID、3/5字节载荷、综合次数128、
金钱/经验保护ID互换共六种异常，核心均进入协议失败且不发布消耗事件。
x64目标重建与完整ctest通过；仅测试代码变更，无需重建Player。真实复活入口仍未完成。

在线商城副本同步辅助函数prepare_revive_shop_manager已补齐：核对使用记录数量、图标、
数据库ID及原参数后，在管理器副本移除已消耗记录或更新次数；综合保护结束清除索引。
原管理器保持不变，过期身份快照拒绝。21项Player测试PASS。仍需在真实复活编排中
把此副本与角色、宠物、持久化结果一起发布；本函数不是复活请求入口。

原地复活候选现已强制接收在线ShopItemManager，从其保护索引确定综合道具，返回同步后的
管理器副本。身份/数量/参数不一致时整个候选拒绝，不再由调用方另行补做同步。
21项Player测试PASS，组合验证综合次数与独立经验保护同时更新管理器且原状态不变。
真实请求处理和提交后的状态发布仍待接入。

commit_present_revive已将完整原地候选映射到单次数据库事务，包含角色进度、生命、
商城和全部当前宠物字段，并核对原死亡状态/候选活动状态及角色与账户身份一致。
实际SQLite组合用例从原始死亡Player生成候选后提交，验证金钱100→94、经验100→70、
宠物亲密度/存活变化一起保存，原在线对象不变，旧快照重放拒绝。该新增用例PASS，
handler目标构建成功；尚无网络请求调用此接口，也尚未发布在线状态与通知。

复活位置线上布局已核实：原MOVE_POS去除MSGBASE后为DWORD角色ID和两个WORD坐标，
共8字节；COMPRESSEDPOS舍弃Y并截断X/Z小数。新增共享character_revive.hpp编解码，
校验头/载荷角色身份、精确长度及可表示坐标。x64固定字节夹具与无效坐标/身份/长度
测试PASS。当前尚未接入服务器发送或Unity接收复活成功事件。

生命恢复顺序补充：原Client/GameIn.cpp:1142将MOVE_POS交给ObjectActionManager::Revive，
后者仅结束死亡状态并调用Object::Revive更新位置，Hero额外关闭自动攻击并恢复镜头。
该路径没有计算生命。原Server/Player.cpp:1305 SetLife默认bSendMsg=TRUE，以LifeAck
发送new-minus-old有符号增量；复活代码顺序为SetLife、SetNaeRyuk、SetShield。
因此不得由客户端收到复活位置消息后自行设置30%生命。当前核心已接Life/Shield，
但缺少内力增量处理；modern CharacterProtocol补齐原NaeryukAck=9，原协议头未修改。

内力增量现已接入原生核心：归属/4字节校验后按有符号delta更新MP，拒绝负值或超过
max_mp；ABI 1.15新增PLAYER_MP(40)，Unity常量已同步，DLL已stage。真实socket夹具
验证222内力经-222消息清零并发布结果；修正夹具使用Hero字段而非基础信息偏移。
原生完整ctest通过；独立Player与Unity Editor尚未重新验证1.15，异常MP专项仍待补齐。

ABI 1.15独立Player已重建，mp-player-build.log记录MXH_PLAYER_BUILD_OK。真实SQLite
三服战斗/死亡冒烟通过，runId=cb0501dec6df440fa7a2d692dce92ee1。该测试仍是夹具角色
的已有流程回归，不是人工验收，也未由真实复活触发内力清零。

内力异常socket测试已补齐：错误角色ID、3/5字节载荷、222内力减223、增加112超过
333上限、INT32_MAX增量均进入协议失败，不发布PLAYER_MP。测试目标构建及完整
x64 ctest通过；仅修改夹具，不改变已构建Player。

复活位置已进入原生核心：MOVE_POS校验后更新本地角色位置并发布CHARACTER_REVIVE(41)，
其他角色仅发布同类事件，不改变本地位置；不据此设置生命。ABI升至1.16并同步Unity，
DLL已stage。真实socket死亡→复活位置夹具确认位置303/404更新、生命保持0、死亡输入
仍被拒绝，完整x64 ctest通过。Unity表现消费和独立Player重建仍待执行。

Unity ConnectionPanel已接本地复活位置反馈，未同步生命时明确显示等待生命状态。
当前ServerEntityRegistry主要处理怪物/NPC，缺完整玩家实体表现，未伪用怪物动画作为
玩家复活。ABI 1.16 Player重建成功，真实SQLite三服死亡回归通过，
runId=dd7827b2a89b445ba907a4ad9d44d64c。该运行未触发真实复活，不证明复活动画或完整流程。

Agent客户端入口现已转发CharRevive的0/3/6请求到会话所属地图，拒绝非空payload和
客户端伪造的响应协议；转发时使用会话角色ID。此前该类别在入口直接落入未处理分支。
新增回归覆盖地图路由、身份替换、错误协议、错误长度及无会话连接；当前完整
mxh_server_handler_tests为166项通过，日志为
modern/scratch/2026-09-14-map10-entities/revive-agent-handler-tests.log。
revive_vitality_messages.hpp另外预生成位置和有符号HP/MP/Shield增量，拒绝int32溢出，
不发送消息；位置与生命序列间仍须插入惩罚及宠物通知。Map请求处理、提交后的在线
状态替换和完整通知尚未接入，Agent路由通过不代表实际复活已实现。

复活提交现在先验证位置与生命消息可编码，非法坐标、同地复活候选跨地图及有符号
增量溢出在事务前拒绝。真实SQLite测试检查拒绝后money/exp保持100/100，未写入
生命或宠物blob，再验证正常候选仍能提交。完整处理器166项通过，日志为
modern/scratch/2026-09-14-map10-entities/revive-commit-wire-tests.log。
接入审查确认PlayerRuntime/PlayerState尚缺looted、战场死亡及公会野战死亡状态，
不可用全部false替代真实上下文；这仍是Map请求处理的实现缺口，并非用户授权阻塞。

PlayerState已加入战场、公会野战、特殊地图三种会话死亡标记，初始化为false；
Player::revive仅清除复活副本中的标记，退出失败路径保留，release清除。
依据原版Player.cpp初始化239-241和原地复活2343-2345；战场标记设置见2660-2672，
依赖PK、比武、地图与敌对关系，尚未将该判定接到modern战斗流程。
本次Player测试22项、完整处理器166项通过。该状态容器不代表战场及搜身流程已完成。

复活候选计算现校验上下文的战场/公会野战死亡标记与PlayerState一致，拒绝过期
上下文造成的错误免罚或错误扣损失。测试覆盖两种标记的双向不一致、正确免罚、
候选清除标记且在线死者不变；Player23项及处理器166项通过。

新增resolve_present_revive_context，复用LootingManagerState及已解析MapKindTable，
搜身房间存在时禁止复活；超时严格大于15000ms后关闭房间才解除，未知地图拒绝解析。
prepare_resolved_present_revive将该解析接入候选计算。SQLite组合测试改走此入口，
先确认搜身阻止候选，关闭房间后计算并提交玩家及宠物状态。Player24项、处理器166项
通过，日志modern/scratch/2026-09-14-map10-entities/revive-resolved-commit-tests.log。
仍非Map网络入口，事件地图/频道事实以及提交后在线发布待整合。

击杀经验发布顺序已修正：先计算Player副本、写modern_player_state，成功且会话一致后
才发布在线progress与ExpAck，同步PlayerInfo等级；首次插入保留候选money，不再写0。
公有apply_monster_damage入口加入递归dispatch锁和draining检查。写入失败立即drain，
不发布候选或奖励通知。SQLite触发器故障测试验证在线level/exp/total_exp不前进，
完整处理器167项通过（experience-publication-tests.log）。此前怪物死亡与任务进度已
变化，整次击杀持久化和奖励恢复尚未实现；当前失败处理不等于零奖励丢失验收。

源码复核发现Player::add_experience原有循环升级和99级上限与原版不符。依据原版
Player.cpp:1665 SetPlayerExpPoint单次if转换及MAX_CHARACTER_LEVEL_NUM=121，现改为
每次最多升一级、保留余量、121级不再增加经验。测试覆盖99→100、不同下一等级阈值、
120→121和满级不变；Player25项、处理器167项通过（experience-level-semantics.log）。

经验未改变时Map不写库或发ExpAck，121级真实SQLite夹具通过，处理器168项通过。
重建mxh_map_server_CHINA后运行独立Unity Player的SQLite三服战斗/死亡探针通过，
runId=30d78c5b40bf454886f4c493073b0829。使用真实modern服务端、HSEL客户端连接及
固定测试角色；humanAcceptance=false。验证覆盖技能释放/命中、受击生命和死亡输入，
未覆盖复活、经验奖励恢复、MSSQL或人工画面验收。

频道审查修正：ChannelSystem.cpp:63-126只有KOR分支加载bBattleChannel；非KOR
创建的Channel由构造函数清零。因此不能把KOR频道配置缺失概括为所有地区的阻塞。
EventMapMgr.h:12和IsEventMap明确使用地图58，现由上下文解析直接取玩家地图，
不再接收独立event_map布尔值。MP_REVIVEMsgParser:3836先忽略活动地图；测试确认
地图58在类型表缺失或未解析时仍产生IgnoreEventMap，其他未知地图继续拒绝解析。
Player25项通过；该解析仍待Map网络入口调用。

重要协议纠正：原版Player.cpp:1726-1736及CommonStruct.h:2549的MSG_EXPPOINT使用
当前经验绝对值和ExpKind(0获得/1死亡/2能力使用)，不是经验增量加升级布尔标志。
Map击杀通知现发送持久化后的level_exp和ACQUIRE，修正升级误标DIE的问题。
旧测试期望奖励123改为升级后余量103，并增加后续击杀绝对值校验；处理器168项通过。
Unity核心尚无该消息消费，仍需接入；原版升级另有完整角色信息同步，尚未完整迁移。

Unity核心现消费Category3/13，按绝对经验更新快照revision，拒绝错误归属、长度、
负数及未知ExpKind。真实socket夹具验证500→死亡123，五种异常均保留最后有效321。
完整x64 ctest通过。DLL已stage（SHA256
516BAD684426991E40CB95DEC86EE38C238D0986B4FF888B9CDB266EE6B4C28F），
Unity独立Player构建成功，日志experience-player-build.log。ABI布局保持1.16。
重建Map并运行SQLite三服Player战斗/死亡回归通过，runId=f0c1c1eff64c43778f120c98ec854c45。
此探针未专门断言经验UI或实际复活，不能代替对应验收。

等级同步源码审查（纠正此前上限结论）：Player.cpp:1542 SetLevel在level>=121时
直接返回，因此MAX_CHARACTER_LEVEL_NUM=121不能直接解释为可达到121级。
当前modern允许120→121的测试只证明自身实现，未证明原版一致，该项重新标为未完成。
SetPlayerExpPoint在调用SetLevel前已减阈值，边界行为需连同调用顺序处理，不能仅换常量。
真正客户端等级消息为Player.cpp:1615及1644的MP_CHAR_LEVEL_NOTIFY，MSG_LEVEL携带
Level、CurExpPoint、MaxExpPoint；CharacterHeroInfoUpdate是数据库路径，不应当作
客户端等级消息。下一步应迁移该消息及边界规则，再进行端到端等级验收。

Player经验边界已按上述调用顺序修正：120级越阈值先减去一次阈值，SetLevel拒绝121，
保持120；已载入121则不增加经验。Player25项和处理器168项通过，日志
experience-level-boundary.log。ExperienceCurve::add_exp的独立实现及等级消息迁移仍待统一。

## 2026-09-28：原地复活请求已进入生产入口和 Unity 命令

MapHandler 的 CharRevive 协议 0 已由真实 SQLite 用例
`MapHandlerTest.PresentSpotRequestAppliesPenaltyAndRestoresOnRelogin` 锁定。
活动地图 58 直接忽略。带载荷、登录点协议 3、村庄协议 6、错误连接和零角色 ID
都不改变死亡状态。Map10、5 级、真实 CharacterExpPoint 与 ExpPenalty 下，
未保护分支扣除 6% 金钱，生命恢复为上限的 30% 并截断，内力归零；
经验损失按表计算，本资源在 5 级该列为 0，因此不发送经验包。
成功消息顺序为复活坐标、金钱、生命增量、内力增量。重复请求返回
CharacterReviveNack(76)，原因字节 1。断线重登仍为存活，生命、内力和金钱保持结算结果。

Unity 命令 `MXH_UNITY_COMMAND_PRESENT_REVIVE=19` 只在游戏中且本地生命快照为 0 时
发送空的 CharRevive 协议 0。存活角色返回 WRONG_STATE，非空参数返回 INVALID_ARGUMENT。
`UnityCoreNetwork.ServerDeathAndRevivePositionDoNotInventRestoredLife` 与
`MovementPredictionCorrectionAndStaleGeneration` 通过。已暂存到 Unity Plugins 的
x64 DLL SHA-256 为 `0DA7799DE5317D28131471FB68A258652D55E60A9A673E83E79D532B2110335E`。
开发连接面板在生命为 0 时显示“原地复活”，只提交这一条命令。

登录点、村庄、死亡断线惩罚、宠物通知封包、人物死亡/复活动作和真人点击验收仍未完成。
不得把这次自动闭环写成完整复活或画面验收。

## 2026-09-28：登录点复活

PlayDH `Resource/Server/LoginPoint.bin` 在 24 字节容器头之后用固定 8 字节 XOR 解出英文登录点表。
Map10 只有一个点 `(18555,29614)`，与 2008 MHFile 参考表的 Map10/Map12 坐标一致，且都落在复活坐标的 16 位范围内。
没有登录点的地图直接拒绝，不使用 25000 兜底，也不加原版 ±250 随机偏移。

CharRevive 协议 3 走与原地复活相同的事务。普通地图未保护分支扣 4% 金钱，经验使用惩罚表的登录点百分比（缺级默认 2%），生命和护盾恢复为上限的 30%，内力归零，位置改为该登录点。
活动地图 58、非空载荷、村庄协议 6 和错误连接不改变死亡、金钱或经验。存活角色收到 Nack。断线重登读回生命、内力、金钱、经验和登录点坐标。
`MapHandlerTest.LoginPointRequestAppliesPenaltyAndRestoresOnRelogin` 与既有原地复活用例均通过。

Unity 命令 `MXH_UNITY_COMMAND_LOGIN_REVIVE=20` 只在生命为 0 时发送空的协议 3。存活角色本地拒绝且不发包。
开发面板在死亡时同时提供“原地复活”和“登录点复活”。已暂存 x64 DLL SHA-256 为
`C74DBB79C076630DBF8956670141641A29D31CC4DFACE498623EF1B3D58D8EDB`。
村庄复活、无敌时间和随机散布仍未实现。

## 2026-09-28：死亡断线按登录点惩罚结算

原版 `RemovePlayer` 在普通死亡且需要惩罚时调用 `ReviveLogInPenelty`，先改内存再写库。
现在退出和断线走同一条 `persist_player_exit`。惩罚表、经验曲线和地图类型都已装载时，普通 Map10 的死亡角色按登录点规则扣 4% 金钱和表内经验，生命与护盾恢复为上限的 30%，内力归零，坐标留在死亡位置。
5 级以下、活动地图 58 和惩罚豁免地图只恢复生命，不扣钱。地图类型未知时仍保持死亡，不猜测惩罚。未装载这些表的旧路径继续原样保存零生命。
`MapHandlerTest.DeadDisconnectAppliesLoginPenaltyWithoutMoving` 通过，重登为存活、金钱 96000、坐标仍是 25000。原地和登录点请求用例仍通过。
村庄复活、比武免罚、无敌时间和登录点随机散布仍未实现。
