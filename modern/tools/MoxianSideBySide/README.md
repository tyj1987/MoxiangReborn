# MoxianSideBySide

Headless side-by-side verification harness for the Moxiang (墨香) modern
re-implementation. Drives a deterministic packet sequence against the modern
Login/Agent/Map servers (and optionally the legacy `SWorking/*` server when
present), then diffs the responses byte-by-byte. Exits 0 when the modern
traces match the expected modern golden, and (with `--legacy-*` arguments)
match the legacy golden trace for the same scenario.

Built as part of the M3 "T3 五段行为对比" milestone. See
[`docs/SIDE_BY_SIDE_T3.md`](../../../docs/SIDE_BY_SIDE_T3.md) for the full
acceptance workflow (login, enter_game, attack, shop, quest).

## Quick start (modern-only)

```powershell
# 1) Build the modern servers (Debug).
cmake --build C:\moxiang\modern\build --config Debug --target mxh_login_server mxh_agent_server mxh_map_server

# 2) Run the 5 scenarios end-to-end. The tool starts the three modern servers
#    in --start mode, runs each scenario, and writes a *.cap file per scenario
#    under --capture-dir.
Get-Process -Name "mxh_*" -ErrorAction SilentlyContinue | Stop-Process -Force
C:\moxiang\modern\build\tools\MoxianSideBySide\Debug\mxh_side_by_side.exe `
    --modern-only --modern-legacy --modern-agent-port 7001 `
    --capture-dir C:\moxiang\modern\scratch\sbs_captures `
    --modern-server-dir C:\moxiang\modern\build\tools `
    --start --timeout 5
```

After the run, `sbs_captures/` contains 5 golden captures
(`modern_login.cap`, `modern_enter_game.cap`, `modern_attack.cap`,
`modern_shop.cap`, `modern_quest.cap`). These captures are checked in
under `modern/tests/fixtures/sbs_captures_modern/` and locked by the
`SideBySideModernGolden.*` tests in
`modern/tests/unit/tools_side_by_side_test.cpp`.

## Scenarios

| Name          | Server  | Wire (cat, proto, payload)                                                                                          | Expected modern reply                                                              |
| ------------- | ------- | ------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| `login`       | Login   | UserConn.LoginSyn (cat=7, proto=0, payload=[id: 17B][pw: 17B])                                                       | UserConn.DistConnectSuccess + UserConn.NotifyUserLoginAck                          |
| `enter_game`  | Agent   | UserConn.CharacterSelectSyn (cat=7, proto=16) + UserConn.GameInSyn (cat=7, proto=28)                                | UserConn.AgentConnectSuccess + UserConn.GameInNack (no character selected yet)     |
| `attack`      | Map     | Skill.StartSyn (cat=22, proto=0, payload=[skill_idx:u32][main_target:u32][target_x:f32][target_z:f32])              | Skill.StartNack (err=3 unknown caster, no caster manager yet)                      |
| `shop`        | Map     | Item.BuySyn (cat=5, proto=22, payload=[item:u16][qty:u16])                                                          | Item.BuyNack (4B payload echo, no NPC shop table yet)                              |
| `quest`       | Map     | Quest.StartSyn (cat=39, proto=9, payload=[quest_id:u16])                                                            | Quest.StartNack (2B quest_id echo, no modern quest manager yet)                    |

The `attack` / `shop` / `quest` replies are stable Nacks by design: the modern
code does not yet have caster / NPC shop / quest manager modules, but the
server must always answer `StartSyn` per the legacy client contract. The
golden captures lock this wire shape so the side-by-side harness can keep
diffing against a known-good modern trace while the underlying modules
land. When each module ships, the expected reply will upgrade to the
proper Ack and the goldens will be refreshed.

## CLI flags

