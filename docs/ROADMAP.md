# Moxiang 开发路线图

本路线图绑定当前分支提交和可复现证据。原版候选最近已验证代码基线为 `5b7d78be`，Unity 候选当前为 `3d68eb38`；`main` 仍不是现代客户端基线，`codex/unity-client-remaster` 保持独立。服务端与数据库只在 PVE 验证，客户端在本机验证。

## 当前优先级

1. 保持原始出生路线契约：特定恢复库的 `MP_CHARACTER_CreateCharacter` 静态契约写入 Map17/LoginPoint2017；准确 `17.hfl` 已恢复，九个使用纹理仍缺失，不能用 Unity 占位资源代替。
2. 在隔离 PVE 数据库继续完成真实新账号 Map17 移动、任务、战斗、自然掉落、点击拾取、装备、切图与重登持久化；登录、建角、选角和首次 GameIn 已通过。
3. 保留 Map10 作为高等级战斗/掉落对照门禁；Map10 不是已证明的 1 级出生地图。
4. 补齐原版剩余 HFL/TTB/纹理缺口，并为每个包记录来源、哈希和许可；缺失时维持阻塞。
5. 原版闭环稳定后，再在 Unity 独立工作树推进 Map17 重制场景、人物动作、可辨识度、掉落拾取和 Editor/Player 验收。

## 证据门禁

- 工程：原版与 Unity 可从干净工作树独立构建；相关 C++、Unity EditMode/Player 测试通过。
- 原版玩法：真实 GUI 输入完成登录→建角→出生地图→移动/NPC/任务→战斗→掉落→拾取→装备→切图→重登，并与 PVE 数据库读回一致。
- 资源：原版资源逐字节/解析/视觉音频分层核验；没有准确来源的 HFL/TTB/纹理不得以替代品宣称 1:1。
- 稳定性：断线重连、服务重启、数据库失败恢复、更新回滚和长时间运行均保留失败证据。
- 清理：每个删除对象有唯一去向、恢复证据和绝对路径记录；清理后重新构建和运行两线。

## 当前证据

- `docs/evidence/original-pve-fresh-starter-combat-20261015.json`：全新账号建角和首发装备写入通过；Map10 1级首战仅护盾伤害，未完成掉落/拾取。
- `docs/evidence/original-birth-route-static-20261015.json`：恢复库 Map17/2017 静态契约。
- `docs/evidence/original-pve-map17-e2e-20261015.json`：VM112 隔离原版端口真实完成 Map17 登录、空角色列表、建角、重列、选角和 GameIn，探针数据已清理。
- `docs/evidence/original-resource-admission-20261015.json`：当前原版在恢复 Map17 `17.hfl` 后仍有 22 个 HFL、18 个 TTB 和 Map203 的 161 个纹理依赖阻塞；Map17 另有 9 个已使用纹理待恢复。
- `docs/evidence/original-map-pak-resource-audit-20261015.json`：四个可解析 Map.pak 候选共检查 18 个缺失 HFL/TTB 名称，均无准确条目；MoyoI 包中的原始 HFL/TTB 不关闭当前文件级缺口。
- `docs/evidence/original-client-endpoint-regression-20261015.json`：MoxianClientE2E 默认指向 VM112 `192.168.2.244`，本机 loopback 只能通过显式参数或本地 CMake/脚本入口选择。
- `docs/evidence/original-map17-route-wrapper-20261015.json`：VM112 当前启动包装器已提供显式 `-EnableMap17 -Map17Port 18004` profile，可追加同源 Map17 服务而不接管默认五服；尚未把它误报为视觉资源完成。

## 交付顺序

A 保全与远端恢复 → B 双线干净构建 → C PVE 独立服务/数据库 → D 原版 Map17 首个可玩闭环 → E 原版功能和资源覆盖 → F Unity 重制闭环 → G 双线恢复、稳定性和最终清理。

未经对应证据收口，不提升完成声明、不删除唯一副本、不把历史通过记录当作当前提交的验收。

### Map17 原始资源进展

Map17 的准确原始 `17.hfl` 已从当前 `Map.pak` 恢复并纳入原版 canonical 资源；隔离 PVE 已通过新账号登录、建角、选角和首次 GameIn。剩余门禁为 9 个使用中的地形纹理，以及移动、玩法和持久化验收。

本轮已复核本机五个 Map.pak 候选，9 个 `tile_17_mou_a_01..09` 仍没有准确包条目；来源搜索回执为 [`original-map17-texture-source-search-20261015.json`](evidence/original-map17-texture-source-search-20261015.json)。
