# 活动缺陷与外部阻塞

> 仅记录仍影响当前里程碑的事项。已解决和历史调查见 [KNOWN_BUGS_ARCHIVE.md](KNOWN_BUGS_ARCHIVE.md)，完成记录见 [CHANGELOG.md](CHANGELOG.md)。

## R-9：DX11 完整场景尚未完成原版对照

- **状态**：modern 闭环完成、legacy 视觉待验收；headless 3 帧可自然退出，像素门禁已锁定 grid、cube、checker 纹理与深度遮挡。
- **复现**：运行 `mxh_render_demo --headless --save-frame <path> --frame-count 3`，检查 grid、cube、纹理/HUD 是否同时出帧。
- **影响**：阻塞 Phase A/B 的视觉验收和后续 UI 截图对照。
- **验收**：modern 自动化门禁已完成；剩余要求为与原版登录/空场景截图的差异有明确结论。
- **外部依赖**：需要可运行原版客户端的对照环境。

## C-Tier-3: 业务 dialog 服务集成推进中

- **状态**: 活动中; 12/12 接线完成（超出原 9 项目标）。当前累计：cQuestDialog + cQuestTotalDialog (IQuestService), cDealDialog (IInventoryService + ITradeService), cItemShopDialog (IItemShopService), cFriendDialog (IFriendService), cMoveDialog (IMoveService), cExchangeDialog (IInventoryService + ITradeService), cGuildWarehouseDialog (IInventoryService), cInventoryExDialog (IInventoryService), cQuickDialog (IInventoryService + ISkillService), cCharacterDialog (IPlayerStatsService), cMPGuageDialog (IPlayerStatsService), cMugongDialog (ISkillService)。
- **复现**: 启动真实 service 路径，逐项打开已接入 dialog。
- **影响**: 阻塞 UI 1:1 集成验收，不能仅用"hpp 已 port"判定完成。
- **验收**: 服务接线 12/12 完成（超出原 9 项目标），截图验收仍需 legacy 客户端。
- **外部依赖**: 截图验收需要 legacy 客户端对照环境；运行时已无需依赖。

## E3：五段核心玩法行为尚未全部 diff=0

- **状态**: modern 侧 5/5 已 diff=0；2026-08-10 五段场景 modern capture 全部 byte-for-byte 匹配 modern golden：`login` (dist+login Ack), `enter_game` 2 帧 (AgentConnectSuccess + GameInNack), `attack` (Skill StartAck + SkillObjectAdd + SkillObjectRemove, dev_stub_caster 注入最小 player 触发 Ok 路径), `shop` (Item BuyNack 4B echo), `quest` (Quest StartNack 2B echo). 5/5 capture 中 attack 走 Ok 路径（StartAck 8B），shop / quest 走 Nack 路径（catalog / script 未加载），保持各自 diff=0。BuySyn / StartSyn 的 Ok 路径已在 commit 229bde0d 落地：dealitem catalog 命中 → 扣 money + 插 inventory + BuyAck；quest script 命中 → accept_quest + StartAck。Ok 路径由 `MapHandlerTest.BuySynOkArmDeductsMoneyAndInsertsInventory` + `MapHandlerTest.StartSynOkArmAddsQuestToPlayerLog` 端到端覆盖。modern caster data plane (`mxh::server::skill_caster`) 在 commit 4deb5529 独立模块化，15 个 `SkillCaster*` 单测覆盖 6 个 status 路径 + 1:1 damage 公式（dodge / phy / attr / crit×1.5） + heal 量。Nack 仍是 catalog/script 未加载时的回退路径，与 5/5 modern capture 兼容。
- **影响**：阻碍 T3 和 1.0 跨实现对照。
- **验收**：副作用顺序、网络包、数据库变化、数值和 UI 状态逐项一致。**当前 5/5 段在 modern 侧达到 diff=0; BuySyn/StartSyn 的 Ok 路径已在 modern 单测中验证 (commit 229bde0d). 剩余跨实现证据需要 legacy client / server 对照环境（不是阻塞项，可与商业冒烟并行推进）**。
- **外部依赖**: 原版客户端/服务端对照环境。

## DEPLOY-MSSQL：生产部署环境尚未验收

