# 独立 Player 击杀—拾取—重登探针

**当前阻碍更新：** ROG实测裸等级48角色打50023每次仅1伤害并死亡。云端已核实当前默认物攻与目标防御不匹配；现增加建库前失败保护，`--pickup-loop`输出`pickup-fixture-audit.json`并停在`combat-fixture-preflight`，直到标准战斗属性初始化与新profile完成审查。详见`docs/UNITY_PICKUP_FIXTURE_AUDIT_20261001.md`。以下是既有执行流程，不代表现在能通过。

本模式是自动组件/协议/持久化验证，不是真人点击或整体游戏验收。此前ROG报告Map10/228怪物/6 kind/动画/首击Hit通过；首击探针不会证明击杀、掉落或拾取。本模式不替换`--combat-timeline`。

## 执行范围

同步本批提交并重新构建Development Player后，使用现有三服工具：

```text
python modern/tools/unity_three_server_smoke.py --player <DevelopmentPlayer.exe> --pickup-loop
```

仅允许独立SQLite模式，禁止混用Editor、MSSQL及其他功能探针。沿用工具的新目录、新数据库和随机生成账号；三服绑定与客户端连接均固定loopback，无外部目标参数。初始角色111、Map10、等级48、位置44178/13253沿用现有combat fixture，初始库存必须为空。未修改掉率、生命、伤害、奖励、技能、服务端校验或资源。开始后不再写数据库，仅使用只读SQLite连接取证。

流程：

1. 第一个Player登录并等待完整库存和地图就绪。通过现有`UseSkill`入口每0.8秒至多提交一次技能1给实际场景目标50023；收到目标伤害和生命0通知后等待掉落。没有自然掉落就失败，不注入掉落，也不提高掉率。
2. 寻找场景中的真实`GroundDropPickup`，必要时使用正常移动命令接近；调用`RequestPickup`，要求匹配的服务端Ack和其后的完整库存快照，且恰好增加一个DBID、物品和数量匹配。
3. 调用连接面板`disconnect.onClick.Invoke`，等待原生Disconnected事件及Idle，再正常退出Player。工具核验SQLite所有物品字段与客户端库存一致，才终止/重启自己创建的三服。重启复用同一数据库，不改角色或物品。
4. 第二个Player重登，要求库存与上一会话完全一致，再执行击杀/拾取。必须保留旧物品全部字段，新增不同的持久DBID；再次正常断开并核验SQLite。
5. 再重启三服，第三个Player只重登检查，不战斗。两件物品与SQLite必须完全一致且DBID唯一，最后正常断开。只有三阶段均通过才汇总成功。

重启服务可覆盖物品ID计数器从持久库存重新初始化；报告保留每次ground drop ID与DBID，二者不是同一身份。当前托管掉落事件未暴露source monster ID，故关联证据限于隔离单账号、只攻击50023、目标生命0后出现掉落；不声称收到精确的掉落来源字段。

## 超时和证据门槛

- 建库/注册各30秒；每个服务监听20秒。
- Player阶段：登录30秒、战斗120秒、等待掉落/接近30秒、Ack及库存15秒、正常断开10秒；整体210秒上限，外层进程240秒硬超时。
- 每次退出后SQLite核验20秒。失败即停止后续阶段，保留日志及失败summary；无自动成功、无限循环、自动复活或第二种奖励路径。
- 受控重启仅终止本次Popen对象，10秒未退才kill并再等5秒；服务意外退出不当作正常重启。

输出在`modern/out/unity-remaster/three-server/<runId>/`：

- `pickup-first/second/verify/pickup-report.json`：阶段、错误、技能提交次数/最近返回值、权威库存前后快照、掉落/物品/DBID、断开结果。
- 各阶段`pickup-events.jsonl`：筛选的原生事件类型、sequence、session/map generation和数值参数。无密码、账号凭据或聊天文本。
- 各阶段`persisted-inventory.json`：只读SQLite快照，包含DBID、物品、槽位、durability、rare、quick position、item parameter。
- 各阶段`player.log`、三服及重启日志、`pickup-loop-summary.json`、总`three-server-summary.json`。

输出目录创建后，顶层失败边界也覆盖Player路径校验、数据库migration、账号registration、夹具准备及三服首次启动。若尚未进入Player阶段即失败，仍写`failure-summary.json`、`three-server-summary.json`和失败的`pickup-loop-summary.json`，返回码为1。JSON记录failureStage、server、errorType、timeoutSeconds和returnCode，不复制异常内的命令、stdout/stderr或env。已有进程先经原有finally清理再报告；仅操作本次Popen对象。报告写盘只尝试一次；若磁盘不可写，stdout仍输出含reportWriteError的机器可读失败结果。

Python独立校验器不只读取布尔结果：检查原生击中/生命0/掉落/Ack/完整库存事件及顺序和代次，核对每次重登库存、旧物品不变、新DBID唯一以及实际SQLite记录。致死一击的伤害通知可能晚于生命0通知，故不强制二者的错误先后假设；生命0必须早于掉落、掉落早于Ack、完整库存晚于Ack。

交互分级：技能/移动属于协议命令调用，拾取属于组件方法自动调用，断开属于UI按钮事件自动调用；均不是OS鼠标输入。`humanAcceptance=false`、`mouseInteraction=false`始终保留。该模式不验证标签可读性、遮挡、真人点击、伤害动画、截图质量或全部场景性能。

## 云端验证和待验项

Python测试28/28通过（其中本批10项），覆盖缺事件、错误代次/顺序、旧报告、ID重复/旧物品丢失、只读SQLite、进程超时失败落盘、三阶段编排及互斥CLI约束。Python编译、项目治理、diff检查通过。

云端无Windows三服可执行文件和Unity Editor/Player，未执行本批C#编译或真实端到端验收。ROG先完成132950b的冷导入复测，再同步本批提交构建并运行新模式。任何超时应按具体阶段日志记录缺口；测试代码通过不能替代真实击杀/掉落/拾取/重登结果。

失败报告增量新增3项回归：migration 30秒超时、registration非零退出、首次MapServer监听20秒超时。检查三份失败JSON和stdout、禁止敏感子进程信息入报告，并验证监听超时仅清理拥有的进程。云端Python Unity工具测试现31/31通过；本次没有改变真实成功门槛。

## Review范围

独立只读review覆盖`99b3a72f6fe5093c07495a311d007f6e5a51ea75`至`af66675cc2e44e0e95ff82b55cd9b8a90f7097f7`的9个提交、44个文件，本次修复其失败产物P2问题。协作端报告PR1相对main仍包含1752个提交、7143个文件且mergeable=false；该整PR状态未由本云端重新核验。局部review不能视为整PR通过。本次未修改PR base、未合并，也未处理整PR冲突。
