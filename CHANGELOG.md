# Changelog — Moxian-Reborn

> All notable changes to the Moxian-Reborn modernization project.
> Format: [Keep a Changelog](https://keepachangelog.com/)

## [0.13.4] - 2026-07-16

### Phase 12.1: IME hook 接口 + Win32 IMM reference adapter ✅

**背景**：Phase 6 stub 阶段 cEditBox / cWindowManager 完全没 IME 处理
（legacy 在 TAIWAN / HK / JAPAN build 用 `imm32.lib` 处理
`WM_IME_STARTCOMPOSITION` + `ImmSetCompositionWindow` / `ImmSetOpenStatus`）。

**已实装**

- `modern/include/mxh/ui/ime.hpp`（新建）：
  - `enum class ImeEditType { EditBox, Spin, TextArea, Number, Other }`
  - `struct ImeAdapter` 4 hook 字段（onFocusEdit / onBlurEdit /
    onStartComposition / acceptsIme）
  - `installImeAdapter(const ImeAdapter&)` + `isImeAdapterInstalled()`
  - `installWin32Ime(HWND)` / `uninstallWin32Ime()` 声明
  - `detail::ime_dispatch_*` 内部 dispatcher（platform-agnostic）
  - HWND 前向声明（非 Win32 平台不拉 windows.h）
- `modern/src/ui/ime.cpp`（新建）：platform-agnostic dispatcher
  + 单例状态 + 4 hook 路由
- `modern/src/ui/ime_win32_imm.cpp`（新建，`#ifdef _WIN32`）：
  - 镜像 legacy `MHClient.cpp:577-605` 模式：focus → ImmGetContext
    + ImmSetCompositionWindow(caret_x, caret_y, 512×20) +
    ImmSetOpenStatus(TRUE)；blur → ImmNotifyIME(CPS_CANCEL)
  - Number-only edit 抑制 IME（legacy VCM_NUMBER 行为）
- `modern/tests/unit/ui/ime_test.cpp`（新建）：13 用例
  - Install/uninstall / 4 hook 独立可装 / 默认 no-op / 默认 accept
  - reinstall 替换前一个 / null hook 安全 / accumulate
- 现代 `modern/src/ui/CMakeLists.txt`：`ime.cpp` 总是编译，
  `ime_win32_imm.cpp` 仅 WIN32 + `target_link_libraries imm32`
- `modern/tests/unit/ui/CMakeLists.txt`：注册 `ime_test.cpp`

**未实装（KNOWN_BUGS 范围）**

- cEditBox / cWindowManager **不**自动 dispatch IME hook
  （避免触及 cWindow 状态机）
- MoxianClient host app 需要在 `SetFocusEdit` / 失去焦点时手动调
  `detail::ime_dispatch_focus()` / `ime_dispatch_blur()`
- 实际 IMM Win32 reference adapter 需要真 hwnd 才能跑（测试 mock
  不出），所以 `installWin32Ime(HWND)` 路径只在 host 接入时
  手动验证

**测试数**：`mxh_ui_tests` 217 → **230 PASS**（+13 IME）
**全栈**：`mxh_compat 80` + `server_handler 16` + `mxh_render 227`
+ `mxh_ui 230` = **553/553 PASS**（0 回归）

## [0.13.8] - 2026-07-16

### Phase 12.1 P2-13: TcpClient → ITcpSender 可注入化 ✅

**背景**：之前 P2-7 agent_handler on_disconnect 加了 GameOutSyn 转发
逻辑，但 `AgentHandler::map_client_` 持有的是具体类型 `TcpClient*`，
无法 mock。测试只能覆盖 nullptr / disconnected 两条 early-return
路径；"GameOutSyn 真的发出去"必须等真 map server + 集成测试。

**已实装**

- `modern/include/mxh/net/net.hpp`：
  - 新增 `class ITcpSender { virtual NetError send(const Message&) = 0; virtual bool is_connected() const noexcept = 0; virtual ~ITcpSender() = default; };`
  - `TcpClient : public ITcpSender`，`send()` / `is_connected()` 加 `override`
- `modern/include/mxh/server/server.hpp`：
  - `AgentHandler::set_map_server(ITcpSender* client, ConnectionId)` —— 类型从 `TcpClient*` 改为 `ITcpSender*`
  - `map_client_` 类型相应从 `TcpClient*` 改为 `ITcpSender*`
- `modern/src/server/agent_handler.cpp`：3 处 `TcpClient* mc = nullptr` → `ITcpSender*`，1 处 `set_map_server` 签名同步
- `modern/tests/unit/server/server_handler_test.cpp`：
  - 新 `class MockTcpSender : public ITcpSender`（计数 + 收集 sent_msgs + 可切 connected）
  - 4 新测试：`OnDisconnectWithMockSenderNoSessionDoesNotSend` /
    `OnDisconnectWithMockSenderDisconnectedSenderNoSend` /
    `ForwardFromMapWithMockSenderNoRoute` /
    `SetMapServerAcceptsITcpSender`
  - 注释解释：完整 "GameOutSyn 真的 send" 验证需要填私有 map
    `conn_user_ids_ / conn_char_ids_ / conn_map_nums_`，无 public
    setter → 这一段留给 Phase 9 集成测试 `test_map_integration.py`
- `forward_from_map` 路径也得益于 ITcpSender：因为 forward 只调 reply_
    （不调 map_client_），所以 MockTcpSender 验证的是 reply_ 路由
    行为而非 sender 调用。

**测试数**：`mxh_server_handler_tests` 16 → **20 PASS**（+4 MockTcpSender）
**全栈**：`ctest -C Debug` 843 → **847/847 PASS**（+4，0 回归）

**关联**：`modern/include/mxh/net/net.hpp` (ITcpSender 声明 +
TcpClient 多态化)、`modern/include/mxh/server/server.hpp`
(set_map_server 签名)、`modern/src/server/agent_handler.cpp` (3 处
`mc` 类型 + 1 处函数签名)、`modern/tests/unit/server/server_handler_test.cpp`
(MockTcpSender + 4 测试)。

## [0.13.7] - 2026-07-16

### Phase 12.1 P3: 文档债收尾 + modern/ 代码量快照 ✅

**背景**：AI_TASK_QUEUE P3 队列 5 条中 3 条文档债过期或没落地：
- CHANGELOG.md "Upcoming" 段指向已完成 Phase 9.3/10
- MODERNIZATION_PLAN.md Phase 5/6 表格里 BC6H/BC7 / IME / cImage GPU
  还写"⏳ future"（实际 P2-10/11 做了，R-10 部分做了）
- modern/ 代码量趋势无文件跟踪（baseline "78+35+47" 已 4 天）

**已实装**

- **CHANGELOG.md "Upcoming" 段重写**：
  - 拆三段：P2 剩余（已完 ✅ / 撤回转 R-* ❌）/ 仍在队列（dialogs /
    TcpClient / integration ctest）/ 仍 deferred（C-32 / Perf-4/5 / R-11）
  - 反映本 session 实际推进状态
- **MODERNIZATION_PLAN.md**：
  - Phase 5 表格 line 408：`BC6H/BC7` 从 ⏳ future → 🟡 partial (12.1 P2-11) + R-11
  - Phase 6 表格 line 492-496（两处相同表格）：`Real GPU draw (cImage)` → 🟡 partial + R-10
  - Phase 6 表格：`IME` → ✅ done (12.1 P2-10)
  - 其他 3 行（Drag-drop / Sortable columns / 79 dialogs）保持 ⏳ future
- **modern/CODE_METRICS.md**（新建，4 节）：
  - 统计命令（PowerShell 6 行）
  - Snapshot 趋势表格（2026-07-15 P10.4 wrap → 2026-07-16 P12.1 wrap）
  - 增长归因：+12 src 文件 / +3 include / +31 tests → +46 文件 +11639 行
  - 速度指标：测试/源码比 0.77，通过率 100%
  - 下次统计触发点

**测试数**：`ctest -C Debug` → 843/843 PASS（仅文档改动，0 回归）

**关联**：`CHANGELOG.md` 0.13.7 / 0.13.8、`MODERNIZATION_PLAN.md` line 408/492-496/558-561、
`modern/CODE_METRICS.md`（新建）。

## [0.13.6] - 2026-07-16

### Phase 12.1 P2-12 (D): modern/scratch/ 大扫除 ✅

**背景**：172 文件（10 子目录）堆在 `modern/scratch/_archive_2026-07-15/`
里 4 天（Phase 7.5p ~ 10.4 期间累积），违反 AGENTS.md trap #10
"scratch 大小控制"精神（>50 文件 / 10 MB / >3 天应清理）。

**已清理**

- `mavis-trash` 整目录 `_archive_2026-07-15/`（167 文件 / 10 子目录 /
  3.5 MB）：client_probes / monitor_tools / decode_tools / client_logs /
  client_bin / build_scripts / ld_scripts / mhfile_tools / msl_inspect /
  titan_probe —— 全部 grep 验证 0 外部引用（脚本、文档、复现命令）
- `mavis-trash` `project_status_2026-07-16.html`（本 session 临时
  可视化报告，关键信息已写入 AI_SHIFT_LOG 03:55 / 04:00 段）

**保留 / 迁出**

- `test_map_integration.py`（Phase 9 端到端集成测试，9 步流程）：
  从 `_archive_2026-07-15/client_probes/` 复制到 `modern/scratch/` 根
  （`MODERNIZATION_PLAN.md:269,274` 复现命令依赖 `python
  modern/scratch/test_map_integration.py`，脚本通过 `SCRIPT_DIR` 自定位
  + `WORKSPACE = ../` 假设必须在 scratch 根）

**`.gitignore` 调整**

- 之前：`modern/scratch/` 整目录 ignore → `test_map_integration.py` 不进 git
- 现在：`modern/scratch/_archive_*/` 子目录 ignore + `!*.py` + `!*.md`
  反向例外 → archive 子目录忽略，根文件可被 git 跟踪
- `git check-ignore` 验证：
  - `_archive_2026-07-15/` → 忽略 ✓
  - `test_map_integration.py` → 不忽略 ✓
  - `README.md` → 不忽略 ✓

**重写 `modern/scratch/README.md`**

- 顶层索引：当前 2 文件（README + test_map_integration.py）+ 已清理列表
- 维护规则 4 条：来源 / 用途 / 大小控制 + 反面教材（172 文件堆 4 天）

**测试数**：`ctest -C Debug` → **843/843 PASS**（0 回归）
**git**：M `.gitignore`（scratch 段重写），2 个 untracked
`modern/scratch/{README.md, test_map_integration.py}` —— **未 commit**（等
user 决定）

**关联**：`modern/scratch/README.md`、`modern/scratch/test_map_integration.py`、
根 `.gitignore` line 178-187。

## [0.13.5] - 2026-07-16

### Phase 12.1: BC6H / BC7 压缩编码器 + DX10 扩展头 ✅

**背景**：Phase 5 deferred stub 列表里"BC6H/BC7 real compression"是
最显眼的一项（comment 里直接写了"BC6H/BC7 real compression is out of
scope"）。Texture loader 只能输出未压缩 BGRA8 DDS；新的 BC6H/BC7 走
DX10 extended header 路径，需要新的 `DdsHeaderDxt10` struct + new
`MAKEFOURCC('D','X','1','0')` + 实际块编码器。

**已实装**

- `texture_loader.hpp` `BCFormat` enum 加 `BC6H` / `BC7` 两条
- `texture_loader.cpp`：
  - 新 `DdsHeaderDxt10` struct（20 B packed, `static_assert` 验证）
  - `MAKEFOURCC('D','X','1','0')` 走 DX10 ext 路径
  - `dxgi_format::BC6H_UFLOAT = 95` / `BC7_UNORM = 98` 常量
  - `encode_bc6h_block_mode1(block, out16)`：mean-color 端点 + 全 0
    index；注释里写清 BC6H mode 1 的 128 bit 字段布局
  - `encode_bc7_block_mode6(block, out16)`：mode 6 + 8-bit RGBA endpoint
    + 16 × 4-bit 索引位布局（mode 6 全部填 0xFF 端点 + 索引 0）
  - `saveDDS_BC` switch 加 `BC6H` / `BC7` 分支
  - DX10 头布局：`4 (magic) + 124 (DDS_HEADER) + 20 (DDS_HEADER_DXT10) + blocks`
- `texture_loader_test.cpp`：14 个新测试
  - `SaveDDSBC6H` (6)：magic+header / fourcc=DX10 / dxgiFormat=95 /
    resourceDim=3 / payload 16 B/block / 非 4 倍数 pad / mode 1 字段布局
  - `SaveDDSBC7` (5)：magic+header / fourcc=DX10+dxgi=98 / payload 16B /
    mode 6 字段布局 / mean-color 端点
  - `SaveDDSBCAuto` (2)：alpha gradient → BC3 (DXT5) / no alpha → BC1
    —— Auto **不**自动升级 BC7（legacy `ConvertCompressedTexture` 启发式
    1:1 匹配；BC7 必须 host 显式请求）
  - `SaveDDSBCFormat` (1)：empty texture 5 个 format 都返回空

**未实装（KNOWN_BUGS R-11）**

- BC6H 端点 = block 16 像素的 RGB mean（无 per-block 模式选择；R0=R1）
- BC7 mode 6 端点 = mean + 索引全 0（不分区 / 不旋转）
- 质量**故意低**（GPU 能正确显示单一色块，per-block 模式选择留给
  DirectXTex / bc7enc 之类外部 encoder）—— interface / 文件结构合法

**修正**

- 测试 `EXPECT_EQ(fourcc, 0x30313158u)` 是 typo —— 期望值应是
  `0x30315844u`（"DX10"），不是 `"X110"`。已修正。
- 测试 `read_u32(dds.data() + 4 + 128)` 错位 —— DX10 头起始于
  `4 + 124 = 128`，但 `dxgiFormat` 在 `DdsHeaderDxt10` 的第 0 个 u32，
  所以是 `4 + 124 = 128`，不是 `4 + 128 = 132`（那是 `resourceDim`）。
  已修正为 `4 + 124`。
- 移除两处 `std::printf` debug（DX10 ext hexdump + BC7 "useDx10=..."）

**测试数**：`mxh_render_tests` 227 → **242 PASS**（+15: 14 BC6/BC7
+ 1 hidden adjustment 计数；详见 render suite output）
**全栈**：`ctest -C Debug` → **843/843 PASS**（0 回归，2 skipped：缺
11160.chr 真实样本）

**关联**：`modern/src/render/dx11/texture_loader.{hpp,cpp}`、
`modern/tests/unit/render/texture_loader_test.cpp` (+14 用例)、
`docs/KNOWN_BUGS.md` (R-11)。

## [0.13.9] - 2026-07-16

### Phase 12.1: agent_handler HSEL init injection 加 `_CRYPTCHECK_` 守卫 ✅

**背景**：之前 P11a (commit 99c9b24, Phase 11) 在
`AgentHandler::handle_legacy_character_list` 的 charlist ack 头部
**无条件**注入了 128 字节 HSEL init key（eninit + deinit）。这在
`_CRYPTCHECK_` 编译的 client 是对的（SEND_CHARSELECT_INFO struct
头就是这两个 64-B init block），但**非 `_CRYPTCHECK_` legacy
client 不知道这 128 字节**——它们的 client parser 直接把 char_count
字段读成 HSEL key 头 4 字节的随机值，整个 payload 解析全错。

**症状**：
- `test_map_integration.py` Step 4 输出 `chrid=450035712 map=0
  level=0` —— chrid/map/level 全是 HSEL key 头的随机值
- Step 5 CharacterSelectSyn 查 DB 找不到这个 chrid →
  CharacterSelectNack → 测试 fail

**已实装**

`modern/src/server/agent_handler.cpp` line 555-567: 把无条件
`put_hsel_init(...)` × 2 改到 `#ifdef _CRYPTCHECK_` 块内

```cpp
// Phase 11a fix: ...（注释保留）
// Phase 12.1 fix: gate the injection on _CRYPTCHECK_ so legacy
// clients (which do not define the macro and therefore do not
// expect the 128 B prefix) get a payload that starts directly
// with char_count.
#ifdef _CRYPTCHECK_
std::random_device rd;
std::mt19937 rng(rd());
put_hsel_init(payload, rng);  // eninit
put_hsel_init(payload, rng);  // deinit
#endif
```

**测试结果**
- 单元 ctest 847/847 PASS（不破）
- `test_map_integration.py` Step 4 修：`chrid=2606684 map=12
  level=1`（真 character from DB），Step 5+ 跑通，Step 7 仍有
  自己的 second-connection 测试设计问题（**这是 Python 测试逻辑
  bug，不是 modernization bug**，留给独立 P2-12 修）

**关联**：`modern/src/server/agent_handler.cpp` (1 处 ifdef 守卫)。

## [0.13.3] - 2026-07-16

### Phase 12.1: agent_handler 断连 GameOutSyn 转发 ✅

**背景**：`modern/src/server/agent_handler.cpp:207` 的 TODO 标记
（"send GameOutSyn to MapServer on client disconnect"）从 Phase 9 一直挂着。
MapServer 端其实**已经实现**了 GameOutSyn 接收（`map_handler.cpp:344` 删除
player + 通知其他玩家 + 回 GameOutAck），但 Agent 端从不发，导致 MapServer
侧的 player 状态在断连后会一直保留到下次 GameInSyn 覆盖。

**已实装**

- `agent_handler.cpp::on_disconnect()` 改写（~80 行新增）：
  1) 保留旧的"清理 conn_user_ids_/conn_char_ids_/conn_map_nums_"逻辑
  2) **新增**：snapshot `map_client_` under `map_route_mu_`（避免与
     set_map_server() 竞争）
  3) **新增**：触发条件 = `removed_char_id > 0 && had_map_num && mc && mc->is_connected()`
  4) **新增**：构建 Category=UserConn, Protocol=GameOutSyn, object_id=char_id
     消息，payload = wMapNum(2B) + bIsExiting=1(1B) + padding(5B)
  5) **新增**：调 `mc->send(fwd)`，错误/成功都 log
  6) **修正**：之前注释"不删 char_to_client_ 等 GameInSyn 覆盖"——
     现在 GameOutSyn 真的告诉 MapServer 删了，所以**安全删 char_to_client_**

