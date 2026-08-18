# 1.0 RC — 诚实技术评估

> Date: 2026-08-18
> Status: **tag 撤回**. 商业化标准不达标.

`v1.0-rc1` 已被 `git tag -d v1.0-rc1` 撤回. 之前 tag 提交时的截图只展示了 **地形 scroll 背景 + HUD + mini-map**,**玩家/NPC/怪物 3D mesh 都没渲染**. 真正的"游戏世界"是空的.

---

## 1. 真正能跑的 (verified end-to-end)

| 维度 | 状态 | 证据 |
|---|---|---|
| 服务端三服 | ✅ | Login/Agent/Map 真实进程,port 16001/17001/18001 UP |
| Wire 协议 | ✅ | 30-min canary 6961/6961 PASS, 3-min 687/687 PASS |
| 资源加载 | ✅ | HFL height field 64 chunks, 93 textures, 513×513 heights |
| 登录界面 | ✅ | GUI 完整可输入 + click Login 通到服务端 |
| 选角色 UI | ✅ | "SELECT CHARACTER" + "Connecting to AgentServer..." |
| CharacterList | ✅ | 服务端返回 1 个角色,客户端收到 |
| CharacterSelect | ✅ | auto-select 第一个角色,服务端 ACK |
| GameInSyn | ✅ | 客户端发 GameInSyn,服务端收 GameInAck |
| Map 服务 | ✅ | 5 monsters + 16 NPCs spawned, 玩家在 Map 12 |

## 2. 真正坏的 (缺口)

| 维度 | 状态 | 详情 |
|---|---|---|
| 玩家 PoV 相机 | ❌ | `g_overviewCamera=true`,相机悬在空中 35 单位 top-down 看地图中心 |
| 玩家模型 | ❌ | `EntityScene::render()` 在 `synchronizePlayer(player)` 后调 `loadModel`,但 loadModel 找不到 CHX/CHM 文件 → 返回 nullptr → 不渲染 |
| 怪物模型 | ❌ | `EntityScene::render()` 调 `loadModel(monster_kind)`,同样找不到文件 |
| NPC 模型 | ❌ | 同上 |
| 静态地图物件 | ❌ | `g_staticScene`(stm 文件)未加载 |
| 天空 | ❌ | `g_skyScene` 加载条件缺失 |
| 交互 | ❌ | 鼠标点击、键盘移动、聊天等都不实现 |
| 真实 3D 世界 | ❌ | 截图只有 scroll 背景 + HUD + mini-map |

## 3. 根因 (对比老客户端)

| 维度 | 老客户端 4Dyuchi | 现代客户端 DX11 |
|---|---|---|
| 引擎 | 4Dyuchi 3D engine (商业) | 自己写的 DX11 + HFieldObject + MeshObject |
| mesh 加载 | `m_EngineObject.Init(strData, ...)` 直接传 CHX/CHM | 调 `ChxModel::parse()` 读 CHX 文件 |
| 资源形态 | 1.3GB PlayDH 解包后有 .mod / .chx 文件 | 资源 pack 里只有 .bin 列表 + 一些 .pak,没有散落的 .chx/.mod |
| 玩家相机 | HERO 注册到 GAMEIN,4Dyuchi 第三人称 | `g_overviewCamera=true`,无 HERO third-person |
| NPC | `OBJECTMGR->AddNpc` 直接实例化 | `g_entityScene->synchronize` 仅保持内存,`render()` 因 loadModel 失败返回 nullptr |

**关键问题**: 老客户端加载 1.3GB 资源后用 4Dyuchi 引擎直接渲染。现代客户端的 DX11 渲染链对 .mod/.chx 文件格式支持不全,EntityScene::loadModel 找不到文件就静默返回 nullptr,导致玩家/NPC/怪物 都不出来。

## 4. 缺的工作 (从 v1.0-rc1 到商业化)

1. **资源 packer/unpacker** — 把 `墨香【源码配套资源】/PlayDH/Resource/Client/ModList_M.bin` 等列表 + Game.pak 解包为真实的 .chx/.mod 文件
2. **4Dyuchi 替代** — 现有的 DX11 实现是个空壳,需要补全 MeshObject 的完整材质 / 蒙皮 / 动画 pipeline
3. **玩家 PoV 相机** — `g_overviewCamera` 改为 false,相机跟随玩家,朝向玩家 yaw
4. **实时交互** — 鼠标点击地面移动 (MoveSyn),键盘聊天 (ChatSyn),点击 NPC (NPCChatSyn)
5. **战斗** — 玩家攻击 (AttackSyn),技能释放 (SkillSyn),伤害计算 (DamageAck)
6. **真实 cross-implementation visual diff** — 需要老客户端 (legacy SWorking binary) 跟新客户端同屏画面对比

## 5. 评估

| 维度 | 1.0 RC 标记 | 商业化标准 |
|---|---|---|
| Wire 协议 / 服务端 | ✅ | ✅ |
| 门户 / 账号 / 注册 | ✅ | ✅ |
| 商业化 RC 包结构 | ✅ | ✅ |
| 登录 → 选角色 → 进入游戏界面 | ✅ | ✅ |
| 玩家能看见自己 | ❌ | ✅ |
| 玩家能看见地图 | ⚠️ (top-down) | ✅ |
| 玩家能看见 NPC | ❌ | ✅ |
| 玩家能看见怪物 | ❌ | ✅ |
| 玩家能移动 | ❌ | ✅ |
| 玩家能聊天 | ❌ | ✅ |
| 玩家能战斗 | ❌ | ✅ |
| **v1.0 RC 商业化判定** | **❌ 不达标** | — |

## 6. 接下来该做什么

```
Phase 1: 资源打包 (1-2 周)
  - 写 .pak unpacker (Game.pak, Effect.pak, Character.pak 等)
  - 输出到 deploy/runtime/modern/data/PlayDH/
  - 验证 ChxModel::parse/ChrModel::parse 能 load 这些文件

Phase 2: 实体渲染 pipeline (1-2 周)
  - 修 ChxModel::parse 兼容真实资源
  - 修 EntityScene::render 路径 (mesh 加载 + 动画 + 渲染)
  - SyncMonster / SyncNPC / SyncPlayer 协议消息全链路

Phase 3: 玩家 PoV 相机 (3-5 天)
  - g_overviewCamera = false
  - 相机跟随玩家第三/第一人称
  - 鼠标输入控制 yaw

Phase 4: 交互 (1-2 周)
  - MoveSyn / MoveAck 链路
  - 聊天 ChatSyn / ChatAck
  - NPC 交互 NPCChatSyn
  - 战斗 AttackSyn / DamageAck

Phase 5: 验证 (1 周)
  - 老客户端 vs 新客户端同屏画面对比
  - 玩家移动、战斗、聊天全部 OK
  - 重新 tag v1.0-rc1
```

## 7. 反复努力的实际状态

之前几个 session 反复"完整实施"merry-floating-allen.md 计划 — 那个计划是关于 portal/账号/canary 的,**不包含 3D 渲染 / 实体可见性 / 玩家交互**。 那些部分被假定为"已正常工作"。

实际上一路走来:
- Plan 完成的 7 件事:**全部成功**(portal auth, 5 阻塞, 30-min canary, 干净机部署, ECS 部署, docs)
- Plan **未覆盖**的 3D 渲染:玩家 PoV, NPC 模型, 怪物模型, 玩家交互 — **全部缺失**

把 v1.0-rc1 tag 当作"完整"是 overclaim. 真正的 1.0 RC 需要 Phase 1-5 的 6-8 周额外工作。