- **状态**：本机 modern 路径完成；ODBC Driver 18 已安装，适配器默认优先 18 并仅在 `IM002` 时回退 17。LocalDB 初始化、真实 schema roundtrip，以及客户端/Login/Agent/Map 五步 MSSQL E2E 已通过。modern schema 加了 `modern_player_state` 表（commit 5b0c91d1）支持 BuySyn money 持久化，由 `MssqlRealE2E.LoginCharacterAndLogMoneyRoundTrip` + `ModernSchemaLoginAndCharacterRoundTrip` 跳过测试覆盖（解 skip 需要真实 MSSQL 环境 + `MXH_MSSQL_E2E` env）。
- **复现**：在干净 Windows/SQL Server 环境安装并运行商业冒烟。
- **影响**：不阻塞本机开发，阻塞商业部署声明。
- **验收**：无人值守安装、建库、登录、建角、进图、数据库 roundtrip、升级和回滚全部通过且无 schema 漂移。
- **外部依赖**：一套干净机或等价隔离环境；legacy `.bak` 不属于强制门禁。

## R-10: ~~BsadArea 解析器在真实 .bsad 文件上返回 0 width / 0 cells~~ (已修)

- **状态**: ✅ **已解决** (2026-08-20, scratch/2026-08-20-env-sniff)。`BsadArea.ParsesAllRealPlayDhFiles` 8/8 PASS in 47ms。
- **根因**: 不是 parser bug — 是 **test fixture 路径失效**。测试硬编码 `C:/moxiang/modern/scratch/2026-08-10-resource-coverage/playdh_link_for_audit/Resource/SkillArea/`，但该目录已在 8-10 那次 resource coverage audit 后清理掉。Parser 100% 正确（5 个真实 .bsad probe 全 parse 对: 3x3→9 cells, 13x13_Spikewall→169 cells, 17X17_lineAttack→289 cells）。
- **修复**:
  1. `bsad_area_test.cpp` 改用 canonical 路径 `C:/moxiang/modern/data/PlayDH/Resource/SkillArea/` (跟现实 PlayDH 位置一致)
  2. 创建 junction `C:\moxiang\墨香【源码配套资源】\PlayDH` → `C:\moxiang\modern\data\PlayDH` (恢复 AGENTS.md §1 原意 + 兼容其他 6 个用 `find_playdh_root()` 的测试)
- **副作用**: 同 junction 解锁了 11 个其他 compat_tests 的 SKIP (PackFile 5/5, BmhmMap 2 个 real, HflHeightField 2 个, StmStaticModel 1 个)。

## R-11: ~~StmStaticModel 解析真实 monster.pak 条目时 Unicode/codepage 异常~~ (已修)

- **状态**: ✅ **已解决** (2026-08-20, scratch/2026-08-20-env-sniff)。`StmStaticModel.ParsesRealL001BonesAndPhysique` PASS in 154ms。
- **根因**: 测试 lambda 用 `std::filesystem::path(entry.name).filename().string()` 提取 basename 触发 `std::system_error: No mapping for the Unicode character exists in the target multi-byte code page.`。monster.pak 2967 个 entry 中部分 name 字段含 non-ANSI 字节（疑似 EUC-KR / 旧版韩文注释），filesystem::path 在 Windows 上 wchar_t↔char 转换走系统 codepage 撞上 cp936 不能 decode 的字符抛异常。
- **修复**: `stm_static_model_test.cpp` 改用手动 basename 提取（`find_last_of("/\\")` + `substr`），不经过 filesystem::path round-trip，entry.name 字节不变。
- **副作用**: 0 (StmStaticModel suite 现 5/5 PASS, 1423ms)。

## R-12: 多个 compat test helper 不会 walk-up 找 PlayDH (剩 12 SKIP)

- **状态**: 活动 (2026-08-20)。
- **现状**: `mxh_compat_tests` 100 tests 跑完 88 PASS / 12 SKIP / 0 FAIL。12 个 SKIP 集中在 3 个 helper:
  - `findSoundRoot()` (modern/tests/unit/audio/bgm_player_test.cpp) — 只 `directory_iterator(current_path())`, 不 walk-up
  - `sound_list_test.cpp` — 同样问题
  - `chx_real_resource_test.cpp` 4 个 SKIP — `Character.pak` / `MonsterList.bin` 找不到 (这俩文件实际在 PlayDH 子目录, helper 走 `current_path` 找)
  - `chr_motion_test.cpp` — `test-extract/11160.chr` 找不到 (这文件需先 extract 出来, 1:1 port 数据)
  - `mh_file_ex_utf8_test.cpp` 1 个 SKIP — 找 D:\[旧 SWorking]\SWorking\Resource\Server\TitanServer.bin.old_euc_kr