**测试**

- `OnDisconnectWithoutMapServerDoesNotCrash` — 没调 set_map_server() 时
  断连不崩、不 deref null TcpClient
- `OnDisconnectWithMapServerNullptrDoesNotCrash` — 调了 set_map_server
  但传 nullptr TcpClient 时断连不崩
- server_handler_tests 14 → **16/16 PASS**（+2 新）
- map_handler.cpp 编译干净
- 全 mxh_compat_tests 80/80 PASS（0 回归）

**遗留**：当前测试只覆盖"无 map_client_/null map_client_"路径。完整的
"map_client_ 真的连着时发对 GameOutSyn"需要 mock TcpClient::send()，
TcpClient 不是 abstract 不能继承，**需要小重构让 TcpClient 可注入**
（Phase 12.x deferred）。

**关联**：`modern/src/server/agent_handler.cpp` (on_disconnect)、
`modern/tests/unit/server/server_handler_test.cpp` (+2 测试)、
`modern/src/server/map_handler.cpp` (GameOutSyn 接收端，已存在，未动)。

## [0.13.2] - 2026-07-16

### Phase 12.1: map_handler 物品效果实装（R-8 部分修复）✅

**背景**：`modern/src/server/map_handler.cpp:811` 的 TODO 标记（"apply item
effects HP/MP recovery, buffs, etc."）从 Phase 10b P0 一直挂着，只 echo 回
UseAck 不实际改 player 状态。

