# Unity 拾取链修复（2026-10-01）

基线 `99b3a72f6fe5093c07495a311d007f6e5a51ea75`，分支 `codex/fix-pickup-loop-20261001`。

## 行为变更

- 服务器成功拾取后保留原 PickupAck，追加现有 TotalInfoLocal 权威库存快照；同步在线玩家库存镜像。Unity 不根据回包里的 item/count 猜测背包槽位或 DBID。
- 原生核心严格解析8字节 PickupAck/Nack，以现有事件18的 OK/REJECTED 区分结果。拒绝不会进入连接错误状态；错误长度、空掉落ID或不一致字段仍失败。没有更改旧协议头、ABI大小或玩法数值。
- Unity 拒绝时立即清除 pending，保留地面物并显示距离/背包/可领取状态提示；成功时移除地面物，库存由后续 EventInventory 更新。无回包仍保留5秒超时重试。
- ConnectionValidation 场景和场景构建器绑定独立 GroundDropLabel prefab，包含 Collider、拾取组件和使用现有 TMP 字体的可点击 uGUI 标签。标签展示服务器物品 ID 和数量，是功能界面，不是最终物品美术验收。不改 NPC/B16、武器/武馆 GLB 或 DX11 客户端。
- 修复直接依赖 `drop_item.hpp` 与 `battle_factory.hpp` 中裸 CR 将注释拆成代码的编译问题；仅修两行注释，不改掉落或战斗算法。

## 测试证据与限制

本批 Linux 实测：可移植 C++ PickupWire **5/5 PASS**；现有 Unity Python 工具 **25/25 PASS**；完整Python工具测试 **42/44 PASS，2 ERROR**（缺Windows MapServer/DbTool二进制；25项是42项的子集）；项目治理检查及 `git diff --check` PASS。Prefab六个YAML对象、内部fileID、脚本/字体GUID和场景引用静态检查通过，不代替Unity导入。

新增/扩展但尚未在 Windows 执行：

- MapHandler 成功后快照DBID/物品/数量、重复拾取不重复更新；500/501距离边界与80格背包满拒绝；SQLite正常断开后新Handler重登保持同一库存。
- HSEL loopback socket：拒绝后再次成功，事件携带同一掉落及会话身份，连接保持 InGame。
- Unity EditMode：pending清除/超时/重复提交抑制、prefab与场景引用、错误掉落ID不解除等待、拒绝保留/成功移除。

主工程在此 Linux 环境仍受已有Windows API及大小写路径限制，未运行完整C++、Unity EditMode/Player或ROG验收。额外 GCC语法探测受已有 LoginRateLimiter默认参数兼容性、localtime_s 等阻断，不作为服务器编译通过。原生核心仍依赖Windows SDK/WinSock。当前没有随仓库提供的 `.github` / GitLab CI工作流，本批未配置付费runner或密钥。

复现可移植测试：

```sh
cmake -S modern/unity-core/tests/portable -B modern/out/pickup-wire-tests
cmake --build modern/out/pickup-wire-tests
ctest --test-dir modern/out/pickup-wire-tests --output-on-failure
python -m unittest discover -s modern/tests/unit/tools -p 'test_*unity*.py'
python scripts/check-project-governance.py
```

## Windows/ROG继续验证

1. 检出本分支对应提交，先构建三服/DbTool并运行既有服务端测试；定向 `mxh_server_handler_tests --gtest_filter=MapHandlerTest.*Pickup*`，随后完整相关回归。可执行路径使用当前CMake配置的实际输出（可能在Debug子目录）。
2. `scripts/build-unity-core.cmd` 构建并运行 x64核心测试，`scripts/stage-unity-core.ps1` 暂存；Unity6000.6.0f1 执行 EditMode（重点 GroundDropPickupTests、ServerEntityRegistryTests），再用现有 RemasterSetup.BuildDevelopment 构建Player。不要把本页未执行测试记作通过。
3. 先跑现有隔离三服 `unity_three_server_smoke.py --player <Player.exe> --combat-timeline` 验证战斗回归，再真人使用测试账号：登录→选角→移动至怪物→击杀→看到物品ID/数量标签→点击拾取→背包出现权威物品；记录截图、日志、DBID/槽位/数量。
4. 过远/背包满时点击：提示拒绝，标签保留且可再次点击；重复点击等待期间不得重复发包。靠近/腾出空间后重试成功。
5. 正常退出，重登同角色：对照DBID/物品/数量只存在一份。复用隔离测试数据，不使用生产账号或原工作数据库。

正常退出存档沿用既有持久化路径；本次没有承诺拾取瞬间崩溃持久化、跨进程事务或其他观察者地面物即时消失。标签真实可读性、遮挡/高度和点击手感仍需ROG验收。不得把此修复称为整条游戏或正式发布通过。

## 自检后增量：持久库存ID

拾取不再把短期地面对象ID作为库存DBID，而是复用Handler已有、从持久化最大DBID及载入物品推进的`next_item_db_idx_`。原子预留不回绕；耗尽时拒绝领取，失败插入允许留下ID空档。正常退出的事务存档机制不变。该计数器是Handler成员，不是跨进程全局序列，本次没有引入或声称跨进程全局唯一性保证。

SQLite回归现包含旧DBID90000→拾取新物品→退出→新Handler重登→再次产生地面90000→继续拾取→再次退出重登；旧物品、新物品的DBID和数量分别保留。并发回归在同一Handler提交12种掉落、每种两次，要求12成功/12拒绝/12个不同库存ID。另有可移植原子分配回归：8线程共1600次预留无重复，以及种子/耗尽边界。本增量Linux可移植测试8/8通过；新增完整Handler测试仍待Windows执行，未涉及3项尚待定位的历史资源fixture。

## 自检后增量：掉落标签可读性

世界空间Canvas改用相机深度、FOV和pixelHeight计算缩放，目标保持160×36屏幕像素、字体名义17像素。正交相机使用视野高度计算；相机缺失、无效投影或锚点在近远裁剪面外时隐藏且禁用点击。缩放仅作用于标签，不改变服务器坐标、距离判定或物品Collider。

标签保持世界空间深度关系，并检测到锚点前方的非自身Collider时同时关闭绘制/点击；排除掉落自己的Collider。uGUI按钮拥有标签点击，物理OnMouseDown不再穿过前景UI重复触发。过远仍走服务器拒绝反馈，不自动走路或改变拾取距离。

独立静态投影计算覆盖800×600（深度5、38.08341、71.98346）、1024×768（150）、1920×1080（5）、2560×1440（72）、3840×2160（299）共7组，均得到160×36标签及17名义字高。此计算不是Unity C#执行或截图验收。

新增GroundDropLabelTests含7组透视、2组正交、无效参数与自身/外部Collider遮挡，共11个EditMode用例，尚待Unity运行。本批已有Python工具25/25、治理、diff及prefab静态引用检查通过。ROG请在默认800×600与高分辨率分别查看近/远掉落，确认文字可读、被地图/实体遮挡时不可点击、返回可见后可拾取；检查前景UI不触发背后的掉落。大量掉落时射线检测开销尚未实测，不作为性能验收。