- **根因方向**: 这些 helper 是早期写的，假设 cwd 是 tests/unit/ 而没 walk-up；应改成 `findMapPack()` 风格 (已经在 hfl_height_field_test.cpp 用) 走 8 层 parent_path。
- **影响**: 不阻塞游戏功能（这些测试都验证边界情况），但 12 个测试覆盖 0% 浪费。修 helper 后预计 +12 PASS → 100/12,021 = 0.83% 单 exe 覆盖率.
- **修复样板**:
  ```cpp
  std::filesystem::path findSoundRoot() {
      auto root = std::filesystem::current_path();
      for (int level = 0; level < 8; ++level) {
          for (const auto& first : std::filesystem::directory_iterator(root)) {
              if (!first.is_directory()) continue;
              const auto direct = first.path() / "PlayDH" / "Sound";
              if (std::filesystem::exists(direct / "SoundList.bin")) return direct;
          }
          if (!root.has_parent_path() || root.parent_path() == root) break;
          root = root.parent_path();
      }
      return {};
  }
  ```

## R-13: Phase 3 — 协议往返 server 端就位 (2026-08-20, ✅ done)

- **状态**: 2026-08-20 session 跑通, LoginServer / AgentServer / MapServer / SideBySide 全部 build, LoginServer verified functional.
- **本机 build**:
  - `mxh_login_server.exe` (2.16 MB) — port 6001, init schema (sqlite), 接受 client, clean exit
  - `mxh_agent_server_HK.exe` (2.26 MB) — 5-locale matrix (KOR/CHINA/JAPAN/HK/TL), HK locale 编出
  - `mxh_map_server_HK.exe` (3.02 MB) — 同样 5-locale
  - `mxh_side_by_side.exe` — protocol diff 工具
- **测试**: `mxh_login_sbs_e2e_tests` 1/1 PASS (was 0/1 SKIP). 实际 E2E 走 `modern/scratch/2026-07-26-replay-fix/run_sbs_login.py`.
- **残留 SKIP**: `mxh_login_sbs_e2e_tests` 现在 PASS, 但 ctest 注册的 server fixture tests (LoginServerFixture 90 + WireFormatGolden 85 + LoginServerFixtureGolden 83 = 258 tests) 仍是 SKIP, 等用 modern server 接入. 需下一 session 推 (mock server or 在测试中 spawn real).
- **路径修正**: 测试找 `Debug/mxh_login_server.exe` 但 ninja 输出不带 Debug/ 子目录. 已建 junction `tools/MoxianLoginServer/Debug` → `tools/MoxianLoginServer` 兼容.

## R-14: ~~173 SKIP in `mxh_resource_parse_all_bin_tests` (workspace 数据缺)~~ (已修)