```
mxh_side_by_side [options]

  --scenario NAME            One of login|enter_game|attack|shop|quest|all
                             (default: all).
  --modern-only              Only drive the modern servers, skip the legacy
                             side. (Used for modern golden capture.)
  --modern-legacy            Drive the modern servers in legacy-compatible
                             mode (default modern server config).
  --start                    Launch the three modern servers
                             (mxh_login_server / mxh_agent_server /
                             mxh_map_server) from --modern-server-dir
                             before running the scenarios. When omitted, the
                             tool assumes the servers are already running.
  --modern-server-dir DIR    Directory that contains the three
                             mxh_*_server executables (default: tools/).
  --modern-port N            Login port (default 16001).
  --modern-agent-port N      Agent port (default 17001).
  --modern-map-port N        Map port (default 18001).
  --capture-dir DIR          Where to write *.cap files (default
                             modern/scratch/sbs_captures/).
  --timeout N                Per-scenario timeout in seconds (default 10).
  --ignore-trace-length      Diff content-only; do not require identical
                             packet counts. Useful when the legacy server
                             has timing jitter.
  --legacy-exe PATH          Optional: path to the legacy server exe. When
                             present the tool also runs the legacy side and
                             diffs the modern reply against the legacy one.
  --legacy-port N            Legacy Login/Map port (default 6001).
  --legacy-agent-port N      Legacy Agent port (default 7001).

Exit code 0 on success, 1 on any diff. The tool prints a per-scenario
summary to stdout, e.g.:

```
  [sbs] login       ok    2 frames, diff=0
  [sbs] enter_game  ok    2 frames, diff=0
  [sbs] attack      ok    1 frames, diff=0
  [sbs] shop        ok    1 frames, diff=0
  [sbs] quest       ok    1 frames, diff=0
```

## Layout

```
modern/tools/MoxianSideBySide/
├── main.cpp                # CLI entry + scenario dispatcher
├── packet.{hpp,cpp}        # Packet struct + hex encode/decode
├── replay/
│   └── replay.{hpp,cpp}  # 5 scenarios + wire contract (locked)
├── capture/
│   └── packet_capture.{hpp,cpp}  # *.cap writer (text, one frame per line)
└── diff/
    └── packet_diff.{hpp,cpp}   # Frame-by-frame category / protocol / payload diff
```

Capture format (`*.cap`) is one frame per line:

```
<cat> <proto> <object_id> <hex_payload>
```

e.g.:

```
7 0 1000
7 2 1000
```

Captures are diffed by category + protocol + object_id + payload bytes in
order; payload bytes that differ only in jitter (e.g. timestamps) can be
masked with `--ignore-trace-length` (we still compare the trace per-frame
but tolerate a single-frame timing offset).

## Updating the modern goldens

The five modern captures in
`modern/tests/fixtures/sbs_captures_modern/` are the locked-in modern
golden. To refresh them when a Nack becomes an Ack (e.g. caster manager
lands and `attack` now returns `Skill.StartAck`):

1. Update `modern/src/server/map_handler.cpp` so the modern reply
   matches the legacy wire shape byte-for-byte.
2. Re-run the harness with `--modern-only --start` and a fresh
   `--capture-dir` (e.g. `modern/scratch/sbs_captures_next/`).
3. Diff the new captures against the existing golden with
   `git diff -- modern/tests/fixtures/sbs_captures_modern/`; every
   change must be explained by a corresponding code change in
   `modern/src/server/` or `modern/include/mxh/proto/`.
4. Update the matching `SideBySideModernGolden.*` test in
   `modern/tests/unit/tools_side_by_side_test.cpp` to assert the new
   category / protocol / payload.
5. Run `ctest -C Debug --test-dir modern/build --timeout 120
   -R SideBySide` to confirm the locked-in golden matches the modern
   runtime reply.

## Acceptance gate

`scripts/commercial-smoke.ps1` runs the 5-stage harness as part of the
RC gate. Failure exits non-zero. The `SideBySideModernGolden.*` unit
tests run in `ctest` and lock the goldens against drift.