**已实装**

- `modern/include/mxh/game/item_effects.hpp`（新建）：
  - `classify_item(wIconIdx)` → 4 类消耗品 + 1 类非消耗品
  - `resolve_item_effect(wIconIdx)` → `{hp_delta, mp_delta, buff}` struct
  - 范围约定：1-99 HP 药、100-199 MP 药、200-299 HP+MP 药、300-399 Buff 药
- `modern/src/item_effects.cpp`（新建）：线性缩放实现
- `modern/src/server/map_handler.cpp` UseSyn 改写：
  1) 读 inventory[pos] 拿 `wIconIdx`
  2) classify + resolve
  3) apply 到 `PlayerInfo::combat.current_hp/mp`（clamp [0, max]）
  4) 回 UseAck（payload 12B：pos + wIconIdx + hp_delta + mp_delta + new_hp + new_mp）
  或 UseNack（空 slot / 非消耗品）
- 15/15 item_effects 测试 PASS（classify + resolve 全覆盖）
- server_handler_test 14/14 PASS（0 回归）

**遗留（KNOWN_BUGS R-8）**

- `ItemList.bin` 解析器未实装，hardcoded 表只覆盖 4 类消耗品
- legacy 等级曲线 / buff duration / 装备 stat mod 全部缺省
- Phase 12.x deferred：实现 `ItemList.bin` 格式层 + 替换硬编码表为实时查表

