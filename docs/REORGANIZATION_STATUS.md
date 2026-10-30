# 重整执行状态

2026-10-09 10:55 UTC固定证据快照。优先原版modern DX11；完整计划尚未交付。入口为[ROADMAP](../ROADMAP.md)→[PLAYABLE_STATUS](PLAYABLE_STATUS.md)→[VERIFICATION_MATRIX](VERIFICATION_MATRIX.md)。

## 工程、远端与实际运行

| 分线 | 已验证源码和环境 | 已完成验证 | 当前边界 |
|---|---|---|---|
| 原版 | GitLab/PVE `c26f5760` / CI753；本地vitals候选另行开发 | r17配对部署、协议及非SA DB通过；双客户端互见/移动，重叠窗口138条怪物Move/136位置，同毫秒同实体同坐标重复0 | CharacterDie本轮未捕获；死亡姿态/复活和实时状态持久化未闭合；新vitals纯层10/10不代表运行通过 |
| Unity | GitLab `32fed6ba` / CI755；Player与native `28654f7e`，PVE仍v6b | native112/112；EditMode217/0/13及最终定向5/5；真实GUI UI滚轮拦截、近端角色可见、232实体、攻击距离拒绝与近处两次释放确认 | 目标生命5788→5787→5786；Lv1对Lv57怪各1伤，正常出生/低级路线与数值核对中；无完整击杀/拾取或本轮独立DB读回 |
| Unity网络 | PVE服务器 `84d3a0c5`，v6b包 `69f98655` | 两轮严格2秒228/228；PVE独立handler195/195、3禁用；native110/110纯测试 | 协议通过不替代GUI和业务持久化验收；故障恢复/POSIX待验 |
| Foundation | Unity独立目录，22个原始Git blob保全 | 37通过/2跳过，7组35产物可重生成 | 现有武馆和刀剑枪继续保留；完整权利凭证和重制质量另验 |

GitLab项目12是已验证私有工程主源；三个checkout均设置remote.pushDefault=gitlab。当前再次查询仍4分支，GitHub8分支；原版远端c26f5760、Unity远端32fed6ba，长期分支尚未提升/修剪。Broker专用mTLS及普通推送已恢复，令牌留在服务器。精确观测见branch-consolidation/{gitlab,github}-refs-1050.json。

CI753/755只覆盖治理、Python及精确资源来源；Windows C++、Unity Editor/Player仍是本机证据。两端四个LFS对象分别完整SHA读回，合计1,228,054,889字节；不代表WIP、全部ignored资源和配置已同步。

## PVE与真实GUI

- CT105原件快照保全，仅克隆CT930修复属主并CHECKDB。CT930 `192.168.2.238`与VM112 `192.168.2.244`承载独立测试库及两套三服；原版16001/17001/18001，Unity26101/27101/28101。非SA writer及ODBC18证书校验保持。VM100保留作旧对照。
- **当前原版部署r17**：源码`c26f5760`，manifest`b875d4080c52d6805263455d006a10a3c9f14967069c77577d8ab193a40c76bb`，Map4276/Agent2844/Login4112实际身份/哈希/监听端口通过，唯一任务已退役。配对Agent+Map切换前17文件回退与COPY_ONLY/CHECKSUM/VERIFYONLY备份通过，Unity三服连续；本次未改schema。实际链见`original-integration/r17-monster-route-runtime-final.md`。旧r16b启动失败及r16c恢复/部署证据保留。
- r17协议100003/100002正常GameIn，生命125/125、四属性12、互收CharacterAdd；两个旧NULL角色拒绝。GUI后CT930非SA仍4角色/0物品/2旧NULL，投影与fresh COPY_ONLY备份前一致。当前无实时HP/死态/位置列，不能由该投影证明持久化。
- 原版客户端源码`6c37e5d6`、SHA`b102e509`、Launcher`6c2a3577`未改。r17两窗口互见；HP0主角色地面点击仍250,250，存活观察者250,250→250,256且主窗口见移动。08:03:34–08:07:03 UTC重叠期138条怪物Move头/136位置、重复0，r16同口径曾176重复组。本轮未捕获CharacterDie，死亡动画仍异常，不扩大到全事件/跨地图/自然掉落通过。三个GUI08:07–08:09 UTC正常退出0。证据`gui-original-r17/gui-result.json`。
- Unity PVE仍v6b包`adb2d516`、manifest`1d7e6377`。新Player/原生插件源码`28654f7e`，托管DLL`b7b7058c`/native`e98fbc4c`、ABI10012配对。真实UI滚轮ui-blocked，近端1.60角色保持可见；232实例/0缺失，建角面板隐藏。选目标50099，远处Combo被服务器拒绝out-of-range；正常地面点击26241,24167→25671,25412后两次Combo收到释放确认，生命5788→5787→5786。仅1伤且无击杀/掉落，数值与正常地图路线继续核对；本轮无独立DB读回。08:32:56 UTC正常断线退出0，证据`gui-unity-combat/gui-result.json`。
- 旧helper曾误在PC执行含SQLite的194项handler，不计PVE结果；后续PVE v3为193通过/2失败/3禁用，fixture修复后的v6实际195/195通过、3禁用，业务库未变。数据库自动测试继续限定PVE。QGA仍StopPending，使用正常加密WinRM管理，不为它重启VM。
- 本机七个旧SQL库在PVE独立恢复、VERIFYONLY和CHECKDB通过；旧业务读写尚未验证，本机原数据与共享SQL继续保留。