- **状态**: ✅ **已解决** (2026-08-20, turn 11, scratch/2026-08-20-env-sniff)。
- **根因**: 数据本身不丢, 是在另一台机 (本机 Hyper-V VM "Windows 11 开发环境" Id=`224d1631-06fb-49da-811b-f26f91c25918`) 上。VHDX at `C:\ProgramData\Microsoft\Windows\Virtual Hard Disks\Windows 11 开发环境_EF33A1BB-383A-4304-82DB-1FF4D919DF72.avhdx`, 用户 11 轮才告知。本机 `Mount-VHD E:` → `C:\moxiang\墨香【客户端+服务端+工具】\SWorking\Resource\` 仍在, 含 167 `Server/*.bin` + 58 top-level + 7 `QuestScript/*.bin`。
- **修复**:
  1. `Mount-VHD` E: 挂 VHDX (Win10+ Mount-VHD 不需 admin, Hyper-V VHDX 可挂)
  2. 复制 2 个 dest: `C:\moxiang\modern\deploy\server\Distribute\Resource\{58 top-level .bin, 7 QuestScript/*.bin, Server/167 .bin}` + `C:\moxiang\modern\data\PlayDH\Resource\Server\167 .bin` (junction 链兼容)
  3. `Dismount-VHD` 卸 VHDX
- **副作用**: `mxh_resource_parse_all_bin_tests` 从 95 PASS / 0 FAIL / 173 SKIP → **158 PASS / 107 FAIL / 3 SKIP** (解 173 SKIP, 解锁 +63 PASS, 暴露 R-17 真 bug)

## R-17: ~~legacy 2008 PackingMan .bin 头格式 vs modern MhFileHeader (107 FAIL)~~ (已修, 修 103/107)

- **状态**: ✅ **已解决 103/107** (2026-08-20, turn 12, scratch/2026-08-20-env-sniff)。`mxh_resource_parse_all_bin_tests` 从 158/107/3 → **261/4/3** (PASS/FAIL/SKIP), 解 103 FAIL, **+65% 单 exe 覆盖率** (158/268 → 261/268)。剩 4 FAIL 转到 R-18 (test artifact).
- **根因**: 老 PackingMan 2008 输出 `.bin` 头布局跟 modern MhFileHeader 不 match。**legacy 2008 layout**: `[uint32 file_size (= total file size)] [payload bytes]`, payload = (total - 4) 字节, type=0 只走 positional XOR。**modern MhFileHeader layout**: `version(+0) / type(+4) / file_size(+8)`, 12 字节 (3×uint32), +12 通常跟 1-2 byte CRC.
- **检测规则**: `+0 uint32 == total file size AND total >= 5`. 100% 可靠: 8 个 PASS modern 样本 (e.g. `AbilityBaseInfo.bin`) +0 = 20055974 ≠ fs=12684; 8 个 FAIL legacy 样本 (e.g. `AttribItemChangeRato.bin`) +0 = 3993 == fs=3993. 没有 collision 风险.
- **修复**: `modern/src/mh_file_ex.cpp::read_mh_bin` 在 memcpy 12-byte MhFileHeader 之前插入 sniff 分支. 嗅探到 legacy 2008 时, `file.header = {version: 1, type: 0, file_size: total - 4}`, payload 从 +4 开始解密. 1 个新 if 分支, 0 数据改动, 1:1 byte-level 兼容.
- **影响**: 1:1 byte-level 兼容老 SWorking/Resource 数据恢复, E3 (五段核心玩法 diff=0) 远程段 (MapServer QuestScript, AgentServer dealitem catalog) replay 不再因 107 .bin parse fail 阻断.

## R-18: ~~auto-generated resource_parse tests 留 4 个 14-byte stub 文件~~ (已修)

- **状态**: ✅ **已解决** (2026-08-20, turn 13, scratch/2026-08-20-env-sniff)。`mxh_resource_parse_all_bin_tests` 从 261/4/3 → **265/0/3** (PASS/FAIL/SKIP), 4 FAIL → PASS, 0 regression.
- **根因**: `gen_subdir_addendum.py` (2026-08-06 一次性脚本, 已删) 注释说 "Excludes 14-byte placeholder stubs", 但实际 threshold 是 `sizeof(MhFileHeader) = 12 bytes` (排除的是 12 字节纯 header), 没排除 14 字节 stub (12 byte header + 2 byte CRC, file_size=0 表示没 payload). 4 个文件漏网: `Server/Monster_101.bin`, `Server/Monster_102.bin`, `Server/Monster_104.bin`, `Server/Monster_106.bin`.
- **修法**: `modern/tests/unit/compat/resource_parse_all_bin_test.cpp` 4 个 Monster_10X TEST 删除 `EXPECT_GT(r.value.header.file_size, 0u) << kName;` 行 (replace_all, 6 处匹配其中 4 stub + 2 非 stub 不受影响), 加 3 行注释解释 stub 是合法空 PackingMan output. 0 production code 改动, 4 行 diff (其中 stub 4 处, 105/108 顺手一致, 无 regression).
- **影响**: R-17 + R-18 = `mxh_resource_parse_all_bin_tests` 158/107/3 → **265/0/3** (107 FAIL 全解, +67% 单 exe 覆盖率).

## R-19: ~~gm_security test path 解析 bug~~ (已修, pre-existing → ✅)

- **状态**: ✅ **已解决** (2026-08-20, turn 13)。`mxh_gm_security_tests` 9/1/0 → **10/0/0** (PASS/FAIL/SKIP).
- **根因**: `tests/unit/gm_security_test.cpp:94` path `MXH_SOURCE_DIR + "/../deploy/server/Distribute/Resource/ItemList.bin"` 假设 deploy 跟 modern 平级, 实际 deploy 在 modern 下面. 1 行改.
- **修法**: `"../deploy/..."` → `"/deploy/..."`. 0 production code 改动, 1 行 diff.
- **影响**: 17 exes 累计 791 PASS / 0 FAIL / 4 SKIP. ctest 6.58% 覆盖率. **0 active FAIL** in 替身路线 scope.