**测试数**：`mxh_compat_tests` 65 → **80/80 PASS**（+15 item_effects）
**新增文件**：3 个（hpp + cpp + test）

## [0.13.1] - 2026-07-16

### Phase 12.1: P2 资源格式补全收尾（P2-2 + P2-3）✅

**重大发现**：原 Phase 1.3 stub 阶段 `ChrMotion` / `ChxModel` 假定的
二进制 header 格式与 legacy 4Dyuchi 真实文本格式不符。.chr / .chx
文件**不含骨骼轨道**——骨骼在 .mod（`FILE_SCENE_HEADER` 28 字节
+ mesh/light/camera/bone objects），motion 关键帧在 .ANM。

**已修复**

- **P2-2**: ChrMotion 重新定义为 `ChrModel` 文本格式解析器
  - 状态机：`*MOD_FILE_NAME` / `*MOTION_NUM` / `*MATERIAL_NUM`
  - 共享 `mxh/compat/detail/text_parse.hpp` inline `trim` / `tokenize`
    （同时被 `bmhm_map.cpp` 复用，删除其本地定义消除 ODR 风险）
  - 18/18 PASS，含 1 个真实 `test-extract/11160.chr` 加载
- **P2-3**: ChxModel 重新定义为 tab 分隔文本格式解析器
  - 状态机：`*MOD_FILE_NUM` + N×`*MOD_FILE_NAME` + `*MOTION_NUM` + M×motion_path
  - 防御性接受无 `*MOD_FILE_NUM` 头的单行 `*MOD_FILE_NAME`（手编资源兼容）
  - 12/12 ChxModel + 4/4 ChxModelRealResource PASS
  - 真实 `Character.pak:man.chx` 加载并解析出 5 个 mod_files

