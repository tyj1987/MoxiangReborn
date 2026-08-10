# T3 5-Stage Side-by-Side Acceptance Workflow

> Status: **modern 5/5 diff=0 against the locked-in modern golden**
> (2026-08-10). Cross-implementation evidence (modern vs legacy golden)
> is the next gate; it requires the legacy `SWorking/*` server to be
> runnable on the same machine. The RC gate (`scripts/commercial-smoke.ps1`)
> is **GREEN** for the modern side today.

This document is the M3 acceptance checklist for the T3 ("五段核心玩法行为一致")
milestone. It captures the five fixed scenarios, the modern + legacy
comparison pipeline, the locked-in modern goldens, and the steps required
to refresh a golden when an underlying manager (caster / NPC shop / quest)
lands in the modern code.

See [`modern/tools/MoxianSideBySide/README.md`](../modern/tools/MoxianSideBySide/README.md)
for the harness CLI and scenario wire contract.

## Scenarios

| # | Name          | Server  | What it exercises                                                | Wire contract reference                                             |
|---|---------------|---------|------------------------------------------------------------------|---------------------------------------------------------------------|
| 1 | `login`       | Login   | Auth round-trip + connect-success + login-ack                    | `replay.cpp::login_scenario`                                       |
| 2 | `enter_game`  | Agent   | Character select + game-in                                       | `replay.cpp::enter_game_scenario`                                   |
| 3 | `attack`      | Map     | Skill start (caster resolution -> start-ack)                      | `replay.cpp::attack_scenario`                                       |
| 4 | `shop`        | Map     | NPC shop buy (catalog + money)                                   | `replay.cpp::shop_scenario`                                         |
| 5 | `quest`       | Map     | Quest start (manager -> start-ack with quest record)              | `replay.cpp::quest_scenario`                                        |

All five scenarios are deterministic: the client packet is fixed by the
scenario definition, and the server reply depends only on the modern
server state. There is no random seed and no wall-clock dependency
in the wire shape.

## Modern golden (locked-in 2026-08-10)

The five captures checked in under `modern/tests/fixtures/sbs_captures_modern/`
are the locked-in modern golden. They are produced by running the
harness with `--modern-only --start` against the modern Login/Agent/Map
servers and then copied into the fixture directory.

| File                      | Server frames (cat, proto)            | Notes                                                          |
|---------------------------|---------------------------------------|----------------------------------------------------------------|
| `modern_login.cap`        | (7, 0)  + (7, 2)                      | DistConnectSuccess + NotifyUserLoginAck; byte-for-byte golden |
| `modern_enter_game.cap`   | (7, 8)  + (7, 18)                     | AgentConnectSuccess + GameInNack (no character selected yet)   |
| `modern_attack.cap`       | (22, 2)                               | Skill StartNack (err=3 unknown caster)                          |
| `modern_shop.cap`         | (5, 24)                               | Item BuyNack (4B payload echo)                                 |
| `modern_quest.cap`        | (39, 11)                              | Quest StartNack (2B quest_id echo)                             |

All five are diffed by category + protocol + object_id + payload bytes.
Per-frame `SideBySideModernGolden.*` unit tests in
`modern/tests/unit/tools_side_by_side_test.cpp` lock each one against
regression.

## End-to-end reproduction (modern-only)

```powershell
# 1) Clean up any leftover modern servers.
Get-Process -Name "mxh_*" -ErrorAction SilentlyContinue | Stop-Process -Force

# 2) Build the three modern servers + the harness (Debug).
cmake --build C:\moxiang\modern\build --config Debug --target `
    mxh_login_server mxh_agent_server mxh_map_server mxh_side_by_side

# 3) Run the 5 scenarios end-to-end (--start launches the three servers,
#    --timeout 5 caps each scenario at 5 seconds).
C:\moxiang\modern\build\tools\MoxianSideBySide\Debug\mxh_side_by_side.exe `
    --modern-only --modern-legacy --modern-agent-port 7001 `
    --capture-dir C:\moxiang\modern\scratch\sbs_captures `
    --modern-server-dir C:\moxiang\modern\build\tools `
    --start --timeout 5

# 4) Verify the captures match the locked-in golden.
Get-ChildItem C:\moxiang\modern\scratch\sbs_captures
foreach ($f in @("modern_login", "modern_enter_game", "modern_attack", "modern_shop", "modern_quest")) {
    $a = Get-Content -LiteralPath "C:\moxiang\modern\scratch\sbs_captures\$f.cap" -Encoding UTF8
    $b = Get-Content -LiteralPath "C:\moxiang\modern\tests\fixtures\sbs_captures_modern\$f.cap" -Encoding UTF8
    if (Compare-Object $a $b) { Write-Error "MISMATCH: $f" } else { Write-Output "OK: $f" }
}

