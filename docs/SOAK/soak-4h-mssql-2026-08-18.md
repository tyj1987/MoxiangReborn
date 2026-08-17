# M6-B Canary: 4h mssql_odbc — 2026-08-18

> Status: **PENDING** — gate not yet exercised. Plan: docs/SOAK/soak-4h-mssql.md.

## Run Command

```powershell
powershell -File C:\moxiang\scripts\soak-24h.ps1 `
    -DurationHours 4 `
    -Backend mssql_odbc `
    -Concurrency 4 `
    -BuildDir C:\moxiang\modern\build `
    -ReportDir C:\moxiang\modern\build\runtime\soak-4h-mssql-2026-08-18
```

## Pre-Run Checklist

- [ ] LocalDB running: `sqllocaldb start MSSQLLocalDB`
- [ ] Moxiang DB initialized: `sqlcmd -S "(localdb)\MSSQLLocalDB" -i deploy\database\mx_modern_schema_mssql.sql`
- [ ] Modern stack built: `cmake --build modern/build --config Debug`
- [ ] Game servers idle / staged for canary

## Expected Verdict

Per the 1h SQLite canary (11,586 cycles, 0 crashes, handles bounded):
PASS expected for 4h mssql canary.

## Verdict

(TBD — recorded after the 4h run completes.)