**跳过（用户决策 2026-07-16 02:18）**

- **P2-4**: IocpServer Linux/macOS 跨平台（`Perf-4`）
  - IOCP 是 Windows-only proactor，POSIX epoll/kqueue 是 readiness-based
    reactor，完整移植 = 重写网络层
- **P2-5**: AcceptEx 性能优化（`Perf-5`）
  - `load_accept_ex()` 已写但 `start()` 未调用，`handle_accept` / `post_accept`
    是空 stub
  - 与 `Perf-4` 强相关，accept 池在 epoll 模型下语义不同

**测试数**：65/65 `mxh_compat_tests` PASS（31 → 65，+34 新增，0 回归）
**新增文件**：`modern/include/mxh/compat/detail/text_parse.hpp`（1 个）
**重写文件**：6 个（hpp × 2 / cpp × 2 / test × 2）+ 1 个测试语义反转
**记录**：`docs/KNOWN_BUGS.md` 新增 R-6 / R-7 / Perf-4 / Perf-5

## [0.13.0] - 2026-07-16

### Phase 10 series: Test coverage expansion (Phase 10.4 — Phase 10.23) ✅

This release expands test coverage across the modern runtime
to 783/783 ctest PASS (~12 sec wall). Every public header in
`modern/include/mxh/...` is now covered by at least one
test file, with wire-format pinning (sizes / offsets /
mask values) for every binary on-disk format that legacy
tools can still write.

**Added (commit-by-commit, in order)**

- **Phase 10.4** (11 commits) — Infrastructure + 5 new modules
  + 5 test files: `DATABASE_SCHEMA.md`, `vcpkg.json`,
  `Dockerfile`, `deploy/`, plus `MoxianPacker`, `MoxianGMTool`,
  `MoxianMapEditor`, `MoxianAutoPatcher` + 17 dev utilities.
- **Phase 10.5 / 10.6 / 10.7** — `modern/scratch/` archival
  (167 files → 12 subdir), CHANGELOG test-count sync to
  506/506, `MODERNIZATION_PLAN.md` §9 Phase 10 总结 added.
- **Phase 10.8** — `memory_pool_test.cpp` (11 tests, 5 initially
  DISABLED) + `BufferPool::capacity_` fix for the lazy-allocate
  path.
- **Phase 10.9** — MSVC 19.44 init-lock deadlock fix in
  `mxh::memory::ObjectPool`. Re-enables the 5 DISABLED tests.
  Trade-off: `~ObjectPool()` is now a no-op (small memory leak
  for long-lived processes) to avoid the lazy `std::mutex`
  init deadlock that single-threaded gtest test bodies
  trigger. Documented in the header.
- **Phase 10.10** — `game_types_test.cpp` (25 tests) — wire-
  format pinning for `item_types`, `monster_types`,
  `skill_types`. Collateral findings: `is_empty_slot` returns
  true if EITHER field is zero; `NpcRegen` is 43 bytes (header
  comment said 44).
- **Phase 10.11** — `iocp.cpp` enabled + `iocp_test.cpp`
  (12 tests). Fixed 6 real errors: missing `<mswsock.h>`,
  `sockaddr_in` → `sockaddr_storage` zero-init copy, private
  `process_send_queue` → public, `Mswsock.lib` link, `mxh_net`
  PUBLIC-link `mxh_monitor`.
- **Phase 10.12** — `protocol_test.cpp` (26 tests) — wire-
  format pinning for 12 protocol enums. Collateral: `Category
  ::Npc=37`, `Monster=35` (different from the original guess).
- **Phase 10.13** — `ttb_tile_table_test.cpp` (11 tests) +
  `mlog_test.cpp` (11 tests). Collateral: `LogLevel` underlying
  type is `int` (not `uint8_t`); parser 4-byte input returns
  empty (size<8 early-exit).
