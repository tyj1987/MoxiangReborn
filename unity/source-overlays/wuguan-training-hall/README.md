# 武馆训练厅：生产管线与正式地图接入

本资产属于现有 Unity 客户端。唯一仓库为 `E:\Workspaces\moxiang\repo`，`C:\moxiang` 只是兼容目录联接；没有另建Unity项目或独立Demo。概念图不能作为游戏资产或商业美术验收证据。

## 原始资产与高低模

保留原始 `WuguanTrainingHall.blend`、`build.py` 和原始FBX，不覆盖它们。原始Blend SHA256为 `a01133434249b51e914381b2e9f03c56e28fb6a160a39b1c97354255b21ae307`。

已有 `bake_pbr.py` 把原材质转为4096平方的BaseColor、Normal、MetallicSmoothness图集。金属度和光滑度独立编码，不以Alpha透明度反预乘RGB。`pbr-manifest.json` 记录全部源文件和输出哈希。

新增 `refine_high_low.py` 读取经验证的PBR派生Blend，建立66组LOW/HIGH/CAGE关系。21组近景对象（木人躯干/手臂、梁柱、铜炉碗体、匾额）进行真实网格细分及几何旧化/凹痕处理；低模共13308个Blender三角面，高模133440个。使用明确的0.04m包络、Cycles selected-to-active投射生成法线及AO，而不是仅增加程序噪声节点。

产物为本目录 `WuguanTrainingHall_HighLow.blend`，以及引擎Art目录下 `Textures/Hall_HighLow_Normal.png`、`Textures/Hall_AO.png`、`highlow-manifest.json`。高低模、笼子、贴图均可继续编辑。Unity材质实际引用新的法线和AO；AO强度0.8。该轮是近景几何细节首轮，不能等同于最终商业雕刻验收。

## 光照烘焙

`WuguanLightingBake.Bake` 在现有工程生成 `Assets/Moxiang/Scenes/Map44_Wuguan.unity`。使用Progressive CPU、24 texels/m、2048图集上限、64直射/256间接/64环境采样与2至4次反弹；静态58个渲染器写入光照图，8个木人活动部件使用探针。45个探针和1组实际光照图及LightingData已保存。

场景地板/切磋垫可见表面与原版FIXHEIGHT=0导航平面一致，避免角色脚底穿入美术垫面。外层运行时根节点保持单位变换，FBX轴转换隔离在Visual子节点。

`WuguanBakedPreview.Capture` 从持久化的正式Map44场景读取真实光照贴图，输出入口、侧视和木人近景三张1920×1080图及相机/光照图证据。原有 `unity-preview.png` 仍只是Prefab检查图，不得混称为场景光照烘焙结果。

## 服务器战斗事件

`ServerEntityRegistry` 现有原生回调中的 `SkillHit` 已连接 `WuguanServerHitReceiver`。只有经过服务端MonsterAdded赋予的对象ID、会话代次、地图代次和递增事件序号才会接受命中。血量仍由LifeNotify决定，不在表现层扣血。

Map44服务器实际生成的怪物可以使用 `ServerTrainingDummy.prefab` 木人表现，保留原对象ID、visualKind、出生位置、名字、生命和伤害；其他地图仍使用原有审计模型。静态馆内4个木人装饰不被凭空赋予服务器ID。命中表现只由确认事件触发，帧更新恢复原姿态。

SingleResult并未提供攻击者、技能ID、命中点和方向，因此不伪造这些字段，当前采用无方向的短促压缩反馈；音频和粒子接口不代表对应成品效果已经完成。原生TCP协议夹具测试与Unity消息分发/表现测试分别通过，但这不是生产三服的完整修炼战斗联机验收。

## 正式Map44接入及限制

原始 `reference/legacy-source/4dddd9a6/[CC]Suryun/SuryunDefine.h` 明确SURYUNMAP=44、入口游戏坐标(10650,12500)。原Map44配置是固定高度0和静态模型，不是HFL地形。

