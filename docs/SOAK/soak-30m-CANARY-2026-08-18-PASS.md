# M6-B Canary: 30-min sqlite — 2026-08-18

> Status: **PASS** — canary verdict cleared after fixing E2E 5-slot limit.

## Result

```
verdict: PASS
duration_hours_actual: 0.5001
backend: sqlite
concurrency: 4
cycle_total:  6961
cycle_ok:     6961
cycle_fail:   0
cycle_success_rate: 1.0
server_crash_observed: false
sample_count: 353
report: C:\moxiang\modern\build\runtime\soak-30m-CANARY-2026-08-18
```

6961 cycles in 30 min (~232 cycles/min) — far exceeds the 1h SQLite baseline
of 11,586 cycles (~193 cycles/min). 100% success rate, 0 server crashes,
353 1Hz samples captured.

## Root cause of previous FAIL_ERROR_RATE

The pre-existing `MoxianClientE2E` B.2.5 harness always issued a fresh
`CharacterMakeSyn` every cycle. After 5 cycles the user filled the 5-slot
limit and the agent rejected subsequent creations with
`CharacterMakeNack received (name taken or invalid params)`.

Fix (`commit 24e938c0`):
1. Skip Step 3 (CharMake) when at least one valid slot exists.
2. Move the valid-slot count BEFORE `chsel.Release()` — the dtor clears
   `m_characters`, so a post-Release count would always be 0.

The earlier 5-min smoke (after the fix) ran 326 cycles with 0 failures.

## Memory + handle samples (30-min window)

Stable across the 30-min run. No monotonic growth, no spikes, no leaks.

## Path to v1.0-rc1

✅ 5-min smoke (326/326 PASS)
✅ 30-min sqlite (6961/6961 PASS)
🟡 4h mssql_odbc canary — pending 4h wall-clock window
🟡 24h full canary — pending 24h wall-clock window

The 30-min canary proves the harness + modern stack can sustain 6.9k+
cycles with 0 failures. The 4h and 24h runs are pure wall-clock extensions
of the same workload. Tag `v1.0-rc1` once both gates complete.
