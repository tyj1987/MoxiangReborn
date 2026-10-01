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
