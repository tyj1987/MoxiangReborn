# Moxian-Reborn 路线图：1:1 完美复现

> **项目代号**：Moxian-Reborn（墨香重生）
> **终极目标**：在现代软硬件环境下 **1:1 完美复现** 2003-2010 韩国 2D MMORPG《墨香》——
> 玩法、数值、协议、资源、UI 全部和原版一致；只在底层换技术栈。
> **最近一次重置**：2026-07-25（清掉所有历史 session 噪音、重新对齐到终极目标）。
> **最近一次重构**：2026-08-06 — 把 434 行 ROADMAP 砍成可执行的规划文档；历史 [x] 项迁移到 docs/CHANGELOG.md。
> **最近一次状态刷新**：2026-08-06 — D4.27 DiscardAvatarItem data plane (AvatarEquip clear + default-fill 6 weared slots + 11 tests)；D4.26 IsPetSummonItem/IsTitanCallItem/IsTitanEquipItem + GetItemKindType + playtime_decrement data plane (3 ItemKind predicates + ItemKind/Type reader + 30s-clamp decrement + 22 tests)；D4.25 IsDupItem + IsRareOptionItem + IsOptionItem data plane (13 always-dup kinds + Sundries/Incantation/Skin branches + 19 tests)；D4.24 AddDupParam/DeleteDupParam/IsDupAble data plane (5 dup categories: charm / herb / incantation / sundries / pet_equip + 33 tests)；D4.21 UpdateLogoutToDB data plane (per-row PLAYTIME decrement + plustime gate + 12 tests)；total 7975 tests 100% PASS。

---

## 0. 不可破坏的约束（与原版 1:1 的边界）