## 资源与独有成果

原版与Unity的canonical PlayDH均精确验证5131文件、1,681,042,155字节，清单SHA `27ce90226af0b6a97ab2ebf8ec7272392f74bcbb3e75efb7d8f3e605023215a5`。241份派生移入overlay，原资源不回写。Unity复合来源5135/0、Effekseer279项分别验证。45个地图发布缺口（27HFL、18TTB）未闭合；Map10无此缺口，Map44准确44.hfl仍缺失。

NPC27/50来自canonical精确条目，不用其他类型占位。新增21份私有派生输入已在PVE宿主700目录/600文件保存，并逐项SHA验证；ZIP `56bded19e9c83dac9d18b2153feac69a473d3243d845b263145dd6a8fe553d5e`。不加入公开裸资源镜像。已有原创成果→合格免费商用资源→补充原生创作顺序保持，来源与许可见 [ASSET_LICENSE_LEDGER](ASSET_LICENSE_LEDGER.md)。

初始182修改/241删除/1499未跟踪、29refs和指定WIP/资产已私有保全；后续delta-v8的19文件已PVE恢复，ZIP SHA323da7ee26bf57a66eb7d219f2e10d33a66bd75990811f72bd1291df2d61069d，覆盖10:02快照。本次更新继续独立增量保全，不能把旧归档视为当前全部WIP已整合。隔离副本独有源/Prefab/meta、派生资产和配置继续保留。

## 当前新增候选与出生依据

- 两份PVE恢复库的原建角过程均明确写Map17/2017、触发器0，见[出生契约](BIRTH_ROUTE_CONTRACT.md)。此为静态版本依据，Map17实际建角/持久化/GUI尚未通过。Map96准确资源继续单列版本与全地图缺口。
- 原版r18仍在本地开发：死态/复活、技能内力及击杀XP/vitals必须确认事务成功后才发布；未知提交结果隔离。当前Item/Equip/Quest入口及21个旧正例明确暂禁，不能作为完整D验收。正常击杀新发现同门槛循环升级偏差，正在按原源码单次最多一级、分段属性点及升级回满补齐最小状态契约；未部署。
- Unity局部提交32fed6ba按旧引擎只插值scale向量，继续拒绝mesh动画。最新Editor225通过/0失败/13图形跳过，包含六怪模型、材质和12动作采样；之前4通过/3失败的v3记录保留。实际Player仍28654f7e，不能将新Editor证据移用到Player。
- Map17私有资产包275项、27,251,275B和复现代码包4项、14,660B均已PVE真实提取/读回；ZIP SHA分别43c7f257、0fb6f57c。33张准确地形DDS可用，9张有名缺图与13个“1”槽语义待决，未宣称整地图完成。

