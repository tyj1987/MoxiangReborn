# M6-B Canary: 24h full

> Status: 1h SQLite canary PASSED (11,586 cycles). 4h mssql_odbc pending.
> 24h full canary is the final stability gate before 1.0 RC.

## Goal

Run `scripts/soak-24h.ps1 -DurationHours 24 -Concurrency 4` for **both**
backends, capturing summary.json for each:

| Backend | Cycles target | Failure budget | Crashes |
|---|---|---|---|
| sqlite (default) | ~277,000 | 20 | 0 |
| mssql_odbc       | ~277,000 | 20 | 0 |

24h × 60min × 60s × 4 concurrent ÷ 5s/cycle ≈ 11,520 cycles per backend;
double that across both backends.

## How to Run

```powershell
# Run 1: SQLite (default backend, fastest path).
powershell -File scripts\soak-24h.ps1 `
    -DurationHours 24 `
    -Backend sqlite `
    -Concurrency 4 `
    -BuildDir modern\build `
    -ReportDir modern\build\runtime\soak-24h-sqlite-2026-XX-XX

# Run 2: mssql_odbc (production-realistic).
powershell -File scripts\soak-24h.ps1 `
    -DurationHours 24 `
    -Backend mssql_odbc `
    -Concurrency 4 `
    -BuildDir modern\build `
    -ReportDir modern\build\runtime\soak-24h-mssql-2026-XX-XX
```

## Expected Output

Per the 1h SQLite baseline:
- `summary.json` shows `verdict: "PASS"`, `crash_observed: false`, memory
  and handle counts bounded.
- `samples.csv` shows 1Hz sampling across 14400 samples (4h) or 86400 (24h).

## Failure Modes

Inherits from `docs/SOAK/soak-4h-mssql.md` plus:
- Disk full → `soak-24h.ps1` writes to `runtime\soak-<runId>`; check disk
  before triggering 24h runs (~5 GB free recommended).
- Server OOM → drop `-Concurrency` to 2; rerun.
- Thermal throttle on laptop → move to desktop-class hardware; check
  `throttle` flag in `samples.csv`.

## Status

- 2026-08-18: 24h full canary — **PENDING**. Requires a 24h window with
  the modern stack built and game servers running.