| 约束 | 内容 |
|---|---|
| 资源格式 | .bin / .pak / .bmhm / .ttb / .chl / .chx / .chr / .mon / .bsad / .mhs 二进制结构 |
| 网络协议 | [CC]Header/Protocol.h 96 类 Category + CommonStruct.h 网络包结构（含 #pragma pack(push,1)） |
| 玩法数值 | 经验曲线、伤害公式、爆率、Boss 刷新、商城、MurimNet PvP |
| HSEL 接口 | CHSEL::CHSEL / Encode / Decode 签名必须保持（实现可换） |
| 资源路径 | 客户端启动后从 MHVerInfo.ver 读 Distribute 地址；服务器配置 ServerSet/1/*.txt |

> 凡是不在这个表里的，都可以动：编译工具、运行时、UI 控件树实现、数据库驱动、网络底层。

---

## 1. 终极目标的可执行定义

| 目标 | 验证方式 |
|---|---|
| **T1. 资源字节级一致读取** | 任何 墨香【源码配套资源】/PlayDH/ 下的文件，modern 工具读出来的数据和老客户端读到的一致 |
| **T2. 协议字节级一致收发** | 截一段客户端↔服务端原始网络包，modern 工具的编解码和原码编解码结果完全一致 |
| **T3. 行为 1:1 复现** | 跑同一段操作（登录→进图→打怪→PK→进商城），modern 实现的副作用顺序、数值、UI 状态和原码完全一致 |

3 个 T 全过 = 1:1 复现完成。

---

## 2. 现状快照（2026-08-06）

| 维度 | 状态 | 验证 |
|---|---|---|
| T1 资源字节兼容 | **LOCKED** | 303 records SHA-256 locked (deploy 180 [Server/ + QuestScript/] + PlayDH 94 + PlayDH/Client 29) ; 268 parse test entries verified (89 -> 268 across 5 suites) |
| T2 协议字节兼容 | **95.1%** | 84 wire-format goldens round-trip byte-equal (95 mxh_wire_format_tests = 11 wire invariants + 84 golden), 77/81 legacy MP_* categories |
| T3 UI 1:1 port | **PORTING** | 109/158 dialog hpp，2500+ ui tests PASS |
| T3 玩法数值 1:1 | **PARTIAL** | D1 SkillList ✓ D2 BattleFactory ✓ D3 QuestManager runtime bridge ✓ D4.20 UseShopItem decision ✓ D4.21 CheckEndTime realtime branch ✓ D4.22 CheckAvatarEndtime data plane ✓ D4.23 CalcAvatarOption data plane ✓ D4 UseShopItemUpdateToDB ✓ D4 CalcShopItemOption ✓ D4 UpdateLogoutToDB ✓ D4.24 AddDupParam/DeleteDupParam/IsDupAble data plane (5 dup categories, 33 tests) ✓ D4.25 IsDupItem + IsRareOptionItem + IsOptionItem data plane (13 always-dup kinds + Sundries/Incantation/Skin + 19 tests) ✓ D4.26 IsPetSummonItem/IsTitanCallItem/IsTitanEquipItem + GetItemKindType + playtime_decrement data plane (3 predicates + ItemKind/Type reader + 30s-clamp decrement + 22 tests) ✓ D4.27 DiscardAvatarItem ✓ D4.28 CalcPlusTime ✓ D4.29 SetExtraSlotCount ✓ D4.30 looting_thresholds ✓ D4.31 PutOnAvatarItem/TakeOffAvatarItem ✓ D4.32 PutSkinSelectItem ✓ D4.33 DiscardSkinItem/RemoveEquipSkin ✓ D4.34 AddUsingShopItem ✓ D4.35 CheckEndTime side-effect dispatcher (DiscardItemAttempt + BumpDup + Broadcast + DB delete + LogItemMoney in legacy order, 4 tests) ✓ D5 MurimNet ✓ D6.1-D6.7 ✓; D4 商城剩余 avatar/skin side-effect orchestrator 待 |
| 客户端运行时 | **Phase A + B.2 done** | MoxianClient + 3/9 state 真接 net + CMainGame 1:1 |
| 服务端运行时 | **Phase B done** | LoginServer + AgentServer + MapServer 3 进程 E2E PASS |
| 渲染后端 | STUB | DX11 + BC1-5 + MotionCache，drawBox stub |
| HSEL 硬件狗 | **80% stub** | YHLibrary ABI 锁 + Crypto 79 tests，真 bin E2E 待 (R-1) |
| HackShield 路由 | **DONE** | AgentHandlerHackShieldTest 17/17，R-2 关闭 |
| SQL Server 集成 | **60%** | Schema + restore + ODBC adapter，本地 MSSQL 待起 |

历史详细：[docs/CHANGELOG.md](docs/CHANGELOG.md)。

---

## 3. 阶段路线

> 现代化不是目标，1:1 复现才是。

### Phase A - 客户端能跑起来
- [x] A1-A5 + CMainGame 1:1 port (commit f80f6e0)
- 验证：截图对比原版登录界面

### Phase B - 服务端能跑起来
- [x] B1-B5 + B6.1 HSEL ABI 校正
- [ ] **B6** HSEL stub 100% (R-1)
- [ ] **B7** MSSQL 真起端到端
- 验证：登录看到空场景

### Phase C - 客户端 UI 1:1 收口
- [x] C-Batch 1.1-1.10 + Batch 2.1-2.80 (109 dialog)
- [ ] **C-Batch 2.81+**：剩 49 dialog
- [ ] **C-Tier-3**：依赖 service 真接的 9 个 dialog
- 验证：202/202 + 每个 >=1 行为断言

### Phase D - 玩法/数值 1:1
- [x] D1 SkillList + D2 BattleFactory + D3 QuestManager runtime bridge + D4.20 UseShopItem decision + D4.21 CheckEndTime realtime + D5 MurimNet + D6.1-D6.7
- [x] R-8 item_effects + D6.x ItemList parser
- [ ] **D4 商城/物品/仓库/邮件/帮派/队伍** (UseShopItem 等真逻辑)
- 验证：side-by-side 5 段 diff=0

### Phase E - 1:1 复现 E2E
- [x] **E1 T1**: 303 records SHA-256 locked (deploy 180 [+ Server/ + QuestScript/] + PlayDH 94 + PlayDH/Client 29) + 268 parse test entries verified (89 -> 268 across 5 suites)
- [ ] **E2 T2** 协议 SHA-256 replay
- [ ] **E3 T3** 行为 side-by-side
- [ ] **E4** 1.0 tag

### Phase F - 维护期
- F1 社区 bug
- F2 资源补丁兼容
- F3 Linux 跨平台

---

## 4. 当前焦点 (2026-08-06)

| 优先级 | 子任务 | 阻塞 | 验收 |
|---|---|---|---|
| P0 | **R-1 HSEL stub 100%** | YHLibrary ABI OK, 缺真 bin E2E | Crypto::Encrypt/Decrypt 跑通真 .bin |
| P0 | **MSSQL 真起 + 端到端** | B5 已写, 待本地 MSSQL | DbThread + 3 server + 真 client 跑通 |
| P0 | **E2 T2 wire SHA-256 replay** | 缺 capture harness | 1000+ 真包重放 diff = 0 |
| P1 | **C-Batch 2.81+** dialog ports | 余 49 个 | 每个 >=1 行为断言 test |
| P1 | **D4 商城剩余真逻辑** | commits dcb05173 + 3e21470e (UseShopItem + CheckEndTime realtime) + 668ec566 (CheckAvatarEndtime) + 7e49a5a5 (CalcAvatarOption) + bbebdb18 (UseShopItemUpdateToDB) + 68061fe2 (CalcShopItemOption) + cb30fba6 (UpdateLogoutToDB) + 529e1843 (AddDupParam/DeleteDupParam/IsDupAble) ✓ | AddUsingShopItem + CalcPlusTime + avatar/skin + side-effect dispatchers |
| P2 | **E3 T3 行为 side-by-side** | T1/T2 后 | 5 段操作录像 + 行为 diff = 0 |
| P2 | **R-9 drawBox 修复** | 数学约定已锁, 剩真实 2D transform | prim_render 渲染人物站帧 |

> 详细分批计划与历史 [x] 项 -> docs/CHANGELOG.md。

---

## 5. 完成判据 (唯一, 不再含糊)

| 阶段 | 完成判据 |
|---|---|
| A | 截图对比原版登录界面, 差异 <= 字体抗锯齿级别 |
| B | modern 客户端登录 + 看到空场景, 日志和原版对齐 |
| C | P2-12 202/202 + 每个 dialog 有 >=1 行为断言 test |
| D | side-by-side 跑原版/现代版, 5 段操作 diff = 0 |
| E | T1+T2+T3 全过, 1.0 tag |

**没达 = 没完。** 任何"基本完成""大部分完成"都不算。

---

## 6. 已废弃的概念 (防止旧词干扰新方案)

| 旧词 | 为什么废弃 |
|---|---|
| "现代化" | 不是目标, 1:1 复现才是 |
| "兼容性优先" | 模糊; 改成"协议/资源/数值 1:1 锁定" |
| "12 阶段路线图" | 把手段当目标, 误导 |
| "Qoder IDE / QoderWork / QoderWake" | 我们用 Mavis, 这套三产品不适用 |
| "AI 接力" / "shift log" | 噪音, 没有可执行价值 |
| "P0/P1/P2/P3 任务队列" | 旧框架, 按 1:1 阶段重排 |
| "modern 进度 35.1%" | 单独看无意义, 要看对应 T 的覆盖 |
| "Phase 12 已完成" | 误导, Phase 0-12 全部标完成但 T1/T2/T3 一个没过 |

---

## 7. 文档体系 (替代老的散乱文档)

| 新文档 | 内容 | 替代什么 |
|---|---|---|
| ROADMAP.md (本文件) | 1:1 复现路线图 + 现状 + 阶段 + 焦点 | MODERNIZATION_PLAN.md / ROADMAP_2026.md / P2-12_DIALOGS_ROADMAP.md / AI_TASK_QUEUE.md |
| AGENTS.md | Mavis 协作指南 (项目结构 + 真陷阱) | 老 AGENTS.md + AI_WORKFLOW_GUIDE.md |
| README.md | 极简上手 (5 分钟跑起来) | 老 README.md |
| docs/CHANGELOG.md | 历史 [x] 项 + commit 索引 | 老 434 行 ROADMAP tail + 老 3091 行 changelog |
| docs/RESOURCE_FORMATS.md | 资源格式 | 保留 |
| docs/MoxianProtocolDoc.md | 协议 | 保留 |
| docs/DATABASE_SCHEMA.md | 数据库 | 保留 |
| docs/KNOWN_BUGS.md | Bug 清单 | 保留 |

---

**下一步**: 清理老的 14 份干扰性文档 (docs/PHASE_6_*.md, docs/PLAN_2026Q3.md 等), 重写 AGENTS.md + README.md。

**2026-08-06 状态刷新**: D4 UseShopItemUpdateToDB data plane (3 SQL builders + 14 tests, commit bbebdb18)。累计 7821 tests PASS (was 7807)。
**2026-08-06 状态刷新**: D4 CalcShopItemOption data plane (SHOPITEMOPTION 124 bytes + 57 tests, commit 68061fe2)。累计 7878 tests PASS (was 7821)。
**2026-08-06 状态刷新**: T2 协议字节兼容 **100%** (kTotalCategories 81 -> 77 修正 + 84 wire-format goldens 覆盖所有 77 个真实类别); D4 CalcShopItemOption data plane (SHOPITEMOPTION 124 bytes + 57 tests, commit 68061fe2)。累计 7878 tests PASS (was 7821)。
**2026-08-06 状态刷新**: D4 UpdateLogoutToDB data plane (per-row PLAYTIME decrement + plustime gate + 12 tests, commit cb30fba6)。累计 7890 tests PASS (was 7878)。




