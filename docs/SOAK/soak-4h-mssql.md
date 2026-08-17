# M6-B Canary: 4h mssql_odbc

> Status: harness scaffolded (1h SQLite canary passed at 11,586 cycles).
> 4h mssql_odbc run is the next gate before the 24h full canary.

## Goal

Run `scripts/soak-24h.ps1 -DurationHours 4 -Backend mssql_odbc -Concurrency 4`
against the modern Login/Agent/Map chain backed by SQL Server 2022 (LocalDB
or Express). Verify:
- 0 server crashes
- 0 client harness failures above 20-failure budget
- Memory + handle counts bounded (no monotonic growth)
- Latency P95 within 2x of the SQLite baseline

## How to Run

```powershell
# Pre-flight: ensure LocalDB is up + a clean Moxiang DB exists.
sqllocaldb start MSSQLLocalDB
sqlcmd -S "(localdb)\MSSQLLocalDB" -i deploy\database\mx_modern_schema_mssql.sql

# Build Release for canary (faster + closer to production).
powershell -File scripts\build-modern.ps1 -Config Release

# Start the modern server chain (Login/Agent/Map).
powershell -File deploy\scripts\start_modern.ps1 -Mode start -Locale CHINA

# Start the 4h mssql_odbc canary.
powershell -File scripts\soak-24h.ps1 `
    -DurationHours 4 `
    -Backend mssql_odbc `
    -Concurrency 4 `
    -BuildDir modern\build `
    -ReportDir modern\build\runtime\soak-4h-mssql-2026-XX-XX
```

## Expected Output

The script writes to `$ReportDir`:
- `summary.json` — `{verdict, total_cycles, failed_cycles, crash_observed, samples, ...}`
- `samples.csv` — 1Hz time-series of (pid, rss_mb, cpu_pct, handle_count)

Passing summary.json:
```json
{
  "verdict": "PASS",
  "duration_seconds": 14400,
  "total_cycles": 11520,
  "failed_cycles": 3,
  "crash_observed": false,
  "memory": {
    "login": {"min_mb": 142, "max_mb": 168, "stable": true},
    "agent": {"min_mb": 152, "max_mb": 176, "stable": true},
    "map":   {"min_mb": 150, "max_mb": 161, "stable": true}
  },
  "handles": {
    "login": {"min": 164, "max": 168, "stable": true},
    "agent": {"min": 152, "max": 176, "stable": true},
    "map":   {"min": 150, "max": 161, "stable": true}
  }
}
```

## Failure Modes

| Symptom | Likely cause | Action |
|---|---|---|
| `crash_observed: true` | Server segfault | Add `--gtest_filter` to focus next run; file bug |
| `failed_cycles > 20` | Server overload or DB bottleneck | Drop `-Concurrency` to 2; check ODBC data source |
| `memory.login.max_mb > 500` | Login server leak | Run UMDH / heap diff; likely object pool churn |
| `handles.login.max - min > 1000` | Handle leak | Same as above — track recent MapHandler changes |

## Status

- 2026-08-12: 1h SQLite canary PASSED (11,586 cycles, 0 crashes, handles bounded).
- 2026-08-18: 4h mssql_odbc — PENDING. Run command above; copy summary.json
  into `docs/SOAK/soak-4h-mssql-<date>.json` and append a verdict to this file.