- Unity 新增私有出生开发场景仅覆盖出生附近4×4地形，完整Editor233通过/0失败/13跳过；35项场景输入已PVE真实恢复。六怪/人物尺度定向7/7通过。此前颜色异常是未激活SRP的诊断截图造成，真实激活URP后的离线静态图形用例1/1通过；旧失败证据保留，没有修改游戏材质。新的14项尺度诊断和16项材质诊断包均PVE逐文件恢复，依赖275/35项私有包。以上不代表Map17实际出生、联网或Player验收。
- 独立出生位置纯策略候选c4d3b723已本地提交，9/9纯测试；按恢复库Map17/2017及旧GetRegenPoint优先级实现，尚未接入两线运行时或推送。
- r18准备使用VM112独立stage-r18-20261009、36001/37001/38001和CT930新库MoxiangOriginalR18Test_20261009，writer/schema v3。复制旧库只作对照，不回填四个旧角色；从正常建角验证新状态。具体执行与回滚门槛见audit/original-integration/r18-isolated-deployment-plan.md；SQL实际契约/构建/部署/GUI待验。
- 原版宽MapHandler测试误含本机SQLite用例，失败及中断已保留，作为执行偏差，不计PVE或通过证据。已停止宽过滤，正在增加PC纯测试完整case白名单；SQLite夹具测试只在PVE执行。
- Windows runner2在线但ref_protected，候选分支未保护；VM162只读盘点未找到MSVC/SDK/CMake/Ninja/Python/Unity。远端C++/Unity CI未就绪，不能用runner在线或Linux CI成功替代。没有放宽runner权限。

## 本机清理

19个唯一项目已盘点；21条checkout记录包含ChatLink与Woai各两种角色。OpenWebUI及textbook-parser经私有恢复验证后退役；Junction单独处理，不跟随重复删除。

| 已删除范围 | 逻辑字节 |
|---|---:|
| 已恢复验证的源码、recovery、reference及重复资源 | 13,293,009,918 |
| 旧bundle、输入传输包与重复本机归档 | 5,688,389,778 |
| 下载缓存、旧日志、测试与状态输出 | 287,791,246 |
| 合计 | **19,269,190,942** |

以上为账本绑定的逻辑字节；不等于磁盘净增空间。最新17,234个scratch重复文件删除2,270,207,276B，8,617个canonical参考及94个排除文件全量SHA仍匹配，全部目录保留。E盘该区间观察增加2,201,022,464B。旧无界清理入口单文件786B也已退役，Git历史和原字节保留。完整证据见pc-inventory/CURRENT_CLEANUP_RESULT.json。

初始WIP/资产两份归档已经在PVE全量物理恢复，17,022文件/3,668,886,974B逐项读回；远端原归档与本机manifest保留。核验后仅删除本机两份重复ZIP共1,597,035,048B。两份过时Unity bundle另删除429,877,410B，最新32fed完整bundle仍保留。恢复路径见audit/baseline/RECOVERY.md；这些均已计入上表，不是额外释放量。

10:50 UTC只读核验桌面仍为Default可交互，屏保关闭，AC/DC息屏和睡眠超时均0；没有添加自动登录或自动解锁。

## 剩余交付

继续原版r18事务与正确经验奖励实现，再在PVE新副本正常建角验证，不给既有4个缺vitals行角色回填。之后按明确Map17出生契约完成首个自然玩法闭环；Map10对照保留。Unity推进独立Map17场景与重制缺图处理。全部地图/功能、Windows/Unity远端CI、恢复/回滚/24小时运行未闭合，暂不提升或修剪长期分支。

证据根`E:\Workspaces\moxiang\audit\2026-10-09-reorg`；私有恢复根PVE `/vmstore/ci-backups/moxiang-reorg/20261009/`。历史必要原文恢复位置见 [HISTORICAL_EVIDENCE_INDEX](HISTORICAL_EVIDENCE_INDEX.md)。
