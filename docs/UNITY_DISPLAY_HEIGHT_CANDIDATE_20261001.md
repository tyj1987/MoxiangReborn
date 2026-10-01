# 实体显示高度候选修复（未通过ROG GUI验收）

起点`9277c68966ee422e095608909ba3a56f7edc0601`。Map10原实体/掉落创建代码把Y固定为0；真实height sidecar SHA为`2177825dd2057f4bbebfe8d7a520a4caa5847d9e6e2d4bc5ecc1b62c70c023f7`。六类刷怪样本处地面scene Y为4.36–6.56，故实例计数不能证明在地面上可见。本轮不加人物装配、树木、相机跟随或移动插值，不动模型scale、服务器XZ/碰撞/数值。

## 最小改动

- 新增`MapDisplayHeight.TrySample`：按可见`TerrainTileMesh`的00→11对角线做分片三角形插值；不是双线性，也不是inspectionMesh的另一条对角线。数据按z行/x列，game坐标输入，game高度输出，再由既有`MapCoordinates`统一居中/乘0.001。含最外侧顶点，拒绝越界/非有限输入、网格/尺寸不一致及采样格非有限高度。
- `MapVisualController`保存当前高度场，并校验会话/地图代次；清理和缺失目标地图使查询未就绪。非identity世界地图变换被拒绝，避免在既有坐标约定外悄悄改变XZ。Map44只查询它已有的专用导航Collider，不查询其他实体或创建替代平面；miss/disabled拒绝。
- `ServerEntityRegistry`在创建怪物、NPC及掉落前查询显示高度；失败时保留明确`PresentationFailure`并记录错误，不在Y0创建。旧代次事件不使用新地形；已有等待地图就绪的事件队列保留。物品ID/数量、对象ID、XZ及协议不变。
- **inspectionMesh与可见地形的对角线不同属于既有事实。** 本笔不改点击碰撞网格或六项冷导入测试。高度贴合可见三角形；服务TTB仍唯一权威阻挡来源。

## 测试与执行边界

先补高度用例，再实现查询和接线。新增`MapDisplayHeightTests`六项：非共面格两半/对角线插值（能区分双线性及错误对角线）、矩形网格坐标轴、真实Map10非零高度、会话/地图切换/原点变换、未就绪拒绝实体创建、专用场景Collider命中/失败。原registry测试增加实体/掉落Y=5断言，原固定Y实现会违反该断言；掉落还验证转换后的XZ保留。旧协议夹具显式安装原创合成地面，未加生产fallback。Map44身份/命中测试显式使用零高合成面保留其原始目的；实际Map44仍需ROG回归。

云端没有Unity Editor或C#编译器，因此**未实际执行新增EditMode，也没有红→绿运行证据或GUI通过结论**。可运行检查：`python3 -m unittest discover -s modern/tests/unit/tools -p 'test_unity*.py'` **42/42通过**；`python3 scripts/check-project-governance.py`、`git diff --check`通过。这些不验证Unity物理/编译行为。原`TerrainImportDependencyTests`六项及导入器未修改，不把“文件没改”当作回归通过。

## ROG下一次可用时段

1. 先编译并运行`MapDisplayHeightTests`、`ServerEntityRegistryTests`、`MapVisualControllerTests`、`WuguanProductionTests`，再跑原六项`TerrainImportDependencyTests`及Map10导入测试；资源树未配齐的失败需单列，禁止替换成占位。
2. 使用真实Map10广播实例记录gameXZ、root Y、Renderer bounds、地形高度、scale及材质；截图确认贴地后模型是否可辨。截图对照保持原相机，不同时放大模型或改材质。检查地面掉落的标签/选择/拾取仍可用，服务端XZ与库存行为不变。
3. 验证换图/断开后旧实例和旧高度清除；Map2来源未就绪要明确失败；Map44专用导航范围内命中、范围外不生成假地面。
4. GUI黑屏/安全窗口未解除时不重复Play。即使以上代码测试全过，本地人物仍缺装配、树木仍缺静态物件链；本笔不升级视觉/可玩性门禁。

本轮云端日志`/workspace/moxiang-audit-20261001/display-height-python.log`属于执行环境产物，不是所有克隆可访问的仓库附件；ROG结果应另附完整SHA、XML和截图路径。