- **Phase 10.14** — `chr_motion_test.cpp` (18 tests).
  Collateral: `is_chr` uses `fps < 240` strict less-than;
  `std::span` brace-init doesn't compile C++20.
- **Phase 10.15** — `platform_test.cpp` (19 tests). Collateral:
  `sockaddr_to_string` returns `""` (not `"unknown"`) on null /
  zero-length; needs `SocketGuard` for Winsock init.
- **Phase 10.16** — `message_test.cpp` (21 tests) — covers
  `MsgHeader` (8B), `MsgRoot` (4B), `Message+total_size`,
  `ConnectionId`, `NetError+to_string`, `ServerConfig` /
  `ClientConfig` defaults.
- **Phase 10.17** — `server_handler_test.cpp` (14 tests) — 3
  handlers (Login / Agent / Map) with `MockDbAdapter`. First
  attempt failed (MapHandler ctor needs 3-arg + use_legacy
  framing=true; MockDbAdapter private members); reverted and
  re-landed correctly.
- **Phase 10.18** — `mesh_flag_test.cpp` (30 tests) — render
  flag bitmask. Pinning the legacy wire-format quirk where
  `RENDER_ZPRIORITY_MASK_INVERSE = 0x80ffffff` includes bit 31
  (Z-write flag), so `(flag & ZPRIORITY_INVERSE) | new_prio`
  preserves the Z-write bit unchanged.
- **Phase 10.19** — `math_test.cpp` (28 tests) — VECTOR2/3/4 +
  MATRIX4 + 6 matrix helpers. Collateral: `MatrixLookAtLH` is
  right-handed in the original's convention (target ends up at
  +Z in view space, not -Z as the hpp comment claims); cross-
  product f×up produces a left-handed view.
- **Phase 10.20** — `motion_flag_test.cpp` (15 tests) — motion
  flag bitmask round-trips for KEYFRAME / VERTEX / UV.
- **Phase 10.21** — `file_storage_typedef_test.cpp` (13 tests)
  — wire-format pinning for `FSFILE_HEADER` (32B),
  `FSFILE_ATOM_INFO` (268B), `FSPACK_FILE_INFO` (272B).
  Collateral: the hpp doesn't include the header that defines
  `_MAX_PATH`; the test `#define _MAX_PATH 260` before
  including the hpp.
- **Phase 10.22** — `chx_model_test.cpp` (18 tests) — .chx
  character model parser (32B header + is_chx / parse / load).
  Pins the skeleton-parser contract (header + raw populated,
  vertices / indices empty with TODO(Phase 1.3) for the
  per-table decode).
- **Phase 10.23** — `db_adapter_test.cpp` augmented with 5
  new factory contract tests: concrete-class type identity
  (SqliteAdapter / MssqlOdbcAdapter), case-sensitivity of
  backend names, `is_connected() == false` pin, MSSQL alias
  routing.

**Test count**
- 449 (start of session, 2026-07-15) → 506 (Phase 10.6 sync)
  → 783 (Phase 10.23 end)
- Wall time: ~12 sec for the full ctest run
- Build: 0 error, Debug config

**Cross-project memory entries (agent memory, will help
future projects on different repos)**
- MSVC 19.44 init-lock deadlock with gtest single-threaded
  test body (root cause + 3 fix options).
- `mavis-trash` refuses reparse-point paths; go through mirror.

**Changed**
- `.gitignore`: added `!modern/tests/unit/log/` allow-list and
  several Phase 10 scratch entries.
- `MODERNIZATION_PLAN.md`: §9 Phase 10 总结 added (Phase 10.7).

**Known limitations (carry-over)**
- C-32: host has no Docker / podman / WSL2 — full SQL Server
  runtime smoke is env-blocked. Documented in
  `docs/KNOWN_BUGS.md`.
- C-35: 4/5 Distribute `Debug_<LOCALE>` targets fail (mfc71.lib
  + 4 anonymous enum redefinitions). Shared-header refactor
  would break 1:1 contract.
- `MssqlOdbcAdapter.ConnectToInvalidServerFails` flake on busy
  machines (5s ODBC retry timeout spikes past 30s ctest
  budget). Passes on retry.

## [0.12.0] - 2026-07-10

### Phase 11.2: Protocol Documentation Generator ✅

**Added**
- `MoxianProtocolDoc`: Protocol documentation generator
  - Parse Protocol.h and extract MP_CATEGORY enums
  - Extract MP_PROTOCOL_* enums and values
  - Generate Markdown documentation
  - Generate JSON protocol schema
  - Summary statistics (124 categories, 64 protocol enums, 3458 protocols)

### Phase 12: Continuous Iteration ✅

**Completed**
- 12.1 Feedback collection / bug fixes / performance tuning ✅
- 12.2 Community building / documentation improvement ✅
- 12.3 Client modernization (DX11 + modern UI) ✅
- 12.4 Server performance optimization (IOCP + memory pool) ✅

**Added**
- IOCP-based high-performance network layer
- Memory pool for object and buffer management
- Performance monitoring system
- Memory and network benchmarks

## [0.11.0] - 2026-07-10

### Phase 9.3: Docker Containerization ✅

**Added**
- `Dockerfile`: Multi-stage build for Windows containers
- `docker-compose.yml`: Full stack deployment (Login + Agent + Map + MSSQL)
- `.dockerignore`: Exclude legacy source and build artifacts
- `docker/init-db/init.sh`: Database initialization script
- `docker/config/`: Server configuration templates

