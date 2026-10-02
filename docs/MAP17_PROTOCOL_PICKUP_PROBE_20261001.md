# Map17 非GUI真实协议拾取候选探针

起点`8a875c1369796a5576390bc7f6eaa4f40de4d281`。仅修改MoxianClientE2E工具及其测试/构建文件；不修改CInGameState、服务实现、源数据、地图素材或配置。

旧combat模式直接`try_attack()`，但客户端已要求公开点击产生pending target；另有“任意掉落/槽数增加/同itemId重登”假阳性风险。本笔以有界Map17模式取代旧combat块：

- `--exercise-combat`只允许`--map-number 17`、自动新建SQLite、自启动loopback三服、timeout1..12；拒绝外部DB、no-spawn、keep-servers、远端地址和混合exercise模式。普通非combat运行不改变。使用新临时目录，先检查16001/17001/18001可绑定，三服显式bind-address127.0.0.1，再确认自建进程仍存活。失败退出清理自建进程；不终止原本占端口的进程。
- 启动前固定核验MonsterList、Monster_17、MonsterDropItemList和17.ttb SHA；掉落表实际为`a65baf6ec5ffae8e3af8ec2c9fd9a4878a8773f69624aea1cafa594fc07e858e`。协作消息曾多写一个d；本值来自仓库字节，不放宽哈希门禁。
- 真实注册/建角/选角/GameIn路径保持；要求Map17及216条初始广播、空携带背包。只在真实广播kind1中选可沿原TTB四邻接到达的目标，复制目标记录避免Process使vector迭代器失效。逐格使用现有send_move普通请求并检查源TTB直线；服务仍决定接受/纠正，不改碰撞、位置DB或怪物。
- 用公开`set_camera_yaw`调整逻辑投影视角，再`project_npc_to_screen`→`OnMouseButton`按下/释放；由原客户端选择目标/检查射程/冷却/发送攻击，不直接设pending target或发攻击包。若点击选中另一怪则失败。无需Unity、HFL渲染或完整Map17材质；这不是视觉验收。
- 要求目标Hit及死亡状态（零生命或命中后移除），并只接受source_monster_id为目标的自然掉落；固定哈希原表的非零项8000/8007/8500、数量1。不给奖，不写装备/血量，不降低怪物；正常新角色若施法/伤害条件不够会完整失败。
- 拾取也走公开投影点击，核pending drop。PickupAck证据来自本次新日志中**真实CInGameState处理Ack后写出的完整drop/item/count行**，不是工具伪造回包，也不是原始网络抓包。再要求权威库存同物品/数量且非零DBID，并用`SQLITE_OPEN_READONLY`核相同角色/DBID/item/count/container/slot。
- 发正常GameOutSyn，等真实GameOutAck日志，再读SQLite；fresh CInGameState重登后匹配同DBID/物品/数量/位置并再次只读核DB。保留日志解析这一工具依赖：客户端日志格式改变会使探针失败，不能删Ack条件来凑通过。

## 云端验证

`map17_evidence_test.cpp`使用实际SQLite库测试匹配行成功，错误owner/DBID/item/数量/槽位、重复记录、缺失文件均拒绝，且只读查询不会创建缺失文件。云端g++编译运行通过。工具新增CTest `Map17ReadOnlyEvidence`。

完整E2E编译仍需Windows：Linux语法尝试在既有`IRenderer.hpp`的`objbase.h`处阻塞，**新Windows分支尚未编译/运行**；不能把只读SQLite测试当作真实击杀通过。治理和diff检查通过。没有GUI或ROG服务运行。

## 合并ROG验证清单（无需GUI）

1. 获取本笔完整SHA，在既有Windows构建环境构建`mxh_client_e2e`及`mxh_map17_evidence_test`；执行`ctest -R '^Map17ReadOnlyEvidence$' --output-on-failure`。
2. 确認三个隔离端口空闲，运行`mxh_client_e2e.exe --exercise-combat --map-number 17 --backend sqlite --timeout 12 --use-hsel`，必要时显式提供三个exe路径；不要提供DB路径、no-spawn或发奖。逻辑投影点击沿原客户端执行，毋需Unity Play。
3. 标准输出给出新`moxian-map17-<pid>-<stamp>`证据目录。收集`result.json`、`client.log`、`login.log`、`agent.log`、`map.log`及该目录SQLite：结果初始为失败/started，所有正常失败返回更新exitCode；异常中止仍留下初始记录和日志。目录创建本身失败只能保留启动器stdout/stderr，不宣称已落文件。combat及重登共用timeout×15（最大180秒）截止；此前启动/登录各有现有阶段超时。外部监督总预算建议360秒，卡住时保留日志及自建PID，不反复自动重跑。
4. success必须同时有source hashes、真实kind1 ID、kill/source-matched drop、匹配PickupAck日志、同DBID数量、GameOutAck和重登SQLite记录。失败先据具体阶段诊断；不改规则或放宽测试。`visualAcceptance`永远false，Map17缺9DDS/test4.tif仍独立阻塞。

后续Unity高度/冷导入和Effekseer生命周期验证应在GUI所有者释放后合并安排，不与此非GUI进程测试混为同一门禁。

2026-10-02修订：死亡必须有权威LifeNotify产生的Death事件，目标消失不能证明死亡。子服日志采用独立文件和受限继承句柄；Windows采集验收尚待复跑。HP修复及回归边界见`MONSTER_LIFE_AUTHORITY_FIX_20261002.md`。