# 5) Run the unit-test goldens.
ctest -C Debug --test-dir C:\moxiang\modern\build --timeout 120 -R SideBySideModernGolden -V
```

Expected: 5x `OK: ...` from step 4 and 6/6 tests passing in step 5.

## Cross-implementation evidence (next gate)

The modern goldens above prove the modern side is deterministic and
matches its locked-in wire contract. The next gate is to prove the
modern reply matches the legacy reply **for the same client packet**.
That requires the legacy `SWorking/*` server to be runnable on the
same machine; it is currently sitting in `SWorking/` and is referenced
from `scripts/start-server.ps1`.

When the legacy side is available, the workflow is:

```powershell
# A) Produce a legacy golden by replaying the same scenario against the
#    legacy server (start it via scripts/start-server.ps1 first).
C:\moxiang\modern\build\tools\MoxianSideBySide\Debug\mxh_side_by_side.exe `
    --scenario attack --legacy-exe C:\moxiang\SWorking\Distribute.exe `
    --legacy-port 6001 --capture-dir C:\moxiang\modern\scratch\sbs_captures_legacy `
    --timeout 5

# B) Compare the modern + legacy captures (manually or with the harness's
#    diff module). For each frame we expect category + protocol + payload
#    to be byte-equal; object_id and timing may differ.
```

For each scenario the diff target is:

- `login`: 2 frames, both byte-equal (legacy + modern use the same
  `MP_USERCONN_*` constants and the same 17B id + 17B pw shape).
- `enter_game`: 2 frames. The first (`AgentConnectSuccess`) is byte-equal;
  the second (`GameInAck` vs `GameInNack`) differs because the modern
  harness does not have a character pre-selected. The diff target is
  "category=7 protocol=8 then category=7" -- the second frame protocol
  may legitimately differ until the modern client E2E test pre-selects
  a character.
- `attack`: 1 frame. The diff target is "category=22 protocol=Ack (modern)
  with payload [damage_actual: u32]". The current modern golden is a
  Nack (no caster manager); the legacy target is an Ack with a small
  damage value. The harness will return diff=1 until the caster manager
  lands; the modern golden will then be refreshed.
- `shop`: 1 frame. The diff target is "category=5 protocol=Ack (modern)
  with payload [item: u16][qty: u16]". The current modern golden is a
  Nack (no NPC shop); the legacy target is an Ack. The harness will
  return diff=1 until the NPC shop table lands; the modern golden will
  then be refreshed.
- `quest`: 1 frame. Same pattern: the legacy target is `Quest.StartAck`
  with a quest record; the modern target is currently `Quest.StartNack`.
  When `modern/src/server/quest_manager.cpp` lands the golden will be
  refreshed.

## Refreshing a modern golden

When an underlying manager lands and the modern reply changes from
Nack to Ack:

1. Land the manager (e.g. `quest_manager.cpp` for the quest scenario).
2. Run the harness end-to-end with `--modern-only --start` and a fresh
   capture dir.
3. `git diff -- modern/tests/fixtures/sbs_captures_modern/` should show
   exactly the change you expect (one frame, same length or longer,
   category + protocol matching the legacy wire shape).
4. Update the matching `SideBySideModernGolden.*` test to assert the
   new category / protocol / payload bytes.
5. Re-run `ctest -C Debug --test-dir modern/build --timeout 120
   -R SideBySideModernGolden` and confirm the test passes.
6. Land the manager + golden + test in the same commit series:
   - `server: <manager> Ack with <payload>`
   - `tests: refresh modern golden for <scenario>`
   - `docs: update SIDE_BY_SIDE_T3.md modern golden row`

## Acceptance status (D-stage)

The D-stage acceptance criterion from [`ROADMAP.md`](../ROADMAP.md)
section 5 is: "五段玩法 side-by-side 的副作用、数值和数据库 diff=0".
The current status:

- **Network packets (cat / proto / payload)**: 5/5 scenarios diff=0 against
  the locked-in modern golden. (Locked by `SideBySideModernGolden.*` tests.)
- **Side-effect order**: 0/5 scenarios have side-effect order diff=0 today
  because the modern Nack handlers do not mutate world state. This is the
  next gate: the underlying managers (caster, NPC shop, quest) need to
  land to produce Ack replies, then the harness can compare mutation
  order against the legacy trace.
- **Numerical diff**: same as side-effect order. The modern Nacks carry no
  numerical payload; the diff target is the legacy Ack's numbers. The
  acceptance gate is the manager landing + golden refresh.
- **Database state diff**: requires the modern MSSQL backend to be running
  and the legacy `.bak` to be restorable. Currently the modern code reads
  from the LocalDB / MSSQL schema (see `modern/tests/unit/db/mssql_real_e2e_test.cpp`)
  but the side-by-side harness does not snapshot the DB before/after each
  scenario. That is the E-stage deliverable: a `pre_scenario.sql` /
  `post_scenario.sql` snapshot diff against the legacy `Moxian` database.

## Open items for the next session

1. `modern/src/server/quest_manager.cpp` -- produce `Quest.StartAck` with
   quest record (5-stage diff=1 -> 0).
2. `modern/src/server/npc_shop.cpp` -- produce `Item.BuyAck` with money
   delta (5-stage diff=1 -> 0).
3. `modern/src/server/skill_caster.cpp` -- resolve caster by object_id and
   produce `Skill.StartAck` with `damage_actual: u32` (5-stage diff=1 -> 0).
4. `modern/tools/MoxianSideBySide/sbs_db_diff.cpp` -- DB before/after
   snapshot diff for the 5 scenarios (E-stage deliverable).
5. Cross-implementation runbook once the legacy `SWorking/*` is back up
   on the verification host (see "Cross-implementation evidence" above).