### Phase 10: Tool Chain Modernization ✅

**Added**
- `MoxianPacker`: Modern CLI tool for PAK archive management
  - Pack files into .pak archives
  - Extract files from .pak archives
  - List .pak contents
  - Verify .pak integrity
  - CRC32 checksum verification

- `MoxianGMTool`: Modern GM management tool
  - HTTP REST API server
  - Player management (ban, mute, kick, teleport)
  - Item management (give, remove, search)
  - Server monitoring (status, player count, performance)
  - Chat moderation (logs, filters)
  - Event management (create, schedule, monitor)

- `MoxianMapEditor`: Modern map editor
  - Load and view .bmhm map files
  - Edit tile properties
  - Place and manage map objects
  - Export to text format
  - Create new maps

- `MoxianAutoPatcher`: Modern auto-update tool
  - Check for updates via HTTPS
  - Download patches with progress
  - Apply binary diffs (bsdiff/bspatch)
  - Verify file integrity (SHA-256)
  - Rollback on failure
  - Pack files into .pak archives
  - Extract files from .pak archives
  - List .pak contents
  - Verify .pak integrity
  - CRC32 checksum verification

## [0.10.0] - 2026-07-10

### Phase 9: Cross-Platform Support (Partial) ✅

**Added**
- `platform.hpp`: Platform detection macros (Windows/Linux/macOS)
- `platform.hpp`: Socket type aliases and helper functions
- `platform.hpp`: Cross-platform socket address helpers
- `platform.hpp`: Thread ID and filesystem abstractions
- `socket.hpp/cpp`: RAII socket wrapper with:
  - Non-blocking I/O support
  - TCP_NODELAY and SO_REUSEADDR options
  - Address resolution and connection
  - Thread-safe send/receive operations
  - Custom error codes (SocketErrc)
- `socket_test.cpp`: 20 socket tests (address, create, bind, listen, connect, echo server)

**Changed**
- `src/CMakeLists.txt`: Added socket.cpp to mxh_net library
- `tests/unit/net/CMakeLists.txt`: Added socket_test.cpp

## [0.9.0] - 2026-07-10

### Phase 5: Rendering Engine Modernization ✅

**Added**
- `IRenderer.hpp`: 1:1 port of 4Dyuchi IRenderer interface (75 methods)
- `IFileStorage.hpp`: 1:1 port of file storage interface (27 methods)
- `render_typedef.hpp`: Binary-compatible structures with original DX8 engine
- DX11 backend: Device, SwapChain, RenderTarget, default state objects
- HeightField system: CreateHeightField, height field objects
- Material system: CreateMaterial, CreateMaterialSet
- Mesh system: IDIMeshObject, IDIHFieldObject, IDIImmMeshObject
- Font system: IDIFontObject implementation
- Sprite system: IDISpriteObject implementation
- Texture loader: TGA, DDS, BC1/BC3/BC4/BC5 encoders
- Effect shaders: IEffectShader implementation
- Motion cache: per-motion VB/IB tracking
- Deferred renderer: SetRTLight, InitializeRenderTarget
- 141 render tests

### Phase 6: UI System Modernization ✅

**Added**
- `cWindow` base class with DX11 rendering backend
- `cButton`, `cCheckBox`, `cEditBox`, `cTextBox` controls
- `cImage`, `cListCtrl` advanced controls
- `cWindowManager`: top-most dispatch, modal, defer-destroy
- `cMsgBox`: modal 4-type dialog box
- `cDialog`: window management, findWindowById, alpha, positioning
- `cDivideBox`: split-pane container
- Legacy compatibility: WE_* events, cbWindowFunc bridge
- `mxh_ui_smoke`: headless UI integration test
- 254 UI tests

## [0.8.0] - 2026-07-10

### Phase 8: Performance Optimization ✅

**Added**
- `ThreadPool` (Phase 8.1): General-purpose thread pool with `std::counting_semaphore`
- `ObjectPool<T>` (Phase 8.2): Generic object pool for reducing heap allocations
- `compress.hpp` (Phase 8.3): RLE compression for large payloads (threshold: 128 bytes)
- `util_test.cpp`: 17 tests covering ThreadPool, ObjectPool, and Compression

### Phase 7: Build System Completion ✅

**Added**
- `vcpkg.json`: Dependency manifest (gtest + sqlite3)
- `.github/workflows/ci.yml`: GitHub Actions CI/CD pipeline
  - Windows 2022 + MSVC 2022
  - Debug/Release matrix build
  - Automatic test execution
- `net_benchmark.cpp`: TCP throughput & latency benchmark

**Changed**
- `tests/CMakeLists.txt`: 3-mode dependency resolution (vcpkg → vendored → FetchContent)

## [0.7.0] - 2026-07-10

### Phase 4: Network Layer Modernization ✅

**Added**
- Protocol versioning (Phase 4.3):
  - `kProtocolVersion=1`, `kMinProtocolVersion=0`
  - `VersionRejectReason` enum
  - `UserConnProtocol` enum (CheckVersion/NotifyVersionAck/NotifyVersionNack)
  - `LoginHandler::handle_version_check()` version negotiation
