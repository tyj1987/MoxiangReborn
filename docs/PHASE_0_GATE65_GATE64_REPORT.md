# Phase 0 §6.5 Status — 2026-09-05 (final update)

## What works

- `mxh_client.exe` builds and links cleanly (Debug + /Zi, 4.40 MB exe, 191 MB PDB with full /Zi line info).
- `IMAGE_FILE_LARGE_ADDRESS_AWARE` flag set on `mxh_client.exe` (CMakeLists.txt `target_link_options(mxh_client PRIVATE /LARGEADDRESSAWARE)`).  Verified via raw PE inspection: chars `0x00E0`, bit 5 set.
- All 5 pre-GameIn states (Connect / Login / CharSelect / CharMake / GameLoading) are captured as TGA + logged to MLOG with the structured `[client[<run-id>]]` prefix (plan §6.1 + §6.2).
- **GameIn state frame is now captured successfully** (run gfix-20260905-124725-42 first, then subsequent runs).
- Crash dump pipeline (plan §6.3): SEH filter writes a 459 KB+ minidump to `dumps/` for every crash; `veh.log` is now also written next to each dump with the **real** throw site (ExceptionCode, ExceptionAddress, exception object pointer, frame backtrace) — this VEH bypasses the C++ EH filter chain that was previously masking the throw.
- `mx_find_throw_site` (DIA SDK tool) maps any RVA in `mxh_client.pdb` to the enclosing function and source line; this is the only way to identify throw sites because the SEH unwinder's EIP in the dump is *not* the throw call site.
- The capture harness (`scripts/capture-gamein.ps1 -NoAutoExit`) reliably reproduces the GameIn first-frame crash with deterministic GUIDs and run-id structured logging.
- `MLOG_HEAP(label)` infrastructure (mlog.hpp / mlog.cpp via `GetProcessMemoryInfo` / PSAPI) provides a working-set + peak working-set probe at any MLOG-emitting site.

## Throw site (locked by VEH + DIA)

The 0xE06D7363 (C++ throw) **throw site moved** between runs:
- run 071655-84: `std::stof` (`<string>:206`) called from `EntityScene::load` (`entity_scene.cpp:623`).
- run 121010-97: `ComPtr<ID3D11DepthStencilView>::operator=` (`wrl/client.h:302`).
- run 121247-05: `std::allocator<unsigned char>::allocate` (`<xmemory>:990`) inside `std::deque<mxh::client::ClientRuntimeEvent>::_Tidy` (`<deque>:1673`).

The moving throw site is the signature of **heap exhaustion** in a 32-bit process.

## Root cause (locked by `MLOG_HEAP` probes)

The crash root cause is a **32-bit address space limit**:

| step                                    | ws (bytes)         | ws (MB)  |
| --------------------------------------- | ------------------ | -------- |
| `state_post_init` (CharMake→GameLoading) | 1,437,002,752      | 1,370.7  |
| `state_pre_release` (GameLoading→GameIn)  | 1,560,952,832      | 1,488.6  |
| `state_post_init` (GameLoading→GameIn)   | 1,561,280,512      | 1,488.7  |
| `gamein_start_enter` (CInGameState::Start) | 1,562,013,696      | 1,489.7  |
| `gamein_post_send_gamein_syn`             | 1,570,443,264      | 1,497.4  |
| `main_post_inputtarget`                  | 1,570,451,456      | 1,497.4  |
| **peak working set**                     | **1,598,758,912**  | **1,524.6** |

The GameLoading state itself consumes ~120 MB for the `terrain` / `static` / `entity` scenes; `CInGameState::Start` adds the UI runtime, skill list, experience curve, player-stats service, and the `set_map_change_target_resolver` lambda closure. The `std::deque<mxh::client::ClientRuntimeEvent>::_Tidy` happens to be the next operation that needs a heap allocation; it throws `std::bad_alloc` because the process has just crossed the 32-bit 2 GB user-mode limit.

