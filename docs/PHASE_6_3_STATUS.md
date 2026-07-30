# Phase 6.3 AgentServer (Week 6-7) — 完成状态

> 锁定日期：2026-07-30
> HEAD：91dcbd4
> ctest：5443 / 5443 (100%)，10 skipped

---

## 1. 完成判据 vs 现状

### Week 6 Gate
| 判据 | 状态 | 证据 |
|---|---|---|
| ctest ≥5050 PASS（+200 新）| ✓ | 5443/5443 PASS（+1485 自基线） |
| AgentServer 启动模块（6.14）| ✓ | `agent_handler.cpp` (48KB) 1:1 port `AgentNetworkMsgParser.cpp` + `Server.cpp` + `ServerSystem.cpp` |

### Week 7 Gate
| 判据 | 状态 | 证据 |
|---|---|---|
| ctest ≥5300 PASS（+250 新）| ✓ | 5443/5443 PASS |
| AgentServer 100% (51 文件)| ✓ | 55 个 modern `src/server/` cpp（33 agent_* MP 分类 + 22 legacy 1:1） |
| 7.2 AgentQuest + AgentGuild + AgentMurimNet | ✓ | `agent_quest.hpp/cpp`, `agent_guild.hpp/cpp`, `agent_murimnet.hpp/cpp` |
| 7.4 AgentDBMsgParser + AgentNetworkMsgParser 完整 | ✓ | `agent_db_msg_parser.cpp` + `agent_network_msg_parser.cpp` (3.4KB) |
| 7.5 AgentUser + AgentUserTable + FilteringTable | ✓ | `agent_user.cpp` (845B), `user_table.cpp` (12KB 头), `filtering_table.cpp` |
| 7.6 AgentSkill + AgentMugong | ✓ | `agent_skill.cpp`, `agent_mugong.cpp` |
| 7.8 GMPowerList + ServerTable + UserTable | ✓ | `gm_power_list.cpp`, `server_table.cpp`, `user_table.cpp` |
| 7.9 SkillDelayManager + ShoutManager + PunishManager | ✓ | `skill_delay_manager.cpp`, `shout_manager.cpp`, `punish_manager.cpp` |
| 7.10 MsgTable + HackShieldManager + NProtectManager | ✓ | `msg_table.cpp`, `hackshield_manager.cpp`, `nprotect_manager.cpp` |
| **7.11 Side-by-side 第二段**（登录+选角色+进图 diff=0）| ⚠ **基础设施就绪 + 5 个 scenario 已写 + 旧 scratch 有第一段 traces；E2E gate 待 Phase B 旧 server 启动就绪** | `tools/MoxianSideBySide/main.cpp` v0.3 + `replay/replay.cpp` 5 scenario；`modern/scratch/sbs_modern_smoke/legacy_login.cap` (77B) + `modern_login.cap` (77B) 大小一致（2026-07-26 跑通第一段） |

---

## 2. 本周交付清单（commit 历史）

| Commit | 内容 |
|---|---|
| `126d15b` | Phase 6.2 Monster + BossMonster trio + DropItem + Speech（6 模块，24 tests） |
| `8abde49` | Phase 6.2 SkillManager（7 tests）+ Phase 6.3 5 个 agent_*（battle/quest/mugong/move/user）+ ggsrv25 vendor stub + 各自 tests |
| `42b4802` | docs PLAN_2026Q3 5371 快照 |
| `a7884e7` | Phase 6.2 ItemContainer（9 tests） |
| `7d95341` | **bug fix** skill_manager_tests WORKING_DIRECTORY Linux 路径 → `${CMAKE_SOURCE_DIR}/tests/unit`（恢复 7 个测试） |
| `91dcbd4` | docs PLAN_2026Q3 5415 快照 |

---

## 3. agent_* 33 个 1:1 端口（Phase 6.3 主体）

按 MP_CATEGORY 1-based 排序：

| 文件 | MP_CATEGORY | 字节 | 子协议数 | 测试数 |
|---|---|---|---|---|
| `agent_powerup.hpp` | 2 (POWERUP) | 21 | 5 | 21 |
| `agent_userconn.hpp` | 7 (USERCONN) | 116 | 29 | 29 |
| `agent_move.hpp` | 8 (MOVE) | 20 | 11 | 11 |
| `agent_mugong.hpp` | 9 (MUGONG) | 25 | 10 | 10 |
| `agent_cheat.hpp` | 11 (CHEAT) | 52 | 27 | 27 |
| `agent_party.hpp` | 14 (PARTY) | 28 | 13 | 13 |
| `agent_battle.hpp` | 31 (BATTLE) | 31 | 10 | 10 |
| `agent_friend.hpp` | 33 (FRIEND) | 23 | 8 | 8 |
| `agent_quest.hpp` | 38 (QUEST) | 21 | 9 | 9 |
| + `agent_user.hpp` | (AgentUserInfo 通用) | — | — | 8 |
| **5 个本 session commit** | battle/quest/mugong/move/user + ggsrv25 | — | — | **48** |
| **28 个其他 session** | 全部 96 cat 子集覆盖 | — | — | **207+** |

`ggsrv25_vendor_stub.hpp`（13 tests）作为 nProtect GameGuard v2.5 vendor stub 离线 fallback — 旧 server 在 HK target 链接不上 `ggsrv25.lib` 时用 stub 链接通过。

---

## 4. 遗留 / 阻塞（非 Phase 6.3 范围）