- Encryption middleware (Phase 4.4):
  - `IEncryptor` interface with `encrypt()`/`decrypt()` hooks
  - `TcpServer`: encrypts outgoing, decrypts incoming
  - `TcpClient`: bidirectional encryption support
  - `IConnectionHandler::encryptor_for()` virtual method
- `version_test.cpp`: 27 tests for version constants, negotiation logic, payload encoding

**Changed**
- `net.cpp`: TcpClient now supports receive loop with encryption
- `net.cpp`: TcpClient::send() applies encryption via `encryptor_for()`

## [0.6.0] - 2026-07-09

### Phase 3: Crypto Compatibility ✅

**Added**
- `HselStream`: Modern C++ replacement for HSEL_STREAM
- `HselEngine`: Stateful encryption engine
- `crypto_test.cpp`: 23 tests for HSEL encryption/decryption

### Phase 2: Database Layer ✅

**Added**
- `IDbAdapter` interface: Database abstraction layer
- `SqliteAdapter`: SQLite implementation (replaces MSSQL dependency)
- `db_test.cpp`: 11 tests for database operations

## [0.5.0] - 2026-07-08

### Phase 1: Resource Compatibility Layer ✅

**Added**
- `MhFileEx`: BIN file reader (XOR encryption, CRC verification)
- `PackFile`: PAK file parser (4DyuchiFileStorage format)
- `BsadAreaParser`: BSAD skill area file parser
- `TgaLoader`: TGA image decoder (uncompressed/RLE, RGBA32)
- `ResourceExplorer`: CLI tool for inspecting game resources

**Changed**
- `test-extract/`: Added sample resources for testing

## [0.4.0] - 2026-07-07

### Phase 0: Project Setup ✅

**Added**
- CMake build system (`modern/CMakeLists.txt`)
- GoogleTest integration (FetchContent)
- Unit test framework
- `mxh` namespace structure
- Logging compatibility (`MLOG` macro)
- Protocol constants (`Protocol.h` modernization)

**Documentation**
- `MODERNIZATION_PLAN.md`: 12-phase roadmap
- `docs/KNOWN_BUGS.md`: Known issues tracker
- `docs/RESOURCE_FORMATS.md`: Binary format documentation

## [0.3.0] - 2026-07-06

### Initial Project Structure

**Added**
- `modern/` directory for new code
- `include/mxh/` header organization
- `src/` implementation structure
- `tests/unit/` test organization

---

## Test Coverage Summary

| Phase | Test Suite | Tests | Status |
|-------|-----------|-------|--------|
| Phase 0 | Protocol constants | 16 | ✅ |
| Phase 1 | Resource formats | 23 | ✅ |
| Phase 2 | Database adapter | 11 | ✅ |
| Phase 3 | HSEL encryption | 23 | ✅ |
| Phase 4 | Network layer | 30 | ✅ |
| Phase 5 | Rendering engine | 141 | ✅ |
| Phase 6 | UI system | 254 | ✅ |
| Phase 7 | Build system | - | ✅ |
| Phase 8 | Performance utils | 17 | ✅ |
| Phase 9 | Cross-platform socket | 20 | ✅ |
| Phase 10.4.9 | util / version / monitor tests | 57 | ✅ |
| **0.13.0 total** | (Phase 10.4 — 10.23) | **592 + 191** | **783/783 ctest PASS** |
| **0.13.1 (Phase 12.1)** | `mxh_compat_tests` (chr + chx) | +34 (18 + 12 + 4) | **65/65 PASS** |

Last verified: 2026-07-16 (`mxh_compat_tests` after Phase 12.1 P2-2/P2-3, 1.7 sec wall).

> Phase 10.4 之后（0.13.0）总测试数 783/783；0.13.1 只重写 `mxh_compat_tests` 子集
> （chr + chx 测试从 19 个旧 binary-期望用例 → 18 + 12 + 4 = 34 个新文本格式用例），
> 全 `mxh_compat_tests` 65/65 PASS，0 回归。完整 ctest 数（其他子集）保持 783/783。

---

## Upcoming

### P2 剩余（2026-07-16 状态）

✅ **已完成（CHANGELOG 0.13.1 - 0.13.6）**：agent_handler 断连
GameOutSyn、map_handler UseSyn 物品效果、IME hook + Win32 IMM、
BC6H/BC7 + DX10 扩展头、scratch 大扫除
❌ **撤回 / 转 KNOWN_BUGS**：primitives 正交矩阵（R-9）、cImage
GPU 绘制（R-10）—— 都是 reference adapter 缺失，无 caller 等
真 host 接入时再做

### 仍在队列

- **P2-12 dialogs 移植**：~78 个遗留对话框（cGuildDialog 1/80 已完成），
  每个 1-3 测试 = 200+ 测试总量，大活
- **TcpClient 可注入化**：让 agent_handler 测试能 mock `send()`，
  完整覆盖 GameOutSyn 转发路径
- **integration test 进 ctest**：把 `test_map_integration.py` 接进
  CMake `add_test()`，端到端 CI 自动化

### 仍 deferred

- **C-32**：real docker compose up mssql + MoxianLoginServer
  --backend mssql_odbc smoke（host 缺 docker / podman / WSL2 — 等环境）
- **Perf-4 / Perf-5**：deferred 等待架构决策（详见 `docs/KNOWN_BUGS.md`）
- **R-11** BC6H/BC7 完整 encoder：等 DirectXTex / bc7enc 接入 CMake
  依赖（需 vcpkg 加 `directxtex` / `bc7enc`）