`Assets/Moxiang/Derived/Map44/Map44.binding.json` 记录上述定义、TileManager的50单位格尺寸、FixedTile的碰撞位、Map44头及44.ttb五份来源哈希。`44.ttb.bytes` 是原TTB逐字节复制。点击导航面严格对应3617个原始可走格，未扩张服务器碰撞；入口连通分量为3494格，包围范围约3.2m×3.2m（不是整块可走矩形）。这与美术冻结的7m×7m切磋区不一致，目前不能宣称整块美术切磋区均可用于正式战斗。

`ConnectionValidation.unity` 的原有MapVisualController现已保留Map10/Map2并登记Map44。标准 `MapVisualSetup.Apply` 也保留已审查的Map44配置。Map44场景加入原Windows客户端构建目录，运行时按服务器地图状态增量加载；绑定成功后才启用输入；退出与代次取消只卸载本模块拥有的场景。烘焙数据在加载/卸载测试中实际使用。回归中发现旧Map10/Map2导入缓存失效，重新导入通过原始哈希检查的依赖后恢复了原GUID和子资源ID，没有删除旧地图或压制失败测试。

Map44属于修炼专用流程，没有凭空新增普通MapChange传送门。当前NativeClient尚缺完整Suryun进出请求/确认链路；实际三服、账户、任务进度和客户端进出战斗仍待联机验收。地图注册/运行加载测试与真实联网传送不是同一个结论。

## 验证与复现

本轮默认验证覆盖34项Unity测试（含原Map10/Map2、服务器实体回归、真PlayMode地图加载/取消、高低模/AO和光照图）。另执行了原生 `UnityCoreNetwork.ServerSkillTimelineEmitsReleaseThenAuthoritativeHit` TCP夹具测试1/1通过。不得把这些测试描述成生产账号完整联网操作。

```powershell
powershell -NoProfile -NonInteractive -File scripts\verify-wuguan-training-hall.ps1 -BakeLighting
```

可使用 `python scripts/refine-wuguan-highlow.py` 单独重建高低模/AO；`-BakeHighLow` 会在验证前调用它；`-BakePbr` 会依次重建PBR及其依赖的高低模。耗时烘焙应给足独立任务超时，不在运行时删除进程或伪造产物时间。

输出在 `modern/out/unity-remaster/wuguan-training-hall/`：highlow日志、lighting-bake.json、map44-binding-result.txt、map44-baked-*.png/json、editmode-results.xml、native-authoritative-hit.xml、各阶段日志与本轮生产摘要。修改前备份在 `modern/scratch/2026-09-23-wuguan-production/baseline/`。

Unity基底审计仍是66 Renderer、12060三角面、1材质、26 Collider（9 Trigger）、4灯和10类型化点。新增lightmap UV使导入顶点增至23704；Blender评估后三角面13308另行记录。这个静态基底计数不包含未来服务器实际刷新出来的怪物实例，也不是FPS或实际Draw Call测量。

## 尚未达到的验收等级

这轮不是最终商业美术或发布验收：核心形体、材质叙事及高分辨率逐物件精修仍需迭代；音效/完整命中特效、角色动画衔接、7m切磋范围与原通行数据冲突、真实Suryun三服进出战斗仍未闭环。没有修改原始游戏资源、服务器协议和玩法数值，没有提交或推送GitHub/GitLab。Windows开发构建的实际状态以生产摘要和构建日志为准，不能等同于联网验收。

## 本轮最终记录（2026-09-23）

最后一次包含重烘焙光照的完整验证于05:00:18 UTC结束，34/34通过，进程退出0。原生服务器SingleResult TCP夹具1/1通过。

沿用既有 `RemasterSetup.BuildDevelopment`，Windows开发客户端于05:08:14 UTC构建成功，退出0。实际包位于 `modern/out/unity-remaster/player/MoxiangClient.exe` 及其同目录依赖；序列化包目录中已核验ConnectionValidation和Map44_Wuguan两场景。构建成功不表示启动或真实联机已验收。本轮没有启动生产账号或修改在线服务器。

完整来源、贴图、光照、真实截图、测试结果与Windows包关键文件哈希记录在 `modern/out/unity-remaster/wuguan-training-hall/validation-production-20260923.json`。该记录明确保留本页所列商业美术、通行范围和三服修炼流程的未完成项。