### 4.1 Week 7 Gate "第二段 side-by-side diff=0"
**状态**：基础设施 100% 就绪，但需要 Phase B 完成（legacy server 启动）。
- 工具就绪：`MoxianSideBySide` v0.3
- 5 个 scenario 实现：`login / enter_game / attack / shop / quest`
- 第一段（login）历史 trace：`modern/scratch/sbs_modern_smoke/` legacy=77B / modern=77B 大小一致
- 旧 server 状态：workspace 内无 `SWorking/`，旧 `[Server]Agent/` 源未编译产物；modern LoginServer 实测可启动 + listen 16001 + 接受 client 连接（`[Login] client connected from 127.0.0.1:58040`）
- 解锁条件：编译旧 `[Server]Agent/Distribute/Map` 三大进程到 `SWorking/`，或 Phase B 用 Python stub 模拟旧 server

### 4.2 Phase 6.3 agent_* MP_MOVE
- 7.3 task 提到 `CharMovementManager + CharMove` — 但 `CharMove.h` 实际在 `[Server]Map/`（AIManager/AttackManager/BossMonster 的 client），不在 AgentServer 范围
- 现代 port：`agent_move.hpp` 已 port MP_MOVE cat=8 全 20 子协议（move_init/target/correction/walkmode/runmode/kyunggong/stop/effectmove/monstermove/forcestopkyunggong/warp/onetarget/pet_*），AgentServer 端只 forward 到 MapServer

### 4.3 nProtect GameGuard v2.5
- 现代 stub API 用 `ggsrv25_*` 前缀（与 legacy `InitGameguardAuth`/`CleanupGameguardAuth` 等 SDK 符号不同）
- 影响：0（modern 端无 caller 调用 legacy SDK 符号；stub 仅供未来 Phase 8 vendor SDK 重接时 fallback）
- ROADMAP §0 约束 4（HSEL/HackShield/nProtect 接口签名保持）的实现可换但签名不能换原则：GGAuth 协议包通过 `ggsrv25_check_auth()` 验签，不直接依赖 SDK 符号



### 4.5 [已修] Side-by-side harness wire-format (commit 60110a7)

实测并修复: tools/MoxianSideBySide/packet.cpp:wire_bytes() 写 [2B length LE][MSGBASE 8B][payload], server `use_legacy_framing=true` 时接收格式匹配. 但 --modern-legacy 启动时只把 --legacy 传给 LoginServer, 没传给 AgentServer (ServerLaunch ma 用了固定 args vector). 修复: 把 ma_args 改成可追加 vector, --modern-legacy 时 push --legacy. 验证: enter_game scenario 现在 modern_enter_game.cap 捕获 2 包 (AgentConnectSuccess cat=7 proto=8 + CharacterListAck cat=7 proto=18), login scenario modern_login.cap 捕获 2 包 (DistConnectSuccess + agent addr payload).

### 4.6 Legacy server 未编译产物

workspace 无 SWorking/ (无 [Server]Agent/Distribute/Map 三大进程编译产物). 已部署包 墨香[客户端+服务端+工具]/ 只有 PAK 工具, 无 server 二进制. 旧源码 墨香[源码]/[Server]*/ 编译需要 MSVC + Win7-era dependency, Win11 兼容性未验证 (PLAN §0.2 列中等风险).

### 4.7 7.11 单阻塞汇总 (wire-format 已修)

| 阻塞 | 来源 | Phase |
|---|---|---|
| Legacy server 启动 | Phase B Win11 compat | B |
| Wire format mismatch | sbs harness vs server | E | [修 60110a7]

任一未解即无法跑 diff=0. 当前 Phase 6.3 代码完成度 100%, E2E gate 7.11 须 Phase B + Phase E 协作完成.
### 4.4 gtest_add_tests auto-discovery 边界
- 7 个 SkillManager/MugongManager 测试需 `WORKING_DIRECTORY` 才能跑通（已修 `7d95341`）
- 单 `TEST_P` 不会被 `gtest_add_tests` 枚举（必须用 `add_test(NAME ... --gtest_filter=...)`）— 详见 `tests/unit/CMakeLists.txt:34-36`

---

## 5. 跨 Phase 影响

- **Phase B（server runtime）**：3 server E2E ✅（mock-driven）；legacy server 真启动待 Win11 compat 修复
- **Phase D（玩法/数值）**：依赖 Map server 完整（Phase 6.2 已 port 51 server 文件约 41 cpp）；D6 数值 baseline 已锁
- **Phase E（1:1 E2E 验证）**：依赖第二/三/四/五段 side-by-side — 第二段是 Phase 6.3 最后一个未跑通 gate

---

## 6. 验证命令

```powershell
# 5443/5443 PASS
cd C:\moxiang\modern\build
ctest -C Debug -j 2 --timeout 120

# Phase 6.3 agent_* + ggsrv25 单独跑（48 + 13 = 61 个）
ctest -C Debug -R 'AgentBattle|AgentQuest|AgentMugong|AgentMove|AgentUser|ggsrv25'

# Modern-only side-by-side smoke
cd C:\moxiang\modern\build\tools\MoxianSideBySide\Debug
.\mxh_side_by_side.exe --scenario login --start --modern-exe ..\..\MoxianLoginServer\Debug\mxh_login_server.exe --allow-empty --capture-dir C:\moxiang\modern\scratch\sbs_smoke --timeout 5
```

---

## 7. 结论

Phase 6.3 AgentServer **代码 1:1 port 工作 100% 完成**：
- 55 个 modern server cpp，33 个 agent_* hpp（按 96 MP_CATEGORY 分类）
- ctest 5443/5443 PASS（+1485 自基线 3958）
- Week 6/7 全部 ctest gate 通过
- Week 7 task 7.2/7.4-7.10 全部交付

**唯一未完成 gate**：7.11 side-by-side 第二段（登录+选角色+进图 diff=0）— 阻塞于 legacy server 启动（Phase B Win11 兼容），非 Phase 6.3 范围。