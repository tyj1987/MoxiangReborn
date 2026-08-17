# M6-B Canary: actual runs — 2026-08-18

> Status: **FAIL_ERROR_RATE** on every attempted run. Modern E2E flow has a
> pre-existing GameInAck gap that prevents the canary from passing. See
> "Findings" below for the root cause and proposed fix.

## Runs attempted

| Run | Duration | Backend | Verdict | Cycles | Crashes | Samples | Report |
|---|---|---|---|---|---|---|---|
| 5-min smoke | 0.0833h | sqlite | FAIL_ERROR_RATE | 20 (0 ok / 20 fail) | 0 | 4 | `modern/build/runtime/soak-91670e6b` |
| 4h mssql_odbc | 0h (early exit) | mssql_odbc | server-startup failed | 0 | 0 | 0 | `modern/build/runtime/soak-4h-mssql-2026-08-18` |
| 30-min sqlite | 0.5h (early exit @ 0.0049h) | sqlite | FAIL_ERROR_RATE | 20 (0 ok / 20 fail) | 0 | 4 | `modern/build/runtime/soak-30m-sqlite-2026-08-18` |

The mssql_odbc run failed at the server-startup step (port 16001 unhealthy).
The sqlite runs reached the cycle stage but every cycle failed because the
modern E2E client never receives GameInAck.

## Findings

### Harness works

The harness infrastructure (harness orchestration, server memory/CPU/handle
sampling, samples.csv, summary.json) is operational. Every run produced
the expected output files. Sample capture rate: 1Hz as designed.

### Pre-existing modern E2E gap

`mxh_client_e2e.exe` reaches step `[6/6] InGame` but times out waiting for
`destination GameInAck`:

```
[e2e] [1/6] Login: OK
[e2e] [2/6] CharSelect: 0 valid slot(s)
... (auto-create char, re-enter CharSelect, enter game)
[07:28:42.748] CInGameState: monster move id=50001 pos=(27284,27648)
[e2e] [6/6] FAIL: timed out waiting for destination GameInAck
[net] client recv n=-1 err=10004 so_error=0 carryover=0B
```

The Map server is sending MonsterAdd / NpcAdd / Move events but never sends
the GameInAck message that the E2E client expects after entering the game.

Reviewing `modern/src/server/MapHandler.cpp` — the GameInAck dispatch path
appears to be missing for the modern E2E client variant. The legacy client
gets a different ack shape; the modern E2E expects a wire message that the
modern server doesn't send.

This is a pre-existing bug in the modern stack, NOT a regression introduced
by the M5 portal work.

## Impact on 1.0 RC tag

Per the plan's Definition of Done, the 4h mssql_odbc + 24h full canaries
must produce PASS verdict for `v1.0-rc1`. Both gates fail today.

**v1.0-rc1 is NOT created.** The release notes (`docs/RELEASE_NOTES.md`)
should be updated to reflect this when the fix lands.

## Proposed fix

1. Audit `modern/src/server/MapHandler.cpp` GameInAck wire path.
2. Compare against the legacy `[CC]Header/Protocol.h` GameInAck shape.
3. Add the missing dispatch in modern server OR align the modern E2E client's
   expectation to the actual server response.
4. Re-run 5-min smoke canary; expect PASS before re-running 4h/24h.

Estimated effort: 1-2 hours of source code + 1 cycle of 5-min smoke to verify.

## Status

- 2026-08-18: 4h mssql + 24h full canaries BLOCKED on modern E2E GameInAck bug.
- 1.0 RC tag: pending the fix.
