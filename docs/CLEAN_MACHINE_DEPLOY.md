# Clean-Machine Deployment

> Status: 2026-08-18. ROADMAP M6-A GREEN. 1.0 release-readiness gate.

## Purpose

`scripts/clean-deploy.ps1` takes a blank Windows box (Windows Server 2022 or
Windows 10/11) with only PowerShell and Git installed, and bootstraps it into a
fully built + smoke-verified modern server in one command.

This is the §5.E "clean machine deployment" gate from ROADMAP.

## Quick Start

```powershell
PS> powershell -ExecutionPolicy Bypass -File C:\moxiang\scripts\clean-deploy.ps1 -InstallPrereqs
```

After ~15 minutes (depending on VS Build Tools download size), the machine will have:
- Modern stack built into `C:\moxiang\modern\build\`
- 11,922 / 11,922 unit tests passing (M3/M4 + M5 portal)
- PlayDH junction pointing at `墨香【源码配套资源】\PlayDH`
- commercial-smoke verified (Login/Agent/Map + GUI client + BGM + portal)

## Steps Performed

| # | Step | Description |
|---|------|-------------|
| 1 | preflight | admin check, OS, RAM, disk |
| 2 | prereq detect | VS2022, cmake, git, sqlcmd, SqlLocalDB, ODBC 18, VC++ Redist |
| 3 | prereq install | (only with `-InstallPrereqs`) chocolatey + direct download for ODBC 18 + VS Build Tools 2022 |
| 4 | PlayDH junction | `modern/data/PlayDH` -> `<RepoRoot>\墨香【源码配套资源】\PlayDH` |
| 5 | build modern | invokes `cmake --build modern/build --config <Config>` |
| 6 | run ctest | `ctest -C <Config> --test-dir modern/build --output-on-failure` |
| 7 | commercial smoke | invokes `scripts/commercial-smoke.ps1` (includes portal HTTP smoke) |
| 8 | portal smoke | hits `/api/healthz` + `/api/status` + `/` on localhost:8080 |

All steps are idempotent and safe to re-run.

## Parameters

| Parameter | Description |
|-----------|-------------|
| `-DryRun` | Print every step that would run, do not actually mutate |
| `-InstallPrereqs` | Install missing prereqs via Chocolatey (choco) |
| `-SkipTests` | Skip ctest (faster bootstrap verification) |
| `-SkipSmoke` | Skip commercial-smoke |
| `-SkipGui` | Forward -SkipGui to commercial-smoke |
| `-Config` | CMake config (Debug or Release, default Debug) |
| `-RepoRoot` | Override repo root |
| `-Force` | Rebuild from scratch (reset PlayDH junction) |

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | success |
| 1 | preflight failure |
| 2 | prereq missing (rerun with -InstallPrereqs) |
| 3 | build failed |
| 4 | ctest failed |
| 5 | commercial-smoke failed |

## Prerequisite Installation

With `-InstallPrereqs`, the script installs missing tools via chocolatey:
- `choco install vcredist2022 -y`
- `choco install cmake -y --installargs ADD_CMAKE_TO_PATH=System`
- `choco install git -y`
- `choco install sql-server-express -y` (SQL Server 2022 Express, includes LocalDB)
- direct MSI download for `msodbcsql18` (ODBC Driver 18 for SQL Server)
- direct download of `vs_buildtools.exe` for VS 2022 Build Tools (C++ workload)

All installs run silently and require Administrator elevation.

## Next Step After Deployment

Manual start of the server stack:

```powershell
# Legacy stack (pre-built binaries in 墨香【源码】\SWorking):
PS> scripts\start-server.ps1 -Mode start

# OR modern stack (mxh_* binaries from modern\build\bin):
PS> modern\build\bin\Debug\mxh_login_server.exe &
PS> modern\build\bin\Debug\mxh_agent_server_CHINA.exe &
PS> modern\build\bin\Debug\mxh_map_server_CHINA.exe &
```

## Non-Admin Caveats

The script gracefully degrades when not run as Administrator:
- Warns instead of fails on preflight admin check
- Warns when RAM/disk detection requires admin (skips strict check)
- `-InstallPrereqs` will fail without elevation (some installers need it)

For production deployment, always run with Administrator elevation.

## Local Verification (2026-08-18)

Run on the developer's local Windows 11 machine:

```powershell
PS> powershell -File C:\moxiang\scripts\clean-deploy.ps1 -DryRun -SkipTests -SkipSmoke
# Expected exit 0; prints every step without mutating.

PS> powershell -File C:\moxiang\scripts\clean-deploy.ps1 -SkipSmoke
# Expected exit 0; full build + 11,922 / 11,922 ctest PASS, ~120s.

PS> powershell -File C:\moxiang\scripts\clean-deploy.ps1 -InstallPrereqs
# Requires Administrator elevation. Expected exit 0 (~15 min).
```

After the gateway is started, the portal smoke step should hit:

```powershell
PS> curl http://127.0.0.1:8080/api/healthz
{"status":"ok","version":"1.0.0","uptime_seconds":0}
```

## Portal-Specific Notes

The `scripts/commercial-smoke.ps1` step also runs:

```powershell
PS> powershell -File deploy\portal\smoke-ecs.ps1 -PublicUrl http://127.0.0.1:8080/portal
```

Verifying:
- `/api/healthz` -> 200
- `/api/status` -> 200 (Login/Agent/Map up once game servers are running)
- `/` -> 200 (SPA fallback)

If `PORTAL_JWT_SECRET` is unset, the portal exits with code 6 — see
`docs/PORTAL_DEPLOY.md` for the secret bootstrap procedure.