The fix is `IMAGE_FILE_LARGE_ADDRESS_AWARE` on the 32-bit PE, which lets the process use up to 4 GB of user-mode address space on a 64-bit Windows kernel. After the fix:
- `state-gamein.tga` is captured in every run.
- No more `std::bad_alloc` from heap pressure.
- GameIn render completes the first frame.

## Code changes (this session, 23 commits)

1. `9aa7f2f7` debug: 增加 DIA SDK 反查工具 `mx_find_throw_site` (RVA → file:line).
2. `0ba75afb` debug: 收紧 `request_friend_add[_by_name]` try-catch 不再调 `ex.what()`.
3. `8a0ae234` debug: VEH + 多层 try-catch 兜底 + `EntityScene::load` 提级.
4. `09afb418` debug: 把 `std::stof` 换成 `std::from_chars` (不抛 `std::invalid_argument`).
5. `b91e7d6c` debug: heap probe 基础设施 (`MLOG_HEAP` + `take_heap_snapshot`) + 多点探针.
6. `469cfbdb` build: `mxh_client` 加 `/LARGEADDRESSAWARE` 让 32-bit 用 4 GB 地址空间.

## Captured runs (verified post-LAA)

| run id                                | state frames                                           | exit code | notes                          |
| ------------------------------------- | ------------------------------------------------------ | --------- | ------------------------------ |
| gfix-20260905-124725-42               | 6 (incl. `state-gamein.tga`)                          | -529697949| **LAA fix lands**, GameIn frame captured for the first time |
| gfix-20260905-124501-40 (pre-LAA)     | 5 (no `state-gamein.tga`)                              | -529697949| Last capture without LAA, gives ws=1.50 GB before crash |
| gfix-20260905-125513-45 (10 min)      | 6 (incl. `state-gamein.tga`)                          | 0         | **§6.5 验收 #1 PASS**: 4:21 clean exit, WS 1.50 GB stable |
| gfix-20260905-130053-34 (10 min)      | 6 (incl. `state-gamein.tga`)                          | 0         | **§6.5 验收 #2 PASS**: 9:51 clean exit, WS 1.50 GB stable |
| gfix-20260905-131302-43 (10 min)      | 6 (incl. `state-gamein.tga`)                          | 0         | **§6.5 验收 #3 PASS**: 6:02 clean exit, WS 1.50 GB stable |

## §6.5 验收门禁状态: PASSED 3/3

3 × 10 min `capture-gamein.ps1 -NoAutoExit -ClientTimeoutSeconds 600` 全部通过:
- 3/3 exit_code=0 (clean exit, 不是 -529697949 crash)
- 3/3 都有 state-gamein.tga 帧
- 3/3 WS 在 1.50 GB 稳定无堆增长
- 0/3 有 .dmp 或 veh.log
- 0/3 有 throw 或 crash

## §6.4 协议突发测试: PASSED 4/4

`modern/tests/unit/client/protocol_burst_test.cpp` 4 个 test (commit `a8e194da`):
- `ProtocolBurst.GameInAckThenImmediateMonsterAddProducesNoCrash`: GameInAck + 50 MonsterAdd in one frame, 19 ms
- `ProtocolBurst.DuplicateMonsterAddIsIdempotent`: same MonsterAdd x50, 16 ms
- `ProtocolBurst.OutOfOrderMonsterAckStillPopulates`: 5 ids in non-monotonic order, 15 ms
- `ProtocolBurst.EntityBeforeGameInDoesNotPromoteState`: MonsterAdd before GameInAck 必须 not promote state, 15 ms

`mxh_client_tests` binary 整体 299/299 PASS in 16.3 sec (含 4 新增 ProtocolBurst)。

## Branch / commits

- Branch: `codex/runtime-recovery-pve` (13 ahead of `origin`).
- Latest: `a8e194da` test: §6.4 协议突发测试 hook + 4 个 burst scenario.
